---
title: 'etcd 进阶: HA 集群管理与性能调优 [cluster-create]'
description: 'title: ''etcd 进阶: HA 集群管理与性能调优'''
summary: 'title: ''etcd 进阶: HA 集群管理与性能调优'''
category: general
tags:
- reference
- etcd
- apiserver
- kubelet
- controller-manager
- prometheus
- coredns
- containerd
- job
- cronjob
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 'etcd 进阶: HA 集群管理与性能调优 是什么'
- '如何 etcd 进阶: HA 集群管理与性能调优'
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- etcd
- '进阶:'
- HA
- 集群管理与性能调优
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- prometheus-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 'etcd 进阶: HA 集群管理与性能调优'
description: '# etcd 进阶: HA 集群管理与性能调优'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
- kubelet
- controller-manager
- prometheus
- coredns
- containerd
last_updated: '2026-05-18'
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes开发者
- DevOps工程师
- SRE
- DBA
estimated_read_time: 5min
intent_queries:
- Kubernetes etcd HA cluster management member add remove
- etcd defragment compact performance tuning
- etcd backup restore snapshot disaster recovery
- etcd learner mode Kubernetes
- etcd monitoring metrics Prometheus alerts
trigger_keywords:
- etcd
- HA
- member add
- member remove
- defragment
- compact
- snapshot
- backup
- restore
- learner
- raft
- quorum
- Prometheus
- monitoring
related_domains:
- domain-01-cluster-fundamentals
- domain-10-troubleshooting-diagnostics
related_topics:
- etcd
- kubeadm
- API Server
- backup
- disaster recovery
- monitoring
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

# etcd 进阶: HA 集群管理与性能调优

## 函数/流程签名

```go
func CreateEtcdStaticPodManifestHA(cfg *kubeadmapi.InitConfiguration, endpoints []string) error
func AddEtcdMember(client clientset.Interface, name string, peerURLs []string) (*etcdserverpb.Member, error)
func RemoveEtcdMember(client clientset.Interface, memberID uint64) error
func UpdateEtcdManifest(existingMembers []string, newMemberName string, newMemberIP string) error
func CheckEtcdClusterHealth(etcdClient *clientv3.Client) error
func DefragmentEtcd(endpoint string, certsDir string) error
func CompactEtcd(revision int64) error
```

## 源码位置

| 文件路径 | 行号范围 | 说明 |
|---------|---------|------|
| `cmd/kubeadm/app/phases/etcd/local.go` | L35-L250 | etcd manifest 生成 |
| `cmd/kubeadm/app/util/etcd/etcdutil.go` | L30-L200 | etcd 工具函数 (member add/remove) |
| `cmd/kubeadm/app/phases/etcd/util.go` | L25-L150 | etcd 集群操作工具 |
| `staging/src/k8s.io/apiserver/pkg/storage/storagebackend/factory/etcd3.go` | L50-L300 | API Server etcd v3 后端 |

## 参数说明

### etcd HA 参数

| 参数名 | 类型 | 说明 | 验证规则 |
|--------|------|------|---------|
| `--initial-cluster` | `string` | 初始集群成员列表 | 格式: `name=url,name=url` |
| `--initial-cluster-state` | `string` | 集群初始状态 | `new` 或 `existing` |
| `--initial-cluster-token` | `string` | 集群唯一 token | 不同集群必须不同 |
| `--listen-client-urls` | `[]string` | 客户端监听 URL | 必须包含本地地址 |
| `--listen-peer-urls` | `[]string` | 对等通信监听 URL | 必须包含广播地址 |
| `--advertise-client-urls` | `[]string` | 客户端广播 URL | 集群内其他成员可访问 |
| `--initial-advertise-peer-urls` | `[]string` | 对等通信广播 URL | 集群内所有成员可访问 |
| `--heartbeat-interval` | `string` | 心跳间隔 | 100ms-500ms，默认 100ms |
| `--election-timeout` | `string` | 选举超时 | 1000ms-50000ms，默认 1000ms |
| `--snapshot-count` | `int` | 快照阈值 | 默认 100000 |
| `--quota-backend-bytes` | `int64` | 后端配额 | 默认 2GB，推荐 8GB |
| `--auto-compaction-mode` | `string` | 自动压缩模式 | `periodic` 或 `revision` |
| `--auto-compaction-retention` | `string` | 压缩保留时间 | 如 `1h` |
| `--max-request-bytes` | `int` | 最大请求字节数 | 默认 1572864 (1.5MB) |

