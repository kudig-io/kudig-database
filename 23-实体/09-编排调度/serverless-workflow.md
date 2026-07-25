---
title: Serverless Workflow (entities)
description: '## 概述'
summary: 'Serverless Workflow 是一个厂商中立的开源工作流规范，用于定义和编排 Serverless 应用的工作流程。该规范由 CNCF Serverless Working Group 维护，旨在提供一种标准化的、声明式的方式来描述复杂的业务流程、事件驱动的工作流和微服务编排。'
category: entities
tags:
- k8s
- cncf
- serverless
- serverless-workflow
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
- Serverless Workflow 是什么
- 如何 Serverless Workflow
trigger_keywords:
- Serverless
- Workflow
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Serverless Workflow

> **CNCF 状态**: Sandbox | **类别**: Serverless | **主要语言**: Go

## 概述

Serverless Workflow 是一个 CNCF 孵化项目（规范），定义了一种厂商中立的工作流定义 DSL（Domain Specific Language）。它使用 JSON/YAML 格式描述事件驱动的 Serverless 应用工作流，支持状态、操作、事件、错误处理等抽象。该规范由 Red Hat、Google、IBM、NEC 等公司共同推动，目标是解决不同 Serverless 和 Workflow 平台（AWS Step Functions、Azure Logic Apps、Zeebe 等）之间的厂商锁定问题。开发者只需编写一次工作流定义，即可在任何兼容平台上运行。

## Key Features（核心能力）

- **厂商中立 DSL**：标准化的 JSON/YAML 工作流定义语言
- **状态机模型**：支持 Operation、Event、Switch、Parallel、ForEach、Delay 等状态类型
- **事件驱动**：原生支持 CloudEvents 格式的事件触发和处理
- **错误处理**：内置 Retry、Compensation、Error Handler 机制
- **函数即操作**：可将 Serverless Function 定义为工作流操作
- **多 SDK**：提供 Java、Go、TypeScript、Python SDK

## 架构与工作原理

Serverless Workflow 规范定义了一套结构化的工作流描述模型：Workflow 是顶级容器，包含 States（状态）、Functions（函数定义）、Events（事件定义）、Retries（重试策略）。State 是工作流执行的基本单元，每个 State 定义了进入/退出操作和到下一个 State 的转移条件。Runtime 负责解析工作流定义并驱动状态机执行，可以是任何兼容的实现。

## K8s 集成

Serverless Workflow 规范的 K8s 原生实现包括 SonataFlow（原 Kogito Serverless Workflow），通过 CRD 在 K8s 上部署和管理工作流。工作流定义以 JSON/YAML 格式存储在 ConfigMap 或独立的 CRD 中。与 Knative 集成可以将每个工作流操作映射为 Knative Service，实现自动伸缩。

## 生产用例

- **跨云工作流迁移**：一次编写，在 AWS Step Functions 或本地平台运行
- **事件驱动业务流程**：基于 CloudEvents 的订单处理、审批流
- **微服务编排**：将多个微服务编排为复杂业务流程
- **自动化运维流水线**：基础设施部署和配置的工作流编排

## 安装与配置

```bash
# 🟢 安装 SonataFlow Operator (K8s 实现)
kubectl apply -f https://github.com/apache/incubator-kie-kogito-serverless-operator/releases/latest/download/sonataflow-operator.yaml

# 🟢 验证安装
kubectl get pods -n sonataflow-operator-system

# 🟢 Java SDK 集成
# pom.xml:
# <dependency>
#   <groupId>io.serverlessworkflow</groupId>
#   <artifactId>serverlessworkflow-api</artifactId>
#   <version>4.0.0</version>
# </dependency>

# 🟢 Go SDK
go get github.com/serverlessworkflow/sdk-go/v4
```

### 工作流定义示例 (JSON)

```json
{
  "id": "order-processing",
  "version": "1.0",
  "name": "Order Processing Workflow",
  "start": "ValidateOrder",
  "states": [
    {
      "name": "ValidateOrder",
      "type": "operation",
      "actions": [
        {
          "name": "validate",
          "functionRef": "validateOrder"
        }
      ],
      "transition": "ProcessPayment"
    },
    {
      "name": "ProcessPayment",
      "type": "operation",
      "actions": [
        {
          "name": "charge",
          "functionRef": "processPayment"
        }
      ],
      "transition": "ShipOrder",
      "onErrors": [
        {
          "errorRef": "PaymentError",
          "transition": "HandlePaymentError"
        }
      ]
    },
    {
      "name": "ShipOrder",
      "type": "operation",
      "actions": [
        {
          "name": "ship",
          "functionRef": "shipOrder"
        }
      ],
      "end": true
    },
    {
      "name": "HandlePaymentError",
      "type": "operation",
      "actions": [
        {
          "name": "notify",
          "functionRef": "sendNotification"
        }
      ],
      "end": true
    }
  ],
  "functions": [
    {
      "name": "validateOrder",
      "operation": "http://order-service:8080/api/validate#POST"
    },
    {
      "name": "processPayment",
      "operation": "http://payment-service:8080/api/charge#POST"
    },
    {
      "name": "shipOrder",
      "operation": "http://shipping-service:8080/api/ship#POST"
    },
    {
      "name": "sendNotification",
      "operation": "http://notification-service:8080/api/notify#POST"
    }
  ],
  "retries": [
    {
      "name": "defaultRetry",
      "maxAttempts": 3,
      "delay": "PT1S",
      "multiplier": 2.0
    }
  ]
}
```

