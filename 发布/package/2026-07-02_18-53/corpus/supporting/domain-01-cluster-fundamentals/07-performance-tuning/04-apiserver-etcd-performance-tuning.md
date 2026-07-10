---
title: API Server 与 etcd 性能调优
description: 'APF 配置、请求超时限流、etcd 磁盘 I/O 优化、compaction 策略、碎片整理、Watch 缓存调优及性能基线测试方法'
summary: 'APF 配置、请求超时限流、etcd 磁盘 I/O 优化、compaction 策略、碎片整理、Watch 缓存调优及性能基线测试方法'
category: cluster-fundamentals
tags:
- apiserver
- etcd
- performance-tuning
- apf
- rate-limiting
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- API Server 性能调优 是什么
- 如何优化 etcd 性能
- APF Priority and Fairness 如何配置
trigger_keywords:
- apiserver 性能
- etcd 调优
- APF
- Priority and Fairness
- etcd defrag
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# API Server 与 etcd 性能调优

## 概述

API Server 和 etcd 是 Kubernetes 控制平面的核心瓶颈点。大规模集群（>1000 节点）下，请求排队、etcd 延迟飙升、Watch 风暴等问题频发。本文档覆盖 APF 限流、请求超时、etcd 磁盘优化、compaction 策略等关键调优手段。

```
性能瓶颈定位路径:

API 请求慢？
  ├─ API Server 请求排队 → APF 配置不当 / 限流过严
  ├─ etcd 延迟高 → 磁盘 I/O / 碎片化 / compaction 积压
  ├─ Watch 风暴 → Watch 缓存不足 / 客户端重连
  └─ CPU / 内存不足 → 资源限制过低

整体性能基线:
  etcd 写延迟 (p99): < 10ms (SSD)
  API Server 请求延迟 (p99): < 1s (非 LIST)
  etcd 数据大小: < 8GB (推荐 < 2GB)
  etcd compaction 延迟: < 5 分钟
```

## 1. API Priority and Fairness (APF)

### 1.1 APF 架构

APF（1.20+ GA）将 API 请求分类到不同的 FlowSchema，每个 FlowSchema 关联一个 PriorityLevel，通过令牌桶和排队机制控制并发。

```
请求流经路径:

Client Request
  → FlowSchema 匹配（基于用户/资源/动词）
    → PriorityLevel 分配
      → 令牌桶限流
        → 排队（如果桶满）
          → 并发执行
```

### 1.2 查看当前 APF 状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 FlowSchema
kubectl get flowschemas

# 查看所有 PriorityLevelConfiguration
kubectl get prioritylevelconfigurations

# 查看 APF 的实际执行状态
kubectl get --raw /metrics | grep apiserver_flowcontrol

# 查看特定 PriorityLevel 的请求排队情况
kubectl get --raw /metrics | grep 'apiserver_flowcontrol_request_concurrency_limit'

# 查看被拒绝的请求
kubectl get --raw /metrics | grep 'apiserver_flowcontrol_rejected_requests_total'

# 查看当前排队情况
kubectl get --raw /metrics | grep 'apiserver_flowcontrol_current_inqueue_requests'
```
### 1.3 自定义 FlowSchema

```yaml
# 为 CronJob 控制器创建专用 FlowSchema，避免被其他请求挤占
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: cronjob-controller
spec:
  priorityLevelConfiguration:
    name: cronjob-controller-plc
  rules:
  - subjects:
    - kind: ServiceAccount
      serviceAccount:
        name: cronjob-controller
        namespace: kube-system
    resourceRules:
    - verbs: ["get", "list", "watch", "update", "create"]
      apiGroups: ["batch"]
      resources: ["cronjobs", "jobs"]
      namespaces: ["default"]
---
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: cronjob-controller-plc
spec:
  type: Limited
  limited:
    assuredConcurrencyShares: 5
    limitResponse:
      type: Queue
      queuing:
        queues: 16
        handSize: 4
        queueLengthLimit: 50
```

### 1.4 APF 调优参数

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# API Server 启动参数调整
--max-requests-inflight=400        # 非 mutating 请求并发上限（默认 400）
--max-mutating-requests-inflight=200  # mutating 请求并发上限（默认 200）

# APF 相关 feature gates（1.29+ 默认全开）
--feature-gates=APIPriorityAndFairness=true

# 查看当前配置
kubectl get kube-apiserver -n kube-system -o yaml | grep -E "max-requests|max-mutating"
```
```yaml
# 全局 APF 优化（大规模集群）
# kube-apiserver 启动参数:
--max-requests-inflight=800
--max-mutating-requests-inflight=400
--request-timeout=60s
--min-request-timeout=300
```

