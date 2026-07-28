---
title: Cadence (entities)
description: '## 概述'
summary: 'Cadence 是一个分布式、可扩展、持久化的工作流编排引擎，用于以可靠、可扩展的方式执行异步长时间运行的业务逻辑。Cadence 由 Uber 开源，能将复杂的分布式系统交互逻辑简化为简单的编程模型，自动处理失败重试、状态持久化和超时管理。'
category: entities
tags:
- k8s
- cncf
- streaming
- cadence
- mysql
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cadence 是什么
- 如何 Cadence
trigger_keywords:
- Cadence
prerequisites:
- kubectl-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cadence

> **CNCF 状态**: Sandbox | **类别**: Streaming | **主要语言**: Go

## 概述

Cadence 是一个 CNCF 孵化项目，由 Uber 开发，是一个分布式、高可用的 workflow 编排引擎。它简化了构建和管理大规模异步业务流程的复杂性，通过代码定义工作流而非 YAML/JSON 配置。Cadence 保证工作流执行的可靠性——即使进程崩溃、网络中断，工作流也能在恢复后从断点继续执行。它已在 Uber 内部运行超过数千个业务工作流，处理数百万并发执行。

## Key Features（核心能力）

- **代码即工作流**：使用 Go/Java/Python 代码定义工作流逻辑，而非声明式配置
- **执行保证**：工作流在进程崩溃或网络故障后可恢复继续执行
- **Activity 隔离**：将工作流逻辑（确定性的）与副作用操作（Activity）分离
- **自动重试**：Activity 失败自动重试，支持自定义重试策略
- **版本管理**：支持工作流版本控制，确保运行中工作流的兼容性
- **可视化**：提供 Web UI 查看工作流执行历史和状态

## 架构与工作原理

Cadence 架构包含多个组件：Frontend 处理 gRPC/Thrift API 请求；History Service 管理工作流状态机，是核心引擎； Matching Service 负责将 Activity Task 分配给 Worker；Worker（客户端 SDK）执行 Activity 和工作流逻辑。底层使用 Cassandra/MySQL/PostgreSQL 作为持久化存储，通过事件溯源（Event Sourcing）模式记录工作流执行历史，重建工作流状态。

## K8s 集成

Cadence Server 可通过 Helm Chart 部署到 Kubernetes。History Service 和 Worker 通过 Deployment 部署，使用 PVC 或外部数据库存储。Cadence Worker SDK 运行在应用 Pod 中，通过 gRPC 与 Cadence Server 通信。K8s 的滚动更新与 Cadence 的工作流版本管理配合，可实现无缝的工作流代码升级。

## 生产用例

- **订单处理流程**：电商订单的多步骤异步处理（支付、库存、物流）
- **数据管道编排**：编排 ETL 管道、数据校验、异常处理的执行顺序
- **微服务编排**：Saga 模式的分布式事务编排
- **CI/CD 流水线**：替代传统 CI 工具的代码定义式流水线

## 安装与配置

```bash
# 🟢 Helm 安装
helm repo add cadence https://cadence-workflow.github.io/cadence-helm-charts
helm install cadence cadence/cadence \
  -n cadence --create-namespace \
  --set server.replicaCount=3

# 🟢 验证安装
kubectl get pods -n cadence
kubectl get svc -n cadence

# 🟢 检查服务健康
curl http://cadence-frontend.cadence.svc:7933/health

# 🟢 安装 Cadence CLI
go install github.com/uber/cadence/cmd/tools/cli@latest

# 🟢 注册 Domain
cadence --domain default domain register \
  --global_domain false \
  --retention 7

# 🟢 查看 Domain
cadence --domain default domain describe
```

### 工作流代码示例 (Go SDK)

```go
// workflow.go - 订单处理工作流
package workflows

import (
    "time"
    "go.uber.org/cadence/workflow"
)

func OrderWorkflow(ctx workflow.Context, orderID string) error {
    logger := workflow.GetLogger(ctx)
    
    // Activity 选项: 重试策略
    ao := workflow.ActivityOptions{
        StartToCloseTimeout: 30 * time.Second,
        RetryPolicy: &cadence.RetryPolicy{
            InitialInterval:    time.Second,
            BackoffCoefficient: 2.0,
            MaximumInterval:    time.Minute,
            MaximumAttempts:    5,
        },
    }
    ctx = workflow.WithActivityOptions(ctx, ao)
    
    // Step 1: 验证支付
    var paymentResult PaymentResult
    err := workflow.ExecuteActivity(ctx, ValidatePayment, orderID).Get(ctx, &paymentResult)
    if err != nil {
        return err
    }
    
    // Step 2: 扣减库存
    err = workflow.ExecuteActivity(ctx, ReserveInventory, orderID, paymentResult.Items).Get(ctx, nil)
    if err != nil {
        // 补偿: 退款
        workflow.ExecuteActivity(ctx, RefundPayment, orderID, paymentResult.TransactionID).Get(ctx, nil)
        return err
    }
    
    // Step 3: 安排物流
    err = workflow.ExecuteActivity(ctx, ScheduleShipping, orderID).Get(ctx, nil)
    if err != nil {
        // 补偿: 释放库存 + 退款
        workflow.ExecuteActivity(ctx, ReleaseInventory, orderID).Get(ctx, nil)
        workflow.ExecuteActivity(ctx, RefundPayment, orderID, paymentResult.TransactionID).Get(ctx, nil)
        return err
    }
    
    logger.Info("Order processed successfully", "orderID", orderID)
    return nil
}
```

