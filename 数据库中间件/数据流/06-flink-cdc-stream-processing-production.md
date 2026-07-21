---
title: Apache Flink CDC and Stream Processing on Kubernetes — Production Patterns
description: K8s 上 Flink 流处理 — Flink CDC、Flink Kubernetes Operator、状态管理、Exactly-Once、性能调优、监控
summary: 使用 Flink Kubernetes Operator 运行生产级流处理与 CDC 管道的完整实践
category: practice
tags:
- flink
- cdc
- stream-processing
- operator
- exactly-once
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: database
---
# Flink CDC 与流处理 Kubernetes 生产实践

> 使用 Flink Kubernetes Operator 构建生产级实时数据管道。

## 架构概览

```
┌──────────┐     ┌──────────┐     ┌──────────────────────┐
│ MySQL    │────▶│ Debezium │────▶│  Flink CDC Job       │
│ PostgreSQL│    │ / CDC    │     │  (Source → Transform │
│ MongoDB  │     │ Connector│     │   → Sink)            │
└──────────┘     └──────────┘     └──────────┬───────────┘
                                              │
                    ┌─────────────────────────┼─────────────────┐
                    ▼                         ▼                 ▼
             ┌──────────┐            ┌──────────┐      ┌──────────┐
             │ Kafka    │            │ ES/OS    │      │ Data     │
             │ (下游)   │            │ (搜索)   │      │ Lake     │
             └──────────┘            └──────────┘      └──────────┘
```

## Flink Kubernetes Operator 部署

```bash
# 安装 Flink Kubernetes Operator
helm repo add flink-operator-repo https://downloads.apache.org/flink/flink-kubernetes-operator-1.9.0/
helm install flink-operator flink-operator-repo/flink-kubernetes-operator \
  --namespace flink-system --create-namespace \
  --set watchNamespaces="{data-pipeline,analytics}"
```

## Flink CDC 作业部署

### FlinkDeployment CRD

```yaml
apiVersion: flink.apache.org/v1beta1
kind: FlinkDeployment
metadata:
  name: cdc-mysql-to-kafka
  namespace: data-pipeline
spec:
  image: registry.example.com/flink-cdc:1.18-scala_2.12
  flinkVersion: v1_18
  serviceAccount: flink
  jobManager:
    resource:
      memory: 2048m
      cpu: 1
    replicas: 1
  taskManager:
    resource:
      memory: 4096m
      cpu: 2
    replicas: 3
  flinkConfiguration:
    # 状态后端
    state.backend: rocksdb
    state.backend.rocksdb.memory.managed: "true"
    state.checkpoints.dir: s3://flink-checkpoints/cdc-mysql-to-kafka/
    state.savepoints.dir: s3://flink-savepoints/cdc-mysql-to-kafka/
    # Checkpoint 配置
    execution.checkpointing.interval: "60s"
    execution.checkpointing.mode: EXACTLY_ONCE
    execution.checkpointing.min-pause: "30s"
    execution.checkpointing.timeout: "600s"
    execution.checkpointing.max-concurrent-checkpoints: "1"
    # 重启策略
    restart-strategy: fixed-delay
    restart-strategy.fixed-delay.attempts: "3"
    restart-strategy.fixed-delay.delay: "30s"
    # 网络
    taskmanager.network.memory.fraction: "0.2"
    taskmanager.memory.network.min: "128mb"
    taskmanager.memory.network.max: "1gb"
    # RocksDB
    state.backend.rocksdb.block.cache-size: "256mb"
    state.backend.incremental: "true"
  job:
    jarURI: local:///opt/flink/usrlib/cdc-pipeline.jar
    parallelism: 3
    upgradeMode: savepoint
    state: running
    args:
      - --source.host
      - mysql-primary.database.svc
      - --source.port
      - "3306"
      - --source.database
      - orders
      - --sink.bootstrap.servers
      - kafka:9092
      - --sink.topic
      - cdc-orders
```

### Flink CDC SQL 作业（推荐）

