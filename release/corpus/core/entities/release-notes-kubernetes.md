---
title: 发布说明索引 — Kubernetes
description: '| v1.2 → v1.36 | CHANGELOG | 35 | 正式变更日志 |'
summary: '| v1.2 → v1.36 | CHANGELOG | 35 | 正式变更日志 |'
category: references
tags:
- k8s
- release-notes
- kubernetes
- changelog
- core
- docker
- daemonset
- rbac
- crd
- webhook
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — Kubernetes 是什么
- 如何 发布说明索引 — Kubernetes
trigger_keywords:
- 发布说明索引
- Kubernetes
prerequisites:
- kubectl-basics
---



# 发布说明索引 — Kubernetes

> 本文档汇总 Kubernetes 核心项目的发布说明索引，共覆盖 **55 篇**发布说明。

---

## 版本覆盖范围

| 范围 | 文件类型 | 数量 | 说明 |
|------|----------|------|------|
| v1.2 → v1.36 | CHANGELOG | 35 | 正式变更日志 |
| v0.19 → v1.1 | RELEASE-NOTES | 20 | 早期发布说明 |

---

## 最新版本

- **最新版本**: v1.1 (RELEASE-NOTES) / v1.36 (CHANGELOG)
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/kubernetes/`

---

## Breaking Changes 重点版本

以下版本包含重大破坏性变更，升级时需特别关注：

| 版本 | 关键 Breaking Changes |
|------|----------------------|
| v1.36 | 最新版本，持续关注弃用 API 移除 |
| v1.35 | API 弃用和移除策略更新 |
| v1.34 | 安全上下文默认值变更 |
| v1.32 | Pod 安全准入控制器默认策略收紧 |
| v1.29 | FlowSchema 和 PriorityLevelConfiguration GA |
| v1.26 | CRI v1 移除 dockershim 残留 |
| v1.25 | PodSecurityPolicy 正式移除 |
| v1.22 | 多项 beta API 移除（extensions/v1beta1 等） |

---

## 版本演进里程碑

| 阶段 | 版本范围 | 关键特性 |
|------|----------|----------|
| 早期探索 | v0.19 → v0.21 | 基础功能验证 |
| 初步成型 | v1.0 → v1.5 | Deployment/RS/DaemonSet GA |
| API 稳定 | v1.6 → v1.10 | RBAC GA、CRD 替代 TPR |
| 扩展成熟 | v1.11 → v1.15 | Pod 优先级、IPVS、Admission Webhook |
| 规模优化 | v1.16 → v1.20 | Topology Manager、Server-Side Apply |
| 安全加固 | v1.21 → v1.25 | PSP 移除、SeccompDefault、Pod 安全准入 |
| 云原生深化 | v1.26 → v1.30 | 冻结旧 API、Sidecar 容器、用户命名空间 |
| 最新迭代 | v1.31 → v1.36 | 持续性能优化和安全增强 |

---

## 相关导航

- [[kubernetes|Kubernetes]]
- [[entities/kubernetes-changelog.md|Kubernetes 变更日志索引]]
- [[concepts/kubernetes-version-evolution.md|Kubernetes 版本演进]]
- [[entities/version-upgrade-guide.md|版本升级指南]]
- [[domain-19-landscape-references/98-merged-indexes/index.md|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## Related

- [[domain-19-landscape-references/98-merged-indexes/index.md|release-notes-reading-guide]] — 发布说明阅读指南
- [[entities/kudig-contribution-guide.md|kudig-contribution-guide]] — 贡献指南、项目概览与版本发布说明
- [[domain-19-landscape-references/98-merged-indexes/index.md|release-notes-security]] — 发布说明索引 — 安全
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
