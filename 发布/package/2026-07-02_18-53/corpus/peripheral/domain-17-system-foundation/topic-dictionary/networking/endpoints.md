---
title: 端点
description: Endpoints 是 Service 后端 Pod 的 IP 地址和端口组合。当 Service 使用 selector 时，Kubernetes
  自动创建对...
summary: Endpoints 是 Service 后端 Pod 的 IP 地址和端口组合。当 Service 使用 selector 时，Kubernetes
  自动创建对...
category: dictionary
tags:
- k8s
- glossary
- endpoints
- networking
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 端点 是什么
- Endpoints 详解
trigger_keywords:
- 端点
- Endpoints
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 端点

> **英文名**: Endpoints

## 概述

Endpoints 是 Service 后端 Pod 的 IP 地址和端口组合。当 Service 使用 selector 时，Kubernetes 自动创建对应的 Endpoints 对象，记录匹配 Pod 的网络信息。

## 核心概念/原理

### 核心概念

- **自动管理**：Service 的 selector 匹配 Pod 后，Endpoints Controller 自动更新 Endpoints。
- **手动 Endpoints**：不使用 selector 的 Service 可以手动指定 Endpoints，指向外部服务。
- **EndpointSlice**：Endpoints 的替代方案，将端点分片存储，适合大规模集群。

### Endpoints vs EndpointSlice

| 特性 | Endpoints | EndpointSlice |
|------|-----------|---------------|
| 容量 | 单对象存储所有端点 | 分片存储，每片最多 100 个 |
| 性能 | 大规模时 API Server 压力大 | 显著减少 API Server 负载 |
| 推荐 | 小规模 | 生产推荐 |

## 关键机制或特性

- EndpointSlice 从 K8s v1.21 起成为默认方案。
- Endpoints 对象仍可使用但不推荐在大规模集群中使用。
- Headless Service 的 DNS 查询直接返回 Endpoints 中的 Pod IP。

## 使用场景与最佳实践

- 大规模集群确保启用 EndpointSlice API。
- 排查 Service 不通时检查 Endpoints 是否包含正确的后端 Pod。
- 使用 `kubectl get endpointslices` 查看分片的端点信息。

## 参考链接

- [Endpoints - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/endpoint-slices/)

## Related

[[domain-17-system-foundation/知识字典/networking/endpointslices.md|EndpointSlices]]


<!-- risk-assessed -->