### K8s 部署配置

```yaml
# Cadence Server 配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cadence-frontend
  namespace: cadence
spec:
  replicas: 3
  selector:
    matchLabels:
      app: cadence-frontend
  template:
    spec:
      containers:
      - name: cadence-frontend
        image: ubercadence/server:1.2.0
        ports:
        - containerPort: 7933  # gRPC
        - containerPort: 7934  # Thrift
        env:
        - name: CADENCE_PERSISTENCE_DRIVER
          value: "sql"
        - name: SQL_DRIVER
          value: "postgres"
        - name: SQL_HOST
          value: "postgres.cadence.svc"
        - name: SQL_PORT
          value: "5432"
        - name: SQL_DATABASE
          value: "cadence"
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 2
            memory: 4Gi
```

## 运维操作

### 常用命令

```bash
# 🟢 查看工作流执行历史
cadence --domain default workflow list

# 🟢 查看工作流详情
cadence --domain default workflow show -w <workflow-id> -r <run-id>

# 🟢 查看工作流状态
cadence --domain default workflow describe -w <workflow-id>

# 🟡 终止工作流
cadence --domain default workflow terminate -w <workflow-id> --reason "manual termination"

# 🟡 取消工作流
cadence --domain default workflow cancel -w <workflow-id>

# 🟡 发送信号
cadence --domain default workflow signal -w <workflow-id> -n "approval" -d '"approved"'

# 🟢 查看任务队列
cadence --domain default taskqueue describe -tq order-processing

# 🟢 查看集群信息
cadence cluster health

# 🟢 查看服务日志
kubectl logs -n cadence -l app=cadence-frontend --tail=50
kubectl logs -n cadence -l app=cadence-history --tail=50
kubectl logs -n cadence -l app=cadence-matching --tail=50
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 工作流卡住 | Activity Worker 未运行 | `cadence taskqueue describe -tq <queue>` | 检查 Worker Pod 状态 |
| Activity 超时 | 处理时间过长/Worker 崩溃 | `cadence workflow show -w <id>` | 调整 timeout/检查 Worker 日志 |
| 工作流失败 | 重试耗尽/业务错误 | `cadence workflow describe -w <id>` | 查看失败原因，修复后重置 |
| Server 不可用 | 数据库连接失败 | `kubectl logs -l app=cadence-frontend` | 检查 PostgreSQL 连接 |
| 高延迟 | History 分片不足 | 查看 History 服务指标 | 增加 History 分片数 |
| Worker 连接失败 | gRPC 端口不通 | `kubectl get svc -n cadence` | 检查 Service 和 NetworkPolicy |

### 排查流程

```
1. kubectl get pods -n cadence → 确认服务组件状态
2. cadence cluster health → 检查集群健康
3. cadence workflow describe -w <id> → 查看工作流状态
4. cadence workflow show -w <id> → 查看执行历史
5. kubectl logs -l app=cadence-history → 查看服务日志
6. 检查 Worker 连接和任务队列状态
```

## 生产案例

### 案例1: 电商订单 Saga 编排
- **场景**: 订单处理涉及支付、库存、物流多个微服务
- **方案**: Cadence Workflow 编排 Saga 模式，每步失败自动补偿
- **效果**: 分布式事务可靠性从 99% 提升至 99.99%，无需手动干预

### 案例2: 数据管道编排
- **场景**: ETL 管道包含 20+ 步骤，需要失败重试和断点续传
- **方案**: Cadence 编排数据管道，每步 Activity 独立重试
- **效果**: 管道成功率从 85% 提升至 99.5%，故障恢复时间从小时级降至分钟级

## 对比替代方案

| 维度 | Cadence | Temporal | Argo Workflow | Airflow |
|------|---------|----------|---------------|--------|
| 工作流定义 | 代码 | 代码 | YAML | Python DAG |
| 执行保证 | 强 | 强 | 中 | 弱 |
| 状态持久化 | 自动 | 自动 | Pod 日志 | 元数据库 |
| 复杂逻辑 | 支持 | 支持 | 有限 | 有限 |
| 版本管理 | 支持 | 支持 | 无 | 无 |
| 社区 | Uber | Temporal Inc | CNCF | Apache |
| 学习曲线 | 中 | 中 | 低 | 中 |

## 检查清单

- [ ] Cadence Server 副本数 >= 3 (Frontend/History/Matching)
- [ ] 使用外部数据库 (PostgreSQL/Cassandra) 而非内存
- [ ] Worker 部署充足且监控任务队列积压
- [ ] 工作流配置了合理的 timeout 和 retry 策略
- [ ] Domain 保留期已配置 (retention)
- [ ] 监控工作流执行延迟和失败率
- [ ] Web UI 可访问用于调试
- [ ] 工作流版本管理策略已制定

## Related

- [[koordinator]] — Koordinator
- [[oxia]] — Oxia
- [[krkn]] — Krkn
- [[opengitops]] — OpenGitOps
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cadence
- [[23-实体/drasi.md|[[drasi|Drasi]]]]
- [[23-实体/tremor.md|[[tremor|Tremor]]]]
- [[23-实体/12-数据与消息/nats.md|NATS]]
- [[23-实体/15-参考与索引/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference


<!-- risk-assessed -->
