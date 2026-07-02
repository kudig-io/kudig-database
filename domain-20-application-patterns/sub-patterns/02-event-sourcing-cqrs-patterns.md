---
title: Event Sourcing 与 CQRS 模式
description: 'Event Sourcing+CQRS在K8s上的实现：事件存储选型、读写分离架构、最终一致性处理与事件版本化'
summary: 'Event Sourcing+CQRS在K8s上的实现：事件存储选型、读写分离架构、最终一致性处理与事件版本化'
category: application-patterns
tags:
- event-sourcing
- cqrs
- kafka
- event-store
- eventual-consistency
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 所有工程师
- 架构师
- SRE
estimated_read_time: 15min
intent_queries:
- Event Sourcing CQRS 是什么
- 如何 实现 Event Sourcing
trigger_keywords:
- Event Sourcing
- CQRS
- 事件溯源
- 命令查询分离
prerequisites:
- kubectl-basics
- microservice-basics
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


# Event Sourcing 与 CQRS 模式

## 1. 概述

Event Sourcing（事件溯源）将系统状态变更记录为不可变的事件序列，CQRS（命令查询职责分离）将读写模型解耦。两者结合可构建高可审计、可回放、可水平扩展的分布式系统。本文档覆盖在 Kubernetes 上的完整实现方案。

## 2. 核心概念

### 2.1 Event Sourcing 基本原理

```
传统 CRUD 模式:
  Command → 读取当前状态 → 修改 → 覆盖写入
  问题: 历史状态丢失，无法审计

Event Sourcing 模式:
  Command → 生成事件 → 追加到事件流 → 投影到读模型
  优势: 完整历史、可回放、可重建任意时间点状态

事件流示例（订单生命周期）:
  Event 1: OrderCreated {orderId: "O-001", items: [...], total: 99.00}
  Event 2: OrderPaid {orderId: "O-001", paymentId: "P-001", amount: 99.00}
  Event 3: OrderShipped {orderId: "O-001", trackingNo: "SF123456"}
  Event 4: OrderDelivered {orderId: "O-001", signedBy: "张三"}
```

### 2.2 CQRS 读写分离

```
                    ┌─────────────┐
   Command ────────→│  Write Model │──────→ Event Store
   (写操作)          │  (聚合根)     │         │
                    └─────────────┘         │
                                            ▼
                                    ┌──────────────┐
                                    │  Event Bus    │
                                    │  (Kafka)      │
                                    └──────┬───────┘
                                           │
                    ┌──────────────┐        │
   Query ─────────→│  Read Model   │←───────┘
   (读操作)         │  (物化视图)    │
                    └──────────────┘

写模型: 强一致性，处理业务不变量
读模型: 最终一致性，优化查询性能
```

## 3. 事件存储选型

### 3.1 对比矩阵

| 特性 | Kafka | EventStoreDB | PostgreSQL + Outbox | DynamoDB Streams |
|------|-------|-------------|-------------------|-----------------|
| **持久性** | 高（分区副本） | 高（集群模式） | 高（WAL） | 高（跨AZ） |
| **顺序保证** | 分区内有序 | 流内有序 | 需自行保证 | 分区内有序 |
| **订阅模式** | Consumer Group | Catch-up Subscription | CDC/Debezium | Lambda Trigger |
| **快照支持** | 需自行实现 | 内置 | 需自行实现 | 需自行实现 |
| **运维复杂度** | 中（Strimzi） | 低 | 低 | 低（托管） |
| **适用规模** | 超大 | 中大 | 中小 | 中大 |
| **K8s 原生** | Strimzi Operator | Helm Chart | CloudNativePG | N/A（托管） |

### 3.2 Kafka 作为事件存储

