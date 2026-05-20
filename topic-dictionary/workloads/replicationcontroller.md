---
title: ReplicationController
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- daemonset
- job
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ReplicationController 是什么
- 如何 ReplicationController
trigger_keywords:
- ReplicationController
- dictionary
title_en: Replicationcontroller
---


# ReplicationController

## 概述
ReplicationController 是一种遗留 API，用于确保指定数量的 Pod 副本始终处于运行状态。它已被 Deployment 和 ReplicaSet 取代，仅在维护旧系统或学习 Kubernetes 历史时可能遇到。

## 核心概念/原理
- **副本保障**：ReplicationController 持续监控与选择器匹配的 Pod 数量，过多则删除，过少则创建。
- **Pod 模板（`spec.template`）**：必填字段，`restartPolicy` 只能为 `Always`。
- **标签选择器（`spec.selector`）**：基于等式（equality-based）的选择器，管理所有匹配的 Pod，不论其创建者是谁。
- **Pod 替换**：当 Pod 因节点故障、维护或被删除而终止时，ReplicationController 会自动创建新的替代 Pod。

## 关键机制或特性
- **扩缩容**：通过修改 `spec.replicas` 即可手动扩缩容；也支持外部 autoscaler 修改。
- **滚动更新**：ReplicationController 本身不直接支持声明式滚动更新。推荐做法是先创建新的 ReplicationController，然后逐个缩放新旧控制器，最后删除旧控制器。
- **删除策略**：
  - `kubectl delete rc` 默认级联删除所有 Pod。
  - `--cascade=orphan` 可仅删除 ReplicationController 而保留 Pod，新的同选择器控制器可收养这些 Pod。
- **与 Service 配合**：多个 ReplicationController 可共享同一个 Service，实现金丝雀发布或多版本共存。

## 使用场景
- 维护历史遗留系统。
- 作为学习 Kubernetes 控制器原理的参考。
- **新系统应完全避免直接使用 ReplicationController**。

## 最佳实践/注意事项
- **强烈建议使用 Deployment 替代 ReplicationController**，因为 Deployment 提供声明式滚动更新、回滚和更丰富的生命周期管理。
- 若需要基于集合（set-based）的选择器，应使用 ReplicaSet。
- 避免创建标签与现有 ReplicationController 选择器重叠的裸 Pod，否则会被意外收养。
- 对于预期自行终止的任务，使用 Job；对于节点级服务，使用 DaemonSet。

## 生产 YAML 示例

### ReplicationController 基本定义（仅供参考，请使用 Deployment 替代）

```yaml
# ⚠️ 已废弃 — 仅用于维护遗留系统或学习目的
apiVersion: v1
kind: ReplicationController
metadata:
  name: legacy-web-app
  namespace: legacy
spec:
  replicas: 3
  selector:
    app: legacy-web                # 等式选择器（不支持集合选择器）
  template:
    metadata:
      labels:
        app: legacy-web
        version: v1.0
    spec:
      containers:
      - name: web
        image: registry.example.com/legacy/web-app:v1.0
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: "200m"
            memory: "256Mi"
          limits:
            cpu: "500m"
            memory: "512Mi"
        readinessProbe:
          httpGet:
            path: /healthz
            port: 8080
          periodSeconds: 10
```

### 迁移到 Deployment 的等效配置

```yaml
# ✅ 推荐：使用 Deployment 替代 ReplicationController
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-app                 # 支持集合选择器 matchExpressions
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0            # 零停机更新
  template:
    metadata:
      labels:
        app: web-app
        version: v2.0
    spec:
      containers:
      - name: web
        image: registry.example.com/apps/web-app:v2.0
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: "200m"
            memory: "256Mi"
          limits:
            cpu: "500m"
            memory: "512Mi"
```

## ReplicationController vs Deployment 对比

| 特性 | ReplicationController | Deployment + ReplicaSet |
|------|----------------------|------------------------|
| API 版本 | `v1`（核心 API） | `apps/v1` |
| 选择器类型 | 仅等式（equality-based） | 等式 + 集合（set-based） |
| 滚动更新 | 手动创建新 RC 逐步切换 | 声明式 `strategy.rollingUpdate` |
| 回滚 | 不支持（需手动管理） | `kubectl rollout undo` |
| 暂停/恢复 | 不支持 | `kubectl rollout pause/resume` |
| 历史修订版本 | 不记录 | 通过 ReplicaSet 保留历史 |
| 状态 | 已废弃 | 推荐使用 |

## 迁移指南

```bash
# 步骤 1：导出现有 RC 配置
kubectl get rc legacy-web-app -n legacy -o yaml > legacy-rc.yaml

# 步骤 2：转换为 Deployment（手动修改）
# - apiVersion: v1 → apps/v1
# - kind: ReplicationController → Deployment
# - spec.selector → spec.selector.matchLabels
# - 添加 spec.strategy

# 步骤 3：删除 RC 但保留 Pod（孤儿模式）
kubectl delete rc legacy-web-app -n legacy --cascade=orphan

# 步骤 4：应用新 Deployment（会收养已有 Pod）
kubectl apply -f deployment.yaml

# 步骤 5：验证
kubectl get pods -n legacy -l app=legacy-web -o wide
kubectl rollout status deployment/web-app -n legacy
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 被意外收养或删除 | 裸 Pod 的标签与 RC 选择器匹配 | `kubectl get rc -o wide` 检查选择器；避免创建标签重叠的裸 Pod |
| 副本数不断波动 | 多个 RC 使用重叠的选择器 | `kubectl get rc -l app=xxx` 确认无重叠 |
| 删除 RC 后 Pod 未清理 | 使用了 `--cascade=orphan` | `kubectl get pods -l app=xxx` 手动清理孤儿 Pod |
| 滚动更新过程中服务中断 | RC 不支持原生滚动更新，新旧版本切换有间隔 | 迁移到 Deployment 使用声明式滚动更新 |

## 生产检查清单

- [ ] 评估是否可以迁移到 Deployment（强烈推荐）
- [ ] RC 的 `selector` 不与其他控制器或裸 Pod 的标签冲突
- [ ] Pod 模板设置了 `readinessProbe`
- [ ] 资源 requests/limits 已配置
- [ ] 若需删除 RC 并保留 Pod，使用 `--cascade=orphan`
- [ ] 监控 RC 管理的 Pod 数量是否稳定

## 命令快速参考

```bash
# 查看 ReplicationController 列表
kubectl get rc -n <namespace>

# 扩缩容
kubectl scale rc legacy-web-app --replicas=5 -n legacy

# 查看 RC 管理的 Pod
kubectl get pods -l app=legacy-web -n legacy

# 删除 RC 但保留 Pod
kubectl delete rc legacy-web-app --cascade=orphan -n legacy

# 删除 RC 及其所有 Pod
kubectl delete rc legacy-web-app -n legacy
```

## 交叉引用

- [Deployments](deployments.md) — 推荐的替代方案，支持声明式滚动更新
- [ReplicaSet](replicaset.md) — Deployment 的底层副本管理器
- [工作负载管理](managing-workloads.md) — 资源的批量管理和更新策略
- [工作负载选型指南](workload-management.md) — 各工作负载类型的选型决策

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/replicationcontroller/
