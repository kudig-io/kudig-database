---
title: K8s 集群联邦
description: Kubernetes Federation（Federation v2/KubeFed）是多集群管理的标准和实现，通过联邦控制平面统一管理跨多个
  K8s 集群的...
summary: Kubernetes Federation（Federation v2/KubeFed）是多集群管理的标准和实现，通过联邦控制平面统一管理跨多个
  K8s 集群的...
category: dictionary
tags:
- k8s
- glossary
- multi-cloud
- federation
- multi-cluster
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 集群联邦 是什么
- Federation 详解
trigger_keywords:
- K8s 集群联邦
- Federation
- dictionary
prerequisites:
- kubernetes
---



# K8s 集群联邦（Federation）

## 概述

Kubernetes Federation（Federation v2/KubeFed）是多集群管理的标准和实现，通过联邦控制平面统一管理跨多个 K8s 集群的资源和策略，实现全局调度和策略一致性。

## 核心概念/原理

- **多集群管理**：统一管理多个 K8s 集群
- **策略传播**：全局策略下发到成员集群
- **KubeFed v2**：当前主流的联邦实现
- **灵活联邦**：不要求所有集群完全统一

## 关键机制或特性

- FederatedTypeConfig 定义联邦资源类型
- 资源模板（Template）+ 放置策略（Placement）
- Override 集群级覆盖
- 联邦 DNS（跨集群服务发现）
- 联邦 RBAC（全局权限管理）
- 策略控制器（合规检查）
- 多集群 Ingress

## 使用场景与最佳实践

- 全球分布的多集群管理
- 多区域容灾和高可用
- 统一的策略和合规管理
- 跨区域流量调度
- 最佳实践：从单集群开始、渐进式联邦、策略先行

## 参考链接

- https://github.com/kubernetes-sigs/kubefed
- https://kubernetes.io/docs/concepts/cluster-administration/federation/

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/karmada.md|Karmada]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/open-cluster-management.md|OCM]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/kubefleet.md|KubeFleet]]
