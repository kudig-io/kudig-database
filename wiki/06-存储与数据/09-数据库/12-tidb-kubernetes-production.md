---
title: TiDB on Kubernetes 生产部署
description: 'TiDB Operator 架构、PD/TiKV/TiDB 组件管理、Lightning 迁移、TiCDC、扩缩容与备份恢复'
summary: 'TiDB Operator 架构、PD/TiKV/TiDB 组件管理、Lightning 迁移、TiCDC、扩缩容与备份恢复'
category: database-middleware
tags:
- database
- k8s
- tidb
- newsql
- htap
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
- TiDB on Kubernetes 生产部署 是什么
- 如何 TiDB on Kubernetes 生产部署
trigger_keywords:
- tidb
- tikv
- pd
- ticdc
- lightning
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


# TiDB on Kubernetes 生产部署

## 1. TiDB Operator 架构

### 1.1 组件总览

```
┌─────────────────────────────────────────────────────────────┐
│                    TiDB Operator                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │ TidbCluster│  │TidbMonitor│  │TidbBackup│  │TidbImport│    │
│  │ Controller │  │ Controller│  │ Controller│  │ Controller│    │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘    │
└────────────────────────┬────────────────────────────────────┘
                         │ Reconcile
    ┌────────────────────┼────────────────────┐
    ▼                    ▼                    ▼
┌─────────┐       ┌──────────┐         ┌──────────┐
│    PD    │       │   TiKV   │         │   TiDB   │
│ (3 节点) │       │ (3+ 节点) │         │ (无状态)  │
│ 调度/元数据│       │ 存储引擎  │         │ SQL 层    │
└─────────┘       └──────────┘         └──────────┘
```

### 1.2 安装 TiDB Operator

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 PingCAP Helm 仓库
helm repo add pingcap https://charts.pingcap.org/
helm repo update

# 安装 CRD
kubectl create -f https://raw.githubusercontent.com/pingcap/tidb-operator/v1.6.0/manifests/crd.yaml

# 安装 Operator
helm install tidb-operator pingcap/tidb-operator \
  --namespace tidb-admin --create-namespace \
  --version v1.6.0 \
  --set operator.replicas=2 \
  --set scheduler.replicas=2 \
  --set admissionWebhook.create=true

# 验证
kubectl get pods -n tidb-admin
```
## 2. TiDB 集群部署

### 2.1 生产级 TidbCluster CR

```yaml
apiVersion: pingcap.com/v1alpha1
kind: TidbCluster
metadata:
  name: prod-tidb
  namespace: tidb
spec:
  version: v8.5.0
  pvReclaimPolicy: Retain
  timezone: Asia/Shanghai
  # PD 配置
  pd:
    baseImage: pingcap/pd
    version: v8.5.0
    replicas: 3
    requests:
      storage: 20Gi
    config: |
      [schedule]
        max-merge-region-size = 20
        max-merge-region-keys = 200000
        split-merge-interval = "1h"
        max-snapshot-count = 3
        max-pending-peer-count = 16
      [replication]
        max-replicas = 3
        location-labels = ["region", "zone", "rack"]
    nodeSelector:
      node-role: tidb-pd
    tolerations:
    - key: dedicated
      value: tidb
      effect: NoSchedule
    storageClassName: gp3
    resources:
      requests:
        cpu: "1"
        memory: 2Gi
      limits:
        cpu: "2"
        memory: 4Gi
  # TiKV 配置
  tikv:
    baseImage: pingcap/tikv
    version: v8.5.0
    replicas: 3
    requests:
      storage: 500Gi
    config: |
      [server]
        grpc-concurrency = 8
        grpc-raft-conn-num = 16
      [storage]
        block-cache.capacity = "12GB"
      [raftstore]
        raft-db-path = "/var/lib/tikv/raft"
        capacity = "480GB"
        region-split-size = "256MB"
      [coprocessor]
        region-split-keys = 2560000
    nodeSelector:
      node-role: tikv
    tolerations:
    - key: dedicated
      value: tidb
      effect: NoSchedule
    storageClassName: gp3-ssd
    resources:
      requests:
        cpu: "8"
        memory: 32Gi
      limits:
        cpu: "16"
        memory: 48Gi
  # TiDB 配置
  tidb:
    baseImage: pingcap/tidb
    version: v8.5.0
    replicas: 3
    service:
      type: LoadBalancer
      annotations:
        service.beta.kubernetes.io/aws-load-balancer-type: nlb
        service.beta.kubernetes.io/aws-load-balancer-scheme: internal
    config: |
      [log]
        slow-threshold = 200
        [performance]
        max-procs = 0
        max-memory = 0
        tcp-keep-alive = true
      [prepared-plan-cache]
        enabled = true
        capacity = 1000
    nodeSelector:
      node-role: tidb
    resources:
      requests:
        cpu: "4"
        memory: 8Gi
      limits:
        cpu: "8"
        memory: 16Gi
