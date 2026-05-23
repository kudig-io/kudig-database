---
title: topic-migration MOC
description: topic-migration 专题导航页，覆盖 10 篇文档
category: moc
tags:
- k8s
- moc
- migration
- rag
- gpu
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- topic-migration MOC 是什么
- 如何 topic-migration MOC
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- topic-migration
- MOC
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# topic-migration MOC.md|MOC]]

> **MOC 版本**: 1.0
> **专题**: topic-migration
> **文档数量**: 10 篇
> **最后更新**: 2026-05-21
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

迁移 — 数据迁移、应用迁移、版本升级

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-migration |
| **文档数量** | 10 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[domain-08-release-change-management/topic-migration/01-migration-assessment-planning|01 - 迁移评估与规划]] |  | migration, upgrade |  |
| 2 | [[domain-08-release-change-management/topic-migration/02-ack-target-cluster-design|02 - ACK 目标集群设计与搭建]] |  | migration, upgrade |  |
| 3 | [[domain-08-release-change-management/topic-migration/03-application-workload-migration|03 - 应用工作负载迁移]] |  | migration, upgrade |  |
| 4 | [[domain-08-release-change-management/topic-migration/04-storage-data-migration|04 - 存储与数据迁移]] |  | migration, upgrade, storage |  |
| 5 | [[domain-08-release-change-management/topic-migration/05-network-migration-traffic-cutover|05 - 网络迁移与流量切换]] |  | migration, upgrade, networking |  |
| 6 | [[domain-08-release-change-management/topic-migration/06-stateful-services-migration|06 - 有状态服务迁移]] |  | migration, upgrade |  |
| 7 | [[domain-08-release-change-management/topic-migration/07-observability-security-migration|07 - 可观测性与安全迁移]] |  | migration, upgrade, observability |  |
| 8 | [[domain-08-release-change-management/topic-migration/08-validation-cutover-decommission|08 - 验收、切换与旧集群退役]] |  | migration, upgrade |  |
| 9 | [[domain-08-release-change-management/topic-migration/09-migration-toolchain|09 - 迁移工具链参考]] |  | migration, upgrade |  |
| 10 | [[domain-08-release-change-management/topic-migration/10-real-world-case-study|10 - 生产迁移实战案例]] |  | migration, upgrade, tutorial |  |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 10 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/networking|networking]]
- 01-observability-architecture-overview
- storage
- 05-network-migration-traffic-cutover
- [[_reports/WIKI-LINT-REPORT-2026-05-21|Wiki Lint Report — 2026-05-21]] — Cross-reference
- [[references/release-notes-storage|[[发布说明索引 — 存储|发布说明索引 — 存储]]]] — Cross-reference
- [[references/release-notes-observability|发布说明索引 — 可观测性]] — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[references/release-notes-kubernetes|发布说明索引 — Kubernetes]] — Cross-reference
- [[references/release-notes-security|发布说明索引 — 安全]] — Cross-reference
- [[references/k8s-knowledge-map|Kubernetes Knowledge Map]] — Cross-reference
- [[references/release-notes-cicd-gitops|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[references/release-notes-cli-tools|发布说明索引 — CLI 工具]] — Cross-reference
- [[references/release-notes-core-deps|发布说明索引 — 核心依赖]] — Cross-reference
- [[references/k8s-difficulty-index|Kubernetes Difficulty Index]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- [[domain-03-networking-traffic/00-core-k8s-networking/02-cni-architecture-fundamentals|CNI 架构与核心原理]] — Cross-reference
- [[domain-06-observability/01-overview/01-observability-architecture-overview|Kubernetes 可观测性架构体系]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[domain-01-cluster-fundamentals/05-kubectl/05-kubectl-commands-reference|kubectl 命令完整参考]] — Cross-reference
- [[domain-01-cluster-fundamentals/01-architecture-overview/02-core-components-deep-dive|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[domain-04-storage-data/01-k8s-storage/02-pv-architecture-fundamentals|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[domain-04-storage-data/01-k8s-storage/01-storage-architecture-overview|存储架构概览与核心组件]] — Cross-reference
