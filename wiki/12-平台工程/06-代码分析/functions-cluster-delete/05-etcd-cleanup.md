---
title: etcd 数据清理与成员移除 — 源码分析 (topic-code-analysis)
description: '## 概述'
summary: '阶段的 etcd 类型检测、数据目录获取、RemoveStackedEtcdMemberFromCluster 成员移除逻辑、唯一成员特殊处理以及外部 etcd'
category: general
tags:
- reference
- etcd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- etcd 数据清理与成员移除 — 源码分析 是什么
- 如何 etcd 数据清理与成员移除 — 源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- etcd
- 数据清理与成员移除
- 源码分析
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
---

> **内容已并入主场**：[[wiki/12-平台工程/06-代码分析/cluster-delete/05-etcd-cleanup.md|etcd 数据清理与成员移除 — 源码分析 (topic-code-analysis)]]（本页为重排去重桩，正文见主场）
