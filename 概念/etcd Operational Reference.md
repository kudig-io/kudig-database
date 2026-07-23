---
title: etcd Operational Reference
description: etcd Operational Reference — Kubernetes 生产运维知识库
summary: etcd Operational Reference — Kubernetes 生产运维知识库
category: concept
tags:
- k8s
- etcd
- control-plane
- distributed-systems
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcd Operational Reference 是什么
- 如何 etcd Operational Reference
trigger_keywords:
- etcd
- Operational
- Reference
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# etcd Operational Reference

## Overview

etcd is a distributed, reliable key-value store that serves as Kubernetes' single source of truth for all cluster state and configuration. It uses the Raft consensus algorithm for consistency and MVCC for versioning.

## Core Properties

| Property | Detail |
|----------|--------|
| **Simple** | Standard HTTP/JSON API, curl-friendly |
| **Secure** | Optional SSL client certificate authentication |
| **Fast** | Benchmark: 10,000+ writes/second |
| **Reliable** | Raft consensus for distributed consistency |

## Raft Consensus

- **Leader Election**: Cluster elects a leader to handle all write operations
- **Log Replication**: Leader replicates log entries to follower nodes
- **Safety**: Committed entries are never lost; all nodes see the same log

## MVCC Storage

- **Multi-Version Concurrency Control**: Each key modification retains historical versions
- **Version Numbering**: Global version number increments on each modification
- **Compaction**: Automatic or manual deletion of expired versions to reclaim space

## Cluster Topology

| Topology | Fault Tolerance | Use Case |
|----------|----------------|----------|
| Single node | 0 | Development/testing only |
| 3 nodes | 1 node | Minimum production |
| 5 nodes | 2 nodes | High-availability production |
| 7+ nodes | 3+ nodes | Not recommended - write performance degrades |

### Node Types

- **Leader**: Handles all client write requests, replicates log
- **Follower**: Receives and persists leader's log entries
- **Learner**: Read-only node, does not participate in consensus (for read scaling)

## Key Operational Commands

### Cluster Management

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl member remove`：移除 etcd 成员，误删多数派会致集群不可用/丢数据

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# List cluster members
etcdctl member list

# Add a member
etcdctl member add <name> --peer-urls=<urls>

# Remove a member
etcdctl member remove <member_id>  # ⚠️ 移除 etcd 成员，可能丢数据

# Check cluster health
etcdctl endpoint health

# Check endpoint status
etcdctl endpoint status --write-out=table
```
### Data Operations

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# Write data
etcdctl put /key "value"

# Read data
etcdctl get /key

# Read with specific revision
etcdctl get /key --rev=4

# Delete data
etcdctl del /key

# Watch for changes
etcdctl watch /key

# Range query
etcdctl get /prefix --prefix
```
### Backup and Restore

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Create snapshot
etcdctl snapshot save /path/to/backup.db

# Verify snapshot
etcdctl snapshot status /path/to/backup.db

# Restore from snapshot
etcdctl snapshot restore /path/to/backup.db --data-dir=/var/lib/etcd  # ⚠️ 覆盖 etcd 数据，集群状态回退
```
## Critical Failure Modes (from FTA)

| Failure Mode | FTA ID | RPN | Impact |
|-------------|--------|-----|--------|
| Disk space exhausted | BE-1.2.1 | 135 | Cluster read-only |
| Quorum lost | BE-1.2.2 | 120 | Cluster unavailable |
| Data corruption | BE-1.2.3 | 120 | Data loss |
| High response latency | BE-1.2.4 | 84 | API Server degraded |
| Certificate expiry | BE-1.2.5 | 72 | Connection refused |
| Version incompatibility | BE-1.2.6 | 80 | Communication failure |

## Production Tuning

- **Disk**: Use SSD/ESSD with low latency (< 10ms fsync). Avoid network storage.
- **Memory**: Sufficient for working set. Watch for OOM from large data volumes.
- **Network**: Low-latency, high-bandwidth between etcd members. Same availability zone preferred.
- **Compaction**: Run compaction regularly to prevent unbounded growth.
- **Snapshot**: Take regular snapshots for disaster recovery.

## Related

- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[概念/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[技能/backup-restore-etcd.md|backup-restore-etcd]] — Backup and Restore etcd
- [[元数据/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]]
- [[etcd|etcd]]
- [[归档/kubernetes-fault-distribution-and-mttr-en.md|Kubernetes Fault Distribution and MTTR]]
- [[技能/backup-restore-etcd.md|Backup and Restore etcd]]
- [[技能/fta-方法论/top-events-index/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[kudig-man-pages-index]]

- RELEASE-NOTES-0.2
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.5.md|RELEASE-NOTES-3.5]]
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.1.md|RELEASE-NOTES-3.1]]
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.0.md|RELEASE-NOTES-3.0]]
- RELEASE-NOTES-0.3
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.4.md|RELEASE-NOTES-3.4]]
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.3.md|RELEASE-NOTES-3.3]]
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.1
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.6.md|RELEASE-NOTES-3.6]]
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-2.3.md|RELEASE-NOTES-2.3]]
- [[归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.2.md|RELEASE-NOTES-3.2]]

<!-- risk-assessed -->
