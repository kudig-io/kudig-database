---
title: SchemaHero (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- database
- schemahero
- argocd
- flux
- mysql
- postgresql
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SchemaHero 是什么
- 如何 SchemaHero
trigger_keywords:
- SchemaHero
prerequisites:
- kubectl-basics
- gitops-basics
- mysql-basics
created: "2026-05-23"
---

# SchemaHero

> **CNCF 状态**: Sandbox | **类别**: Database | **主要语言**: Go

## 概述

SchemaHero 是一个 Kubernetes 原生的数据库 Schema 迁移工具。它采用声明式方法管理数据库表结构，开发者只需定义期望的 Schema 状态，SchemaHero 自动计算并执行所需的 DDL 变更。支持 PostgreSQL、MySQL、CockroachDB、SQLite 等数据库。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **声明式管理**: 只定义期望的 Schema 状态，让 SchemaHero 计算变更
- **审批流程**: 生产环境始终启用审批流程，审查 DDL 后再执行
- **GitOps**: 将 Table CRD 存储在 Git 中，通过 ArgoCD/Flux 管理
- **增量变更**: 每次只修改一个表结构，便于追踪和回滚
- **数据库密钥**: 使用 Kubernetes Secret 管理数据库连接字符串

## 架构定位

在 CNCF 生态中，schemahero 属于 **Database** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[flux]]
- [[entities/argocd.md|argocd]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/gitops-principles.md|gitops-principles]]

## Related

- [[modelpack]] — ModelPack
- [[oauth2-proxy]] — OAuth2 Proxy
- [[flux]] — Flux
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[entities/argocd.md|argocd]] — ArgoCD

- schemahero
- [[entities/opengemini.md|openGemini]]
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
