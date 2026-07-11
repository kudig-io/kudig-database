---
title: ReplicaSet
summary: ReplicaSet 是 Kubernetes 中用于维护一组稳定 Pod 副本的控制器。
category: concepts
tags:
- core-concept
- k8s
- workloads
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# ReplicaSet

## 概述

ReplicaSet（RS）是 Kubernetes 中用于维持**指定数量的稳定 Pod 副本**的控制器。它通过 label selector 匹配 Pod，副本不足则按 Pod 模板创建新 Pod，副本过多则删除多余的。ReplicaSet 是 Deployment 的底层依赖：Deployment 每次更新会创建一个新的 ReplicaSet 并缩容旧的，从而实现滚动更新与版本历史。在生产实践中，**几乎不应直接使用 ReplicaSet**，而是通过 Deployment 间接管理。

## 架构与工作原理

```
        ┌────────── ReplicaSet (rs-a) ──────────┐
        │  replicas: 3                           │
        │  selector: matchLabels {app: web}      │
        │  template: Pod (image: web:v1)          │
        └───────────────┬────────────────────────┘
                        │ 控制循环：desired vs current
        ┌───────────────┴───────────────┐
        ▼                               ▼
   Pod-1 app=web (匹配)            Pod-X app=cache (不匹配，忽略)
   Pod-2 app=web
   Pod-3 app=web
   期望 3 / 当前 3 → 收敛完成
```

**工作流**：
1. RS Controller 监听 ReplicaSet 资源，对比 `.spec.replicas` 与通过 selector 实际匹配到的 Pod 数。
2. 副本数不足 → 用 `.spec.template` 创建新 Pod；副本数过多 → 按 `deletePriority` 删除（未调度/未就绪优先）。
3. **selector 是核心**：RS 只关心 label 匹配，Pod 模板改动**不会**触发已有 Pod 更新（这是 ReplicaSet 与 Deployment 的关键区别）。
4. Deployment 在每次 Pod 模板变更时，生成新 hash 的 RS，把旧 RS 缩到 0、新 RS 扩到目标，从而实现版本切换与回滚。

## 关键组件与特性

| 字段 | 作用 |
|------|------|
| `spec.replicas` | 期望副本数（默认 1） |
| `spec.selector` | label 选择器，必须匹配 template.labels，且创建后不可改 |
| `spec.template` | Pod 模板，仅用于新建 Pod |
| `spec.minReadySeconds` | Pod 就绪后多久算可用 |
| OwnerReferences | RS 创建的 Pod 自动带 ownerRef，RS 删除会级联删除 Pod |

## 配置示例

```yaml
---
# 直接使用 ReplicaSet（不推荐，演示用）
apiVersion: apps/v1
kind: ReplicaSet
metadata:
  name: webapp-rs
  namespace: production
spec:
  replicas: 4
  minReadySeconds: 10
  selector:
    matchLabels:
      app: webapp
  template:
    metadata:
      labels:
        app: webapp
        version: v1
    spec:
      containers:
      - name: webapp
        image: registry.example.com/webapp:v1.2.0
        ports: [{containerPort: 8080}]
        resources:
          requests: {cpu: 250m, memory: 256Mi}
        readinessProbe:
          httpGet: {path: /ready, port: 8080}
---
# 推荐：用 Deployment，它内部管理 ReplicaSet
apiVersion: apps/v1
kind: Deployment
metadata: {name: webapp, namespace: production}
spec:
  replicas: 4
  selector: {matchLabels: {app: webapp}}
  template:
    metadata: {labels: {app: webapp}}
    spec:
      containers:
      - name: webapp
        image: registry.example.com/webapp:v2.0.0
```

## 常用操作与命令

```bash
# 查看 Deployment 底层 ReplicaSet
kubectl get rs -n production
kubectl describe rs webapp-7b9c4d -n production

# 查看某 RS 的 Pod（owner 关系）
kubectl get pods -n production -o wide \
  --field-selector=metadata.ownerReferences[0].kind=ReplicaSet

# 手动伸缩（Deployment 会同步到底层 RS）
kubectl scale deployment webapp --replicas=6

# 临时调整 RS 副本（不推荐，会被 Deployment 控制器覆盖）
kubectl scale rs webapp-7b9c4d --replicas=2

# 清理孤立的 ReplicaSet（Deployment 残留）
kubectl delete rs <rs-name> -n production

# 查看某 Deployment 的所有 RS 版本
kubectl get rs -l app=webapp -n production -o custom-columns=NAME:.metadata.name,REPLICAS:.spec.replicas,IMAGE:.spec.template.spec.containers[0].image
```

## 最佳实践

1. **用 Deployment 而非 ReplicaSet**：Deployment 提供版本历史、回滚、滚动更新，直接管 RS 会失去这些能力。
2. **selector 与 template 严格一致**：不一致 RS 创建时即报错（API 校验），避免手动绕过。
3. **selector 不要随意扩展**：把新标签加进 selector 可能"吞掉"其他控制器管理的 Pod。
4. **副本数由 HPA 管理**：启用 HPA 后不在 Deployment 模板写死 replicas，让 HPA 自主伸缩。
5. **不要混用直接创建的 Pod**：裸 Pod 可能被 RS 意外认领/删除，统一用工作负载管理。
6. **PodDisruptionBudget 保护**：为 RS/Deployment 对应的 Pod 设 PDB，避免维护驱逐导致不可用。

## 常见陷阱

- **Pod 模板改动不生效**：ReplicaSet 不会重建已存在的 Pod，必须靠 Deployment 滚动或手动删 Pod 才会更新。
- **selector 漂移**：修改 selector 会与现有 Pod 失配，导致 RS 创建一堆"孤儿"Pod。
- **多个 RS selector 重叠**：同一 Pod 被多个 RS 认领会引发竞争，副本数不可预测。
- **手动 scale 被 HPA 覆盖**：HPA 启用时手动 `kubectl scale rs` 会被立刻纠正。
- **级联删除误伤**：删 RS 默认级联删除其 Pod；用 `--cascade=orphan` 保留 Pod（用于迁移）。
- **RS 残留占用 Endpoints**：旧 RS 副本虽为 0，但其 selector 仍可能匹配手工误打的 Pod。

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/pods.md|Pod]]
- [[概念/deployments.md|Deployment]] — ReplicaSet 的上层管理器
- [[概念/statefulset.md|StatefulSet]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