```yaml
apiVersion: flink.apache.org/v1beta1
kind: FlinkDeployment
metadata:
  name: cdc-sql-pipeline
  namespace: data-pipeline
spec:
  image: registry.example.com/flink-sql-cdc:1.18
  flinkVersion: v1_18
  serviceAccount: flink
  jobManager:
    resource:
      memory: 2048m
      cpu: 1
  taskManager:
    resource:
      memory: 8192m
      cpu: 4
    replicas: 4
  flinkConfiguration:
    state.backend: rocksdb
    state.checkpoints.dir: s3://flink-checkpoints/cdc-sql/
    execution.checkpointing.interval: "30s"
    execution.checkpointing.mode: EXACTLY_ONCE
    state.backend.incremental: "true"
    table.exec.state.ttl: "86400000"  # 状态 TTL 24h
  job:
    jarURI: local:///opt/flink/usrlib/flink-sql-runner.jar
    parallelism: 4
    upgradeMode: savepoint
    state: running
    args:
      - /opt/flink/usrlib/pipeline.sql
```

```sql
-- pipeline.sql — Flink CDC SQL 管道
-- MySQL CDC Source
CREATE TABLE orders_cdc (
  id BIGINT,
  user_id BIGINT,
  amount DECIMAL(10, 2),
  status STRING,
  created_at TIMESTAMP(3),
  updated_at TIMESTAMP(3),
  PRIMARY KEY (id) NOT ENFORCED
) WITH (
  'connector' = 'mysql-cdc',
  'hostname' = 'mysql-primary.database.svc',
  'port' = '3306',
  'username' = 'flink_cdc',
  'password' = '${MYSQL_CDC_PASSWORD}',
  'database-name' = 'orders',
  'table-name' = 'orders',
  'server-time-zone' = 'Asia/Shanghai',
  'scan.incremental.snapshot.enabled' = 'true',
  'scan.incremental.snapshot.chunk.size' = '8096'
);

-- Kafka Sink（变更事件）
CREATE TABLE orders_sink (
  id BIGINT,
  user_id BIGINT,
  amount DECIMAL(10, 2),
  status STRING,
  created_at TIMESTAMP(3),
  updated_at TIMESTAMP(3),
  PRIMARY KEY (id) NOT ENFORCED
) WITH (
  'connector' = 'upsert-kafka',
  'topic' = 'cdc-orders',
  'properties.bootstrap.servers' = 'kafka:9092',
  'key.format' = 'json',
  'value.format' = 'json',
  'value.json.timestamp-format.standard' = 'ISO-8601'
);

-- Elasticsearch Sink（搜索索引）
CREATE TABLE orders_es (
  id BIGINT,
  user_id BIGINT,
  amount DECIMAL(10, 2),
  status STRING,
  created_at TIMESTAMP(3),
  PRIMARY KEY (id) NOT ENFORCED
) WITH (
  'connector' = 'elasticsearch-8',
  'hosts' = 'https://logs-es-http.logging.svc:9200',
  'index' = 'orders',
  'format' = 'json'
);

-- 实时聚合（物化视图）
CREATE TABLE order_stats (
  user_id BIGINT,
  order_count BIGINT,
  total_amount DECIMAL(12, 2),
  last_order_time TIMESTAMP(3),
  PRIMARY KEY (user_id) NOT ENFORCED
) WITH (
  'connector' = 'jdbc',
  'url' = 'jdbc:postgresql://analytics-db:5432/analytics',
  'table-name' = 'user_order_stats',
  'username' = 'flink',
  'password' = '${PG_PASSWORD}'
);

-- 执行管道
INSERT INTO orders_sink SELECT * FROM orders_cdc;
INSERT INTO orders_es SELECT id, user_id, amount, status, created_at FROM orders_cdc;
INSERT INTO order_stats
SELECT
  user_id,
  COUNT(*) as order_count,
  SUM(amount) as total_amount,
  MAX(created_at) as last_order_time
FROM orders_cdc
WHERE status != 'cancelled'
GROUP BY user_id;
```

## 状态管理

### Checkpoint 与 Savepoint

