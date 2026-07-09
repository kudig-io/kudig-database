---
title: 发布变更 MOC
description: 发布变更 知识域导航页，覆盖 13 篇文档
summary: 发布变更 知识域导航页，覆盖 13 篇文档
category: moc
tags:
- k8s
- moc
- gitops
- argocd
- flux
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布变更 MOC 是什么
- 如何 发布变更 MOC
- Kubernetes 08 release change management 最佳实践
trigger_keywords:
- 发布变更
- MOC
- release
- change
- management
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 发布变更 [[MOC]]

> **MOC 版本**: 1.0
> **知识域**: 发布变更
> **文档数量**: 13 篇
> **最后更新**: 2026-05-21
> **用途**: 本知识域的导航入口，汇总所有相关文档、关联领域、和场景入口

---

## 领域概述

GitOps 与 CI/CD — ArgoCD、Flux、Jenkins、GitHub Actions

### 知识域定位

| 维度 | 说明 |
|---|---|
| **知识域** | 发布变更 |
| **文档数量** | 13 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | Domain-23 GitOps & CI/CD — 开源项目索引 |  | gitops, cicd, devops |  |
| 2 | Argo CD企业级GitOps实践指南 |  | gitops, cicd, devops |  |
| 3 | Jenkins企业级CI/CD流水线深度实践 |  | gitops, cicd, devops |  |
| 4 | GitLab CI/CD 企业级流水线自动化平台 |  | gitops, cicd, devops |  |
| 5 | GitHub Actions Enterprise CI/CD Platform 深度实践 |  | gitops, cicd, devops |  |
| 6 | Tekton 云原生 CI/CD 深度实践 |  | gitops, cicd, devops |  |
| 7 | Flux v2 GitOps 持续交付深度实践 |  | gitops, cicd, devops |  |
| 8 | GitOps 安全与合规深度实践 |  | gitops, cicd, devops |  |
| 9 | CI/CD 流水线模式与渐进式交付深度实践 |  | gitops, cicd, devops |  |
| 10 | Argo CD 企业级 GitOps 实践指南 |  | gitops, cicd, devops |  |
| 11 | Flux GitOps 实践指南 |  | gitops, cicd, devops |  |
| 12 | Tekton 云原生 CI/CD 实践指南 |  | gitops, cicd, devops |  |
| 13 | Tekton Java CI/CD 流水线实践指南 |  | gitops, cicd, devops |  |

---

## 知识图谱

```mermaid
graph TD
    subgraph 发布变更
        A["Domain-23 GitOps & CI/CD — 开源项目索引"]
    B["Argo CD企业级GitOps实践指南"]
    C["Jenkins企业级CI/CD流水线深度实践"]
    D["GitLab CI/CD 企业级流水线自动化平台"]
    E["GitHub Actions Enterprise CI/CD Platform 深度实践"]
    F["Tekton 云原生 CI/CD 深度实践"]
    end

    A --> B
    A --> C
    A --> D
    A --> E
    A --> F

    style A fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style B fill:#22c55e,stroke:#166534,color:#fff
```

---

## 关联入口

| 入口 | 说明 |
|---|---|
| FTA 故障树 | 发布变更 相关故障树分析 |
| Skills 技能 | 发布变更 相关操作技能 |
| 深度研究入口 | 语料库索引与向量检索 |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 13 |
| 覆盖 K8s 版本 | v1.25 - v1.32 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## Related

- [[README]]
- [[README]]
- [[MOC]]


<!-- risk-assessed -->
