---
title: PodDisruptionBudget 中断预算模式
description: PDB 配置模式、滚动更新保护与自愿中断管理
summary: 使用 PodDisruptionBudget 保护应用在自愿中断（节点维护/升级/伸缩）期间的可用性
category: manifests-patterns
tags:
- k8s
- manifests
- reliability
- pdb
- disruption
- ha
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 平台工程师
- SRE
estimated_read_time: 8min
intent_queries:
- PDB 如何配置
- PodDisruptionBudget 最佳实践
- 如何防止 Pod 中断
trigger_keywords:
- pdb
- poddisruptionbudget
- disruption
- minAvailable
- maxUnavailable
prerequisites:
- k8s-deployment-basics
authors:
- name: KUDIG Team
  role: contributor
---

# PodDisruptionBudget 中断预算模式

## 1. 自愿中断 vs 非自愿中断

| 类型 | 原因 | PDB 保护 |
|------|------|----------|
| **自愿中断** | 节点维护、集群升级、HPA 缩容、 draining | ✅ PDB 生效 |
| **非自愿中断** | 硬件故障、内核 panic、网络分区 | ❌ PDB 不生效 |

PDB 只能保护**自愿中断**，非自愿中断需要多副本 + 反亲和性来防御。

## 2. 基础 PDB 配置

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: frontend-pdb
  namespace: production
spec:
  minAvailable: 2               # 始终保持至少 2 个 Pod 可用
  selector:
    matchLabels:
      app: frontend
---
# 或使用 maxUnavailable
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: backend-pdb
  namespace: production
spec:
  maxUnavailable: 1             # 最多允许 1 个 Pod 不可用
  selector:
    matchLabels:
      app: backend
```

## 3. 选择 minAvailable vs maxUnavailable

| 字段 | 适用场景 | 优点 |
|------|----------|------|
| `minAvailable` | 固定副本数应用 | 绝对保证可用数量 |
| `maxUnavailable` | HPA 动态伸缩应用 | 按比例允许中断 |

```yaml
# HPA 场景：按百分比
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: auto-scaled-app-pdb
spec:
  maxUnavailable: 25%           # 最多 25% Pod 不可用
  selector:
    matchLabels:
      app: auto-scaled
```

## 4. 不同应用的 PDB 策略

### 4.1 有状态应用（StatefulSet）

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: postgres-pdb
  namespace: data
spec:
  minAvailable: 1               # 至少保持 1 个副本
  selector:
    matchLabels:
      app: postgresql
```

### 4.2 单实例应用

```yaml
# 单实例应用也应设置 PDB
# 阻止节点 drain（除非 force）
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: critical-singleton-pdb
spec:
  minAvailable: 1               # 唯一实例不能被驱逐
  selector:
    matchLabels:
      app: critical-service
```

### 4.3 DaemonSet（通常不需要 PDB）

DaemonSet 的 Pod 绑定到节点，drain 时会逐个迁移，通常不需要 PDB。

## 5. 滚动更新与 PDB 的关系

PDB **不阻止**滚动更新（Deployment update），只阻止自愿驱逐：

```yaml
# Deployment 滚动更新配置（与 PDB 配合）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
spec:
  replicas: 6
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 2               # 滚动更新时额外允许 2 个 Pod
      maxUnavailable: 1         # 最多 1 个不可用（与 PDB 一致）
```

## 6. 节点 Drain 与 PDB

```bash
# 🟡 中风险：节点维护操作
# drain 时 PDB 会阻止驱逐
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data

# 如果 PDB 阻止了驱逐
# 输出: error: unable to drain node "node-1" due to error: ...
#pod "frontend-xxx" - PodDisruptionBudget ...

# 强制驱逐（谨慎）
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data --force --disable-eviction

# 查看节点上受 PDB 保护的 Pod
kubectl get pods -n production --field-selector spec.nodeName=node-1
```

## 7. 反亲和性 + PDB 组合

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ha-app
spec:
  replicas: 3
  template:
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  app: ha-app
              topologyKey: kubernetes.io/hostname   # 每个 Pod 不同节点
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: ha-app-pdb
spec:
  minAvailable: 2               # 至少 2 个 Pod，配合 3 副本 + 反亲和性
  selector:
    matchLabels:
      app: ha-app
```

## 8. 生产实践

| 实践 | 说明 |
|------|------|
| 所有生产应用都设 PDB | 防止意外驱逐 |
| HPA 应用用百分比 | `maxUnavailable: 25%` |
| PDB 与反亲和性配合 | 跨节点分布 |
| 单实例应用设 `minAvailable: 1` | 阻止 drain |
| 避免 PDB 死锁 | 全集群 PDB 过严会导致 drain 卡死 |
| 监控 PDB 状态 | `kubectl get pdb` |

## 9. 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| drain 卡住 | PDB 阻止驱逐 | 检查应用是否有多副本 |
| PDB 不生效 | selector 不匹配 | 检查标签匹配 |
| 滚动更新太慢 | maxUnavailable 过小 | 调整滚动策略 |

## 10. 全集群 PDB 审计

```bash
# 🟢 低风险：只读审计
# 列出所有 PDB
kubectl get pdb --all-namespaces

# 检查 PDB 状态
kubectl get pdb -n production -o wide

# 查找没有 PDB 的生产应用
kubectl get deployments -n production \
  -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.replicas}{"\n"}{end}'
```

## Related

- [[03-清单模式/08-韧性模式/02-hpa-advanced-patterns|HPA 高级模式]]
- [[03-清单模式/08-韧性模式/06-graceful-shutdown|优雅关闭]]

## See Also

- [PodDisruptionBudget 文档](https://kubernetes.io/docs/concepts/workloads/pods/disruptions/)
- [安全驱逐实践](https://kubernetes.io/docs/tasks/administer-cluster/safely-drain-node/)

<!-- risk-assessed -->
