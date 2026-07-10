---
title: 扩缩容
description: Scale（扩缩容）是调整 Kubernetes 工作负载副本数量的操作。包括手动扩缩容和基于指标的自动扩缩容。...
summary: Scale（扩缩容）是调整 Kubernetes 工作负载副本数量的操作。包括手动扩缩容和基于指标的自动扩缩容。...
category: dictionary
tags:
- k8s
- glossary
- operations
- autoscaling
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 扩缩容 是什么
- Scale 详解
trigger_keywords:
- 扩缩容
- Scale
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 扩缩容

> **英文名**: Scale

## 概述

Scale（扩缩容）是调整 Kubernetes 工作负载副本数量的操作。包括手动扩缩容和基于指标的自动扩缩容。

## 核心概念/原理

### 扩缩容方式

#### 手动扩缩容

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 设置副本数
kubectl scale deployment/my-app --replicas=5

# 通过 apply 修改
kubectl apply -f deployment.yaml
```
#### 自动扩缩容

- **HPA（Horizontal Pod Autoscaler）**：基于 CPU/内存/自定义指标自动调整 Pod 副本数。
- **VPA（Vertical Pod Autoscaler）**：自动调整 Pod 的资源 Request/Limit。
- **Cluster Autoscaler / Karpenter**：自动调整节点数量。

### HPA 示例

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

## 关键机制或特性

- HPA 依赖 Metrics Server 提供的指标数据。
- HPA 的扩缩容有冷却时间（默认缩容 5 分钟，扩容 3 分钟）。
- PDB 限制缩容时的最小可用 Pod 数。

## 使用场景与最佳实践

- 为生产服务配置 HPA 实现自动扩缩容。
- 设置合理的 minReplicas 和 maxReplicas 边界。
- 使用自定义指标（如 QPS、延迟）实现更精准的扩缩容。
- 定期审查 ResourceQuota 确保有足够空间扩容。

## 参考链接

- [Scale - Official Documentation](https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/)

## Related

- [[domain-17-system-foundation/知识字典/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/知识字典/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/知识字典/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/知识字典/operations/cordon.md|Cordon]]
- [[domain-17-system-foundation/知识字典/operations/uncordon.md|Uncordon]]

```

<!-- risk-assessed -->