```yaml
# Strimzi Kafka 集群配置
apiVersion: kafka.strimzi.io/v1beta2
kind: Kafka
metadata:
  name: event-store
  namespace: event-sourcing
spec:
  kafka:
    version: 3.7.0
    replicas: 3
    listeners:
      - name: plain
        port: 9092
        type: internal
        tls: false
      - name: tls
        port: 9093
        type: internal
        tls: true
    config:
      offsets.topic.replication.factor: 3
      transaction.state.log.replication.factor: 3
      transaction.state.log.min.isr: 2
      default.replication.factor: 3
      min.insync.replicas: 2
      log.retention.hours: -1        # 事件永不过期
      log.retention.bytes: -1
      log.segment.bytes: 1073741824   # 1GB per segment
    storage:
      type: persistent-claim
      size: 100Gi
      class: fast-ssd
  zookeeper:
    replicas: 3
    storage:
      type: persistent-claim
      size: 20Gi
```

### 3.3 EventStoreDB 部署

```yaml
# EventStoreDB StatefulSet
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: eventstoredb
  namespace: event-sourcing
spec:
  serviceName: eventstoredb
  replicas: 3
  selector:
    matchLabels:
      app: eventstoredb
  template:
    metadata:
      labels:
        app: eventstoredb
    spec:
      containers:
        - name: eventstoredb
          image: eventstore/eventstore:23.10.0-jammy
          env:
            - name: EVENTSTORE_CLUSTER_SIZE
              value: "3"
            - name: EVENTSTORE_GOSSIP_SEED
              value: "eventstoredb-0.eventstoredb:2113,eventstoredb-1.eventstoredb:2113"
            - name: EVENTSTORE_INSECURE
              value: "false"
            - name: EVENTSTORE_ENABLE_ATOM_PUB_OVER_HTTP
              value: "true"
          ports:
            - containerPort: 2113
              name: http
            - containerPort: 1113
              name: tcp
          volumeMounts:
            - name: data
              mountPath: /var/lib/eventstore
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 50Gi
```

## 4. CQRS 读写分离实现

### 4.1 写模型（聚合根）

```go
// 订单聚合根
type Order struct {
    ID        string
    Items     []OrderItem
    Status    OrderStatus
    Total     Money
    Version   int
    Changes   []Event  // 未提交的事件
}

func (o *Order) CreateOrder(cmd CreateOrderCommand) error {
    if o.Status != "" {
        return errors.New("order already exists")
    }
    event := OrderCreatedEvent{
        OrderID: cmd.OrderID,
        Items:   cmd.Items,
        Total:   calculateTotal(cmd.Items),
    }
    o.apply(event)
    o.Changes = append(o.Changes, event)
    return nil
}

func (o *Order) Pay(cmd PayOrderCommand) error {
    if o.Status != OrderStatusCreated {
        return errors.New("order not in created state")
    }
    event := OrderPaidEvent{
        OrderID:   o.ID,
        PaymentID: cmd.PaymentID,
        Amount:    o.Total,
    }
    o.apply(event)
    o.Changes = append(o.Changes, event)
    return nil
}

// 从事件流重建聚合状态
func (o *Order) LoadFromHistory(events []Event) {
    for _, e := range events {
        o.apply(e)
        o.Version++
    }
}
```

### 4.2 读模型（投影）

```go
// 订单查询投影
type OrderReadModel struct {
    OrderID     string    `json:"orderId"`
    CustomerID  string    `json:"customerId"`
    Status      string    `json:"status"`
    Total       float64   `json:"total"`
    ItemCount   int       `json:"itemCount"`
    CreatedAt   time.Time `json:"createdAt"`
    PaidAt      *time.Time `json:"paidAt,omitempty"`
    ShippedAt   *time.Time `json:"shippedAt,omitempty"`
}

// 投影处理器
type OrderProjection struct {
    db *sql.DB
}

func (p *OrderProjection) HandleEvent(event Event) error {
    switch e := event.(type) {
    case OrderCreatedEvent:
        return p.db.Exec(`
            INSERT INTO order_read_model (order_id, customer_id, status, total, item_count, created_at)
            VALUES ($1, $2, 'CREATED', $3, $4, $5)
        `, e.OrderID, e.CustomerID, e.Total, len(e.Items), e.Timestamp)

    case OrderPaidEvent:
        return p.db.Exec(`
            UPDATE order_read_model SET status = 'PAID', paid_at = $1
            WHERE order_id = $2
        `, e.Timestamp, e.OrderID)

    case OrderShippedEvent:
        return p.db.Exec(`
            UPDATE order_read_model SET status = 'SHIPPED', shipped_at = $1
            WHERE order_id = $2
        `, e.Timestamp, e.OrderID)
    }
    return nil
}
```

