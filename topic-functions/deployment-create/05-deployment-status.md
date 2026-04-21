# Deployment Status 计算逻辑

## 概述

Deployment 的 `Status` 字段反映了控制器当前执行的真实状态，包括副本数、可用性、进度和条件。控制器每次同步完成后都会更新 Status，这是用户和外部系统（如 HPA、ArgoCD）判断 Deployment 健康度的核心依据。

---

## 源码路径

- **Status 计算**: `pkg/controller/deployment/progress.go`
- **Status 同步**: `pkg/controller/deployment/sync.go`
- **工具函数**: `pkg/controller/deployment/util/deployment_util.go`

---

## Status 结构

```yaml
apiVersion: apps/v1
kind: Deployment
status:
  observedGeneration: 3        # 已处理的 Generation
  replicas: 5                  # 总 Pod 数
  updatedReplicas: 5           # 使用最新 PodTemplate 的 Pod 数
  readyReplicas: 5             # Ready 状态的 Pod 数
  availableReplicas: 5         # Available 状态的 Pod 数
  unavailableReplicas: 0       # 不可用的 Pod 数
  conditions:
  - type: Progressing
    status: "True"
    reason: NewReplicaSetAvailable
    message: ReplicaSet "nginx-7c4c8d5d4f" has successfully progressed.
  - type: Available
    status: "True"
    reason: MinimumReplicasAvailable
```

---

## Status 字段计算源码

```go
// pkg/controller/deployment/progress.go
func (dc *DeploymentController) syncRolloutStatus(ctx context.Context, allRSs []*apps.ReplicaSet, newRS *apps.ReplicaSet, deployment *apps.Deployment) error {
    // 1. 计算各类副本数
    totalActualReplicas := deploymentutil.GetActualReplicaCountForReplicaSets(allRSs)
    updatedReplicas := deploymentutil.GetActualReplicaCountForReplicaSets([]*apps.ReplicaSet{newRS})
    readyReplicas := deploymentutil.GetReadyReplicaCountForReplicaSets(allRSs)
    availableReplicas := deploymentutil.GetAvailableReplicaCountForReplicaSets(allRSs, deployment.Spec.MinReadySeconds)
    
    // 2. 计算不可用副本数
    unavailableReplicas := totalActualReplicas - availableReplicas
    if unavailableReplicas < 0 {
        unavailableReplicas = 0
    }
    
    // 3. 构建新 Status
    newStatus := apps.DeploymentStatus{
        ObservedGeneration:     deployment.Generation,
        Replicas:               totalActualReplicas,
        UpdatedReplicas:        updatedReplicas,
        ReadyReplicas:          readyReplicas,
        AvailableReplicas:      availableReplicas,
        UnavailableReplicas:    unavailableReplicas,
    }
    
    // 4. 计算 Conditions
    newStatus.Conditions = dc.updateConditions(deployment, newStatus, allRSs, newRS)
    
    // 5. 更新 Deployment Status
    return dc.updateDeploymentStatus(ctx, deployment, &newStatus)
}
```

---

## 各字段计算详解

### 1. Replicas — 总 Pod 数

```go
// 所有 ReplicaSet 的实际副本数之和
totalActualReplicas := deploymentutil.GetActualReplicaCountForReplicaSets(allRSs)
```

**注意**：这是所有 RS（新旧）的 Pod 总数，可能大于 `Spec.Replicas`（滚动更新期间受 maxSurge 影响）。

### 2. UpdatedReplicas — 已更新副本数

```go
// 新 ReplicaSet 的实际副本数
updatedReplicas := deploymentutil.GetActualReplicaCountForReplicaSets([]*apps.ReplicaSet{newRS})
```

**意义**：
- `UpdatedReplicas == Spec.Replicas` 表示所有 Pod 都已更新到最新版本
- 滚动更新完成的核心指标

### 3. ReadyReplicas — Ready 状态副本数

```go
// 所有 ReplicaSet 中处于 Ready 状态的 Pod 数
readyReplicas := deploymentutil.GetReadyReplicaCountForReplicaSets(allRSs)
```

