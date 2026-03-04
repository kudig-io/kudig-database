# Serverless Workflow

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/serverlessworkflow/specification |
| **官网** | https://serverlessworkflow.io/ |
| **许可证** | Apache-2.0 |
| **规范版本** | 0.9 (Latest) |
| **CNCF 分类** | Serverless / Orchestration |
| **支持格式** | JSON / YAML |

---

## 项目概述

Serverless Workflow 是一个厂商中立的开源工作流规范，用于定义和编排 Serverless 应用的工作流程。该规范由 CNCF Serverless Working Group 维护，旨在提供一种标准化的、声明式的方式来描述复杂的业务流程、事件驱动的工作流和微服务编排。

### 核心价值

- **厂商中立**: 不绑定特定云厂商或运行时
- **声明式**: 使用 JSON/YAML 定义工作流，无需编写代码
- **事件驱动**: 原生支持 CloudEvents 规范
- **丰富语义**: 支持条件、循环、并行、子工作流等高级特性
- **可移植性**: 一次编写，多处运行

---

## 核心概念

### 工作流模型

```
┌─────────────────────────────────────────────────────────────────┐
│                    Serverless Workflow                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                    Workflow Definition                     │  │
│  │                                                            │  │
│  │   id: order-processing                                     │  │
│  │   version: 1.0.0                                           │  │
│  │   specVersion: 0.9                                         │  │
│  │                                                            │  │
│  │   ┌─────────────────────────────────────────────────────┐ │  │
│  │   │                      States                          │ │  │
│  │   │                                                      │ │  │
│  │   │  ┌──────────┐    ┌──────────┐    ┌──────────┐      │ │  │
│  │   │  │ Operation│───▶│ Switch   │───▶│ Parallel │      │ │  │
│  │   │  │  State   │    │  State   │    │  State   │      │ │  │
│  │   │  └──────────┘    └──────────┘    └──────────┘      │ │  │
│  │   │       │               │               │             │ │  │
│  │   │       ▼               ▼               ▼             │ │  │
│  │   │  ┌──────────┐    ┌──────────┐    ┌──────────┐      │ │  │
│  │   │  │  Event   │    │  Sleep   │    │  ForEach │      │ │  │
│  │   │  │  State   │    │  State   │    │  State   │      │ │  │
│  │   │  └──────────┘    └──────────┘    └──────────┘      │ │  │
│  │   │                                                      │ │  │
│  │   └─────────────────────────────────────────────────────┘ │  │
│  │                                                            │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 状态类型

| 状态类型 | 用途 | 描述 |
|:---|:---|:---|
| **Operation** | 执行动作 | 调用函数或服务 |
| **Event** | 事件等待 | 等待一个或多个事件 |
| **Switch** | 条件分支 | 基于数据或事件条件路由 |
| **Sleep** | 延时等待 | 暂停执行指定时间 |
| **Parallel** | 并行执行 | 同时执行多个分支 |
| **ForEach** | 循环迭代 | 遍历数组执行操作 |
| **Inject** | 数据注入 | 向工作流数据注入静态值 |
| **Callback** | 回调等待 | 执行操作后等待回调 |

---

## 架构设计

```
┌───────────────────────────────────────────────────────────────────┐
│                  Serverless Workflow Ecosystem                     │
├───────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                   Workflow Definition                         │ │
│  │                    (JSON / YAML)                              │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                              │                                     │
│                              ▼                                     │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                    Workflow Runtime                           │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │ │
│  │  │    Parser    │  │   Executor   │  │ State Machine│       │ │
│  │  │              │  │              │  │              │       │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘       │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                              │                                     │
│           ┌──────────────────┼──────────────────┐                 │
│           │                  │                  │                  │
│           ▼                  ▼                  ▼                  │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐          │
│  │   Functions  │   │    Events    │   │  Data Store  │          │
│  │              │   │ (CloudEvents)│   │              │          │
│  │ - REST APIs  │   │              │   │ - State      │          │
│  │ - gRPC       │   │ - Kafka      │   │ - Variables  │          │
│  │ - GraphQL    │   │ - NATS       │   │ - Secrets    │          │
│  │ - Lambda     │   │ - HTTP       │   │              │          │
│  └──────────────┘   └──────────────┘   └──────────────┘          │
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                Compatible Runtimes                            │ │
│  │                                                                │ │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐│ │
│  │  │ Synapse │ │ Kogito  │ │Automatiko│ │ Temporal│ │ Direktiv││ │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘│ │
│  │                                                                │ │
│  └──────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 基本工作流示例

