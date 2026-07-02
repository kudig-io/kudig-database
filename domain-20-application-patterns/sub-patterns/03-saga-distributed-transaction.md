---
title: Saga 分布式事务
description: '编排型与协调型Saga模式、补偿事务设计、Temporal工作流引擎集成与幂等性保证'
summary: '编排型与协调型Saga模式、补偿事务设计、Temporal工作流引擎集成与幂等性保证'
category: application-patterns
tags:
- saga
- distributed-transaction
- temporal
- compensation
- choreography
- orchestration
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
- Saga 分布式事务 是什么
- 如何 实现 Saga 模式
trigger_keywords:
- Saga
- 分布式事务
- 补偿事务
- Temporal
- 最终一致性
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


# Saga 分布式事务

## 1. 概述

Saga 模式将长事务拆分为一系列本地事务，每个本地事务有对应的补偿操作。当某个步骤失败时，按逆序执行已完成步骤的补偿操作，实现最终一致性。相比两阶段提交（2PC），Saga 避免了分布式锁，更适合微服务架构。

## 2. Saga 两种实现模式

### 2.1 编排型 Saga（Orchestration）

由中心协调器（Saga Orchestrator）指挥各服务按序执行：

```
编排型 Saga 流程（订单创建）:

Orchestrator                    Services
    │                              │
    ├──→ CreateOrder ─────────────→│ Order Service
    │    (PENDING)                 │
    │                              │
    ├──→ ReserveStock ────────────→│ Inventory Service
    │    (STOCK_RESERVED)          │
    │                              │
    ├──→ ProcessPayment ──────────→│ Payment Service
    │    (PAYMENT_PROCESSED)       │
    │                              │
    ├──→ ConfirmOrder ────────────→│ Order Service
    │    (CONFIRMED)               │
    │                              │
    └── 完成                        │

失败回滚流程:
    Payment 失败
    │
    ├──→ ReleaseStock ────────────→│ Inventory Service (补偿)
    │                              │
    ├──→ CancelOrder ─────────────→│ Order Service (补偿)
    │                              │
    └── 标记 Saga 失败
```

### 2.2 协调型 Saga（Choreography）

各服务通过事件自主协调，无中心协调器：

```
协调型 Saga 流程:

Order Service                    Inventory Service              Payment Service
    │                                  │                              │
    ├── OrderCreated ─────────────────→│                              │
    │                                  ├── StockReserved ────────────→│
    │                                  │                              ├── PaymentProcessed
    │←─────────────────────────────────┼──────────────────────────────┘
    │                                  │                              │
    └── OrderConfirmed                 │                              │

失败场景:
    Inventory Service 发布 StockReservationFailed
    │
    ├──→ Order Service 监听到 → 取消订单
    └──→ Payment Service 监听到 → 退款（如果已扣款）
```

### 2.3 模式对比

| 维度 | 编排型 (Orchestration) | 协调型 (Choreography) |
|------|----------------------|---------------------|
| **耦合度** | 中心依赖 Orchestrator | 服务间事件耦合 |
| **可见性** | 高（集中管理流程） | 低（流程分散在各服务） |
| **复杂度** | Orchestrator 复杂 | 服务间协调复杂 |
| **适用步骤** | 5+ 步骤的复杂流程 | 2-4 步骤的简单流程 |
| **调试难度** | 低（单点追踪） | 高（需分布式追踪） |
| **单点风险** | Orchestrator 是单点 | 无单点 |
| **扩展性** | 垂直扩展 Orchestrator | 水平扩展各服务 |

## 3. 补偿事务设计

### 3.1 补偿操作设计原则

```
补偿事务设计准则:

1. 幂等性: 补偿操作必须幂等，重复执行结果一致
   ✗ DELETE FROM orders WHERE id = ?           (不幂等)
   ✓ UPDATE orders SET status = 'CANCELLED'    (幂等)

2. 可交换: 补偿操作与并发操作可交换
   → 使用乐观锁或版本号

3. 可重试: 补偿失败后可安全重试
   → 退避重试 + 死信队列

4. 语义补偿: 不是物理回滚，而是业务层面的反向操作
   → 不是 DELETE，而是 CANCEL
   → 不是撤回扣款，而是发起退款

5. 补偿范围: 只补偿已成功完成的步骤
   → Saga 状态机记录每个步骤的完成状态
```

