---
title: 告警管理器
description: Alertmanager 是 Prometheus 生态中的告警处理组件。它接收来自 Prometheus 的告警，执行分组、抑制、静默和路由逻辑，最终通过多种...
summary: Alertmanager 是 Prometheus 生态中的告警处理组件。它接收来自 Prometheus 的告警，执行分组、抑制、静默和路由逻辑，最终通过多种...
category: dictionary
tags:
- k8s
- glossary
- observability
- alertmanager
tier: core
created: '2026-06-24'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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


<!-- risk-assessed -->