## 返回值

| 返回值 | 类型 | 说明 |
|--------|------|------|
| `*etcdserverpb.Member` | `struct` | etcd 成员信息 (ID, Name, PeerURLs, ClientURLs) |
| `MemberListResponse` | `struct` | 成员列表响应 |
| `error` | `error` | 操作失败错误 |

## 调用链

```mermaid
flowchart TB
    subgraph MemberAdd["添加 etcd 成员"]
        A[etcdutil.go: AddEtcdMember] --> B[连接 etcd 集群]
        B --> C[etcdctl member add]
        C --> D[生成新成员 manifest]
        D --> E["--initial-cluster-state=existing"]
        E --> F[kubelet 启动新 etcd]
        F --> G[新成员同步数据]
        G --> H[达到新 quorum]
    end

    subgraph MemberRemove["移除 etcd 成员"]
        I[etcdutil.go: RemoveEtcdMember] --> J[连接健康成员]
        J --> K[etcdctl member remove]
        K --> L[删除旧 manifest]
        L --> M[清理数据目录]
        M --> N[集群达到新 quorum]
    end

    subgraph Defrag["碎片整理"]
        O[DefragmentEtcd] --> P[连接 etcd endpoint]
        P --> Q[etcdctl defrag]
        Q --> R[重建数据库索引]
        R --> S[释放空间]
    end

    subgraph Compact["压缩"]
        T[CompactEtcd] --> U[获取当前 revision]
        U --> V[etcdctl compact]
        V --> W[删除历史 revision]
        W --> X[执行 defrag]
    end
```

## 源码分析

### 添加 etcd 成员

```go
// cmd/kubeadm/app/util/etcd/etcdutil.go
// AddEtcdMember 向现有 etcd 集群添加新成员
func AddEtcdMember(
    etcdClient *clientv3.Client,
    name string,
    peerURLs []string,
) (*etcdserverpb.Member, error) {
    // 1. 检查集群当前健康状态
    //    所有成员必须 healthy 才能添加新成员
    ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
    defer cancel()

    members, err := etcdClient.MemberList(ctx)
    if err != nil {
        return nil, fmt.Errorf("failed to list members: %w", err)
    }

    // 2. 检查成员数不超过合理范围
    //    推荐 3 或 5 个成员 (奇数，保证 quorum)
    if len(members.Members) >= 7 {
        return nil, fmt.Errorf("etcd cluster already has %d members (max recommended: 7)",
            len(members.Members))
    }

    // 3. 检查新成员名称不重复
    for _, member := range members.Members {
        if member.Name == name {
            return nil, fmt.Errorf("member with name %s already exists", name)
        }
    }

    // 4. 调用 etcd API 添加成员
    //    POST /v3/cluster/member/add
    addResp, err := etcdClient.MemberAdd(ctx, peerURLs)
    if err != nil {
        return nil, fmt.Errorf("failed to add member: %w", err)
    }

    // 5. 获取新成员 ID
    var newMember *etcdserverpb.Member
    for _, member := range addResp.Members {
        if member.Name == "" && containsURL(member.PeerURLs, peerURLs) {
            newMember = member
            break
        }
    }

    fmt.Printf("[etcd] Added member %s (ID: %d) to cluster\n",
        name, newMember.ID)
    return newMember, nil
}
```

### 移除 etcd 成员