### 3.2 补偿操作实现

```go
// Saga 步骤定义
type SagaStep struct {
    Name       string
    Action     func(ctx context.Context, data SagaData) error
    Compensate func(ctx context.Context, data SagaData) error
    Retries    int
    Timeout    time.Duration
}

// 订单 Saga 步骤
var OrderSagaSteps = []SagaStep{
    {
        Name: "create_order",
        Action: func(ctx context.Context, data SagaData) error {
            return orderService.Create(ctx, data.OrderID, data.Items)
        },
        Compensate: func(ctx context.Context, data SagaData) error {
            return orderService.Cancel(ctx, data.OrderID, "saga_compensation")
        },
        Retries: 3,
        Timeout: 5 * time.Second,
    },
    {
        Name: "reserve_stock",
        Action: func(ctx context.Context, data SagaData) error {
            return inventoryService.Reserve(ctx, data.OrderID, data.Items)
        },
        Compensate: func(ctx context.Context, data SagaData) error {
            return inventoryService.Release(ctx, data.OrderID)
        },
        Retries: 3,
        Timeout: 10 * time.Second,
    },
    {
        Name: "process_payment",
        Action: func(ctx context.Context, data SagaData) error {
            return paymentService.Charge(ctx, data.OrderID, data.TotalAmount)
        },
        Compensate: func(ctx context.Context, data SagaData) error {
            return paymentService.Refund(ctx, data.OrderID, data.TotalAmount)
        },
        Retries: 5,
        Timeout: 30 * time.Second,
    },
}
```

## 4. Temporal 工作流引擎集成

### 4.1 Temporal 架构

```
Temporal on Kubernetes 架构:

┌─────────────────────────────────────────────────┐
│  Kubernetes Cluster                              │
│                                                  │
│  ┌──────────────┐    ┌──────────────┐           │
│  │ Frontend Svc  │    │ History Svc   │           │
│  │ (gRPC API)    │───→│ (状态机)      │           │
│  └──────────────┘    └──────────────┘           │
│         │                    │                   │
│         ▼                    ▼                   │
│  ┌──────────────┐    ┌──────────────┐           │
│  │ Matching Svc  │    │ Worker Svc    │           │
│  │ (任务队列)     │    │ (执行工作流)   │           │
│  └──────────────┘    └──────────────┘           │
│         │                                        │
│         ▼                                        │
│  ┌──────────────────────────────────┐           │
│  │  Cassandra / PostgreSQL / MySQL   │           │
│  │  (持久化存储)                      │           │
│  └──────────────────────────────────┘           │
└─────────────────────────────────────────────────┘
```

### 4.2 Temporal 部署

```yaml
# Temporal Server Helm 部署
apiVersion: helm.toolkit.fluxcd.io/v2beta1
kind: HelmRelease
metadata:
  name: temporal
  namespace: temporal
spec:
  chart:
    spec:
      chart: temporal
      version: 0.35.0
      sourceRef:
        kind: HelmRepository
        name: temporal
  values:
    server:
      replicaCount: 3
      persistence:
        default:
          driver: postgres
          existingClaim: temporal-pvc
    cassandra:
      enabled: false
    elasticsearch:
      enabled: true
      replicas: 3
      persistence:
        volumeClaimTemplate:
          resources:
            requests:
              storage: 50Gi
```

### 4.3 Temporal Workflow 实现

