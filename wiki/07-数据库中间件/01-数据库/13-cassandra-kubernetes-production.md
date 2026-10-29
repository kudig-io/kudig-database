---
title: Cassandra on Kubernetes 生产部署
description: 'Cass Operator 部署、多 DC 跨区域、Compaction 策略、一致性级别、节点替换与修复'
summary: 'Cass Operator 部署、多 DC 跨区域、Compaction 策略、一致性级别、节点替换与修复'
category: database-middleware
tags:
- database
- k8s
- cassandra
- nosql
- distributed
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- DBA
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Cassandra on Kubernetes 生产部署 是什么
- 如何 Cassandra on Kubernetes 生产部署
trigger_keywords:
- cassandra
- cass-operator
- compaction
- 多dc
- 一致性
prerequisites:
- kubectl-basics
- database-basics
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


# Cassandra on Kubernetes 生产部署

## 1. K8ssandra Operator 安装

### 1.1 安装 K8ssandra Operator

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Helm 仓库
helm repo add k8ssandra https://helm.k8ssandra.io/stable
helm repo update

# 安装 Operator
helm install k8ssandra-operator k8ssandra/k8ssandra-operator \
  -n k8ssandra-operator --create-namespace \
  --set image.tag=v1.18.0

# 验证
kubectl get pods -n k8ssandra-operator
kubectl get crd | grep k8ssandra
```
### 1.2 CRD 说明

```bash
# K8ssandra Operator 提供的核心 CRD
# - K8ssandraCluster: 集群定义（Cassandra + Stargate + Reaper + Monitoring）
# - CassandraDatacenter: 数据中心
# - CassandraTask: 管理任务（cleanup, rebuild, upgrade）
```

## 2. 生产集群定义

### 2.1 单 DC 集群

```yaml
apiVersion: k8ssandra.io/v1alpha1
kind: K8ssandraCluster
metadata:
  name: prod-cassandra
  namespace: cassandra
spec:
  cassandra:
    serverVersion: "4.1.4"
    serverImage: k8ssandra/cass-management-api:4.1.4
    jmxInitImage: busybox:1.36.1
    config:
      cassandraYaml:
        allocate_tokens_for_local_replication_key_partition_count: 128
        authenticator: PasswordAuthenticator
        authorizer: CassandraAuthorizer
        role_manager: CassandraRoleManager
        partitioner: org.apache.cassandra.dht.Murmur3Partitioner
        commitlog_segment_size_in_mb: 32
        commitlog_total_space_in_mb: 8192
        compaction_throughput_mb_per_sec: 256
        concurrent_reads: 32
        concurrent_writes: 64
        concurrent_counter_writes: 32
        memtable_allocation_type: offheap_objects
        native_transport_max_threads: 128
        native_transport_max_frame_size_in_mb: 256
        seed_provider:
          - class_name: org.apache.cassandra.locator.SimpleSeedProvider
        endpoint_snitch: GossipingPropertyFileSnitch
      jvmOptions:
        heap_size: 8G
        gc:
          - name: G1GC
            options:
              - "-XX:G1RSetUpdatingPauseTimePercent=5"
              - "-XX:MaxGCPauseMillis=300"
              - "-XX:InitiatingHeapOccupancyPercent=70"
        additional:
          - "-Dcassandra.allow_unsafe_aggressive_sstable_expiration=true"
          - "-Dcassandra.max_queued_native_transport_requests=1024"
          - "-Dcassandra.io.netty.eventloop.maxPendingTasks=512"
    datacenters:
    - metadata:
        name: dc1
      k8sContext: us-east-1
      size: 6
      racks:
      - name: rack1
        nodeAffinityLabels:
          topology.kubernetes.io/zone: us-east-1a
      - name: rack2
        nodeAffinityLabels:
          topology.kubernetes.io/zone: us-east-1b
      - name: rack3
        nodeAffinityLabels:
          topology.kubernetes.io/zone: us-east-1c
      storageConfig:
        cassandraDataVolumeClaimSpec:
          storageClassName: gp3-ssd
          accessModes:
          - ReadWriteOnce
          resources:
            requests:
              storage: 500Gi
      resources:
        requests:
          cpu: "4"
          memory: 16Gi
        limits:
          cpu: "8"
          memory: 32Gi
      config:
        cassandraYaml:
          allocate_tokens_for_local_replication_key_partition_count: 128
    managementApiAuth:
      insecure: {}
  stargate:
    size: 3
    heapSize: 1G
    cassandraConfigRef:
      name: prod-cassandra
    resources:
      requests:
        cpu: "1"
        memory: 2Gi
      limits:
        cpu: "2"
        memory: 4Gi
  reaper:
    keyspace: reaper_db
    scheduling:
      placement:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-role
                operator: In
                values:
                - cassandra