```go
// cmd/kubeadm/app/util/etcd/etcdutil.go
// RemoveEtcdMember 从 etcd 集群移除成员
func RemoveEtcdMember(
    etcdClient *clientv3.Client,
    memberID uint64,
) error {
    ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
    defer cancel()

    // 1. 获取当前成员列表
    members, err := etcdClient.MemberList(ctx)
    if err != nil {
        return fmt.Errorf("failed to list members: %w", err)
    }

    // 2. 检查移除后集群仍能维持 quorum
    //    quorum = (N-1)/2 + 1 (N 为移除后的成员数)
    remainingCount := len(members.Members) - 1
    quorum := remainingCount/2 + 1
    if remainingCount < quorum {
        return fmt.Errorf("removing member %d would break quorum (%d remaining, need %d)",
            memberID, remainingCount, quorum)
    }

    // 3. 调用 etcd API 移除成员
    _, err = etcdClient.MemberRemove(ctx, memberID)
    if err != nil {
        return fmt.Errorf("failed to remove member %d: %w", memberID, err)
    }

    fmt.Printf("[etcd] Removed member %d from cluster\n", memberID)
    return nil
}
```

### 碎片整理

```go
// cmd/kubeadm/app/util/etcd/etcdutil.go
// DefragmentEtcd 对 etcd 后端数据库进行碎片整理
func DefragmentEtcd(
    endpoint string,
    certsDir string,
) error {
    // 1. 连接 etcd
    etcdClient, err := getEtcdClient(endpoint, certsDir)
    if err != nil {
        return err
    }
    defer etcdClient.Close()

    ctx, cancel := context.WithTimeout(context.Background(), 5*time.Minute)
    defer cancel()

    // 2. 执行碎片整理
    //    重建 B+ 树索引，释放已删除数据的空间
    _, err = etcdClient.Defragment(ctx, endpoint)
    if err != nil {
        return fmt.Errorf("defragmentation failed: %w", err)
    }

    fmt.Printf("[etcd] Defragmented %s\n", endpoint)
    return nil
}
```

## 执行流程

### etcd 成员变更流程

```
# 🟢 低风险：只读/信息收集，通常无副作用
步骤 1: 准备新节点
    → 安装 kubeadm, kubelet, containerd
    → 确保网络连通
    ↓
步骤 2: kubeadm join --control-plane
    → 解密获取 etcd CA 证书
    → 生成 etcd server/peer/healthcheck 证书
    ↓
步骤 3: 向 etcd 集群注册新成员
    → etcdctl member add --peer-urls=https://new:2380
    → 集群进入 "member adding" 状态
    ↓
步骤 4: 生成 etcd manifest
    → --initial-cluster-state=existing
    → --initial-cluster 包含所有成员 (含新成员)
    ↓
步骤 5: kubelet 启动 etcd 容器
    → etcd 连接到现有集群
    → 开始同步数据
    ↓
步骤 6: 数据同步完成
    → 新成员状态变为 started
    → 集群达到新的 quorum
```
## 使用场景

### 场景 1: etcd 定期备份脚本

```yaml
# etcd-backup-cronjob.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: etcd-backup
  namespace: kube-system
spec:
  schedule: "*/30 * * * *"  # 每 30 分钟
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          nodeName: master-1
          containers:
          - name: backup
            image: registry.k8s.io/etcd:3.5.9-0
            command:
            - /bin/sh
            - -c
            - |
              TIMESTAMP=$(date +%Y%m%d-%H%M%S)
              BACKUP_DIR="/backup/etcd-${TIMESTAMP}"
              mkdir -p ${BACKUP_DIR}
              ETCDCTL_API=3 etcdctl snapshot save ${BACKUP_DIR}/snapshot.db \
                --endpoints=https://127.0.0.1:2379 \
                --cacert=/etc/kubernetes/pki/etcd/ca.crt \
                --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
                --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
              ETCDCTL_API=3 etcdctl snapshot status ${BACKUP_DIR}/snapshot.db --write-table
              # 保留最近 10 个备份
              ls -t /backup/ | tail -n +11 | xargs -I {} rm -rf /backup/{}
            volumeMounts:
            - name: etcd-certs
              mountPath: /etc/kubernetes/pki/etcd
              readOnly: true
            - name: backup-dir
              mountPath: /backup
          volumes:
          - name: etcd-certs
            hostPath:
              path: /etc/kubernetes/pki/etcd
          - name: backup-dir
            hostPath:
              path: /var/backups/etcd
          restartPolicy: OnFailure
```

