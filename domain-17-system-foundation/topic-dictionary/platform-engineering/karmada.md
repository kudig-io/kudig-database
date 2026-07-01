---
title: Karmada 多集群管理
description: 'Karmada 是 CNCF 孵化项目，提供 Kubernetes 多集群的统一管理和应用分发能力，支持跨集群调度、故障迁移和资源聚合，是 Federation...'
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- multi-cluster
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Karmada 多集群管理 是什么
- Karmada 详解
trigger_keywords:
- Karmada 多集群管理
- Karmada
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Karmada 多集群管理（Karmada）

## 概述

Karmada 是 CNCF 孵化项目，提供 Kubernetes 多集群的统一管理和应用分发能力，支持跨集群调度、故障迁移和资源聚合，是 Federation v2 的演进方案。

## 核心概念/原理

- **多集群联邦**：将多个 K8s 集群统一管理为一个逻辑集群
- **应用分发**：将应用声明式分发到指定集群集合
- **跨集群调度**：根据集群能力、亲和性和资源自动选择目标集群
- **CNCF 孵化**：华为开源，社区活跃

## 关键机制或特性

- PropagationPolicy / ClusterPropagationPolicy 定义应用分发策略
- 多集群资源视图（karmadactl get pods --all-clusters）
- 跨集群故障迁移（Failover）
- 资源解释器（Resource Interpreter）适配自定义资源
- 与 HPA/VPA 配合的跨集群弹性伸缩
- Karmada Dashboard Web UI

## 使用场景与最佳实践

- 企业多集群统一管理
- 跨区域/多云的应用部署
- 集群故障时的自动迁移
- 灰度发布中的多集群流量管理
- 集群资源统一视图和容量规划

## 参考链接

- https://karmada.io/
- https://github.com/karmada-io/karmada

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/submariner.md|Submariner]]
- [[domain-17-system-foundation/topic-dictionary/networking/clusternet.md|Clusternet]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/crossplane.md|Crossplane]]
