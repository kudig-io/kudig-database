---
title: 核心依赖变更日志索引
description: '# 核心依赖变更日志索引'
summary: '生态参考/_archived-release-notes/core-deps/ 目录下全部 83 个文件。'
category: entities
tags:
- k8s
- release-notes
- etcd
- containerd
- cri-o
- coredns
- runc
tier: core
created: '2026-05-23'
last_updated: 2026-07
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 核心依赖变更日志索引

> 本文档是 `生态参考/_archived-release-notes/core-deps/` 目录下核心依赖组件变更日志的索引 ^[inferred]

## 概述

Kubernetes 集群依赖于一组**核心组件**（Core Dependencies）作为基础设施。这些组件虽然不属于 Kubernetes 核心代码，但每个生产集群都离不开它们：**etcd**（键值存储/状态数据库）、**containerd**（容器运行时）、**CRI-O**（替代容器运行时）、**CoreDNS**（集群 DNS）、**runc**（底层 OCI 容器运行时）。本页索引了这些核心依赖的完整版本变更日志，帮助运维团队跟踪版本变化、规划升级和排查兼容性问题。

每个核心依赖都有独立的版本节奏和兼容性矩阵。Kubernetes 版本与核心依赖版本之间有严格的兼容性要求——例如 K8s 1.30 要求 etcd 3.5+、containerd 1.7+。升级前必须验证依赖版本兼容性。

## 组件版本文件统计

| 组件 | 文件数 | 版本范围 | 说明 |
|---|---|---|---|
| etcd | 15 | v3.0+ | Kubernetes 状态存储，Raft 共识 |
| [[containerd|containerd]] | 13 | v1.0+ | 默认容器运行时（CRI） |
| [[cri-o|CRI-O]] | 32 | v1.10+ | 替代容器运行时，专为 K8s 设计 |
| [[coredns|coredns]] | 16 | v0.10+ | 集群 DNS 服务发现 |
| runc | 7 | v0.1+ | 底层 OCI 容器运行时 |
| **合计** | **83** | - | - |

## 关键版本参考

### etcd

- **v3.0**：v3 API 引入，lease 机制，snapshot restore
- **v3.4**：gRPC proxy，gRPC gateway，性能提升
- **v3.5**：安全性增强，默认 TLS，维护 K8s 1.20+
- 参见 [[22-概念/core-dependency-version-matrix.md|[[22-概念/01-核心架构/core-dependency-version-matrix|核心依赖版本矩阵]]]] 了解完整版本矩阵

### containerd

- **v1.0**：CRI 支持初版，FIFO 死锁修复
- **v1.6**（LTS）：长期支持版本，K8s 1.23-1.26 默认
- **v2.0**：CRI v1（移除 alpha2），配置格式 v2，NRI 内置
- 参见 [[23-实体/container-runtime.md|[[22-概念/15-运行时与系统/container-runtime|Container Runtime]]]] 了解容器运行时架构

### CRI-O

- **v1.10 - v1.21+**：渐进式 CRI 实现
- 版本号与 K8s 版本对齐（CRI-O 1.30 对应 K8s 1.30）
- 专为 Kubernetes 设计的轻量运行时，替代 containerd

### CoreDNS

- **v1.0**：首次 GA
- **v1.8+**：性能优化，插件生态扩展
- Kubernetes v1.11 起成为默认 DNS（替代 kube-dns）

### runc

- **v1.0**：OCI 规范 v1.0 完全实现
- 底层容器运行时，containerd/CRI-O 底层调用
- **v1.1**：安全修复，性能优化

## Architecture

核心依赖在 Kubernetes 架构中各自承担不同角色：**etcd** 存储所有集群状态（Pod、Service、ConfigMap 等），是唯一的状态数据库；**containerd/CRI-O** 负责容器的镜像管理和生命周期；**runc** 是底层 OCI 运行时，由 containerd/CRI-O 调用创建/运行容器；**CoreDNS** 提供 Service 名称解析和 DNS 发现。这些组件的版本兼容性直接影响集群稳定性。

## K8s 集成

Kubernetes 版本与核心依赖有严格的兼容性矩阵。每个 K8s 版本测试时验证特定版本的 etcd、containerd、CoreDNS。升级 K8s 版本时必须同步检查并升级核心依赖。`kubeadm` 在 init/join 时自动检查依赖版本兼容性。

## 生产部署要点

- **版本矩阵**：升级前查询 [[22-概念/01-核心架构/core-dependency-version-matrix.md|核心依赖版本矩阵]] 确认兼容性
- **滚动升级**：核心依赖升级必须逐节点进行（cordon → drain → upgrade → uncordon）
- **回滚预案**：升级前备份 etcd 快照，准备 containerd/CRI-O 回滚版本
- **变更日志**：升级前阅读目标版本的 CHANGELOG，了解破坏性变更

## 生产场景

1. **K8s 大版本升级**：升级 K8s 1.29 → 1.30 时同步升级 etcd 和 containerd
2. **安全补丁**：定期升级核心依赖到最新补丁版本修复 CVE
3. **兼容性排查**：Pod 创建失败时检查 containerd 版本兼容性
4. **性能调优**：升级 CoreDNS 到高性能版本优化 DNS 解析延迟

## 来源文档

生态参考/_archived-release-notes/core-deps/ 目录下全部 83 个文件。

## Related

- [[coredns]] — CoreDNS
- [[cri-o]] — CRI-O
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd


<!-- risk-assessed -->
