---
title: ClickHouse on Kubernetes 生产部署
description: 'ClickHouse Operator 部署、分片副本拓扑、MergeTree 优化、数据分布与 Keeper 管理'
summary: 'ClickHouse Operator 部署、分片副本拓扑、MergeTree 优化、数据分布与 Keeper 管理'
category: database-middleware
tags:
- database
- k8s
- clickhouse
- olap
- analytics
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
- ClickHouse on Kubernetes 生产部署 是什么
- 如何 ClickHouse on Kubernetes 生产部署
trigger_keywords:
- clickhouse
- operator
- mergetree
- 分片
- 副本
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


# ClickHouse on Kubernetes 生产部署

## 1. ClickHouse Operator 安装

### 1.1 安装 Altinity ClickHouse Operator

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 克隆 Operator 仓库
git clone https://github.com/Altinity/clickhouse-operator.git
cd clickhouse-operator

# 安装 CRD 和 Operator
kubectl apply -f deploy/operator/clickhouse-operator-install-crd.yaml
kubectl apply -f deploy/operator/clickhouse-operator-install.yaml

# 验证安装
kubectl get pods -n kube-system -l app=clickhouse-operator
kubectl get crd | grep clickhouse
```
### 1.2 Operator 配置优化

```yaml
# clickhouse-operator-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: etc-clickhouse-operator-config
  namespace: kube-system
data:
  config.yaml: |
    watch:
      namespaces:
        - ""
      label: ""
    clickhouse:
      config:
        user:
          default/profile: default
          default/password: ""
          default/access_management: 1
          default/networks/ip:
            - "0.0.0.0/0"
        networks:
          host_regexp: ".*\\.chi-.*\\.svc\\.cluster\\.local$"
      templates:
        volumeClaimTemplate:
          spec:
            storageClassName: gp3
            accessModes:
              - ReadWriteOnce
            resources:
              requests:
                storage: 100Gi
    reconcile:
      host:
        wait:
          include:
            - "NotReady"
          exclude: []
          timeout: 300
```

## 2. 生产集群定义

### 2.1 3 分片 2 副本拓扑

```yaml
apiVersion: clickhouse.altinity.com/v1
kind: ClickHouseInstallation
metadata:
  name: prod-ch
  namespace: analytics
spec:
  configuration:
    clusters:
    - name: prod-cluster
      layout:
        shardsCount: 3
        replicasCount: 2
      templates:
        podTemplate: clickhouse-stable
        volumeClaimTemplate: ch-data-volume
    settings:
      max_connections: 4096
      max_concurrent_queries: 200
      mark_cache_size: 5368709120
      uncompressed_cache_size: 8589934592
      background_pool_size: 16
      background_merges_mutations_concurrency_ratio: 4
      max_server_memory_usage_to_ram_ratio: 0.8
    users:
      default/password_sha256_hex: <sha256>
      default/profile: default
      default/quota: default
      default/networks/ip:
        - "10.0.0.0/8"
    profiles:
      default:
        max_threads: 8
        max_memory_usage: 10737418240
        max_memory_usage_for_all_queries: 32212254720
        load_balancing: random
        use_uncompressed_cache: 1
        merge_tree_uniform_read_distribution: 1
    quotas:
      default:
        interval:
          duration: 3600
          queries: 10000
          errors: 1000
          result_rows: 1000000000
          read_rows: 10000000000
          execution_time: 6000
  templates:
    podTemplates:
    - name: clickhouse-stable
      spec:
        containers:
        - name: clickhouse
          image: clickhouse/clickhouse-server:24.8
          ports:
          - name: http
            containerPort: 8123
          - name: tcp
            containerPort: 9000
          - name: interserver
            containerPort: 9009
          resources:
            requests:
              memory: 32Gi
              cpu: "8"
            limits:
              memory: 32Gi
              cpu: "16"
          volumeMounts:
          - name: ch-data-volume
            mountPath: /var/lib/clickhouse
          - name: ch-config
            mountPath: /etc/clickhouse-server/config.d
          livenessProbe:
            httpGet:
              path: /ping
              port: http
            initialDelaySeconds: 30
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /ping
              port: http
            initialDelaySeconds: 5
            periodSeconds: 5
        affinity:
          podAntiAffinity:
            requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchExpressions:
                - key: clickhouse.altinity.com/chi
                  operator: In
                  values:
                  - prod-ch
              topologyKey: kubernetes.io/hostname
    volumeClaimTemplates:
    - name: ch-data-volume
      spec:
        storageClassName: gp3-ssd
        accessModes:
        - ReadWriteOnce
        resources:
          requests:
            storage: 1Ti
