---
sources:
- "存储/K8s存储/20-csi-storage-capacity-tracking.md"
title: CSIStorageCapacity 与存储容量感知调度
summary: 解析 CSIStorageCapacity 对象如何让 kube-scheduler 在拓扑感知下避免将 Pod 调度到存储不足的节点。
category: concepts
tags:
- csistoragecapacity
- capacity-tracking
- scheduling
- csi
- topology
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 存储架构师
- SRE
- CSI 驱动开发者
estimated_read_time: 15min
intent_queries:
- CSIStorageCapacity 是什么
- 如何让调度器感知存储容量
- WaitForFirstConsumer 与容量
trigger_keywords:
- CSIStorageCapacity
- capacity
- 容量感知
- topology
- WaitForFirstConsumer
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **内容已并入主场**：[[wiki/06-存储/01-K8s存储/21-csi-storage-capacity-tracking.md|CSIStorageCapacity 与存储容量感知调度]]（本页为重排去重桩，正文见主场）