```go
// 订单 Saga Workflow
func OrderSagaWorkflow(ctx workflow.Context, order OrderRequest) error {
    ao := workflow.ActivityOptions{
        StartToCloseTimeout: 30 * time.Second,
        RetryPolicy: &temporal.RetryPolicy{
            InitialInterval:    time.Second,
            BackoffCoefficient: 2.0,
            MaximumInterval:    time.Minute,
            MaximumAttempts:    3,
        },
    }
    ctx = workflow.WithActivityOptions(ctx, ao)

    var sagaState SagaState

    // Step 1: 创建订单
    err := workflow.ExecuteActivity(ctx, CreateOrderActivity, order).Get(ctx, &sagaState.OrderID)
    if err != nil {
        return err
    }

    // Step 2: 锁定库存（带补偿）
    err = workflow.ExecuteActivity(ctx, ReserveStockActivity, order).Get(ctx, nil)
    if err != nil {
        // 补偿：取消订单
        _ = workflow.ExecuteActivity(ctx, CancelOrderActivity, sagaState.OrderID).Get(ctx, nil)
        return err
    }

    // Step 3: 处理支付（带补偿）
    err = workflow.ExecuteActivity(ctx, ProcessPaymentActivity, order).Get(ctx, &sagaState.PaymentID)
    if err != nil {
        // 补偿：释放库存 + 取消订单
        _ = workflow.ExecuteActivity(ctx, ReleaseStockActivity, order.OrderID).Get(ctx, nil)
        _ = workflow.ExecuteActivity(ctx, CancelOrderActivity, sagaState.OrderID).Get(ctx, nil)
        return err
    }

    // Step 4: 确认订单
    err = workflow.ExecuteActivity(ctx, ConfirmOrderActivity, sagaState.OrderID).Get(ctx, nil)
    if err != nil {
        // 补偿：退款 + 释放库存 + 取消订单
        _ = workflow.ExecuteActivity(ctx, RefundPaymentActivity, sagaState.PaymentID).Get(ctx, nil)
        _ = workflow.ExecuteActivity(ctx, ReleaseStockActivity, order.OrderID).Get(ctx, nil)
        _ = workflow.ExecuteActivity(ctx, CancelOrderActivity, sagaState.OrderID).Get(ctx, nil)
        return err
    }

    return nil
}
```

## 5. Saga 状态机

### 5.1 状态定义

```
Saga 状态机:

STARTED
  │
  ▼
STEP_1_RUNNING ──失败──→ COMPENSATING
  │ 成功                    │
  ▼                         ▼
STEP_2_RUNNING ──失败──→ COMP_STEP_1
  │ 成功                    │
  ▼                         ▼
STEP_3_RUNNING ──失败──→ COMP_STEP_2 → COMP_STEP_1
  │ 成功                    │
  ▼                         ▼
COMPLETED               FAILED

状态持久化到数据库，支持断点续传
```

### 5.2 状态持久化

```sql
-- Saga 实例表
CREATE TABLE saga_instances (
    saga_id         UUID PRIMARY KEY,
    saga_type       VARCHAR(100) NOT NULL,
    current_step    INT NOT NULL DEFAULT 0,
    state           VARCHAR(20) NOT NULL DEFAULT 'STARTED',
    data            JSONB NOT NULL,
    created_at      TIMESTAMP NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMP NOT NULL DEFAULT NOW(),
    completed_at    TIMESTAMP,
    error_message   TEXT
);

-- Saga 步骤执行记录
CREATE TABLE saga_step_executions (
    id              UUID PRIMARY KEY,
    saga_id         UUID REFERENCES saga_instances(saga_id),
    step_name       VARCHAR(100) NOT NULL,
    step_index      INT NOT NULL,
    status          VARCHAR(20) NOT NULL,  -- PENDING/RUNNING/COMPLETED/COMPENSATED/FAILED
    input           JSONB,
    output          JSONB,
    error           TEXT,
    attempts        INT NOT NULL DEFAULT 0,
    started_at      TIMESTAMP,
    completed_at    TIMESTAMP
);

CREATE INDEX idx_saga_state ON saga_instances(state) WHERE state NOT IN ('COMPLETED', 'FAILED');
```

## 6. 异常处理与幂等性

### 6.1 异常分类处理