```yaml
# hello-world.yaml
id: hello-world
version: '1.0.0'
specVersion: '0.9'
name: Hello World Workflow
description: A simple greeting workflow
start: Greet
states:
  - name: Greet
    type: operation
    actions:
      - functionRef: greetingFunction
        actionDataFilter:
          results: "${ .greeting }"
    end: true

functions:
  - name: greetingFunction
    operation: https://api.example.com/greet
    type: rest
```

### 订单处理工作流

```yaml
id: order-processing
version: '1.0.0'
specVersion: '0.9'
name: Order Processing Workflow
description: Process customer orders with inventory check and payment

start: ReceiveOrder

states:
  # 1. 接收订单
  - name: ReceiveOrder
    type: event
    onEvents:
      - eventRefs:
          - OrderReceivedEvent
        eventDataFilter:
          data: "${ .order }"
    transition: ValidateOrder

  # 2. 验证订单
  - name: ValidateOrder
    type: operation
    actions:
      - name: validateOrderAction
        functionRef:
          refName: validateOrder
          arguments:
            orderId: "${ .order.id }"
            items: "${ .order.items }"
    transition: CheckInventory

  # 3. 检查库存
  - name: CheckInventory
    type: operation
    actions:
      - name: checkInventoryAction
        functionRef:
          refName: checkInventory
          arguments:
            items: "${ .order.items }"
        actionDataFilter:
          results: "${ .inventoryResult }"
    transition: InventoryDecision

  # 4. 库存决策
  - name: InventoryDecision
    type: switch
    dataConditions:
      - condition: "${ .inventoryResult.available == true }"
        transition: ProcessPayment
      - condition: "${ .inventoryResult.available == false }"
        transition: NotifyOutOfStock
    defaultCondition:
      transition: HandleError

  # 5. 处理支付
  - name: ProcessPayment
    type: operation
    actions:
      - name: processPaymentAction
        functionRef:
          refName: processPayment
          arguments:
            orderId: "${ .order.id }"
            amount: "${ .order.total }"
            paymentMethod: "${ .order.paymentMethod }"
        actionDataFilter:
          results: "${ .paymentResult }"
    transition: PaymentDecision

  # 6. 支付决策
  - name: PaymentDecision
    type: switch
    dataConditions:
      - condition: "${ .paymentResult.status == 'success' }"
        transition: FulfillOrder
      - condition: "${ .paymentResult.status == 'failed' }"
        transition: NotifyPaymentFailed
    defaultCondition:
      transition: HandleError

  # 7. 履行订单（并行执行）
  - name: FulfillOrder
    type: parallel
    branches:
      - name: updateInventory
        actions:
          - functionRef: updateInventory
      - name: createShipment
        actions:
          - functionRef: createShipment
      - name: sendConfirmation
        actions:
          - functionRef: sendOrderConfirmation
    completionType: allOf
    transition: OrderComplete

  # 8. 订单完成
  - name: OrderComplete
    type: operation
    actions:
      - name: completeOrderAction
        functionRef:
          refName: markOrderComplete
          arguments:
            orderId: "${ .order.id }"
    end: true

  # 错误处理状态
  - name: NotifyOutOfStock
    type: operation
    actions:
      - functionRef: notifyOutOfStock
    end: true

  - name: NotifyPaymentFailed
    type: operation
    actions:
      - functionRef: notifyPaymentFailed
    end: true

  - name: HandleError
    type: operation
    actions:
      - functionRef: handleError
    end: true

# 函数定义
functions:
  - name: validateOrder
    operation: https://api.example.com/orders/validate
    type: rest
  - name: checkInventory
    operation: https://api.example.com/inventory/check
    type: rest
  - name: processPayment
    operation: https://api.example.com/payments/process
    type: rest
  - name: updateInventory
    operation: https://api.example.com/inventory/update
    type: rest
  - name: createShipment
    operation: https://api.example.com/shipments/create
    type: rest
  - name: sendOrderConfirmation
    operation: https://api.example.com/notifications/order-confirmed
    type: rest
  - name: markOrderComplete
    operation: https://api.example.com/orders/complete
    type: rest
  - name: notifyOutOfStock
    operation: https://api.example.com/notifications/out-of-stock
    type: rest
  - name: notifyPaymentFailed
    operation: https://api.example.com/notifications/payment-failed
    type: rest
  - name: handleError
    operation: https://api.example.com/errors/handle
    type: rest

# 事件定义
events:
  - name: OrderReceivedEvent
    type: com.example.order.received
    source: orders-service
```