```

### 2.2 多 DC 跨区域部署

```yaml
apiVersion: k8ssandra.io/v1alpha1
kind: K8ssandraCluster
metadata:
  name: prod-cassandra
  namespace: cassandra
spec:
  cassandra:
    serverVersion: "4.1.4"
    config:
      cassandraYaml:
        endpoint_snitch: GossipingPropertyFileSnitch
    datacenters:
    - metadata:
        name: us-east-1
      k8sContext: us-east-1
      size: 6
      racks:
      - name: rack1
        nodeAffinityLabels:
          topology.kubernetes.io/zone: us-east-1a
      - name: rack2
        nodeAffinityLabels:
          topology.kubernetes.io/zone: us-east-1b
      - name: rack3
        nodeAffinityLabels:
          topology.kubernetes.io/zone: us-east-1c
      storageConfig:
        cassandraDataVolumeClaimSpec:
          storageClassName: gp3-ssd
          resources:
            requests:
              storage: 500Gi
    - metadata:
        name: us-west-2
      k8sContext: us-west-2
      size: 6
      racks:
      - name: rack1
        nodeAffinityLabels:
          topology.kubernetes.io/zone: us-west-2a
      - name: rack2
        nodeAffinityLabels:
          topology.kubernetes.io/zone: us-west-2b
      - name: rack3
        nodeAffinityLabels:
          topology.kubernetes.io/zone: us-west-2c
      storageConfig:
        cassandraDataVolumeClaimSpec:
          storageClassName: gp3-ssd
          resources:
            requests:
              storage: 500Gi
```

### 2.3 多 DC 架构图

```
┌──────────── us-east-1 (DC1) ────────────┐     ┌──────────── us-west-2 (DC2) ────────────┐
│  ┌─rack1──┐  ┌─rack2──┐  ┌─rack3──┐    │     │  ┌─rack1──┐  ┌─rack2──┐  ┌─rack3──┐    │
│  │ Node1  │  │ Node3  │  │ Node5  │    │     │  │ Node7  │  │ Node9  │  │ Node11 │    │
│  │ Node2  │  │ Node4  │  │ Node6  │    │     │  │ Node8  │  │ Node10 │  │ Node12 │    │
│  └────────┘  └────────┘  └────────┘    │     │  └────────┘  └────────┘  └────────┘    │
│         ↕  跨 DC 复制 (Gossip + Streaming)  ←────────────→  跨 DC 复制  ↕               │
└─────────────────────────────────────────┘     └─────────────────────────────────────────┘
```

## 3. Keyspace 复制策略

### 3.1 单 DC 推荐配置

```sql
-- NetworkTopologyStrategy 即使单 DC 也推荐使用
CREATE KEYSPACE app_data WITH replication = {
  'class': 'NetworkTopologyStrategy',
  'dc1': 3
};

-- 每个 DC 副本数 = 3（跨 3 个 rack 各 1 个副本）
```

### 3.2 多 DC 推荐配置

```sql
-- 生产级多 DC 配置
CREATE KEYSPACE app_data WITH replication = {
  'class': 'NetworkTopologyStrategy',
  'us-east-1': 3,
  'us-west-2': 3
};

