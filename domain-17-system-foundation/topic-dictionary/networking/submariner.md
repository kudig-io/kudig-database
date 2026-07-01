---
title: Submariner 多集群网络
description: Submariner 是 Red Hat 主导的 CNCF Sandbox 项目，专注于解决 Kubernetes 多集群间的网络互联问题，实现跨集群
  Serv...
summary: Submariner 是 Red Hat 主导的 CNCF Sandbox 项目，专注于解决 Kubernetes 多集群间的网络互联问题，实现跨集群
  Serv...
category: dictionary
tags:
- k8s
- glossary
- networking
- multi-cluster
- cni
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Submariner 多集群网络 是什么
- Submariner 详解
trigger_keywords:
- Submariner 多集群网络
- Submariner
- dictionary
prerequisites:
- kubernetes
---



# Submariner 多集群网络（Submariner）

## 概述

Submariner 是 Red Hat 主导的 CNCF Sandbox 项目，专注于解决 Kubernetes 多集群间的网络互联问题，实现跨集群 Service 发现和 Pod 直通，无需依赖外部网络方案。

## 核心概念/原理

- **跨集群网络**：在不同 K8s 集群间建立安全的 IPsec/WireGuard 隧道
- **Service 发现**：基于 MCS（Multi-Cluster Services）API 实现跨集群服务发现
- **CNI 无关**：兼容 Flannel、Calico、Cilium、OVN 等各种 CNI
- **Gateway 模型**：每个集群通过 Gateway 节点建立隧道连接

## 关键机制或特性

- 支持 IPsec 和 WireGuard 两种隧道协议
- Globalnet 解决集群 CIDR 重叠问题
- 与 K8s MCS API 标准对齐
- Submariner Operator 简化部署
- 内置连接状态监控和健康检查
- 支持 Headless Service 和 StatefulSet 跨集群访问

## 使用场景与最佳实践

- 多集群应用的服务间通信
- 集群迁移期间的流量平滑切换
- 混合云/多云环境的网络打通
- 开发/测试环境的跨集群联调

## 参考链接

- https://submariner.io/
- https://github.com/submariner-io/submariner

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/cilium.md|Cilium Cluster Mesh]]
- [[domain-17-system-foundation/topic-dictionary/networking/linkerd.md|Linkerd]]
- [[domain-17-system-foundation/topic-dictionary/networking/consul.md|Consul]]
