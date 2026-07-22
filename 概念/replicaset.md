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

## 源码实现分析

### ReplicaSet Controller 对账逻辑

```go
// k8s.io/kubernetes/pkg/controller/replicaset/replica_set.go
func (rsc *ReplicaSetController) syncReplicaSet(ctx context.Context, rs *apps.ReplicaSet) error {
    // 1. 通过 selector 获取所有匹配的 Pod
    selector, _ := metav1.LabelSelectorAsSelector(rs.Spec.Selector)
    allPods := rsc.podLister.List(selector)
    // 2. 过滤属于本 RS 的 Pod（OwnerReference 匹配）
    ownedPods := filterOwnedPods(allPods, rs.UID)
    // 3. 计算当前副本数 vs 期望副本数
    activePods := filterActivePods(ownedPods) // 排除 Succeeded/Failed
    diff := int(*rs.Spec.Replicas) - len(activePods)
    // 4. 扩容：创建新 Pod
    if diff > 0 {
        for i := 0; i < diff; i++ {
            pod := rsc.createPod(rs) // 使用 RS 的 Pod Template
            // 设置 OwnerReference 指向 RS
            ctrl.SetControllerReference(rs, pod, rsc.scheme)
            rsc.kubeClient.CoreV1().Pods(ns).Create(ctx, pod)
        }
    }
    // 5. 缩容：删除多余 Pod（按创建时间排序，删最新的）
    if diff < 0 {
        podsToDelete := getPodsToDelete(activePods, -diff)
        for _, pod := range podsToDelete {
            rsc.kubeClient.CoreV1().Pods(ns).Delete(ctx, pod.Name)
        }
    }
    // 6. 更新 Status
    rs.Status.Replicas = int32(len(ownedPods))
    rs.Status.ReadyReplicas = countReady(ownedPods)
    rsc.kubeClient.AppsV1().ReplicaSets(ns).UpdateStatus(ctx, rs)
    return nil
}
```

### ReplicaSet 与 Deployment 关系

```
┌──────────────────────────────────────────────────────────┐
│          Deployment → ReplicaSet → Pod 层次关系       │
├──────────────────────────────────────────────────────────┤
│  Deployment (webapp)                                     │
│    │  管理多个 RS 版本，控制滚动更新/回滚            │
│    ├─ ReplicaSet (webapp-7b9c4d) [replicas=3] ← 当前  │
│    │     ├─ Pod (webapp-7b9c4d-abc12)                   │
│    │     ├─ Pod (webapp-7b9c4d-def34)                   │
│    │     └─ Pod (webapp-7b9c4d-ghi56)                   │
│    ├─ ReplicaSet (webapp-6a8b3c) [replicas=0] ← 旧版  │
│    └─ ReplicaSet (webapp-5z7x2v) [replicas=0] ← 更旧  │
│                                                          │
│  滚动更新时：新 RS 逐步扩容，旧 RS 逐步缩容          │
│  回滚时：旧 RS 重新扩容，新 RS 缩容到 0              │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：观察滚动更新中的 RS 变化

```bash
# 🟢 低风险：只读观察
# 触发滚动更新
kubectl set image deployment/webapp webapp=registry/webapp:v2.0.0 -n production
# 观察 RS 变化（新 RS 扩容，旧 RS 缩容）
kubectl get rs -n production -w
# 查看 RS 事件
kubectl describe rs webapp-7b9c4d -n production | grep -A20 Events
# 查看 Pod 归属
kubectl get pods -n production -o custom-columns=NAME:.metadata.name,RS:.metadata.ownerReferences[0].name,STATUS:.status.phase
```

### 场景二：清理历史 ReplicaSet

```bash
# 🟡 中风险：删除 RS 会级联删除其 Pod
# 查看 Deployment 的 revisionHistoryLimit
kubectl get deployment webapp -o jsonpath='{.spec.revisionHistoryLimit}'
# 手动清理旧 RS（副本数为 0 的）
kubectl get rs -n production -o json | \
  jq -r '.items[] | select(.spec.replicas==0) | .metadata.name' | \
  xargs -I{} kubectl delete rs {} -n production  # 🟡 删除操作
