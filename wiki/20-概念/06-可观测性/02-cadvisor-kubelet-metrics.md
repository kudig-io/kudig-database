---
sources:
- "可观测性/指标/20-cadvisor-kubelet-metrics.md"
title: cAdvisor 与 kubelet 指标采集
summary: 解析 kubelet 内置 cAdvisor 的三个 metrics 端点、容器级资源指标分类与 Prometheus 采集配置。
category: concepts
tags:
- cadvisor
- kubelet
- metrics
- container-metrics
- resource-usage
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
- cAdvisor 是什么
- kubelet metrics 端点有哪些
- 如何采集容器 CPU 内存指标
- container_cpu 与 container_fs 指标含义
trigger_keywords:
- cAdvisor
- kubelet metrics
- /metrics/cadvisor
- 容器指标
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **内容已并入主场**：[[wiki/08-可观测性/01-指标/12-cadvisor-kubelet-metrics.md|cAdvisor 与 kubelet 指标采集]]（本页为重排去重桩，正文见主场）
