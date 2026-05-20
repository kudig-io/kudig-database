---
title: ReplicaSet 控制器源码分析
category: deployment
tags:
- replicaset
- controller
- pod
- syncReplicaset
- manageReplicas
- workload
last_updated: 2026-05-18
description: 深入分析 Kubernetes ReplicaSet 控制器的源码实现，涵盖 syncReplicaSet 核心同步函数、manageReplicas 期望状态对齐、Pod 创建流程、Status 更新以及与 Deployment
  的数据流关系。
difficulty: advanced
intent_queries:
- kubernetes replicaset controller source code analysis
- syncReplicaSet manageReplicas kubernetes
- replicaset pod creation workflow kubernetes
- BurstReplicas 500 kubernetes controller manager
- replicaset status availableReplicas readyReplicas
trigger_keywords:
- ReplicaSet Controller
- syncReplicaSet
- manageReplicas
- BurstReplicas
- CreatePods
- Pod OwnerReference
- FilterActivePods
- PodTemplateHash
- replicaset status
- FullyLabeledReplicas
reading_level: advanced
audience:
- platform-engineer
- kubernetes-developer
- sre
estimated_read_time: 5min
related_domains:
- domain-4-workloads
- domain-3-control-plane
related_topics:
- deployment-controller
- rolling-update
- deployment-status
domain_link: '[Workloads](../domain-4-workloads/README.md)'
topic_link: '[Deployment Create](./README.md)'
---


# ReplicaSet 控制器源码分析

## 概述

ReplicaSet（RS）是 Deployment 的底层执行器，负责维护指定数量的 Pod 副本。Deployment 控制器决定"要哪个版本、要多少个"，ReplicaSet 控制器则负责"实际创建和删除 Pod"。

---

## 源码路径

- **ReplicaSet 控制器主控**: `pkg/controller/replicaset/replica_set.go`
- **Pod 管理工具**: `pkg/controller/replicaset/replica_set_utils.go`
- **期望状态计算**: `pkg/controller/controller_utils.go`

---

## ReplicaSet 控制器架构

```
Deployment Controller
         │
         ▼  创建/更新 ReplicaSet 对象
    ┌─────────────┐
    │  ReplicaSet  │  (存储在 etcd)
    │   Object     │
    └─────────────┘
         │
         ▼  Watch 事件
    ┌─────────────────────────────┐
    │    ReplicaSet Controller     │
    │                              │
    │  1. 获取当前 Pod 数量        │
    │  2. 计算差值 = Replicas - 实际 │
    │  3. 差值 > 0 → 创建 Pod      │
    │  4. 差值 < 0 → 删除 Pod      │
    └─────────────────────────────┘
```

---

## syncReplicaSet — 核心同步函数

```go
// pkg/controller/replicaset/replica_set.go
func (rsc *ReplicaSetController) syncReplicaSet(ctx context.Context, key string) error {
    // 1. 解析 namespace/name
    namespace, name, err := cache.SplitMetaNamespaceKey(key)
    
    // 2. 获取 ReplicaSet
    rs, err := rsc.rsLister.ReplicaSets(namespace).Get(name)
    if err != nil {
        if errors.IsNotFound(err) {
            // ReplicaSet 已被删除，Pod 会被垃圾回收器清理
            return nil
        }
        return err
    }
    
    // 3. 获取该 RS 关联的所有 Pod
    podList, err := rsc.podLister.Pods(namespace).List(labels.SelectorFromSet(rs.Spec.Selector.MatchLabels))
    
    // 4. 过滤出真正属于该 RS 的 Pod（通过 OwnerReference 确认）
    filteredPods := controller.FilterActivePods(podList)
    
    // 5. 计算期望状态
    diff := *(rs.Spec.Replicas) - int32(len(filteredPods))
    
    // 6. 执行同步
    if diff > 0 {
        // 需要创建 diff 个 Pod
        return rsc.manageReplicas(ctx, rs, filteredPods, diff)
    } else if diff < 0 {
        // 需要删除 -diff 个 Pod
        return rsc.manageReplicas(ctx, rs, filteredPods, diff)
    }
    
    // 7. 更新 Status
    return rsc.updateReplicaSetStatus(ctx, rs, filteredPods)
}
```

---

## 期望状态对齐 — manageReplicas

