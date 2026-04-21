# Deployment 控制器架构总览

## 概述

Kubernetes Deployment 是最核心的工作负载控制器之一，它通过管理 **ReplicaSet** 来实现 Pod 的声明式更新。用户只需描述期望状态（镜像版本、副本数、更新策略），Deployment 控制器自动完成底层的创建、扩缩容、滚动替换和回滚操作。

---

## 源码路径

- **Deployment 控制器主控**: `pkg/controller/deployment/deployment_controller.go`
- **同步逻辑**: `pkg/controller/deployment/sync.go`
- **ReplicaSet 控制器**: `pkg/controller/replicaset/replica_set.go`
- **工具函数**: `pkg/controller/deployment/util/`

---

## 控制器链架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Deployment 控制器链                                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  用户操作                                                            │
│  ├── kubectl apply -f deployment.yaml                               │
│  ├── kubectl set image deployment/xxx                               │
│  └── kubectl scale deployment/xxx --replicas=5                      │
│         │                                                            │
│         ▼                                                            │
│  ┌──────────────┐                                                   │
│  │  API Server  │  ← 写入 etcd，触发 Watch 事件                     │
│  └──────────────┘                                                   │
│         │                                                            │
│         ▼                                                            │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │              Deployment Controller                           │   │
│  │                                                              │   │
│  │  1. Informer 通过 List/Watch 监听 Deployment 变更           │   │
│  │  2. 变更事件 → RateLimiter → WorkQueue                      │   │
│  │  3. Worker Goroutine 出队 → syncDeployment()                │   │
│  │  4. 计算期望 ReplicaSet 状态                                  │   │
│  │  5. 创建/更新 ReplicaSet 对象                                 │   │
│  │  6. 更新 Deployment Status                                    │   │
│  └─────────────────────────────────────────────────────────────┘   │
│         │                                                            │
│         ▼                                                            │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │              ReplicaSet Controller                           │   │
│  │                                                              │   │
│  │  1. 监听 ReplicaSet 变更                                      │   │
│  │  2. 计算期望 Pod 数量 = Replicas                             │   │
│  │  3. 对比当前 Pod 数量                                         │   │
│  │  4. 创建缺失的 Pod / 删除多余的 Pod                           │   │
│  │  5. 更新 ReplicaSet Status                                    │   │
│  └─────────────────────────────────────────────────────────────┘   │
│         │                                                            │
│         ▼                                                            │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │              kubelet + 调度器                                 │   │
│  │                                                              │   │
│  │  1. 调度器将 Pod 绑定到节点                                   │   │
│  │  2. kubelet 创建容器运行时实例                                │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Deployment 与 ReplicaSet 的关系

### 核心设计：分层控制

```
Deployment (用户直接操作)
    │
    ├── ReplicaSet-v1 (当前版本, replicas=5)
    │       ├── Pod-1
    │       ├── Pod-2
    │       ├── Pod-3
    │       ├── Pod-4
    │       └── Pod-5
    │
    └── ReplicaSet-v2 (旧版本, replicas=0)  ← 保留用于回滚
```

**分层原因**：
1. **解耦声明与实现**：用户声明 Deployment，控制器负责拆解为 ReplicaSet
2. **支持滚动更新**：同时存在多个 ReplicaSet，逐步迁移流量
3. **支持回滚**：保留历史 ReplicaSet，可快速切换版本
4. **状态追踪**：通过 ReplicaSet 的 PodTemplateHash 区分版本

---

## Informer 与 WorkQueue 机制

### 1. Deployment Informer

```go
// pkg/controller/deployment/deployment_controller.go
func NewDeploymentController(...) *DeploymentController {
    // 1. 创建 Informer，监听 Deployment 变更
    dc.dLister = informer.Lister()
    dc.dListerSynced = informer.Informer().HasSynced
    
    // 2. 注册事件处理器
    informer.Informer().AddEventHandler(cache.ResourceEventHandlerFuncs{
        AddFunc:    dc.addDeployment,
        UpdateFunc: dc.updateDeployment,
        DeleteFunc: dc.deleteDeployment,
    })
    
    // 3. 创建限速工作队列
    dc.queue = workqueue.NewNamedRateLimitingQueue(...)
}
```

