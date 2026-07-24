---
title: Controller Pattern (Reconciliation Loop)
description: Controller Pattern (Reconciliation Loop) — Kubernetes 生产运维知识库
summary: Controller Pattern (Reconciliation Loop) — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- controller
- reconciliation
- design-pattern
- etcd
- hpa
- statefulset
- daemonset
- job
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Controller Pattern (Reconciliation Loop) 是什么
- 如何 Controller Pattern (Reconciliation Loop)
trigger_keywords:
- Controller
- Pattern
- Reconciliation
- Loop
prerequisites:
- kubectl-basics
- etcd-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Controller Pattern (Reconciliation Loop)

## Core Mechanism

The controller pattern is Kubernetes's fundamental automation mechanism. Every controller follows the same reconciliation loop:

1. **Observe**: Use Informer to Watch for resource changes and cache them locally
2. **Compare**: Diff desired state (Spec) against actual state (Status)
3. **Act**: Take corrective action to reduce the gap
4. **Update**: Write the new Status back to the API Server
5. **Repeat**: Wait for the next event or periodic re-sync

## Informer + Workqueue Architecture

Controllers use the **Informer pattern** for efficient state observation:
- **Informer**: Maintains a local cache, handles List+Watch, triggers event handlers
- **Indexer**: Provides fast lookup by labels, namespaces, or custom keys
- **Workqueue**: Decouples event detection from processing; supports rate-limited retries with exponential backoff

This architecture ensures controllers are resilient to API Server outages and network partitions.

## Built-in Controllers

| Controller | Observes | Manages | Purpose |
|-----------|----------|---------|---------|
| Deployment Controller | Deployment | [[ReplicaSet|ReplicaSet]] | Rolling updates, rollback |
| ReplicaSet Controller | ReplicaSet | Pod | Maintain replica count |
| [[StatefulSet|StatefulSet]] Controller | StatefulSet | Pod, PVC | Ordered stateful management |
| [[DaemonSet|DaemonSet]] Controller | DaemonSet, Node | Pod | One Pod per node |
| Job Controller | Job | Pod | Run-to-completion tasks |
| Node Controller | Node | Pod (eviction) | Node health monitoring |
| PV Controller | PV, PVC | PV, PVC | Volume binding |
| HPA Controller | HPA, metrics | Deployment | Horizontal autoscaling |

## Key Properties

- **Idempotent**: Running reconciliation multiple times produces the same result
- **Eventually Consistent**: System converges to desired state over time
- **Fault Tolerant**: Controller restart does not lose state (state lives in etcd)
- **Non-blocking**: Errors are re-queued with backoff, not blocking other reconciliations

## 源码实现分析

### Deployment Controller 调谐循环

```go
// kubernetes/pkg/controller/deployment/deployment_controller.go
func (dc *DeploymentController) syncDeployment(ctx context.Context, key string) error {
    // 1. 从 Informer 缓存获取 Deployment
    d, err := dc.deployLister.Deployments(ns).Get(name)
    
    // 2. 获取关联的所有 ReplicaSet
    rsList, _ := dc.getReplicaSetsForDeployment(d)
    
    // 3. 根据 strategy 执行滚动更新
    switch d.Spec.Strategy.Type {
    case apps.RollingUpdateDeploymentStrategyType:
        dc.rollOutRolling(ctx, d, rsList)  // 渐进式替换
    case apps.RecreateDeploymentStrategyType:
        dc.rollOutRecreate(ctx, d, rsList) // 先杀后建
    }
    
    // 4. 更新 Status（availableReplicas, conditions）
    dc.updateDeploymentStatus(ctx, d, rsList)
    return nil
}
```

### Workqueue 重试机制

```go
// client-go/util/workqueue/rate_limiting_queue.go
// 指数退避重试：5ms → 10ms → 20ms → ... → 1000s (cap)
queue.AddRateLimited(key)  // 失败后重新入队

// 完整流程：
// Event → Informer Handler → queue.Add(key)
// Worker goroutine → queue.Get(key) → syncHandler(key)
//   成功 → queue.Forget(key)
//   失败 → queue.AddRateLimited(key)  // 指数退避重试
```

