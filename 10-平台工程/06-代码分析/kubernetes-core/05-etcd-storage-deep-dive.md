---
title: etcd 与 Kubernetes 存储链路源码剖析
description: 基于 etcd-3.7.0 与 kubernetes-1.36.2 源码的 Raft 提交、MVCC 存储、Watch 通知与 K8s 存储编码链路完整剖析
summary: 从 apiserver etcd3 客户端出发，剖析 EtcdServer.Put→Raft 共识→MVCC(treeIndex+boltdb)→watchableStore 通知的全链路，覆盖 compaction、defrag、resourceVersion 语义，全部函数附实测行号。
category: source-analysis
tags:
- k8s
- source-code
- etcd
- raft
- mvcc
- boltdb
- watch
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
- etcd 一次写入的完整流程源码
- etcd MVCC treeIndex boltdb 关系
- resourceVersion 与 etcd revision 的对应关系
- etcd Watch 机制如何通知 apiserver
trigger_keywords:
- etcd
- raft
- mvcc
- treeIndex
- watchableStore
- revision
- compaction
- defrag
related_domains:
- 集群基础
- 数据库中间件
- 可靠性
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# etcd 与 Kubernetes 存储链路源码剖析

> **源码基线**：`33-源码/控制平面/etcd-3.7.0/` + `kubernetes-1.36.2/`
> 概念层配套阅读：[[01-集群基础/03-控制平面/11-etcd-deep-dive.md|控制平面：etcd Deep Dive]] · [[01-集群基础/02-设计原则/08-distributed-consensus-etcd.md|分布式共识与 etcd 原理]]

## 概述

Kubernetes 的「集群状态」在物理上就是 etcd 里 `/registry/` 前缀下的一批 protobuf 编码 KV。理解这条链路要打通两个仓库：

- **K8s 侧**：`staging/src/k8s.io/apiserver/pkg/storage/etcd3/`（客户端封装，见 [[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md|02 篇]]第三节）
- **etcd 侧**：gRPC 入口 → Raft 共识 → MVCC 存储 → Watch 通知

---

## 一、写入链路：从 Put 到落盘

### 1.1 请求入口与 Raft 提交

```go
// server/etcdserver/v3_server.go:295（实测行号）
func (s *EtcdServer) Put(ctx context.Context, r *pb.PutRequest) (*pb.PutResponse, error) {
    resp, err := s.raftRequest(ctx, pb.InternalRaftRequest{Put: r})
}

// v3_server.go:1058 — 所有写请求的共识入口
func (s *EtcdServer) processInternalRaftRequestOnce(ctx context.Context, r *pb.InternalRaftRequest) (*apply2.Result, error) {
    // 1. 生成唯一 requestID，注册 wait channel
    // 2. s.r.Propose(ctx, data)   → 提交给 Raft 状态机
    // 3. 阻塞等待该提案被 apply 后的结果（或超时）
}
```

### 1.2 Raft 节点循环

```go
// server/etcdserver/raft.go:174
func (r *raftNode) start(rh *raftReadyHandler) {
    // 消费 raft 库的 Ready() 通道，每轮:
    //   - 持久化 HardState + Entries 到 WAL（fsync，写延迟主要来源）
    //   - 发送消息给 peer（AppendEntries）
    //   - 已提交条目送 applyc → apply 层
}
```

写路径全景：

```
client PUT
  → Leader: processInternalRaftRequestOnce (:1058)
  → raft.Propose → WAL fsync（本地）+ 并行复制给 Followers
  → 多数派确认 → committed
  → apply 层 (server/etcdserver/apply/uber_applier.go → apply.go)
  → MVCC store 写入 → 唤醒 wait channel → 响应 client
```

**生产要点**：`wal_fsync_duration_seconds` 高 → 磁盘慢拖累整个写路径；`slow apply` 告警 → apply 层被大事务/大 value 阻塞。etcd 对 K8s 而言写延迟直接放大为 API 写延迟。

---

## 二、MVCC 存储引擎

### 2.1 两层结构：treeIndex（内存）+ boltdb（磁盘）

```
server/storage/mvcc/（实测目录）
├── index.go / key_index.go   # treeIndex: B-tree, key → 各代 revision 列表
├── kvstore.go                # store: 读写协调、compaction
├── watchable_store.go        # 带 Watch 能力的 store 包装
server/storage/backend/
└── backend.go                # boltdb 封装; BatchTx():262 批量提交事务
```

- **treeIndex**：内存 B-tree，记录每个 key 的所有历史 revision（`key → [(main,sub), ...]`）
- **boltdb**：真正的数据文件（`db`），bucket `key` 中以 **revision 为键**、KeyValue protobuf 为值

一次读 `Get(key, rev)`：先查 treeIndex 拿到 ≤rev 的最新 revision，再以该 revision 去 boltdb 取值。**revision 是全局单调递增的逻辑时钟**——这正是 K8s `resourceVersion` 的本体（透传 ModRevision）。

### 2.2 Compaction 与 Defrag