# 或者设置 revisionHistoryLimit 自动清理
kubectl patch deployment webapp -p '{"spec":{"revisionHistoryLimit":3}}'
```

### 场景三：排查 RS 副本数不收敛

```bash
# 🟢 低风险：只读诊断
# 检查 RS 状态
kubectl get rs webapp-7b9c4d -o yaml | grep -A10 status
# 检查是否有 Pod 创建失败
kubectl get events -n production --field-selector reason=FailedCreate
# 检查资源配额是否超限
kubectl get resourcequota -n production -o yaml
# 检查节点资源是否充足
kubectl describe nodes | grep -A5 "Allocated resources"
# 检查 Pod 是否 Pending
kubectl get pods -n production --field-selector=status.phase=Pending
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | 可以直接管理 ReplicaSet | 应通过 Deployment 管理；直接操作 RS 会被 Deployment 控制器覆盖 |
| 2 | 修改 RS 的 Pod Template 会更新现有 Pod | RS 不会重建已存在的 Pod；必须通过 Deployment 滚动更新或手动删 Pod |
| 3 | selector 可以随意修改 | RS 创建后 selector 不可变（API 校验）；修改需删除重建 |
| 4 | 多个 RS 可以有相同 selector | selector 重叠会导致 Pod 被多个 RS 认领，副本数不可预测 |
| 5 | 删除 RS 不会影响 Pod | 默认级联删除 Pod；用 --cascade=orphan 保留 Pod（用于迁移） |
| 6 | HPA 启用后可以手动 scale | HPA 会立即覆盖手动 scale 操作；应通过调整 HPA 参数控制副本数 |

## 面试要点

1. **Q: ReplicaSet 的对账逻辑是怎样的？**
   A: ① 通过 label selector 获取所有匹配 Pod；② 通过 OwnerReference 过滤属于本 RS 的 Pod；③ 计算 activePods（排除 Succeeded/Failed）与期望副本数的差值；④ 差值>0 则创建新 Pod（设置 OwnerReference）；⑤ 差值<0 则删除多余 Pod（按创建时间排序，优先删最新）；⑥ 更新 Status（replicas/readyReplicas/availableReplicas）。整个过程是 level-triggered，幂等可重复。

2. **Q: Deployment 和 ReplicaSet 的关系是什么？为什么不直接用 RS？**
   A: Deployment 是 RS 的上层抽象：① 版本管理：每次 Pod Template 变更创建新 RS，旧 RS 保留（revisionHistoryLimit）；② 滚动更新：控制新 RS 扩容和旧 RS 缩容的节奏（maxSurge/maxUnavailable）；③ 回滚：将旧 RS 重新扩容到期望副本数；④ 暂停/恢复：支持金丝雀发布的暂停观察。直接用 RS 会失去版本历史、回滚、滚动策略等能力。

3. **Q: ReplicaSet 如何确保 Pod 副本数正确？**
   A: Level-triggered 机制：① Informer Watch Pod 变化（创建/删除/标签变更）；② 任何变化触发 syncReplicaSet 重新对账；③ 通过 OwnerReference 确认 Pod 归属（避免误删其他控制器的 Pod）；④ 缩容时按创建时间排序删除最新 Pod（保留稳定运行的旧 Pod）；⑤ 考虑 PodDisruptionBudget 避免驱逐过多 Pod；⑥ 考虑节点故障时的 Pod 重建（node lifecycle controller 配合）。

4. **Q: 什么情况下 ReplicaSet 副本数会不收敛？**
   A: ① 资源不足：节点 CPU/内存不足导致新 Pod Pending；② 配额超限：ResourceQuota 限制 Pod 数量或资源总量；③ 镜像拉取失败：ImagePullBackOff 导致 Pod 无法 Running；④ 调度约束：nodeSelector/affinity/taint 无匹配节点；⑤ PDB 保护：缩容时 PodDisruptionBudget 阻止删除；⑥ 准入拒绝：Webhook 拒绝 Pod 创建请求。排查：kubectl get events + describe rs + describe pods。

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/pods.md|Pod]]
- [[概念/deployments.md|Deployment]] — ReplicaSet 的上层管理器
- [[概念/statefulset.md|StatefulSet]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
