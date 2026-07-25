---
title: 垃圾回收与 ownerReference 级联删除机制
summary: 深入解析 Kubernetes Garbage Collector 的依赖图、级联删除状态机与 ownerReference 协同机制。
category: 设计原则
tags:
- garbage-collection
- owner-reference
- cascading-deletion
- finalizer
- controller
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 核心贡献者
estimated_read_time: 25min
intent_queries:
- Kubernetes 垃圾回收机制是什么
- ownerReference 级联删除三种策略区别
- Garbage Collector 依赖图如何工作
trigger_keywords:
- 垃圾回收
- 级联删除
- ownerReference
- Garbage Collector
- finalizer
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 19 - 垃圾回收与 ownerReference 级联删除机制 (Garbage Collection & Owner Reference)

## 1. 概述

Garbage Collector（下称 GC）是 Kubernetes 声明式 API 自动清理资源的基石。它让用户只关心"创建"，无需手动追踪"删除"——当某个 Owner 对象（如 Deployment、ReplicaSet）被删除时，依附于它的子资源（Pod、ConfigMap、Secret 等）会按既定策略被自动回收。

**一句话定义**：GC 是一个运行在 kube-controller-manager 内、基于 `ownerReference` 字段在内存中维护资源依赖有向图，并据此驱动级联删除（Cascading Deletion）的控制器。

### 1.1 GC 与 Finalizer 的边界

GC 与 Finalizer 都参与"删除前的清理"，但二者职责不同，常被混淆：

| 机制 | 触发依据 | 处理对象 | 谁来执行清理 | 典型场景 |
|------|----------|----------|--------------|----------|
| **Garbage Collector** | `metadata.ownerReference` 形成的父子树 | 系统内建的资源树（RS→Pod、Deployment→RS） | GC Controller 自动执行 | Deployment 删除后回收 ReplicaSet 与 Pod |
| **Finalizer** | `metadata.finalizers` 列表中的字符串 | 任意自定义清理逻辑（外部云资源、第三方系统） | 注册该 finalizer 的控制器执行 | Operator 删除外部数据库实例、Namespace 清空内部资源 |

二者并非互斥：Foreground 级联删除正是**通过 finalizer 与 GC 联动**实现的——apiserver 给 Owner 打上 `foregroundDeletion` finalizer，GC 负责删除全部依赖后再移除该 finalizer，最终允许对象真正消失。

> 本文是 [[01-集群基础/03-控制平面/13-kube-controller-manager-deep-dive.md|kube-controller-manager Deep Dive]] 第 2.6/3.6 节 GC 概述的**源码级补强**。概述文档介绍了三种策略与流程图，本文聚焦于 GC 控制器**内部机制**（依赖图、monotonic 队列、UID tracking、absent cache）、状态机源码与生产排障，避免重复入门内容。

## 2. 架构与工作原理

### 2.1 整体架构

GarbageCollectorController 由两大子系统组成：**GraphBuilder**（依赖图构建者）与 **GarbageCollector**（删除执行者）。GraphBuilder 通过对几乎所有资源建立 List-Watch（Informers），把 `ownerReference` 关系建模为内存中的有向图；当图发生变化时，把需要处理的节点投递到两条工作队列，由 GarbageCollector 的 worker 并发消费并真正发起 DELETE 请求。

```
                    kube-controller-manager 进程内的 GarbageCollectorController
 ┌──────────────────────────────────────────────────────────────────────────────────────┐
 │                                                                                      │
 │   apiserver (Discovery API + 所有 GVR 的 List/Watch)                                  │
 │           │                                                                          │
 │           ▼  (event: add/update/delete)                                              │
 │   ┌─────────────────────────────────────────────────────┐                            │
 │   │  monitors (per-GVR Informer + event handler)        │                            │
 │   │  controllerFor() → handler → graphChanges.Add(e)    │                            │
 │   └───────────────────────┬─────────────────────────────┘                            │
 │                           │                                                          │
 │                           ▼                                                          │
 │   ┌─────────────────────────────────────────────────────┐   single-threaded          │
 │   │  GraphBuilder                                       │   processGraphChanges()    │
 │   │  ┌─────────────────────────────────────────────┐    │   (图唯一的写入者)          │
 │   │  │ uidToNode  (UID → *node 的并发 map)         │    │                            │
 │   │  │   node { identity, owners, dependents,      │    │                            │
 │   │  │           beingDeleted, deletingDependents, │    │                            │
 │   │  │           virtual }                         │    │                            │
 │   │  └─────────────────────────────────────────────┘    │                            │
 │   │  graphChanges (workqueue, 生产者=monitors)          │                            │
 │   └───────────────┬──────────────────────┬──────────────┘                            │
 │                   │                      │                                            │
 │      enqueue      │                      │ enqueue                                    │
 │                   ▼                      ▼                                            │
 │      attemptToDelete ──────┐    attemptToOrphan ──────┐                              │
 │      (TypedRateLimiting    │    (TypedRateLimiting    │   消费者 = GarbageCollector   │
 │       Queue, N workers)    │     Queue, N workers)    │   attemptToDeleteWorker /    │
 │                           │                          │   attemptToOrphanWorker       │
 │                           ▼                          ▼                                │
 │           gc.attemptToDeleteItem()        gc.orphanDependents() + removeFinalizer    │
 │           ───────────────────────         ──────────────────────────────             │
 │           classifyReferences() 把 owner 分为                                            │
 │           solid / dangling / waitingForDependentsDeletion                              │
 │           → 决定 Background / Foreground / Orphan 策略                                 │
 │           → DELETE apiserver (带 propagationPolicy)                                    │
 │                                                                                        │
 │   ┌────────────────────────────────────────────────────────────────────────────┐      │
 │   │ absentOwnerCache (GraphBuilder 与 GC 共享的 LRU, 容量 500)                 │      │
 │   │ 记录"经 apiserver 确认不存在"的 owner，避免 worker 反复 GET 查不到的 owner  │      │
 │   └────────────────────────────────────────────────────────────────────────────┘      │
 └──────────────────────────────────────────────────────────────────────────────────────┘
                                                │
                                                ▼  PATCH/DELETE
                                      kube-apiserver → etcd
```

