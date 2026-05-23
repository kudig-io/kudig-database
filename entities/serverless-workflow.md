---
title: Serverless Workflow (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- serverless
- serverless-workflow
- crd
- operator
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
created: "2026-05-23"
---

# Serverless Workflow

> **CNCF 状态**: Sandbox | **类别**: Serverless | **主要语言**: Go

## 概述

Serverless Workflow 是一个厂商中立的开源工作流规范，用于定义和编排 Serverless 应用的工作流程。该规范由 CNCF Serverless Working Group 维护，旨在提供一种标准化的、声明式的方式来描述复杂的业务流程、事件驱动的工作流和微服务编排。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

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

- [[concepts/secrets-management|secrets-management]]

## Related

- [[confidential-containers]] — [[Confidential Containers|Confidential Containers]]rs (CoCo)|Confidential Containers (CoCo)]]
- [[k8sgpt]] — K8sGPT
- [[trickster]] — Trickster
- [[bootc]] — bootc
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- serverless-workflow
- [[entities/slimfaas|SlimFaas]]
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
