---
title: Kepler 能耗监控
description: Kepler（Kubernetes Efficient Power Level Exporter）是 CNCF Sandbox 项目，通过
  eBPF 和 CPU...
summary: Kepler（Kubernetes Efficient Power Level Exporter）是 CNCF Sandbox 项目，通过 eBPF
  和 CPU...
category: dictionary
tags:
- k8s
- glossary
- observability
- energy
- sustainability
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kepler 能耗监控 是什么
- Kepler 详解
trigger_keywords:
- Kepler 能耗监控
- Kepler
- dictionary
prerequisites:
- kubernetes
---



# Kepler 能耗监控（Kepler）

## 概述

Kepler（Kubernetes Efficient Power Level Exporter）是 CNCF Sandbox 项目，通过 eBPF 和 CPU 模型估算 Kubernetes 中每个 Pod 的能耗，导出为 Prometheus 指标，支持绿色计算和碳足迹追踪。

## 核心概念/原理

- **能耗估算**：通过 eBPF 采集 CPU/DRAM/GPU 能耗指标
- **Pod 粒度**：将节点级能耗拆分到 Pod 级别
- **Prometheus 导出**：标准 Prometheus metrics 格式
- **CNCF Sandbox**：Red Hat/IBM 主导的绿色计算项目

## 关键机制或特性

- eBPF 采集 CPU C-state 和能耗计数器
- 基于机器学习模型的能耗估算（RAPL + Model）
- GPU 能耗采集（NVIDIA DCGM）
- Kepler Dashboard（Grafana 预置看板）
- 碳排放计算（结合区域电力碳强度数据）
- OpenTelemetry 集成

## 使用场景与最佳实践

- 数据中心碳足迹追踪
- Kubernetes 集群的能耗优化
- 绿色计算和可持续发展报告
- 成本核算中的能耗分摊
- 工作负载的能效对比（Perf/Watt）

## 参考链接

- https://sustainable-computing.io/
- https://github.com/sustainable-computing-io/kepler

## Related

- [[系统基础/知识字典/observability/prometheus.md|Prometheus]]
- [[系统基础/知识字典/observability/opentelemetry.md|OpenTelemetry]]
- [[系统基础/知识字典/observability/grafana.md|Grafana]]
