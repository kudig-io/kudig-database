---
title: Sermant 服务治理
description: Sermant 是华为开源的 CNCF Sandbox 项目，基于 Java Agent 的无代理服务治理框架，无需 Sidecar 即可实现流量管理、灰度发布...
summary: Sermant 是华为开源的 CNCF Sandbox 项目，基于 Java Agent 的无代理服务治理框架，无需 Sidecar 即可实现流量管理、灰度发布...
category: dictionary
tags:
- k8s
- glossary
- networking
- service-mesh
- java
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Sermant 服务治理 是什么
- Sermant 详解
trigger_keywords:
- Sermant 服务治理
- Sermant
- dictionary
prerequisites:
- kubernetes
---



# Sermant 服务治理（Sermant）

## 概述

Sermant 是华为开源的 CNCF Sandbox 项目，基于 Java Agent 的无代理服务治理框架，无需 Sidecar 即可实现流量管理、灰度发布和服务可观测性。

## 核心概念/原理

- **Java Agent**：无 Sidecar 的服务治理
- **零侵入**：通过字节码增强实现，应用无需修改
- **CNCF Sandbox**：华为主导
- **服务网格替代**：轻量级的服务治理方案

## 关键机制或特性

- 流量管理（路由/灰度/限流/熔断）
- 标签路由（基于 Header/参数）
- 服务可观测性（追踪/指标）
- 插件体系（可扩展治理能力）
- Sermant Backend 管控面
- 与 Istio 控制面兼容
- 支持 Spring Cloud/Dubbo

## 使用场景与最佳实践

- Java 微服务的无侵入治理
- 传统应用的灰度发布
- Sidecar 不可用场景的替代
- 服务路由和流量管理
- 微服务的可观测性接入

## 参考链接

- https://sermant.io/
- https://github.com/sermant-io/Sermant

## Related

- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
- [[17-系统基础/06-知识字典/networking/linkerd.md|Linkerd]]
- [[17-系统基础/06-知识字典/networking/kuma.md|Kuma]]