```go
// pkg/controller/replicaset/replica_set.go
func (rsc *ReplicaSetController) manageReplicas(ctx context.Context, rs *apps.ReplicaSet, pods []*v1.Pod, diff int32) error {
    if diff < 0 {
        // ===== 缩容：删除多余的 Pod =====
        // 计算需要删除的 Pod 数量
        diff *= -1
        
        // 优雅删除：考虑 PodDisruptionBudget
        // 先标记删除，等待 kubelet 终止容器
        errCh := make(chan error, diff)
        var wg sync.WaitGroup
        wg.Add(int(diff))
        
        for i := 0; i < int(diff); i++ {
            go func(pod *v1.Pod) {
                defer wg.Done()
                if err := rsc.podControl.DeletePod(ctx, rs.Namespace, pod.Name, rs); err != nil {
                    errCh <- err
                }
            }(pods[i])
        }
        wg.Wait()
        
    } else if diff > 0 {
        // ===== 扩容：创建缺失的 Pod =====
        // 使用限速器控制批量创建速度，避免 API Server 压力过大
        // 每次最多创建 500 个（BurstReplicas = 500）
        
        // 实际创建的 Pod 数受限于 Batch 大小
        batchSize := diff
        if batchSize > controller.BurstReplicas {
            batchSize = controller.BurstReplicas
        }
        
        errCh := make(chan error, batchSize)
        var wg sync.WaitGroup
        wg.Add(int(batchSize))
        
        for i := 0; i < int(batchSize); i++ {
            go func() {
                defer wg.Done()
                // 从 ReplicaSet 的 Template 创建 Pod
                if err := rsc.podControl.CreatePods(ctx, rs.Namespace, &rs.Spec.Template, rs, metav1.GetOptionsOf(rs)); err != nil {
                    errCh <- err
                }
            }()
        }
        wg.Wait()
        
        // 如果还有剩余未创建的，重新入队
        if diff > batchSize {
            // 将 RS 重新加入队列，触发下一轮创建
            rsc.enqueueRS(rs)
        }
    }
    
    return nil
}
```

**关键设计**：

| 设计点 | 说明 |
|-------|------|
| `BurstReplicas = 500` | 单次同步最多创建/删除 500 个 Pod，防止突发流量压垮 API Server |
| 并发创建 | 使用 Goroutine 并发创建 Pod，提高吞吐量 |
| 限速器 | 全局 `podCreationRateLimiter` 控制创建速率 |
| 优雅删除 | 调用 `DeletePod` 而非强制终止，让 kubelet 执行优雅关闭 |

### Pod 创建限速器设计

```go
// pkg/controller/controller_utils.go
var podCreationRateLimiter = workqueue.NewRateLimiter(
    workqueue.NewItemExponentialFailureRateLimiter(1*time.Millisecond, 1*time.Minute),
    100, // QPS
)

// 在 manageReplicas 中使用
if err := rsc.podControl.CreatePods(...); err != nil {
    // 如果创建失败，将 RS 重新入队等待重试
    rsc.enqueueRS(rs)
}
```

**RateLimiter 配置**：
- 初始延迟：1ms（指数增长）
- 最大延迟：1分钟
- 增长因子：2（每次失败翻倍）
- 最大并发：100 QPS

### Pod 删除与 PDB 交互

```go
// 删除 Pod 前的 PDB 检查
func (r *RealPodControl) deletePod(namespace, name string, pod *v1.Pod) error {
    // 1. 获取 Pod 的 PDB
    pdbs, err := r.pdbLister.PodDisruptionBudgets(namespace).List(...)
    
    for _, pdb := range pdbs {
        if selector.Matches(labels.Set(pod.Labels)) {
            // 2. 检查 PDB 当前允许的 disruptions
            allowed := pdb.Status.DisruptionsAllowed
            if allowed == 0 {
                // 等待或跳过
                return fmt.Errorf("PodDisruptionBudget %s blocks deletion", pdb.Name)
            }
        }
    }
    
    // 3. 执行删除
    return r.KubeClient.CoreV1().Pods(namespace).Delete(ctx, name, ...)
}
```

---

## Pod 创建流程

```go
// pkg/controller/replicaset/replica_set_utils.go
func (r RealPodControl) CreatePods(ctx context.Context, namespace string, template *v1.PodTemplateSpec, object runtime.Object) error {
    // 1. 从 Template 深拷贝创建 Pod 对象
    pod := &v1.Pod{
        ObjectMeta: metav1.ObjectMeta{
            Namespace:    namespace,
            GenerateName: template.GenerateName,  // 或使用 ControllerRef 生成名称
            Labels:       template.Labels,
            Annotations:  template.Annotations,
            OwnerReferences: []metav1.OwnerReference{
                // 设置 ReplicaSet 为 Pod 的 Owner
                *metav1.NewControllerRef(controllerObject, rsKind),
            },
        },
        Spec: template.Spec,
    }
    
    // 2. 通过 clientset 创建 Pod
    // 注意：此时 Pod 的 spec.nodeName 为空，需要等待调度器分配节点
    _, err := r.KubeClient.CoreV1().Pods(namespace).Create(ctx, pod, metav1.CreateOptions{})
    
    return err
}
```

