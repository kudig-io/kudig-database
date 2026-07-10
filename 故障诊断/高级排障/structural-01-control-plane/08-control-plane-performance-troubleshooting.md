---
title: 控制平面性能瓶颈分析与优化指南 [topic-structural-trouble-shooting]
description: 'title: 控制平面性能瓶颈分析与优化指南'
summary: 'title: 控制平面性能瓶颈分析与优化指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- performance
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- docker
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- 控制平面性能瓶颈分析与优化指南 是什么
- 如何 控制平面性能瓶颈分析与优化指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 控制平面性能瓶颈分析与优化指南 故障排查
- 控制平面性能瓶颈分析与优化指南 排障步骤
trigger_keywords:
- 控制平面性能瓶颈分析与优化指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 控制平面性能瓶颈分析与优化指南
description: '# 控制平面性能瓶颈分析与优化指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- scheduler
- controller-manager
- [[Prometheus|prometheus]]
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 控制平面性能瓶颈分析与优化指南 是什么
- 如何 控制平面性能瓶颈分析与优化指南
- 控制平面性能瓶颈分析与优化指南 故障排查
- 控制平面性能瓶颈分析与优化指南 排障步骤
trigger_keywords:
- 控制平面性能瓶颈分析与优化指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 控制平面性能瓶颈分析与优化指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 生产环境性能优化实战

## 问题现象与影响分析

### 常见性能瓶颈现象

| 问题现象 | 典型指标 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| API Server 响应延迟高 | `apiserver_request_duration_seconds > 1s` | ⭐⭐⭐ 高 | P0 |
| etcd 读写延迟增加 | `etcd_disk_wal_fsync_duration_seconds > 100ms` | ⭐⭐⭐ 高 | P0 |
| Scheduler 调度延迟 | `scheduler_e2e_scheduling_duration_seconds > 5s` | ⭐⭐ 中 | P1 |
| Controller Manager 同步慢 | `workqueue_depth > 1000` | ⭐⭐ 中 | P1 |
| 控制平面 CPU/Memory 使用率过高 | `process_cpu_seconds_total > 80%` | ⭐⭐⭐ 高 | P0 |
| 对象数量过多导致性能下降 | `apiserver_storage_objects > 10000` | ⭐⭐ 中 | P1 |

### 性能监控指标查看

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# API Server 性能指标
kubectl get --raw /metrics | grep apiserver_request_duration_seconds

# etcd 性能指标
kubectl get --raw /metrics | grep etcd_disk_wal_fsync_duration_seconds

# Scheduler 性能指标
kubectl get --raw /metrics | grep scheduler_e2e_scheduling_duration_seconds

# Controller Manager 指标
kubectl get --raw /metrics | grep workqueue_depth

# 资源使用情况
kubectl top nodes
kubectl top pods -n kube-system
```
## 排查方法与步骤

### 诊断原理说明

控制平面性能瓶颈通常来源于以下几个方面：

1. **API Server 层面**：
   - 请求处理能力不足
   - 对象序列化/反序列化开销
   - 鉴权/授权处理延迟
   - Watch 机制资源消耗

2. **etcd 层面**：
   - 磁盘 I/O 性能瓶颈
   - 网络延迟影响
   - 数据库大小增长
   - 压缩/碎片整理不及时

3. **组件层面**：
   - 控制器工作队列积压
   - 调度算法复杂度过高
   - 缓存同步延迟

### 性能诊断决策树

```
性能问题发现
    ├── API Server 性能分析
    │   ├── 请求延迟分布
    │   ├── QPS 统计分析
    │   ├── 资源对象数量
    │   └── 连接数限制
    ├── etcd 性能分析
    │   ├── WAL fsync 延迟
    │   ├── 磁盘 I/O 性能
    │   ├── 数据库大小
    │   └── 网络延迟
    ├── 组件性能分析
    │   ├── 工作队列深度
    │   ├── 控制器同步延迟
    │   ├── 调度器评分时间
    │   └── 缓存命中率
    └── 系统资源分析
        ├── CPU 使用率
        ├── 内存使用情况
        ├── 磁盘空间占用
        └── 网络带宽使用
