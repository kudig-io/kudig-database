---
title: 发布说明索引 — CI/CD 与 GitOps
description: '- **升级要点**: v2.x 全面重构为组件化架构（Source/Kustomize/Helm/Notification 控制器）'
category: references
tags:
- k8s
- release-notes
- cicd
- gitops
- argo-cd
- flux
- tekton
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — CI/CD 与 GitOps 是什么
- 如何 发布说明索引 — CI/CD 与 GitOps
trigger_keywords:
- 发布说明索引
- CI
- CD
- GitOps
prerequisites:
- kubectl-basics
- helm-basics
created: "2026-05-23"
---

# 发布说明索引 — CI/CD 与 GitOps

> 本文档汇总 CI/CD 与 GitOps 领域 3 个核心项目的发布说明索引，共覆盖 **171 篇**发布说明。

---

## 项目总览

| 项目 | 文件数 | 最新版本 | 最近 Breaking Changes | 说明 |
|------|--------|----------|----------------------|------|
| Argo CD | 40 | v3.3 | v2.4 | GitOps 持续交付 |
| Flux | 51 | v2.8 | v2.5 | GitOps 工具集 |
| Tekton | 80 | v1.11 | v1.11 | 云原生 CI/CD 引擎 |

---

## 项目详情

### Argo CD

- **最新版本**: v3.3
- **发布说明目录**: `domain-19-landscape-references/topic-release-notes/cicd-gitops/argo-cd/`
- **版本覆盖**: v0.1 → v3.3（40 个版本）
- **Breaking Changes 提醒**:
  - v2.4: ApplicationSet 控制器合并与 API 变更
- **升级要点**: v2.x 引入 ApplicationSet 和多集群支持增强

### Flux

- **实体页面**: [[flux|Flux]]
- **最新版本**: v2.8
- **发布说明目录**: `domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/`
- **版本覆盖**: v0.1 → v2.8（51 个版本）
- **Breaking Changes 提醒**:
  - v2.5: Source API 和 Kustomization 控制器行为变更
  - v2.3/v2.4: HelmRelease 和 GitRepository API 调整
- **升级要点**: v2.x 全面重构为组件化架构（Source/Kustomize/Helm/Notification 控制器）

### Tekton

- **最新版本**: v1.11
- **发布说明目录**: `domain-19-landscape-references/topic-release-notes/cicd-gitops/tekton/`
- **版本覆盖**: v0.1 → v1.11（80 个版本）
- **Breaking Changes 提醒**:
  - v1.11: Task 和 Pipeline API 字段变更
  - v1.0 (里程碑): GA 版本，API 稳定化
- **升级要点**: v1.x 为 GA 版本，API 向后兼容

---

## 跨项目 Breaking Changes 汇总

| 版本 | 项目 | 变更摘要 |
|------|------|----------|
| v2.4 | Argo CD | ApplicationSet 控制器合并 |
| v2.5 | Flux | Source API 和 Kustomization 控制器变更 |
| v1.11 | Tekton | Task/Pipeline API 字段变更 |

---

## 相关导航

- [[concepts/gitops-tool-evolution.md|GitOps 工具演进]]
- [[references/release-notes-reading-guide.md|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## Related

- [[references/release-notes-cli-tools.md|release-notes-cli-tools]] — 发布说明索引 — CLI 工具
- [[references/k8s-production-operations.md|k8s-production-operations]] — 生产运维：GitOps、FinOps、灾备恢复与变更管理
- [[flux]] — Flux
- [[helm]] — Helm
- [[argo]] — Argo Workflows
