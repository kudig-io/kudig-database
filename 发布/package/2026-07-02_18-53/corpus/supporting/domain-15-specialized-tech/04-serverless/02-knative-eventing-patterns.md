---
title: Knative Eventing 事件驱动模式
description: 'Knative Eventing Broker/Trigger 模型、Source 集成与 CloudEvents 规范实战'
summary: 'Knative Eventing Broker/Trigger 模型、Source 集成与 CloudEvents 规范实战'
category: specialized-tech
tags:
- knative
- eventing
- cloud-events
- event-driven
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Knative Eventing 是什么
- 如何配置 Knative Eventing Broker/Trigger
- 如何使用 Knative Eventing Source
trigger_keywords:
- knative
- eventing
- broker
- trigger
- cloud-events
- event-driven
prerequisites:
- kubectl-basics
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


# Knative Eventing 事件驱动模式

## 1. 架构概览

Knative Eventing 提供松耦合的事件生产与消费架构：

```
Event Sources → Broker → Triggers → Sink(Service/Broker/Channel)
    │              │
    │              └── CloudEvents 标准格式
    └── KafkaSource / ApiServerSource / PingSource / Custom
```

核心组件：

| 组件 | 职责 |
|------|------|
| **Source** | 事件源，将外部事件转化为 CloudEvents |
| **Broker** | 事件路由中心，接收并分发事件 |
| **Trigger** | 基于事件属性的过滤规则，路由到 Sink |
| **Channel / Subscription** | 消息通道模型（Fan-out） |
| **Sink** | 事件消费者（Knative Service / K8s Service / Broker） |

## 2. Broker / Trigger 模型

### 2.1 创建 Broker

```yaml
apiVersion: eventing.knative.dev/v1
kind: Broker
metadata:
  name: default
  namespace: production
  annotations:
    # 使用 MTChannelBasedBroker（默认）或 Kafka
    eventing.knative.dev/broker.class: MTChannelBasedBroker
spec:
  config:
    apiVersion: v1
    kind: ConfigMap
    name: config-br-default-channel
    namespace: knative-eventing
```

Kafka-backed Broker（生产推荐）：

```yaml
apiVersion: eventing.knative.dev/v1
kind: Broker
metadata:
  name: kafka-broker
  namespace: production
  annotations:
    eventing.knative.dev/broker.class: Kafka
spec:
  config:
    apiVersion: v1
    kind: ConfigMap
    name: kafka-broker-config
    namespace: knative-eventing
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: kafka-broker-config
  namespace: knative-eventing
data:
  default.topic.partitions: "10"
  default.topic.replication.factor: "3"
  bootstrap.servers: "kafka-cluster-kafka-bootstrap.kafka:9092"
```

### 2.2 Trigger 过滤与路由

```yaml
apiVersion: eventing.knative.dev/v1
kind: Trigger
metadata:
  name: order-processor
  namespace: production
spec:
  broker: default
  filter:
    attributes:
      type: com.example.order.created
      source: /apis/v1/orders
      # 扩展属性过滤
      orderregion: "us-west"
  subscriber:
    ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: order-processor
    uri: /events
```

### 2.3 多级 Trigger 路由

```yaml
# 场景：订单事件 → 多个处理器并行处理
---
apiVersion: eventing.knative.dev/v1
kind: Trigger
metadata:
  name: order-inventory
spec:
  broker: default
  filter:
    attributes:
      type: com.example.order.created
  subscriber:
    ref:
      kind: Service
      name: inventory-service
---
apiVersion: eventing.knative.dev/v1
kind: Trigger
metadata:
  name: order-notification
spec:
  broker: default
  filter:
    attributes:
      type: com.example.order.created
  subscriber:
    ref:
      kind: Service
      name: notification-service
---
apiVersion: eventing.knative.dev/v1
kind: Trigger
metadata:
  name: order-analytics
spec:
  broker: default
  filter:
    attributes:
      type: com.example.order.*
      # 通配符匹配所有订单事件
  subscriber:
    ref:
      kind: Service
      name: analytics-service
```

## 3. Event Sources

### 3.1 KafkaSource