### 2.2 数据流关键点

1. **单一写入者原则**：图中 `node` 的依赖关系（`owners` / `dependents`）**仅由单线程的 `processGraphChanges()` 写入**，而读取者（`attemptToDeleteItem` 等多 worker）通过每字段的独立锁读取。这是源码注释中明确强调的并发模型。
2. **UID 是图的主键**：图中一切对象都以 `types.UID` 索引（`uidToNode`），而非 `namespace/name`。因为 `namespace/name` 可被新对象复用，只有 UID 才能稳定标识一个对象的生命周期。这也是级联删除可靠性的根基。
3. **两条队列职责分离**：`attemptToDelete` 处理"该被删除的节点"；`attemptToOrphan` 处理"需要切断 ownerReference 关系"的节点（对应 Orphan 策略）。

## 3. ownerReference 结构详解

### 3.1 字段语义

`ownerReference` 描述"我归属于谁"，每个字段都直接影响 GC 的行为：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nginx-7b4f5d8c9-abc12
  namespace: default
  uid: aaaa1111-aaaa-aaaa-aaaa-aaaaaaaaaaaa
  ownerReferences:
  - apiVersion: apps/v1
    kind: ReplicaSet
    name: nginx-7b4f5d8c9                 # owner 的 name（用于人类可读，GC 实际靠 uid）
    uid: 12345678-1234-1234-1234-123456789012   # ★ owner 的 UID，GC 真正依赖的主键
    controller: true                       # ★ 标识"谁是控制器"，决定依赖方向
    blockOwnerDeletion: true               # ★ 子资源是否阻塞 owner 的删除（触发 Foreground）
```

| 字段 | 作用 | 对 GC 的影响 |
|------|------|--------------|
| `apiVersion` / `kind` | owner 的 GVK | 用于构造 REST 请求、跨 GVR 查找 owner |
| `name` | owner 名称 | 仅用于日志/可读性，GC 以 `uid` 为准 |
| `uid` | owner 的全局唯一 ID | **图的主键**；UID 不匹配会被当作"已重建"，触发删除 |
| `controller: true` | 标记唯一的控制器 | 同一对象只能有一个 `controller: true`；决定由哪个控制器负责其生命周期 |
| `blockOwnerDeletion: true` | 子资源阻塞 owner 删除 | owner 删除时，若存在 `blockOwnerDeletion=true` 的依赖，apiserver 会注入 `foregroundDeletion` finalizer，强制走前台删除 |

### 3.2 controller=true 决定依赖方向

一个对象可以有多个 `ownerReference`（如一个 Pod 可能同时被 ReplicaSet 和某个自定义控制器引用），但**至多一个** `controller: true`。这个字段决定了"谁负责调谐我"。

GC 的 `classifyReferences()` 逻辑会把所有 owner 分为三类（见第 5 节源码走读），其中 `controller: true` 的 owner 是"主要"归属。如果一个 Pod 的 controller owner（RS）已经不存在、但另一个非 controller owner（如某自定义资源）仍存在，GC **不会**删除该 Pod——只要存在任一 solid owner 即保留。

### 3.3 blockOwnerDeletion 触发 Foreground

当用户删除 Owner（如 Deployment）时，apiserver 的删除逻辑会检查是否有依赖设置了 `blockOwnerDeletion: true`。若有，apiserver 会在 Owner 上注入 `foregroundDeletion` finalizer，使 Owner 进入"等待依赖删除完成"的状态，而非立即删除。这就是 Foreground 级联删除的起点。

> **注意**：`blockOwnerDeletion` 是子资源对父资源的声明。它要求子资源对 Owner 所在 namespace（或集群级）有 `update/finalize` 或 `block` 的 RBAC 权限；apiserver 会校验这一点，否则该字段会被静默忽略。

## 4. 三种级联删除策略对比

Kubernetes 提供三种删除策略，通过 `DELETE /api/.../{name}` 的 `propagationPolicy` 字段选择：

### 4.1 策略对比表

| 维度 | Foreground（前台） | Background（后台，默认） | Orphan（孤儿） |
|------|--------------------|--------------------------|----------------|
| **行为** | 先删所有依赖，最后删本体 | 直接删本体，异步 GC 依赖 | 删本体，保留依赖并切断关系 |
| **注入的 finalizer** | `foregroundDeletion` | 无 | `orphan` |
| **Owner 是否立即可见删除** | 进入 `deletingDependents` 状态（带 deletionTimestamp + finalizer） | 立即被删除（带 deletionTimestamp） | 立即被删除 |
| **依赖何时消失** | Owner 删除前同步完成 | Owner 删除后由 GC 异步删除 | 永不删除，仅 ownerReference 被移除 |
| **GC 队列** | `attemptToDelete`（先删依赖） | `attemptToDelete`（owner 删除后异步删依赖） | `attemptToOrphan`（移除依赖的 ownerReference） |
| **典型用途** | 严格保证删除顺序（先清子再清父） | 日常删除，性能优先 | 手动管理子资源生命周期 |
| **kubectl** | `--cascade=foreground` | `--cascade=background`（默认） | `--cascade=orphan` |
| **风险** | 删除慢，依赖删不完会卡住 | 本体先没，依赖延迟可见删除 | 子资源变成孤儿，需自行管理 |

### 4.2 三种 finalizer 的精确字符串

源码中（`staging/src/k8s.io/apimachinery/pkg/apis/meta/v1/types.go`）定义了两个内建 finalizer 常量：

```go
const (
    FinalizerOrphanDependents = "orphan"
    FinalizerDeleteDependents = "foregroundDeletion"
)
```

- **`foregroundDeletion`**：Foreground 策略注入。owner 删除时由 apiserver 添加，GC 在所有 `blockOwnerDeletion=true` 的依赖被删除后移除它，owner 才最终消失。
- **`orphan`**：Orphan 策略注入。owner 删除时由 apiserver 添加，GC（`attemptToOrphanWorker`）遍历依赖移除指向 owner 的 ownerReference，再移除该 finalizer。
- **Background**：**不注入任何 finalizer**——owner 直接被删除，GC 通过监听到 owner 的 deleteEvent 后，在依赖图中将 owner 标记为"已删除"，并把依赖加入 `attemptToDelete` 队列异步删除。

> 这解释了一个常见困惑：为什么 Background 删除时 Owner 很快消失，而 Foreground 删除时 Owner 长期卡在 `Terminating`？因为 Foreground 的 owner 携带 `foregroundDeletion` finalizer，在依赖未清空前无法真正删除。

### 4.3 选择策略的决策树

```
删除对象时选择策略:
│
├─ 是否需要"父先消失、子慢慢清"（性能优先，常见）？
│    └─ 是 → Background（默认，无需任何参数）
│
├─ 是否需要严格"先清子、再清父"（保证删除顺序）？
│    └─ 是 → Foreground（--cascade=foreground）
│            注意: 若有依赖删不掉（如 finalizer 卡住），Owner 会长期 Terminating
│
└─ 是否想保留子资源、只删父资源？
     └─ 是 → Orphan（--cascade=orphan）
             注意: 子资源会失去 owner，变成孤儿，需自行管理
