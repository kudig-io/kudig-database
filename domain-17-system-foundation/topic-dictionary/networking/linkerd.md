---
title: Linkerd
description: Linkerd 是最早的服务网格项目之一，现为 CNCF 毕业项目。以极简设计和高性能著称，相比 Istio 更轻量、更易运维，适合不需要复杂
  Istio 功能...
summary: Linkerd 是最早的服务网格项目之一，现为 CNCF 毕业项目。以极简设计和高性能著称，相比 Istio 更轻量、更易运维，适合不需要复杂 Istio
  功能...
category: dictionary
tags:
- k8s
- glossary
- linkerd
- service-mesh
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linkerd 是什么
- Linkerd 详解
trigger_keywords:
- Linkerd
- dictionary
prerequisites:
- kubectl-basics
---



# Linkerd

> **英文名**: Linkerd

## 概述

Linkerd 是最早的服务网格项目之一，现为 CNCF 毕业项目。以极简设计和高性能著称，相比 Istio 更轻量、更易运维，适合不需要复杂 Istio 功能的中小规模服务网格场景。

## 核心概念/原理

### 核心架构

- **Linkerd Proxy**：Rust 编写的超轻量 sidecar（~10MB 内存）。
- **Linkerd Control Plane**：管理代理配置和证书。
- **Linkerd Viz**：可观测性 Dashboard。

### 与 Istio 对比

| 特性 | Linkerd | Istio |
|------|---------|-------|
| Proxy | Rust (轻量) | Envoy (功能丰富) |
| 复杂度 | 低 | 高 |
| mTLS | 内置 | 内置 |
| L7 策略 | 有限 | 丰富 |
| 资源开销 | 极低 | 较高 |

## 关键机制或特性

- **mTLS**：自动为所有服务间通信启用 mTLS。
- **负载均衡**：P2C（Power of Two Choices）算法。
- **重试和超时**：应用级别的重试策略。
- **流量拆分**：金丝雀发布和 A/B 测试。
- **Multi-cluster**：跨集群服务通信。

## 使用场景与最佳实践

- 需要服务网格但希望最小运维复杂度时选择 Linkerd。
- 使用 Linkerd 的 mTLS 实现零信任网络。
- 启用 Linkerd Viz 监控服务网格指标。
- 配合 Flagger 实现自动化金丝雀发布。
- 使用 `linkerd check` 验证安装和配置。

## 参考链接

- [Linkerd Official](https://linkerd.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/istio.md|Istio]]
- [[domain-17-system-foundation/topic-dictionary/networking/envoy.md|Envoy]]
- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/networking/cilium.md|Cilium]]
- [[domain-17-system-foundation/topic-dictionary/security/certificate.md|Certificate]]