```yaml
apiVersion: sources.knative.dev/v1beta2
kind: KafkaSource
metadata:
  name: order-events-source
  namespace: production
spec:
  consumerGroup: knative-eventing-consumer
  bootstrapServers:
    - kafka-cluster-kafka-bootstrap.kafka:9092
  topics:
    - orders.created
    - orders.updated
    - orders.cancelled
  sink:
    ref:
      apiVersion: eventing.knative.dev/v1
      kind: Broker
      name: default
    uri: /orders
  # 认证配置
  net:
    tls:
      enable: true
      cert:
        secretKeyRef:
          name: kafka-tls
          key: tls.crt
      key:
        secretKeyRef:
          name: kafka-tls
          key: tls.key
    sasl:
      enable: true
      type: SCRAM-SHA-512
      password:
        secretKeyRef:
          name: kafka-sasl
          key: password
      user:
        secretKeyRef:
          name: kafka-sasl
          key: user
  # CloudEvents 属性映射
  delivery:
    retry: 3
    backoffPolicy: exponential
    backoffDelay: PT2S
```

### 3.2 ApiServerSource

```yaml
apiVersion: sources.knative.dev/v1
kind: ApiServerSource
metadata:
  name: pod-watcher
  namespace: monitoring
spec:
  serviceAccountName: api-server-source-sa
  mode: Resource    # Resource(完整对象) / Reference(仅引用)
  resources:
    - apiVersion: v1
      kind: Pod
    - apiVersion: apps/v1
      kind: Deployment
  sink:
    ref:
      apiVersion: eventing.knative.dev/v1
      kind: Broker
      name: default
---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: api-server-source-sa
  namespace: monitoring
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: api-server-source-watcher
rules:
  - apiGroups: [""]
    resources: ["pods"]
    verbs: ["get", "list", "watch"]
  - apiGroups: ["apps"]
    resources: ["deployments"]
    verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: api-server-source-watcher-binding
subjects:
  - kind: ServiceAccount
    name: api-server-source-sa
    namespace: monitoring
roleRef:
  kind: ClusterRole
  name: api-server-source-watcher
  apiGroup: rbac.authorization.k8s.io
```

### 3.3 PingSource（定时事件）

```yaml
apiVersion: sources.knative.dev/v1
kind: PingSource
metadata:
  name: health-check-schedule
  namespace: monitoring
spec:
  schedule: "*/5 * * * *"    # 每 5 分钟
  contentType: application/json
  data: '{"checkType": "health", "namespace": "production"}'
  sink:
    ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: health-checker
```

### 3.4 自定义 Source

```yaml
# ContainerSource - 自定义事件源
apiVersion: sources.knative.dev/v1
kind: ContainerSource
metadata:
  name: custom-metric-source
spec:
  template:
    spec:
      containers:
        - image: my-registry/custom-source:v1
          env:
            - name: METRICS_ENDPOINT
              value: "http://prometheus.monitoring:9090"
            - name: SINK_URL
              value: ""    # 由 Controller 自动注入
  sink:
    ref:
      apiVersion: eventing.knative.dev/v1
      kind: Broker
      name: default
```

## 4. CloudEvents 规范

### 4.1 CloudEvents 结构

```json
{
  "specversion": "1.0",
  "type": "com.example.order.created",
  "source": "/apis/v1/orders",
  "id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "time": "2026-07-02T10:00:00Z",
  "datacontenttype": "application/json",
  "subject": "order-12345",
  "data": {
    "orderId": "12345",
    "amount": 99.99,
    "currency": "USD"
  }
}
```

### 4.2 CloudEvents SDK（Go 示例）

```go
package main

import (
    cloudevents "github.com/cloudevents/sdk-go/v2"
    "github.com/cloudevents/sdk-go/v2/protocol/http"
    "log"
    "net/http"
)

func handler(writer http.ResponseWriter, request *http.Request) {
    event, err := cloudevents.NewEventFromHTTPRequest(request)
    if err != nil {
        writer.WriteHeader(http.StatusBadRequest)
        return
    }

    log.Printf("Event: %s/%s/%s", event.Type(), event.Source(), event.ID())

    var data OrderData
    if err := event.DataAs(&data); err != nil {
        writer.WriteHeader(http.StatusInternalServerError)
        return
    }

    // 处理业务逻辑
    processOrder(data)

    writer.WriteHeader(http.StatusOK)
}
```

### 4.3 CloudEvents 属性映射（KafkaSource）

