---
sources:
- "可观测性/指标/19-kube-state-metrics-deep-dive.md"
title: kube-state-metrics 深度解析
summary: 解析 kube-state-metrics 在可观测性 pipeline 中的角色、指标模型、分片架构与生产配置。
category: concepts
tags:
- kube-state-metrics
- ksm
- metrics
- observability
- prometheus
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 可观测性工程师
estimated_read_time: 20min
intent_queries:
- kube-state-metrics 是什么
- kube-state-metrics 与 Metrics Server 区别
- KSM 指标有哪些
- KSM 如何分片
trigger_keywords:
- kube-state-metrics
- KSM
- 状态指标
- metrics pipeline
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **内容已并入主场**：[[wiki/08-可观测性/01-指标/11-kube-state-metrics-deep-dive.md|kube-state-metrics 深度解析]]（本页为重排去重桩，正文见主场）