-- 跨 DC 查询需要 LOCAL_QUORUM
-- 写入使用 LOCAL_QUORUM 保证本地一致性
-- 使用 CLUSTER_QUORUM 实现跨 DC 强一致（高延迟）
```

## 4. Compaction 策略选型

### 4.1 策略对比

| 策略 | 适用场景 | 读放大 | 写放大 | 空间放大 |
|------|---------|-------|-------|---------|
| SizeTieredCompactionStrategy (STCS) | 写多读少 | 高 | 低 | 高 |
| LeveledCompactionStrategy (LCS) | 读多写少 | 低 | 中 | 低 |
| TimeWindowCompactionStrategy (TWCS) | 时序数据/TTL | 中 | 低 | 低 |
| UnifiedCompactionStrategy (UCS) | 通用（Cassandra 5.0+） | 可调 | 可调 | 可调 |

### 4.2 策略配置

```sql
-- STCS（默认，写密集型）
ALTER TABLE app_data.events WITH compaction = {
  'class': 'SizeTieredCompactionStrategy',
  'min_sstable_size': 50,
  'bucket_low': 0.5,
  'bucket_high': 1.5,
  'min_threshold': 4,
  'max_threshold': 32
};

-- LCS（读密集型，空间敏感）
ALTER TABLE app_data.user_profiles WITH compaction = {
  'class': 'LeveledCompactionStrategy',
  'sstable_size_in_mb': 160
};

-- TWCS（时序数据，配合 TTL）
ALTER TABLE app_data.sensor_readings WITH compaction = {
  'class': 'TimeWindowCompactionStrategy',
  'compaction_window_unit': 'DAYS',
  'compaction_window_size': 1
}
AND default_time_to_live = 2592000;  -- 30 天 TTL

-- UCS（Cassandra 5.0+，统一策略）
ALTER TABLE app_data.events WITH compaction = {
  'class': 'UnifiedCompactionStrategy',
  'scaling_parameters': 'T4',
  'min_sstable_size': 100,
  'expired_sstable_check_frequency_seconds': 600
};
```

### 4.3 TWCS 时序数据架构

```
写入 ──→ MemTable ──→ Flush ──→ SSTable (Day 1)
                                      │
                              TWCS Compaction
                                      │
                      ┌───────────────┼───────────────┐
                      ▼               ▼               ▼
                SSTable (Day 1)  SSTable (Day 2)  SSTable (Day 3)
                    │               │               │
                    ▼               ▼               ▼
              TTL 到期删除     TTL 到期删除     TTL 到期删除
```

## 5. 一致性级别配置

### 5.1 一致性级别对照

| CL 级别 | 描述 | 需要确认节点数 | 适用场景 |
|---------|------|-------------|---------|
| ONE | 1 个节点确认 | 1 | 低延迟非关键写入 |
| TWO | 2 个节点确认 | 2 | 一般写入 |
| THREE | 3 个节点确认 | 3 | 较高一致性 |
| QUORUM | 多数节点确认 | ⌊RF/2⌋ + 1 | 推荐生产默认 |
| LOCAL_QUORUM | 本地 DC 多数确认 | ⌊本地RF/2⌋ + 1 | 多 DC 推荐 |
| ALL | 全部节点确认 | RF | 极高一致性，不推荐 |
| LOCAL_ONE | 本地 DC 1 个节点 | 1 | 跨 DC 低延迟读取 |

### 5.2 生产一致性配置

```yaml
# 客户端驱动配置
# DataStax Java Driver
datastax-java-driver:
  basic:
    request:
      consistency: LOCAL_QUORUM
      serial-consistency: LOCAL_SERIAL
    load-balancing-policy:
      local-datacenter: us-east-1

# GoCQL
# cluster.Consistency = gocql.LocalQuorum
# cluster.SerialConsistency = gocql.LocalSerial
```

### 5.3 一致性与可用性权衡

```
                     强一致 (ALL / QUORUM)
                          │
                          │   延迟增加
                          │   可用性降低
                          ▼
写入 ──→ ┌──────────────────────────┐ ──→ 读取
         │  RF=3, CL=LOCAL_QUORUM   │
         │  需要 2/3 节点确认        │
         └──────────────────────────┘
                          ▲
                          │   延迟降低
                          │   一致性降低
                          │
                     弱一致 (ONE / LOCAL_ONE)
