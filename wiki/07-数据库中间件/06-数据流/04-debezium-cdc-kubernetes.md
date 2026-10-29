---
title: Debezium CDC on Kubernetes
description: 'Kafka Connect 集群部署、Debezium Source Connector 配置、Schema Evolution、监控与性能调优'
summary: 'Kafka Connect 集群部署、Debezium Source Connector 配置、Schema Evolution、监控与性能调优'
category: database-middleware
tags:
- database
- k8s
- debezium
- cdc
- kafka-connect
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
- Debezium CDC on Kubernetes 是什么
- 如何 Debezium CDC on Kubernetes
trigger_keywords:
- debezium
- cdc
- kafka-connect
- change-data-capture
- schema-evolution
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


# Debezium CDC on Kubernetes

## 1. CDC 架构总览

```
┌─────────────────────────────────────────────────────────────────┐
│                      Source Database                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │  MySQL   │  │PostgreSQL│  │ MongoDB  │  │  Oracle  │       │
│  │ binlog   │  │  WAL     │  │ oplog    │  │  redo    │       │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘       │
└───────┼──────────────┼──────────────┼──────────────┼────────────┘
        │              │              │              │
        ▼              ▼              ▼              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Kafka Connect (Debezium)                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ MySQL Source │  │ PG Source    │  │Mongo Source  │          │
│  │ Connector    │  │ Connector    │  │ Connector    │          │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘          │
└─────────┼─────────────────┼─────────────────┼───────────────────┘
          │                 │                 │
          ▼                 ▼                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Kafka Cluster                            │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐           │
│  │ Topic A │  │ Topic B │  │ Topic C │  │SchemaReg│           │
│  │ (CDC)   │  │ (CDC)   │  │ (CDC)   │  │         │           │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘           │
└─────────────────────────────────────────────────────────────────┘
          │                 │                 │
          ▼                 ▼                 ▼
    ┌──────────┐     ┌──────────┐     ┌──────────┐
    │ Flink    │     │ Elastic  │     │ Data     │
    │ Stream   │     │ Search   │     │ Warehouse│
    └──────────┘     └──────────┘     └──────────┘
```

## 2. Kafka Connect 集群部署

### 2.1 Strimzi Kafka Connect 集群

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaConnect
metadata:
  name: debezium-connect
  namespace: messaging
  annotations:
    strimzi.io/use-connector-resources: "true"
spec:
  version: 3.7.0
  replicas: 3
  image: quay.io/debezium/connect:2.5
  bootstrapServers: kafka-cluster-kafka-bootstrap.messaging:9092
  config:
    # 连接器配置
    group.id: debezium-connect-cluster
    offset.storage.topic: debezium-connect-offsets
    offset.storage.replication.factor: 3
    offset.storage.partitions: 25
    config.storage.topic: debezium-connect-configs
    config.storage.replication.factor: 3
    status.storage.topic: debezium-connect-status
    status.storage.replication.factor: 3
    config.storage.replication.factor: 3
    # 转换器
    key.converter: io.confluent.connect.avro.AvroConverter
    key.converter.schema.registry.url: http://schema-registry.messaging:8081
    value.converter: io.confluent.connect.avro.AvroConverter
    value.converter.schema.registry.url: http://schema-registry.messaging:8081
    # 性能优化
    producer.max.request.size: 10485760
    producer.buffer.memory: 67108864
    producer.batch.size: 65536
    producer.linger.ms: 5
    producer.compression.type: lz4
    # 协调器
    scheduled.rebalance.max.delay.ms: 60000
    connect.protocol: eager
  resources:
    requests:
      cpu: "2"
      memory: 4Gi
    limits:
      cpu: "4"
      memory: 8Gi
  build:
    output:
      type: docker
      image: registry.example.com/debezium-connect:latest
    plugins:
    - name: debezium-mysql
      artifacts:
      - type: tgz
        url: https://repo1.maven.org/maven2/io/debezium/debezium-connector-mysql/2.5.4/debezium-connector-mysql-2.5.4-plugin.tar.gz
    - name: debezium-postgres
      artifacts:
      - type: tgz
        url: https://repo1.maven.org/maven2/io/debezium/debezium-connector-postgres/2.5.4/debezium-connector-postgres-2.5.4-plugin.tar.gz
    - name: debezium-mongodb
      artifacts:
      - type: tgz
        url: https://repo1.maven.org/maven2/io/debezium/debezium-connector-mongodb/2.5.4/debezium-connector-mongodb-2.5.4-plugin.tar.gz
    - name: avro-converter
      artifacts:
      - type: tgz
        url: https://packages.confluent.io/maven/io/confluent/kafka-connect-avro-converter/7.6.0/kafka-connect-avro-converter-7.6.0.tar.gz
  template:
    pod:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  strimzi.io/name: debezium-connect-connect
              topologyKey: kubernetes.io/hostname
    connectContainer:
      env:
      - name: JAVA_OPTS
        value: "-Xms2g -Xmx4g -XX:+UseG1GC"
