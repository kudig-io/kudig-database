---
title: Cadence
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
- mysql
- postgresql
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Cadence 是什么
- 如何 Cadence
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Cadence
- cncf
- landscape
---

# Cadence

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://cadenceworkflow.io/ |
| **GitHub** | https://github.com/uber/cadence |
| **许可证** | MIT |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Cadence 是一个分布式、可扩展、持久化的工作流编排引擎，用于以可靠、可扩展的方式执行异步长时间运行的业务逻辑。Cadence 由 Uber 开源，能将复杂的分布式系统交互逻辑简化为简单的编程模型，自动处理失败重试、状态持久化和超时管理。

### 核心特性

- **持久化工作流**: 工作流状态自动持久化，进程崩溃后可恢复
- **活动 (Activity)**: 可靠执行的独立任务单元，自动重试
- **定时器和 Cron**: 内置定时和 Cron 调度能力
- **信号和查询**: 向运行中的工作流发送信号或查询状态
- **子工作流**: 工作流组合和嵌套
- **版本控制**: 支持工作流定义的安全变更和版本管理
- **多语言 SDK**: Go, Java, Python 客户端

---

## 架构设计

```
┌────────────────────────────────────────────────┐
│             Cadence Service Cluster              │
│                                                  │
│  ┌───────────┐  ┌───────────┐  ┌────────────┐  │
│  │ Frontend  │  │ History   │  │ Matching   │  │
│  │ Service   │  │ Service   │  │ Service    │  │
│  │ (API GW)  │  │ (State)   │  │ (Task Q)   │  │
│  └─────┬─────┘  └─────┬─────┘  └──────┬─────┘  │
│        │               │               │         │
│  ┌─────┴───────────────┴───────────────┴──────┐ │
│  │         Persistence Layer                   │ │
│  │  (Cassandra / MySQL / PostgreSQL)           │ │
│  └─────────────────────────────────────────────┘ │
└──────────────────────┬───────────────────────────┘
                       │
         ┌─────────────┼─────────────┐
         ▼             ▼             ▼
   ┌──────────┐  ┌──────────┐  ┌──────────┐
   │ Worker   │  │ Worker   │  │ Worker   │
   │ (Go SDK) │  │(Java SDK)│  │(Python)  │
   │          │  │          │  │          │
   │Workflows │  │Workflows │  │Workflows │
   │Activities│  │Activities│  │Activities│
   └──────────┘  └──────────┘  └──────────┘
```

---

## 快速开始

### Docker Compose 部署

```bash
git clone https://github.com/uber/cadence.git
cd cadence/docker
docker compose up -d
```

### Go SDK 工作流示例

```go
package workflows

import (
    "context"
    "time"

    "go.uber.org/cadence/activity"
    "go.uber.org/cadence/workflow"
    "go.uber.org/zap"
)

// 定义工作流
func OrderWorkflow(ctx workflow.Context, orderID string) error {
    logger := workflow.GetLogger(ctx)
    logger.Info("Starting order workflow", zap.String("orderID", orderID))

    ao := workflow.ActivityOptions{
        ScheduleToStartTimeout: time.Minute,
        StartToCloseTimeout:    5 * time.Minute,
        RetryPolicy: &cadence.RetryPolicy{
            InitialInterval:    time.Second,
            BackoffCoefficient: 2.0,
            MaximumAttempts:    3,
        },
    }
    ctx = workflow.WithActivityOptions(ctx, ao)

    // 步骤 1: 验证订单
    var validationResult string
    err := workflow.ExecuteActivity(ctx, ValidateOrder, orderID).Get(ctx, &validationResult)
    if err != nil {
        return err
    }

    // 步骤 2: 处理支付
    var paymentResult string
    err = workflow.ExecuteActivity(ctx, ProcessPayment, orderID).Get(ctx, &paymentResult)
    if err != nil {
        // 支付失败 - 执行补偿
        _ = workflow.ExecuteActivity(ctx, CancelOrder, orderID).Get(ctx, nil)
        return err
    }

    // 步骤 3: 等待发货确认（信号）
    signalCh := workflow.GetSignalChannel(ctx, "shipment-confirmed")
    var shipmentID string
    signalCh.Receive(ctx, &shipmentID)

    // 步骤 4: 发送通知
    return workflow.ExecuteActivity(ctx, SendNotification, orderID, shipmentID).Get(ctx, nil)
}

// 定义活动
func ValidateOrder(ctx context.Context, orderID string) (string, error) {
    // 验证订单逻辑
    return "validated", nil
}

func ProcessPayment(ctx context.Context, orderID string) (string, error) {
    // 支付处理逻辑
    return "paid", nil
}

func CancelOrder(ctx context.Context, orderID string) error {
    // 取消订单逻辑
    return nil
}

func SendNotification(ctx context.Context, orderID, shipmentID string) error {
    // 通知逻辑
    return nil
}
```

### Worker 启动

```go
func main() {
    // 创建 Cadence 客户端
    serviceClient := buildCadenceClient()
    
    // 注册 Worker
    workerOptions := worker.Options{
        Logger: zap.NewExample(),
    }
    w := worker.New(serviceClient, "my-domain", "order-tasklist", workerOptions)
    
    // 注册工作流和活动
    w.RegisterWorkflow(OrderWorkflow)
    w.RegisterActivity(ValidateOrder)
    w.RegisterActivity(ProcessPayment)
    w.RegisterActivity(CancelOrder)
    w.RegisterActivity(SendNotification)
    
    // 启动 Worker
    if err := w.Start(); err != nil {
        log.Fatal(err)
    }
    select {} // 阻塞等待
}
```

---

## 高级功能

### Cron 工作流

```go
func CronWorkflow(ctx workflow.Context) error {
    ao := workflow.ActivityOptions{
        StartToCloseTimeout: time.Minute,
    }
    ctx = workflow.WithActivityOptions(ctx, ao)
    return workflow.ExecuteActivity(ctx, DailyReportActivity).Get(ctx, nil)
}

// 启动 Cron 工作流
opts := client.StartWorkflowOptions{
    CronSchedule: "0 9 * * *",  // 每天 9:00 执行
    TaskList:     "report-tasklist",
}
```

### 信号和查询

```go
// 向运行中的工作流发送信号
client.SignalWorkflow(ctx, workflowID, runID, "shipment-confirmed", "SHIP-12345")

// 查询工作流状态
result, err := client.QueryWorkflow(ctx, workflowID, runID, "getStatus")
```

---

## 与 Temporal 对比

| 特性 | Cadence | Temporal |
|:---|:---|:---|
| **起源** | Uber 开源 | Cadence 创始人分支 |
| **协议** | TChannel/Thrift | gRPC/Protobuf |
| **命名空间** | Domain | Namespace |
| **存储** | Cassandra/MySQL | Cassandra/MySQL/PostgreSQL |
| **社区** | 活跃 | 更活跃 |

---

## 最佳实践

1. **幂等 Activity**: 所有 Activity 实现幂等性，确保重试安全
2. **工作流版本化**: 使用 `workflow.GetVersion()` 安全升级工作流定义
3. **超时配置**: 为每个 Activity 配置合理的超时和重试策略
4. **补偿逻辑**: 关键业务流程实现 Saga 补偿模式
5. **监控**: 监控工作流执行延迟、Activity 失败率和 Task List 积压
6. **持久化选择**: 高吞吐使用 Cassandra，低延迟使用 MySQL

---

## 参考资源

- [Cadence 官方文档](https://cadenceworkflow.io/docs/)
- [Cadence GitHub](https://github.com/uber/cadence)
- [Go SDK](https://github.com/uber-go/cadence-client)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
