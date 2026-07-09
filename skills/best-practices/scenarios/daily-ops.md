---
title: '场景: 日常运维'
description: Kubernetes 日常运维操作手册
summary: Kubernetes 日常运维操作手册
category: scenario
tags:
- k8s
- scenario
- daily-ops
- rag
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-20'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '场景: 日常运维 是什么'
- '如何 场景: 日常运维'
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- '场景:'
- 日常运维
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 场景: 日常运维

> **场景 ID**: SC-09
> **英文**: Daily Operations
> **最后更新**: 2026-05-20

---

## 场景概述

日常运维是保障集群稳定的基础工作。

---

## 快速决策树

```mermaid
graph TD
    A["日常运维"] --> B{"问题确认"}
    B -->|"已知问题"| C["参考相关文档"]
    B -->|"未知问题"| D{"组件定位"}
    D -->|"控制平面"| E["参考 集群基础"]
    D -->|"工作负载"| F["参考 工作负载"]
    D -->|"网络"| G["参考 网络"]
    D -->|"存储"| H["参考 存储"]
    D -->|"安全"| I["参考 安全"]

    C --> J["执行修复"]
    E --> J
    F --> J
    G --> J
    H --> J
    I --> J

    J --> K{"验证"}
    K -->|"已解决"| L["记录关闭"]
    K -->|"未解决"| M["升级到专家"]

    style A fill:#ef4444,stroke:#b91c1c,color:#fff
    style L fill:#22c55e,stroke:#166534,color:#fff
    style M fill:#f59e0b,stroke:#b45309,color:#fff
```

---

## 相关文档

- [[平台工程/README.md|README]]
- 集群基础/05-kubectl-commands-reference.md
- [[故障诊断/topic-skills/README.md|README]]


---

## FTA 故障树

暂无专项 FTA


---

## 操作技能

- [[故障诊断/topic-skills/MOC.md|所有操作技能]]


---

## 关联场景

| 关联场景 | 说明 |
|---|---|

## Related

- [[entities/kudig-metadata-index.md|README]].md|README]]
- MOC.md|MOC]]
- 05-kubectl-commands-reference


<!-- risk-assessed -->
