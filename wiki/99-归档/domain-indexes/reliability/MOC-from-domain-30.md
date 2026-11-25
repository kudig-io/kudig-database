---
title: domain-30-disaster-recovery-business-continuity MOC
description: domain-30-disaster-recovery-business-continuity 知识域导航页，覆盖 10 篇文档
summary: domain-30-disaster-recovery-business-continuity 知识域导航页，覆盖 10 篇文档
category: moc
tags:
- k8s
- moc
- disaster-recovery
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- domain-30-disaster-recovery-business-continuity MOC 是什么
- 如何 domain-30-disaster-recovery-business-continuity MOC
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- domain-30-disaster-recovery-business-continuity
- MOC
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# domain-30-disaster-recovery-business-continuity MOC

> **MOC 版本**: 1.0
> **知识域**: domain-30-disaster-recovery-business-continuity
> **文档数量**: 10 篇
> **最后更新**: 2026-05-21
> **用途**: 本知识域的导航入口，汇总所有相关文档、关联领域、和场景入口

---

## 领域概述

灾备与业务连续性 — 备份、恢复、容灾演练

### 知识域定位

| 维度 | 说明 |
|---|---|
| **知识域** | domain-30-disaster-recovery-business-continuity |
| **文档数量** | 10 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | Domain-30 灾备与业务连续性 — 开源项目索引 |  | disaster-recovery, backup-restore |  |
| 2 | VMware vSphere 企业级灾备与业务连续性 |  | disaster-recovery, backup-restore |  |
| 3 | Veeam Backup & Replication 企业级备份恢复解决方案 |  | disaster-recovery, backup-restore |  |
| 4 | 企业级容灾架构与混沌工程深度实践 |  | disaster-recovery, backup-restore |  |
| 5 | Commvault 企业级灾备与业务连续性深度实践 |  | disaster-recovery, backup-restore |  |
| 6 | Rubrik 企业级灾备与业务连续性深度实践 |  | disaster-recovery, backup-restore |  |
| 7 | Kubernetes 备份与恢复深度实践 |  | disaster-recovery, backup-restore |  |
| 8 | 混沌工程平台实践：LitmusChaos 与 Chaos Mesh |  | disaster-recovery, backup-restore |  |
| 9 | 应用级灾备架构：多区域部署与故障转移 |  | disaster-recovery, backup-restore |  |
| 10 | Velero 企业级备份恢复实践指南 |  | disaster-recovery, backup-restore, guide |  |

---

## 知识图谱

```mermaid
graph TD
    subgraph domain-30-disaster-recovery-business-continuity
        A["Domain-30 灾备与业务连续性 — 开源项目索引"]
    B["VMware vSphere 企业级灾备与业务连续性"]
    C["Veeam Backup & Replication 企业级备份恢复解决方案"]
    D["企业级容灾架构与混沌工程深度实践"]
    E["Commvault 企业级灾备与业务连续性深度实践"]
    F["Rubrik 企业级灾备与业务连续性深度实践"]
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
| FTA 故障树 | domain-30-disaster-recovery-business-continuity 相关故障树分析 |
| Skills 技能 | domain-30-disaster-recovery-business-continuity 相关操作技能 |
| 深度研究入口 | 语料库索引与向量检索 |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 10 |
| 覆盖 K8s 版本 | v1.25 - v1.32 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*


<!-- risk-assessed -->
