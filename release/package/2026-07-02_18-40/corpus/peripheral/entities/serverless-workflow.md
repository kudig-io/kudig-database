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
last_updated: 2026-05
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

Serverless Workflow 是一个厂商中立的开源工作流规范，用于定义和编排 Serverless 应用的工作流程。该规范由 CNCF Serverless Working Group 维护，旨在提供一种标准化的、声明式的方式来描述复杂的业务流程、事件驱动的工作流和微服务编排。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **幂等性**: 确保操作可重复执行
- **补偿机制**: 为关键操作设计回滚逻辑
- **超时处理**: 为所有等待状态设置超时
- **错误边界**: 明确定义错误处理策略
- **数据最小化**: 只传递必要的数据
- name: ProcessPayment

## 架构定位

在 CNCF 生态中，serverless-workflow 属于 **Serverless** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[confidential-containers]] — Confidential Containersrs (CoCo)|Confidential Containers (CoCo)]]
- [[k8sgpt]] — K8sGPT
- [[trickster]] — Trickster
- [[bootc]] — bootc
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- serverless-workflow
- [[entities/slimfaas.md|SlimFaas]]
- [[entities/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference


<!-- risk-assessed -->
