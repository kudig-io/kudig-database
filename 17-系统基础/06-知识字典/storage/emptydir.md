---
title: 空目录卷
description: emptyDir 是一种临时存储卷，在 Pod 被分配到节点时创建，Pod 从节点移除时数据永久丢失。适用于 Pod 内容器间的临时数据共享。...
summary: emptyDir 是一种临时存储卷，在 Pod 被分配到节点时创建，Pod 从节点移除时数据永久丢失。适用于 Pod 内容器间的临时数据共享。...
category: dictionary
tags:
- k8s
- glossary
- storage
- emptydir
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 空目录卷 是什么
- emptyDir 详解
trigger_keywords:
- 空目录卷
- emptyDir
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 空目录卷

> **英文名**: emptyDir

## 概述

emptyDir 是一种临时存储卷，在 Pod 被分配到节点时创建，Pod 从节点移除时数据永久丢失。适用于 Pod 内容器间的临时数据共享。

## 核心概念/原理

### 核心特性

- **生命周期**：与 Pod 绑定。Pod 删除 → emptyDir 数据丢失。
- **容器间共享**：Pod 中多个容器可以挂载同一个 emptyDir。
- **内存模式**：`medium: Memory` 使用 tmpfs（RAM），速度更快但受内存限制。

### 示例

```yaml
volumes:
- name: scratch
  emptyDir:
    sizeLimit: 1Gi
- name: cache
  emptyDir:
    medium: Memory
    sizeLimit: 512Mi
```

## 关键机制或特性

- emptyDir 的默认存储介质是节点的本地磁盘。
- `sizeLimit` 限制 emptyDir 的最大容量。
- Memory 类型的 emptyDir 计入容器的内存使用。

## 使用场景与最佳实践

- 用作多容器 Pod 的共享工作区。
- 存放崩溃恢复所需的检查点数据。
- 大文件处理的暂存目录。
- 不要用于需要持久化的数据。

## 参考链接

- [emptyDir - Official Documentation](https://kubernetes.io/docs/concepts/storage/volumes/#emptydir)

## Related

- [[17-系统基础/06-知识字典/storage/persistent-volume.md|Persistent Volume]]
- [[17-系统基础/06-知识字典/storage/persistent-volume-claim.md|Persistent Volume Claim]]
- [[17-系统基础/06-知识字典/storage/storage-class.md|Storage Class]]
- [[17-系统基础/06-知识字典/storage/volume.md|Volume]]
- [[17-系统基础/06-知识字典/storage/hostpath.md|Hostpath]]


<!-- risk-assessed -->