```

## 5. GC 控制器内部机制（源码级）

> 本节基于 `源码/kubernetes-release-1.32/pkg/controller/garbagecollector/` 目录的真实源码。文件路径已标注。

### 5.1 内存依赖图：node 结构

依赖图的核心是 `node` 结构（`graph.go`）。它是 GC 一切判断的基础：

```go
// graph.go
type node struct {
    identity objectReference             // 本对象的坐标：GVK + namespace + name + UID
    dependentsLock sync.RWMutex
    dependents map[*node]struct{}        // ★ 谁依赖我（指向我的子资源）
    deletingDependents bool              // 是否带 FinalizerDeleteDependents（前台删除中）
    beingDeleted bool                    // deletionTimestamp 是否非空
    virtual bool                         // 是否"虚拟节点"（从未被 informer 真正观察到）
    owners []metav1.OwnerReference       // 我依赖谁（父资源列表）
}
```

关键设计：

- **`dependents` 是反向边**：GC 维护的是"谁依赖我"的反向索引。这样当 owner 被删除时，可以 O(1) 找到要级联删除的子资源。
- **`virtual` 节点**：当一个依赖引用了一个尚不存在的 owner 时，GraphBuilder 会创建一个 `virtual=true` 的占位节点（`addDependentToOwners`）。这个节点会被加入 `attemptToDelete`，由 worker 去 apiserver 验证 owner 是否真的不存在。
- **每字段独立锁**：`dependents`、`deletingDependents`、`beingDeleted`、`virtual` 各自有独立的 `sync.RWMutex`。因为写入者是单线程 `processGraphChanges()`，而读取者是多个 `attemptToDeleteItem` worker，细粒度锁避免读写互相阻塞。

### 5.2 GraphBuilder：图的唯一构建者

`GraphBuilder`（`graph_builder.go`）负责把 Informer 事件转换为图变更。它的结构：

```go
// graph_builder.go
type GraphBuilder struct {
    restMapper meta.RESTMapper
    monitors   monitors                  // 每个 GVR 一个 List/Watch Informer
    graphChanges workqueue.TypedRateLimitingInterface[*event]   // 输入队列（生产者=monitors）
    uidToNode *concurrentUIDToNode       // ★ UID → *node 的并发 map（图本体）
    attemptToDelete workqueue.TypedRateLimitingInterface[*node] // 输出队列1
    attemptToOrphan  workqueue.TypedRateLimitingInterface[*node] // 输出队列2
    absentOwnerCache *ReferenceCache     // 与 GC 共享
}
```

数据流：`monitor 事件 → graphChanges → processGraphChanges() → 更新 uidToNode + enqueue → attemptToDelete/attemptToOrphan`。

**`processGraphChanges()` 是图唯一的写入者**（单线程 `runProcessGraphChanges` 循环）。它处理三类事件：

```
processGraphChanges() 事件处理（简化）:
│
├─ add/update 且节点不存在 (found==false)
│    └─ insertNode(): 新建 node，addDependentToOwners() 注册反向边
│       processTransitions(): 若对象正在删除且带 FinalizerDeleteDependents
│         → 把依赖加入 attemptToDelete（前台删除启动）
│
├─ add/update 且节点已存在 (found==true)
│    └─ referencesDiffs(): 计算 ownerReferences 的 added/removed/changed
│       addUnblockedOwnersToDeleteQueue(): 检测被解除阻塞的 owner → attemptToDelete
│       更新 node.owners，重连反向边
│       若 beingDeleted() → markBeingDeleted()
│
└─ delete 事件
     └─ 若是 virtual delete（GC 自己产生的验证事件）:
        ├─ 依赖若认同该 owner 坐标 → absentOwnerCache.Add() + enqueue 依赖删除
        └─ 依赖坐标不一致 → 尝试用 alternate identity 替换虚拟节点，重新验证
        若是真实 delete → removeNode()，清理反向边
