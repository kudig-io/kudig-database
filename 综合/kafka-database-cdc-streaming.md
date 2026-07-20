---
title: "Kafka × 数据库 × CDC × 流处理"
summary: "Debezium CDC 将数据库变更事件流入 Kafka，Schema Registry 保障契约演进，Exactly-once 语义确保数据一致性，流处理与数据库形成实时数据协同架构"
category: synthesis
tags:
- kafka
- cdc
- debezium
- schema-registry
- exactly-once
- stream-processing
- database
tier: supporting
sources:
- 实体/strimzi.md
- 概念/ci-cd-pipeline-patterns.md
- 概念/high-availability-patterns.md
- 概念/cloud-native-storage-systems.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# Kafka × 数据库 × CDC × 流处理

## The Connection（为什么这两个领域交叉）

传统架构中数据库是数据的"终点"——写入即完成，其他系统通过定时批量查询（ETL）获取数据。这种模式在实时性要求越来越高的场景中成为瓶颈：搜索索引需要秒级更新、风控系统需要毫秒级响应、数据仓库需要准实时同步。CDC（Change Data Capture）将数据库从"被动查询"变为"主动推送"——每一次 INSERT/UPDATE/DELETE 都变成事件流，下游系统实时消费。

Kafka 作为分布式事件流平台，是 CDC 事件的自然载体：高吞吐、持久化、多消费者组、精确一次语义。Debezium 是最主流的开源 CDC 引擎，通过数据库的日志机制（MySQL binlog、PostgreSQL WAL、MongoDB oplog）捕获变更，发布到 Kafka Topic。Schema Registry 管理事件 Schema 的演进（向前/向后兼容），确保生产者和消费者的契约不被破坏。

交叉的完整图景是：数据库（事实来源）→ CDC（变更捕获）→ Kafka（事件传输）→ 流处理（实时计算）→ 目标系统（搜索/缓存/数仓/微服务）。这构成了"事件驱动架构"的数据骨干，将原本耦合的点对点集成解耦为发布-订阅模式。

## Where They Co-occur（生产中的交叉场景）

### 场景一：数据库到搜索引擎的实时同步

电商平台的商品数据存储在 PostgreSQL，搜索服务使用 Elasticsearch。传统方案：定时全量/增量同步（延迟分钟级）。CDC 方案：Debezium 捕获 PostgreSQL WAL → Kafka Topic → Flink/Kafka Streams 转换 → Elasticsearch 索引更新。延迟从分钟级降到秒级，且无需修改业务代码。

### 场景二：微服务间的数据复制

订单服务写入订单数据库，库存服务、物流服务、通知服务都需要订单数据。传统方案：服务间 API 调用（强耦合、级联故障）。CDC 方案：订单数据库变更 → Kafka → 各服务独立消费（松耦合、可重放）。每个服务维护自己的数据视图（CQRS 模式）。

### 场景三：数据仓库准实时加载

分析团队需要近实时的业务数据（而非 T+1 批量）。CDC 将 OLTP 数据库变更流入 Kafka，Flink/Spark Streaming 做实时 ETL（清洗、聚合、维度关联），写入 OLAP 系统（ClickHouse/StarRocks/Delta Lake）。实现"分钟级新鲜度"的分析能力。

### 场景四：缓存失效与更新

Redis 缓存与数据库的一致性是经典难题。CDC 方案：数据库更新 → Debezium → Kafka → 缓存更新服务 → 更新/失效 Redis。相比"双写"（应用同时写 DB 和 Redis），CDC 保证最终一致性且不影响主路径性能。

### 场景五：审计日志与合规

金融/医疗系统需要完整的变更审计日志。CDC 捕获所有数据变更（包括谁在什么时间修改了什么字段），写入不可篡改的审计 Topic（保留期 7 年）。相比应用层审计日志，CDC 无法被绕过（即使直接 SQL 修改也能捕获）。

### 场景六：跨数据中心数据同步

多活架构中，不同数据中心的数据库需要双向同步。CDC + Kafka MirrorMaker 实现跨 DC 数据复制，配合冲突解决策略（Last-Write-Wins / 业务规则）处理并发写入冲突。

## Production Patterns（生产模式与架构）

### 模式一：CDC 数据流架构

