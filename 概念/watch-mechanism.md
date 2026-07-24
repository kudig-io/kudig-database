---
title: Watch Mechanism (List-Watch)
description: Watch Mechanism (List-Watch) — Kubernetes 生产运维知识库
summary: Watch Mechanism (List-Watch) — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- watch
- list
- informer
- event-driven
- etcd
- apiserver
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Watch Mechanism (List-Watch) 是什么
- 如何 Watch Mechanism (List-Watch)
trigger_keywords:
- Watch
- Mechanism
- List-Watch
prerequisites:
- kubectl-basics
- etcd-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Watch Mechanism (List-Watch)

## How It Works

The List-Watch mechanism is the backbone of Kubernetes's event-driven architecture:

1. **List**: Client fetches the full resource list from API Server, records the latest `resourceVersion`
2. **Watch**: Client opens an HTTP chunked streaming connection, passing `?watch=true&resourceVersion=N`
3. **Event Stream**: API Server watches etcd and pushes events (ADDED, MODIFIED, DELETED, BOOKMARK) to the client
4. **Cache Update**: Client (typically Informer) updates local cache and triggers event handlers

## Key Parameters

| Parameter | Purpose | Example |
|-----------|---------|---------|
| `watch=true` | Enable streaming watch | `?watch=true` |
| `resourceVersion` | Start watching from revision | `?resourceVersion=1000` |
| `timeoutSeconds` | Connection timeout | `?timeoutSeconds=600` |
| `allowWatchBookmarks` | Allow bookmark events | `?allowWatchBookmarks=true` |

## Event Types

- **ADDED**: New resource created
- **MODIFIED**: Resource spec or status changed
- **DELETED**: Resource removed
- **BOOKMARK**: Keep-alive signal with current resourceVersion (no data change)
- **ERROR**: Watch stream broken (e.g., resourceVersion too old due to etcd compaction)

## Reconnection Logic

When a Watch connection breaks, the client reconnects using its last known `resourceVersion`. If that version has been compacted by etcd, the client performs a full List to re-sync, then starts a new Watch.

## Why It Matters

Watch enables:
- Real-time reconciliation in [[概念/controller-pattern.md|[[Controller Pattern (Reconciliation Loop)|Controller Pattern]]]]
- Efficient state synchronization without polling
- Event-driven architecture where components react to state changes
- Horizontal scalability (each controller independently watches what it needs)

## 源码实现分析

### Informer 架构

```go
// k8s.io/client-go/tools/cache/shared_informer.go
type SharedInformer struct {
    // 1. Reflector: 执行 List-Watch
    reflector *Reflector
    // 2. DeltaFIFO: 事件队列
    fifo *DeltaFIFO
    // 3. Indexer: 本地缓存 + 索引
    indexer Indexer
    // 4. ProcessLoop: 分发事件到 Handler
    processors []processorListener
}

// Reflector 核心循环
func (r *Reflector) ListAndWatch(stopCh <-chan struct{}) {
    // 1. List: 获取全量数据 + resourceVersion
    list, rv := r.listerWatcher.List()
    r.store.Replace(list, rv)
    
    // 2. Watch: 从 rv 开始监听增量
    w, _ := r.listerWatcher.Watch(rv)
    for event := range w.ResultChan() {
        r.store.Update(event)  // 更新 DeltaFIFO
    }
    // 3. 断连后重新 List-Watch
}
```

### Watch 连接生命周期

```
Client (Informer)                    API Server                    etcd
    │                                    │                          │
    │─── List(rv=0) ────────────────▶│─── Range ─────────────▶│
    │◀── 全量数据 + rv=1000 ────────│◀── 结果 ────────────────│
    │                                    │                          │
    │─── Watch(rv=1000) ─────────────▶│─── Watch ─────────────▶│
    │◀── ADDED/MODIFIED/DELETED ─────│◀── 事件流 ─────────────│
    │◀── BOOKMARK(rv=1050) ─────────│   (心跳保活)          │
    │                                    │                          │
    │─── 连接超时/断开 ─────────────▶│                          │
    │─── Watch(rv=1050) 重连 ────────▶│─── 继续监听 ──────────▶│
    │                                    │                          │
    │─── rv 已压缩 (410 Gone) ───────▶│                          │
    │─── List(rv=0) 重新同步 ────────▶│─── Range ─────────────▶│
```

## 源码实现分析

### client-go Informer 实现

