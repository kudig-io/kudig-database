---
title: '场景: AI 基础设施运维'
description: GPU 调度、模型服务、LLM 部署在 Kubernetes 上的运维
summary: GPU 调度、模型服务、LLM 部署在 Kubernetes 上的运维
category: scenario
tags:
- k8s
- scenario
- ai
- rag
- agent
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-20'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '场景: AI 基础设施运维 是什么'
- '如何 场景: AI 基础设施运维'
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- '场景:'
- AI
- 基础设施运维
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
---



# 场景: AI 基础设施运维

> **场景 ID**: SC-10
> **英文**: AI Infrastructure Operations
> **最后更新**: 2026-05-20

---

## 场景概述

AI 基础设施是 [[Kubernetes|Kubernetes]] 的新兴场景。

---

## 快速决策树

```mermaid
graph TD
    A["AI 基础设施运维"] --> B{"问题确认"}
    B -->|"已知问题"| C["参考相关文档"]
    B -->|"未知问题"| D{"组件定位"}
    D -->|"控制平面"| E["参考 domain-01-cluster-fundamentals"]
    D -->|"工作负载"| F["参考 domain-02-workloads-applications"]
    D -->|"网络"| G["参考 domain-03-networking-traffic"]
    D -->|"存储"| H["参考 domain-04-storage-data"]
    D -->|"安全"| I["参考 domain-05-security-compliance"]

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

- [[domain-14-ai-ml-infra/README.md|README]]
- [[domain-14-ai-ml-infra/topic-ai-agent/README.md|README]]


---

## FTA 故障树

暂无专项 FTA


---

## 操作技能

暂无专项技能卡片


---

## 关联场景

| 关联场景 | 说明 |
|---|---|

## Related

- [[entities/kudig-metadata-index.md|README]].md|README]]
- [[entities/kubernetes.md|kubernetes]]