### 2. 事件入队逻辑

```go
func (dc *DeploymentController) addDeployment(obj interface{}) {
    d := obj.(*apps.Deployment)
    // 获取 Deployment 的 key: namespace/name
    key, err := controller.KeyFunc(d)
    // 将 key 加入工作队列
    dc.queue.Add(key)
}

func (dc *DeploymentController) updateDeployment(oldObj, newObj interface{}) {
    oldD := oldObj.(*apps.Deployment)
    newD := newObj.(*apps.Deployment)
    
    // 只有当 Spec 变更时才需要同步
    // Status 变更不需要触发新的同步
    if !reflect.DeepEqual(oldD.Spec, newD.Spec) {
        dc.queue.Add(controller.KeyFunc(newD))
    }
}
```

**关键设计**：
- 队列中存储的是对象的 `namespace/name`，而非对象本身
- Worker 从队列取出 key 后，再向 Lister 查询最新对象
- 使用 `RateLimiter` 防止故障对象无限重试
- `Spec` 变更触发同步，`Status` 变更不触发（避免死循环）

### 3. Worker 处理循环

```go
func (dc *DeploymentController) worker() {
    for dc.processNextWorkItem() {
    }
}

func (dc *DeploymentController) processNextWorkItem() bool {
    // 1. 从队列取出 key
    key, quit := dc.queue.Get()
    if quit {
        return false
    }
    defer dc.queue.Done(key)
    
    // 2. 解析 namespace 和 name
    namespace, name, err := cache.SplitMetaNamespaceKey(key.(string))
    
    // 3. 从 Lister 获取最新 Deployment
    deployment, err := dc.dLister.Deployments(namespace).Get(name)
    
    // 4. 执行同步
    err = dc.syncHandler(deployment)
    
    // 5. 错误处理
    if err != nil {
        dc.queue.AddRateLimited(key)  // 限速重试
        return true
    }
    
    dc.queue.Forget(key)  // 成功，清除重试计数
    return true
}
```

---

## 控制器启动与初始化

```go
// cmd/kube-controller-manager/app/apps.go
func startDeploymentController(...) (controller.Interface, bool, error) {
    // 1. 创建 clientset
    client := ctx.ClientBuilder.ClientOrDie("deployment-controller")
    
    // 2. 创建 Deployment Controller
    dc, err := deployment.NewDeploymentController(
        ctx.InformerFactory.Core().V1().Pods(),
        ctx.InformerFactory.Apps().V1().ReplicaSets(),
        ctx.InformerFactory.Apps().V1().Deployments(),
        client,
    )
    
    // 3. 启动 Controller
    go dc.Run(ctx.Stop)
    
    return dc, true, nil
}
```

**控制器启动参数**：
```bash
# Deployment 控制器并发 workers 数量
--deployment-controller-sync-workers=5

# ReplicaSet 控制器并发 workers 数量
--replicaset-controller-sync-workers=5
```

---

## 关键源码文件索引

| 功能 | 源码路径 |
|-----|---------|
| 控制器入口与事件处理 | `pkg/controller/deployment/deployment_controller.go` |
| 主同步逻辑 | `pkg/controller/deployment/sync.go` |
| 滚动更新 | `pkg/controller/deployment/rolling.go` |
| Recreate 策略 | `pkg/controller/deployment/recreate.go` |
| 进度追踪 | `pkg/controller/deployment/progress.go` |
| 回滚逻辑 | `pkg/controller/deployment/rollback.go` |
| 工具函数 | `pkg/controller/deployment/util/` |
| ReplicaSet 控制器 | `pkg/controller/replicaset/replica_set.go` |
| Pod 创建/删除 | `pkg/controller/replicaset/replica_set_utils.go` |
