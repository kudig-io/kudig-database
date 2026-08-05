---
title: kube-controller-manager 源码深度剖析
description: 基于 kubernetes-1.36.2 源码的 KCM 启动机制、控制器注册表、Deployment/ReplicaSet 调谐链、GC 引用图与 Leader Election 完整剖析
summary: 剖析 NewControllerDescriptors 控制器注册表、leaderElectAndRun 选主启动、Deployment→ReplicaSet→Pod 三级调谐、垃圾回收 GraphBuilder 引用图，全部函数附实测行号。
category: source-analysis
tags:
- k8s
- source-code
- controller-manager
- deployment
- replicaset
- garbage-collector
- leader-election
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 30min
intent_queries:
- kube-controller-manager 控制器如何注册启动
- Deployment 滚动更新源码流程
- ReplicaSet manageReplicas 扩缩容逻辑
- Kubernetes 垃圾回收 ownerReference 源码
trigger_keywords:
- kube-controller-manager
- syncDeployment
- manageReplicas
- GraphBuilder
- leaderElectAndRun
- ControllerDescriptor
related_domains:
- 集群基础
- 工作负载
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# kube-controller-manager 源码深度剖析

> **源码基线**：`33-源码/控制平面/kubernetes-1.36.2/`
> 概念层配套阅读：[[01-集群基础/03-控制平面/13-kube-controller-manager-deep-dive.md|控制平面：KCM Deep Dive]] · [[01-集群基础/02-设计原则/04-controller-pattern.md|控制器模式与调谐循环]]

## 概述

KCM 本质上是**一个进程壳 + 40 多个独立控制器**。每个控制器遵循同一范式：Informer 感知事件 → key 入 WorkQueue → worker 取出执行 `syncXxx` 调谐 → 失败限速重入队。本文剖析：

1. 控制器注册表与启动流程（含 Leader Election）
2. 工作负载调谐主链：Deployment → ReplicaSet → Pod
3. 垃圾回收器的引用图（GraphBuilder）机制

---

## 一、启动流程与控制器注册表

### 1.1 Run 与选主

```go
// cmd/kube-controller-manager/app/controllermanager.go:199
func Run(ctx context.Context, c *config.CompletedConfig) error {
    // run 闭包：构建 ControllerContext → StartControllers → 启动 Informer 工厂
    ...
    // :397 —— 未禁用选主时，先赢得 Lease 再执行 run
    leaderElectAndRun(ctx, c, id, electionChecker,
        c.ComponentConfig.Generic.LeaderElection.ResourceLock,  // 默认 "leases"
        "kube-controller-manager",                              // kube-system/kube-controller-manager
        leaderelection.LeaderCallbacks{OnStartedLeading: run, OnStoppedLeading: ...})
}

// controllermanager.go:840 — 封装 client-go leaderelection
func leaderElectAndRun(...)
```

底层选主实现（多副本 KCM/Scheduler 热备的公共机制）：

```go
// staging/src/k8s.io/client-go/tools/leaderelection/leaderelection.go（实测行号）
func NewLeaderElector(lec LeaderElectionConfig) (*LeaderElector, error)  // :76
func (le *LeaderElector) Run(ctx context.Context)      // :211  acquire → OnStartedLeading → renew
func (le *LeaderElector) acquire(ctx context.Context)  // :252  按 RetryPeriod 轮询抢锁
func (le *LeaderElector) renew(ctx context.Context)    // :279  RenewDeadline 内续约失败即退出
```

三个时间参数的语义（默认 15s/10s/2s）：`LeaseDuration` 是备节点认定锁过期的时长；`RenewDeadline` 是主节点放弃领导权前的续约窗口；`RetryPeriod` 是动作间隔。**主备切换的最坏空窗 ≈ LeaseDuration + RetryPeriod**，这是「KCM 切主后工作负载停止调谐十几秒」的源码解释。

### 1.2 控制器注册表

```go
// cmd/kube-controller-manager/app/controller_descriptor.go:148
func NewControllerDescriptors() map[string]*ControllerDescriptor {
    register(newDeploymentControllerDescriptor())        // deployment
    register(newReplicaSetControllerDescriptor())        // replicaset
    register(newNodeLifecycleControllerDescriptor())     // node-lifecycle
    register(newGarbageCollectorControllerDescriptor())  // garbage-collector
    // ... 40+ 个，--controllers 参数按此名单启停
}
```