### 1.5 APF 故障排查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 问题：APF 限流导致请求被拒绝（429 Too Many Requests）

# 1. 查看哪个 PriorityLevel 被限流
kubectl get --raw /metrics | grep 'apiserver_flowcontrol_rejected_requests_total'

# 2. 查看并发限制
kubectl get --raw /metrics | grep 'apiserver_flowcontrol_request_concurrency_limit'

# 3. 查看排队长度
kubectl get --raw /metrics | grep 'apiserver_flowcontrol_request_queue_length_after_enqueue'

# 4. 临时提高并发限制（紧急情况）
kubectl patch prioritylevelconfigurations workload-low \
  --type merge \
  -p '{"spec":{"limited":{"assuredConcurrencyShares":10}}}'

# 5. 为关键服务创建独立 FlowSchema
# （参考 1.3 节的示例）
```
## 2. API Server 请求超时与限流

### 2.1 请求超时配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kube-apiserver 超时参数
--request-timeout=60s               # 全局请求超时（默认 60s）
--min-request-timeout=240           # 最小请求超时（Watch 用，默认 240s）

# 诊断请求超时
# 查看 API Server 日志中的超时请求
kubectl logs -n kube-system -l component=kube-apiserver \
  --tail=100 | grep -i "timeout\|deadline"

# 查看 API Server 请求延迟分布
kubectl get --raw /metrics | grep apiserver_request_duration_seconds_bucket
```
### 2.2 客户端限流

```yaml
# kubeconfig 中的客户端限流配置
apiVersion: v1
kind: Config
clusters:
- cluster:
    server: https://apiserver:6443
    # 客户端限流
    # 由 client-go 自动处理，通常不需要手动调整
users:
- name: my-user
  user:
    # QPS 和 Burst 由 client-go 控制
    # 默认 QPS=5, Burst=10
```

```go
// Go client-go 限流配置示例
config, _ := rest.InClusterConfig()
config.QPS = 50      // 每秒请求数
config.Burst = 100   // 突发请求数
clientset, _ := kubernetes.NewForConfig(config)
```

## 3. etcd 磁盘 I/O 优化

### 3.1 etcd 磁盘 I/O 基线测试

```bash
# 使用 fio 测试 etcd 数据目录的磁盘性能
# etcd 对随机写延迟非常敏感

# 顺序写测试
fio --name=etcd-seq-write \
  --directory=/var/lib/etcd \
  --rw=write \
  --bs=4K \
  --size=1G \
  --numjobs=1 \
  --runtime=30 \
  --time_based \
  --group_reporting

# 随机写测试（最关键的指标）
fio --name=etcd-rand-write \
  --directory=/var/lib/etcd \
  --rw=randwrite \
  --bs=4K \
  --size=1G \
  --numjobs=1 \
  --runtime=30 \
  --time_based \
  --group_reporting \
  --ioengine=libaio \
  --iodepth=1

# 顺序读测试
fio --name=etcd-seq-read \
  --directory=/var/lib/etcd \
  --rw=read \
  --bs=4K \
  --size=1G \
  --numjobs=1 \
  --runtime=30 \
  --time_based \
  --group_reporting

# 混合读写测试（模拟 etcd 实际负载）
fio --name=etcd-mixed \
  --directory=/var/lib/etcd \
  --rw=randrw \
  --rwmixread=70 \
  --bs=4K \
  --size=1G \
  --numjobs=1 \
  --runtime=30 \
  --time_based \
  --group_reporting \
  --ioengine=libaio \
  --iodepth=1
```

### 3.2 etcd 磁盘性能要求

| 指标 | 推荐值 | 最低要求 |
|------|--------|---------|
| 顺序写吞吐 | > 50 MB/s | > 10 MB/s |
| 随机写 IOPS | > 5000 | > 500 |
| 随机写延迟 (p99) | < 2ms | < 10ms |
| fsync 延迟 (p99) | < 5ms | < 20ms |
| 磁盘类型 | NVMe SSD | SATA SSD |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 etcd 磁盘延迟（从 etcd 内部指标获取）
ETCDCTL_API=3 etcdctl endpoint status \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --write-out=table

# 查看 etcd 磁盘同步延迟
ETCDCTL_API=3 etcdctl endpoint health \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 查看 etcd 后端提交延迟
curl -s https://127.0.0.1:2379/metrics \
  --cacert /etc/kubernetes/pki/etcd/ca.crt \
  --cert /etc/kubernetes/pki/etcd/server.crt \
  --key /etc/kubernetes/pki/etcd/server.key \
  | grep 'etcd_disk_backend_commit_duration_seconds'
```
### 3.3 etcd 数据目录优化

```bash
# 确保 etcd 数据目录在独立磁盘分区上
# 不要与系统盘、日志盘混用