**Ready 判断条件**：
- Pod 的所有容器都已启动并通过 readinessProbe
- 或没有配置 readinessProbe 时容器已运行

### 4. AvailableReplicas — 可用副本数

```go
// Ready 状态持续超过 MinReadySeconds 的 Pod 数
availableReplicas := deploymentutil.GetAvailableReplicaCountForReplicaSets(allRSs, deployment.Spec.MinReadySeconds)
```

**关键区别**：

| 字段 | 条件 | 用途 |
|-----|------|------|
| `ReadyReplicas` | Pod Ready | 反映瞬时状态 |
| `AvailableReplicas` | Pod Ready **且** 持续 `minReadySeconds` | 反映稳定状态，用于可用性保证 |

**`minReadySeconds` 的作用**：
```yaml
spec:
  minReadySeconds: 30   # Pod Ready 持续 30 秒后，才被视为 Available
```

- 防止 Pod 刚 Ready 后立即崩溃导致的流量损失
- 给 Pod 一个"稳定期"
- 默认值：`0`（立即视为 Available）

### 5. UnavailableReplicas — 不可用副本数

```go
unavailableReplicas := totalActualReplicas - availableReplicas
```

**什么时候大于 0**：
- 滚动更新期间，新 Pod 还未 Ready
- Pod 因健康检查失败变成 NotReady
- 节点故障导致 Pod 不可达

---

## Conditions 条件计算

Deployment Status 包含两个核心 Condition：

### 1. Available 条件

```go
func (dc *DeploymentController) updateAvailableCondition(status *apps.DeploymentStatus, d *apps.Deployment) {
    // 判断标准：AvailableReplicas >= Deployment.Spec.Replicas - MaxUnavailable
    minAvailable := *(d.Spec.Replicas) - maxUnavailable
    
    if status.AvailableReplicas >= minAvailable {
        // 设置 Available=True
        SetDeploymentCondition(status, apps.DeploymentAvailable, v1.ConditionTrue,
            "MinimumReplicasAvailable",
            fmt.Sprintf("Deployment has minimum availability."))
    } else {
        // 设置 Available=False
        SetDeploymentCondition(status, apps.DeploymentAvailable, v1.ConditionFalse,
            "MinimumReplicasUnavailable",
            fmt.Sprintf("Deployment does not have minimum availability."))
    }
}
```

### 2. Progressing 条件

```go
func (dc *DeploymentController) updateProgressingCondition(status *apps.DeploymentStatus, d *apps.Deployment, allRSs []*apps.ReplicaSet, newRS *apps.ReplicaSet) {
    // 检查滚动更新是否在推进
    
    // 情况 1: 更新完成
    if deploymentutil.DeploymentComplete(d, status) {
        SetDeploymentCondition(status, apps.DeploymentProgressing, v1.ConditionTrue,
            "NewReplicaSetAvailable",
            fmt.Sprintf("ReplicaSet \"%s\" has successfully progressed.", newRS.Name))
        return
    }
    
    // 情况 2: 更新被暂停
    if d.Spec.Paused {
        SetDeploymentCondition(status, apps.DeploymentProgressing, v1.ConditionUnknown,
            "DeploymentPaused", "Deployment is paused")
        return
    }
    
    // 情况 3: 检查是否超时
    if deploymentutil.DeploymentProgressing(d, status) {
        // 更新正在进行中
        // 如果超过 progressDeadlineSeconds 仍未完成，标记为 False
        deadline := time.Duration(*d.Spec.ProgressDeadlineSeconds) * time.Second
        if time.Since(getLastProgressTime(d, status)) > deadline {
            SetDeploymentCondition(status, apps.DeploymentProgressing, v1.ConditionFalse,
                "ProgressDeadlineExceeded",
                fmt.Sprintf("ReplicaSet \"%s\" has timed out progressing.", newRS.Name))
        }
    }
}
```

---

## DeploymentComplete 判断逻辑

```go
// pkg/controller/deployment/util/deployment_util.go
func DeploymentComplete(deployment *apps.Deployment, newStatus *apps.DeploymentStatus) bool {
    return newStatus.UpdatedReplicas == *(deployment.Spec.Replicas) &&
        newStatus.Replicas == *(deployment.Spec.Replicas) &&
        newStatus.AvailableReplicas == *(deployment.Spec.Replicas) &&
        newStatus.ObservedGeneration >= deployment.Generation
}
```