```

#### virtual 节点与坐标冲突的处理

这是 GC 最精巧也最难懂的部分。考虑这种竞态：

1. Pod A 的 ownerReference 指向 RS X（uid=u1，但 X 实际不存在或还没被观察到）。
2. GraphBuilder 创建一个 **virtual 节点**代表 X，并把它加入 `attemptToDelete`。
3. GC worker 去 apiserver GET X，发现不存在（或 UID 变了），产生 **virtual delete event** 回灌 `graphChanges`。
4. `processGraphChanges` 收到 virtual delete，但发现"有依赖以不同坐标引用 X"（例如另一个 Pod 引用了同名但不同 uid 的 RS），就不会贸然删除节点，而是尝试用 `getAlternateOwnerIdentity()` 找一个替代坐标重新验证。

这套机制确保：**即使 informer 事件乱序、UID 被复用，GC 也不会误删依赖**。代价是逻辑复杂、需要 absent cache 配合。

### 5.3 absentOwnerCache：避免无效查询

```go
// uid_cache.go
type ReferenceCache struct { cache *lru.Cache }   // 容量 500

func (c *ReferenceCache) Add(reference objectReference) { c.cache.Add(reference, nil) }
func (c *ReferenceCache) Has(reference objectReference) bool { ... }
```

`absentOwnerCache` 是一个 **LRU 缓存（容量 500）**，记录"已被 apiserver 确认不存在"的 owner 坐标。它的作用：

- 当 worker 处理一个依赖时，先查 cache：若 owner 已知不存在，直接进入删除流程，**跳过一次对 apiserver 的 GET**。
- 这在大量孤儿资源场景下显著降低 apiserver 压力（避免每个子资源都去 GET 一次已删除的 owner）。
- **GraphBuilder 与 GarbageCollector 共享**同一个 cache 实例（构造时传入同一个 `*ReferenceCache`）。

### 5.4 attemptToDelete / attemptToOrphan：monotonic、rate-limited 队列

两条队列都是 `workqueue.TypedRateLimitingInterface[*node]`：

| 队列 | 生产者 | 消费者 | 处理内容 |
|------|--------|--------|----------|
| `attemptToDelete` | GraphBuilder | `attemptToDeleteWorker`（多 worker 并发） | 删除"应被回收"的节点（Background/Foreground） |
| `attemptToOrphan` | GraphBuilder | `attemptToOrphanWorker`（多 worker 并发） | 移除依赖的 ownerReference（Orphan） |

特性：

- **Rate-limited**：失败重试有指数退避（`NewTypedRateLimitingQueue`），避免对 apiserver 形成删除风暴。
- **Monotonic 去重**：同一个 `*node` 在队列中至多一份（workqueue 内置去重），避免重复处理。
- **worker 数量**：由 kube-controller-manager 的 `--concurrent-gc-syncs`（默认 20）控制。

### 5.5 attemptToDeleteItem 状态机（核心源码）

这是 GC 删除决策的灵魂（`garbagecollector.go`）。`attemptToDeleteItem(ctx, item)` 的核心逻辑是 **owner 分类 + 策略选择**：

```go
// garbagecollector.go（简化展示核心分支）
func (gc *GarbageCollector) attemptToDeleteItem(ctx context.Context, item *node) error {
    // 1. 若对象已在删除中且不在 deletingDependents，直接返回（等最终删除）
    if item.isBeingDeleted() && !item.isDeletingDependents() { return nil }

    // 2. 从 apiserver 取最新对象，校验 UID（防 UID 复用误删）
    latest, err := gc.getObject(item.identity)
    if errors.IsNotFound(err) {
        gc.dependencyGraphBuilder.enqueueVirtualDeleteEvent(item.identity)  // 灌回 virtual delete
        return enqueuedVirtualDeleteEventErr
    }
    if latest.GetUID() != item.identity.UID { /* UID 变了，按已删除处理 */ }

    // 3. 若对象正在 deletingDependents（前台删除中）→ 检查阻塞依赖
    if item.isDeletingDependents() {
        return gc.processDeletingDependentsItem(logger, item)  // 无阻塞依赖则移除 foregroundDeletion finalizer
    }

    // 4. 分类 ownerReferences：solid / dangling / waitingForDependentsDeletion
    ownerReferences := latest.GetOwnerReferences()
    if len(ownerReferences) == 0 { return nil }   // 根对象，GC 不处理
    solid, dangling, waitingForDependentsDeletion, _ := gc.classifyReferences(ctx, item, ownerReferences)

    switch {
    case len(solid) != 0:
        // 有存活的 owner → 不删除；但清理 dangling/waiting 的失效引用
        // PATCH 移除悬空 ownerReference
        ...
    case len(waitingForDependentsDeletion) != 0 && item.dependentsLength() != 0:
        // owner 正在等依赖删除，且自己有依赖 → 自己也走前台删除（传递前台语义）
        // （含循环检测：若依赖也在 deletingDependents，先解除阻塞再删，防死锁）
        return gc.deleteObject(item.identity, &metav1.DeletePropagationForeground)
    default:
        // 无任何 solid owner → 该被 GC。按现有 finalizer 决定策略：
        var policy metav1.DeletionPropagation
        switch {
        case hasOrphanFinalizer(latest):         policy = metav1.DeletePropagationOrphan
        case hasDeleteDependentsFinalizer(latest): policy = metav1.DeletePropagationForeground
        default:                                  policy = metav1.DeletePropagationBackground
        }
        return gc.deleteObject(item.identity, &policy)
    }
}
```

#### owner 三分类语义

`classifyReferences()` 把对象的每个 ownerReference 归入三类：

| 分类 | 含义 | GC 行为 |
|------|------|---------|
| **solid**（坚实的） | owner 在图中存在且**未在删除中** | 保留对象，不删除 |
| **dangling**（悬空的） | owner 在图中**不存在**（已被删除或从未存在） | 该引用是失效的，需 PATCH 移除 |
| **waitingForDependentsDeletion**（等待依赖删除） | owner 存在但**正在前台删除中**（带 `foregroundDeletion`） | 该引用也应被移除（否则对象会永远卡住） |

决策规则总结：
- **有任何 solid owner** → 保留对象，只清理 dangling/waiting 的失效引用。
- **无 solid owner，但有 waitingForDependentsDeletion owner 且自己有依赖** → 自己也走 Foreground（前台语义向下游传递）。
- **否则** → 该对象该被删除，策略由其自身 finalizer 决定（无 finalizer 则 Background）。

### 5.6 attemptToOrphanWorker：Orphan 的两步提交

```go
// garbagecollector.go
func (gc *GarbageCollector) attemptToOrphanWorker(logger klog.Logger, item interface{}) workQueueItemAction {
    owner := item.(*node)
    dependents := owner.getDependents()
    // 第1步：并发 PATCH 所有依赖，移除指向 owner 的 ownerReference
    err := gc.orphanDependents(logger, owner.identity, dependents)
    if err != nil { return requeueItem }
    // 第2步：移除 owner 的 orphan finalizer（此时依赖关系已断，可安全放行删除）
    err = gc.removeFinalizer(logger, owner, metav1.FinalizerOrphanDependents)  // "orphan"
    if err != nil { return requeueItem }
    return forgetItem
}
```

注意 `orphanDependents` 用 `dependent.identity.UID` 作为 PATCH 前置条件（precondition），确保不会误改一个 UID 已被复用的新对象。

### 5.7 乐观并发：resourceVersion 处理竞争

GC 的所有写操作（PATCH/DELETE）都带 `resourceVersion` 或 UID precondition。源码中 `gc.patch()` 封装了重试逻辑：

```go
// patch.go（核心思想）
func (gc *GarbageCollector) patch(item *node, patch []byte, ...) (...) {
    // 调用前已用 UID 作 precondition；若 apiserver 返回 409/PreconditionFailed
    // → 重新 getObject 取最新版本，重新生成 patch，重试
}
```

这保证了：即使多个 GC worker（或外部控制器）同时修改同一对象，也不会产生 lost-update。失败的 PATCH 会被重新加入队列重试。

## 6. 删除时序：apiserver 收到 DELETE 之后

下图展示一次 Foreground 删除的完整时序（Background/Orphan 是其子集）：

```
Client                  kube-apiserver              etcd          GC Controller           其他控制器
  │                          │                         │                  │                       │
  │ DELETE /deployments/web  │                         │                  │                       │
  │  propagationPolicy=      │                         │                  │                       │
  │  Foreground              │                         │                  │                       │
  ├─────────────────────────▶│                         │                  │                       │
  │                          │                         │                  │                       │
  │                          │ 1. Authn/Authz          │                  │                       │
  │                          │ 2. admission webhook    │                  │                       │
  │                          │ 3. 写 deletionTimestamp  │                  │                       │
  │                          │    注入 foregroundDeletion finalizer       │                       │
  │                          │───────写对象───────────▶│                  │                       │
  │                          │                         │                  │                       │
  │                          │ 4. 返回 200（对象带     │                  │                       │
  │◀─────────────────────────│    deletionTimestamp）  │                  │                       │
  │                          │                         │                  │                       │
  │                          │ 5. Watch 推送 Update 事件                  │                       │
  │                          │──────────────────────────────────────────▶│                       │
  │                          │                         │  GraphBuilder 收到 Update:                │
  │                          │                         │   markBeingDeleted + markDeletingDependents│
  │                          │                         │   把所有依赖(RS)加入 attemptToDelete      │
  │                          │                         │                  │                       │
  │                          │                         │  worker 删除 RS:  │                       │
  │                          │   DELETE /replicasets/x │                  │                       │
  │                          │◀──────────────────────────────────────────│                       │
  │                          │   RS 删除 → 其 Pod 依赖被 enqueue          │                       │
  │                          │                         │  worker 删 Pod...│                       │
  │                          │   (递归直到叶子)        │                  │                       │
  │                          │                         │                  │                       │
  │                          │ 6. 所有 blockOwnerDeletion=true 依赖已删   │                       │
  │                          │    GC 移除 owner 的 foregroundDeletion finalizer                  │
  │                          │   PATCH /deployments/web (移除 finalizer) │                       │
  │                          │◀──────────────────────────────────────────│                       │
  │                          │                         │                  │                       │
  │                          │ 7. finalizer 清空 → 对象真正从 etcd 删除   │                       │
  │                          │───────DELETE───────────▶│                  │                       │
  │                          │                         │                  │                       │
  │                          │ 8. 若有 deletionGracePeriodSeconds         │                       │
  │                          │    则等待 grace 期满才真正物理删除          │                       │