# 推荐的磁盘布局:
# /var/lib/etcd   → NVMe SSD (独立分区)
# /var/log        → 普通 SSD
# /               → 系统盘

# 检查当前挂载
df -h /var/lib/etcd
mount | grep etcd

# 禁用 atime（减少不必要的磁盘写入）
# /etc/fstab:
# /dev/nvme1n1 /var/lib/etcd ext4 defaults,noatime,nodiratime 0 2

# 验证 noatime 生效
mount | grep etcd
```

## 4. etcd Compaction 策略

### 4.1 自动 Compaction 配置

```bash
# etcd 自动 compaction 配置
# 在 etcd 启动参数中设置:

--auto-compaction-mode=periodic    # 按时间周期压缩（默认）
--auto-compaction-retention=1h     # 保留 1 小时的历史数据

# 或者按版本号压缩
--auto-compaction-mode=revision
--auto-compaction-retention=1000   # 保留最近 1000 个版本

# K8s 集群中 etcd 的 compaction 建议值:
# 小型集群 (< 100 节点): --auto-compaction-retention=8h
# 中型集群 (100-1000 节点): --auto-compaction-retention=1h
# 大型集群 (> 1000 节点): --auto-compaction-retention=30m
```

### 4.2 手动 Compaction

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前 etcd 版本号
ETCDCTL_API=3 etcdctl endpoint status \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --write-out=json | jq '.[0].Status.header.revision'

# 手动触发 compaction（压缩到指定版本）
ETCDCTL_API=3 etcdctl compact <revision> \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 查看 compaction 后的数据库大小
ETCDCTL_API=3 etcdctl endpoint status \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --write-out=table
```
## 5. etcd 碎片整理（Defrag）

### 5.1 何时需要 Defrag

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# defrag 会释放磁盘空间，但会阻塞 etcd 几秒到几十秒
# 只在以下情况执行:
# 1. compaction 后 DB 大小仍然很大
# 2. 磁盘空间不足
# 3. 计划维护窗口

# 查看 DB 大小
ETCDCTL_API=3 etcdctl endpoint status \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --write-out=json | jq '.[0].Status.dbSize'

# 如果 DB 大小 > 2GB，建议执行 defrag
```
### 5.2 安全执行 Defrag

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ⚠️ defrag 会阻塞 etcd，生产环境必须逐节点执行
# 且需要确认集群有 3 个以上节点（保证 quorum）

# 步骤 1: 检查集群健康
ETCDCTL_API=3 etcdctl endpoint health \
  --endpoints=https://etcd-1:2379,https://etcd-2:2379,https://etcd-3:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 步骤 2: 对第一个节点执行 defrag
ETCDCTL_API=3 etcdctl defrag \
  --endpoints=https://etcd-1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 步骤 3: 验证第一个节点恢复正常
ETCDCTL_API=3 etcdctl endpoint health \
  --endpoints=https://etcd-1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 步骤 4: 重复步骤 2-3，逐个处理其他节点
# ⚠️ 每个节点 defrag 完成并确认健康后再处理下一个
```
### 5.3 Defrag 自动化脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# etcd-defrag.sh - 逐节点安全 defrag
# 用法: ./etcd-defrag.sh

ENDPOINTS="https://etcd-1:2379,https://etcd-2:2379,https://etcd-3:2379"
CACERT="/etc/kubernetes/pki/etcd/ca.crt"
CERT="/etc/kubernetes/pki/etcd/server.crt"
KEY="/etc/kubernetes/pki/etcd/server.key"

# 解析单个端点
IFS=',' read -ra EPS <<< "$ENDPOINTS"

for ep in "${EPS[@]}"; do
  echo "=== Defragmenting: $ep ==="
  
  # 检查集群健康
  healthy=$(ETCDCTL_API=3 etcdctl endpoint health \
    --endpoints="$ENDPOINTS" \
    --cacert=$CACERT --cert=$CERT --key=$KEY 2>&1 | grep -c "is healthy")
  
  if [ "$healthy" -lt 2 ]; then
    echo "ERROR: Cluster not healthy enough, aborting"
    exit 1
  fi
  
  # 执行 defrag
  ETCDCTL_API=3 etcdctl defrag \
    --endpoints="$ep" \
    --cacert=$CACERT --cert=$CERT --key=$KEY
  
  # 等待节点恢复
  sleep 5
  
  # 验证节点健康
  ETCDCTL_API=3 etcdctl endpoint health \
    --endpoints="$ep" \
    --cacert=$CACERT --cert=$CERT --key=$KEY
  
  echo "=== $ep defrag completed ==="