```

### 2.2 部署验证

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Connect 集群状态
kubectl get kafkaconnect -n messaging
kubectl get pods -n messaging -l strimzi.io/cluster=debezium-connect

# 查看连接器插件
kubectl exec -n messaging debezium-connect-connect-0 -- \
  curl -s http://localhost:8083/connector-plugins | jq

# 查看 Connect 集群信息
kubectl exec -n messaging debezium-connect-connect-0 -- \
  curl -s http://localhost:8083/connectors | jq
```
## 3. Debezium Source Connector 配置

### 3.1 MySQL Source Connector

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaConnector
metadata:
  name: mysql-source-connector
  namespace: messaging
  labels:
    strimzi.io/cluster: debezium-connect
spec:
  class: io.debezium.connector.mysql.MySqlConnector
  tasksMax: 3
  config:
    # 数据库连接
    database.hostname: mysql.database.svc.cluster.local
    database.port: 3306
    database.user: debezium
    database.password: ${file:/opt/kafka/external-configuration/db-credentials/password}
    database.server.id: 10001
    database.server.name: mysql-prod

    # 捕获配置
    database.include.list: app_db,order_db
    table.include.list: app_db.users,app_db.orders,order_db.transactions
    database.history.kafka.bootstrap.servers: kafka-cluster-kafka-bootstrap.messaging:9092
    database.history.kafka.topic: schema-changes.mysql-prod

    # 快照配置
    snapshot.mode: initial
    snapshot.locking.mode: minimal
    snapshot.select.statement.overrides: app_db.users

    # Binlog 配置
    binlog.buffer.size: 8388608
    binlog.format: ROW
    binlog.row.image: FULL

    # GTID 配置 (MySQL 5.6+)
    gtid.enabled: true
    gtid.source.includes: mysql-prod

    # 时区
    time.precision.mode: connect
    decimal.handling.mode: precise

    # Topic 命名
    topic.naming.strategy: io.debezium.schema.DefaultTopicNamingStrategy
    topic.prefix: cdc

    # 转换器
    key.converter: io.confluent.connect.avro.AvroConverter
    key.converter.schema.registry.url: http://schema-registry.messaging:8081
    value.converter: io.confluent.connect.avro.AvroConverter
    value.converter.schema.registry.url: http://schema-registry.messaging:8081

    # 消息转换 (SMT)
    transforms: unwrap,route
    transforms.unwrap.type: io.debezium.transforms.ExtractNewRecordState
    transforms.unwrap.drop.tombstones: false
    transforms.unwrap.delete.handling.mode: rewrite
    transforms.unwrap.add.fields: "op,table,source.ts_ms"
    transforms.route.type: org.apache.kafka.connect.transforms.RegexRouter
    transforms.route.regex: "cdc\\.mysql-prod\\.(.*)"
    transforms.route.replacement: "cdc.$1"

    # 错误处理
    errors.log.enable: true
    errors.log.include.messages: true
    errors.tolerance: none

    # 心跳
    heartbeat.interval.ms: 300000
```

### 3.2 PostgreSQL Source Connector

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaConnector
metadata:
  name: postgres-source-connector
  namespace: messaging
  labels:
    strimzi.io/cluster: debezium-connect
spec:
  class: io.debezium.connector.postgresql.PostgresConnector
  tasksMax: 3
  config:
    # 数据库连接
    database.hostname: postgres.database.svc.cluster.local
    database.port: 5432
    database.user: debezium
    database.password: ${file:/opt/kafka/external-configuration/db-credentials/password}
    database.dbname: app_db

    # 捕获配置
    schema.include.list: public
    table.include.list: public.users,public.orders,public.products
    slot.name: debezium_slot
    publication.name: debezium_publication

    # WAL 配置
    plugin.name: pgoutput
    wal.level: logical

    # 快照配置
    snapshot.mode: initial
    snapshot.select.statement.overrides: public.users

    # Topic 命名
    topic.prefix: cdc-pg
    topic.naming.strategy: io.debezium.schema.DefaultTopicNamingStrategy

    # 数据类型映射
    time.precision.mode: connect
    decimal.handling.mode: precise
    hstore.handling.mode: json
    interval.handling.mode: numeric
    timezone: UTC

    # 转换器
    key.converter: io.confluent.connect.avro.AvroConverter
    key.converter.schema.registry.url: http://schema-registry.messaging:8081
    value.converter: io.confluent.connect.avro.AvroConverter
    value.converter.schema.registry.url: http://schema-registry.messaging:8081

    # SMT
    transforms: unwrap
    transforms.unwrap.type: io.debezium.transforms.ExtractNewRecordState
    transforms.unwrap.drop.tombstones: false
    transforms.unwrap.add.fields: "op,table,lsn,source.ts_ms"

    # 错误处理
    errors.log.enable: true
    errors.tolerance: none
```

