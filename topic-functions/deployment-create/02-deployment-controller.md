# Deployment 控制器入口源码分析

## 概述

Deployment 控制器的核心逻辑集中在 `syncDeployment` 函数中。当用户创建或更新 Deployment 时，该函数被触发，负责将用户的声明式配置转换为底层 ReplicaSet 的创建、更新和删除操作。

---

## 源码路径

- **控制器主控**: `pkg/controller/deployment/deployment_controller.go`
- **同步逻辑**: `pkg/controller/deployment/sync.go`
- **工具函数**: `pkg/controller/deployment/util/deployment_util.go`

---

## syncDeployment — 核心同步函数

```go
// pkg/controller/deployment/sync.go
func (dc *DeploymentController) syncDeployment(ctx context.Context, key string) error {
    // 1. 解析 namespace 和 name
    namespace, name, err := cache.SplitMetaNamespaceKey(key)
    
    // 2. 获取 Deployment 对象
    deployment, err := dc.dLister.Deployments(namespace).Get(name)
    if err != nil {
        if errors.IsNotFound(err) {
            // Deployment 已被删除，清理相关 ReplicaSet
            return dc.cleanupDeployment(ctx, key)
        }
        return err
    }
    
    // 3. 获取该 Deployment 关联的所有 ReplicaSet
    rsList, err := dc.getReplicaSetsForDeployment(ctx, deployment)
    
    // 4. 获取这些 ReplicaSet 关联的所有 Pod
    podList, err := dc.getPodsForReplicaSets(rsList)
    
    // 5. 根据部署策略执行同步
    if deployment.Spec.Paused {
        // 如果 Deployment 被暂停，只同步状态不做变更
        return dc.sync(ctx, deployment, podList, rsList, rsList)
    }
    
    switch deployment.Spec.Strategy.Type {
    case apps.RecreateDeploymentStrategyType:
        // Recreate 策略：先删除所有旧 Pod，再创建新 Pod
        return dc.rolloutRecreate(ctx, deployment, rsList, podList)
        
    case apps.RollingUpdateDeploymentStrategyType:
        // RollingUpdate 策略：渐进式替换 Pod
        return dc.rolloutRolling(ctx, deployment, rsList, podList)
    }
    
    return fmt.Errorf("unexpected deployment strategy type: %s", deployment.Spec.Strategy.Type)
}
```

**核心流程**：
1. 获取目标 Deployment 及其关联的所有 ReplicaSet
2. 获取这些 ReplicaSet 管理的所有 Pod
3. 根据 `Spec.Strategy.Type` 选择执行路径
4. 更新 Deployment Status

---

## 获取关联 ReplicaSet

```go
// pkg/controller/deployment/deployment_controller.go
func (dc *DeploymentController) getReplicaSetsForDeployment(ctx context.Context, d *apps.Deployment) ([]*apps.ReplicaSet, error) {
    // 1. 获取命名空间下的所有 ReplicaSet
    rsList, err := dc.rsLister.ReplicaSets(d.Namespace).List(labels.Everything())
    
    // 2. 筛选出属于该 Deployment 的 ReplicaSet
    // 匹配条件：ReplicaSet 的 OwnerReferences 指向该 Deployment
    var ownedRS []*apps.ReplicaSet
    for _, rs := range rsList {
        if metav1.IsControlledBy(rs, d) {
            ownedRS = append(ownedRS, rs)
        }
    }
    
    return ownedRS, nil
}
```

**OwnerReferences 机制**：
```yaml
apiVersion: apps/v1
kind: ReplicaSet
metadata:
  name: nginx-7c4c8d5d4f
  ownerReferences:
  - apiVersion: apps/v1
    kind: Deployment
    name: nginx
    uid: <deployment-uid>
    controller: true       # ← 标记为控制器关系
    blockOwnerDeletion: true
```

---

## 确定当前和新 ReplicaSet

```go
// pkg/controller/deployment/sync.go
func (dc *DeploymentController) getAllReplicaSetsAndSyncRevision(...)

// 核心逻辑：区分"当前活跃的 ReplicaSet"和"需要创建的 ReplicaSet"
```

### PodTemplateHash 的作用

Deployment 通过计算 PodTemplate 的 hash 来识别不同版本：

```go
// pkg/controller/deployment/util/deployment_util.go
func GetPodTemplateSpecHash(deployment *apps.Deployment) (string, error) {
    // 对 PodTemplateSpec 进行哈希计算
    podTemplateSpecHasher := fnv.New32a()
    hash.DeepHashObject(podTemplateSpecHasher, deployment.Spec.Template)
    return fmt.Sprintf("%d", podTemplateSpecHasher.Sum32()), nil
}
```

