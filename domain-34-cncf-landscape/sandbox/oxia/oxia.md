---
title: Oxia
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- helm
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Oxia 是什么
- 如何 Oxia
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Oxia
- cncf
- landscape
---

# Oxia

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/streamnative/oxia |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Oxia 是一个可水平扩展的元数据存储和协调系统，旨在作为 ZooKeeper 和 etcd 的高可扩展性替代方案。它由 StreamNative 开发，最初用于解决 Apache Pulsar 在大规模场景下对 ZooKeeper 的扩展性瓶颈。Oxia 通过分片架构将数据分布到多个节点，支持百万级 Key 的元数据管理，同时提供与 ZooKeeper 兼容的通知和协调原语。

### 核心特性

- **水平扩展**: 通过数据分片实现线性扩展，突破单节点瓶颈
- **高吞吐低延迟**: 基于 gRPC 和高效存储引擎，提供亚毫秒级延迟
- **通知机制**: 支持 Key 变更的实时通知（类似 ZooKeeper Watch）
- **乐观并发**: 基于版本号的乐观锁，支持 CAS 操作
- **自动分片重均衡**: 集群扩缩容时自动迁移数据分片
- **Raft 共识**: 每个分片使用 Raft 协议保证强一致性
- **ZooKeeper 兼容代理**: 提供 ZK 协议代理，便于无缝迁移

---

## 架构设计

```
┌─────────────────────────────────────────────────┐
│                 Oxia Cluster                     │
│                                                  │
│  ┌───────────────────────────────────────┐      │
│  │         Coordinator Node              │      │
│  │  (分片分配 / 集群管理 / Leader 选举)   │      │
│  └───────────────┬───────────────────────┘      │
│                  │                               │
│  ┌───────────────▼───────────────────────┐      │
│  │          Shard Map                     │      │
│  │  Shard 0: [Node1*, Node2, Node3]      │      │
│  │  Shard 1: [Node2*, Node1, Node3]      │      │
│  │  Shard 2: [Node3*, Node2, Node1]      │      │
│  │  (* = Leader)                          │      │
│  └───────────────────────────────────────┘      │
│                                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │ Node 1   │  │ Node 2   │  │ Node 3   │      │
│  │ Shard 0L │  │ Shard 1L │  │ Shard 2L │      │
│  │ Shard 1F │  │ Shard 0F │  │ Shard 0F │      │
│  │ Shard 2F │  │ Shard 2F │  │ Shard 1F │      │
│  │ ┌──────┐ │  │ ┌──────┐ │  │ ┌──────┐ │      │
│  │ │Pebble│ │  │ │Pebble│ │  │ │Pebble│ │      │
│  │ │  DB  │ │  │ │  DB  │ │  │ │  DB  │ │      │
│  │ └──────┘ │  │ └──────┘ │  │ └──────┘ │      │
│  └──────────┘  └──────────┘  └──────────┘      │
└─────────────────────────────────────────────────┘
         ▲              ▲              ▲
         │              │              │
    ┌────┴──────────────┴──────────────┴────┐
    │          gRPC Client SDK              │
    │   (Go / Java / 自动分片路由)           │
    └───────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 Docker Compose 快速启动 (3 节点集群)
git clone https://github.com/streamnative/oxia.git
cd oxia
docker compose up -d

# 或在 Kubernetes 上部署
helm repo add oxia https://streamnative.github.io/oxia/charts
helm install oxia oxia/oxia \
  --namespace oxia \
  --create-namespace \
  --set replicas=3 \
  --set shards=8
```

### 使用 CLI

```bash
# 写入 Key
oxia client put --key="/config/db-host" --value="db.example.com"

# 读取 Key
oxia client get --key="/config/db-host"

# 列出 Key 范围
oxia client list --key-min="/config/" --key-max="/config/~"

# 删除 Key
oxia client delete --key="/config/db-host"

# CAS 更新（乐观锁）
oxia client put --key="/leader" --value="node-1" --expected-version=5
```

### Go 客户端 SDK

```go
package main

import (
    "context"
    "github.com/streamnative/oxia/oxia"
)

func main() {
    client, _ := oxia.NewSyncClient("localhost:6648")
    defer client.Close()

    ctx := context.Background()

    // 写入
    version, _ := client.Put(ctx, "/config/key1", []byte("value1"))

    // 读取
    result, _ := client.Get(ctx, "/config/key1")
    // result.Value = "value1"

    // CAS 更新
    newVersion, _ := client.Put(ctx, "/config/key1",
        []byte("value2"),
        oxia.ExpectedVersionId(version.VersionId))

    // 监听变更通知
    notifications, _ := client.GetNotifications()
    for notification := range notifications {
        // notification.Key, notification.Type (Created/Modified/Deleted)
    }

    // 删除
    client.Delete(ctx, "/config/key1",
        oxia.ExpectedVersionId(newVersion.VersionId))
}
```

---

## 高级特性

### ZooKeeper 兼容代理

```bash
# 启动 ZooKeeper 代理，使现有 ZK 应用无需修改代码即可迁移
oxia zk-proxy --oxia-address=localhost:6648 --listen=0.0.0.0:2181
```

### 批量操作

```go
// 原子性批量写入
results, _ := client.WriteBatch(ctx,
    oxia.PutOp("/keys/a", []byte("1")),
    oxia.PutOp("/keys/b", []byte("2")),
    oxia.DeleteOp("/keys/old"),
)
```

---

## 与其他方案对比

| 特性 | Oxia | etcd | ZooKeeper |
|:---|:---|:---|:---|
| 数据模型 | KV (字节) | KV (字节) | 树形节点 |
| 水平扩展 | 分片扩展 | 不支持 | 不支持 |
| 最大 Key 数 | 百万+ | ~百万 | ~十万 |
| 共识协议 | Raft (每分片) | Raft | ZAB |
| Watch/通知 | Key 级通知 | Key/Prefix Watch | 节点 Watch |
| 存储引擎 | Pebble (LSM) | BoltDB (B+Tree) | 内存+日志 |
| 适用场景 | 大规模元数据 | 中小规模配置 | 中小规模协调 |

---

## 最佳实践

1. **分片数量**: 根据数据量和写入吞吐设置分片数，通常 8-64 个分片
2. **副本因子**: 生产环境使用 3 副本保证数据安全
3. **Key 设计**: 使用层级式 Key (如 `/service/config/key`) 便于范围查询
4. **连接池**: 客户端复用连接，避免频繁建立 gRPC 连接
5. **渐进迁移**: 先部署 ZK 代理运行已有应用，再逐步迁移到原生 Oxia SDK

---

## 参考资源

- [Oxia GitHub](https://github.com/streamnative/oxia)
- [Oxia 设计文档](https://github.com/streamnative/oxia/blob/main/docs/design.md)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
