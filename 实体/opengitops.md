---
title: OpenGitOps [entities]
description: '## 概述'
summary: 'OpenGitOps 是一个 CNCF Sandbox 项目，定义了 GitOps 的标准原则和最佳实践。它并非一个软件工具，而是一组社区驱动的 GitOps 规范和标准，为 GitOps 实践提供厂商中立的定义和指南。'
category: entities
tags:
- k8s
- cncf
- platform
- opengitops
- argocd
- flux
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
- OpenGitOps 是什么
- 如何 OpenGitOps
trigger_keywords:
- OpenGitOps
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OpenGitOps

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Markdown（规范文档）

## 概述

OpenGitOps 是 CNCF 的一个**规范和工作组**（Sandbox 项目），由 GitOps Working Group 于 2021 年发起。它不是一个软件工具，而是一组社区驱动的 **GitOps 标准定义和最佳实践指南**。其核心产出是 **GitOps Principles**（GitOps 原则）文档，为业界提供了厂商中立的 GitOps 定义。

OpenGitOps 定义了 GitOps 的四大核心原则（The Four Principles of GitOps）：**声明式（Declarative）**、**版本化和不可变（Versioned and Immutable）**、**自动拉取（Pulled Automatically）**和**持续协调（Continuously Reconciled）**。这些原则是 ArgoCD、Flux、Jenkins X、Rancher Fleet 等 GitOps 工具共同遵循的基础标准。

## Key Features（GitOps 四大原则）

- **声明式系统（Declarative）**：系统期望状态用声明式方式描述（YAML/JSON），而非命令式脚本
- **版本化和不可变（Versioned and Immutable）**：所有配置存储在 Git 中，Git 的不可变历史提供完整审计追踪
- **自动拉取（Pulled Automatically）**：状态变更通过 Agent 自动从 Git 拉取应用，而非外部 Push 推送
- **持续协调（Continuously Reconciled）**：Agent 持续比较实际状态与期望状态，自动纠正漂移

## Architecture

OpenGitOps 工作组通过 **Open Governance** 模式运作。产出的 GitOps Principles 文档以 Markdown 格式发布在 GitHub，通过 RFC（Request for Comments）流程由社区审核和更新。工作组也维护 GitOps Landscape（生态地图），列出符合 GitOps 原则的工具和它们的合规性级别。

## K8s 集成

OpenGitOps 原则在 Kubernetes 生态中最佳体现。ArgoCD 和 Flux 是遵循 OpenGitOps 原则的两个 CNCF Graduated 项目。它们都实现了"声明式 + Git 版本化 + 自动拉取 + 持续协调"的完整闭环。OpenGitOps 工作组也与 Kubernetes SIG（特别是 SIG Cluster Lifecycle）合作确保 GitOps 原则与 K8s 最佳实践一致。

## 生产部署要点

- **Git 作为唯一来源**：所有配置变更通过 Git PR/MR 流程管理
- **不可变部署**：使用镜像 digest 而非 mutable tag (如 latest)
- **自动协调**：部署工具应持续监控并纠正状态漂移
- **分离仓库**：应用代码和部署配置使用独立的 Git 仓库
- **审计追踪**：利用 Git 历史提供完整的变更审计日志

## 生产场景

1. **标准化 GitOps 实践**：组织参照 OpenGitOps 原则建立内部 GitOps 标准
2. **工具选型评估**：使用 GitOps Principles 检查列表评估 ArgoCD/Flux 等工具的合规性
3. **团队培训**：基于 OpenGitOps 原则培训团队理解 GitOps 核心概念
4. **审计合规**：向审计方证明组织的 GitOps 实践符合行业标准

## 参考资源

```markdown
# GitOps Principles 速查
1. **Declarative**: 系统状态声明式描述（YAML）
2. **Versioned & Immutable**: 存储在 Git 中，不可变历史
3. **Pulled Automatically**: Agent 自动拉取，非 Push
4. **Continuously Reconciled**: 持续协调实际与期望状态

# 合规检查列表
- [ ] 所有 K8s 资源是否用声明式 YAML 管理？
- [ ] 配置是否存储在 Git 中并通过 PR 审核？
- [ ] 部署是否由集群内 Agent 自动拉取？
- [ ] 是否持续检测和纠正状态漂移？
- [ ] 是否使用不可变标签（digest）而非 latest？
```

## 对比

| 特性 | OpenGitOps | ArgoCD | Flux | Rancher Fleet |
|------|-----------|--------|------|--------------|
| 类型 | 规范/标准 | 工具实现 | 工具实现 | 工具实现 |
| 声明式 | ✅ 定义 | ✅ 遵循 | ✅ 遵循 | ✅ 遵循 |
| 自动拉取 | ✅ 定义 | ✅ | ✅ | ✅ |
| 持续协调 | ✅ 定义 | ✅ | ✅ | ✅ |

## 参考链接

- [[flux]]
- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[概念/gitops-principles.md|gitops-principles]]
- [[概念/declarative-api.md|declarative-api]]

## Related

- [[cohdi]] — Cohdi
- [[koordinator]] — Koordinator
- [[oxia]] — Oxia
- [[krkn]] — Krkn
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- opengitops
- [[概念/GitOps x 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