```

## 6. 节点管理操作

### 6.1 节点替换（故障节点恢复）

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
# 获取故障 Pod 名称
kubectl get pods -n cassandra -l app.kubernetes.io/managed-by=cass-operator

# 标记节点为 decommission
kubectl exec -n cassandra prod-cassandra-dc1-rack1-0 -c cassandra -- \
  nodetool status

# 删除故障 PVC 让 Operator 重建
kubectl delete pvc server-data-prod-cassandra-dc1-rack1-1 -n cassandra

# Operator 自动执行 replace_address 流程
# 1. 新 Pod 启动，检测到数据目录为空
# 2. 从种子节点获取集群信息
# 3. 从其他副本流式复制数据
# 4. 加入集群，开始服务

# 监控替换进度
kubectl exec -n cassandra prod-cassandra-dc1-rack1-0 -c cassandra -- \
  nodetool netstats
```
### 6.2 扩缩容

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 水平扩容（增加节点数）
kubectl patch k8ssandracluster prod-cassandra -n cassandra --type merge \
  -p '{"spec":{"cassandra":{"datacenters":[{"name":"dc1","size":9}]}}}'

# 监控扩容
kubectl get cassandradatacenter -n cassandra -w

# 缩容（Operator 自动执行 decommission）
kubectl patch k8ssandracluster prod-cassandra -n cassandra --type merge \
  -p '{"spec":{"cassandra":{"datacenters":[{"name":"dc1","size":3}]}}}'
```
### 6.3 Repair 操作

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Reaper 自动 repair（推荐）
# Reaper 已在 K8ssandraCluster 中配置
# 自动按计划执行增量 repair

# 手动触发 repair
kubectl exec -n cassandra prod-cassandra-dc1-rack1-0 -c cassandra -- \
  nodetool repair -pr app_data

# 全量 repair
kubectl exec -n cassandra prod-cassandra-dc1-rack1-0 -c cassandra -- \
  nodetool repair -full app_data

# 并行 repair（慎用，可能影响性能）
kubectl exec -n cassandra prod-cassandra-dc1-rack1-0 -c cassandra -- \
  nodetool repair -par app_data
```
## 7. 监控告警

### 7.1 Prometheus 指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: cassandra-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app.kubernetes.io/managed-by: cass-operator
  endpoints:
  - port: prometheus
    interval: 30s
```

### 7.2 关键告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cassandra-alerts
  namespace: monitoring
spec:
  groups:
  - name: cassandra
    rules:
    - alert: CassandraNodeDown
      expr: up{job="cassandra"} == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Cassandra 节点不可达"
    - alert: CassandraHighPendingCompactions
      expr: cassandra_table_pending_compactions > 50
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Cassandra 待压缩任务积压超过 50"
    - alert: CassandraDroppedMessages
      expr: |
        rate(cassandra_dropped_message_total[5m]) > 10
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Cassandra 消息丢弃率异常"
    - alert: CassandraHighReadLatency
      expr: |
        histogram_quantile(0.99, rate(cassandra_client_request_latency_bucket[5m])) > 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Cassandra P99 读延迟超过 100ms"
    - alert: CassandraHintedHandoffBacklog
      expr: cassandra_storage_total_hints > 10000
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Cassandra Hinted Handoff 积压过高"
```

## 8. 故障排查速查

| 问题 | 排查命令 | 常见原因 |
|------|---------|---------|
| 节点无法加入 | `nodetool status` + 检查日志 | 种子节点不可达、防火墙规则 |
| 读写超时 | `nodetool tablestats` | Compaction 积压、内存不足 |
| 数据不一致 | `nodetool repair` | 副本同步延迟、网络分区 |
| 磁盘满 | `nodetool tablestats` | TTL 未生效、Compaction 未清理 |
| GC 停顿 | 检查 GC 日志 | 堆过大导致 Full GC |
| Hint 积压 | `nodetool statusgossip` | 目标节点宕机、网络分区 |


<!-- risk-assessed -->
