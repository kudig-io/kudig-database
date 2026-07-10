---
title: Serverless Devs [entities]
description: '## 概述'
summary: 'Serverless Devs 是一个开源的 Serverless 开发者平台和命令行工具，致力于为开发者提供强大便捷的 Serverless 应用全生命周期管理能力。项目采用组件化设计，支持多云厂商的 Serverless 服务，让开发者能够使用统一的开发体验在不同云平台上开发、部署和管理 Serverless 应用。'
category: entities
tags:
- k8s
- cncf
- serverless
- serverless-devs
- scheduler
- opa
- crd
- operator
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Serverless Devs 是什么
- 如何 Serverless Devs
trigger_keywords:
- Serverless
- Devs
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Serverless Devs

> **CNCF 状态**: Sandbox | **类别**: Serverless | **主要语言**: TypeScript / JavaScript

## 概述

Serverless Devs 是一个开源的 Serverless 开发者平台和命令行工具，致力于为开发者提供强大便捷的 Serverless 应用全生命周期管理能力。项目采用组件化设计，支持多云厂商的 Serverless 服务，让开发者能够使用统一的开发体验在不同云平台上开发、部署和管理 Serverless 应用。

## 核心能力

- **fc**: 阿里云函数计算组件
- **fc-domain**: 自定义域名管理
- **fc-api**: API 网关配置
- **lambda**: AWS Lambda 组件
- **scf**: 腾讯云云函数组件
- **layer**: 层(依赖)管理组件

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，serverless-devs 属于 **Serverless** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[概念/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[概念/secrets-management.md|secrets-management]]
- [[实体/kube-scheduler.md|kube-scheduler]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[oauth2-proxy]] — OAuth2 Proxy
- [[schemahero]] — SchemaHero
- [[composefs]] — composefs
- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- serverless-devs
- [[实体/slimfaas.md|[[SlimFaas|SlimFaas]]]]
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