### K8s CRD 部署

```yaml
apiVersion: sonataflow.org/v1alpha08
kind: SonataFlow
metadata:
  name: order-workflow
  namespace: workflows
spec:
  flow:
    id: order-processing
    version: "1.0"
    start: ValidateOrder
    states:
    - name: ValidateOrder
      type: operation
      actions:
      - name: validate
        functionRef: validateOrder
      transition: ProcessPayment
    - name: ProcessPayment
      type: operation
      actions:
      - name: charge
        functionRef: processPayment
      end: true
  resources:
    configMaps:
    - configMap:
        name: order-functions
      workflowPath: /functions
```

## 运维操作

### 常用命令

```bash
# 🟢 查看工作流
kubectl get sonataflow -A
kubectl describe sonataflow order-workflow -n workflows

# 🟢 查看工作流 Pod
kubectl get pods -n workflows -l app=order-workflow

# 🟢 查看工作流日志
kubectl logs -n workflows -l app=order-workflow --tail=50

# 🟡 触发工作流执行
curl -X POST http://order-workflow.workflows.svc:8080 \
  -H 'Content-Type: application/json' \
  -d '{"orderId": "12345", "amount": 99.99}'

# 🟢 查看工作流执行状态
curl http://order-workflow.workflows.svc:8080/management/processes

# 🟡 删除工作流
kubectl delete sonataflow order-workflow -n workflows
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 工作流未部署 | CRD 格式错误 | `kubectl describe sonataflow` | 检查 JSON/YAML 语法 |
| 状态转换失败 | 函数不可达 | 查看工作流日志 | 检查函数服务端点 |
| 重试耗尽 | 下游服务故障 | 查看执行历史 | 修复下游服务/调整重试 |
| Pod 未就绪 | 资源不足/镜像拉取失败 | `kubectl describe pod` | 检查资源和镜像配置 |

### 排查流程

```
1. kubectl get sonataflow → 确认工作流状态
2. kubectl describe sonataflow → 查看部署状态
3. kubectl logs -l app=<workflow> → 查看执行日志
4. 检查函数服务端点可达性
5. 验证工作流定义语法
```

## 生产案例

### 案例1: 跨云工作流迁移
- **场景**: 工作流从 AWS Step Functions 迁移到本地 K8s
- **方案**: 使用 Serverless Workflow 规范重写，部署到 SonataFlow
- **效果**: 消除云厂商锁定，工作流可在任何兼容平台运行

### 案例2: 事件驱动订单处理
- **场景**: 订单事件触发多步骤处理流程
- **方案**: Serverless Workflow + CloudEvents 事件触发
- **效果**: 标准化工作流定义，支持多种事件源

## 对比替代方案

| 维度 | Serverless Workflow | Argo Workflow | AWS Step Functions | Temporal |
|------|--------------------|--------------|-------------------|----------|
| 标准化 | CNCF 规范 | K8s 原生 | AWS 专有 | 开源 |
| 厂商锁定 | 无 | 无 | AWS | 无 |
| 事件驱动 | CloudEvents | 有限 | EventBridge | 有限 |
| 状态类型 | 丰富 | DAG | 丰富 | 代码 |
| 学习曲线 | 中 | 低 | 中 | 中 |

## 检查清单

- [ ] 工作流定义符合规范语法
- [ ] 函数服务端点已配置并可达
- [ ] 错误处理和重试策略已定义
- [ ] 工作流执行有监控和日志
- [ ] 测试覆盖了正常和异常路径
- [ ] 与 CloudEvents 集成已验证 (事件驱动场景)

## Related

- [[confidential-containers]] — Confidential Containersrs (CoCo)|Confidential Containers (CoCo)]]
- [[k8sgpt]] — K8sGPT
- [[trickster]] — Trickster
- [[bootc]] — bootc
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- serverless-workflow
- [[23-实体/09-编排调度/slimfaas.md|SlimFaas]]
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference


<!-- risk-assessed -->
