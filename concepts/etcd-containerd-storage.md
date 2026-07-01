---
title: "etcd 与 containerd 存储架构"
category: synthesis
tags: [synthesis, etcd, containerd, storage]
sources: []
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

# etcd 与 containerd 存储架构

> etcd 作为 Kubernetes 的数据存储后端与 containerd 作为容器运行时的存储机制对比与集成。

## 存储层次

Kubernetes 集群有两个关键的存储层:

| 组件 | 存储内容 | 存储引擎 | 数据类型 |
|------|---------|---------|---------|
| etcd | 集群状态、配置、Secrets | bbolt (B+tree) | 键值对 |
| containerd | 镜像层、容器快照 | content store | OCI 格式 |

## etcd 存储

- 存储所有 Kubernetes 资源对象
- 使用 Raft 共识协议保证一致性
- 快照备份是灾备的核心手段

## containerd 存储

- 镜像层以 content-addressable 方式存储
- 容器可写层使用 overlayfs
- snapshotter 管理容器文件系统

## 运维交叉点

- etcd 磁盘 I/O 直接影响 API Server 性能
- containerd 镜像存储占用节点磁盘空间
- 两者都需要定期清理和监控

## 相关页面

- [[etcd]] — etcd 运维
- [[containerd]] — containerd 运行时
- [[kubernetes]] — 集群架构