**完成条件（全部必须满足）**：
1. `UpdatedReplicas == Spec.Replicas` — 所有 Pod 都是最新版本
2. `Replicas == Spec.Replicas` — 总副本数等于期望副本数（没有多余的旧 Pod）
3. `AvailableReplicas == Spec.Replicas` — 所有 Pod 都可用
4. `ObservedGeneration >= Generation` — 已处理最新的 Spec 变更

---

## Progress Deadline 机制

```yaml
spec:
  progressDeadlineSeconds: 600   # 默认 600 秒（10 分钟）
```

### 超时检测逻辑

```go
// pkg/controller/deployment/progress.go
func (dc *DeploymentController) checkProgressDeadline(deployment *apps.Deployment, newStatus *apps.DeploymentStatus) {
    if deployment.Spec.ProgressDeadlineSeconds == nil {
        return  // 未设置 deadline，不检查
    }
    
    condition := GetDeploymentCondition(newStatus, apps.DeploymentProgressing)
    if condition == nil {
        return
    }
    
    // 计算距离上次进度更新的时间
    now := time.Now()
    lastProgressTime := condition.LastTransitionTime.Time
    deadline := time.Duration(*deployment.Spec.ProgressDeadlineSeconds) * time.Second
    
    if now.Sub(lastProgressTime) > deadline {
        // 超时！标记 Progressing=False
        SetDeploymentCondition(newStatus, apps.DeploymentProgressing, v1.ConditionFalse,
            "ProgressDeadlineExceeded",
            "Deployment exceeded its progress deadline")
        
        // 生成事件
        dc.eventRecorder.Eventf(deployment, v1.EventTypeWarning, "ProgressDeadlineExceeded",
            "ReplicaSet \"%s\" has timed out progressing.", newRS.Name)
    }
}
```

**常见超时原因**：
- 镜像拉取失败（ImagePullBackOff）
- 资源不足导致 Pod Pending
- 健康检查持续失败（CrashLoopBackOff）
- 节点 NotReady 导致 Pod 无法调度

---

## ObservedGeneration 的作用

```go
status:
  observedGeneration: 3
```

**Generation 机制**：
- `metadata.generation`：每次 `Spec` 变更时自动递增
- `status.observedGeneration`：控制器已处理到的 Generation

**用途**：
```bash
# 判断控制器是否已处理最新配置
kubectl get deployment nginx -o jsonpath='{.metadata.generation} {.status.observedGeneration}'
# 输出: 3 3  → 已同步
# 输出: 4 3  → 新配置尚未处理完成
```

**为什么需要这个字段**：
- 控制器的同步是异步的
- 用户 apply 新配置后，不能立即假设新配置已生效
- `observedGeneration == generation` 表示控制器已完成对新 Spec 的处理

---

## 实战：通过 Status 诊断 Deployment 问题

```bash
# 1. 快速查看 Deployment 状态
kubectl get deployment nginx -o wide

# 2. 详细查看 Status
kubectl get deployment nginx -o jsonpath='{
  "replicas": .status.replicas,
  "updated": .status.updatedReplicas,
  "ready": .status.readyReplicas,
  "available": .status.availableReplicas,
  "unavailable": .status.unavailableReplicas,
  "generation": .metadata.generation,
  "observedGen": .status.observedGeneration
}' | jq .

# 3. 查看 Conditions
kubectl get deployment nginx -o jsonpath='{.status.conditions}' | jq .

# 4. 诊断脚本：判断 Deployment 是否健康
kubectl get deployment nginx -o json | jq '
  if (.status.observedGeneration == .metadata.generation)
     and (.status.updatedReplicas == .spec.replicas)
     and (.status.availableReplicas == .spec.replicas)
  then "HEALTHY"
  elif (.status.unavailableReplicas > 0) then "DEGRADED"
  elif (.status.observedGeneration != .metadata.generation) then "SYNCING"
  else "UNKNOWN"
  end
'
```