---

## 高级特性

### 条件分支 (Switch State)

```yaml
states:
  - name: CustomerTypeSwitch
    type: switch
    dataConditions:
      - name: Premium Customer
        condition: "${ .customer.tier == 'premium' }"
        transition: ApplyPremiumDiscount
      - name: Regular Customer
        condition: "${ .customer.tier == 'regular' }"
        transition: ApplyRegularDiscount
      - name: New Customer
        condition: "${ .customer.isNew == true }"
        transition: ApplyWelcomeDiscount
    defaultCondition:
      transition: NoDiscount
```

### 并行执行 (Parallel State)

```yaml
states:
  - name: ParallelProcessing
    type: parallel
    branches:
      - name: branch1
        actions:
          - functionRef: serviceA
      - name: branch2
        actions:
          - functionRef: serviceB
      - name: branch3
        actions:
          - functionRef: serviceC
    completionType: atLeast
    numCompleted: 2  # 至少完成2个分支即可继续
    transition: MergeResults
```

### 循环迭代 (ForEach State)

```yaml
states:
  - name: ProcessItems
    type: foreach
    inputCollection: "${ .order.items }"
    iterationParam: item
    outputCollection: "${ .processedItems }"
    actions:
      - name: processItem
        functionRef:
          refName: processItem
          arguments:
            itemId: "${ .item.id }"
            quantity: "${ .item.quantity }"
    transition: SummarizeResults
```

### 事件驱动 (Event State)

```yaml
states:
  - name: WaitForApproval
    type: event
    exclusive: true
    onEvents:
      - eventRefs:
          - ApprovedEvent
        eventDataFilter:
          data: "${ .approval }"
        actions:
          - functionRef: processApproval
        transition: Approved
      - eventRefs:
          - RejectedEvent
        eventDataFilter:
          data: "${ .rejection }"
        actions:
          - functionRef: processRejection
        transition: Rejected
    timeouts:
      eventTimeout: P7D  # 7天超时

events:
  - name: ApprovedEvent
    type: com.example.approval.approved
    source: approval-service
  - name: RejectedEvent
    type: com.example.approval.rejected
    source: approval-service
```

### 错误处理

```yaml
states:
  - name: CallExternalService
    type: operation
    actions:
      - functionRef: externalService
    onErrors:
      - errorRef: TimeoutError
        transition: HandleTimeout
      - errorRef: ServiceUnavailable
        transition: RetryOrFallback
      - errorRef: '*'  # 捕获所有错误
        transition: GenericErrorHandler
    transition: Success

errors:
  - name: TimeoutError
    code: TIMEOUT
  - name: ServiceUnavailable
    code: "503"
```

### 重试策略

