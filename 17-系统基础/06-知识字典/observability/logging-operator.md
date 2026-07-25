---
title: Logging Operator 日志路由
description: Logging Operator 是 Kube Logging（原 Banzai Cloud）开源的 CNCF Sandbox 项目，通过
  Operator 模...
summary: Logging Operator 是 Kube Logging（原 Banzai Cloud）开源的 CNCF Sandbox 项目，通过 Operator
  模...
category: dictionary
tags:
- k8s
- glossary
- observability
- logging
- operator
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Logging Operator 日志路由 是什么
- Logging Operator 详解
trigger_keywords:
- Logging Operator 日志路由
- Logging Operator
- dictionary
prerequisites:
- kubernetes
---



# Logging Operator 日志路由（Logging Operator）

## 概述

Logging Operator 是 Kube Logging（原 Banzai Cloud）开源的 CNCF Sandbox 项目，通过 Operator 模式管理 Kubernetes 日志采集和路由，统一 Fluent Bit + Fluentd/Flame 的部署和配置。

## 核心概念/原理

- **Operator 模式**：CRD 管理日志采集和路由
- **双层架构**：Fluent Bit（采集）+ Fluentd/Syslog-NG（聚合）
- **CNCF Sandbox**：Kube Logging 社区主导
- **多租户**：支持日志的租户隔离和路由

## 关键机制或特性

- Logging CRD 定义日志基础设施
- Flow / ClusterFlow 定义日志路由
- Output / ClusterOutput 定义日志目标
- 自动部署 Fluent Bit DaemonSet
- 支持 Fluentd 和 Syslog-NG 后端
- 日志过滤和转换（Filter）
- 多租户日志隔离（Tenant CRD）

## 使用场景与最佳实践

- K8s 日志的统一采集和路由
- 多租户环境的日志隔离
- 日志转发到多种后端（ES/Loki/S3/Kafka）
- 日志格式化和过滤
- Fluentd/Fluent Bit 的自动化运维

## 参考链接

- https://kube-logging.dev/
- https://github.com/kube-logging/logging-operator

## Related

- [[17-系统基础/06-知识字典/observability/fluentd.md|Fluentd]]
- [[17-系统基础/06-知识字典/observability/loki.md|Loki]]
- [[17-系统基础/06-知识字典/observability/opentelemetry.md|OpenTelemetry]]