```
┌─────────────────────────────────────────────────────────┐
│  CDC + Kafka + Stream Processing Architecture            │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Source Databases                                       │
│  ├── PostgreSQL (WAL) ──┐                              │
│  ├── MySQL (binlog) ────┤                              │
│  └── MongoDB (oplog) ───┤                              │
│                          ▼                              │
│  Debezium Connectors (Kafka Connect)                    │
│  ├── 解析数据库日志                                     │
│  ├── 生成变更事件 (JSON/Avro)                          │
│  ├── 发布到 Kafka Topics                               │
│  └── 记录 offset (支持断点续传)                        │
│                          │                              │
│                          ▼                              │
│  Kafka Cluster (Strimzi/Confluent)                      │
│  ├── Topic: db.orders (分区键: order_id)               │
│  ├── Topic: db.products (分区键: product_id)           │
│  ├── Topic: db.users (分区键: user_id)                 │
│  └── Schema Registry (Avro/Protobuf 契约)             │
│                          │                              │
│              ┌───────────┼───────────┐                  │
│              ▼           ▼           ▼                  │
│  Stream Processing  Sink Connectors  Consumer Groups    │
│  ├── Flink Jobs    ├── ES Sink      ├── 库存服务       │
│  ├── Kafka Streams ├── S3 Sink      ├── 通知服务       │
│  └── ksqlDB        └── JDBC Sink    └── 分析服务       │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 模式二：Debezium Connector 配置

```yaml
# Kafka Connect Debezium PostgreSQL Connector
apiVersion: kafka.strimzi.io/v1beta2
kind: KafkaConnector
metadata:
  name: debezium-postgres-orders
  namespace: kafka
spec:
  class: io.debezium.connector.postgresql.PostgresConnector
  tasksMax: 1
  config:
    database.hostname: postgres.orders.svc.cluster.local
    database.port: "5432"
    database.user: debezium
    database.password: "${file:/opt/kafka/secrets/debezium.properties:password}"
    database.dbname: orders
    database.server.name: prod
    schema.include.list: public
    table.include.list: public.orders,public.order_items
    plugin.name: pgoutput
    slot.name: debezium_orders
    publication.name: debezium_publication
    # 性能配置
    max.batch.size: "2048"
    max.queue.size: "8192"
    poll.interval.ms: "500"
    # 事件格式
    key.converter: io.confluent.connect.avro.AvroConverter
    value.converter: io.confluent.connect.avro.AvroConverter
    key.converter.schema.registry.url: http://schema-registry:8081
    value.converter.schema.registry.url: http://schema-registry:8081
    # Tombstone 配置 (DELETE 事件)
    tombstones.on.delete: "true"
```

### 模式三：Schema Registry 契约管理

```
Schema 演进策略:

  BACKWARD (默认): 新 Schema 可读旧数据
    - 可以: 删除有默认值的字段、添加可选字段
    - 不可以: 删除无默认值的字段、修改字段类型
    - 适用: 消费者先升级

  FORWARD: 旧 Schema 可读新数据
    - 可以: 删除字段、添加有默认值的字段
    - 不可以: 添加无默认值的字段
    - 适用: 生产者先升级

  FULL: 同时满足 BACKWARD + FORWARD
    - 可以: 添加/删除有默认值的字段
    - 不可以: 修改字段类型
    - 适用: 生产者和消费者独立升级

  生产推荐: BACKWARD (最常用) 或 FULL (最严格)

  CI 集成:
    Schema 变更 PR → Schema Registry 兼容性检查 → 通过则合并
    curl -X POST schema-registry/compatibility -d @new-schema.avsc
```

### 模式四：Exactly-once 语义

```
端到端 Exactly-once 实现:

  1. Kafka 事务 (Producer → Kafka):
     producer.enable.idempotence=true
     producer.transactional.id=order-processor-1

  2. Kafka Streams (Kafka → 处理 → Kafka):
     processing.guarantee=exactly_once_v2
     # 自动管理事务，消费-处理-生产原子化

  3. Flink (Kafka → 处理 → 外部系统):
     Checkpoint + Two-Phase Commit
     # Flink Checkpoint 保存 Kafka offset
     # Sink 通过 2PC 写入外部系统

  4. Sink 幂等性 (Kafka → 数据库):
     # 使用 UPSERT (INSERT ON CONFLICT UPDATE)
     # 或基于主键的幂等写入
     # Debezium 事件包含 op 字段 (c/u/d/r) 支持幂等

  注意: 真正的端到端 exactly-once 需要所有环节支持
  实践中常用: at-least-once + 幂等消费 (更简单可靠)
```

### 模式五：Outbox 模式（解决双写问题）

```
问题: 应用需要同时写数据库和发 Kafka 消息，如何保证一致性？

Outbox 模式:
  1. 应用在同一事务中写业务表 + outbox 表
  2. Debezium 捕获 outbox 表变更 → 发布到 Kafka
  3. 发布成功后删除 outbox 记录 (或标记已发送)

优势:
  - 数据库事务保证业务数据和消息的原子性
  - 无需分布式事务 (2PC)
  - Debezium 保证消息至少发送一次
  - 消费者幂等处理重复消息

实现:
  BEGIN;
    INSERT INTO orders (...) VALUES (...);
    INSERT INTO outbox (aggregate_type, aggregate_id, event_type, payload)
      VALUES ('Order', '123', 'OrderCreated', '{"order_id": "123", ...}');
  COMMIT;
  -- Debezium 自动捕获 outbox 变更并发布到 Kafka
