---
title: '场景: 应用部署'
description: 在 Kubernetes 上部署和运维应用的完整流程
summary: 在 Kubernetes 上部署和运维应用的完整流程
category: scenario
tags:
- k8s
- scenario
- deployment
- statefulset
- daemonset
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
- '场景: 应用部署 是什么'
- '如何 场景: 应用部署'
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- '场景:'
- 应用部署
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
---



# 场景: 应用部署

> **场景 ID**: SC-02
> **英文**: Application Deployment
> **最后更新**: 2026-05-20

---

## 场景概述

应用部署是 [[Kubernetes|Kubernetes]] 最常见的操作场景。本场景汇总了 Deployment、[[StatefulSet|StatefulSet]]、[[DaemonSet|DaemonSet]] 等所有工作负载的部署模式和最佳实践。

---

## 快速决策树

```mermaid
graph TD
    A["应用部署"] --> B{"问题确认"}
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

- domain-02-workloads-applications/02-deployment-production-patterns.md
- [[domain-02-workloads-applications/00-core-workloads/03-statefulset-advanced-operations.md|03 statefulset advanced operations]]
- [[domain-02-workloads-applications/00-core-workloads/04-daemonset-management.md|04 daemonset management]]
- [[domain-18-manifests-patterns/README.md|README]]


---

## FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta.md|pod fta]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/deployment-fta.md|deployment fta]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/statefulset-fta.md|statefulset fta]]


---

## 操作技能

暂无专项技能卡片


---

## 关联场景

| 关联场景 | 说明 |
|---|---|

## Related

- [[entities/kudig-metadata-index.md|README]].md|README]]
- 24-production-deployment-best-practices
- 99-kubernetes-deployment-patterns-architecture
- [[entities/kubernetes.md|kubernetes]]