```

关键阶段说明：

1. **deletionTimestamp 写入**：apiserver 收到 DELETE 后，先置 `metadata.deletionTimestamp`（并把 `deletionGracePeriodSeconds` 设为 grace），对象进入"逻辑删除中"。
2. **finalizer 注入**：根据 propagationPolicy 注入 `foregroundDeletion` 或 `orphan` finalizer（Background 不注入）。此时对象因带 finalizer，**不会真正消失**。
3. **Watch 推送**：Update 事件推给 GC 的 monitor。
4. **GC 接管**：GraphBuilder 标记 `beingDeleted`/`deletingDependents`，把依赖加入 `attemptToDelete`。
5. **递归删除**：worker 按 Background/Foreground 策略递归删除依赖（依赖的依赖也会被处理）。
6. **finalizer 移除**：阻塞依赖清空后，GC PATCH 移除 `foregroundDeletion` finalizer。
7. **物理删除**：finalizer 为空 → apiserver 从 etcd 真正删除对象。
8. **grace period**：`deletionGracePeriodSeconds` 控制从置 timestamp 到物理删除的宽限期（Pod 默认 30s，用于 kubelet 优雅终止）。

## 7. Finalizer 与 GC 的协作

### 7.1 系统内建 finalizer 全景

| Finalizer 字符串 | 注入者 | 移除者 | 触发条件 |
|------------------|--------|--------|----------|
| `foregroundDeletion` | apiserver（Foreground 删除时） | GC（所有 `blockOwnerDeletion` 依赖删除后） | `propagationPolicy=Foreground` |
| `orphan` | apiserver（Orphan 删除时） | GC（`attemptToOrphanWorker` 移除依赖 ownerReference 后） | `propagationPolicy=Orphan` |
| `kubernetes` | Namespace Controller 相关逻辑 | Namespace Controller（namespace 内资源清空后） | Namespace 删除 |
| `kubernetes.io/pv-protection` | PV Controller | PV Controller（防止 PV 被误删，等 PVC 解绑） | PersistentVolume 删除 |
| `kubernetes.io/pvc-protection` | PVC Controller | PVC Controller（防止 PVC 在 Pod 使用中被删） | PersistentVolumeClaim 删除 |
| `kubernetes.io/service-account-token` | Token Controller | Token Controller（清理关联的 Secret） | ServiceAccount 删除 |

### 7.2 CRD / Namespace / PV 的 finalizer 示例

**CRD（自定义资源）的 Operator finalizer**：Operator 在创建 CR 时注入自己的 finalizer，确保删除 CR 时能先清理外部资源（如云数据库）：

```yaml
apiVersion: example.com/v1
kind: Database
metadata:
  name: my-db
  finalizers:
  - example.com/db-cleanup    # Operator 自定义
  deletionTimestamp: "2026-07-23T10:00:00Z"   # 用户 kubectl delete 后出现
