---
title: OpenKruise 增强工作负载
description: 'OpenKruise 是阿里巴巴开源的 CNCF 孵化项目，为 Kubernetes 提供增强型工作负载管理能力，包括原地升级、Sidecar 管理、镜像预热等...'
category: dictionary
tags:
- k8s
- glossary
- workloads
- operator
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenKruise 增强工作负载 是什么
- OpenKruise 详解
trigger_keywords:
- OpenKruise 增强工作负载
- OpenKruise
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# OpenKruise 增强工作负载（OpenKruise）

## 概述

OpenKruise 是阿里巴巴开源的 CNCF 孵化项目，为 Kubernetes 提供增强型工作负载管理能力，包括原地升级、Sidecar 管理、镜像预热等原生 K8s 缺失的高级功能。

## 核心概念/原理

- **增强工作负载**：扩展 K8s 原生工作负载的能力边界
- **生产验证**：阿里巴巴大规模生产环境使用
- **CNCF 孵化**：活跃的增强工作负载社区
- **兼容原生**：不替换而是增强，与原生 K8s 资源互补

## 关键机制或特性

- CloneSet：增强版 Deployment（支持原地升级、指定删除、分批发布）
- Advanced StatefulSet：增强版 StatefulSet（原地升级、无序扩缩）
- SidecarSet：统一管理 Sidecar 容器注入和升级
- NodeImage / ImagePullJob：镜像预热和按需拉取
- ResourceDistribution：跨命名空间资源分发
- Advanced DaemonSet：增强版 DaemonSet

## 使用场景与最佳实践

- 大规模集群的工作负载管理
- 需要原地升级（不重建 Pod）的场景
- Sidecar 容器的统一管理和升级
- 镜像预热加速大规模扩容
- 分批发布和金丝雀发布的精细化控制

## 参考链接

- https://openkruise.io/
- https://github.com/openkruise/kruise

## Related

- [[domain-17-system-foundation/topic-dictionary/workloads/deployment.md|Deployment]]
- [[domain-17-system-foundation/topic-dictionary/workloads/statefulset.md|StatefulSet]]
- [[domain-17-system-foundation/topic-dictionary/workloads/daemonset.md|DaemonSet]]
