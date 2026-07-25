---
title: topic-migration MOC
description: topic-migration 专题导航页，覆盖 10 篇文档
summary: topic-migration 专题导航页，覆盖 10 篇文档
category: moc
tags:
- k8s
- moc
- migration
- rag
- gpu
tier: supporting
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
| 1 | [[11-发布变更/07-迁移方案/01-migration-assessment-planning.md|01 - 迁移评估与规划]] |  | migration, upgrade |  |
| 2 | [[11-发布变更/07-迁移方案/02-ack-target-cluster-design.md|02 - ACK 目标集群设计与搭建]] |  | migration, upgrade |  |
| 3 | [[11-发布变更/07-迁移方案/03-application-workload-migration.md|03 - 应用工作负载迁移]] |  | migration, upgrade |  |
| 4 | [[11-发布变更/07-迁移方案/04-storage-data-migration.md|04 - 存储与数据迁移]] |  | migration, upgrade, storage |  |
| 5 | [[11-发布变更/07-迁移方案/05-network-migration-traffic-cutover.md|05 - 网络迁移与流量切换]] |  | migration, upgrade, networking |  |
| 6 | [[11-发布变更/07-迁移方案/06-stateful-services-migration.md|06 - 有状态服务迁移]] |  | migration, upgrade |  |
| 7 | [[11-发布变更/07-迁移方案/07-observability-security-migration.md|07 - 可观测性与安全迁移]] |  | migration, upgrade, observability |  |
| 8 | [[11-发布变更/07-迁移方案/08-validation-cutover-decommission.md|08 - 验收、切换与旧集群退役]] |  | migration, upgrade |  |
| 9 | [[11-发布变更/07-迁移方案/09-migration-toolchain.md|09 - 迁移工具链参考]] |  | migration, upgrade |  |
| 10 | [[11-发布变更/07-迁移方案/10-real-world-case-study.md|10 - 生产迁移实战案例]] |  | migration, upgrade, tutorial |  |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 10 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## 迁移最佳实践导航

### 迁移类型对比

| 类型 | 复杂度 | 风险 | 停机时间 |
|---|---|---|---|
| 集群内迁移 | 低 | 低 | 无 |
| 跨集群迁移 | 中 | 中 | 分钟级 |
| 跨云迁移 | 高 | 高 | 小时级 |
| 混合云迁移 | 极高 | 高 | 可控制 |

### 迁移检查清单

1. **评估阶段**：应用依赖分析、数据量评估、网络规划
2. **准备阶段**：目标环境搭建、数据同步、回滚方案
3. **执行阶段**：灰度迁移、流量切换、验证测试
4. **收尾阶段**：旧环境下线、文档更新、复盘总结

## 面试要点

1. **Q：跨集群迁移的关键挑战？**
   A：数据一致性、服务发现、网络连通性、配置差异、版本兼容性、回滚方案。

2. **Q：如何保证迁移零数据丢失？**
   A：双写过渡期、数据校验工具、增量同步、一致性检查、回滚点保留。

3. **Q：迁移失败如何回滚？**
   A：DNS 切回原集群、数据反向同步、保留原环境至验证完成、自动化回滚脚本。

## Related

- [[17-系统基础/05-速查卡/networking.md|networking]]
- 01-observability-architecture-overview
- storage
- 05-network-migration-traffic-cutover
- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[21-生态参考/98-merged-indexes/index.md|[[发布说明索引 — 存储|发布说明索引 — 存储]]]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- 网络 MOC — Cross-reference
- [[05-网络/01-K8s网络核心/02-cni-architecture-fundamentals.md|CNI 架构与核心原理]] — Cross-reference
- [[09-可观测性/01-总览/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]] — Cross-reference
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[01-集群基础/05-kubectl/05-kubectl-commands-reference.md|kubectl 命令完整参考]] — Cross-reference
- [[01-集群基础/01-架构总览/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[06-存储/01-K8s存储/02-pv-architecture-fundamentals.md|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[06-存储/01-K8s存储/01-storage-architecture-overview.md|存储架构概览与核心组件]] — Cross-reference


<!-- risk-assessed -->
