---
title: 日志体系
description: Kubernetes 日志体系涵盖容器标准输出、节点级日志采集和集中式日志平台的完整链路，是可观测性三大支柱之一，为故障诊断和安全审计提供基础数据。...
summary: Kubernetes 日志体系涵盖容器标准输出、节点级日志采集和集中式日志平台的完整链路，是可观测性三大支柱之一，为故障诊断和安全审计提供基础数据。...
category: dictionary
tags:
- k8s
- glossary
- observability
- logging
- fluentd
- loki
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 日志体系 是什么
- Logging 详解
trigger_keywords:
- 日志体系
- Logging
- dictionary
prerequisites:
- kubernetes
---



# 日志体系（Logging）

## 概述

Kubernetes 日志体系涵盖容器标准输出、节点级日志采集和集中式日志平台的完整链路，是可观测性三大支柱之一，为故障诊断和安全审计提供基础数据。

## 核心概念/原理

- **三层架构**：应用日志 → 节点采集 → 集中存储
- **标准输出**：stdout/stderr 被容器运行时捕获
- **日志采集**：Fluent Bit/Fluentd/Promtail 等
- **日志存储**：Elasticsearch/Loki/S3 等

## 关键机制或特性

- 容器 stdout/stderr → /var/log/containers/*.log
- 节点级日志采集 DaemonSet
- Fluent Bit（轻量采集）+ Fluentd（聚合路由）
- OpenTelemetry Collector（统一采集）
- 结构化日志（JSON）优于纯文本
- 日志索引和保留策略
- 日志关联（trace_id/span_id 贯穿链路）

## 使用场景与最佳实践

- 应用错误的快速定位
- 安全审计和合规日志
- 性能问题的日志分析
- 多租户日志隔离
- 最佳实践：结构化 JSON、保留策略、日志级别控制、敏感信息脱敏

## 参考链接

- https://kubernetes.io/docs/concepts/cluster-administration/logging/
- https://opentelemetry.io/

## Related

- [[17-系统基础/06-知识字典/observability/fluentd.md|Fluentd]]
- [[17-系统基础/06-知识字典/observability/loki.md|Loki]]
- [[17-系统基础/06-知识字典/observability/logging-operator.md|Logging Operator]]