```yaml
retries:
  - name: DefaultRetry
    delay: PT1S
    maxAttempts: 3
    multiplier: 2
    maxDelay: PT30S
    jitter: 0.1

states:
  - name: CallWithRetry
    type: operation
    actions:
      - functionRef: unreliableService
        retryRef: DefaultRetry
    transition: NextState
```

---

## 运行时实现

### 兼容运行时列表

| 运行时 | 语言 | 特点 |
|:---|:---|:---|
| **Synapse** | .NET | Microsoft 官方实现 |
| **Kogito** | Java | Red Hat 流程自动化 |
| **Automatiko** | Java | 轻量级工作流引擎 |
| **Temporal** | Go/Java | 高可靠工作流编排 |
| **Direktiv** | Go | 事件驱动工作流 |

### 使用 Kogito 运行

```java
// pom.xml 依赖
<dependency>
    <groupId>org.kie.kogito</groupId>
    <artifactId>kogito-serverless-workflow</artifactId>
    <version>1.x.x</version>
</dependency>

// 将 workflow.yaml 放在 src/main/resources 目录
// Kogito 会自动加载和执行工作流
```

### 使用 Synapse (Python SDK)

```python
from synapse.workflow import Workflow, WorkflowRunner

# 加载工作流定义
workflow = Workflow.from_file('order-processing.yaml')

# 创建运行器
runner = WorkflowRunner()

# 执行工作流
result = await runner.run(workflow, input_data={
    'order': {
        'id': 'ORD-001',
        'items': [
            {'id': 'ITEM-1', 'quantity': 2},
            {'id': 'ITEM-2', 'quantity': 1}
        ],
        'total': 99.99,
        'paymentMethod': 'credit_card'
    }
})

print(f"Workflow result: {result}")
```

---

## 与 CloudEvents 集成

### 事件定义

```yaml
events:
  - name: OrderCreated
    type: com.example.orders.created
    source: /orders/service
    dataOnly: false  # 包含完整 CloudEvent 结构
    correlation:
      - contextAttributeName: orderid
        contextAttributeValue: "${ .order.id }"

  - name: PaymentCompleted
    type: com.example.payments.completed
    source: /payments/service
    correlation:
      - contextAttributeName: orderid
        contextAttributeValue: "${ .order.id }"
```

### CloudEvent 格式

```json
{
  "specversion": "1.0",
  "type": "com.example.orders.created",
  "source": "/orders/service",
  "id": "A234-1234-1234",
  "time": "2026-03-04T12:00:00Z",
  "datacontenttype": "application/json",
  "orderid": "ORD-001",
  "data": {
    "order": {
      "id": "ORD-001",
      "customerId": "CUST-123",
      "items": [{"id": "ITEM-1", "quantity": 2}],
      "total": 99.99
    }
  }
}
```

---

## 最佳实践

### 工作流设计原则

1. **幂等性**: 确保操作可重复执行
2. **补偿机制**: 为关键操作设计回滚逻辑
3. **超时处理**: 为所有等待状态设置超时
4. **错误边界**: 明确定义错误处理策略
5. **数据最小化**: 只传递必要的数据

### 命名约定

```yaml
# 工作流 ID: kebab-case
id: order-processing-workflow

# 状态名称: PascalCase
states:
  - name: ProcessPayment
  - name: ValidateOrder

# 函数名称: camelCase
functions:
  - name: validateOrder
  - name: processPayment

# 事件名称: PascalCase + Event 后缀
events:
  - name: OrderCreatedEvent
  - name: PaymentCompletedEvent
```

---

## 参考资源

- [GitHub 仓库](https://github.com/serverlessworkflow/specification)
- [官方规范文档](https://serverlessworkflow.io/docs/specification)
- [SDK 列表](https://github.com/serverlessworkflow/sdk-java)
- [示例工作流](https://github.com/serverlessworkflow/specification/tree/main/examples)
- [CloudEvents 规范](https://cloudevents.io/)
- [CNCF Serverless WG](https://github.com/cncf/wg-serverless)

---

**维护者**: Kudig Team | **许可证**: MIT