### 场景 2: etcd 性能调优配置

```yaml
# kubeadm-config.yaml (etcd 调优)
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
etcd:
  local:
    extraArgs:
      # 心跳和选举 (网络延迟高时调大)
      heartbeat-interval: "500"
      election-timeout: "5000"

      # 快照 (减少内存使用)
      snapshot-count: "10000"

      # 自动压缩 (控制数据库增长)
      auto-compaction-mode: "periodic"
      auto-compaction-retention: "1h"

      # 后端配额 (默认 2GB，大集群调大)
      quota-backend-bytes: "8589934592"  # 8GB

      # 最大请求 (支持更大的资源对象)
      max-request-bytes: "10485760"  # 10MB

      # 数据一致性检查
      experimental-initial-corrupt-check: "true"
      experimental-corrupt-check-time: "240m"

      # 监控指标
      listen-metrics-urls: "http://127.0.0.1:2381"
```

### 场景 3: etcd 数据压缩和碎片整理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看当前 revision
ETCDCTL_API=3 etcdctl endpoint status -w json \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key | jq '.[0].Status.header.revision'

# 2. 压缩历史 revision (保留当前 revision 之前的)
CURRENT_REV=$(ETCDCTL_API=3 etcdctl endpoint status -w json \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key | jq '.[0].Status.header.revision')
ETCDCTL_API=3 etcdctl compact "$CURRENT_REV"

