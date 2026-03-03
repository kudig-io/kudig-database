# Dapr

> **成熟度**: Graduated | **加入时间**: 2021-11 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://dapr.io |
| **GitHub** | https://github.com/dapr/dapr |
| **文档** | https://docs.dapr.io |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | App Definition & Development |

---

## 项目概述

### 简介
Dapr (Distributed Application Runtime) 是一个可移植的、事件驱动的运行时，通过 Sidecar 模式为微服务提供构建块(Building Blocks)，简化分布式应用的开发，使开发者专注于业务逻辑而非基础设施复杂性。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2019-10 | 由 Microsoft 开源发布 |
| 2021-02 | v1.0 正式发布 |
| 2021-11 | 捐赠给 CNCF (Incubating) |
| 2024-01 | 晋升为 CNCF Graduated |

### 核心定位
Dapr 是云原生应用开发的"分布式系统工具包"，通过标准化 API 抽象底层基础设施，实现代码与基础设施解耦，支持任意语言和框架。

---

## 架构设计

### Sidecar 架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      Dapr Sidecar 架构                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Application Pod                                                │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │                                                          │   │
│   │  ┌──────────────────┐      ┌──────────────────┐         │   │
│   │  │   Application    │      │   Dapr Sidecar   │         │   │
│   │  │                  │      │                  │         │   │
│   │  │  ┌────────────┐  │ HTTP │  ┌────────────┐  │         │   │
│   │  │  │ Your Code  │◄─┼──────┼─►│ Building   │  │         │   │
│   │  │  │            │  │ gRPC │  │ Blocks API │  │         │   │
│   │  │  └────────────┘  │      │  └────────────┘  │         │   │
│   │  │                  │      │        │         │         │   │
│   │  └──────────────────┘      │        ▼         │         │   │
│   │                            │  ┌────────────┐  │         │   │
│   │                            │  │ Components │  │         │   │
│   │                            │  │ (可插拔)   │  │         │   │
│   │                            │  └────────────┘  │         │   │
│   │                            └──────────────────┘         │   │
│   │                                                          │   │
│   └─────────────────────────────────────────────────────────┘   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Building Blocks

```
┌─────────────────────────────────────────────────────────────────┐
│                    Dapr Building Blocks                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │ Service     │  │   State     │  │  Pub/Sub    │              │
│  │ Invocation  │  │ Management  │  │             │              │
│  │ (服务调用)  │  │  (状态管理) │  │ (发布订阅)  │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
│                                                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │  Bindings   │  │   Actors    │  │  Secrets    │              │
│  │  (输入/输出 │  │  (虚拟Actor │  │ Management  │              │
│  │   绑定)     │  │   模型)     │  │ (密钥管理)  │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
│                                                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │Configuration│  │Distributed  │  │   Crypto    │              │
│  │  (配置管理) │  │   Lock      │  │  (加密)     │              │
│  │             │  │  (分布式锁) │  │             │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
│                                                                  │
│  ┌─────────────┐                                                │
│  │  Workflow   │                                                │
│  │ (工作流编排)│                                                │
│  └─────────────┘                                                │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 核心功能

### 1. 服务调用 (Service Invocation)

```
┌─────────────────────────────────────────────────────────────────┐
│                    服务调用流程                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Service A                              Service B               │
│  ┌──────────────┐                      ┌──────────────┐         │
│  │  App Code    │                      │  App Code    │         │
│  │     │        │                      │     ▲        │         │
│  │     ▼        │                      │     │        │         │
│  │ ┌────────┐   │    mTLS + Retry      │ ┌────────┐   │         │
│  │ │ Dapr   │───┼──────────────────────┼─│ Dapr   │   │         │
│  │ │Sidecar │   │                      │ │Sidecar │   │         │
│  │ └────────┘   │                      │ └────────┘   │         │
│  └──────────────┘                      └──────────────┘         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

```bash
# 调用其他服务 (通过 Dapr Sidecar)
curl http://localhost:3500/v1.0/invoke/order-service/method/orders

# 支持的特性：
# - 服务发现 (mDNS / Kubernetes DNS)
# - 负载均衡 (Round Robin)
# - 自动重试
# - mTLS 加密
# - 访问控制策略
```

