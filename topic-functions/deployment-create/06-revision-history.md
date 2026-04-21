# 版本历史与回滚机制

## 概述

Deployment 通过维护多个 ReplicaSet 来实现版本历史记录和一键回滚。每次修改 `PodTemplate` 都会创建一个新的 ReplicaSet，旧版本不会被立即删除，而是保留用于回滚。本文档基于 `pkg/controller/deployment/rollback.go` 和相关源码，分析版本管理和回滚的完整逻辑。

---

## 源码路径

- **回滚逻辑**: `pkg/controller/deployment/rollback.go`
- **版本管理**: `pkg/controller/deployment/util/deployment_util.go`
- **清理逻辑**: `pkg/controller/deployment/sync.go`

---

## Revision 标识机制

### PodTemplateHash 与 Revision

```go
// pkg/controller/deployment/util/deployment_util.go
const (
    RevisionAnnotation            = "deployment.kubernetes.io/revision"
    RevisionHistoryAnnotation     = "deployment.kubernetes.io/revision-history"
    DesiredReplicasAnnotation     = "deployment.kubernetes.io/desired-replicas"
    MaxReplicasAnnotation         = "deployment.kubernetes.io/max-replicas"
)
```

**Deployment 上的 Annotation**：
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  annotations:
    deployment.kubernetes.io/revision: "3"
    deployment.kubernetes.io/revision-history: "1,2"
spec:
  revisionHistoryLimit: 10
```

**ReplicaSet 上的 Annotation**：
```yaml
apiVersion: apps/v1
kind: ReplicaSet
metadata:
  annotations:
    deployment.kubernetes.io/revision: "2"
    deployment.kubernetes.io/desired-replicas: "5"
    deployment.kubernetes.io/max-replicas: "7"
```

### Revision 号的分配规则

```go
// pkg/controller/deployment/util/deployment_util.go
func SetNewReplicaSetAnnotations(deployment *apps.Deployment, newRS *apps.ReplicaSet, newRevision int64) {
    // 1. 获取当前最大 Revision 号
    currentRevision := GetRevision(deployment)
    
    // 2. 新 RS 的 Revision = 当前最大 + 1
    annotation := strconv.FormatInt(newRevision, 10)
    newRS.Annotations[RevisionAnnotation] = annotation
    
    // 3. 记录创建时的期望副本数和最大副本数（用于比例缩放）
    newRS.Annotations[DesiredReplicasAnnotation] = strconv.Itoa(int(*deployment.Spec.Replicas))
    newRS.Annotations[MaxReplicasAnnotation] = strconv.Itoa(int(GetMaxReplicas(deployment)))
}
```

**Revision 递增规则**：
- 每次 `PodTemplate` 变更触发新的 ReplicaSet 创建
- 新 ReplicaSet 的 Revision = 当前最大 Revision + 1
- `kubectl rollout history` 显示的版本号即为此值

---

## revisionHistoryLimit 与清理策略

### 配置

```yaml
apiVersion: apps/v1
kind: Deployment
spec:
  revisionHistoryLimit: 10   # 默认 10，设为 0 则不保留历史版本
```

### 清理逻辑

```go
// pkg/controller/deployment/sync.go
func (dc *DeploymentController) cleanupDeployment(oldRSs []*apps.ReplicaSet, deployment *apps.Deployment) {
    // 1. 获取 revisionHistoryLimit
    revisionHistoryLimit := deployment.Spec.RevisionHistoryLimit
    if revisionHistoryLimit == nil {
        // 默认值为 10
        defaultLimit := int32(10)
        revisionHistoryLimit = &defaultLimit
    }
    
    // 2. 如果设为 0，清理所有旧 ReplicaSet
    if *revisionHistoryLimit == 0 {
        for _, rs := range oldRSs {
            dc.client.AppsV1().ReplicaSets(rs.Namespace).Delete(ctx, rs.Name, metav1.DeleteOptions{})
        }
        return
    }
    
    // 3. 按 Revision 排序，保留最新的 N 个
    SortReplicaSetsByRevision(oldRSs)
    
    // 4. 删除超出限制的旧 ReplicaSet
    if len(oldRSs) > int(*revisionHistoryLimit) {
        for i := 0; i < len(oldRSs) - int(*revisionHistoryLimit); i++ {
            dc.client.AppsV1().ReplicaSets(oldRSs[i].Namespace).Delete(ctx, oldRSs[i].Name, metav1.DeleteOptions{})
        }
    }
}
```

**注意**：
- 清理的是 **ReplicaSet 对象**，不是 Pod
- ReplicaSet 被删除后，其 Pod 会被垃圾回收器清理
- `revisionHistoryLimit` 不影响当前活跃的 ReplicaSet

---

## 回滚机制

### 用户触发回滚

```bash
# 查看版本历史
kubectl rollout history deployment/nginx

# 回滚到上一个版本
kubectl rollout undo deployment/nginx

