---
title: domain-11-production-operations MOC
description: domain-11-production-operations 知识域导航页，覆盖 32 篇文档，按 6 个 topic 子目录组织
category: moc
tags:
- k8s
- moc
- production
- operations
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- domain-11-production-operations MOC 是什么
- 如何 domain-11-production-operations MOC
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- domain-11-production-operations
- MOC
- production
- operations
prerequisites:
- kubectl-basics
- gpu-ml-basics
created: "2026-05-23"
---

# domain-11-production-operations MOC

> **MOC 版本**: 2.1
> **知识域**: domain-11-production-operations
> **文档数量**: 32 篇
> **Topic 数量**: 6 个
> **最后更新**: 2026-05-21
> **用途**: 本知识域的导航入口，汇总所有相关文档、关联领域、和场景入口

---

## 领域概述

生产运维 — 生产最佳实践、容量规划、变更管理。按 **6 个 Topic 子目录** 模块化组织，与 `domain-12-troubleshooting` 的 topic 模式保持一致。

### 知识域定位

| 维度 | 说明 |
|---|---|
| **知识域** | domain-11-production-operations |
| **文档数量** | 32 篇 |
| **Topic 数量** | 6 个 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 📁 Topic 目录

### 🏗️ topic-production-architecture — 架构与设计（6 篇）

生产环境架构设计原则、多云混合部署、边缘计算部署，以及生产架构蓝图。

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|

---

### 🔍 topic-observability-performance — 可观测性与性能（8 篇）

企业级监控体系、日志收集分析平台、APM 应用性能监控，以及集群/网络/存储性能调优和自动扩展指南。

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|

> **关联领域**：domain-06-observability 企业级监控与告警（工具实现层）、domain-06-observability 日志管理与分析（工具实现层）

---

### 🛡️ topic-security-compliance — 安全与合规（3 篇）

零信任安全架构、CIS 基准合规检查、SBOM 软件物料清单。

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|

> **关联领域**：domain-05-security-compliance 云原生安全（工具实现层）

---

### 🔄 topic-automation-platform — 运维自动化（3 篇）

GitOps 流水线实践、基础设施即代码、自动化运维工具链。

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|

> **关联领域**：domain-08-release-change-management GitOps CI-CD、domain-08-release-change-management 基础设施即代码（工具实现层）

---

### 💰 topic-cost-governance — 成本与治理（5 篇）

Kubernetes 成本治理、资源配额管理、绿色计算，以及 FinOps/GreenOps 深度指南。

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|

---

### 📦 topic-reliability-operations — 可靠性与运营（6 篇）

企业级备份策略、灾难恢复演练、跨区域容灾部署，以及变更管理流程、事件响应处理、容量规划与预测。

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|

> **关联领域**：domain-09-reliability-engineering 灾备与业务连续性（工具实现层）、topic-skills 运维技能卡片（横向操作切片）

---

## 开源项目索引

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|

---

## 知识图谱

```mermaid
graph TD
    subgraph domain-11-production-operations
        subgraph topic-production-architecture
            A1["01-生产架构设计原则"]
            A2["02-多云混合部署策略"]
            A3["03-边缘计算生产部署"]
            A99a["99-生产架构蓝图"]
            A99b["99-部署模式架构"]
            A99c["99-多租户架构"]
        end

        subgraph topic-observability-performance
            B1["04-企业级监控体系"]
            B2["05-日志收集分析平台"]
            B3["06-APM应用性能监控"]
            G1["19-集群性能调优"]
            G2["20-网络性能优化"]
            G3["21-存储性能优化"]
            G99a["99-Karpenter自动扩展"]
            G99b["99-KEDA自动缩放"]
        end

        subgraph topic-security-compliance
            C1["07-零信任安全架构"]
            C2["08-CIS基准合规检查"]
            C3["09-软件物料清单"]
        end

        subgraph topic-automation-platform
            D1["10-GitOps流水线实践"]
            D2["11-基础设施即代码"]
            D3["12-自动化运维工具链"]
        end

        subgraph topic-cost-governance
            E1["13-Kubernetes成本治理"]
            E2["14-资源配额管理"]
            E3["15-绿色计算可持续发展"]
            E99a["99-FinOps成本优化"]
            E99b["99-GreenOps可持续计算"]
        end

        subgraph topic-reliability-operations
            F1["16-企业级备份策略"]
            F2["17-灾难恢复演练"]
            F3["18-跨区域容灾部署"]
            H1["22-变更管理流程"]
            H2["23-事件响应处理"]
            H3["24-容量规划预测"]
        end
    end

    A1 --> B1
    A1 --> C1
    B1 --> D1
    C1 --> D1
    D1 --> E1
    E1 --> F1
    F1 --> G1
    G1 --> H1
    H1 --> A2

    style A1 fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style B1 fill:#22c55e,stroke:#166534,color:#fff
    style C1 fill:#ef4444,stroke:#991b1b,color:#fff
    style D1 fill:#f59e0b,stroke:#b45309,color:#fff
    style E1 fill:#8b5cf6,stroke:#5b21b6,color:#fff
    style F1 fill:#06b6d4,stroke:#0e7490,color:#fff
    style G1 fill:#ec4899,stroke:#be185d,color:#fff
    style H1 fill:#10b981,stroke:#047857,color:#fff
```

---

## 关联入口

| 入口 | 说明 |
|---|---|

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 32 |
| Topic 数量 | 6 |
| 覆盖 K8s 版本 | v1.25 - v1.32 |

---

*本文档由重组计划触发更新，最后更新 2026-05-21。*
