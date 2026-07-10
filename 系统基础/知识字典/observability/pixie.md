---
title: Pixie 自动可观测性
description: Pixie 是 New Relic 开源的 CNCF Sandbox 项目，利用 eBPF 技术实现 Kubernetes 应用的零插桩自动可观测性，无需修改应...
summary: Pixie 是 New Relic 开源的 CNCF Sandbox 项目，利用 eBPF 技术实现 Kubernetes 应用的零插桩自动可观测性，无需修改应...
category: dictionary
tags:
- k8s
- glossary
- observability
- ebpf
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pixie 自动可观测性 是什么
- Pixie 详解
trigger_keywords:
- Pixie 自动可观测性
- Pixie
- dictionary
prerequisites:
- kubernetes
---



# Pixie 自动可观测性（Pixie）

## 概述

Pixie 是 New Relic 开源的 CNCF Sandbox 项目，利用 eBPF 技术实现 Kubernetes 应用的零插桩自动可观测性，无需修改应用代码即可获取请求追踪、性能指标和日志。

## 核心概念/原理

- **eBPF 驱动**：自动采集应用级指标，无需代码改造
- **零插桩**：自动追踪 HTTP/gRPC/MySQL/Redis 等协议
- **本地分析**：数据在集群内处理，无需外传
- **CNCF Sandbox**：New Relic 主导

## 关键机制或特性

- Auto-telemetry：自动追踪 HTTP/gRPC/DNS/MySQL/PostgreSQL/Redis/Kafka
- PxL 查询语言（类似 SQL 的数据查询）
- Pixie Live View 实时数据查看
- 脚本化分析（Scripted Analysis）
- 数据保留策略（默认 24h 热数据）
- 与 OpenTelemetry 导出集成

## 使用场景与最佳实践

- 无代码改造的应用可观测性
- 微服务的请求级追踪
- 性能瓶颈的快速定位
- 遗留系统的可观测性接入
- 开发环境的实时调试

## 参考链接

- https://px.dev/
- https://github.com/pixie-io/pixie

## Related

- [[系统基础/知识字典/observability/opentelemetry.md|OpenTelemetry]]
- [[系统基础/知识字典/observability/jaeger.md|Jaeger]]
- [[系统基础/知识字典/networking/cilium.md|Cilium]]