### 3.3 MongoDB Source Connector

```yaml
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaConnector
metadata:
  name: mongodb-source-connector
  namespace: messaging
  labels:
    strimzi.io/cluster: debezium-connect
spec:
  class: io.debezium.connector.mongodb.MongoDbConnector
  tasksMax: 1
  config:
    # MongoDB 连接
    mongodb.connection.string: mongodb://mongo-0.mongo-headless,mongo-1.mongo-headless,mongo-2.mongo-headless/?replicaSet=rs0
    mongodb.user: debezium
    mongodb.password: ${file:/opt/kafka/external-configuration/db-credentials/password}

    # 捕获配置
    mongodb.name: mongodb-prod
    collection.include.list: app_db.users,app_db.orders

    # 变更流配置
    capture.mode: change_streams_update_full
    capture.scope: deployment

    # Topic 命名
    topic.prefix: cdc-mongo

    # 转换器
    key.converter: io.confluent.connect.avro.AvroConverter
    key.converter.schema.registry.url: http://schema-registry.messaging:8081
    value.converter: io.confluent.connect.avro.AvroConverter
    value.converter.schema.registry.url: http://schema-registry.messaging:8081

    # SMT
    transforms: unwrap
    transforms.unwrap.type: io.debezium.transforms.ExtractNewRecordState
    transforms.unwrap.add.fields: "op,ts_ms"
```

## 4. Schema Evolution 策略

### 4.1 兼容性级别

| 级别 | 含义 | 适用场景 |
|------|------|---------|
| BACKWARD | 新 schema 可读旧数据 | 默认推荐，消费者升级先于生产者 |
| FORWARD | 旧 schema 可读新数据 | 生产者升级先于消费者 |
| FULL | 双向兼容 | 最严格，最安全 |
| NONE | 无兼容性检查 | 开发测试 |

### 4.2 Schema Registry 配置

```bash
# 设置全局兼容性级别
curl -X PUT http://schema-registry.messaging:8081/config \
  -H "Content-Type: application/json" \
  -d '{"compatibility": "FULL"}'

# 设置主题级兼容性
curl -X PUT http://schema-registry.messaging:8081/config/cdc.app_db.users-value \
  -H "Content-Type: application/json" \
  -d '{"compatibility": "BACKWARD"}'

# 查看 Schema
curl -s http://schema-registry.messaging:8081/subjects/cdc.app_db.users-value/versions/latest | jq
```

### 4.3 Schema 演进最佳实践

```
安全变更 (BACKWARD 兼容):
  ✓ 添加有默认值的字段
  ✓ 删除有默认值的字段
  ✓ 重命名字段 (通过 alias)

危险变更 (需要迁移):
  ✗ 删除必填字段
  ✗ 修改字段类型
  ✗ 修改字段位置

推荐流程:
  1. 新字段添加默认值
  2. Schema 注册新版本
  3. 消费者更新代码处理新字段
  4. 生产者开始发送新字段
```

## 5. 监控与告警

### 5.1 Connect 集群监控

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: debezium-connect-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      strimzi.io/cluster: debezium-connect
  endpoints:
  - port: metrics
    interval: 30s
```

### 5.2 关键指标

| 指标 | 含义 | 告警阈值 |
|------|------|---------|
| `kafka_connect_connector_status` | 连接器状态 | != 1 |
| `kafka_connect_task_status` | 任务状态 | != 1 |
| `kafka_connect_connect_worker_connector_count` | 连接器数量 | - |
| `kafka_connect_connect_worker_task_count` | 任务数量 | - |
| `debezium_metrics_*` | Debezium 自定义指标 | - |
| `kafka_connect_connector_failed_task_count` | 失败任务数 | > 0 |
| `kafka_consumer_records_lag` | 消费延迟 | > 10000 |

### 5.3 Debezium 特定指标

```bash
# 通过 JMX 获取 Debezium 指标
# MySQL Connector 指标
# - debezium_mysql_metrics_ConnectionCount
# - debezium_mysql_metrics_SnapshotRunning
# - debezium_mysql_metrics_NumberOfDisconnects
# - debezium_mysql_metrics_BinlogPosition