```

### 2.2 分片与副本拓扑图

```
┌──────────── Shard 0 ────────────┐  ┌──────────── Shard 1 ────────────┐  ┌──────────── Shard 2 ────────────┐
│  Replica 0 (leader)  │  Replica 1  │  │  Replica 0 (leader)  │  Replica 1  │  │  Replica 0 (leader)  │  Replica 1  │
│  prod-ch-0-0         │  prod-ch-0-1│  │  prod-ch-1-0         │  prod-ch-1-1│  │  prod-ch-2-0         │  prod-ch-2-1│
└──────────────────────┘  └────────────┘  └──────────────────────┘  └────────────┘  └──────────────────────┘  └────────────┘
         │                        │               │                        │               │                        │
         └───────── Replication ──┘               └───────── Replication ──┘               └───────── Replication ──┘
                   via Keeper                              via Keeper                              via Keeper
```

## 3. MergeTree 引擎优化

### 3.1 表引擎选型

| 引擎 | 适用场景 | 特点 |
|------|---------|------|
| MergeTree | 通用 OLAP | 基础引擎，支持分区、索引 |
| ReplacingMergeTree | 去重场景 | 同分区同排序键自动去重 |
| SummingMergeTree | 预聚合 | 同分区同键自动 SUM |
| AggregatingMergeTree | 复杂聚合 | 存储聚合状态 |
| ReplicatedMergeTree | 高可用 | 跨副本同步 |
| ReplicatedReplacingMergeTree | 高可用+去重 | 生产最常用 |

### 3.2 生产级建表语句

```sql
-- 分布式表 (写入入口)
CREATE TABLE analytics.events_distributed ON CLUSTER 'prod-cluster'
AS analytics.events_local
ENGINE = Distributed('prod-cluster', 'analytics', 'events_local', sipHash64(user_id));

-- 本地表 (每个分片)
CREATE TABLE analytics.events_local ON CLUSTER 'prod-cluster'
(
    event_date Date,
    event_time DateTime64(3),
    user_id UInt64,
    event_type LowCardinality(String),
    session_id String,
    properties String CODEC(ZSTD(3)),
    created_at DateTime DEFAULT now()
)
ENGINE = ReplicatedReplacingMergeTree('/clickhouse/tables/{shard}/events_local', '{replica}', event_time)
PARTITION BY toYYYYMM(event_date)
ORDER BY (user_id, event_type, event_time)
TTL event_date + INTERVAL 90 DAY
SETTINGS
    index_granularity = 8192,
    min_bytes_for_wide_part = 10485760,
    storage_policy = 'hot_and_cold',
    merge_with_ttl_timeout = 86400;

-- 跳数索引
ALTER TABLE analytics.events_local
ADD INDEX idx_event_type event_type TYPE set(100) GRANULARITY 4;

ALTER TABLE analytics.events_local
ADD INDEX idx_properties_props properties TYPE bloom_filter(0.01) GRANULARITY 4;
```

### 3.3 存储策略 - 热冷分离

```xml
<!-- /etc/clickhouse-server/config.d/storage.xml -->
<clickhouse>
  <storage_configuration>
    <disks>
      <hot>
        <path>/var/lib/clickhouse/hot/</path>
      </hot>
      <cold>
        <path>/var/lib/clickhouse/cold/</path>
      </cold>
    </disks>
    <policies>
      <hot_and_cold>
        <volumes>
          <hot>
            <disk>hot</disk>
            <max_data_part_size_bytes>10737418240</max_data_part_size_bytes>
          </hot>
          <cold>
            <disk>cold</disk>
          </cold>
        </volumes>
        <move_factor>0.1</move_factor>
      </hot_and_cold>
    </policies>
  </storage_configuration>
</clickhouse>
```

### 3.4 合并调优

```sql
-- 后台合并线程配置
SET background_pool_size = 16;
SET background_merges_mutations_concurrency_ratio = 4;
SET background_schedule_pool_size = 16;
SET background_fetches_pool_size = 8;
SET background_common_pool_size = 8;
SET background_move_pool_size = 8;

-- 监控合并状态
SELECT
    database,
    table,
    elapsed,
    progress,
    total_size_bytes_compressed,
    rows_read,
    rows_written
FROM system.merges
ORDER BY elapsed DESC;
```

## 4. 数据分布策略

### 4.1 分片键选择

```sql
-- 高基数列哈希分片（推荐）
-- 用户行为分析: user_id
-- 日志分析: tenant_id
-- 电商: order_id

-- 轮询分片（数据均匀但查询跨全部分片）
ENGINE = Distributed('prod-cluster', 'analytics', 'events_local', rand())

