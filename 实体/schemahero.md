---
title: SchemaHero (entities)
description: '## 概述'
summary: 'SchemaHero 是一个 Kubernetes 原生的数据库 Schema 迁移工具。它采用声明式方法管理数据库表结构，开发者只需定义期望的 Schema 状态，SchemaHero 自动计算并执行所需的 DDL 变更。'
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
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# SchemaHero

> **CNCF 状态**: Sandbox | **类别**: Database | **主要语言**: Go

## 概述

SchemaHero 是一个 Kubernetes 原生的数据库 Schema 迁移工具，由 Replicated 团队开发，2021 年加入 CNCF 沙箱。它采用声明式（Declarative）方法管理数据库表结构，开发者只需定义期望的 Schema 状态，SchemaHero 控制器自动计算当前状态与目标状态的差异（diff），并生成和执行所需的 DDL 变更语句。这一理念与 Kubernetes 的 reconcile 模式一致，让数据库 Schema 管理像管理 Deployment 一样简单。SchemaHero 支持 PostgreSQL、MySQL、CockroachDB、SQLite、Cassandra、MongoDB 等主流数据库，可与 ArgoCD/Flux 等 GitOps 工具无缝集成。

## 核心能力

- **声明式 Schema 管理**: 通过 Table CRD 定义期望的表结构，控制器自动计算并执行 DDL
- **多数据库支持**: PostgreSQL、MySQL、CockroachDB、SQLite、Cassandra、MongoDB、Spanner
- **审批流程**: 生产环境可启用 Approval 机制，DDL 变更需人工审查后才能执行
- **GitOps 集成**: 将 Table CRD 存储在 Git 中，通过 ArgoCD/Flux 实现自动化部署
- **版本控制**: 每个 Schema 变更都有版本记录，支持回滚
- **SQL 预览**: 在执行前生成可预览的 SQL 语句，便于审查

## 架构

SchemaHero 采用 Kubernetes Operator 模式：

- **SchemaHero Manager**: 部署在集群中的控制器，监听 Database 和 Table CRD
- **Database CRD**: 定义数据库连接信息（通过 Kubernetes Secret 引用）
- **Table CRD**: 声明期望的表结构（列、类型、索引、约束）
- **Schema Reconciler**: 核心调谐逻辑，连接数据库获取当前 Schema，与 Table CRD 比对，生成 DDL
- **Migration Job**: 实际执行 DDL 的 Kubernetes Job，使用对应数据库的专用镜像

调谐流程：`Table CRD → Reconciler (diff) → Plan → Approval → Migration Job (DDL) → 数据库`

## K8s 集成

SchemaHero 以 Kubernetes Operator 原生运行，通过 CRD（`Database`、`Table`）声明式管理数据库 Schema。Database CRD 通过 Kubernetes Secret 引用数据库连接字符串，Table CRD 定义表结构。控制器根据 Table CRD 与数据库实际状态的差异自动生成 Migration 计划，通过 Kubernetes Job 执行 DDL。可与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 ArgoCD/Flux GitOps 流程深度集成，实现 Schema 变更的全自动部署和审计。

## 生产场景

1. **GitOps Schema 管理**: 将所有 Table CRD 存储在 Git 仓库，通过 ArgoCD 自动同步到集群
2. **多环境 Schema 一致性**: 开发环境自动执行 DDL，生产环境启用 Approval 人工审查
3. **微服务数据库自治**: 每个微服务团队管理自己的 Table CRD，减少 DBA 介入
4. **灾难恢复 Schema 重建**: 通过 Git 中存储的 Table CRD 完整重建数据库 Schema

## 安装

```bash
# 安装 SchemaHero Operator
kubectl apply -f https://raw.githubusercontent.com/schemahero/schemahero/main/install.yaml

# 安装 schemahero CLI
curl -sL https://get.schemahero.io | sh

# 或使用 krew
kubectl krew install schemahero

# 创建数据库连接
schemahero databases add --name mydb --driver postgres \
  --uri "postgresql://user:pass@host:5432/dbname"
```

## 对比

| 特性 | SchemaHero | Flyway | Liquibase | Atlas |
|------|-----------|--------|-----------|-------|
| 声明式 | ✅ 期望状态 | ❌ 命令式 | ⚠️ 混合 | ✅ 期望状态 |
| K8s 原生 | ✅ CRD + Operator | ❌ CLI | ❌ CLI | ⚠️ 有限 |
| GitOps | ✅ 原生 | ⚠️ 需脚本 | ⚠️ 需脚本 | ⚠️ 有限 |
| 审批流程 | ✅ Approval | ❌ | ❌ | ⚠️ 有限 |

## 架构定位

在 CNCF 生态中，SchemaHero 属于 **Database** 类别，为云原生应用提供声明式数据库 Schema 管理能力。

## 参考链接

- [[flux]]
- [[实体/argocd.md|argocd]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/gitops-principles.md|gitops-principles]]

## Related

- [[modelpack]] — ModelPack
- [[oauth2-proxy]] — OAuth2 Proxy
- [[flux]] — Flux
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/argocd.md|argocd]] — ArgoCD

- schemahero
- [[实体/opengemini.md|openGemini]]
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
