---
title: Headless Service 无头服务
description: Headless Service 是 clusterIP 设为 None 的特殊 Service，不分配虚拟 IP，而是通过 DNS 直接返回后端
  Pod 的 ...
summary: Headless Service 是 clusterIP 设为 None 的特殊 Service，不分配虚拟 IP，而是通过 DNS 直接返回后端
  Pod 的 ...
category: dictionary
tags:
- k8s
- glossary
- networking
- service
- dns
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Headless Service 无头服务 是什么
- Headless Service 详解
trigger_keywords:
- Headless Service 无头服务
- Headless Service
- dictionary
prerequisites:
- kubernetes
---



# Headless Service 无头服务（Headless Service）

## 概述

Headless Service 是 clusterIP 设为 None 的特殊 Service，不分配虚拟 IP，而是通过 DNS 直接返回后端 Pod 的 IP 地址列表，适用于需要客户端直接连接 Pod 的场景。

## 核心概念/原理

- **clusterIP: None**：不分配 ClusterIP
- **DNS 记录**：为每个 Pod 创建 A/AAAA 记录
- **直接连接**：客户端通过 DNS 获取 Pod IP 直连
- **有状态应用**：StatefulSet 的标准网络方案

## 关键机制或特性

- `clusterIP: None` 定义 Headless Service
- DNS 格式：`pod-name.svc-name.namespace.svc.cluster.local`
- 有 selector 时返回匹配 Pod 的 IP 列表
- 无 selector 时配合 EndpointSlice 手动管理
- StatefulSet 必须使用 Headless Service
- 与 Service Mesh 的集成（Istio 自动处理）
- DNS SRV 记录支持端口发现

## 使用场景与最佳实践

- StatefulSet（数据库集群）的网络标识
- 服务发现的客户端直连模式
- 需要知道具体后端地址的场景
- gRPC 客户端的 DNS 负载均衡
- 最佳实践：配合 StatefulSet 使用、DNS TTL 调优

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/service/#headless-services
- https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/

## Related

- [[domain-17-system-foundation/知识字典/networking/service.md|Service]]
- [[domain-17-system-foundation/知识字典/networking/dns.md|DNS]]
- [[domain-17-system-foundation/知识字典/workloads/statefulset.md|StatefulSet]]