done
```
## 6. etcd Watch 缓存调优

### 6.1 API Server Watch 缓存配置

```bash
# kube-apiserver Watch 缓存相关参数
--watch-cache=true                         # 启用 Watch 缓存（默认 true）
--default-watch-cache-size=100             # 默认 Watch 缓存大小
--watch-cache-sizes=pods#500,nodes#1000    # 按资源类型设置缓存大小

# 大规模集群推荐配置:
--watch-cache=true
--watch-cache-sizes=pods#1000,nodes#2000,events#500,replicasets#500,deployments#500
--default-watch-cache-size=200
```

### 6.2 Watch 风暴排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Watch 风暴: 大量客户端同时 Watch 同一类资源，导致 API Server 过载

# 检查当前 Watch 连接数
kubectl get --raw /metrics | grep apiserver_current_inflight_requests

# 检查 Watch 事件速率
kubectl get --raw /metrics | grep watch_events_total

# 检查 Watch 缓存命中率
kubectl get --raw /metrics | grep watch_cache_hit_total
kubectl get --raw /metrics | grep watch_cache_miss_total

# 如果命中率低于 80%，考虑增加缓存大小
```
### 6.3 etcd Watch 流控制

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# etcd 流控制参数
--max-concurrent-streams=100    # 最大并发 gRPC 流（默认 1000）
--quota-backend-bytes=8589934592  # etcd 后端存储限额（默认 8GB）

# 检查 etcd Watch 连接数
ETCDCTL_API=3 etcdctl endpoint status \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --write-out=json | jq '.[0].Status'

# 查看 etcd gRPC 连接指标
curl -s https://127.0.0.1:2379/metrics \
  --cacert /etc/kubernetes/pki/etcd/ca.crt \
  --cert /etc/kubernetes/pki/etcd/server.crt \
  --key /etc/kubernetes/pki/etcd/server.key \
  | grep 'etcd_server_streams_total'
```
## 7. 性能基线测试方法

### 7.1 etcd 性能基准测试

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 etcdctl check perf 测试 etcd 性能
ETCDCTL_API=3 etcdctl check perf \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 使用 benchmark 工具（需安装 etcd benchmark）
# 写性能测试
benchmark --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  put --total=10000 --key-size=8 --val-size=256

# 读性能测试
benchmark --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  range key0 key99999 --total=10000
```
### 7.2 API Server 性能基准测试

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 kubectl 进行简单的性能测试
# LIST 所有 Pod 的延迟
time kubectl get pods --all-namespaces -o wide > /dev/null

# LIST 所有节点的延迟
time kubectl get nodes -o wide > /dev/null

# Watch 事件速率测试
kubectl get events --all-namespaces --watch-only &
sleep 60
kill %1

# 使用 hey 进行 HTTP 负载测试
# 获取 API Server 地址和 token
APISERVER=$(kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}')
TOKEN=$(kubectl get secrets -n default -o jsonpath="{.items[0].data.token}" | base64 -d)

# 负载测试 GET 请求
hey -n 1000 -c 10 \
  -H "Authorization: Bearer $TOKEN" \
  -H "Accept: application/json" \
  "$APISERVER/api/v1/namespaces/default/pods"

# 使用 kube-burner 进行大规模测试（推荐）
# 安装: go install github.com/cloud-bulldozer/kube-burner@latest
kube-burner init --config=cluster-density.yml
```
### 7.3 持续监控指标

```bash
# 关键监控指标（Prometheus 格式）

# API Server 请求延迟 (p99)
apiserver_request_duration_seconds{verb="LIST",resource="pods"} 

# etcd 写延迟 (p99)
etcd_disk_wal_fsync_duration_seconds_bucket

# etcd 提交延迟 (p99)
etcd_disk_backend_commit_duration_seconds_bucket

# API Server 当前 inflight 请求数
apiserver_current_inflight_requests

# etcd 已使用 DB 大小
etcd_mvcc_db_total_size_in_bytes

# API Server APF 被拒绝请求数
apiserver_flowcontrol_rejected_requests_total

# etcd Leader 切换次数
etcd_server_leader_changes_seen_total
```

---

## Related

- domain-01-cluster-fundamentals/03-control-plane/
- [[domain-17-system-foundation/速查卡/k8s.md|K8s 速查卡]]
- [[domain-01-cluster-fundamentals/性能调优/19-cluster-performance-tuning|集群性能调优]]

## See Also

- [Kubernetes 官方文档: API Priority and Fairness](https://kubernetes.io/docs/concepts/cluster-administration/flow-control/)
- [etcd 官方文档: Performance](https://etcd.io/docs/latest/op-guide/performance/)
- [etcd 官方文档: Maintenance](https://etcd.io/docs/latest/op-guide/maintenance/)


<!-- risk-assessed -->