```

### 2.2 组件间通信

```
Client (MySQL Protocol)
    │
    ▼
TiDB Service (LoadBalancer :4000)
    │
    ├── SQL Parse → Plan → Execute
    │       │
    │       ▼
    │   PD Client (:2379)  ← 元数据查询/TSO 获取
    │       │
    │       ▼
    │   TiKV Client ← 读写数据 (gRPC :20160)
    │
    └── 结果返回客户端
```

## 3. TiDB Lightning 数据迁移

### 3.1 Lightning 配置

```yaml
apiVersion: pingcap.com/v1alpha1
kind: TidbLightning
metadata:
  name: prod-lightning
  namespace: tidb
spec:
  image: pingcap/tidb-lightning:v8.5.0
  version: v8.5.0
  cluster:
    name: prod-tidb
  resources:
    requests:
      cpu: "8"
      memory: 16Gi
    limits:
      cpu: "16"
      memory: 32Gi
  config: |
    [lightning]
      level = "info"
      file = "/var/log/tidb-lightning.log"
      check-requirements = true
    [tikv-importer]
      backend = "local"
      sorted-kv-dir = "/tmp/sorted-kv"
      disk-quota = "1TB"
    [mydumper]
      data-source-dir = "/data/backup"
      filter = ["*.*"]
    [tidb]
      host = "prod-tidb-tidb"
      port = 4000
      user = "root"
      password = ""
      status-port = 10080
      pd-addr = "prod-tidb-pd:2379"
    [checkpoint]
      enable = true
      driver = "file"
      dsn = "/tmp/tidb_lightning_checkpoint.pb"
    [post-restore]
      level-1-compact = true
      post-restore-analyze = true
```

### 3.2 导入流程

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 准备数据 (SQL dump / CSV)
# 2. 创建 Lightning CR
kubectl apply -f lightning.yaml

# 3. 监控导入进度
kubectl logs -f -n tidb prod-lightning-0

# 4. 验证数据完整性
kubectl exec -n tidb prod-tidb-tidb-0 -- \
  mysql -h 127.0.0.1 -P 4000 -u root \
  -e "SELECT COUNT(*) FROM target_db.target_table;"

# 5. 清理 Lightning 资源
kubectl delete tidblightning prod-lightning -n tidb
```
## 4. TiCDC 变更数据捕获

### 4.1 TiCDC 集群部署

```yaml
apiVersion: pingcap.com/v1alpha1
kind: TidbCluster
metadata:
  name: prod-tidb
  namespace: tidb
spec:
  # ... TiDB 集群配置 ...
  ticdc:
    baseImage: pingcap/ticdc
    version: v8.5.0
    replicas: 3
    config: |
      [capture]
        capture-keepalive-ttl = "120s"
      [sorter]
        max-memory-percentage = 30
        sort-dir = "/tmp/cdc/sorter"
    resources:
      requests:
        cpu: "4"
        memory: 8Gi
      limits:
        cpu: "8"
        memory: 16Gi
```

### 4.2 创建 Changefeed

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 通过 CLI 创建 Changefeed
kubectl exec -n tidb prod-tidb-ticdc-0 -- \
  /cdc cli changefeed create \
    --pd="http://prod-tidb-pd:2379" \
    --sink-uri="kafka://kafka-cluster-kafka-bootstrap.messaging:9092/tidb-cdc-topic?protocol=canal-json" \
    --changefeed-id="mysql-to-kafka" \
    --start-ts=0 \
    --config='{
      "filter": {
        "rules": ["target_db.*"]
      },
      "mounter": {
        "worker-num": 16
      },
      "sink": {
        "dispatchers": [
          {"matcher": ["target_db.*"], "dispatcher": "ts"}
        ]
      }
    }'

# 查看 Changefeed 状态
kubectl exec -n tidb prod-tidb-ticdc-0 -- \
  /cdc cli changefeed list --pd="http://prod-tidb-pd:2379"
```
### 4.3 TiCDC → Kafka 目标配置

| 目标 | sink-uri 格式 | 协议 |
|------|-------------|------|
| Kafka | `kafka://broker/topic?protocol=canal-json` | canal-json / avro / maxwell |
| MySQL | `mysql://user:pass@host:3306/` | - |
| 对象存储 | `s3://bucket/prefix?protocol=csv` | csv / canal-json |
| Pulsar | `pulsar://broker/topic` | canal-json |

## 5. 扩缩容

### 5.1 水平扩缩容

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 扩容 TiKV 节点
kubectl patch tc prod-tidb -n tidb --type merge \
  -p '{"spec":{"tikv":{"replicas":5}}}'

# 扩容 TiDB 节点
kubectl patch tc prod-tidb -n tidb --type merge \
  -p '{"spec":{"tidb":{"replicas":5}}}'

# 监控扩缩容进度
kubectl get pods -n tidb -l app.kubernetes.io/component=tikv -w
kubectl get pods -n tidb -l app.kubernetes.io/component=tidb -w

