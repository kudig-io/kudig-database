---
title: 主机路径卷
description: 'hostPath 卷将节点文件系统的文件或目录挂载到 Pod 中。它提供了对节点文件系统的直接访问，但存在安全和可移植性风险。...'
category: dictionary
tags:
- k8s
- glossary
- storage
- hostpath
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 主机路径卷 是什么
- hostPath 详解
trigger_keywords:
- 主机路径卷
- hostPath
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 主机路径卷

> **英文名**: hostPath

## 概述

hostPath 卷将节点文件系统的文件或目录挂载到 Pod 中。它提供了对节点文件系统的直接访问，但存在安全和可移植性风险。

## 核心概念/原理

### 核心特性

- **直接访问**：Pod 可以直接读写节点文件系统的指定路径。
- **类型检查**：`type` 字段可以指定挂载前需要进行的检查（如 DirectoryExists、FileOrCreate）。

### 安全风险

- Pod 可以访问节点上的敏感文件（如 `/etc/shadow`）。
- 不同节点的文件系统结构可能不同，导致 Pod 不可移植。
- 恶意 Pod 可能修改节点关键文件。

## 关键机制或特性

- hostPath 是少数几种允许容器访问宿主机的 Volume 类型。
- 与 `privileged: true` 组合使用时风险更高。
- PodSecurityStandards 的 `baseline` 和 `restricted` 级别限制 hostPath 使用。

## 使用场景与最佳实践

- **仅在系统级 DaemonSet 中使用**（如日志收集、监控代理）。
- 应用工作负载不应使用 hostPath。
- 使用 `readOnly: true` 减少安全风险。
- 考虑使用 PV/PVC 替代 hostPath。

## 参考链接

- [hostPath - Official Documentation](https://kubernetes.io/docs/concepts/storage/volumes/#hostpath)

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume-claim.md|Persistent Volume Claim]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-class.md|Storage Class]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume.md|Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/emptydir.md|Emptydir]]