每个 Descriptor 携带 `initFunc`、所需 Feature Gate 与别名。`--controllers=*,-ttl` 这类参数就作用于这张表。所有控制器共享同一个 `SharedInformerFactory`——**同一资源在 KCM 内只有一份缓存与一条 Watch 连接**（机制详见 [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|Informer 机制剖析]]）。

---

## 二、工作负载调谐主链：Deployment → ReplicaSet → Pod

三级控制器各管一层，通过 ownerReference 关联，任何一级都不直接跨层操作：

```
Deployment 控制器: 管理 ReplicaSet（新旧 RS 的副本配比 = 滚动更新）
ReplicaSet 控制器: 管理 Pod（数量收敛）
kubelet:           管理容器（Pod → 运行态，见节点侧文档）
```

### 2.1 DeploymentController.syncDeployment

```go
// pkg/controller/deployment/deployment_controller.go:589
func (dc *DeploymentController) syncDeployment(ctx context.Context, key string) error {
    deployment, err := dc.dLister.Deployments(namespace).Get(name) // 读 Informer 缓存
    rsList, err := dc.getReplicaSetsForDeployment(ctx, d)          // 认领/释放 RS (ownerRef)
    if d.Spec.Paused { return dc.sync(ctx, d, rsList) }            // 暂停态只做 scale
    if getRollbackTo(d) != nil { return dc.rollback(ctx, d, rsList) }
    switch d.Spec.Strategy.Type {
    case apps.RecreateDeploymentStrategyType:
        return dc.rolloutRecreate(ctx, d, rsList, podMap)          // recreate.go:29
    case apps.RollingUpdateDeploymentStrategyType:
        return dc.rolloutRolling(ctx, d, rsList)                   // rolling.go:31
    }
}
```

### 2.2 滚动更新的数学核心

```go
// pkg/controller/deployment/rolling.go:31
func (dc *DeploymentController) rolloutRolling(ctx context.Context, d *apps.Deployment, rsList []*apps.ReplicaSet) error {
    newRS, oldRSs, err := dc.getAllReplicaSetsAndSyncRevision(...)  // 无新 RS 则按 pod-template-hash 创建
    scaledUp, err := dc.reconcileNewReplicaSet(...)   // 受 maxSurge 约束扩新 RS
    scaledDown, err := dc.reconcileOldReplicaSets(...) // 受 maxUnavailable 约束缩旧 RS
    // 每轮只推进一步，等下次事件再继续 —— 调谐是增量的、可中断的
}
```

- 新 RS 名 = Deployment 名 + pod-template-hash（对 PodTemplate 哈希），**这就是改镜像会创建新 RS、改 replicas 不会的原因**
- 卡在 `1 old replicas are pending termination`：旧 Pod 无法终止（PDB/finalizer/节点失联）导致 `reconcileOldReplicaSets` 无法推进——从这个函数的约束条件反查即可

### 2.3 ReplicaSetController：数量收敛

```go
// pkg/controller/replicaset/replica_set.go:755
func (rsc *ReplicaSetController) syncReplicaSet(ctx context.Context, key string) error {
    rs, err := rsc.rsLister.ReplicaSets(namespace).Get(name)
    filteredPods := controller.FilterActivePods(allPods)     // 过滤 Succeeded/Failed
    filteredPods, err = rsc.claimPods(ctx, rs, selector, filteredPods) // ownerRef 认领
    return rsc.manageReplicas(ctx, filteredPods, rs)
}

// replica_set.go:649 — diff = len(activePods) - spec.Replicas
func (rsc *ReplicaSetController) manageReplicas(ctx context.Context, activePods []*v1.Pod, rs *apps.ReplicaSet) error {
    if diff < 0 {
        // 慢启动批量创建: 1, 2, 4, 8...（slowStartBatch，防止配额错误刷爆 API）
    } else if diff > 0 {
        // 按 getPodsToDelete 排序挑删除对象:
        // 未调度 < Pending < Unready < 运行时间短 < 重启次数多 ...
    }
}
```