```

### 详细诊断命令

#### 1. API Server 性能诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# API Server 性能诊断脚本

echo "=== API Server 性能诊断 ==="

# 获取 API Server 指标
APISERVER_METRICS=$(kubectl get --raw /metrics)

# 1. 请求延迟分析
echo "1. API Server 请求延迟分析:"
echo "$APISERVER_METRICS" | awk '/apiserver_request_duration_seconds_bucket{.*le="1"}/{print $0}' | head -10

# 2. QPS 统计
echo "2. API Server QPS 统计:"
echo "$APISERVER_METRICS" | grep apiserver_request_total | head -5

# 3. 对象数量统计
echo "3. Kubernetes 对象数量统计:"
echo "$APISERVER_METRICS" | grep apiserver_storage_objects | sort -k2 -nr | head -10

# 4. 连接数统计
echo "4. 当前连接数统计:"
netstat -an | grep :6443 | grep ESTABLISHED | wc -l

# 5. 资源使用情况
echo "5. API Server 资源使用情况:"
kubectl top pod -n kube-system -l component=kube-apiserver
```
#### 2. etcd 性能诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# etcd 性能诊断脚本

echo "=== etcd 性能诊断 ==="

# 获取 etcd 指标
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
ETCD_METRICS=$(kubectl exec -n kube-system $ETCD_POD -- wget -qO- http://localhost:2379/metrics)

# 1. WAL fsync 延迟分析
echo "1. etcd WAL fsync 延迟分析:"
echo "$ETCD_METRICS" | grep etcd_disk_wal_fsync_duration_seconds | head -5

# 2. 磁盘性能检查
echo "2. etcd 磁盘性能检查:"
kubectl exec -n kube-system $ETCD_POD -- dd if=/dev/zero of=/var/lib/etcd/test bs=1M count=100 oflag=direct 2>&1

# 3. 数据库大小检查
echo "3. etcd 数据库大小:"
kubectl exec -n kube-system $ETCD_POD -- du -sh /var/lib/etcd/member/snap/db

# 4. 碎片整理状态
echo "4. etcd 碎片整理状态:"
echo "$ETCD_METRICS" | grep etcd_debugging_mvcc_db_compaction_keys_total

# 5. 网络延迟测试
echo "5. etcd 集群网络延迟:"
for pod in $(kubectl get pods -n kube-system -l component=etcd -o name); do
  echo "Testing $pod:"
  kubectl exec -n kube-system $pod -- ETCDCTL_API=3 etcdctl --endpoints=https://localhost:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt endpoint health
done
```
#### 3. 组件性能诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 控制平面组件性能诊断

echo "=== 控制平面组件性能诊断 ==="

# Scheduler 性能分析
echo "1. Scheduler 性能分析:"
SCHEDULER_METRICS=$(kubectl get --raw /metrics | grep scheduler_)
echo "$SCHEDULER_METRICS" | grep scheduling_duration_seconds | head -5
echo "$SCHEDULER_METRICS" | grep pending_pods | head -3

# Controller Manager 性能分析
echo "2. Controller Manager 性能分析:"
CONTROLLER_METRICS=$(kubectl get --raw /metrics | grep workqueue_)
echo "$CONTROLLER_METRICS" | grep workqueue_depth | head -5
echo "$CONTROLLER_METRICS" | grep workqueue_latency | head -5

# 组件资源使用情况
echo "3. 控制平面组件资源使用:"
kubectl top pods -n kube-system -l tier=control-plane
```
## 解决方案与风险控制

### API Server 优化方案

#### 方案一：调整 API Server 参数

```yaml
# /etc/kubernetes/manifests/kube-apiserver.yaml
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
spec:
  containers:
  - name: kube-apiserver
    image: registry.k8s.io/kube-apiserver:v1.32.0
    command:
    - kube-apiserver
    # 性能优化参数
    - --max-requests-inflight=3000
    - --max-mutating-requests-inflight=1000
    - --request-timeout=2m
    - --min-request-timeout=300
    - --target-ram-mb=8192
    - --kubelet-timeout=10s
    - --watch-cache-sizes=nodes#1000,pods#5000,services#1000,endpoints#10000
    - --default-watch-cache-size=500
    - --enable-aggregator-routing=true
    - --http2-max-streams-per-connection=1000
```

#### 方案二：启用 API 优先级与公平性

```yaml
# FlowSchema 配置示例
apiVersion: flowcontrol.apiserver.k8s.io/v1beta3
kind: FlowSchema
metadata:
  name: critical-operations
spec:
  matchingPrecedence: 100
  priorityLevelConfiguration:
    name: urgent
  rules:
  - resourceRules:
    - apiGroups: [""]
      resources: ["nodes", "persistentvolumes"]
      verbs: ["*"]
    subjects:
    - kind: Group
      group: "system:masters"

---
apiVersion: flowcontrol.apiserver.k8s.io/v1beta3
kind: PriorityLevelConfiguration
metadata:
  name: urgent
spec:
  type: Limited
  limited:
    assuredConcurrencyShares: 100
    limitResponse:
      type: Reject
```

### etcd 优化方案

#### 方案一：硬件和配置优化

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# etcd 性能优化脚本

# 1. 调整系统参数
cat >> /etc/sysctl.conf << EOF
# etcd 性能优化
vm.swappiness=1
fs.file-max=1000000
net.core.somaxconn=32768
EOF

sysctl -p

# 2. 优化 etcd 配置
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
kubectl patch pod $ETCD_POD -n kube-system -p '{"spec":{"containers":[{"name":"etcd","resources":{"requests":{"cpu":"2","memory":"4Gi"},"limits":{"cpu":"4","memory":"8Gi"}}}]}}'

# 3. 定期维护脚本
cat > /usr/local/bin/etcd-maintenance.sh << 'EOF'
#!/bin/bash
# etcd 维护脚本

# 碎片整理
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  defrag

# 压缩历史版本
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  compact $(ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    endpoint status --write-out="json" | jq '.[0].Status.header.revision')

# 告警清理
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  alarm disarm
EOF

chmod +x /usr/local/bin/etcd-maintenance.sh
```
#### 方案二：etcd 集群优化配置

```yaml
# etcd 高性能配置
apiVersion: v1
kind: Pod
metadata:
  name: etcd
  namespace: kube-system
spec:
  containers:
  - name: etcd
    image: registry.k8s.io/etcd:3.5.12-0
    command:
    - etcd
    - --advertise-client-urls=https://$(NODE_IP):2379
    - --cert-file=/etc/kubernetes/pki/etcd/server.crt
    - --client-cert-auth=true
    - --data-dir=/var/lib/etcd
    - --initial-advertise-peer-urls=https://$(NODE_IP):2380
    - --initial-cluster-state=new
    - --key-file=/etc/kubernetes/pki/etcd/server.key
    - --listen-client-urls=https://127.0.0.1:2379,https://$(NODE_IP):2379
    - --listen-metrics-urls=http://127.0.0.1:2381
    - --listen-peer-urls=https://$(NODE_IP):2380
    - --name=$(NODE_NAME)
    - --peer-cert-file=/etc/kubernetes/pki/etcd/peer.crt
    - --peer-client-cert-auth=true
    - --peer-key-file=/etc/kubernetes/pki/etcd/peer.key
    - --peer-trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    - --snapshot-count=10000
    - --trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    # 性能优化参数
    - --quota-backend-bytes=8589934592  # 8GB
    - --auto-compaction-mode=revision
    - --auto-compaction-retention=1000
    - --max-request-bytes=33554432  # 32MB
    - --grpc-keepalive-timeout=30s
```

### 组件性能优化

#### Scheduler 优化

```yaml
# Scheduler 性能优化配置
apiVersion: kubescheduler.config.k8s.io/v1beta3
kind: KubeSchedulerConfiguration
profiles:
- schedulerName: default-scheduler
  plugins:
    score:
      disabled:
      - name: NodeResourcesFit  # 如果不需要资源适配评分
    reserve:
      enabled:
      - name: VolumeBinding
  pluginConfig:
  - name: NodeResourcesFit
    args:
      scoringStrategy:
        type: LeastAllocated
        resources:
        - name: cpu
          weight: 1
        - name: memory
          weight: 1
```

#### Controller Manager 优化

```yaml
# Controller Manager 性能优化
apiVersion: v1
kind: Pod
metadata:
  name: kube-controller-manager
spec:
  containers:
  - name: kube-controller-manager
    image: registry.k8s.io/kube-controller-manager:v1.32.0
    command:
    - kube-controller-manager
    # 性能优化参数
    - --concurrent-deployment-syncs=10
    - --concurrent-endpoint-syncs=10
    - --concurrent-gc-syncs=30
    - --concurrent-namespace-syncs=10
    - --concurrent-replicaset-syncs=10
    - --concurrent-service-syncs=2
    - --concurrent-serviceaccount-token-syncs=10
    - --large-cluster-size-threshold=500
    - --node-eviction-rate=0.1
    - --secondary-node-eviction-rate=0.01
```

## ⚠️ 执行风险评估

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 调整 API Server 并发参数 | ⭐⭐ 中 | 可能影响请求处理能力 | 恢复原始参数值 |
| etcd 碎片整理 | ⭐⭐ 中 | 短暂性能波动 | 监控集群状态 |
| 调整 Controller Manager 并发数 | ⭐⭐ 中 | 控制器同步速度变化 | 恢复默认并发设置 |
| 修改 Scheduler 配置 | ⭐⭐ 中 | 调度行为可能改变 | 恢复原有调度策略 |

## 📊 性能验证与监控

### 性能验证脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 性能优化验证脚本

echo "=== 性能优化效果验证 ==="

# 1. API Server 性能验证
echo "1. API Server 性能指标:"
kubectl get --raw /metrics | grep apiserver_request_duration_seconds_bucket | grep 'le="0.1"' | head -5

# 2. etcd 性能验证
echo "2. etcd 性能指标:"
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
kubectl exec -n kube-system $ETCD_POD -- wget -qO- http://localhost:2379/metrics | grep etcd_disk_wal_fsync_duration_seconds_bucket | grep 'le="0.1"' | head -3

# 3. 组件性能验证
echo "3. 组件工作队列深度:"
kubectl get --raw /metrics | grep workqueue_depth | awk '$2 < 100 {print $0}'

# 4. 资源使用验证
echo "4. 控制平面资源使用情况:"
kubectl top pods -n kube-system -l tier=control-plane
```
### 性能监控告警配置

```yaml
# Prometheus 性能告警规则
groups:
- name: kubernetes.performance
  rules:
  - alert: APIServerHighLatency
    expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m])) > 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "API Server 响应延迟过高"
      description: "API Server 99% 请求延迟超过1秒"

  - alert: EtcdHighFsyncLatency
    expr: histogram_quantile(0.99, rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m])) > 0.1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "etcd WAL fsync 延迟过高"
      description: "etcd WAL fsync 99% 延迟超过100ms"

  - alert: HighWorkqueueDepth
    expr: workqueue_depth > 1000
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "控制器工作队列积压严重"
      description: "工作队列深度超过1000，可能存在性能瓶颈"

  - alert: ControlPlaneHighCPU
    expr: rate(process_cpu_seconds_total{job="kubernetes-control-plane"}[5m]) > 0.8
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "控制平面 CPU 使用率过高"
      description: "控制平面组件 CPU 使用率超过80%"
```

## 📚 性能优化最佳实践

### 性能基线建立

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 性能基线建立脚本

BASELINE_DIR="/var/log/kubernetes/baseline"
mkdir -p "$BASELINE_DIR"

{
  echo "=== Kubernetes 性能基线 $(date) ==="
  
  # API Server 基线
  echo "1. API Server 基线指标:"
  kubectl get --raw /metrics | grep apiserver_request_total | head -10
  
  # etcd 基线
  echo "2. etcd 基线指标:"
  ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
  kubectl exec -n kube-system $ETCD_POD -- wget -qO- http://localhost:2379/metrics | grep -E "(etcd_disk_wal_fsync_duration_seconds|etcd_server_has_leader)" | head -10
  
  # 资源使用基线
  echo "3. 资源使用基线:"
  kubectl top nodes
  kubectl top pods -n kube-system
  
  # 对象数量基线
  echo "4. 对象数量基线:"
  kubectl get --raw /metrics | grep apiserver_storage_objects
  
} > "${BASELINE_DIR}/baseline-$(date +%Y%m%d-%H%M%S).log"
```
### 定期性能检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 定期性能检查脚本

LOG_DIR="/var/log/kubernetes/performance"
mkdir -p "$LOG_DIR"

{
  echo "=== Kubernetes 性能检查报告 $(date) ==="
  
  # 性能指标收集
  echo "1. 关键性能指标:"
  kubectl get --raw /metrics | grep -E "(apiserver_request_duration_seconds|workqueue_depth|process_cpu_seconds_total)" | head -20
  
  # 慢查询检查
  echo "2. 慢查询统计:"
  kubectl get --raw /metrics | grep apiserver_request_duration_seconds_count | awk '$2 > 1000 {print $0}'
  
  # 资源瓶颈检查
  echo "3. 资源瓶颈检查:"
  kubectl top pods -n kube-system | awk '$3 > "80%" || $5 > "80%" {print $0}'
  
} >> "${LOG_DIR}/performance-check-$(date +%Y%m%d).log"
```
## 🔄 典型性能问题案例

### 案例一：大规模集群 API Server 性能瓶颈

**问题描述**：5000+ 节点集群中，kubectl 命令响应时间超过10秒。

**根本原因**：默认 watch 缓存大小不足以支撑大规模集群。

**解决方案**：
1. 增加 watch 缓存大小：`--watch-cache-sizes=nodes#5000,pods#50000`
2. 调整并发请求数：`--max-requests-inflight=5000`
3. 启用聚合路由：`--enable-aggregator-routing=true`

### 案例二：etcd 磁盘 I/O 性能问题

**问题描述**：etcd WAL fsync 延迟持续超过200ms，导致集群不稳定。

**根本原因**：共享存储性能不足，etcd 数据库碎片化严重。

**解决方案**：
1. 迁移至本地 SSD 存储
2. 定期执行碎片整理和压缩
3. 调整 etcd 配置参数优化 I/O 性能

## 📞 性能优化支持

**性能调优咨询**：
- Kubernetes 官方性能调优指南：https://kubernetes.io/docs/setup/best-practices/cluster-large/
- etcd 性能优化文档：https://etcd.io/docs/v3.5/op-guide/performance/

**专业服务**：
- CNCF 认证 Kubernetes 服务提供商
- 企业级 Kubernetes 性能优化咨询服务

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[系统基础/速查卡/go.md|go]]
- [[系统基础/速查卡/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]

## See Also

- [[故障诊断/高级排障/01-control-plane/06-apf-troubleshooting.md|06-apf-troubleshooting]]
- [[故障诊断/高级排障/01-control-plane/07-control-plane-security-troubleshooting.md|07-control-plane-security-troubleshooting]]
- [[故障诊断/高级排障/01-control-plane/09-control-plane-ha-troubleshooting.md|09-control-plane-ha-troubleshooting]]
- [[故障诊断/高级排障/01-control-plane/10-control-plane-upgrade-troubleshooting.md|10-control-plane-upgrade-troubleshooting]]

```

<!-- risk-assessed -->
