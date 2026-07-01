---
title: 核心依赖变更日志索引
description: '# 核心依赖变更日志索引'
category: entities
tags:
- k8s
- release-notes
- etcd
- containerd
- cri-o
- coredns
- runc
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 核心依赖变更日志索引 是什么
- 如何 核心依赖变更日志索引
trigger_keywords:
- 核心依赖变更日志索引
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# 核心依赖变更日志索引

> 本文档是 `domain-19-landscape-references/_archived-release-notes/core-deps/` 目录下核心依赖组件变更日志的索引 ^[inferred]

## 组件版本文件统计

| 组件 | 文件数 | 版本范围 |
|---|---|---|
| etcd | 15 | v3.0+ |
| [[containerd|containerd]] | 13 | v1.0+ |
| [[cri-o|CRI-O]] | 32 | v1.10+ |
| [[coredns|coredns]] | 16 | v0.10+ |
| runc | 7 | v0.1+ |
| **合计** | **83** | - |

## 关键版本参考

### etcd

- v3.0：v3 API 引入，lease 机制，snapshot restore
- 参见 [[concepts/core-dependency-version-matrix.md|[[核心依赖版本矩阵|核心依赖版本矩阵]]]] 了解完整版本矩阵

### containerd

- v1.0：CRI 支持初版，FIFO 死锁修复
- 参见 [[entities/container-runtime.md|[[Container Runtime|Container Runtime]]]] 了解容器运行时架构

### CRI-O

- v1.10 - v1.21+：渐进式 CRI 实现
- 专为 Kubernetes 设计的轻量运行时

### CoreDNS

- v1.0：首次 GA
- Kubernetes v1.11 起成为默认 DNS

### runc

- v1.0：OCI 规范 v1.0 完全实现
- 底层容器运行时

## 来源文档

domain-19-landscape-references/_archived-release-notes/core-deps/ 目录下全部 83 个文件。

## Related

- [[coredns]] — CoreDNS
- [[cri-o]] — CRI-O
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd
