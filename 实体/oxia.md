---
title: Oxia (entities)
description: '## 概述'
summary: 'Oxia 是一个可水平扩展的元数据存储和协调系统，旨在作为 ZooKeeper 和 etcd 的高可扩展性替代方案。它由 StreamNative 开发，最初用于解决 Apache Pulsar 在大规模场景下对 ZooKeeper 的扩展性瓶颈。Oxia 通过分片架构将数据分布到多个节点，支持百万级 Key 的元数据管理，'
category: entities
tags:
- k8s
- cncf
- database
- oxia
- etcd
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Oxia 是什么
- 如何 Oxia
trigger_keywords:
- Oxia
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Oxia

> **CNCF 状态**: Sandbox | **类别**: Database | **主要语言**: Go

## 概述

Oxia 是由 StreamNative（Apache Pulsar 商业化公司）开发的开源分布式协调和元数据存储系统，2023 年进入 CNCF Sandbox。它被设计为 **ZooKeeper 和 etcd 的下一代替代方案**，解决这些系统在大规模场景下的扩展性瓶颈。ZooKeeper 和 etcd 采用单一的 Raft/Paxos 共识组，写入吞吐受限于单节点能力。Oxia 通过**分片（Sharding）架构**将数据分布到多个分片组，每个分片独立运行 Raft 协议，实现线性水平扩展。

Oxia 最初是为 Apache Pulsar 设计的——Pulsar 的元数据和协调操作原本依赖 ZooKeeper，在数千 Topic 场景下 ZK 成为瓶颈。Oxia 支持百万级 Key 的元数据管理，同时提供与 ZooKeeper 兼容的通知（Watch）和分布式锁等协调原语。

## Key Features

- **水平分片扩展**：数据按 Key 分片到多个 Shard 组，每个 Shard 独立 Raft 共识
- **高吞吐写入**：相比 etcd 的单组 Raft，多分片并行写入吞吐提升 10x+
- **ZooKeeper 兼容**：提供 Watch/通知、领导选举、分布式锁等 ZK 兼容 API
- **gRPC API**：现代的 gRPC 接口，支持多语言客户端（Go、Java、Python）
- **层级式 Key**：支持 ZK 风格的层级 Key（`/service/config/key`）和范围查询
- **K8s 原生**：通过 Operator 部署，支持自动故障恢复和分片重平衡

## Architecture

Oxia 由 **Oxia Coordinator**（管理分片分配和领导者选举）、**Oxia Server**（存储分片数据，运行 Raft 协议）和 **Oxia Client**（gRPC 客户端库）组成。数据按 Key 的哈希值分配到 Shard（默认 8-64 个 Shard）。每个 Shard 在 3 个 Server 副本上运行 Raft 共识。Coordinator 监控 Server 健康状态，自动在节点故障时迁移分片。客户端通过全局路由表（Shard Assignment）找到每个 Key 对应的 Shard Leader 进行读写。

## K8s 集成

Oxia 通过 **Oxia Operator**（Helm Chart）部署到 Kubernetes。Operator 管理 Oxia Coordinator 和 Server 的 StatefulSet，自动处理分片重平衡和故障恢复。数据持久化通过 PVC（PersistentVolumeClaim）存储在节点本地存储或网络存储上。客户端通过 Service 发现 Oxia 集群。

## 生产部署要点

- **分片数量**：根据数据量和写入吞吐设置分片数，通常 8-64 个分片
- **副本因子**：生产环境使用 3 副本保证数据安全
- **Key 设计**：使用层级式 Key (如 `/service/config/key`) 便于范围查询
- **连接池**：客户端复用连接，避免频繁建立 gRPC 连接
- **渐进迁移**：先部署 ZK 代理运行已有应用，再逐步迁移到原生 Oxia SDK

## 生产场景

1. **Apache Pulsar 元数据存储**：替代 ZooKeeper 作为 Pulsar 的元数据和协调后端
2. **大规模服务发现**：数十万服务的注册和发现，支持高频率更新
3. **分布式配置管理**：集中式配置存储，支持 Watch 机制实时推送变更
4. **分布式锁和领导选举**：替代 ZK 实现大规模分布式协调

## 安装与配置

```bash
# Helm 安装 Oxia
helm repo add oxia https://oxia.github.io/charts/
helm install oxia oxia/oxia -n oxia --create-namespace \
  --set replicationFactor=3 \
  --set shardCount=16

# 使用 oxia CLI 操作
oxia peek        # 列出所有 Key
oxia put /myapp/config '{"timeout": 30}'
oxia get /myapp/config
oxia list /myapp/

# Go 客户端示例
# client, _ := oxia.NewClient("oxia.oxia.svc:6648")
# client.Put(context.Background(), "/key1", []byte("value1"))
```

### 生产配置示例

```yaml
# oxia-values.yaml
coordinator:
  replicas: 3
  resources:
    requests:
      cpu: 500m
      memory: 512Mi

server:
  replicas: 3
  shardCount: 16
  resources:
    requests:
      cpu: "1"
      memory: 2Gi
  persistence:
    enabled: true
    size: 50Gi
    storageClassName: fast-ssd

# ZooKeeper 兼容代理
zkProxy:
  enabled: true
  port: 2181
```

### 分布式锁示例

```go
package main

import (
    "context"
    "github.com/streamnative/oxia/client-go"
)

func main() {
    client, _ := oxia.NewClient("oxia.oxia.svc:6648")
    
    // 分布式锁
    lock := client.NewLock("/locks/order-service")
    if err := lock.Acquire(context.Background()); err != nil {
        panic(err)
    }
    defer lock.Release(context.Background())
    
    // 执行临界区操作
    // ...
    
    // Watch 配置变更
    watcher := client.Watch("/config/")
    for event := range watcher {
        println("Config changed:", event.Key)
    }
}
```

## 运维操作

```bash
# 🟢 查看 Oxia 集群状态
kubectl get pods -n oxia
kubectl get oxiacluster -n oxia

# 🟢 查看分片分布
kubectl exec -n oxia deploy/oxia-coordinator -- oxia-admin shards

# 🟢 查看 Key 数量
oxia peek | wc -l

# 🟢 检查集群健康
kubectl exec -n oxia deploy/oxia-coordinator -- oxia-admin status

# 🟡 添加分片（扩容）
kubectl patch oxiacluster oxia -n oxia -p '{"spec":{"shardCount":32}}'

# 🟢 查看 Raft 状态
kubectl exec -n oxia sts/oxia-server-0 -- oxia-admin raft-status
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 写入超时 | Leader 不可用 | `kubectl logs sts/oxia-server-0` | 检查 Raft 选举状态 |
| 分片不均衡 | 节点故障 | `oxia-admin shards` | 触发分片重平衡 |
| 连接失败 | Service 不可达 | `kubectl get svc -n oxia` | 检查 Service 和 DNS |
| 数据丢失 | 副本不足 | 检查 replicationFactor | 确保 3 副本 |
| 性能下降 | 分片过少 | 检查写入分布 | 增加分片数 |

### 排查流程

```
Oxia 异常
├─ 集群不可用？
│  ├─ Coordinator 异常 → 检查 Coordinator Pod
│  ├─ Server 全部宕机 → 检查 PVC 和节点
│  └─ 网络问题 → 检查 gRPC 端口 6648
├─ 写入失败？
│  ├─ Leader 选举中 → 等待选举完成
│  ├─ 磁盘满 → 检查 PVC 使用率
│  └─ 分片迁移中 → 等待迁移完成
└─ 性能问题？
   ├─ 写入慢 → 增加分片数
   └─ 读取慢 → 检查副本分布
```

## 生产案例

### 案例 1: Apache Pulsar 元数据迁移

**场景**: Pulsar 集群使用 ZooKeeper 存储元数据，万级 Topic 时 ZK 成为瓶颈。

**方案**:
1. 部署 Oxia 集群（16 分片，3 副本）
2. 启用 ZK 兼容代理
3. Pulsar 配置指向 Oxia ZK Proxy
4. 逐步迁移到原生 Oxia SDK

**效果**: 元数据写入吐吐量提升 10x，P99 延迟从 50ms 降至 5ms。

### 案例 2: 大规模服务发现

**场景**: 50 万微服务实例需高频注册和发现。

**方案**:
1. Oxia 存储服务注册信息
2. Watch 机制实时推送变更
3. 分片架构支持高并发写入

**效果**: 支持 100K+ QPS 服务注册，发现延迟 < 10ms。

## 对比与替代方案

| 维度 | Oxia | etcd | ZooKeeper | Consul |
|------|------|------|-----------|--------|
| 分片扩展 | ✅ | ❌ 单组 Raft | ❌ | ✅ |
| ZK 兼容 API | ✅ | ❌ | ✅ 原生 | ⚠️ |
| 语言 | Go | Go | Java | Go |
| 写入吐吐 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |
| K8s 原生 | ✅ | ✅ | ⚠️ | ✅ |
| 成熟度 | 新兴 | 成熟 | 成熟 | 成熟 |

## 检查清单

- [ ] 分片数已根据数据量配置（8-64）
- [ ] 副本因子为 3（生产环境）
- [ ] PVC 使用高性能存储（SSD）
- [ ] ZK 兼容代理已配置（如需迁移）
- [ ] 客户端连接池已配置
- [ ] 监控告警：分片健康/磁盘使用率
- [ ] 备份策略已配置
- [ ] 分片重平衡已测试

## 对比

| 特性 | Oxia | etcd | ZooKeeper | Consul |
|------|------|------|-----------|--------|
| 分片扩展 | ✅ | ❌ 单组 Raft | ❌ | ✅ |
| ZK 兼容 API | ✅ | ❌ | ✅ 原生 | ⚠️ |
| 语言 | Go | Go | Java | Go |
| 写入吞吐 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |
| K8s 原生 | ✅ | ✅ | ⚠️ | ✅ |

## 参考链接

- [[etcd]]

## Related

- [[cohdi]] — Cohdi
- [[koordinator]] — Koordinator
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC
- [[etcd]] — etcd

- oxia
- [[实体/schemahero.md|[[SchemaHero|SchemaHero]]]]
- index/etcd-index|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
