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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/00-core-workloads/01-statefulset-advanced-operations|03 statefulset advanced operations]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-02-workloads-applications/00-core-workloads/02-daemonset-management|04 daemonset management]]
- [[domain-18-manifests-patterns/README.md|README]]


---

## FTA 故障树

- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/pod-fta.md|pod fta]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/deployment-fta.md|deployment fta]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/statefulset-fta.md|statefulset fta]]


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


<!-- risk-assessed -->
