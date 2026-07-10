---
title: 集群 IP
description: ClusterIP 是 Kubernetes Service 的默认类型，为 Service 分配一个集群内部的虚拟 IP 地址。只有集群内部的
  Pod 可以通...
summary: ClusterIP 是 Kubernetes Service 的默认类型，为 Service 分配一个集群内部的虚拟 IP 地址。只有集群内部的
  Pod 可以通...
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
- 集群 IP 是什么
- ClusterIP 详解
trigger_keywords:
- 集群 IP
- ClusterIP
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 集群 IP

> **英文名**: ClusterIP

## 概述

ClusterIP 是 Kubernetes Service 的默认类型，为 Service 分配一个集群内部的虚拟 IP 地址。只有集群内部的 Pod 可以通过 ClusterIP 访问该 Service。

## 核心概念/原理

### 核心概念

- **虚拟 IP**：ClusterIP 不绑定任何网络接口，由 kube-proxy 通过 iptables/IPVS 规则实现流量转发。
- **分配范围**：由 `--service-cluster-ip-range` 参数指定（如 `10.96.0.0/12`）。
- **无头服务**：设置 `clusterIP: None` 创建 Headless Service，DNS 直接返回后端 Pod IP。

### 使用场景

- 集群内部服务间的通信（如 API 服务调用数据库）。
- 不
需要外部访问的服务使用 NodePort/LoadBalancer/Ingress。

## 关键机制或特性

- ClusterIP 由 kube-proxy 通过 iptables/IPVS 实现，不依赖实际网络接口。
- 分配范围避免与 Pod CIDR 或 Node 网络冲突。
- Headless Service 适合 StatefulSet 和有状态应用的服务发现。

## 使用场景与最佳实践

- 大多数内部服务使用 ClusterIP（默认类型）。
- 需要稳定 DNS 解析到单个 Pod 时使用 Headless Service。
- 监控 ClusterIP 分配池的使用率。

## 参考链接

- [ClusterIP - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/service/#type-clusterip)

## Related

- [[domain-17-system-foundation/知识字典/networking/service.md|Service]]
- [[domain-17-system-foundation/知识字典/networking/ingress.md|Ingress]]
- [[domain-17-system-foundation/知识字典/networking/nodeport.md|Nodeport]]
- [[domain-17-system-foundation/知识字典/networking/loadbalancer.md|Loadbalancer]]
- [[domain-17-system-foundation/知识字典/networking/headless-service.md|Headless Service]]


<!-- risk-assessed -->
