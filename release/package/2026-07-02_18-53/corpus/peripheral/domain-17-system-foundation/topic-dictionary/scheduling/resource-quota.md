---
title: 资源配额
description: ResourceQuota 是 Kubernetes 中限制命名空间总资源使用的策略资源。它控制一个命名空间中所有对象消耗的计算资源、存储资源和对象数量的总和。...
summary: ResourceQuota 是 Kubernetes 中限制命名空间总资源使用的策略资源。它控制一个命名空间中所有对象消耗的计算资源、存储资源和对象数量的总和。...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- resource-quota
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 资源配额 是什么
- ResourceQuota 详解
trigger_keywords:
- 资源配额
- ResourceQuota
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 资源配额

> **英文名**: ResourceQuota

## 概述

ResourceQuota 是 Kubernetes 中限制命名空间总资源使用的策略资源。它控制一个命名空间中所有对象消耗的计算资源、存储资源和对象数量的总和。

## 核心概念/原理

### 配额类型

- **计算资源配额**：限制命名空间的总 CPU/内存 Request 和 Limit。
- **存储资源配额**：限制命名空间的总存储请求量和 PVC 数量。
- **对象数量配额**：限制命名空间中特定类型对象的数量（如 Pod、Service、ConfigMap 数量）。

### 示例

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-quota
spec:
  hard:
    requests.cpu: "10"
    requests.memory: 20Gi
    limits.cpu: "20"
    limits.memory: 40Gi
    pods: "50"
    services: "20"
    persistentvolumeclaims: "10"
```

## 关键机制或特性

- ResourceQuota 超出配额时，创建请求会被 API Server 拒绝。
- 启用 ResourceQuota 后，创建 Pod 时必须指定 resources.requests/limits（或使用 LimitRange 默认值）。
- 可以基于 PriorityClass 设置作用域（Scope），只为特定优先级设置配额。

## 使用场景与最佳实践

- 为每个团队/项目的命名空间设置 ResourceQuota。
- 结合 LimitRange 确保单个 Pod 也有资源限制。
- 监控 ResourceQuota 的使用率，及时扩容或优化。
- 使用 `kubectl describe resourcequota` 查看配额使用情况。

## 参考链接

- [ResourceQuota - Official Documentation](https://kubernetes.io/docs/concepts/policy/resource-quotas/)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md|Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity.md|Anti Affinity]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/taint.md|Taint]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/toleration.md|Toleration]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/node-selector.md|Node Selector]]


<!-- risk-assessed -->