# PostgreSQL Connector 指标
# - debezium_postgres_metrics_Lsn
# - debezium_postgres_metrics_SnapshotRunning
# - debezium_postgres_metrics_NumberOfDisconnects
```

### 5.4 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: debezium-alerts
  namespace: monitoring
spec:
  groups:
  - name: debezium
    rules:
    - alert: DebeziumConnectorFailed
      expr: kafka_connect_connector_status != 1
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "Debezium 连接器状态异常: {{ $labels.connector }}"
    - alert: DebeziumTaskFailed
      expr: kafka_connect_task_status != 1
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "Debezium 任务失败: {{ $labels.connector }} task {{ $labels.task }}"
    - alert: DebeziumHighLag
      expr: kafka_consumer_records_lag > 100000
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Debezium 消费延迟超过 10 万条"
    - alert: DebeziumSnapshotRunning
      expr: debezium_metrics_SnapshotRunning == 1
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "Debezium 快照运行超过 30 分钟"
    - alert: DebeziumDisconnects
      expr: rate(debezium_metrics_NumberOfDisconnects[5m]) > 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Debezium 频繁断开连接"
```

## 6. 性能调优

### 6.1 Kafka Connect 调优

```yaml
# Connect Worker 配置
config:
  # 生产者优化
  producer.max.request.size: 10485760  # 10MB
  producer.buffer.memory: 67108864     # 64MB
  producer.batch.size: 65536           # 64KB
  producer.linger.ms: 5
  producer.compression.type: lz4

  # 消费者优化 (内部 topic)
  consumer.fetch.min.bytes: 1
  consumer.fetch.max.wait.ms: 500

  # 协调器
  scheduled.rebalance.max.delay.ms: 60000
  connect.protocol: eager

  # 任务协调
  task.shutdown.graceful.timeout.ms: 30000
```

### 6.2 Debezium 连接器调优

```yaml
config:
  # 快照优化
  snapshot.mode: initial
  snapshot.fetch.size: 10000
  snapshot.locking.mode: minimal

  # Binlog/WAL 读取优化
  binlog.buffer.size: 8388608  # 8MB

  # 批处理
  max.batch.size: 2048
  max.queue.size: 8192

  # 心跳间隔 (减少快照)
  heartbeat.interval.ms: 300000

  # 事件快照 (仅捕获变更)
  column.include.list: public.users.id,public.users.name,public.users.email

  # 过滤
  table.include.list: public.users,public.orders
  column.exclude.list: public.users.password_hash,public.users.ssn
```

### 6.3 调优决策树

```
CDC 延迟高?
│
├── Connect Worker CPU 高?
│   ├── 是 → 增加 replicas / 任务数
│   └── 否 → 继续排查
│
├── Kafka 生产者延迟高?
│   ├── 是 → 调整 batch.size / linger.ms / compression
│   └── 否 → 继续排查
│
├── 数据库 WAL/binlog 读取慢?
│   ├── 是 → 增大 buffer.size / 调整数据库配置
│   └── 否 → 继续排查
│
├── Schema Registry 延迟高?
│   ├── 是 → 检查 Schema 缓存 / 网络
│   └── 否 → 继续排查
│
└── 大事务阻塞?
    └── 是 → 拆分大事务 / 调整 max.batch.size
```

## 7. 故障排查速查

| 问题 | 排查命令 | 常见原因 |
|------|---------|---------|
| 连接器 FAILED | `GET /connectors/{name}/status` | 数据库不可达、认证失败 |
| 快照中断 | 检查 Connect 日志 | 数据库锁超时、网络中断 |
| 数据丢失 | 检查 offset 存储 | offset 主题配置错误 |
| Schema 不兼容 | Schema Registry API | 字段类型变更 |
| 延迟持续增长 | 监控 lag 指标 | Connect 资源不足、大事务 |
| 重复事件 | 检查 offset 提交 | Connect 崩溃导致重复消费 |

## 8. 源数据库前置条件

### 8.1 MySQL 前置

```sql
-- 创建 Debezium 用户
CREATE USER 'debezium'@'%' IDENTIFIED BY 'password';
GRANT SELECT, RELOAD, SHOW DATABASES, REPLICATION SLAVE, REPLICATION CLIENT ON *.* TO 'debezium'@'%';
GRANT SELECT ON app_db.* TO 'debezium'@'%';
FLUSH PRIVILEGES;

-- MySQL 配置 (my.cnf)
-- server-id=10001
-- log_bin=mysql-bin
-- binlog_format=ROW
-- binlog_row_image=FULL
-- gtid_mode=ON
-- enforce_gtid_consistency=ON
```

### 8.2 PostgreSQL 前置

```sql
-- 创建 Debezium 用户
CREATE USER debezium WITH REPLICATION LOGIN PASSWORD 'password';
GRANT SELECT ON ALL TABLES IN SCHEMA public TO debezium;
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT SELECT ON TABLES TO debezium;

-- PostgreSQL 配置 (postgresql.conf)
-- wal_level=logical
-- max_wal_senders=4
-- max_replication_slots=4

-- 创建 Publication
CREATE PUBLICATION debezium_publication FOR ALL TABLES;

-- 创建 Replication Slot
SELECT pg_create_logical_replication_slot('debezium_slot', 'pgoutput');
```


<!-- risk-assessed -->
