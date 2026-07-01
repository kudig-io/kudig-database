---
title: 容器存储接口
description: CSI（Container Storage Interface）是存储插件的标准接口规范。它定义了存储系统如何与容器编排系统集成的标准化方式，取代了
  Kuber...
summary: CSI（Container Storage Interface）是存储插件的标准接口规范。它定义了存储系统如何与容器编排系统集成的标准化方式，取代了
  Kuber...
category: dictionary
tags:
- k8s
- glossary
- storage
- csi
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器存储接口 是什么
- CSI (Container Storage Interface) 详解
trigger_keywords:
- 容器存储接口
- CSI (Container Storage Interface)
- dictionary
prerequisites:
- kubectl-basics
---



# 容器存储接口

> **英文名**: CSI (Container Storage Interface)

## 概述

CSI（Container Storage Interface）是存储插件的标准接口规范。它定义了存储系统如何与容器编排系统集成的标准化方式，取代了 Kubernetes 早期的 in-tree 存储插件。

## 核心概念/原理

### 核心概念

- **CSI Driver**：实现 CSI 接口的存储驱动程序，通常由存储厂商提供。
- **Controller Plugin**：处理卷的创建/删除/快照等控制操作（运行在任意节点）。
- **Node Plugin**：处理卷的挂载/卸载和格式化（运行在每个节点，通常以 DaemonSet 部署）。

### CSI 操作

- `CreateVolume` / `DeleteVolume`：卷的生命周期管理。
- `ControllerPublishVolume` / `ControllerUnpublishVolume`：卷的附加/分离。
- `NodeStageVolume` / `NodePublishVolume`：卷的格式化/挂载。
- `CreateSnapshot` / `DeleteSnapshot`：快照管理。

## 关键机制或特性

- CSI 驱动通过 `CSIDriver` 和 `CSINode` 对象注册到 Kubernetes。
- 支持动态供给、卷快照、卷克隆、卷扩容等高级功能。
- CSI 驱动与 Kubernetes 版本有兼容性要求。

## 使用场景与最佳实践

- 选择经过认证的 CSI 驱动，确保与 Kubernetes 版本兼容。
- 测试 CSI 驱动在高负载下的性能和稳定性。
- 监控 CSI 操作的延迟和成功率。

## 参考链接

- [CSI (Container Storage Interface) - Official Documentation](https://kubernetes-csi.github.io/docs/)

## Related

[[entities/csi-drivers.md|CSI Drivers]]
