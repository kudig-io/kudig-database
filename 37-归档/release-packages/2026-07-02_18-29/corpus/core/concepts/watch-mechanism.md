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
- Real-time reconciliation in [[concepts/controller-pattern.md|[[Controller Pattern (Reconciliation Loop)|Controller Pattern]]]]
- Efficient state synchronization without polling
- Event-driven architecture where components react to state changes
- Horizontal scalability (each controller independently watches what it needs)

## Related

- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/controller-pattern.md|controller-pattern]] — Controller Pattern (Reconciliation Loop)
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[concepts/controller-pattern.md|Controller Pattern]]
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[etcd|etcd]]
- [[entities/kube-apiserver.md|kube-apiserver]]

- log.md|log]]

<!-- risk-assessed -->
