---
title: Kuma 服务网格
description: Kuma 是 Kong 开源的 CNCF Sandbox 服务网格，基于 Envoy Proxy，支持 Kubernetes 和通用 VM
  环境，以易用性和多网...
summary: Kuma 是 Kong 开源的 CNCF Sandbox 服务网格，基于 Envoy Proxy，支持 Kubernetes 和通用 VM 环境，以易用性和多网...
category: dictionary
tags:
- k8s
- glossary
- networking
- service-mesh
- envoy
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuma 服务网格 是什么
- Kuma 详解
trigger_keywords:
- Kuma 服务网格
- Kuma
- dictionary
prerequisites:
- kubernetes
---



# Kuma 服务网格（Kuma）

## 概述

Kuma 是 Kong 开源的 CNCF Sandbox 服务网格，基于 Envoy Proxy，支持 Kubernetes 和通用 VM 环境，以易用性和多网格（multi-mesh）架构著称。

## 核心概念/原理

- **Envoy 驱动**：基于 Envoy Proxy 的数据面
- **通用平台**：同时支持 Kubernetes 和 VM/裸金属
- **多网格**：原生支持多网格隔离架构
- **CNCF Sandbox**：Kong 主导，社区活跃

## 关键机制或特性

- Mesh CRD 定义网格实例（多网格隔离）
- TrafficPermission / TrafficRoute / TrafficLog 策略
- mTLS 自动管理（内置 CA）
- 速率限制和熔断
- MeshGateway 支持入口流量
- Kong Mesh 商业版提供企业功能
- Kuma GUI 可视化管理

## 使用场景与最佳实践

- 轻量级服务网格部署
- K8s + VM 混合环境的服务治理
- 多团队/多环境的网格隔离
- 需要简单操作体验的服务网格
- Istio 的轻量替代方案

## 参考链接

- https://kuma.io/
- https://github.com/kumahq/kuma

## Related

- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
- [[17-系统基础/06-知识字典/networking/linkerd.md|Linkerd]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
