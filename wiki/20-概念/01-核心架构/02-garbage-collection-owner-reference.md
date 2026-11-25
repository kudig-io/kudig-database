---
sources:
- "集群基础/设计原则/19-garbage-collection-owner-reference.md"
title: 垃圾回收与 ownerReference 级联删除机制
summary: 深入解析 Kubernetes Garbage Collector 的依赖图、级联删除状态机与 ownerReference 协同机制。
category: concepts
tags:
- garbage-collection
- owner-reference
- cascading-deletion
- finalizer
- controller
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 核心贡献者
estimated_read_time: 25min
intent_queries:
- Kubernetes 垃圾回收机制是什么
- ownerReference 级联删除三种策略区别
- Garbage Collector 依赖图如何工作
trigger_keywords:
- 垃圾回收
- 级联删除
- ownerReference
- Garbage Collector
- finalizer
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **内容已并入主场**：[[wiki/01-集群基础与系统/02-设计原则/20-garbage-collection-owner-reference.md|垃圾回收与 ownerReference 级联删除机制]]（本页为重排去重桩，正文见主场）