```yaml
# Kafka 消息 Key → CloudEvents 属性
apiVersion: sources.knative.dev/v1beta2
kind: KafkaSource
metadata:
  name: mapped-source
spec:
  bootstrapServers: ["kafka:9092"]
  topics: ["events"]
  consumerGroup: "knative"
  sink:
    ref:
      kind: Broker
      name: default
  # 自定义 CloudEvents 属性
  ceOverrides:
    extensions:
      orderregion: "us-west"
      environment: "production"
```

## 5. Channel / Subscription 模型

### 5.1 InMemoryChannel（开发/测试）

```yaml
apiVersion: messaging.knative.dev/v1
kind: InMemoryChannel
metadata:
  name: orders-channel
  namespace: production
```

### 5.2 KafkaChannel（生产推荐）

```yaml
apiVersion: messaging.knative.dev/v1
kind: KafkaChannel
metadata:
  name: orders-channel
  namespace: production
spec:
  numPartitions: 6
  replicationFactor: 3
  bootstrapServers:
    - kafka-cluster-kafka-bootstrap.kafka:9092
```

### 5.3 Subscription 路由

```yaml
apiVersion: messaging.knative.dev/v1
kind: Subscription
metadata:
  name: orders-to-inventory
  namespace: production
spec:
  channel:
    apiVersion: messaging.knative.dev/v1
    kind: KafkaChannel
    name: orders-channel
  subscriber:
    ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: inventory-service
  reply:
    ref:
      apiVersion: messaging.knative.dev/v1
      kind: KafkaChannel
      name: inventory-results-channel
```

## 6. 事件驱动架构模式

### 6.1 事件编排（Sequence）

```yaml
apiVersion: flows.knative.dev/v1
kind: Sequence
metadata:
  name: order-pipeline
spec:
  channelTemplate:
    apiVersion: messaging.knative.dev/v1
    kind: InMemoryChannel
  steps:
    - ref:
        kind: Service
        name: validate-order
    - ref:
        kind: Service
        name: enrich-order
    - ref:
        kind: Service
        name: persist-order
  reply:
    ref:
      kind: Broker
      name: default
```

### 6.2 事件并行（Parallel）

```yaml
apiVersion: flows.knative.dev/v1
kind: Parallel
metadata:
  name: order-parallel-processing
spec:
  channelTemplate:
    apiVersion: messaging.knative.dev/v1
    kind: InMemoryChannel
  branches:
    - filter:
        ref:
          kind: Service
          name: high-value-filter
      subscriber:
        ref:
          kind: Service
          name: vip-handler
    - filter:
        ref:
          kind: Service
          name: standard-filter
      subscriber:
        ref:
          kind: Service
          name: standard-handler
  reply:
    ref:
      kind: Broker
      name: default
```

## 7. Dead Letter Sink（失败处理）

```yaml
apiVersion: eventing.knative.dev/v1
kind: Broker
metadata:
  name: default
spec:
  config:
    apiVersion: v1
    kind: ConfigMap
    name: config-br-default-channel
    namespace: knative-eventing
  delivery:
    deadLetterSink:
      ref:
        kind: Service
        name: dead-letter-handler
    retry: 5
    backoffPolicy: exponential
    backoffDelay: PT2S
---
apiVersion: eventing.knative.dev/v1
kind: Trigger
metadata:
  name: risky-trigger
spec:
  broker: default
  filter:
    attributes:
      type: com.example.risky.event
  subscriber:
    ref:
      kind: Service
      name: risky-processor
  delivery:
    deadLetterSink:
      ref:
        kind: Service
        name: specific-dlq
    retry: 3
```

## 8. 监控与排障

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Broker 状态
kubectl get broker default -o yaml

# 查看 Trigger 状态
kubectl get trigger -o wide

# 查看事件投递情况
kubectl get eventtype -A

# 检查 Knative Eventing Controller 日志
kubectl -n knative-eventing logs -l app=eventing-controller -f

# 查看 KafkaSource 状态
kubectl get kafkasource -o yaml
```
---

## Related

- [[domain-15-specialized-tech/无服务器/01-knative-serving-deep-dive|Knative Serving 深度解析]]
- [[domain-15-specialized-tech/无服务器/03-openfaas-serverless-functions|OpenFaaS 无服务器函数]]

## See Also

- [Knative Eventing 官方文档](https://knative.dev/docs/eventing/)
- [CloudEvents 规范](https://cloudevents.io/)


<!-- risk-assessed -->
