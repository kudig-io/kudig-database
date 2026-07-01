---
title: 告警管理器
description: 'Alertmanager 是 Prometheus 生态中的告警处理组件。它接收来自 Prometheus 的告警，执行分组、抑制、静默和路由逻辑，最终通过多种...'
category: dictionary
tags:
- k8s
- glossary
- observability
- alertmanager
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 告警管理器 是什么
- Alertmanager 详解
trigger_keywords:
- 告警管理器
- Alertmanager
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 告警管理器

> **英文名**: Alertmanager

## 概述

Alertmanager 是 Prometheus 生态中的告警处理组件。它接收来自 Prometheus 的告警，执行分组、抑制、静默和路由逻辑，最终通过多种渠道发送通知。

## 核心概念/原理

### 核心功能

- **分组（Grouping）**：将相关告警合并为一条通知。
- **抑制（Inhibition）**：当某个告警触发时，抑制相关的衍生告警。
- **静默（Silencing）**：临时静默特定告警（维护期间使用）。
- **路由（Routing）**：基于标签将告警发送到不同的通知渠道。

### 通知渠道

支持 Email、Slack、PagerDuty、Webhook、OpsGenie、VictorOps 等。

## 关键机制或特性

- 告警规则定义在 Prometheus 中，告警处理在 Alertmanager 中。
- 支持多 Alertmanager 实例的集群模式（去重）。
- `amtool` CLI 工具用于管理静默和查看告警。

## 使用场景与最佳实践

- 配置合理的分组规则避免告警风暴。
- 使用抑制规则减少噪音告警。
- 为 P0 告警配置 PagerDuty/电话通知。
- 定期审查告警规则，清理无效告警。

## 参考链接

- [Alertmanager - Official Documentation](https://prometheus.io/docs/alerting/latest/alertmanager/)

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/grafana.md|Grafana]]
- [[domain-17-system-foundation/topic-dictionary/observability/metrics-server.md|Metrics Server]]
- [[domain-17-system-foundation/topic-dictionary/observability/kubernetes-events.md|Kubernetes Events]]
- [[domain-17-system-foundation/topic-dictionary/observability/logging.md|Logging]]
