---
title: 限制范围
description: LimitRange 是 Kubernetes 中用于限制命名空间内每个容器/Pod 资源使用范围的策略资源。它为命名空间中的资源使用设定上下限。...
summary: LimitRange 是 Kubernetes 中用于限制命名空间内每个容器/Pod 资源使用范围的策略资源。它为命名空间中的资源使用设定上下限。...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- limitrange
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 限制范围 是什么
- LimitRange 详解
trigger_keywords:
- 限制范围
- LimitRange
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 限制范围

> **英文名**: LimitRange

## 概述

LimitRange 是 Kubernetes 中用于限制命名空间内每个容器/Pod 资源使用范围的策略资源。它为命名空间中的资源使用设定上下限。

## 核心概念/原理

### 核心功能

- **默认值**：为未设置 resources 的容器提供默认的 Request/Limit。
- **最小值**：容器必须至少请求的资源量。
- **最大值**：容器允许设置的最大资源量。
- **比例限制**：Limit 与 Request 的最大比例。

### 示例

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
spec:
  limits:
  - default:        # 默认 Limit
      cpu: 500m
      memory: 512Mi
    defaultRequest: # 默认 Request
      cpu: 200m
      memory: 256Mi
    max:
      cpu: "2"
      memory: 2Gi
    min:
      cpu: 100m
      memory: 128Mi
    type: Container
```

## 关键机制或特性

- LimitRange 仅对新创建的 Pod 生效。
- 可以针对 Container、Pod、PersistentVolumeClaim 三种类型设置。
- 如果 Pod 的资源设置超出 LimitRange 范围，创建请求会被拒绝。

## 使用场景与最佳实践

- 为每个命名空间创建 LimitRange 防止资源滥用。
- 结合 ResourceQuota 实现命名空间级别的资源总量控制。
- 设置合理的默认值，避免未配置 resources 的 Pod 影响节点稳定性。

## 参考链接

- [LimitRange - Official Documentation](https://kubernetes.io/docs/concepts/policy/limit-range/)

## Related

- [[系统基础/知识字典/scheduling/affinity.md|Affinity]]
- [[系统基础/知识字典/scheduling/anti-affinity.md|Anti Affinity]]
- [[系统基础/知识字典/scheduling/taint.md|Taint]]
- [[系统基础/知识字典/scheduling/toleration.md|Toleration]]
- [[系统基础/知识字典/scheduling/node-selector.md|Node Selector]]


<!-- risk-assessed -->
