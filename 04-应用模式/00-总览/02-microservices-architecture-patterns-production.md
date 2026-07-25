---
title: Microservices Architecture Patterns on Kubernetes — Production Design Guide
description: K8s 微服务架构模式 — 服务拆分、通信模式、数据管理、容错设计、部署策略、可观测性集成
summary: 在 Kubernetes 上实施微服务架构的生产级设计指南，涵盖拆分策略、通信、数据、容错四大核心
category: practice
tags:
- microservices
- architecture
- service-mesh
- resilience
- domain-driven-design
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: app-patterns
---
# Kubernetes 微服务架构模式

> 生产级微服务设计：拆分、通信、数据、容错。

## 微服务拆分策略

### 拆分原则

```
┌─────────────────────────────────────────────────────────────┐
│  拆分决策框架                                                │
│                                                             │
│  1. 业务边界（DDD Bounded Context）                          │
│     └── 每个服务对应一个限界上下文                            │
│  2. 变更频率（独立部署）                                     │
│     └── 变更频率不同的模块分离                                │
│  3. 团队所有权（Conway's Law）                               │
│     └── 一个团队拥有 1-3 个服务                              │
│  4. 扩展需求（独立扩缩）                                     │
│     └── 负载特征不同的组件分离                                │
│  5. 故障隔离（爆炸半径）                                     │
│     └── 关键路径与非关键路径分离                              │
└─────────────────────────────────────────────────────────────┘
```

### 拆分粒度判断

| 信号 | 太粗（需拆分） | 太细（需合并） |
|------|---------------|---------------|
| 部署 | 改一行代码需协调多团队 | 每次变更需同时部署 5+ 服务 |
| 团队 | 多团队改同一代码库 | 一人维护多个服务 |
| 扩展 | 只需扩一个模块但必须扩整体 | 服务间调用链 > 10 层 |
| 数据 | 共享数据库多表 join | 分布式事务频繁 |
| 故障 | 一个模块崩溃拖垮全部 | 网络延迟成为瓶颈 |

## 通信模式

### 同步通信

```yaml
# gRPC 服务（推荐内部通信）
apiVersion: v1
kind: Service
metadata:
  name: order-service
  namespace: production
spec:
  selector:
    app: order-service
  ports:
    - name: grpc
      port: 9090
      targetPort: 9090
    - name: http
      port: 8080
      targetPort: 8080
---
# 客户端配置（连接池 + 超时 + 重试）
apiVersion: v1
kind: ConfigMap
metadata:
  name: grpc-client-config
data:
  config.yaml: |
    client:
      target: "order-service.production.svc:9090"
      timeout: 3s
      retry:
        maxAttempts: 3
        backoff: 100ms
        maxBackoff: 1s
        retryableStatusCodes: [UNAVAILABLE, DEADLINE_EXCEEDED]
      keepalive:
        time: 30s
        timeout: 10s
      connectionPool:
        maxSize: 100
        idleTimeout: 300s
```

### 异步通信（事件驱动）

```yaml
# 事件驱动架构
# Producer → Kafka/NATS → Consumer

# 订单服务发布事件
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service
spec:
  template:
    spec:
      containers:
        - name: order
          env:
            - name: KAFKA_BROKERS
              value: "kafka-bootstrap.kafka:9092"
            - name: EVENT_TOPIC
              value: "order-events"
          # 事件: OrderCreated, OrderPaid, OrderShipped, OrderCompleted
---
# 库存服务消费事件
apiVersion: apps/v1
kind: Deployment
metadata:
  name: inventory-service
spec:
  template:
    spec:
      containers:
        - name: inventory
          env:
            - name: KAFKA_BROKERS
              value: "kafka-bootstrap.kafka:9092"
            - name: CONSUMER_GROUP
              value: "inventory-group"
            - name: SUBSCRIBE_TOPICS
              value: "order-events"
```

### 通信模式选择

| 场景 | 模式 | 技术 | 适用 |
|------|------|------|------|
| 实时查询 | 同步 RPC | gRPC | 服务间调用 |
| 外部 API | 同步 HTTP | REST/GraphQL | 客户端接口 |
| 事件通知 | 异步消息 | Kafka/NATS | 解耦/削峰 |
| 长任务 | 异步队列 | RabbitMQ/SQS | 后台处理 |
| 实时推送 | 流式 | WebSocket/SSE | 通知/监控 |
| 批量处理 | 事件溯源 | Kafka Streams | 数据分析 |

## 数据管理模式

### Database per Service

```
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ Order Service│  │ User Service │  │Inventory Svc │
│              │  │              │  │              │
│ ┌──────────┐ │  │ ┌──────────┐ │  │ ┌──────────┐ │
│ │PostgreSQL│ │  │ │PostgreSQL│ │  │ │  Redis   │ │
│ │(orders)  │ │  │ │(users)   │ │  │ │(stock)   │ │
│ └──────────┘ │  │ └──────────┘ │  │ └──────────┘ │
└──────────────┘  └──────────────┘  └──────────────┘
     独立数据库，禁止跨服务 JOIN
```

