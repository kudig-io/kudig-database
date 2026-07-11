---
title: composefs (entities)
description: '## 概述'
summary: 'composefs 是一个 Linux 文件系统，设计用于高效挂载和共享容器镜像层。它结合了 EROFS（只读文件系统）作为元数据存储和 fs-verity 提供内容校验，实现了容器镜像的可验证挂载。composefs 允许多个容器镜像共享相同内容的文件块（基于内容寻址的对象存储），大幅减少磁盘空间占用，同时通过 fs-verity 确保镜像内容的完整性。'
category: entities
tags:
- k8s
- cncf
- runtime
- composefs
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- composefs 是什么
- 如何 composefs
trigger_keywords:
- composefs
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# composefs

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: C

## 概述

ComposeFS 是一个 Linux 文件系统项目，由 Red Hat 和 GNOME 社区推动开发，旨在提供高性能的容器镜像挂载方案。它通过将只读文件系统元数据（来自容器镜像）与底层内容寻址存储（CAS）分离，实现快速镜像挂载和高效存储利用。ComposeFS 使容器镜像可以以只读方式直接挂载，无需解包到 OverlayFS 上层，大幅减少存储空间和启动时间。它与 OCI 镜像格式兼容，特别适合大规模容器部署。

## Key Features（核心能力）

- **快速挂载**：容器镜像无需解包即可直接挂载，显著减少启动时间
- **内容寻址**：基于 fs-verity 的文件完整性校验
- **高效存储**：多个镜像共享相同的底层文件层，避免重复存储
- **OverlayFS 兼容**：可作为 OverlayFS 的 lower layer 使用
- **安全增强**：通过 fs-verity 提供文件级完整性保护
- **与 OCI 兼容**：支持标准 OCI 镜像格式

## 架构与工作原理

ComposeFS 由两部分组成：ComposeFS 元数据文件描述了文件系统的目录树结构（权限、文件名等），但不包含文件内容；文件内容存储在底层的内容寻址存储（CAS）中，通常是一个目录，文件名以内容的 SHA-256 哈希命名。挂载时，内核 ComposeFS 驱动读取元数据文件，引用 CAS 中的文件内容，构建虚拟的只读文件系统视图。底层文件通过 fs-verity 自动验证完整性。

## K8s 集成

ComposeFS 与 containerd 集成，作为镜像快照ter（Snapshotter）。Pod 创建时，containerd 不再需要将镜像层解包到 OverlayFS，而是直接通过 ComposeFS 挂载只读层。这减少了磁盘 I/O 和存储空间使用。在 K8s 节点上，所有 Pod 共享同一份 CAS 存储，相同文件只需存储一次。

## 生产用例

- **大规模容器部署**：数千 Pod 集群的镜像存储优化
- **快速启动**：通过直接挂载减少容器启动时间
- **安全加固**：利用 fs-verity 提供镜像文件完整性保护
- **边缘计算**：在存储受限的边缘节点上高效运行容器

## 安装与快速开始

```bash
# 编译安装
modprobe composefs
mount.composefs metadata.cfs /mnt/composefs -o basedir=/var/lib/cas
# containerd 集成
# 配置 containerd 使用 composefs snapshotter
```

## 对比替代方案

相比 OverlayFS（需要解包镜像层），ComposeFS 实现零拷贝挂载，存储效率更高。相比 Stargz/SOCI（Lazy Pulling），ComposeFS 提供完全的本地挂载体验，不存在首次访问延迟。

## Related

- [[containerssh]] — ContainerSSH
- [[modelpack]] — ModelPack
- [[oauth2-proxy]] — OAuth2 Proxy
- [[schemahero]] — SchemaHero
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- composefs
- [[实体/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference


<!-- risk-assessed -->
