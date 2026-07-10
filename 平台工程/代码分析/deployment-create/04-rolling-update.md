---
title: 滚动更新源码分析 (topic-code-analysis)
description: '## 概述'
summary: '扩容、reconcileOldReplicaSets 缩容、比例缩放算法以及暂停恢复机制。'
category: general
tags:
- reference
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 滚动更新源码分析 是什么
- 如何 滚动更新源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 滚动更新源码分析
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 滚动更新源码分析
category: deployment
tags:
- rolling-update
- maxSurge
- maxUnavailable
- rolloutRolling
- proportion
- deployment
last_updated: 2026-05-18
description: 深入分析 Kubernetes Deployment RollingUpdate 策略的源码实现，涵盖 rolloutRolling 入口、reconcileNewReplicaSet
  扩容、reconcileOldReplicaSets 缩容、比例缩放算法以及暂停恢复机制。
difficulty: advanced
intent_queries:
- kubernetes rolling update source code
- maxSurge maxUnavailable calculation kubernetes
- rolloutRolling reconcileNewReplicaSet kubernetes
- deployment proportion scaling algorithm
- kubectl rollout pause resume kubernetes
trigger_keywords:
- RollingUpdate
- maxSurge
- maxUnavailable
- rolloutRolling
- reconcileNewReplicaSet
- reconcileOldReplicaSets
- GetProportion
- progressDeadlineSeconds
- NewRSNewReplicas
- kubectl rollout pause
reading_level: advanced
audience:
- platform-engineer
- kubernetes-developer
- sre
estimated_read_time: 5min
related_domains:
- 工作负载
- 集群基础
related_topics:
- deployment-controller
- replicaset-controller
- deployment-status
- revision-history
domain_link: '[Workloads](../工作负载/README.md)'
topic_link: '[Deployment Create](./README.md)'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 滚动更新源码分析

## 概述

RollingUpdate 是 Deployment 最常用的更新策略。它通过**逐步替换** Pod 来实现零停机更新，核心参数 `maxSurge` 和 `maxUnavailable` 控制替换的速度和可用性保障。本文档基于 `pkg/controller/deployment/rolling.go` 源码，分析滚动更新的完整算法。

---

## 源码路径

- **滚动更新逻辑**: `pkg/controller/deployment/rolling.go`
- **同步入口**: `pkg/controller/deployment/sync.go`
- **比例缩放工具**: `pkg/controller/deployment/proportion.go`

---

## 滚动更新策略配置

```yaml
apiVersion: apps/v1
kind: Deployment
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%        # 更新期间可超出期望副本数的比例/数量
      maxUnavailable: 25%  # 更新期间允许不可用的最大比例/数量
```

**参数解析**：

| 参数 | 类型 | 默认值 | 说明 |
|-----|------|-------|------|
| `maxSurge` | int / string | `25%` | 更新期间允许创建的**额外** Pod 数量。可以是绝对数（如 1）或百分比（如 25%） |
| `maxUnavailable` | int / string | `25%` | 更新期间允许**不可用**的 Pod 数量。可以是绝对数或百分比 |

**约束**：
- `maxSurge` 和 `maxUnavailable` 不能同时为 0
- 如果 `replicas = 1`，建议 `maxUnavailable = 0` 且 `maxSurge = 1`，确保始终有 Pod 可用

---

## rolloutRolling — 滚动更新入口

