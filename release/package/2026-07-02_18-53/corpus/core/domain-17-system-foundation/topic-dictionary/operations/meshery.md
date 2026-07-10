---
title: Meshery 服务网格管理
description: Meshery 是 CNCF Sandbox 项目，提供服务网格和云原生基础设施的统一管理平台，支持 10+ 种服务网格的生命周期管理、性能基准测试和配置管理。...
summary: Meshery 是 CNCF Sandbox 项目，提供服务网格和云原生基础设施的统一管理平台，支持 10+ 种服务网格的生命周期管理、性能基准测试和配置管理。...
category: dictionary
tags:
- k8s
- glossary
- operations
- service-mesh
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
- Meshery 服务网格管理 是什么
- Meshery 详解
trigger_keywords:
- Meshery 服务网格管理
- Meshery
- dictionary
prerequisites:
- kubernetes
---



# Meshery 服务网格管理（Meshery）

## 概述

Meshery 是 CNCF Sandbox 项目，提供服务网格和云原生基础设施的统一管理平台，支持 10+ 种服务网格的生命周期管理、性能基准测试和配置管理。

## 核心概念/原理

- **多网格管理**：支持 Istio/Linkerd/Consul/Kuma/App Mesh 等 10+ 网格
- **性能测试**：内置 SMP（Service Mesh Performance）基准测试
- **配置管理**：跨网格的配置管理和策略执行
- **CNCF Sandbox**：Layer5 主导，社区活跃

## 关键机制或特性

- Meshery Operator 管理 Mesh 生命周期
- SMP（Service Mesh Performance）标准化性能指标
- Meshery Designs 可视化架构设计
- OAM（Open Application Model）集成
- WASM 过滤器管理
- MeshSync 集群状态同步
- 200+ 集成（Adapters）

## 使用场景与最佳实践

- 多服务网格的统一管理和对比评估
- 服务网格的性能基准测试
- 服务网格迁移的辅助工具
- 云原生架构的可视化设计
- 多团队环境的网格治理

## 参考链接

- https://meshery.io/
- https://github.com/meshery/meshery

## Related

- [[domain-17-system-foundation/知识字典/networking/istio.md|Istio]]
- [[domain-17-system-foundation/知识字典/networking/linkerd.md|Linkerd]]
- [[domain-17-system-foundation/知识字典/networking/kuma.md|Kuma]]
