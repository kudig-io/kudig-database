---
title: Mimir
description: Grafana Mimir 是 Grafana Labs 开源的大规模 Prometheus 兼容指标存储和查询系统。它是 Cortex
  的下一代替代品，提供水...
summary: Grafana Mimir 是 Grafana Labs 开源的大规模 Prometheus 兼容指标存储和查询系统。它是 Cortex 的下一代替代品，提供水...
category: dictionary
tags:
- k8s
- glossary
- mimir
- prometheus
- observability
- grafana
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Mimir 是什么
- Mimir 详解
trigger_keywords:
- Mimir
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Mimir

> **英文名**: Mimir

## 概述

Grafana Mimir 是 Grafana Labs 开源的大规模 Prometheus 兼容指标存储和查询系统。它是 Cortex 的下一代替代品，提供水平扩展、多租户和长期指标存储能力。

## 核心概念/原理

### 与 Cortex/Thanos 对比

| 特性 | Mimir | Cortex | Thanos |
|------|-------|--------|--------|
| 状态 | 活跃开发 | 维护模式 | 活跃 |
| 架构 | 单体微服务混合 | 纯微服务 | Sidecar |
| 查询 | PromQL 兼容 | PromQL 兼容 | PromQL 兼容 |
| 多租户 | 原生 | 原生 | 需额外 |

### 核心组件

Distributor、Ingester、Querier、Query-Frontend、Compactor、Store-Gateway、Ruler。

## 关键机制或特性

- **水平扩展**：每个组件可独立扩缩容。
- **PromQL 兼容**：完全兼容 Prometheus 查询语言。
- **Ruler**：分布式规则评估和告警。
- **对象存储**：TSDB 数据存储在 S3/GCS/MinIO。
- 支持 Remote Write 接收指标数据。

## 使用场景与最佳实践

- 大规模 Prometheus 部署使用 Mimir 替代 Thanos。
- 多租户环境使用 Mimir 的租户隔离功能。
- 配合 Grafana 构建统一的指标可视化。
- 使用 Remote Write 将多个 Prometheus 实例的数据汇聚到 Mimir。
- 配置 Compactor 的保留策略管理存储成本。

## 参考链接

- [Mimir Official](https://grafana.com/oss/mimir/)

## Related

- [[系统基础/知识字典/observability/prometheus.md|Prometheus]]
- [[系统基础/知识字典/observability/thanos.md|Thanos]]
- [[系统基础/知识字典/observability/promql.md|PromQL]]
- [[系统基础/知识字典/observability/grafana.md|Grafana]]
- [[系统基础/知识字典/observability/alertmanager.md|Alertmanager]]


<!-- risk-assessed -->