```go
// pkg/controller/deployment/rolling.go
func (dc *DeploymentController) rolloutRolling(ctx context.Context, d *apps.Deployment, rsList []*apps.ReplicaSet, podList []*v1.Pod) error {
    // 1. 获取当前活跃的 ReplicaSet（最新的）
    newRS, oldRSs, err := dc.getAllReplicaSetsAndSyncRevision(d, rsList, podList, true)
    
    // 2. 检查是否需要缩放所有 ReplicaSet（包括新旧）
    allRSs := append(oldRSs, newRS)
    scaled, err := dc.reconcileNewReplicaSet(ctx, allRSs, newRS, d)
    if scaled {
        // 已经执行了缩放操作，本轮同步完成
        return dc.syncRolloutStatus(ctx, allRSs, newRS, d)
    }
    
    // 3. 缩放旧的 ReplicaSet（逐步缩容）
    scaled, err = dc.reconcileOldReplicaSets(ctx, oldRSs, allRSs, d)
    if scaled {
        return dc.syncRolloutStatus(ctx, allRSs, newRS, d)
    }
    
    // 4. 清理完成历史版本的老 ReplicaSet
    if deploymentutil.DeploymentComplete(d, &d.Status) {
        dc.cleanupDeployment(oldRSs, d)
    }
    
    // 5. 同步 Deployment 状态
    return dc.syncRolloutStatus(ctx, allRSs, newRS, d)
}
```

**核心流程**：
1. 识别新旧 ReplicaSet
2. 先扩容新 ReplicaSet（确保有 Pod 可用）
3. 再缩容旧 ReplicaSet（释放资源）
4. 更新 Deployment 状态

---

## reconcileNewReplicaSet — 扩容新 RS

```go
// pkg/controller/deployment/rolling.go
func (dc *DeploymentController) reconcileNewReplicaSet(ctx context.Context, allRSs []*apps.ReplicaSet, newRS *apps.ReplicaSet, deployment *apps.Deployment) (bool, error) {
    // 1. 如果新 RS 已经满足期望副本数，无需操作
    if *(newRS.Spec.Replicas) == *(deployment.Spec.Replicas) {
        return false, nil
    }
    
    // 2. 计算新 RS 的目标副本数
    // 公式：newRSReplicas = deployment.Spec.Replicas + maxSurge - (allOldPods)
    // 简化为：newRSReplicas = min(deployment.Spec.Replicas + maxSurge, totalPods - minAvailable)
    
    newReplicasCount, err := deploymentutil.NewRSNewReplicas(deployment, allRSs, newRS)
    
    // 3. 更新新 RS 的副本数
    if *(newRS.Spec.Replicas) != newReplicasCount {
        newRS.Spec.Replicas = &newReplicasCount
        _, err := dc.client.AppsV1().ReplicaSets(newRS.Namespace).Update(ctx, newRS, metav1.UpdateOptions{})
        return true, err  // 返回 true 表示已执行缩放
    }
    
    return false, nil
}
```

### newReplicasCount 的计算逻辑

```go
// pkg/controller/deployment/util/deployment_util.go
func NewRSNewReplicas(deployment *apps.Deployment, allRSs []*apps.ReplicaSet, newRS *apps.ReplicaSet) (int32, error) {
    // 1. 计算 maxSurge 的绝对值
    maxSurge, err := intstrutil.GetValueFromIntOrPercent(
        intstrutil.ValueOrDefault(deployment.Spec.Strategy.RollingUpdate.MaxSurge, intstrutil.FromString("25%")),
        int(*(deployment.Spec.Replicas)),
        true, // isMaxSurge: 向上取整
    )
    
    // 2. 计算当前所有旧 RS 的 Pod 总数
    oldPodsCount := GetActualReplicaCountForReplicaSets(allRSs) - GetActualReplicaCountForReplicaSets([]*apps.ReplicaSet{newRS})
    
    // 3. 计算新 RS 的目标副本数
    // 目标：尽可能快地让新 RS 达到期望副本数，但不超过 maxSurge 限制
    newReplicasCount := *(deployment.Spec.Replicas)
    
    // 如果旧 RS 还有 Pod，新 RS 可能需要先创建更多副本
    // 具体计算考虑了比例缩放
    
    return newReplicasCount, nil
}
```

---

## reconcileOldReplicaSets — 缩容旧 RS