## 5. 最终一致性处理

### 5.1 投影延迟监控

```yaml
# Prometheus 指标定义
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: event-sourcing-lag
spec:
  groups:
    - name: event-sourcing
      rules:
        - alert: ProjectionLagHigh
          expr: |
            event_projection_lag_seconds > 30
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "投影延迟超过30秒"
            description: "{{ $labels.projection }} 延迟 {{ $value }} 秒"

        - alert: EventStoreBacklog
          expr: |
            kafka_consumer_group_lag{group=~".*projection.*"} > 10000
          for: 10m
          labels:
            severity: critical
```

### 5.2 幂等性保证

```go
// 幂等事件处理
func (p *OrderProjection) HandleEventIdempotent(event Event) error {
    // 检查事件是否已处理
    var processed bool
    err := p.db.QueryRow(`
        SELECT EXISTS(
            SELECT 1 FROM processed_events
            WHERE event_id = $1 AND projection = $2
        )
    `, event.ID, "order").Scan(&processed)
    if err != nil {
        return err
    }
    if processed {
        return nil // 已处理，跳过
    }

    // 事务内处理事件 + 记录已处理
    tx, _ := p.db.Begin()
    defer tx.Rollback()

    if err := p.handleEventTx(tx, event); err != nil {
        return err
    }

    _, err = tx.Exec(`
        INSERT INTO processed_events (event_id, projection, processed_at)
        VALUES ($1, $2, NOW())
    `, event.ID, "order")
    if err != nil {
        return err
    }

    return tx.Commit()
}
```

## 6. 投影管理

### 6.1 投影生命周期

```
投影管理策略:

新建投影:
  1. 从事件流起始位置回放
  2. 记录当前处理位置（offset）
  3. 达到最新事件后进入实时模式

重建投影:
  1. 清空读模型表
  2. 重置 offset 到流起始位置
  3. 全量回放事件
  4. 适用于模型变更或数据修复

多版本投影:
  1. 新版本投影独立部署
  2. 双读验证一致性
  3. 切换流量到新版本
  4. 下线旧版本
```

### 6.2 投影重建 Job

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: order-projection-rebuild
  namespace: event-sourcing
spec:
  template:
    spec:
      restartPolicy: OnFailure
      containers:
        - name: rebuild
          image: registry.example.com/order-projection:latest
          command: ["./rebuild-projection"]
          args:
            - "--source=kafka://event-store:9092/order-events"
            - "--target=postgresql://read-db:5432/order_read_model"
            - "--projection=order-summary"
            - "--batch-size=1000"
            - "--parallelism=4"
          resources:
            requests:
              cpu: "2"
              memory: 4Gi
            limits:
              cpu: "4"
              memory: 8Gi
```

## 7. 事件版本化与快照

### 7.1 事件版本策略

```go
// 事件版本管理
type EventEnvelope struct {
    EventID      string    `json:"eventId"`
    EventType    string    `json:"eventType"`
    Version      int       `json:"version"`      // 事件 schema 版本
    AggregateID  string    `json:"aggregateId"`
    Timestamp    time.Time `json:"timestamp"`
    Data         []byte    `json:"data"`
    Metadata     map[string]string `json:"metadata"`
}

// 事件升级器链
type EventUpgrader func([]byte) ([]byte, error)

var upgraders = map[string][]EventUpgrader{
    "OrderCreated": {
        upgradeV1toV2,  // v1 → v2: 添加 currency 字段
        upgradeV2toV3,  // v2 → v3: 拆分 address 为独立对象
    },
}