```bash
# 手动触发 Savepoint（升级前）
kubectl patch flinkdeployment cdc-sql-pipeline -n data-pipeline \
  --type='json' \
  -p='[{"op":"replace","path":"/spec/job/upgradeMode","value":"savepoint"}]'

# 从 Savepoint 恢复
# 修改 FlinkDeployment spec:
#   job.state: running
#   job.upgradeMode: savepoint
# Operator 自动从最新 savepoint 恢复

# 查看 Checkpoint 状态
kubectl exec -it cdc-sql-pipeline-jobmanager-0 -n data-pipeline -- \
  curl -s http://localhost:8081/jobs/overview | jq '.jobs[0]'
```

### 状态 TTL 配置

```yaml
# 防止状态无限增长
flinkConfiguration:
  table.exec.state.ttl: "86400000"  # 24h（聚合状态）
  # 或按算子设置
  # table.exec.state.ttl.agg: "3600000"  # 聚合 1h
  # table.exec.state.ttl.join: "7200000"  # Join 2h
```

## 性能调优

| 参数 | 推荐值 | 说明 |
|------|--------|------|
| parallelism | = Kafka 分区数 | 最大并行度 |
| taskmanager.memory | 4-16 GB | 含 RocksDB 状态 |
| checkpoint interval | 30-120s | 平衡恢复时间与开销 |
| state.backend.incremental | true | RocksDB 增量检查点 |
| taskmanager.numberOfTaskSlots | 2-4 | 每 TM 槽位数 |
| network buffers | 20% 内存 | 网络缓冲 |

### 反压排查

```bash
# Flink Web UI
kubectl port-forward svc/cdc-sql-pipeline-rest -n data-pipeline 8081:8081
# 访问 http://localhost:8081 → Jobs → 查看 BackPressure 标签

# 指标
# flink_taskmanager_job_task_operator_numRecordsInPerSecond
# flink_taskmanager_job_task_operator_numRecordsOutPerSecond
# 如果 Out << In → 该算子是瓶颈
```

## 监控告警

```yaml
# Prometheus 告警规则
- alert: FlinkCheckpointFailed
  expr: flink_jobmanager_job_numberOfFailedCheckpoints > 0
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "Flink 作业 Checkpoint 失败"

- alert: FlinkJobNotRunning
  expr: flink_jobmanager_job_uptime == 0
  for: 2m
  labels:
    severity: critical
  annotations:
    summary: "Flink 作业停止运行"

- alert: FlinkBackpressure
  expr: flink_taskmanager_job_task_backPressuredTimeMsPerSecond > 500
  for: 10m
  labels:
    severity: warning
  annotations:
    summary: "Flink 算子反压严重"
```

## 故障排查

| 症状 | 原因 | 排查 |
|------|------|------|
| Checkpoint 超时 | 状态过大/网络慢 | 增大 timeout + 增量检查点 |
| OOM | 状态未设 TTL/数据倾斜 | 设置 state.ttl + 检查 keyBy |
| 数据延迟 | 反压/并行度不足 | 增加 parallelism + 排查瓶颈 |
| CDC 断连 | Binlog 被清理 | 检查 MySQL binlog 保留 |
| Savepoint 失败 | 磁盘空间/权限 | 检查 S3 路径 |

```bash
# 作业状态
kubectl get flinkdeployment -n data-pipeline
kubectl describe flinkdeployment cdc-sql-pipeline -n data-pipeline

# JobManager 日志
kubectl logs cdc-sql-pipeline-jobmanager-0 -n data-pipeline --tail=100

# TaskManager 日志
kubectl logs cdc-sql-pipeline-taskmanager-0 -n data-pipeline --tail=100

# Flink REST API
kubectl exec -it cdc-sql-pipeline-jobmanager-0 -n data-pipeline -- \
  curl -s http://localhost:8081/jobs | jq
```

## Related

- [[数据库中间件/数据流/index.md|数据流]]
- [[数据库中间件/数据流/03-flink-on-kubernetes.md|Flink on K8s]]
- [[数据库中间件/数据流/04-debezium-cdc-kubernetes.md|Debezium CDC]]