`getPodsToDelete` 的排序规则解释了缩容时「为什么优先删的是那几个 Pod」；需要自定义时用 `controller.kubernetes.io/pod-deletion-cost` 注解介入排序。

### 2.4 StatefulSet 与 HPA 的调谐入口（对照）

| 控制器 | 调谐入口（实测） | 与 RS 的差异 |
|--------|----------------|-------------|
| StatefulSet | `pkg/controller/statefulset/stateful_set.go:524` `sync` | 有序（ordinal）、逐个推进、身份稳定 |
| HPA | `pkg/controller/podautoscaler/horizontal.go:773` `reconcileAutoscaler` | 定时驱动（非纯事件），经 metrics client 拉指标后改 `/scale` 子资源 |

工作负载行为语义详见 [[02-工作负载/01-核心工作负载/index.md|工作负载域：核心工作负载]]。

---

## 三、垃圾回收器：引用图与级联删除

GC 是唯一「面向全资源」的控制器，靠两个部件协作：

```go
// pkg/controller/garbagecollector/graph_builder.go:383
func (gb *GraphBuilder) Run(ctx context.Context) {
    // 用动态 Informer 监听所有可删除资源，
    // 把 ownerReference 维护成内存有向图 (uidToNode)
}

// pkg/controller/garbagecollector/garbagecollector.go:190
func (gc *GarbageCollector) Sync(ctx context.Context, discoveryClient ..., period time.Duration) {
    // 定期对比 discovery 资源列表，动态增删被监控的资源类型
}
```

删除传播的三种模式在源码中的落点：

| 模式 | 行为 | 实现要点 |
|------|------|---------|
| Background（默认） | 先删 owner，GC 异步删孤儿 | attemptToDeleteItem 扫描图中失去 owner 的节点 |
| Foreground | owner 挂 `foregroundDeletion` finalizer，等子级删完 | GC 负责在子级清空后摘除 finalizer |
| Orphan | owner 挂 `orphan` finalizer，GC 抹掉子级的 ownerRef | 子级保留、脱管 |

**生产陷阱**：跨 namespace 或 cluster-scoped 子级引用 namespaced owner 属非法引用，GC 会打 event 并可能直接删除子级——「资源被莫名删除」时先 `kubectl get events | grep OwnerRefInvalidNamespace`。机制详见 [[01-集群基础/02-设计原则/20-garbage-collection-owner-reference.md|GC 与 OwnerReference]]。

---

## 四、控制器通用骨架（复用清单）

阅读任何一个内置控制器时，可按此骨架快速定位：

```
NewXxxController()        # 注册 Informer EventHandler（AddFunc/UpdateFunc/DeleteFunc → enqueue）
  └── workqueue.NewTypedRateLimitingQueue(...)   # 默认指数退避 5ms→1000s + 令牌桶(10qps/100burst)
Run(workers, stopCh)
  └── WaitForCacheSync()  # ★ 缓存未同步前不启动 worker（否则会基于残缺状态误判）
  └── N × go wait.Until(worker)
worker → processNextWorkItem → syncHandler(key)
  ├── 成功: queue.Forget(key)
  └── 失败: queue.AddRateLimited(key)   # 排障时观察 workqueue_retries_total
```

关键可观测指标（Prometheus）：`workqueue_depth`（积压）、`workqueue_queue_duration_seconds`（排队延迟）、`workqueue_retries_total`（重试风暴）——三者是判断「控制器忙不过来还是调谐持续失败」的第一手证据，联动 [[09-可观测性/02-指标/index.md|可观测性域：指标]]。

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-core/01-source-tree-architecture.md|01 - 源码整体架构与目录结构]]
- [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|06 - 声明式 API 与 Informer 机制源码剖析]]
- [[10-平台工程/06-代码分析/kubernetes-core/07-component-interaction-dataflow.md|07 - 组件交互关系与数据流向]]
- [[10-平台工程/06-代码分析/deployment-create/README.md|代码分析：应用部署流程]]（操作视角）
- [[01-集群基础/03-控制平面/13-kube-controller-manager-deep-dive.md|控制平面：KCM Deep Dive]]
- [[02-工作负载/README.md|工作负载域]]
- [[01-集群基础/02-设计原则/13-operator-development-guide.md|Operator 开发指南]]（自定义控制器套用同一骨架）