```python
# Python SDK 示例
from dapr.clients import DaprClient

with DaprClient() as d:
    result = d.invoke_method(
        app_id='order-service',
        method_name='orders',
        data='{"item": "book"}',
        http_verb='POST'
    )
```

### 2. 状态管理 (State Management)

```yaml
# 状态存储组件配置
apiVersion: dapr.io/v1alpha1
kind: Component
metadata:
  name: statestore
spec:
  type: state.redis
  version: v1
  metadata:
    - name: redisHost
      value: redis:6379
    - name: redisPassword
      secretKeyRef:
        name: redis-secret
        key: password
```

```python
# 状态操作
from dapr.clients import DaprClient

with DaprClient() as d:
    # 保存状态
    d.save_state(
        store_name='statestore',
        key='user-123',
        value='{"name": "John", "email": "john@example.com"}'
    )
    
    # 获取状态
    state = d.get_state(store_name='statestore', key='user-123')
    
    # 带 ETag 的乐观并发控制
    d.save_state(
        store_name='statestore',
        key='user-123',
        value='{"name": "John Updated"}',
        etag=state.etag
    )
    
    # 事务操作
    d.execute_state_transaction(
        store_name='statestore',
        operations=[
            TransactionalStateOperation(key='key1', data='value1', operation_type=TransactionOperationType.upsert),
            TransactionalStateOperation(key='key2', operation_type=TransactionOperationType.delete)
        ]
    )
```

### 3. 发布订阅 (Pub/Sub)

```yaml
# Pub/Sub 组件配置
apiVersion: dapr.io/v1alpha1
kind: Component
metadata:
  name: pubsub
spec:
  type: pubsub.kafka
  version: v1
  metadata:
    - name: brokers
      value: "kafka:9092"
    - name: consumerGroup
      value: "dapr-group"
```

```python
# 发布消息
from dapr.clients import DaprClient

with DaprClient() as d:
    d.publish_event(
        pubsub_name='pubsub',
        topic_name='orders',
        data='{"orderId": "123", "amount": 99.99}',
        data_content_type='application/json'
    )
```

```python
# 订阅消息 (Flask 示例)
from flask import Flask, request
from cloudevents.http import from_http

app = Flask(__name__)

# 声明订阅
@app.route('/dapr/subscribe', methods=['GET'])
def subscribe():
    return [
        {
            'pubsubname': 'pubsub',
            'topic': 'orders',
            'route': '/orders'
        }
    ]

# 处理消息
@app.route('/orders', methods=['POST'])
def handle_order():
    event = from_http(request.headers, request.get_data())
    print(f"Received order: {event.data}")
    return '', 200
```

### 4. Actor 模型

```
┌─────────────────────────────────────────────────────────────────┐
│                    Dapr Actor 模型                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Actor 特性:                                                     │
│  • 单线程执行 (Turn-based concurrency)                          │
│  • 状态自动持久化                                                │
│  • 位置透明 (虚拟 Actor)                                        │
│  • 定时器和提醒器                                                │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Actor Host                           │    │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐                 │    │
│  │  │Actor    │  │Actor    │  │Actor    │                 │    │
│  │  │user-001 │  │user-002 │  │order-99 │                 │    │
│  │  │ ┌─────┐ │  │ ┌─────┐ │  │ ┌─────┐ │                 │    │
│  │  │ │State│ │  │ │State│ │  │ │State│ │                 │    │
│  │  │ └─────┘ │  │ └─────┘ │  │ └─────┘ │                 │    │
│  │  └─────────┘  └─────────┘  └─────────┘                 │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                   │
│                              ▼                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                   State Store                            │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

```python
# Actor 实现示例
from dapr.actor import Actor, ActorInterface, actormethod

class ScoreCardActorInterface(ActorInterface):
    @actormethod(name="AddScore")
    async def add_score(self, points: int) -> None: ...
    
    @actormethod(name="GetScore")
    async def get_score(self) -> int: ...

class ScoreCardActor(Actor, ScoreCardActorInterface):
    async def add_score(self, points: int) -> None:
        current = await self._state_manager.try_get_state('score')
        score = (current.value or 0) + points
        await self._state_manager.set_state('score', score)
        await self._state_manager.save_state()
    
    async def get_score(self) -> int:
        result = await self._state_manager.try_get_state('score')
        return result.value or 0