```
异常处理策略:

可重试异常（Transient Error）:
  → 网络超时、服务暂时不可用
  → 策略：指数退避重试
  → 退避公式：min(base * 2^attempt + jitter, max_delay)

不可重试异常（Non-Retryable Error）:
  → 业务规则违反、参数错误
  → 策略：立即失败，触发补偿

悬挂异常（Unknown Error）:
  → 不确定操作是否成功
  → 策略：查询操作状态后再决定
  → 实现：每个操作生成唯一幂等键
```

### 6.2 幂等性实现

```go
// 幂等操作包装器
func WithIdempotency(ctx context.Context, key string, fn func() error) error {
    // 尝试获取幂等锁
    acquired, err := redis.SetNX(ctx, "idempotent:"+key, "processing", 24*time.Hour).Result()
    if err != nil {
        return fmt.Errorf("idempotency check failed: %w", err)
    }

    if !acquired {
        // 检查之前执行结果
        result, err := redis.Get(ctx, "idempotent:"+key+":result").Result()
        if err == redis.Nil {
            return fmt.Errorf("operation in progress")
        }
        if result == "success" {
            return nil // 已成功，跳过
        }
        return fmt.Errorf("previous attempt failed: %s", result)
    }

    // 执行操作
    err = fn()

    // 记录结果
    if err != nil {
        redis.Set(ctx, "idempotent:"+key+":result", "failed:"+err.Error(), 24*time.Hour)
    } else {
        redis.Set(ctx, "idempotent:"+key+":result", "success", 24*time.Hour)
    }

    return err
}
```

### 6.3 幂等键设计

```yaml
# 幂等键生成规则
idempotency_keys:
  create_order:
    pattern: "order:create:{customer_id}:{timestamp_minute}"
    ttl: 24h
    scope: per-customer

  reserve_stock:
    pattern: "inventory:reserve:{order_id}:{sku}"
    ttl: 1h
    scope: per-order-item

  process_payment:
    pattern: "payment:charge:{order_id}:{amount}"
    ttl: 24h
    scope: per-order

  refund:
    pattern: "payment:refund:{payment_id}:{amount}"
    ttl: 7d
    scope: per-payment
```

## 7. 监控与可观测性

```yaml
# Saga 监控 Dashboard
apiVersion: v1
kind: ConfigMap
metadata:
  name: saga-dashboard
data:
  dashboard.json: |
    {
      "panels": [
        {
          "title": "Saga 成功率",
          "targets": [{
            "expr": "rate(saga_completed_total[5m]) / rate(saga_started_total[5m]) * 100"
          }]
        },
        {
          "title": "Saga 平均耗时",
          "targets": [{
            "expr": "histogram_quantile(0.95, rate(saga_duration_seconds_bucket[5m]))"
          }]
        },
        {
          "title": "补偿操作触发率",
          "targets": [{
            "expr": "rate(saga_compensation_total[5m]) / rate(saga_started_total[5m]) * 100"
          }]
        },
        {
          "title": "步骤失败分布",
          "targets": [{
            "expr": "topk(5, sum by (step_name)(rate(saga_step_failed_total[5m])))"
          }]
        }
      ]
    }
```

## 8. 最佳实践

```
Saga 设计检查清单:

□ 每个步骤都有对应的补偿操作
□ 补偿操作是幂等的
□ Saga 状态持久化到数据库
□ 使用幂等键防止重复执行
□ 设置合理的超时和重试策略
□ 实现死信队列处理无法恢复的失败
□ 添加分布式追踪（OpenTelemetry）
□ 监控 Saga 成功率和耗时
□ 编写 Saga 的集成测试（包括失败场景）
□ 文档化每个 Saga 的业务流程
```

## Related

- [[domain-20-application-patterns/sub-patterns/02-event-sourcing-cqrs-patterns|Event Sourcing 与 CQRS]]
- [[domain-20-application-patterns/sub-patterns/01-microservice-decomposition-strategies|微服务拆分策略]]
- [[domain-08-release-change-management/|发布与变更管理]]

## See Also

- Temporal 官方文档
- Saga 模式详解
- 分布式事务对比


<!-- risk-assessed -->
