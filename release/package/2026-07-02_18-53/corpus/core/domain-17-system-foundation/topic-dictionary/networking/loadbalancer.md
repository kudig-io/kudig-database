---
title: 负载均衡器
description: LoadBalancer 是 Service 的一种类型，通过云厂商的负载均衡器将服务暴露到集群外部。它自动创建云平台的 LB 资源并配置外部
  IP。...
summary: LoadBalancer 是 Service 的一种类型，通过云厂商的负载均衡器将服务暴露到集群外部。它自动创建云平台的 LB 资源并配置外部 IP。...
category: dictionary
tags:
- k8s
- glossary
- networking
- service
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 负载均衡器 是什么
- LoadBalancer 详解
trigger_keywords:
- 负载均衡器
- LoadBalancer
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 负载均衡器

> **英文名**: LoadBalancer

## 概述

LoadBalancer 是 Service 的一种类型，通过云厂商的负载均衡器将服务暴露到集群外部。它自动创建云平台的 LB 资源并配置外部 IP。

## 核心概念/原理

### 工作原理

```
创建 LoadBalancer Service → CCM 调用云 API → 创建 LB → 分配外部 IP → 配置转发规则
```

### 注解

不同云厂商通过 Service 注解自定义 LB 行为：
- AWS: `service.beta.kubernetes.io/aws-load-balancer-*`
- GCP: `cloud.google.com/load-balancer-type`
- Azure: `service.beta.kubernetes.io/azure-load-balancer-*`

## 关键机制或特性

- 依赖 Cloud Controller Manager（CCM）和云平台 API。
- 每个 LoadBalancer Service 通常创建一个独立的 LB 实例（成本较高）。
- `loadBalancerClass`（v1.24+）支持指定自定义 LB 实现。

## 使用场景与最佳实践

- 需要外部访问时使用 LoadBalancer。
- 大量服务考虑使用 Ingress/Gateway API 共享一个 LB。
- 监控 LB 的健康状态和成本。
- 使用 `--allocate-node-ports=false`（v1.24+）避免暴露 NodePort。

## 参考链接

- [LoadBalancer - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/service/#loadbalancer)

## Related

- [[domain-17-system-foundation/知识字典/networking/service.md|Service]]
- [[domain-17-system-foundation/知识字典/networking/ingress.md|Ingress]]
- [[domain-17-system-foundation/知识字典/networking/clusterip.md|Clusterip]]
- [[domain-17-system-foundation/知识字典/networking/nodeport.md|Nodeport]]
- [[domain-17-system-foundation/知识字典/networking/headless-service.md|Headless Service]]


<!-- risk-assessed -->