# 回滚到指定版本
kubectl rollout undo deployment/nginx --to-revision=2
```

### 回滚的 API 操作

`kubectl rollout undo` 实际上执行的是 **Patch Deployment** 操作：

```yaml
# 1. 读取指定版本的 ReplicaSet 的 PodTemplate
# 2. 将该 PodTemplate 应用到 Deployment 的 Spec
# 3. Deployment Controller 检测到 Spec 变更，触发正常的滚动更新
```

**关键**：回滚不是直接切换 ReplicaSet，而是将旧版本的 PodTemplate 写回 Deployment，触发一次"向旧版本更新"的滚动更新。

### 回滚源码分析

```go
// pkg/controller/deployment/rollback.go
func (dc *DeploymentController) rollbackToRevision(deployment *apps.Deployment, toRevision int64) error {
    // 1. 获取所有关联的 ReplicaSet
    rsList, err := dc.getReplicaSetsForDeployment(ctx, deployment)
    
    // 2. 查找目标 Revision 的 ReplicaSet
    var targetRS *apps.ReplicaSet
    for _, rs := range rsList {
        if revision, _ := GetRevision(rs); revision == toRevision {
            targetRS = rs
            break
        }
    }
    
    if targetRS == nil {
        return fmt.Errorf("unable to find specified revision %v in history", toRevision)
    }
    
    // 3. 将目标 RS 的 PodTemplate 应用到 Deployment
    // 这实际上是一次 Deployment Spec 更新
    deployment.Spec.Template = targetRS.Spec.Template
    
    // 4. 更新 Deployment（这会触发正常的 syncDeployment）
    _, err = dc.client.AppsV1().Deployments(deployment.Namespace).Update(ctx, deployment, metav1.UpdateOptions{})
    
    return err
}
```

### 回滚后的 Revision 号

```
初始:
  Revision 1: nginx:1.19
  Revision 2: nginx:1.20
  Revision 3: nginx:1.21  ← 当前

回滚到 Revision 2:
  Revision 1: nginx:1.19
  Revision 2: nginx:1.20
  Revision 3: nginx:1.21
  Revision 4: nginx:1.20  ← 回滚后创建的新 ReplicaSet
```

**关键**：回滚不会复用旧的 ReplicaSet，而是创建一个新的 ReplicaSet，其 PodTemplate 与目标版本相同，但 Revision 号是新的（当前最大 + 1）。

---

## 比例回滚

当 Deployment 规模很大时，回滚也遵循与更新相同的 `maxSurge` 和 `maxUnavailable` 约束，逐步替换 Pod。

```
当前: Revision 3 = nginx:1.21 (replicas=100)
回滚到: Revision 2 = nginx:1.20

过程:
  Step 1: 创建 Revision 4 RS (PodTemplate = 1.20), replicas=25
          Revision 3 RS: replicas=100
          
  Step 2: Revision 4 RS: replicas=50
          Revision 3 RS: replicas=75
          
  Step 3: Revision 4 RS: replicas=75
          Revision 3 RS: replicas=50
          
  Step 4: Revision 4 RS: replicas=100
          Revision 3 RS: replicas=0
          
完成: 所有 Pod 都运行 nginx:1.20，但 Revision 号递增到了 4
```

---

## 金丝雀发布与 Deployment

Deployment 本身不直接支持金丝雀（按百分比流量切换），但可以通过以下方式实现：

### 方案 1: 手动暂停 + 验证

```bash
# 1. 更新镜像
kubectl set image deployment/nginx nginx=nginx:2.0

# 2. 立即暂停
kubectl rollout pause deployment/nginx

# 3. 此时只有 1 个新 Pod（maxSurge=1, maxUnavailable=0）
kubectl get pods -l app=nginx

# 4. 验证新版本
kubectl exec -it <new-pod> -- curl localhost/health

# 5. 验证通过后继续
kubectl rollout resume deployment/nginx
```

### 方案 2: 使用两个 Deployment + Service

```yaml
# deployment-stable.yaml (当前版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-stable
spec:
  replicas: 9
  selector:
    matchLabels:
      app: nginx
      track: stable

---
# deployment-canary.yaml (金丝雀版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-canary
spec:
  replicas: 1    # 10% 流量
  selector:
    matchLabels:
      app: nginx
      track: canary
```

Service 同时选择两个 Deployment 的 Pod，通过副本数比例控制流量分配。

---

## 版本历史实战

```bash
# 1. 查看版本历史
kubectl rollout history deployment/nginx

# 输出:
# deployment.apps/nginx
# REVISION  CHANGE-CAUSE
# 1         <none>
# 2         kubectl set image deployment/nginx nginx=nginx:1.20
# 3         kubectl set image deployment/nginx nginx=nginx:1.21

# 2. 查看特定版本的详细差异
kubectl rollout history deployment/nginx --revision=2

# 3. 回滚
kubectl rollout undo deployment/nginx --to-revision=2

# 4. 设置 change-cause（记录变更原因）
kubectl annotate deployment/nginx kubernetes.io/change-cause="升级 nginx 到 1.21 修复 CVE"

# 5. 查看当前 ReplicaSet（版本对应关系）
kubectl get rs -l app=nginx

# 6. 清理历史版本（限制为 5 个）
kubectl patch deployment nginx -p '{"spec":{"revisionHistoryLimit":5}}'
```

---

## 关键源码文件索引

| 功能 | 源码路径 |
|-----|---------|
| 回滚实现 | `pkg/controller/deployment/rollback.go` |
| 版本号管理 | `pkg/controller/deployment/util/deployment_util.go` |
| 历史清理 | `pkg/controller/deployment/sync.go` |
| Revision Annotation | `pkg/controller/deployment/util/revision.go` |