**Pod 名称生成**：
```
ReplicaSet Name: nginx-7c4c8d5d4f
Pod Name:        nginx-7c4c8d5d4f-abcde  (随机后缀)
                 nginx-7c4c8d5d4f-fghij
                 nginx-7c4c8d5d4f-klmno
```

---

## ReplicaSet Status 更新

```go
// pkg/controller/replicaset/replica_set.go
func (rsc *ReplicaSetController) updateReplicaSetStatus(ctx context.Context, rs *apps.ReplicaSet, pods []*v1.Pod) error {
    // 1. 计算各类副本数
    replicas := int32(len(pods))
    fullyLabeledReplicas := 0
    readyReplicas := 0
    availableReplicas := 0
    
    for _, pod := range pods {
        // 检查 Pod 是否完全匹配 ReplicaSet 的标签
        if labels.Set(rs.Spec.Selector.MatchLabels).AsSelectorPreValidated().Matches(labels.Set(pod.Labels)) {
            fullyLabeledReplicas++
        }
        
        // 检查 Pod Ready
        if podutil.IsPodReady(pod) {
            readyReplicas++
            
            // 检查 Pod Available（Ready 超过 minReadySeconds）
            if podutil.IsPodAvailable(pod, rs.Spec.MinReadySeconds, metav1.Now()) {
                availableReplicas++
            }
        }
    }
    
    // 2. 构建新的 Status
    newStatus := apps.ReplicaSetStatus{
        Replicas:             replicas,
        FullyLabeledReplicas: int32(fullyLabeledReplicas),
        ReadyReplicas:        int32(readyReplicas),
        AvailableReplicas:    int32(availableReplicas),
        ObservedGeneration:   rs.Generation,
    }
    
    // 3. 如果 Status 有变化，更新 ReplicaSet
    if !reflect.DeepEqual(rs.Status, newStatus) {
        rs.Status = newStatus
        _, err := rsc.kubeClient.AppsV1().ReplicaSets(rs.Namespace).UpdateStatus(ctx, rs, metav1.UpdateOptions{})
        return err
    }
    
    return nil
}
```

**Status 字段含义**：

| 字段 | 含义 | 计算方式 |
|-----|------|---------|
| `Replicas` | 实际 Pod 数量 | `len(pods)` |
| `FullyLabeledReplicas` | 标签完全匹配的 Pod 数 | 通过 Selector 验证 |
| `ReadyReplicas` | Ready 状态的 Pod 数 | `IsPodReady(pod)` |
| `AvailableReplicas` | 可用 Pod 数（Ready 超过 MinReadySeconds） | `IsPodAvailable(pod, minReadySeconds, now)` |
| `ObservedGeneration` | 已处理的 Generation | `rs.Generation` |

---

## ReplicaSet 与 Deployment 的数据流

```
用户: kubectl apply -f deployment.yaml
         │
         ▼
API Server: 更新 Deployment 对象 (Generation + 1)
         │
         ▼
Deployment Controller:
    ├─ syncDeployment()
    ├─ 发现 PodTemplate 变更
    ├─ 创建新的 ReplicaSet (Replicas = 0 初始)
    └─ 逐步调整新旧 ReplicaSet 的 Replicas
         │
         ▼ (两次或多次 ReplicaSet 更新)
ReplicaSet Controller (旧 RS):
    ├─ syncReplicaSet()
    ├─ 发现 Replicas 减少
    └─ 删除多余 Pod
         │
ReplicaSet Controller (新 RS):
    ├─ syncReplicaSet()
    ├─ 发现 Replicas 增加
    └─ 创建新 Pod
         │
         ▼
调度器: 为新 Pod 分配节点
         │
         ▼
kubelet: 在节点上创建容器
```

---

## 关键源码文件索引

| 功能 | 源码路径 |
|-----|---------|
| ReplicaSet 控制器主控 | `pkg/controller/replicaset/replica_set.go` |
| Pod 创建/删除工具 | `pkg/controller/replicaset/replica_set_utils.go` |
| 通用控制器工具 | `pkg/controller/controller_utils.go` |
| Pod 状态判断 | `pkg/api/v1/pod/util.go` |
