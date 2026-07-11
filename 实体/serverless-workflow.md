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

## 安装与快速开始

```bash
# Java SDK
<dependency>
  <groupId>io.serverlessworkflow</groupId>
  <artifactId>serverlessworkflow-api</artifactId>
  <version>4.0.0</version>
</dependency>
```

## 对比替代方案

相比 Argo Workflow（K8s 原生但非标准），Serverless Workflow 提供厂商中立的规范，避免平台锁定。相比 AWS Step Functions DSL，Serverless Workflow 是开放标准，不绑定云厂商。

## Related

- [[confidential-containers]] — Confidential Containersrs (CoCo)|Confidential Containers (CoCo)]]
- [[k8sgpt]] — K8sGPT
- [[trickster]] — Trickster
- [[bootc]] — bootc
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- serverless-workflow
- [[实体/slimfaas.md|SlimFaas]]
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference


<!-- risk-assessed -->
