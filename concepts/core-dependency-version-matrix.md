---
title: 核心依赖版本矩阵
description: etcd 是 Kubernetes 的唯一状态存储，版本选择直接影响集群的稳定性和性能。
category: concepts
tags:
- k8s
- release-notes
- etcd
- containerd
- cri-o
- coredns
- runc
- docker
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 核心依赖版本矩阵 是什么
- 如何 核心依赖版本矩阵
trigger_keywords:
- 核心依赖版本矩阵
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# 核心依赖版本矩阵

> 本文档综合了 `domain-19-landscape-references/_archived-release-notes/core-deps/` 目录下 5 个核心依赖项目的 83 个版本发布说明 ^[inferred]

## etcd 版本演进

etcd 是 Kubernetes 的唯一状态存储，版本选择直接影响集群的稳定性和性能。

| etcd 版本 | 关键特性 | 兼容 [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|[[Kubernetes 生产环境速查卡|k8s]]]] 版本 |
|---|---|---|
| v3.0 | v3 API 引入、lease 机制、watch 改进 | v1.3 - v1.6 |
| v3.1 | 快照恢复改进、etcdctl v3 完善 | v1.6 - v1.8 |
| v3.2 | [[gRPC|gRPC]] proxy、etcd operator 支持、性能优化 | v1.8 - v1.12 |
| v3.3 | 改进的压缩机制、更好的监控指标 | v1.12 - v1.15 |
| v3.4 | 压缩优化、WAL 预写日志改进、etcd 嵌入 etcd | v1.15 - v1.18 |
| v3.5 | 多版本并发控制、改进的 leader 选举、更好的集群恢复 | v1.20+ |

### etcd v3.0 关键变更

- v3 API 成为核心：key-value 存储从 v2 转向 v3
- Lease 机制引入，支持 TTL 关联
- Snapshot restore 支持 lease key
- etcdctl v3 命令体系建立

## [[containerd|containerd]] 版本演进

containerd 从 Docker 中独立出来后成为 Kubernetes 的主要容器运行时。

| containerd 版本 | 关键特性 | 兼容 K8s 版本 |
|---|---|---|
| v1.0 | CRI 支持初版、基础容器生命周期管理 | v1.10 - v1.12 |
| v1.1 | CRI v1alpha1 改进、更好的 Windows 支持 | v1.10 - v1.13 |
| v1.2 | CRI v1 稳定、容器快照改进 | v1.13 - v1.15 |
| v1.3 | 改进的快照和迁移 | v1.15 - v1.18 |
| v1.4 | 支持 Kubernetes 1.20、CRI v1 完善 | v1.20 - v1.22 |
| v1.5 | 完整的 CRI v1、更好的 Pod 沙箱支持 | v1.22 - v1.24 |
| v1.6+ | 长期支持版本、全面的 CRI 实现 | v1.24+ |

### containerd v1.0 关键变更

- FIFO 死锁问题修复（healthcheck 相关）
- 快照 GC 修复
- 用户命名空间 mknod 处理
- 依赖 btrfs 更新

## [[cri-o|CRI-O]] 版本演进

CRI-O 是专为 Kubernetes 设计的轻量级容器运行时，是 containerd 的替代方案。

| CRI-O 版本 | 对应 K8s 版本 | 关键特性 |
|---|---|---|
| v1.10 | v1.10 | CRI 基础实现 |
| v1.11 | v1.11 | 改进的容器生命周期 |
| v1.12 | v1.12 | Pod 沙箱优化 |
| v1.13 | v1.13 | SELinux 集成 |
| v1.14 - v1.20 | v1.14 - v1.20 | 逐步完善的 CRI 实现 |
| v1.21+ | v1.21+ | 现代化 CRI 运行时 |

## [[coredns|coredns]] 版本演进

CoreDNS 自 Kubernetes v1.11 起成为默认 DNS 服务。

| CoreDNS 版本 | 关键特性 |
|---|---|
| v010 - v0.99 | 早期开发版本，插件架构确立 |
| v1.0 | 首次 GA、Kubernetes 插件成熟 |
| v1.1 - v1.3 | 改进的 Kubernetes 服务发现 |
| v1.4 - v1.6 | 更好的性能、改进的缓存机制 |
| v1.7+ | 现代 DNS 服务、更好的监控集成 |

## runc 版本演进

runc 是 OCI 容器运行时的参考实现，被 containerd 和 CRI-O 底层使用。

| runc 版本 | 关键特性 |
|---|---|
| v0.1 - v0.4 | 早期 OCI 实现 |
| v0.5 - v0.9 | 改进的 cgroups 支持 |
| v1.0 | OCI 规范 v1.0 完全实现 |
| v1.1+ | 现代 cgroups v2 支持、安全加固 |

## 版本兼容性建议

### 推荐的 K8s + 核心依赖组合

| K8s 版本 | etcd | containerd | CoreDNS |
|---|---|---|---|
| v1.28 | v3.5.x | v1.6.x | v1.10.x |
| v1.29 | v3.5.x | v1.7.x | v1.11.x |
| v1.30 | v3.5.x | v1.7.x | v1.11.x |
| v1.31 | v3.5.x | v1.7.x | v1.11.x |
| v1.32 | v3.5.x | v1.7.x | v1.11.x |

### 升级注意事项

1. **etcd 备份**：升级 etcd 前务必备份数据
2. **containerd 兼容性**：确保 containerd 版本支持目标 K8s 版本的 CRI API
3. **CoreDNS 迁移**：从 kube-dns 迁移到 CoreDNS 需要规划
4. **runc 安全**：关注 runc 的 CVE 修复版本（如 CVE-2019-5736）

## 来源文档

- domain-19-landscape-references/_archived-release-notes/core-deps/etcd/（15 个文件）
- domain-19-landscape-references/_archived-release-notes/core-deps/containerd/（13 个文件）
- domain-19-landscape-references/_archived-release-notes/core-deps/cri-o/（32 个文件）
- domain-19-landscape-references/_archived-release-notes/core-deps/coredns/（16 个文件）
- domain-19-landscape-references/_archived-release-notes/core-deps/runc/（7 个文件）

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd
- [[coredns]] — CoreDNS
- [[cri-o]] — CRI-O
- [[etcd]] — etcd