**应用方式**：
```yaml
# Deployment 的 PodTemplate
spec:
  template:
    metadata:
      labels:
        app: nginx
        pod-template-hash: "7c4c8d5d4f"  # ← 自动注入
```

**作用**：
1. 区分不同版本的 ReplicaSet
2. 作为 Selector 的一部分，确保每个 ReplicaSet 只管理自己的 Pod
3. 用户不应手动设置此标签

---

## 创建新 ReplicaSet

```go
// pkg/controller/deployment/sync.go
func (dc *DeploymentController) getNewReplicaSet(ctx context.Context, d *apps.Deployment, rsList []*apps.ReplicaSet, createIfNotExits bool) (*apps.ReplicaSet, error) {
    // 1. 查找是否存在与当前 PodTemplate 匹配的 ReplicaSet
    existingNewRS := FindNewReplicaSet(d, rsList)
    if existingNewRS != nil {
        return existingNewRS, nil
    }
    
    // 2. 如果不存在且允许创建，则新建 ReplicaSet
    if createIfNotExits {
        newRS := &apps.ReplicaSet{
            ObjectMeta: metav1.ObjectMeta{
                Name:            d.Name + "-" + podTemplateHash,
                Namespace:       d.Namespace,
                OwnerReferences: []metav1.OwnerReference{*metav1.NewControllerRef(d, controllerKind)},
            },
            Spec: apps.ReplicaSetSpec{
                Replicas:        d.Spec.Replicas,
                Selector:        d.Spec.Selector,
                Template:        d.Spec.Template,
            },
        }
        
        // 注入 pod-template-hash 标签
        newRS.Labels = CloneAndAddLabel(d.Spec.Template.Labels, "pod-template-hash", podTemplateHash)
        newRS.Spec.Template.Labels = newRS.Labels
        newRS.Spec.Selector.MatchLabels = newRS.Labels
        
        return dc.client.AppsV1().ReplicaSets(d.Namespace).Create(ctx, newRS, metav1.CreateOptions{})
    }
    
    return nil, nil
}
```

---

## Deployment 暂停机制

```go
// pkg/controller/deployment/sync.go
if deployment.Spec.Paused {
    // 暂停时：只同步 ReplicaSet 数量，不执行滚动更新
    return dc.sync(ctx, deployment, podList, rsList, rsList)
}
```

**使用场景**：
```bash
# 暂停 Deployment，阻止滚动更新
kubectl rollout pause deployment/nginx

# 此时可以多次修改 Deployment Spec（如更换镜像）
kubectl set image deployment/nginx nginx=nginx:1.20
kubectl set resources deployment/nginx -c=nginx --limits=cpu=500m

# 恢复后，所有修改一次性生效
kubectl rollout resume deployment/nginx
```

**源码逻辑**：
- 暂停时，Controller 仍会根据 `Spec.Replicas` 调整 ReplicaSet 的副本数
- 但不会创建新的 ReplicaSet 或执行滚动更新
- 恢复后，如果 PodTemplate 已变更，立即触发正常的滚动更新流程

---

## 删除 Deployment 时的级联清理

```go
// pkg/controller/deployment/deployment_controller.go
func (dc *DeploymentController) cleanupDeployment(ctx context.Context, key string) error {
    // 1. 解析 namespace 和 name
    namespace, name, _ := cache.SplitMetaNamespaceKey(key)
    
    // 2. 由于 Deployment 已被删除，通过 label selector 查找残留的 ReplicaSet
    selector, err := metav1.LabelSelectorAsSelector(deployment.Spec.Selector)
    
    // 3. 获取该 selector 匹配的 ReplicaSet
    rsList, err := dc.rsLister.ReplicaSets(namespace).List(selector)
    
    // 4. 删除所有关联的 ReplicaSet
    // 注意：由于 OwnerReference 的 blockOwnerDeletion=true
    // 在 Deployment 删除完成前，这些 ReplicaSet 会被垃圾回收器自动清理
    // Controller 这里的清理是额外的兜底
    for _, rs := range rsList {
        if metav1.IsControlledBy(rs, deployment) {
            dc.client.AppsV1().ReplicaSets(namespace).Delete(ctx, rs.Name, metav1.DeleteOptions{})
        }
    }
    
    return nil
}
```

**垃圾回收机制**：
- Kubernetes 使用 **级联删除（Cascading Deletion）**
- Deployment 作为 ReplicaSet 的 `ownerReferences[0]`，且 `blockOwnerDeletion: true`
- 删除 Deployment 时，垃圾回收器自动删除其所有 ReplicaSet
- ReplicaSet 被删除时，又自动删除其所有 Pod
- 最终形成完整的级联清理链
