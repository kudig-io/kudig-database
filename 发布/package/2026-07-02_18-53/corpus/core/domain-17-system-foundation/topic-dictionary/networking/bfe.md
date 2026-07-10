---
title: BFE 负载均衡引擎
description: BFE（Baidu Front End）是百度开源的七层负载均衡引擎，已在百度内部大规模使用，支持多租户、高级流量管理和丰富的扩展插件，适用于超大规模互联网架构...
summary: BFE（Baidu Front End）是百度开源的七层负载均衡引擎，已在百度内部大规模使用，支持多租户、高级流量管理和丰富的扩展插件，适用于超大规模互联网架构...
category: dictionary
tags:
- k8s
- glossary
- networking
- load-balancer
- proxy
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- BFE 负载均衡引擎 是什么
- BFE 详解
trigger_keywords:
- BFE 负载均衡引擎
- BFE
- dictionary
prerequisites:
- kubernetes
---



# BFE 负载均衡引擎（BFE）

## 概述

BFE（Baidu Front End）是百度开源的七层负载均衡引擎，已在百度内部大规模使用，支持多租户、高级流量管理和丰富的扩展插件，适用于超大规模互联网架构。

## 核心概念/原理

- **高性能七层代理**：基于 Go 实现的高性能 HTTP/HTTPS 反向代理
- **多租户**：原生支持多租户流量隔离和独立配置
- **插件体系**：丰富的插件机制支持流量染色、限流、灰度等功能
- **大规模验证**：百度生产环境每日处理万亿级请求

## 关键机制或特性

- 基于集群的负载均衡和故障转移
- 精确流量调度（基于 Header/Cookie/IP 等）
- TLS 卸载与会话复用
- 与 K8s Ingress 集成（通过 bfe-ingress-controller）
- 健康检查与慢启动
- Prometheus 指标导出

## 使用场景与最佳实践

- 超大规模 Web 服务的入口负载均衡
- 需要多租户隔离的平台架构
- 国产自主可控的负载均衡方案
- 复杂的灰度发布和流量调度场景

## 参考链接

- https://www.bfe-networks.net/
- https://github.com/bfenetworks/bfe

## Related

- [[domain-17-system-foundation/知识字典/networking/envoy.md|Envoy]]
- [[domain-17-system-foundation/知识字典/networking/traefik.md|Traefik]]
- [[domain-17-system-foundation/知识字典/networking/consul.md|Consul]]
