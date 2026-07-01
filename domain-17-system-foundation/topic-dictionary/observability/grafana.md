---
title: Grafana
description: 'Grafana 是开源的数据可视化平台，支持丰富的图表类型和数据源集成。在 Kubernetes 生态中，Grafana 通常与 Prometheus 搭配使用...'
category: dictionary
tags:
- k8s
- glossary
- observability
- grafana
- monitoring
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Grafana 是什么
- Grafana 详解
trigger_keywords:
- Grafana
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# Grafana

> **英文名**: Grafana

## 概述

Grafana 是开源的数据可视化平台，支持丰富的图表类型和数据源集成。在 Kubernetes 生态中，Grafana 通常与 Prometheus 搭配使用，用于展示监控指标和构建运维仪表盘。

## 核心概念/原理

### 核心功能

- **仪表盘**：支持时间序列图、表格、热力图、拓扑图等多种可视化。
- **数据源**：Prometheus、Loki、Tempo、Elasticsearch、MySQL 等 100+ 数据源。
- **告警**：内置告警引擎，支持多通道通知。
- **Dashboard as Code**：仪表盘可以用 JSON 定义并纳入版本控制。

### Kubernetes 常用仪表盘

- Kubernetes / Compute Resources / Cluster
- Kubernetes / Compute Resources / Namespace (Pods)
- Node Exporter / Nodes
- CoreDNS
- kube-state-metrics

## 关键机制或特性

- Grafana 支持 Provisioning 自动化配置数据源和仪表盘。
- 社区提供大量预制的 Kubernetes 仪表盘（Grafana Labs 官方库）。
- Grafana OnCall 集成告警管理和值班调度。

## 使用场景与最佳实践

- 使用 kube-prometheus-stack 一键部署 Grafana + Prometheus。
- 导入社区推荐的 Kubernetes 仪表盘。
- 配置 Provisioning 自动化仪表盘管理。
- 设置关键 SLI 的告警仪表盘。

## 参考链接

- [Grafana - Official Documentation](https://grafana.com/docs/)

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/alertmanager.md|Alertmanager]]
- [[domain-17-system-foundation/topic-dictionary/observability/metrics-server.md|Metrics Server]]
- [[domain-17-system-foundation/topic-dictionary/observability/kubernetes-events.md|Kubernetes Events]]
- [[domain-17-system-foundation/topic-dictionary/observability/logging.md|Logging]]