# 3. 碎片整理 (释放空间)
ETCDCTL_API=3 etcdctl defrag \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 4. 检查数据库大小
ETCDCTL_API=3 etcdctl endpoint status -w table \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
```
### 场景 4: 移除问题 etcd 成员

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl member remove`：移除 etcd 成员，误删多数派会致集群不可用/丢数据
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 列出成员
ETCDCTL_API=3 etcdctl member list -w table \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 2. 移除问题成员
ETCDCTL_API=3 etcdctl member remove <member-id> \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 3. 在问题节点清理
rm -rf /var/lib/etcd/*  # ⚠️ 删除系统/数据文件

# 4. 重新加入
kubeadm join --control-plane --certificate-key <key>
```
## 配置示例

### etcd Prometheus 监控

```yaml
# etcd-service-monitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: etcd
  namespace: kube-system
spec:
  selector:
    matchLabels:
      component: etcd
  endpoints:
  - port: metrics
    interval: 15s
    scheme: https
    tlsConfig:
      caFile: /etc/kubernetes/pki/etcd/ca.crt
      certFile: /etc/kubernetes/pki/etcd/healthcheck-client.crt
      keyFile: /etc/kubernetes/pki/etcd/healthcheck-client.key
      insecureSkipVerify: true
---
apiVersion: v1
kind: Service
metadata:
  name: etcd-metrics
  namespace: kube-system
  labels:
    component: etcd
spec:
  type: ClusterIP
  clusterIP: None
  ports:
  - name: metrics
    port: 2381
    targetPort: 2381
```

### etcd 关键告警规则

```yaml
# etcd-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: etcd-alerts
  namespace: kube-system
spec:
  groups:
  - name: etcd
    rules:
    - alert: EtcdNoLeader
      expr: etcd_server_has_leader == 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "etcd has no leader"
    - alert: EtcdHighFsyncDurations
      expr: histogram_quantile(0.99, rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m])) > 0.01
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "etcd WAL fsync latency is high"
    - alert: EtcdDatabaseSpaceExceeded
      expr: etcd_mvcc_db_total_size_in_bytes / etcd_server_quota_backend_bytes > 0.8
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "etcd database size exceeds 80% of quota"
```

## 实战示例

### etcd 集群状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 全面健康检查
ETCDCTL_API=3 etcdctl check perf \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
# 60 / 60 Booleans                                         100%
# ----------------------
# PASS: Throughput is 261 QPS
# PASS: Slowest request took 0.054856s
# PASS: Stddev is 0.004680s
# PASS
# PASS: Roughly 261 keys/sec
# PASS: Roughly 0.05s per key

# 查看所有 endpoint 状态
ETCDCTL_API=3 etcdctl endpoint status --cluster -w table \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
# +---------------------------+------------------+---------+---------+-----------+------------+-----------+------------+--------------------+--------+
# |         ENDPOINT          |        ID        | VERSION | DB SIZE | IS LEADER | IS LEARNER | RAFT TERM | RAFT INDEX | RAFT APPLIED INDEX | ERRORS |
# +---------------------------+------------------+---------+---------+-----------+------------+-----------+------------+--------------------+--------+
# | https://192.168.1.10:2379 | 8e9e05c52164694d |   3.5.9 |  5.6 MB |      true |      false |         5 |    1234567 |            1234567 |        |
# | https://192.168.1.11:2379 | cf1b5c5a52164694 |   3.5.9 |  5.6 MB |     false |      false |         5 |    1234567 |            1234567 |        |
# | https://192.168.1.12:2379 | d2b7c5b52164694e |   3.5.9 |  5.6 MB |     false |      false |         5 |    1234567 |            1234567 |        |
# +---------------------------+------------------+---------+---------+-----------+------------+-----------+------------+--------------------+--------+
```
## 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| `database space exceeded` | 超过后端配额 | 紧缩+碎片整理+增大 quota-backend-bytes |
| `too many learners` | learner 成员过多 | 等待 learner 完成同步变为 voting member |
| `member not found` | 成员已被移除 | 清理数据目录后重新加入 |
| `raft: leader changed` 频繁 | 网络延迟/磁盘 IO 慢 | 增大 election-timeout，检查磁盘 |
| `wal: max entry size exceed` | 单个对象过大 | 调大 max-request-bytes |
| `context deadline exceeded` | 操作超时 | 增大超时时间，检查网络 |
| `etcdserver: unhealthy cluster` | 多数成员不可用 | 恢复问题成员或从快照恢复 |

### etcd 数据查看与调试

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 key
ETCDCTL_API=3 etcdctl get / --prefix --keys-only \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
# /registry/apiextensions.k8s.io/customresourcedefinitions/...
# /registry/apiregistration.k8s.io/apiservices/...
# /registry/clusterrolebindings/...
# /registry/clusterroles/...
# /registry/configmaps/kube-system/kubeadm-config
# /registry/namespaces/default
# /registry/namespaces/kube-system
# /registry/nodes/master
# /registry/pods/kube-system/coredns-...
# /registry/secrets/default/default-token-...
# /registry/services/endpoints/kube-system/kube-controller-manager
# /registry/services/specs/default/kubernetes

# 查看特定 Pod 的 etcd 数据
ETCDCTL_API=3 etcdctl get /registry/pods/kube-system/etcd-master \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  --print-value-only | protoc --decode_raw

# 查看 etcd 写入延迟
curl -s http://127.0.0.1:2381/metrics | grep etcd_disk_wal_fsync_duration_seconds_sum
# etcd_disk_wal_fsync_duration_seconds_sum 0.003456

# etcd 内存使用
ps aux | grep etcd
# root  12345  2.5  3.0  /usr/local/bin/etcd ...
```
### etcd 灾难恢复完整流程

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# === 在所有 control-plane 节点执行 ===

# 1. 停止所有控制面组件
systemctl stop kubelet
crictl stop $(crictl ps -q)

# 2. 在健康的 etcd 节点创建快照
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-disaster-recovery.db \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  --endpoints=https://192.168.1.10:2379

# 3. 在所有节点清理 etcd 数据
rm -rf /var/lib/etcd/member/  # ⚠️ 删除系统/数据文件

# 4. 从快照恢复 (在第一个节点)
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-disaster-recovery.db \
  --name=master-1 \
  --initial-cluster=master-1=https://192.168.1.10:2380 \
  --initial-cluster-token=etcd-cluster \
  --initial-advertise-peer-urls=https://192.168.1.10:2380 \
  --data-dir=/var/lib/etcd

# 5. 启动第一个节点的 kubelet
systemctl start kubelet

# 6. 等待 etcd 和 API Server 就绪
ETCDCTL_API=3 etcdctl endpoint health \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 7. 验证集群数据
kubectl get nodes
kubectl get pods -A
```
## 相关函数

- [etcd 基础](07-etcd.md) — etcd 静态 Pod 创建
- [证书管理](03-certs.md) — etcd TLS 证书
- [高可用进阶](14-ha-advanced.md) — HA etcd 架构
- [集群升级](09-upgrade.md) — etcd 版本升级
- [集群概览](01-overview.md) — init 流程

### etcd 关键指标监控配置

```yaml
# etcd 关键告警规则 (Prometheus)
groups:
- name: etcd
  rules:
  - alert: EtcdDatabaseHighFragmentationRatio
    expr: last_over_time(etcd_mvcc_db_total_size_in_use_bytes[5m]) / last_over_time(etcd_mvcc_db_total_size_in_bytes[5m]) < 0.5
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "etcd database fragmentation exceeds 50%"
      description: "etcd cluster {{ $labels.job }} database fragmentation is {{ $value }}%. Run defragmentation."
  - alert: EtcdMemberCommunicationSlow
    expr: histogram_quantile(0.99, rate(etcd_network_peer_round_trip_time_seconds_bucket[5m])) > 0.15
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "etcd peer communication slow"
```

### etcd 版本兼容性

| Kubernetes | 推荐 etcd | 最低 etcd |
|-----------|----------|----------|
| 1.24 | 3.5.x | 3.5.x |
| 1.25 | 3.5.x | 3.5.x |
| 1.26 | 3.5.x | 3.5.x |
| 1.27 | 3.5.x | 3.5.x |
| 1.28 | 3.5.x | 3.5.x |
| 1.29 | 3.5.x | 3.5.x |

### etcd 数据一致性检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查所有成员的 revision 是否一致
ETCDCTL_API=3 etcdctl endpoint status --cluster -w json \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key | \
  jq '.[].Status.header.revision'

# 所有成员的 revision 应该接近 (允许少量延迟)
# 如果差距很大 → 有成员在同步中或问题

# 检查 etcd 存储backend 大小
ETCDCTL_API=3 etcdctl endpoint status --cluster -w json \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key | \
  jq '.[].Status.dbSize'

# 检查 leader 是否正常
ETCDCTL_API=3 etcdctl endpoint status --cluster -w table \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
# 应该只有一个 IS LEADER = true
```
### etcd Learner 模式 (Kubernetes 1.27+)

```yaml
# etcd learner 模式: 新成员以 learner 身份加入，同步完成后再变为 voting member
# 减少了添加新成员时对集群性能的影响
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
featureGates:
  EtcdLearnerMode: true  # 启用 learner 模式
etcd:
  local:
    extraArgs:
      experimental-initial-corrupt-check: "true"
      experimental-corrupt-check-time: "240m"
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看成员是否为 learner
ETCDCTL_API=3 etcdctl member list -w table \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
# IS LEARNER 列: 新成员可能显示 true (同步中)
# 同步完成后自动变为 false (voting member)
```
## Related

- [[reference|#reference Hub]] — tag hub

- [[hot|hot]]
- [[domain-17-system-foundation/速查卡/go.md|go]]
- [[domain-17-system-foundation/速查卡/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/coredns.md|coredns]]
- [[domain-19-landscape-references/领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
