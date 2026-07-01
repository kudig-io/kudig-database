---
title: ComposeFS 只读文件系统
description: 'ComposeFS 是 Linux 内核的只读文件系统，基于内容寻址（content-addressed）存储，为容器镜像和不可变系统提供安全、高效的文件访问，...'
category: dictionary
tags:
- k8s
- glossary
- storage
- filesystem
- security
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- ComposeFS 只读文件系统 是什么
- ComposeFS 详解
trigger_keywords:
- ComposeFS 只读文件系统
- ComposeFS
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# ComposeFS 只读文件系统（ComposeFS）

## 概述

ComposeFS 是 Linux 内核的只读文件系统，基于内容寻址（content-addressed）存储，为容器镜像和不可变系统提供安全、高效的文件访问，与 OSTree 和 Podman 深度集成。

## 核心概念/原理

- **内容寻址**：基于文件内容哈希的去重存储
- **只读安全**：不可修改的文件系统，防止运行时篡改
- **内核级**：Linux 内核模块，性能优异
- **容器优化**：Podman/Buildah 的镜像存储后端

## 关键机制或特性

- 基于 EROFS 的只读文件系统
- 文件级去重（相同内容共享存储）
- fs-verity 完整性验证
- 与 OSTree 集成（Flatcar/Fedora CoreOS）
- Podman ComposeFS 存储驱动
- 支持 Overlayfs 作为底层

## 使用场景与最佳实践

- 不可变容器的安全文件系统
- 容器镜像的存储优化（去重）
- 不可变基础设施的根文件系统
- 安全合规环境的防篡改存储
- 大规模镜像拉取的性能优化

## 参考链接

- https://github.com/containers/composefs
- https://docs.kernel.org/filesystems/composefs.html

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/podman.md|Podman]]
- [[domain-17-system-foundation/topic-dictionary/tooling/bootc.md|bootc]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/containerd.md|containerd]]