```

Operator 的 controller 监听到 deletionTimestamp 后，执行外部清理，完成后 PATCH 移除该 finalizer，CR 才会真正删除。**这与 GC 互不干扰**：GC 管 ownerReference 树，Operator finalizer 管外部副作用。

**Namespace 删除的级联**：Namespace 是特殊的集群级 owner，其下所有命名空间级资源都隐式以 Namespace 为 owner。删除 Namespace 时，NamespaceController 会逐个清理内部资源——这部分并非由 GC 主导，但 GC 也会配合清理 ownerReference 指向已删 Namespace 的对象。

**PV 的保护**：`kubernetes.io/pv-protection` 防止 PV 在还有 PVC 引用时被直接删除，给存储控制器时间解绑。

### 7.3 Finalizer 卡住的典型现象

当 finalizer 对应的控制器故障（如 Operator 宕机、RBAC 权限缺失）时，对象会**永久卡在 Terminating**：

```
$ kubectl get ns stuck-ns
NAME       STATUS        AGE
stuck-ns   Terminating   2d
```

此时 `.metadata.deletionTimestamp` 非空且 `.metadata.finalizers` 非空，但对应控制器迟迟不移除 finalizer。这是生产中最常见的"删不掉"问题（见第 8 节排障）。

## 8. 生产排障

### 8.1 诊断：对象卡在 Terminating

按以下顺序排查（从无害到有害）：

```bash
# 🟢 低风险：查看对象是否带 deletionTimestamp（非空 = 正在删除）
kubectl get pod <name> -o jsonpath='{.metadata.deletionTimestamp}{"\n"}'
# 输出示例: 2026-07-23T10:00:00Z   ← 非空说明已进入删除流程

# 🟢 低风险：查看 ownerReferences，确认归属关系是否正常
kubectl get pod <name> -o jsonpath='{.metadata.ownerReferences}' | jq .

