---
title: K8GB 全球负载均衡
description: K8GB（Kubernetes Global Balancer）是 CNCF Sandbox 项目，实现跨多个 Kubernetes 集群的全球流量负载均衡，基...
summary: K8GB（Kubernetes Global Balancer）是 CNCF Sandbox 项目，实现跨多个 Kubernetes 集群的全球流量负载均衡，基...
category: dictionary
tags:
- k8s
- glossary
- networking
- dns
- multi-cluster
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8GB 全球负载均衡 是什么
- K8GB 详解
trigger_keywords:
- K8GB 全球负载均衡
- K8GB
- dictionary
prerequisites:
- kubernetes
---



# K8GB 全球负载均衡（K8GB）

## 概述

K8GB（Kubernetes Global Balancer）是 CNCF Sandbox 项目，实现跨多个 Kubernetes 集群的全球流量负载均衡，基于 DNS 和 GSLB 策略将用户请求路由到最优集群。

## 核心概念/原理

- **DNS 级负载均衡**：通过 CoreDNS 插件或外部 DNS 提供商实现 GSLB
- **健康检查驱动**：基于端点健康状态自动摘除故障集群
- **多策略路由**：支持 Round Robin、地理位置、故障转移等策略
- **CNCF Sandbox**：轻量级的全球流量管理方案

## 关键机制或特性

- GslbIngress CRD 定义全局流量策略
- 集成 Infoblox、Route53、NS1 等 DNS 提供商
- 基于 Prometheus 的健康检查指标
- 支持加权 Round Robin 和 GeoIP 路由
- 零停机集群维护和故障转移
- 与 Flagger / Argo Rollouts 配合使用

## 使用场景与最佳实践

- 多区域/多集群的高可用部署
- 灾难恢复场景下的流量切换
- 基于地理位置的用户路由
- 灰度发布中的全球流量分配

## 参考链接

- https://www.k8gb.io/
- https://github.com/k8gb-io/k8gb

## Related

- [[domain-17-system-foundation/知识字典/networking/consul.md|Consul]]
- [[domain-17-system-foundation/知识字典/networking/linkerd.md|Linkerd]]
- [[domain-17-system-foundation/知识字典/operations/flagger.md|Flagger]]
