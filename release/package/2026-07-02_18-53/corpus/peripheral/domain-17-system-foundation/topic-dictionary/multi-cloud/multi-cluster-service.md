---
title: 多集群服务 MCS
description: Multi-Cluster Service（MCS）是 Google/Anthos 推动的多集群服务发现标准，通过 ServiceImport/ServiceE...
summary: Multi-Cluster Service（MCS）是 Google/Anthos 推动的多集群服务发现标准，通过 ServiceImport/ServiceE...
category: dictionary
tags:
- k8s
- glossary
- multi-cloud
- service-discovery
- networking
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 多集群服务 MCS 是什么
- Multi-Cluster Service 详解
trigger_keywords:
- 多集群服务 MCS
- Multi-Cluster Service
- dictionary
prerequisites:
- kubernetes
---



# 多集群服务 MCS（Multi-Cluster Service）

## 概述

Multi-Cluster Service（MCS）是 Google/Anthos 推动的多集群服务发现标准，通过 ServiceImport/ServiceExport CRD 实现跨集群的服务注册和发现，已被纳入 Gateway API 生态。

## 核心概念/原理

- **跨集群发现**：服务在不同集群间自动发现
- **Gateway API 集成**：GAMMA  initiative 的核心组件
- **标准化**：SIG Multicluster 推动的标准 API
- **透明路由**：客户端无需知道服务在哪个集群

## 关键机制或特性

- ServiceExport CRD 声明导出的服务
- ServiceImport CRD 表示在其他集群导入的服务
- EndpointSlice 跨集群同步
- 支持 ClusterIP/Headless 两种导入模式
- 与 Service Mesh 集成（Istio/Linkerd）
- DNS 自动注册（.svc.clusterset.local）
- 网络连通性前提（VPC Peering/VPN）

## 使用场景与最佳实践

- 多集群微服务的统一访问
- 跨区域容灾的服务切换
- 蓝绿部署的跨集群流量
- 服务网格的多集群扩展
- 最佳实践：先确保网络连通、配合 Service Mesh、做好服务版本管理

## 参考链接

- https://github.com/kubernetes-sigs/mcs-api
- https://gateway-api.sigs.k8s.io/guides/

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/networking/service-mesh.md|Service Mesh]]
- [[domain-17-system-foundation/topic-dictionary/networking/envoy-gateway.md|Envoy Gateway]]