# 🟢 低风险：查看 finalizers，找出阻塞删除的 finalizer
kubectl get pod <name> -o jsonpath='{.metadata.finalizers}' | jq .
# 若输出 ["foregroundDeletion"] → 等 GC 删依赖
# 若输出 ["example.com/my-finalizer"] → 等 Operator 移除
```

### 8.2 诊断：子资源泄漏（Orphan 残留）

删除 Deployment 后 Pod/RS 残留，通常是 GC 未正常工作或 ownerReference 丢失：

```bash
# 🟢 低风险：列出无 owner 的孤儿 Pod
kubectl get pods -A -o json | jq '.items[] | select(.metadata.ownerReferences == null) | .metadata.name'

# 🟢 低风险：检查 RS 是否仍指向已删除的 Deployment
kubectl get rs <name> -o jsonpath='{.metadata.ownerReferences}' | jq .

# 🟢 低风险：查看 KCM 是否启用了 GC（绝不应为 false）
# 在 kube-controller-manager 的启动参数中确认 --enable-garbage-collector=true（默认）
ps aux | grep kube-controller-manager | grep -o 'enable-garbage-collector=[a-z]*'
```

### 8.3 高风险操作：手动移除 finalizer（仅排障）

当确认对应控制器已彻底失效、且理解后果后，可手动清空 finalizer 强制删除对象。**这会跳过所有清理逻辑，可能留下悬空的外部资源**：

```bash
# 🔴 高风险：手动清空 finalizer，强制让对象被删除（跳过清理逻辑！）
# 仅用于：对应控制器已永久故障、且你已确认无外部资源需要清理
kubectl patch <resource>/<name> -n <ns> --type=merge -p '{"metadata":{"finalizers":[]}}'

# 🔴 高风险：对 Namespace 卡在 Terminating 的最后手段
# 需先 kubectl get ns <ns> -o json > ns.json，把 finalizers 改为 [] 后：
# kubectl replace --raw "/api/v1/namespaces/<ns>/finalize" -f ns.json
```

> **严禁**在生产环境随意 patch finalizer。这会导致 GC/Operator 跳过清理，常见后果：云盘未释放、外部账单持续计费、命名空间"幽灵"资源。务必先确认 finalizer 的归属控制器与清理职责。

### 8.4 Prometheus 指标监控

GC 暴露的关键指标（来自 `/metrics`，前缀 `garbage_collector_`）：

| 指标 | 含义 | 告警建议 |
|------|------|----------|
| `garbage_collector_attempt_to_delete_total` | attemptToDelete 处理总数 | 关注速率异常飙升 |
| `garbage_collector_attempt_to_orphan_total` | attemptToOrphan 处理总数 | — |
| `garbage_collector_graph_changes_queue_length`（workqueue 指标） | graphChanges 队列深度 | 持续 > 0 且不收敛 → GC 跟不上事件 |
| `workqueue_depth{名字含 garbagecollector}` | attemptToDelete/attemptToOrphan 深度 | 长期高位 → 删除积压 |
| `workqueue_retries_total` | 重试次数 | 飙升 → apiserver 拒绝删除（RBAC/冲突） |

Prometheus 告警示例：

```yaml
# GC 队列积压（删除跟不上）
- alert: GarbageCollectorQueueDepthHigh
  expr: histogram_quantile(0.95, sum(rate(workqueue_depth{name=~"garbagecollector.*"}[5m])) by (name, le)) > 100
  for: 10m
  annotations:
    summary: "GC 工作队列深度持续过高 ({{ $value }})，删除可能积压"

# GC 删除持续失败（重试率高）
- alert: GarbageCollectorRetriesHigh
  expr: rate(workqueue_retries_total{name=~"garbagecollector.*"}[5m]) > 1
  for: 5m
  annotations:
    summary: "GC 重试率高，检查 apiserver RBAC 或对象冲突"
```

### 8.5 KCM 日志关键字

```bash
# 🟢 低风险：抓取 GC 相关日志（leader 所在 KCM 节点）
# 找 leader：
kubectl -n kube-system get lease kube-controller-manager -o jsonpath='{.spec.holderIdentity}'
# 然后在该节点：
journalctl -u kube-controller-manager | grep -iE "garbage collector|attemptToDelete|attemptToOrphan"

# 常见错误模式：
# E0101 ... gc_controller.go:... garbage collector: error getting object for gvk xxx
#   → 某资源类型无法访问（CRD 已删但 GC 还在引用，或 RBAC 不足）
# E0101 ... garbage collector: error deleting object ... forbidden
#   → GC 缺少删除权限
```

### 8.6 绝不可关闭 GC

```bash
# 🔴 高风险：以下参数绝不应在生产环境设置为 false
#   --enable-garbage-collector=false
#
# 危害：多 master 集群中若部分 KCM 关闭 GC、部分开启，
#   关闭的实例不维护依赖图，会导致级联删除不一致、资源泄漏甚至数据损坏。
#   官方明确警告：必须所有实例同时开启或同时关闭，且关闭后无任何级联删除保障。
```

`--concurrent-gc-syncs`（默认 20）控制 attemptToDelete/attemptToOrphan 的 worker 数。大集群（>500 节点或大量 CRD）可调至 30-50，但需同步调高 KCM 的 `--kube-api-qps/--kube-api-burst`，否则 worker 会因限流空转。

## 9. 最佳实践

### 9.1 ownerReference 的使用纪律

| 实践 | 说明 | 原因 |
|------|------|------|
| **不要手工写 ownerReference** | 由创建资源的控制器自动设置 | 手工设置的 uid 极易过期/错误，导致 GC 误判 |
| **Operator 用 SetControllerReference** | controller-runtime 提供 `ctrl.SetControllerReference(owner, obj, scheme)` | 自动填正确 uid、设 controller=true、处理更新 |
| **跨 namespace 不信任** | namespace 级资源不能以另一 namespace 的资源为 owner | apiserver 校验：只有集群级资源才能跨 namespace 作为 owner |
| **集群级资源可作 owner** | Node/PV/ClusterRole 等可被任意 namespace 资源引用 | 这是合法的跨域 ownerReference |

### 9.2 controller-runtime 的 SetControllerReference 示例

```go
// Go (controller-runtime) — Operator 创建子资源时正确设置 ownerReference
import (
    "sigs.k8s.io/controller-runtime/pkg/controller/controllerutil"
)