## 使用场景

### 场景一：自定义 Operator 控制器（kubebuilder）

```go
// controllers/myapp_controller.go
func (r *MyAppReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    // 1. 获取 CR
    var app myv1alpha1.MyApp
    if err := r.Get(ctx, req.NamespacedName, &app); err != nil {
        return ctrl.Result{}, client.IgnoreNotFound(err)
    }
    // 2. 确保 Deployment 存在
    deploy := r.buildDeployment(&app)
    if err := ctrl.SetControllerReference(&app, deploy, r.Scheme); err != nil {
        return ctrl.Result{}, err
    }
    // 3. CreateOrUpdate
    _, err := controllerutil.CreateOrUpdate(ctx, r.Client, deploy, func() error {
        deploy.Spec.Replicas = &app.Spec.Replicas
        return nil
    })
    // 4. 更新 Status
    app.Status.ReadyReplicas = deploy.Status.ReadyReplicas
    r.Status().Update(ctx, &app)
    return ctrl.Result{RequeueAfter: 30 * time.Second}, nil
}
```

### 场景二：观察控制器行为

```bash
# 🟢 低风险 - 查看 Deployment 控制器事件
kubectl describe deployment my-app | grep -A 20 Events

# 🟢 低风险 - 查看 controller-manager 日志
kubectl logs -n kube-system kube-controller-manager-master-0 | grep deployment

# 🟢 低风险 - 查看 ReplicaSet 调谐状态
kubectl get rs -l app=my-app -o wide
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 控制器是同步执行的 | 控制器是异步事件驱动，通过 Workqueue 解耦事件与处理 |
| 失败后控制器会停止 | 失败后指数退避重试，不会阻塞其他对象的调谐 |
| 控制器重启会丢失状态 | 状态在 etcd，控制器重启后重新 List+Watch 即可恢复 |
| 一个控制器只管理一种资源 | 控制器可 Watch 多种资源（如 Deployment 控制器同时 Watch RS） |
| Reconcile 只被触发一次 | 周期性 resync（默认 10h）会重新触发所有对象的 Reconcile |
| 控制器直接操作 Pod | 控制器通过创建/删除下层资源（RS→Pod）间接管理 |

## 面试要点

1. **控制器模式的核心思想？** — Level-triggered（基于状态）而非 Edge-triggered（基于事件）。控制器不关心“发生了什么”，只关心“当前状态与期望是否一致”。这保证了幂等性和容错性。

2. **Informer 机制如何工作？** — List（全量拉取）+ Watch（增量事件）维护本地缓存（Store/Indexer）。事件触发 EventHandler 将 key 入 Workqueue，Worker 协程从队列取出并执行 Reconcile。优势：减少 API Server 压力、支持断线重连。

3. **如何保证控制器幂等性？** — 使用 CreateOrUpdate（存在则更新，不存在则创建）；通过 OwnerReference 建立父子关系；Status 更新用乐观锁（resourceVersion）避免冲突。

4. **Operator 与内置控制器的区别？** — 内置控制器管理通用资源（Deployment/StatefulSet）；Operator 通过 CRD 扩展领域知识（如 Prometheus Operator 知道如何配置 Prometheus 集群）。本质相同：都是 Watch + Reconcile。

## Related
- [[概念/控制器模式 × 可观测性.md|控制器模式 × 可观测性]] — 综合
- [[概念/控制器模式 × Deployment.md|控制器模式 × Deployment]] — 综合

- [[deployment]] — Deployment
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/eventual-consistency.md|eventual-consistency]] — Eventual Consistency in Kubernetes
- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[概念/declarative-api.md|Declarative API]]
- [[概念/watch-mechanism.md|Watch Mechanism]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[operator-pattern|Operator Pattern]]
- [[概念/eventual-consistency.md|Eventual Consistency]]

- 控制器模式与调谐循环
- [[实体/metal3-io.md|Metal3]] — Cross-reference


<!-- risk-assessed -->
