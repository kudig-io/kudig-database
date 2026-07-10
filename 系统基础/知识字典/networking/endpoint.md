---
title: 端点
description: Endpoints 是 Kubernetes 中 Service 后端 Pod 的 IP 地址和端口的集合。当 Service 没有指定
  selector 时，...
summary: Endpoints 是 Kubernetes 中 Service 后端 Pod 的 IP 地址和端口的集合。当 Service 没有指定 selector
  时，...
category: dictionary
tags:
- k8s
- glossary
- endpoint
- service
tier: supporting
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

Endpoints 是 Kubernetes 中 Service 后端 Pod 的 IP 地址和端口的集合。当 Service 没有指定 selector 时，需要手动创建 Endpoints 资源。EndpointSlice 是 Endpoints 的现代替代方案，适用于大规模集群。

## 核心概念/原理

### Endpoints vs EndpointSlice

| 特性 | Endpoints | EndpointSlice |
|------|-----------|---------------|
| API | v1 | discovery.k8s.io/v1 |
| 扩展性 | 单个对象包含所有后端 | 分片存储，每片 100 个 |
| 适用场景 | 小规模集群 | 大规模集群（推荐） |

### 工作原理

当 Service 定义了 selector，kube-controller-manager 自动创建对应的 Endpoints/EndpointSlice 对象。

## 关键机制或特性

- 每个 Endpoint 包含 IP、端口和就绪状态。
- EndpointSlice 按拓扑分区，支持 `topology.kubernetes.io/zone` 标签。
- 外部服务可通过手动 Endpoints + ExternalName Service 接入。

## 使用场景与最佳实践

- 大规模集群优先使用 EndpointSlice API。
- 排查 Service 不通时，检查 Endpoints 是否包含预期的后端 Pod。
- 使用 `kubectl get endpointslices -l kubernetes.io/service-name=<svc>` 查看。
- Headless Service 的 Endpoints 直接返回 Pod IP。

## 参考链接

- [Endpoints - Kubernetes Docs](https://kubernetes.io/docs/concepts/services-networking/service/#endpoints)

## Related

- [[系统基础/知识字典/networking/service.md|Service]]
- [[系统基础/知识字典/networking/headless-service.md|Headless Service]]
- [[系统基础/知识字典/networking/clusterip.md|ClusterIP]]
- [[系统基础/知识字典/networking/coredns.md|CoreDNS]]
- [[系统基础/知识字典/networking/networkpolicy.md|NetworkPolicy]]


<!-- risk-assessed -->