```go
// pkg/controller/deployment/rolling.go
func (dc *DeploymentController) reconcileOldReplicaSets(ctx context.Context, oldRSs []*apps.ReplicaSet, allRSs []*apps.ReplicaSet, deployment *apps.Deployment) (bool, error) {
    // 1. 计算允许的最大不可用 Pod 数
    maxUnavailable, err := deploymentutil.MaxUnavailable(deployment)
    
    // 2. 计算当前不可用 Pod 数
    unavailablePods := deploymentutil.GetUnavailablePodsCount(allRSs, deployment)
    
    // 3. 计算可以缩容的旧 RS 数量
    // 原则：确保 unavailablePods <= maxUnavailable
    // 且 totalPods <= deployment.Replicas + maxSurge
    
    // 4. 按比例缩容旧 ReplicaSet
    // 如果有多个旧 RS（罕见），按比例分配缩容数量
    cleanupCount := len(oldRSs) - deploymentutil.MaxRevisionHistoryLimit(deployment)
    if cleanupCount > 0 {
        // 清理超出 revisionHistoryLimit 的老版本
        SortReplicaSetsByRevision(oldRSs)
        for i := 0; i < cleanupCount; i++ {
            dc.client.AppsV1().ReplicaSets(oldRSs[i].Namespace).Delete(ctx, oldRSs[i].Name, metav1.DeleteOptions{})
        }
    }
    
    // 5. 缩容仍然活跃的旧 RS
    scaledDown := false
    for _, rs := range oldRSs {
        if rs == nil {
            continue
        }
        
        // 计算该 RS 应缩容到的目标副本数
        targetScale := int32(0)  // 最终目标是 0
        
        // 比例缩放：如果新 RS 还没完全就绪，旧 RS 保留部分副本
        desiredReplicas := deploymentutil.GetProportion2(rs, deployment, *(deployment.Spec.Replicas), maxSurge)
        
        if desiredReplicas < *(rs.Spec.Replicas) {
            rs.Spec.Replicas = &desiredReplicas
            _, err := dc.client.AppsV1().ReplicaSets(rs.Namespace).Update(ctx, rs, metav1.UpdateOptions{})
            scaledDown = true
        }
    }
    
    return scaledDown, nil
}
```

---

## 比例缩放算法

当存在多个旧 ReplicaSet 时（如快速连续更新），Deployment 控制器使用**比例缩放**确保各版本按正确比例缩容。

```go
// pkg/controller/deployment/proportion.go
func GetProportion(rs *apps.ReplicaSet, d *apps.Deployment) int32 {
    // 1. 获取该 RS 创建时的 Deployment Replicas
    // 从 annotation "deployment.kubernetes.io/revision" 和 RS 创建时的状态推算
    
    // 2. 计算比例
    // proportion = rs.Spec.Replicas * (d.Spec.Replicas / d.Status.Replicas)
    // 或基于 Deployment 更新时的期望状态
    
    // 简化逻辑：按比例分配当前期望副本数到各 RS
    if *(d.Spec.Replicas) == 0 {
        return 0
    }
    
    // 基于 RS 在创建时占 Deployment 副本的比例
    rsFraction := getReplicaSetFraction(rs, d)
    
    return integer.RoundToInt32(rsFraction * float64(*(d.Spec.Replicas)))
}
```

**示例**：
```
初始状态:
  Deployment: replicas=10
  RS-v1: replicas=10

第一次更新（镜像 v1→v2）:
  新 RS-v2 创建，replicas=0
  逐步：RS-v2=3, RS-v1=7
        RS-v2=6, RS-v1=4
        RS-v2=10, RS-v1=0

快速第二次更新（镜像 v2→v3）:
  RS-v2 还未完全缩容到 0 时，RS-v3 创建
  比例缩放确保：
    RS-v3 逐步增加
    RS-v2 和 RS-v1 按各自比例缩容
```

### maxUnavailable=0 的滚动更新

