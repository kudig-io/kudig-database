---
title: SlimFaas 轻量 FaaS
description: 'SlimFaas 是 Axa France 开源的超轻量级 Kubernetes FaaS（Function as a Service）平台，以极低的复杂度和资...'
category: dictionary
tags:
- k8s
- glossary
- workloads
- serverless
- faas
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SlimFaas 轻量 FaaS 是什么
- SlimFaas 详解
trigger_keywords:
- SlimFaas 轻量 FaaS
- SlimFaas
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# SlimFaas 轻量 FaaS（SlimFaas）

## 概述

SlimFaas 是 Axa France 开源的超轻量级 Kubernetes FaaS（Function as a Service）平台，以极低的复杂度和资源开销在 K8s 上运行函数，是 Knative/OpenFaaS 的极简替代。

## 核心概念/原理

- **超轻量**：极简的 FaaS 实现，资源占用极低
- **K8s 原生**：基于 K8s Deployment/HPA 实现
- **零冷启动**：支持保持 Pod 常驻避免冷启动
- **Axa France**：企业级 Serverless 实践

## 关键机制或特性

- SlimData（内置轻量持久化）
- 基于 HPA 的自动扩缩
- HTTP 触发器（同步/异步）
- 事件驱动（Pub/Sub 模式）
- 多语言函数支持
- 资源限制和 QoS 管理

## 使用场景与最佳实践

- 内部系统的轻量 Serverless 需求
- Knative/OpenFaaS 的极简替代
- 微服务的函数化处理
- 事件驱动的异步处理
- 开发团队的自助 Serverless 平台

## 参考链接

- https://github.com/AxaFrance/SlimFaas
- https://axafrance.github.io/SlimFaas/

## Related

- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/openfaas.md|OpenFaaS]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/knative.md|Knative]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/keda.md|KEDA]]
