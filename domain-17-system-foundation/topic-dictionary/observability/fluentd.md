---
title: Fluentd 日志收集
description: 'Fluentd 是 CNCF 毕业项目，统一日志收集层，支持 500+ 插件连接各种数据源和目标，是 Kubernetes 环境中日志收集的事实标准之一。...'
category: dictionary
tags:
- k8s
- glossary
- observability
- logging
- cnCF
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Fluentd 日志收集 是什么
- Fluentd 详解
trigger_keywords:
- Fluentd 日志收集
- Fluentd
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Fluentd 日志收集（Fluentd）

## 概述

Fluentd 是 CNCF 毕业项目，统一日志收集层，支持 500+ 插件连接各种数据源和目标，是 Kubernetes 环境中日志收集的事实标准之一。

## 核心概念/原理

- **统一日志层**：在应用和数据存储之间提供统一的日志收集和处理层
- **插件生态**：500+ 社区插件覆盖几乎所有日志源和目标
- **JSON 优先**：默认使用 JSON 格式处理日志，便于结构化查询
- **CNCF 毕业项目**：经过大规模生产验证

## 关键机制或特性

- Tag 驱动的事件路由（`<match>` / `<filter>` 配置）
- Buffer 机制确保可靠传输（文件 + 内存双层缓冲）
- 高可用模式（forward 协议集群间传输）
- Fluent Bit 作为轻量采集端 + Fluentd 作为聚合端的分层架构
- Kubernetes 元数据自动富化
- 日志解析（parse）插件支持多行日志、正则、JSON 等

## 使用场景与最佳实践

- Kubernetes 集群统一日志收集
- 日志转发到 Elasticsearch/OpenSearch/Loki
- 日志过滤、脱敏、格式转换
- 多租户日志隔离和路由
- 边缘场景下使用 Fluent Bit 替代

## 参考链接

- https://www.fluentd.org/
- https://github.com/fluent/fluentd

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/loki.md|Loki]]
- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry.md|OpenTelemetry]]