func UpgradeEvent(envelope EventEnvelope) (EventEnvelope, error) {
    upgraderChain, ok := upgraders[envelope.EventType]
    if !ok {
        return envelope, nil
    }
    data := envelope.Data
    for i := envelope.Version - 1; i < len(upgraderChain); i++ {
        var err error
        data, err = upgraderChain[i](data)
        if err != nil {
            return envelope, err
        }
        envelope.Version++
    }
    envelope.Data = data
    return envelope, nil
}
```

### 7.2 快照策略

```go
// 快照管理
type SnapshotStore interface {
    Save(aggregateID string, snapshot Snapshot) error
    Load(aggregateID string) (*Snapshot, error)
}

type Snapshot struct {
    AggregateID string
    Version     int
    State       []byte
    CreatedAt   time.Time
}

// 每 N 个事件创建快照
const SnapshotInterval = 100

func (r *OrderRepository) Load(id string) (*Order, error) {
    order := &Order{}

    // 尝试从快照恢复
    snapshot, err := r.snapshots.Load(id)
    if err == nil && snapshot != nil {
        order.LoadFromSnapshot(snapshot)
        // 从快照之后的事件继续回放
        events, err := r.store.LoadEvents(id, snapshot.Version+1)
        if err != nil {
            return nil, err
        }
        order.LoadFromHistory(events)
    } else {
        // 从头回放所有事件
        events, err := r.store.LoadEvents(id, 0)
        if err != nil {
            return nil, err
        }
        order.LoadFromHistory(events)
    }

    // 检查是否需要创建新快照
    if order.Version % SnapshotInterval == 0 {
        r.snapshots.Save(id, Snapshot{
            AggregateID: id,
            Version:     order.Version,
            State:       order.Serialize(),
            CreatedAt:   time.Now(),
        })
    }

    return order, nil
}
```

## 8. K8s 完整部署架构

```yaml
# 完整的 Event Sourcing 微服务部署
apiVersion: v1
kind: Namespace
metadata:
  name: event-sourcing
  labels:
    istio-injection: enabled
---
# 写模型服务
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-command-service
  namespace: event-sourcing
spec:
  replicas: 3
  selector:
    matchLabels:
      app: order-command-service
  template:
    metadata:
      labels:
        app: order-command-service
        role: write-model
    spec:
      containers:
        - name: service
          image: registry.example.com/order-command:v2.0.0
          env:
            - name: KAFKA_BROKERS
              value: "event-store-kafka-bootstrap:9092"
            - name: EVENT_STORE_TOPIC
              value: "order-events"
---
# 读模型服务
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-query-service
  namespace: event-sourcing
spec:
  replicas: 5    # 读多写少
  selector:
    matchLabels:
      app: order-query-service
  template:
    metadata:
      labels:
        app: order-query-service
        role: read-model
    spec:
      containers:
        - name: service
          image: registry.example.com/order-query:v2.0.0
          env:
            - name: READ_DB_HOST
              value: "order-read-db"
```

## 9. 常见问题与解决方案

| 问题 | 根因 | 解决方案 |
|------|------|---------|
| 事件流膨胀 | 事件数量过多导致回放慢 | 定期快照 + 归档旧事件 |
| 投影不一致 | 事件处理顺序错乱 | 分区内有序 + 幂等处理 |
| Schema 演进 | 事件结构变更 | 事件版本化 + 升级器链 |
| 调试困难 | 状态分散在事件中 | 事件浏览器 + 时间旅行查询 |
| 最终一致窗口 | 用户看不到最新数据 | 写后读一致性（read-your-writes） |

## Related

- [[domain-20-application-patterns/sub-patterns/03-saga-distributed-transaction|Saga 分布式事务]]
- [[domain-20-application-patterns/sub-patterns/01-microservice-decomposition-strategies|微服务拆分策略]]
- domain-16-database-middleware//

## See Also

- Kafka 事件流最佳实践
- CQRS 模式详解
- 事件版本化策略


<!-- risk-assessed -->