```

## Trade-offs & Decision Matrix（权衡与决策）

| 维度 | Debezium + Kafka | 原生逻辑复制 (PG) | 应用层事件发布 | 批量 ETL |
|------|-----------------|------------------|--------------|---------|
| 实时性 | 秒级 | 秒级 | 毫秒级 | 分钟-小时 |
| 侵入性 | 零（不改应用） | 零 | 高（改代码） | 零 |
| 可靠性 | 高（Kafka 持久化） | 中（依赖 PG） | 中（需处理失败） | 高 |
| 吞吐量 | 百万事件/秒 | 十万事件/秒 | 取决于应用 | 取决于批次 |
| Schema 管理 | Schema Registry | 无 | 应用自管 | ETL 工具 |
| 多消费者 | 原生（Consumer Group） | 有限 | 需 MQ | 无 |
| 运维复杂度 | 高（Kafka + Connect） | 低 | 低 | 中 |
| 适用规模 | 中大型 | 中小型 | 所有 | 所有 |

### 决策矩阵

- **需要多消费者 + 高吞吐 + 事件回放** → Debezium + Kafka
- **简单的 PG → PG 复制** → 原生逻辑复制
- **事件量小 + 实时性要求极高** → 应用层事件发布（Outbox 模式）
- **T+1 分析即可** → 批量 ETL（最简单）
- **已有 Kafka 基础设施** → Debezium（增量最小）
- **合规审计（不可绕过）** → CDC（数据库日志级捕获）

## Anti-patterns & Pitfalls（反模式）

### 反模式一：CDC Topic 无分区策略

所有变更事件写入单分区 Topic，消费者无法并行处理，吞吐瓶颈。**正确做法**：按主键（如 order_id）分区，保证同一实体的事件有序，同时允许不同实体并行消费。

### 反模式二：忽略 Schema 演进

生产者修改数据库表结构（加列/改类型），CDC 事件 Schema 变化，消费者反序列化失败。**正确做法**：Schema Registry 强制兼容性检查；数据库 DDL 变更走审批流程；消费者使用宽松反序列化（忽略未知字段）。

### 反模式三：消费者处理不幂等

Kafka 保证 at-least-once 投递，消费者可能收到重复事件。非幂等消费者导致数据重复（如重复扣款）。**正确做法**：基于事件 ID 去重；使用 UPSERT 语义；维护已处理事件 ID 集合。

### 反模式四：CDC 延迟监控缺失

Debezium Connector 因数据库负载高或 Kafka 不可用而延迟增大，下游数据"过时"但无人知晓。**正确做法**：监控 Debezium `source.ts_ms` 与当前时间的差值（CDC 延迟）；设置告警（延迟 > 30s）。

### 反模式五：大事务导致 CDC 阻塞

数据库中的大事务（如批量 UPDATE 百万行）产生巨量 CDC 事件，阻塞后续事件处理。**正确做法**：限制业务事务大小；Debezium 配置 `max.batch.size` 控制批次；大事务拆分为小批次。

### 反模式六：Kafka 保留期过短

Topic 保留期设为 1 小时，消费者宕机 2 小时后重启，offset 已过期，数据丢失。**正确做法**：保留期 ≥ 消费者最大停机时间（建议 7 天）；关键 Topic 保留 30 天；监控 Consumer Lag。

## Operational Checklist（运维检查清单）

### 基础设施

- [ ] Kafka 集群高可用（≥3 Broker，RF=3，min.insync.replicas=2）
- [ ] Debezium Connect 集群（≥2 Worker，任务自动 rebalance）
- [ ] Schema Registry 高可用（≥3 实例）
- [ ] 数据库配置：WAL/binlog 保留期足够、复制槽监控
- [ ] Topic 配置：分区数 ≥ 消费者数、保留期 ≥ 7 天

### 监控告警

- [ ] Consumer Lag 监控（> 10000 告警）
- [ ] CDC 延迟监控（> 30s 告警）
- [ ] Connector 状态监控（FAILED 立即告警）
- [ ] Schema Registry 兼容性检查失败告警
- [ ] Kafka 磁盘使用率（> 70% 告警）
- [ ] 数据库复制槽延迟（> 100MB 告警）

### 数据质量

- [ ] 定期验证源端与目标端数据一致性
- [ ] 监控事件格式异常（反序列化失败率）
- [ ] 死信队列（DLQ）配置和监控
- [ ] 事件顺序验证（同一实体事件有序）

### 故障恢复

- [ ] Connector 故障自动重启（Kafka Connect 内置）
- [ ] 消费者故障：从上次 committed offset 恢复
- [ ] Kafka 故障：ISR 机制保证可用性
- [ ] 数据库故障：CDC 在数据库恢复后自动续传
- [ ] 全量重同步预案（Snapshot 模式）

## Related

- [[实体/strimzi.md|Strimzi]]
- [[概念/high-availability-patterns.md|高可用模式]]
- [[概念/cloud-native-storage-systems.md|云原生存储系统]]
- [[概念/ci-cd-pipeline-patterns.md|CI/CD 流水线模式]]
- [[综合/kafka-database-cdc-streaming.md|Kafka × 数据库 × CDC × 流处理]]
- [[综合/storage-ai-workload-data-pipeline.md|存储 × AI 工作负载 × 数据管线]]
- [[综合/chaos-engineering-sre-resilience.md|混沌工程 × SRE × 弹性]]