| 操作 | 作用 | 源码位置 | K8s 关联 |
|------|------|---------|---------|
| compact | 删除某 revision 之前的历史版本（逻辑删除） | `mvcc/kvstore_compaction.go` | apiserver `--etcd-compaction-interval`（默认 5m 自动请求） |
| defrag | 重写 boltdb 文件回收空间（物理整理） | `backend/backend.go` | 需运维触发；执行期间该成员阻塞读写 |

**因果链**：不 compact → 历史版本堆积 → db 超过 quota（默认 2GB，上限建议 8GB）→ 集群进入 `NOSPACE` 告警只读态。而 compact 太激进 → watcher 请求的旧 revision 已被清理 → `ErrCompacted` → apiserver watchCache 重新 List → **大集群 relist 风暴**。这条链是「etcd 参数与 apiserver 稳定性强耦合」的典型。

---

## 三、Watch 机制：从 boltdb 变更到 apiserver 事件

```go
// server/storage/mvcc/watchable_store.go（实测行号）
func newWatchableStore(...) *watchableStore   // :92  启动 syncWatchersLoop / syncVictimsLoop
func (s *watchableStore) notify(rev int64, evs []*mvccpb.Event)  // :468 写事务结束时同步通知
```

三组 watcher 集合的流转：

```
synced   : 跟上进度的 watcher —— 每次写事务 end() 时经 notify(:468) 直接推送
unsynced : 落后的 watcher —— syncWatchersLoop 每 100ms 批量从 boltdb 补历史事件
victims  : 推送缓冲满而阻塞的 watcher —— syncVictimsLoop 异步重试
```

与 K8s 的衔接：apiserver 的 Cacher（[[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md|02 篇]]3.3 节）作为**单一 etcd watcher**订阅 `/registry/<resource>/` 前缀，再在内存中扇出给成百上千个客户端 watch——这就是 etcd 只需维护少量 watch 连接却能支撑全集群 List-Watch 的原因。

---

## 四、K8s 对象在 etcd 中的物理形态

```bash
# 🟢 低风险：只读
etcdctl get /registry/pods/default/nginx --prefix -w protobuf | head
# key:   /registry/pods/<namespace>/<name>
# value: 存储版本(通常 v1)的 protobuf; 前缀 magic "k8s\x00"
```

| K8s 概念 | etcd 对应物 | 源码衔接点 |
|----------|------------|-----------|
| resourceVersion（对象） | ModRevision | `etcd3/store.go` 解码时注入 |
| resourceVersion（List） | 响应头 Revision | `GetList` (etcd3/store.go:736) |
| Watch bookmark | ProgressNotify + apiserver 定时下发 | Cacher |
| 乐观锁 409 | Txn If(ModRevision=) | `GuaranteedUpdate` (etcd3/store.go:463) |
| Secret 加密 (KMS) | value 层加密信封 | `staging/.../storage/value/` transformer 链 |

**存储版本陷阱**：升级 K8s 后 etcd 内旧对象仍是旧存储版本编码，直到被重写。`kube-storage-version-migrator` 的存在与 [[01-集群基础/06-升级路径/index.md|升级路径域]] 中「跨多版本升级前先迁移存储版本」的要求都源于此。

---

## 五、生产排障速查

| 症状 | 源码/机制定位 | 检查手段 |
|------|--------------|---------|
| API 写延迟高 | WAL fsync (raft.go:174 循环) | `wal_fsync_duration_seconds` p99 > 10ms 即磁盘瓶颈 |
| NOSPACE 只读 | quota (apply/quota.go) | compact + defrag + `alarm disarm` |
| Watch 事件延迟 | victims 堆积 (watchable_store.go) | `etcd_debugging_mvcc_pending_events_total` |
| relist 风暴 | ErrCompacted → watchCache 重建 | compaction 间隔 vs watch 缓冲窗口 |
| 成员间数据不一致告警 | corrupt 检测 (etcdserver/corrupt.go) | `etcdctl endpoint hashkv --cluster` |
| Leader 频繁切换 | 心跳超时（网络/磁盘抖动） | `etcd_server_leader_changes_seen_total` |

etcd 集群运维操作（备份/恢复/成员管理）见 [[01-集群基础/03-控制平面/19-etcd-operations.md|etcd 运维操作]] 与 [[10-平台工程/06-代码分析/cluster-create/13-etcd-advanced.md|etcd 进阶：HA 集群管理]]。

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md|02 - kube-apiserver 源码深度剖析]]（存储链路前半程）
- [[10-平台工程/06-代码分析/kubernetes-core/07-component-interaction-dataflow.md|07 - 组件交互关系与数据流向]]
- [[01-集群基础/03-控制平面/11-etcd-deep-dive.md|控制平面：etcd Deep Dive]]
- [[01-集群基础/02-设计原则/08-distributed-consensus-etcd.md|分布式共识与 etcd 原理]]
- [[01-集群基础/02-设计原则/07-resource-version-control.md|资源版本与并发控制]]
- [[07-数据库中间件/README.md|数据库中间件域]]（etcd 作为分布式 KV 的横向对比）
- [[12-可靠性/README.md|可靠性域]]（etcd 灾备与恢复）
