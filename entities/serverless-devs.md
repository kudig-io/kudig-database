---
title: Serverless Devs
description: '## 概述'
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

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，serverless-devs 属于 **Serverless** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[concepts/secrets-management.md|secrets-management]]
- [[entities/kube-scheduler.md|kube-scheduler]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[oauth2-proxy]] — OAuth2 Proxy
- [[schemahero]] — SchemaHero
- [[composefs]] — composefs
- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/sandbox/serverless-devs/serverless-devs.md|serverless-devs]]
- [[entities/slimfaas.md|SlimFaas]]
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
