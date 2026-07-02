---
title: Datadog
description: Datadog 是企业级全栈可观测性 SaaS 平台，提供 Metrics、Logs、Traces、APM、Security 等一站式功能。在云原生环境中，Da...
summary: Datadog 是企业级全栈可观测性 SaaS 平台，提供 Metrics、Logs、Traces、APM、Security 等一站式功能。在云原生环境中，Da...
category: dictionary
tags:
- k8s
- glossary
- datadog
- observability
- saas
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Datadog 是什么
- Datadog 详解
trigger_keywords:
- Datadog
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Datadog

> **英文名**: Datadog

## 概述

Datadog 是企业级全栈可观测性 SaaS 平台，提供 Metrics、Logs、Traces、APM、Security 等一站式功能。在云原生环境中，Datadog Agent 部署为 DaemonSet 采集集群的指标、日志和追踪数据。

## 核心概念/原理

### 核心产品

| 产品 | 功能 |
|------|------|
| Infrastructure | 指标收集和可视化 |
| APM | 分布式追踪 |
| Logs | 日志管理 |
| RUM | 真实用户体验监控 |
| Synthetics | API/浏览器测试 |
| Security | 运行时威胁检测 |

### 与开源方案对比

| 特性 | Datadog | 开源（Prom+Grafana+Loki+Tempo） |
|------|---------|--------------------------------|
| 部署 | SaaS | 自建 |
| 成本 | 高（按主机计费） | 低（硬件成本） |
| 维护 | 零运维 | 需运维 |

## 关键机制或特性

- **Datadog Agent**：DaemonSet 部署，采集 Metrics + Logs + Traces。
- **Cluster Agent**：集群级事件和外部指标。
- **Autodiscovery**：自动发现新 Pod 并配置采集。
- **DogStatsD**：兼容 StatsD 的指标聚合代理。
- 丰富的集成（500+）：K8s、Istio、Redis、PostgreSQL 等。

## 使用场景与最佳实践

- 企业有预算且希望零运维可观测性时选择 Datadog。
- 使用 Datadog Agent DaemonSet 自动采集集群数据。
- 配置 Autodiscovery 自动为新 Pod 启用监控。
- 使用 Datadog 的 APM 替代自建的 Jaeger/Tempo。
- 注意成本控制：合理设置指标保留期和日志采样率。

## 参考链接

- [Datadog Official](https://www.datadoghq.com/)

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/grafana.md|Grafana]]
- [[domain-17-system-foundation/topic-dictionary/observability/loki.md|Loki]]
- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry.md|OpenTelemetry]]
- [[domain-17-system-foundation/topic-dictionary/observability/alertmanager.md|Alertmanager]]


<!-- risk-assessed -->
