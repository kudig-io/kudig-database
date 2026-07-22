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

## 安装与配置

```bash
# 安装 SchemaHero Operator
kubectl apply -f https://raw.githubusercontent.com/schemahero/schemahero/main/install.yaml

# 安装 schemahero CLI
curl -sL https://get.schemahero.io | sh
# 或使用 krew
kubectl krew install schemahero

# 创建数据库连接
kubectl create secret generic mydb-credentials \
  --from-literal=uri="postgresql://user:pass@host:5432/dbname"
```

```yaml
# Database CRD
apiVersion: databases.schemahero.io/v1alpha4
kind: Database
metadata:
  name: mydb
  namespace: production
spec:
  connection:
    postgres:
      uri:
        valueFrom:
          secretKeyRef:
            name: mydb-credentials
            key: uri
---
# Table CRD 示例
apiVersion: schemas.schemahero.io/v1alpha4
kind: Table
metadata:
  name: users
  namespace: production
spec:
  database: mydb
  name: users
  schema:
    postgres:
      columns:
      - name: id
        type: uuid
        constraints:
          primaryKey: true
      - name: email
        type: varchar(255)
        constraints:
          notNull: true
          unique: true
      - name: created_at
        type: timestamptz
        default: now()
      - name: status
        type: varchar(20)
        default: "'active'"
      indexes:
      - columns: [email]
        isUnique: true
      - columns: [status, created_at]
---
# 启用审批流程（生产环境）
apiVersion: databases.schemahero.io/v1alpha4
kind: Database
metadata:
  name: mydb-prod
spec:
  connection:
    postgres:
      uri:
        valueFrom:
          secretKeyRef:
            name: mydb-credentials
            key: uri
  enableShellCommand: true
  # 启用审批：DDL 需人工确认
  immediateDeploy: false
```

## 运维操作

```bash
# 🟢 低风险：查看数据库和表状态
kubectl get databases -A
kubectl get tables -A
kubectl describe table users -n production

# 🟢 低风险：查看待执行的迁移
kubectl krew schemahero plan --database mydb

# 🟡 中风险：批准待执行的 DDL
kubectl krew schemahero approve --database mydb

# 🟡 中风险：手动触发调谐
kubectl annotate table users -n production schemahero.io/reconcile=true --overwrite

# 🔴 高风险：删除 Table CRD（可能触发 DROP TABLE）
kubectl delete table users -n production

# 🟢 低风险：查看迁移历史
kubectl get migrations -A --sort-by=.metadata.creationTimestamp
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Table CRD 未调谐 | 数据库连接失败 | `kubectl describe database mydb` | 检查 Secret 中的连接串 |
| DDL 执行失败 | SQL 语法不兼容 | `kubectl get migrations -o yaml` | 查看 migration 错误信息，修正 Table spec |
| 审批流程卡住 | 无人批准 DDL | `kubectl krew schemahero plan --database mydb` | 执行 approve 或配置自动审批 |
| 索引创建超时 | 大表加索引锁表 | `SELECT * FROM pg_stat_activity` | 使用 CONCURRENTLY 选项 |
| Schema 漂移 | 手动修改了数据库 | `kubectl krew schemahero plan` | 审查差异，决定回滚或更新 CRD |

```
排查流程：
├── Table 未同步？
│   ├── kubectl describe table → 查看 Events
│   ├── kubectl describe database → 检查连接状态
│   └── 确认 Secret 中的 URI 正确
├── DDL 执行失败？
│   ├── kubectl get migrations → 查看失败记录
│   ├── 检查 SQL 与数据库版本兼容性
│   └── 查看 Migration Job 日志
└── Schema 不一致？
    ├── schemahero plan → 查看待执行变更
    ├── 对比 Table CRD 与实际数据库
    └── 决定更新 CRD 或回滚数据库
```

## 生产案例

### 案例 1：微服务团队自助 Schema 管理

- **场景**：15 个微服务团队频繁需要修改数据库 Schema，DBA 成为瓶颈
- **排查**：每次 Schema 变更需要 DBA 审查 + 手动执行，平均等待 2 天
- **方案**：引入 SchemaHero，每个团队管理自己的 Table CRD，开发环境自动执行，生产环境启用 Approval
- **效果**：Schema 变更从 2 天缩短至 10 分钟，DBA 工作量减少 80%

### 案例 2：GitOps 驱动的 Schema 变更审计

- **场景**：金融公司要求所有数据库变更必须有完整审计跟踪
- **排查**：手动执行 DDL 缺乏审计记录，无法追溯变更历史
- **方案**：Table CRD 存储在 Git，通过 ArgoCD 同步，每次变更自动创建 PR 审查，Migration 记录完整审计日志
- **效果**：通过 SOC 2 审计，所有 Schema 变更 100% 可追溯

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