# 缩容 TiKV (Operator 自动执行 store evict)
kubectl patch tc prod-tidb -n tidb --type merge \
  -p '{"spec":{"tikv":{"replicas":3}}}'
```
### 5.2 滚动升级

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 升级 TiDB 版本
kubectl patch tc prod-tidb -n tidb --type merge \
  -p '{"spec":{"version":"v8.5.1"}}'

# 监控滚动升级
kubectl get tc prod-tidb -n tidb -o jsonpath='{.status.tikv.statefulSet}' | jq
kubectl get events -n tidb --field-selector reason=SuccessfulUpdate
```
### 5.3 TiKV 节点替换

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 当某个 TiKV 节点故障时手动下线
kubectl exec -n tidb prod-tidb-pd-0 -- \
  /pd-ctl -u http://127.0.0.1:2379 store delete <store-id>

# Operator 会自动创建新 Pod 替代
```
## 6. BR 备份恢复

### 6.1 S3 备份配置

```yaml
apiVersion: pingcap.com/v1alpha1
kind: Backup
metadata:
  name: prod-tidb-full-backup
  namespace: tidb
spec:
  br:
    cluster: prod-tidb
    clusterNamespace: tidb
    sendCredToTikv: true
  s3:
    provider: aws
    region: us-east-1
    bucket: tidb-backups
    prefix: prod-tidb/full
    secretName: s3-credentials
  storageClassName: gp3
  storageSize: 500Gi
  resources:
    requests:
      cpu: "4"
      memory: 8Gi
    limits:
      cpu: "8"
      memory: 16Gi
```

### 6.2 定时备份 CronJob

```yaml
apiVersion: pingcap.com/v1alpha1
kind: BackupSchedule
metadata:
  name: prod-tidb-daily-backup
  namespace: tidb
spec:
  maxBackups: 7
  backupTemplate:
    br:
      cluster: prod-tidb
      clusterNamespace: tidb
      sendCredToTikv: true
      backupType: full
    s3:
      provider: aws
      region: us-east-1
      bucket: tidb-backups
      prefix: prod-tidb/daily
      secretName: s3-credentials
    storageClassName: gp3
    storageSize: 500Gi
  schedule: "0 2 * * *"
```

### 6.3 恢复流程

```yaml
apiVersion: pingcap.com/v1alpha1
kind: Restore
metadata:
  name: prod-tidb-restore
  namespace: tidb
spec:
  br:
    cluster: prod-tidb
    clusterNamespace: tidb
    sendCredToTikv: true
    backupType: full
  s3:
    provider: aws
    region: us-east-1
    bucket: tidb-backups
    prefix: prod-tidb/full/2026-07-02
    secretName: s3-credentials
  storageClassName: gp3
  storageSize: 500Gi
```

## 7. 监控告警

### 7.1 TiDB Monitor

```yaml
apiVersion: pingcap.com/v1alpha1
kind: TidbMonitor
metadata:
  name: prod-tidb-monitor
  namespace: tidb
spec:
  clusters:
  - name: prod-tidb
  prometheus:
    baseImage: prom/prometheus
    version: v2.51.0
    resources:
      requests:
        cpu: "1"
        memory: 2Gi
      limits:
        cpu: "2"
        memory: 4Gi
    retention: 30d
  grafana:
    baseImage: grafana/grafana
    version: 10.3.1
    resources:
      requests:
        cpu: 500m
        memory: 1Gi
  alertmanager:
    baseImage: prom/alertmanager
    version: v0.27.0
    replicas: 2
  initializer:
    baseImage: pingcap/tidb-monitor-initializer
    version: v8.5.0
```

### 7.2 关键告警规则

| 告警名 | 条件 | 严重性 |
|-------|------|-------|
| TiKV 节点 Down | `tikv_store_status == 0` 持续 5 分钟 | critical |
| PD Leader 切换 | `pd_cluster_status{type="leader"}` 变化 | warning |
| TiDB 连接数过高 | `tidb_server_connections > 1000` | warning |
| Region Unavailable | `pd_regions_status{type="unavailable"} > 0` | critical |
| TiKV CPU > 90% | `tikv_thread_cpu_seconds_total > 90%` | warning |
| 备份失败 | `br_status{status="failed"}` | critical |

## 8. 故障排查速查

| 问题 | 排查命令 | 常见原因 |
|------|---------|---------|
| TiKV 节点 Down | `pd-ctl store` | 磁盘满、OOM、网络分区 |
| Region 无 Leader | `pd-ctl region` | TiKV 节点故障、网络抖动 |
| 查询慢 | `ADMIN SHOW SLOW` | 缺少索引、热点 Region |
| 数据不一致 | `ADMIN CHECK TABLE` | 副本损坏，需修复 |
| CDC 延迟高 | `cdc cli changefeed query` | Kafka 积压、网络瓶颈 |
| Lightning 失败 | 检查 Lightning 日志 | 数据格式错误、磁盘空间不足 |


<!-- risk-assessed -->