-- 按列值分片（同值同分片，适合本地聚合）
ENGINE = Distributed('prod-cluster', 'analytics', 'events_local', tenant_id)
```

### 4.2 Distributed 表配置

```xml
<!-- 跨分片查询优化 -->
<clickhouse>
  <distributed_product_mode>local</distributed_product_mode>
  <max_parallel_replicas>3</max_parallel_replicas>
  <prefer_localhost_replica>1</prefer_localhost_replica>
  <distributed_ddl_task_timeout>300</distributed_ddl_task_timeout>
</clickhouse>
```

## 5. ClickHouse Keeper 集群管理

### 5.1 替代 ZooKeeper

```yaml
apiVersion: clickhouse.altinity.com/v1
kind: ClickHouseInstallation
metadata:
  name: prod-ch-keeper
  namespace: analytics
spec:
  configuration:
    clusters:
    - name: keeper
      layout:
        shardsCount: 1
        replicasCount: 3
      templates:
        podTemplate: keeper-stable
    settings:
      keeper_server:
        tcp_port: 9181
        four_letter_word_white_list: "*"
        raft_configuration:
          - id: 1
            hostname: "prod-ch-keeper-keeper-0.prod-ch-keeper-keeper-headless.analytics.svc"
            port: 9234
          - id: 2
            hostname: "prod-ch-keeper-keeper-1.prod-ch-keeper-keeper-headless.analytics.svc"
            port: 9234
          - id: 3
            hostname: "prod-ch-keeper-keeper-2.prod-ch-keeper-keeper-headless.analytics.svc"
            port: 9234
  templates:
    podTemplates:
    - name: keeper-stable
      spec:
        containers:
        - name: clickhouse-keeper
          image: clickhouse/clickhouse-server:24.8
          resources:
            requests:
              memory: 4Gi
              cpu: "2"
            limits:
              memory: 4Gi
              cpu: "4"
          volumeMounts:
          - name: keeper-data
            mountPath: /var/lib/clickhouse
    volumeClaimTemplates:
    - name: keeper-data
      spec:
        storageClassName: gp3
        accessModes:
        - ReadWriteOnce
        resources:
          requests:
            storage: 50Gi
```

### 5.2 Keeper 监控

```sql
-- 检查 Keeper 状态
SELECT * FROM system.keeper;

-- 检查复制状态
SELECT
    database,
    table,
    is_leader,
    total_replicas,
    active_replicas,
    queue_size,
    inserts_in_queue,
    merges_in_queue
FROM system.replicas
WHERE is_leader = 1
ORDER BY queue_size DESC;
```

## 6. 监控告警

### 6.1 Prometheus 指标暴露

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: clickhouse-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      clickhouse.altinity.com/chi: prod-ch
  endpoints:
  - port: http
    path: /metrics
    interval: 30s
```

### 6.2 关键告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: clickhouse-alerts
  namespace: monitoring
spec:
  groups:
  - name: clickhouse
    rules:
    - alert: ClickHouseReplicationLag
      expr: |
        max by (cluster, shard) (
          ClickHouseMetrics_replicated_max_relative_delay_of_replicas
        ) > 100
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "ClickHouse 副本复制延迟超过 100 秒"
    - alert: ClickHouseTooManyConnections
      expr: ClickHouseMetrics_TCPConnection > 3000
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "ClickHouse TCP 连接数超过 3000"
    - alert: ClickHouseHighMemoryUsage
      expr: |
        ClickHouseAsyncMetrics_MemoryResident /
        (ClickHouseAsyncMetrics_OSMemoryTotal * 1024) > 0.85
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "ClickHouse 内存使用率超过 85%"
    - alert: ClickHouseMergeBacklog
      expr: ClickHouseMetrics_BackgroundMerges > 20
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "ClickHouse 合并积压超过 20"
```

## 7. 故障排查速查

| 问题 | 排查命令 | 常见原因 |
|------|---------|---------|
| 副本不同步 | `SELECT * FROM system.replicas` | Keeper 故障、网络分区 |
| 查询慢 | `SELECT * FROM system.processes` | 缺少索引、数据倾斜 |
| OOM Kill | Pod events + `system.metrics` | max_memory_usage 设置过高 |
| 合并阻塞 | `SELECT * FROM system.merges` | 磁盘空间不足、合并线程满 |
| 写入报错 | 检查分布式表队列 `system.distribution_queue` | 目标分片不可达 |
| TTL 不生效 | `SELECT * FROM system.parts WHERE ttl_info != ''` | merge_with_ttl_timeout 过大 |


<!-- risk-assessed -->