```yaml
# spec:
#   strategy:
#     type: RollingUpdate
#     rollingUpdate:
#       maxSurge: 1
#       maxUnavailable: 0  # 关键：不允许不可用
```

**行为**：
- 始终保持所有 Pod 可用
- 新 Pod 必须 Ready 后才缩容旧 Pod
- 适合对可用性要求高的服务

**时序**：
```
Step 1: RS-v2 replicas=1, RS-v1 replicas=10 (总 11 Pod)
Step 2: RS-v2 replicas=2, RS-v1 replicas=9  (总 11 Pod)
...
Step 10: RS-v2 replicas=10, RS-v1 replicas=1 (总 11 Pod)
Step 11: RS-v2 replicas=10, RS-v1 replicas=0 (总 10 Pod，滚动完成)
```

---

## maxSurge / maxUnavailable 的数学约束

```
设: desired = Deployment.Spec.Replicas
    maxSurge = N
    maxUnavailable = M

约束条件:
    totalPods <= desired + N          (maxSurge 上限)
    unavailablePods <= M              (maxUnavailable 上限)
    availablePods >= desired - M      (最少可用)

更新过程:
    Step 0: 旧 RS = desired, 新 RS = 0
           total = desired, available = desired
    
    Step 1: 新 RS +1, 旧 RS 不变
           total = desired + 1
           检查: desired + 1 <= desired + N ? 即 N >= 1
    
    Step 2: 新 RS +1, 旧 RS -1
           total = desired
           检查: available >= desired - M ? 需考虑新 Pod 是否 Ready
    
    ... 持续直到旧 RS = 0, 新 RS = desired
```

---

## 暂停与恢复滚动更新

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 暂停滚动更新
kubectl rollout pause deployment/nginx

# 查看状态：Paused = True
kubectl get deployment nginx -o jsonpath='{.spec.paused}'

# 恢复滚动更新
kubectl rollout resume deployment/nginx
```
**源码中的暂停处理**：
```go
// pkg/controller/deployment/sync.go
if d.Spec.Paused {
    // 暂停时不执行 rolloutRolling 中的实际替换逻辑
    // 但会同步 ReplicaSet 的副本数到 Deployment.Spec.Replicas
    return dc.sync(ctx, d, podList, rsList, rsList)
}
```

**暂停期间的行为**：
- 不会创建新的 ReplicaSet
- 不会执行新旧 RS 之间的替换
- 但仍会响应 `kubectl scale` 命令（调整所有 RS 的副本数）

---

## 滚动更新的限速保护

```go
// pkg/controller/deployment/progress.go
func (dc *DeploymentController) syncRolloutStatus(ctx context.Context, allRSs []*apps.ReplicaSet, newRS *apps.ReplicaSet, d *apps.Deployment) error {
    // 1. 计算 newRS 中已就绪的 Pod 数
    newRSAvailableReplicas := deploymentutil.GetAvailableReplicasForReplicaSets([]*apps.ReplicaSet{newRS})
    
    // 2. 检查进度
    // 如果 newRS 的可用副本数在增加，说明更新在推进
    // 如果长时间无变化，标记 Progressing=False
    
    // 3. 更新 Deployment Status
    newStatus := calculateStatus(allRSs, newRS, d)
    
    return dc.updateDeploymentStatus(ctx, d, newStatus)
}
```

**Progress Deadline**：
- 默认 `progressDeadlineSeconds = 600`（10 分钟）
- 如果滚动更新在 10 分钟内没有推进（新 Pod 没有变得 Available），Deployment Status 中 `Progressing=False`
- 触发 `ReplicaSetCreateReplicaTimeout` 事件

## Related

- [[reference|#reference Hub]] — tag hub

- [[README|README]]
- [[系统基础/topic-cheat-sheet/go.md|go]]
- [[系统基础/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[系统基础/topic-dictionary/workloads/replicaset.md|replicaset]]


<!-- risk-assessed -->