### 数据一致性（Saga 模式）

```yaml
# 编排式 Saga（订单流程）
# OrderCreated → ReserveInventory → ProcessPayment → ConfirmOrder
# 补偿: CancelPayment → ReleaseInventory → CancelOrder

# 使用 Temporal/Cadence 编排
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-saga-orchestrator
spec:
  template:
    spec:
      containers:
        - name: orchestrator
          image: order-saga:v1
          env:
            - name: TEMPORAL_ADDRESS
              value: "temporal.temporal-system:7233"
            - name: SAGA_TIMEOUT
              value: "30s"
```

### API 聚合（BFF 模式）

```yaml
# Backend for Frontend
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: bff-ingress
spec:
  rules:
    - host: api.example.com
      http:
        paths:
          - path: /mobile
            pathType: Prefix
            backend:
              service:
                name: mobile-bff    # 移动端聚合
                port:
                  number: 8080
          - path: /web
            pathType: Prefix
            backend:
              service:
                name: web-bff       # Web 端聚合
                port:
                  number: 8080
```

## 容错设计

### 超时/重试/熔断

```yaml
# Istio VirtualService（流量治理）
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: order-service
spec:
  hosts: ["order-service"]
  http:
    - route:
        - destination:
            host: order-service
      timeout: 5s
      retries:
        attempts: 3
        perTryTimeout: 2s
        retryOn: "5xx,reset,connect-failure"
        retryRemoteLocalities: true
---
# 熔断（DestinationRule）
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: order-service
spec:
  host: order-service
  trafficPolicy:
    connectionPool:
      tcp:
        maxConnections: 100
      http:
        h2UpgradePolicy: DEFAULT
        http1MaxPendingRequests: 50
        http2MaxRequests: 100
        maxRequestsPerConnection: 10
    outlierDetection:
      consecutive5xxErrors: 5
      interval: 10s
      baseEjectionTime: 30s
      maxEjectionPercent: 50
```

### 优雅降级

```yaml
# 降级策略配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: resilience-config
data:
  resilience.yaml: |
    circuitBreaker:
      failureRateThreshold: 50
      waitDurationInOpenState: 30s
      slidingWindowSize: 10
    fallback:
      # 推荐服务不可用时返回缓存/默认值
      recommendation:
        type: cache
        ttl: 300s
        defaultValue: []
      # 支付服务不可用时排队
      payment:
        type: queue
        maxWait: 60s
```

## 部署模式

### 独立部署管道

```yaml
# 每个服务独立 CI/CD
# GitHub Actions 示例
name: order-service CI/CD
on:
  push:
    paths: ['services/order-service/**']
    branches: [main]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: cd services/order-service && make test

  build:
    needs: test
    runs-on: ubuntu-latest
    steps:
      - run: |
          docker build -t registry.example.com/order-service:${{ github.sha }} .
          docker push registry.example.com/order-service:${{ github.sha }}

  deploy:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - run: |
          cd gitops/apps/order-service/overlays/production
          kustomize edit set image order-service=registry.example.com/order-service:${{ github.sha }}
          git add . && git commit -m "deploy order-service ${{ github.sha }}" && git push
```

## 可观测性集成

### 分布式追踪（OpenTelemetry）

```yaml
# 自动注入 OTel Collector
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service
  annotations:
    sidecar.opentelemetry.io/inject: "true"
spec:
  template:
    spec:
      containers:
        - name: order
          env:
            - name: OTEL_SERVICE_NAME
              value: "order-service"
            - name: OTEL_EXPORTER_OTLP_ENDPOINT
              value: "http://otel-collector.observability:4317"
            - name: OTEL_TRACES_SAMPLER
              value: "parentbased_traceidratio"
            - name: OTEL_TRACES_SAMPLER_ARG
              value: "0.1"  # 10% 采样
```

## 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| 分布式单体 | 服务间强耦合，必须同时部署 | 异步解耦，独立部署 |
| 共享数据库 | 耦合、锁竞争 | Database per Service |
| 同步链式调用 | 延迟叠加、级联故障 | 异步事件 + 熔断 |
| 过度拆分 | 运维复杂度爆炸 | 从单体开始，渐进拆分 |
| 无版本 API | 破坏性变更影响消费者 | 语义化版本 + 兼容期 |
| 忽略数据一致性 | 分布式事务失败 | Saga + 最终一致性 |

## Related

- [[04-应用模式/index.md|应用模式]]
- [[04-应用模式/微服务/index.md|微服务]]
- [[05-网络/03-服务网格/index.md|服务网格]]
- [[07-数据库中间件/03-消息队列/index.md|消息队列]]