```go
// k8s.io/client-go/tools/cache/reflector.go
// Reflector 执行 List-Watch 循环
func (r *Reflector) ListAndWatch(stopCh <-chan struct{}) error {
    // 1. List 获取全量数据
    list, err := r.listerWatcher.List(metav1.ListOptions{
        ResourceVersion: "0",  // 从 etcd 缓存读取
    })
    resourceVersion := list.GetResourceVersion()
    
    // 2. 同步到本地缓存 (DeltaFIFO)
    r.syncWith(list.Items, resourceVersion)
    
    // 3. Watch 增量事件
    w, err := r.listerWatcher.Watch(metav1.ListOptions{
        ResourceVersion: resourceVersion,  // 从上次位置继续
        AllowWatchBookmarks: true,
    })
    
    // 4. 处理事件流
    for event := range w.ResultChan() {
        switch event.Type {
        case watch.Added, watch.Modified, watch.Deleted:
            r.store.Update(event)  // 更新本地缓存
        case watch.Bookmark:
            resourceVersion = event.Object.GetResourceVersion()
        case watch.Error:
            if isGone(event) {
                return errorRetryLater  // 410 Gone: 重新 List
            }
        }
    }
}
```

### List-Watch 架构流程

```
┌───────────────────────────────────────────────────────────┐
│          List-Watch 架构流程                          │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  Controller (Deployment/ReplicaSet/...)                  │
│    │                                                      │
│    ▼                                                      │
│  SharedInformerFactory (共享 Watch 连接)                │
│    │                                                      │
│    ▼                                                      │
│  Reflector: List(rv=0) + Watch(rv=N)                    │
│    │                                                      │
│    ▼                                                      │
│  DeltaFIFO: 事件队列 (Added/Modified/Deleted)          │
│    │                                                      │
│    ▼                                                      │
│  Indexer: 本地缓存 + 索引 (thread-safe store)         │
│    │                                                      │
│    ▼                                                      │
│  WorkQueue: 限速重试队列                               │
│    │                                                      │
│    ▼                                                      │
│  Reconcile: 对比期望/实际状态，执行修复              │
│                                                           │
│  关键优化:                                               │
│  • SharedInformer: 多控制器共享一个 Watch             │
│  • rv=0 List: 从 apiserver 缓存读，不打 etcd        │
│  • BOOKMARK: 心跳保持 rv 新鲜，避免 410 Gone      │
└───────────────────────────────────────────────────────────┘
```

### 观察 Watch 连接状态（🟢 只读）

```bash
# 查看 apiserver 当前 Watch 连接数
kubectl get --raw='/metrics' | grep apiserver_watch_events_sizes

# 查看 etcd Watch 状态
kubectl exec -n kube-system etcd-master -- etcdctl watch --rev=1 --prefix /registry/pods --count-only

# 检查 Informer 同步状态
kubectl get --raw='/metrics' | grep workqueue_depth
# workqueue_depth > 0 表示有积压事件未处理
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| Watch 是轮询 | Watch 是服务端推送，不是客户端轮询 |
| 事件不会丢失 | 断连期间事件可能丢失，需 re-list |
| BOOKMARK 是数据事件 | BOOKMARK 仅是心跳，携带最新 rv |
| resourceVersion 是时间戳 | 它是 etcd revision，单调递增整数 |
| 每个控制器独立 Watch | SharedInformer 共享一个 Watch 连接 |

## 面试要点

1. **List-Watch 机制是如何工作的？**
   - List 获取全量 + resourceVersion
   - Watch 从该 rv 开始接收增量事件
   - 断连后从最后已知 rv 重连
   - rv 过期则重新 List

2. **Informer 的组件有哪些？**
   - Reflector: 执行 List-Watch
   - DeltaFIFO: 事件队列
   - Indexer: 本地缓存 + 索引
   - WorkQueue: 限速重试队列

3. **为什么用 Watch 而不是轮询？**
   - 实时性: 事件立即推送
   - 效率: 无冗余请求
   - 可扩展: 服务端维护连接状态

4. **410 Gone 错误如何处理？**
   - etcd compaction 删除了旧版本
   - 客户端必须重新 List 获取全量
   - 然后从新 rv 开始 Watch

## Related

- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/controller-pattern.md|controller-pattern]] — Controller Pattern (Reconciliation Loop)
- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[概念/controller-pattern.md|Controller Pattern]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[etcd|etcd]]
- [[实体/kube-apiserver.md|kube-apiserver]]

- log.md|log]]

<!-- risk-assessed -->
