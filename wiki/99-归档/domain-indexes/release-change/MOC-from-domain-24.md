---
title: domain-24-infrastructure-as-code MOC
description: domain-24-infrastructure-as-code 知识域导航页，覆盖 7 篇文档
summary: domain-24-infrastructure-as-code 知识域导航页，覆盖 7 篇文档
category: moc
tags:
- k8s
- moc
- iac
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- domain-24-infrastructure-as-code MOC 是什么
- 如何 domain-24-infrastructure-as-code MOC
- Kubernetes 08 release change management 最佳实践
trigger_keywords:
- domain-24-infrastructure-as-code
- MOC
- release
- change
- management
prerequisites:
- kubectl-basics
- gitops-basics
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# domain-24-infrastructure-as-code MOC

> **MOC 版本**: 1.0
> **知识域**: domain-24-infrastructure-as-code
> **文档数量**: 7 篇
> **最后更新**: 2026-05-21
> **用途**: 本知识域的导航入口，汇总所有相关文档、关联领域、和场景入口

---

## 领域概述

基础设施即代码 — Terraform、Pulumi、Crossplane

### 知识域定位

| 维度 | 说明 |
|---|---|
| **知识域** | domain-24-infrastructure-as-code |
| **文档数量** | 7 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | Domain-24 基础设施即代码 — 开源项目索引 |  | iac, terraform |  |
| 2 | Terraform企业级基础设施即代码实践 |  | iac, terraform |  |
| 3 | Ansible企业级自动化运维深度实践 |  | iac, terraform |  |
| 4 | Pulumi Enterprise Infrastructure as Code Platform |  | iac, terraform |  |
| 5 | Azure Resource Manager (ARM) Enterprise 深度实践 |  | iac, terraform |  |
| 6 | Crossplane Enterprise Infrastructure Orchestration 深度实践 |  | iac, terraform |  |
| 7 | Crossplane 平台工程实践指南 |  | iac, terraform, guide |  |

---

## 知识图谱

```mermaid
graph TD
    subgraph domain-24-infrastructure-as-code
        A["Domain-24 基础设施即代码 — 开源项目索引"]
    B["Terraform企业级基础设施即代码实践"]
    C["Ansible企业级自动化运维深度实践"]
    D["Pulumi Enterprise Infrastructure as Code Platform"]
    E["Azure Resource Manager (ARM) Enterprise 深度实践"]
    F["Crossplane Enterprise Infrastructure Orchestration 深度实践"]
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
| FTA 故障树 | domain-24-infrastructure-as-code 相关故障树分析 |
| Skills 技能 | domain-24-infrastructure-as-code 相关操作技能 |
| 深度研究入口 | 语料库索引与向量检索 |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 7 |
| 覆盖 K8s 版本 | v1.25 - v1.32 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*


<!-- risk-assessed -->