```

### 5. 工作流 (Workflow)

```python
# Dapr Workflow 示例
from dapr.ext.workflow import WorkflowRuntime, DaprWorkflowContext
import dapr.ext.workflow as wf

# 定义工作流
def order_processing_workflow(ctx: DaprWorkflowContext, order_input: dict):
    # 步骤 1: 验证库存
    inventory = yield ctx.call_activity(check_inventory, input=order_input)
    
    if not inventory['available']:
        return {"status": "failed", "reason": "out of stock"}
    
    # 步骤 2: 处理支付
    payment = yield ctx.call_activity(process_payment, input=order_input)
    
    # 步骤 3: 更新库存
    yield ctx.call_activity(update_inventory, input=order_input)
    
    # 步骤 4: 发送通知
    yield ctx.call_activity(send_notification, input=order_input)
    
    return {"status": "completed", "orderId": order_input['id']}

# 注册工作流
workflow_runtime = WorkflowRuntime()
workflow_runtime.register_workflow(order_processing_workflow)
workflow_runtime.register_activity(check_inventory)
workflow_runtime.register_activity(process_payment)
workflow_runtime.register_activity(update_inventory)
workflow_runtime.register_activity(send_notification)
```

---

## 安装部署

### Kubernetes 安装

```bash
# 使用 Helm 安装 Dapr
helm repo add dapr https://dapr.github.io/helm-charts/
helm repo update

# 安装 Dapr 控制平面
helm install dapr dapr/dapr \
  --namespace dapr-system \
  --create-namespace \
  --set global.ha.enabled=true \
  --set global.mtls.enabled=true

# 验证安装
kubectl get pods -n dapr-system
```

### 应用注入

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service
spec:
  replicas: 3
  selector:
    matchLabels:
      app: order-service
  template:
    metadata:
      labels:
        app: order-service
      annotations:
        # 启用 Dapr Sidecar 注入
        dapr.io/enabled: "true"
        dapr.io/app-id: "order-service"
        dapr.io/app-port: "8080"
        dapr.io/enable-api-logging: "true"
        dapr.io/log-level: "info"
        # 性能配置
        dapr.io/sidecar-cpu-limit: "1"
        dapr.io/sidecar-memory-limit: "512Mi"
    spec:
      containers:
        - name: order-service
          image: myregistry/order-service:latest
          ports:
            - containerPort: 8080
```

---

## 组件生态

| 类型 | 支持的组件 |
|:---|:---|
| **状态存储** | Redis, PostgreSQL, MongoDB, Cassandra, MySQL, CosmosDB, DynamoDB |
| **Pub/Sub** | Kafka, RabbitMQ, NATS, Redis Streams, AWS SNS/SQS, Azure Service Bus |
| **绑定** | Kafka, AWS S3, Azure Blob, HTTP, Cron, SMTP, Twilio |
| **密钥存储** | Kubernetes Secrets, HashiCorp Vault, AWS Secrets Manager, Azure Key Vault |
| **配置存储** | Redis, PostgreSQL, Azure App Configuration |
| **名称解析** | Kubernetes, mDNS, Consul |

---

## 使用场景

### 1. 微服务通信
```python
# 无需关心服务发现和负载均衡
result = dapr_client.invoke_method('payment-service', 'process', data)
```

### 2. 事件驱动架构
```python
# 发布事件到任意消息中间件
dapr_client.publish_event('pubsub', 'orders', order_data)
```

### 3. 有状态工作流
```python
# 编排长时间运行的业务流程
workflow_client.start_workflow('order-workflow', order_input)
```

---

## 参考资源

- [官方文档](https://docs.dapr.io)
- [GitHub Repo](https://github.com/dapr/dapr)
- [CNCF 项目页面](https://www.cncf.io/projects/dapr/)
- [Dapr Quickstarts](https://github.com/dapr/quickstarts)
- [SDK 列表](https://docs.dapr.io/developing-applications/sdks/)

---

**维护者**: Kudig Team | **许可证**: MIT
