---
title: topic-febm MOC
description: topic-febm 专题导航页，覆盖 10 篇文档
summary: topic-febm 专题导航页，覆盖 10 篇文档
category: moc
tags:
- k8s
- moc
- febm
- agent
- gpu
- rag
tier: core
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: expert
reading_level: expert
audience:
- SRE
- 运维专家
- 技术支持
estimated_read_time: 5min
intent_queries:
- topic-febm MOC 是什么
- 如何 topic-febm MOC
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- topic-febm MOC 故障排查
- topic-febm MOC 排障步骤
trigger_keywords:
- topic-febm
- MOC
- troubleshooting
- diagnostics
- febm
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-febm MOC.md|MOC]]

> **MOC 版本**: 1.0
> **专题**: topic-febm
> **文档数量**: 10 篇
> **最后更新**: 2026-05-21
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

FEBM 取证 — 问题事件取证方法文档

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-febm |
| **文档数量** | 10 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[故障诊断/FEBM方法论/01-febm-theory-foundations.md|[[第一章：FEBM 方法论原理与理论基础|第一章：FEBM 方法论原理与理论基础]]]] |  | febm, troubleshooting |  |
| 2 | [[故障诊断/FEBM方法论/02-febm-technical-implementation.md|[[第二章:FEBM 技术实现体系|第二章:FEBM 技术实现体系]]]] |  | febm, troubleshooting |  |
| 3 | [[故障诊断/FEBM方法论/03-febm-best-practices.md|[[第三章：FEBM 最佳实践|第三章：FEBM 最佳实践]]]] |  | febm, troubleshooting, best-practice |  |
| 4 | [[故障诊断/FEBM方法论/04-febm-agent-ticket-processing.md|第四章：FEBM 对云平台工单智能体托管的意义]] |  | febm, troubleshooting |  |
| 5 | [[故障诊断/FEBM方法论/05-febm-construction-methodology.md|第五章：FEBM 体系建设方法论]] |  | febm, troubleshooting |  |
| 6 | [[故障诊断/FEBM方法论/06-febm-future-evolution.md|第六章：未来演进方向]] |  | febm, troubleshooting |  |
| 7 | [[故障诊断/FEBM方法论/07-febm-appendix.md|第七章:附录]] |  | febm, troubleshooting |  |
| 8 | [[故障诊断/FEBM方法论/08-febm-production-quick-start.md|第八章：FEBM 生产环境快速启动与 Kubernetes 问题取证手册]] |  | febm, troubleshooting, production |  |
| 9 | [[故障诊断/FEBM方法论/febm-methodology-deep-dive.md|法医鉴定循证方法论（FEBM）深度解析]] |  | febm, troubleshooting |  |
| 10 | [[故障诊断/FEBM方法论/fta-febm-joint-diagnosis.md|FTA-FEBM 联合诊断最佳实践]] |  | febm, troubleshooting |  |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 10 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## 学习路径建议

| 阶段 | 目标 | 推荐文档 |
|------|------|----------|
| 入门 | 理解 FEBM 核心思想 | 01-febm-theory-foundations |
| 实践 | 掌握技术实现 | 02-febm-technical-implementation |
| 进阶 | 生产环境应用 | 08-febm-production-quick-start |
| 精通 | 体系建设与演进 | 05-febm-construction-methodology + 06-febm-future-evolution |
| 融合 | FTA+FEBM 联合诊断 | fta-febm-joint-diagnosis |

## FEBM 核心原则

1. **证据优先**：先收集证据再形成假设，避免先入为主
2. **可重现性**：每个诊断步骤必须可重复执行
3. **证据链完整性**：从现象→证据→假设→验证→根因，每步有据可查
4. **最小侵入**：诊断操作不应影响生产环境
5. **时间线还原**：建立精确的事件时间线是根因分析的基础

## 与其他模块的关系

- **FTA 故障树**：提供结构化的故障分解路径，FEBM 提供循证验证方法
- **技能体系**：将 FEBM 方法论落地为可执行的诊断技能
- **多故障场景**：复杂场景需要 FEBM 方法论指导证据收集和因果分析

## Related

- [[实体/kubernetes.md|kubernetes]]
- [[log|log]]
- [[故障诊断/FEBM方法论/08-febm-production-quick-start.md|08-febm-production-quick-start]]
- [[故障诊断/FEBM方法论/01-febm-theory-foundations.md|01-febm-theory-foundations]]
- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[实体/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[实体/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[实体/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[实体/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[实体/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[实体/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[实体/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[实体/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[实体/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- 网络 MOC — Cross-reference
- [[网络/K8s网络核心/02-cni-architecture-fundamentals.md|CNI 架构与核心原理]] — Cross-reference
- [[可观测性/总览/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[集群基础/kubectl/05-kubectl-commands-reference.md|kubectl 命令完整参考]] — Cross-reference
- [[集群基础/架构总览/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[存储/K8s存储/02-pv-architecture-fundamentals.md|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[存储/K8s存储/01-storage-architecture-overview.md|存储架构概览与核心组件]] — Cross-reference


<!-- risk-assessed -->