// reconcile 中创建 ConfigMap，归属当前 CR
cm := &corev1.ConfigMap{ObjectMeta: metav1.ObjectMeta{Name: "app-cm", Namespace: ns}}
if err := controllerutil.SetControllerReference(myCR, cm, r.Scheme); err != nil {
    return err
}
// 此时 cm.metadata.ownerReferences 会自动包含:
//   {kind: MyCR, uid: <myCR 的真实 uid>, controller: true, blockOwnerDeletion: true}
```

`SetControllerReference` 内部会：从 scheme 取 GVK、用 owner 的真实 uid、设 `controller: true`。当 CR 被删除时，GC 会自动级联删除这个 ConfigMap。

### 9.3 删除策略的选择建议

| 场景 | 推荐策略 | 理由 |
|------|----------|------|
| 日常删除 Deployment/Service | Background（默认） | 性能优先，本体先消失，依赖异步清 |
| 需要严格顺序的删除（如先停 Pod 再删 ConfigMap） | Foreground | 保证父删之前子已清完 |
| 迁移资源归属、保留子资源 | Orphan | 子资源保留，仅断关系 |
| 删除 Namespace | 不指定（NamespaceController 主导） | Namespace 有专用级联逻辑 |

### 9.4 避免 finalizer 卡住的工程实践

1. **Operator 必须幂等处理 finalizer**：即使外部清理已部分完成，重复 reconcile 也应安全。
2. **finalizer 名字用域名前缀**（`example.com/cleanup`）：避免与系统或其他 Operator 冲突。
3. **给 Operator 配 Leader Election**：保证任意时刻有实例在处理 finalizer，否则对象会卡住。
4. **监控 `deletionTimestamp` 长期非空的对象**：这是 finalizer 卡住的早期信号。

## 10. 常见误区澄清

| 误区 | 真相 |
|------|------|
| "GC 会删除没有 ownerReference 的根对象" | ❌ GC 只处理"owner 不存在"的依赖。根对象（无 ownerReference）GC 从不主动删除 |
| "Background 删除会给 Owner 加 finalizer" | ❌ Background 不加任何 finalizer，Owner 直接删除，依赖异步回收 |
| "UID 相同就是同一个对象" | ❌ 对象删除后 uid 被复用极罕见但可能。GC 用 `latest.GetUID() != item.identity.UID` 校验，不匹配即视为已删 |
| "手工 patch ownerReference 没风险" | ❌ 手工写错 uid 会让 GC 把对象当孤儿删除，或反之保留泄漏 |
| "关闭 GC 能提升性能" | ❌ 关闭 GC 会导致资源泄漏、多 master 不一致，官方明确禁止 |
| "Foreground 比 Background 更安全" | ⚠️ 不绝对：Foreground 若有依赖删不掉（finalizer 卡住），Owner 会永久 Terminating |

## 11. 相关文档

- [[01-集群基础/03-控制平面/13-kube-controller-manager-deep-dive.md|kube-controller-manager Deep Dive]] — GC 控制器概述入口（第 2.6/3.6 节含三种策略表格与级联删除流程图）
- [[01-集群基础/02-设计原则/02-declarative-api-pattern.md|声明式 API]] — 声明式 API 与面向终态设计，Finalizer 与 ownerReference 的入门概念
- [[01-集群基础/02-设计原则/12-operator-development-guide.md|Operator 开发指南]] — Operator 中 SetControllerReference 与 finalizer 的工程实践
- [[01-集群基础/02-设计原则/03-controller-pattern.md|控制器模式]] — 控制器模式与调谐循环，GC 是典型的 controller 实现
- [[01-集群基础/02-设计原则/05-informer-workqueue.md|Informer 架构与工作队列]] — GC 的 GraphBuilder 基于 Informer + workqueue 构建，原理同此
- [[01-集群基础/02-设计原则/06-resource-version-control.md|资源版本与并发控制]] — GC 乐观并发（resourceVersion precondition）的底层机制

## See Also

- 02-declarative-api-pattern
- 03-controller-pattern
- 05-informer-workqueue
- 06-resource-version-control
- 12-operator-development-guide

## Related

- [[01-集群基础/03-控制平面/13-kube-controller-manager-deep-dive.md|kube-controller-manager Deep Dive]]
- [[01-集群基础/02-设计原则/02-declarative-api-pattern.md|声明式 API]]
- [[01-集群基础/02-设计原则/12-operator-development-guide.md|Operator 开发指南]]

<!-- risk-assessed -->
