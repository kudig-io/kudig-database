---
title: 限制范围
description: 'LimitRange 是 Kubernetes 命名空间级别的资源默认值和约束机制，为 Pod/Container 自动设置资源的 requests/limit...'
category: dictionary
tags:
- k8s
- glossary
- configuration
- resource-management
- defaults
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 限制范围 是什么
- LimitRange 详解
trigger_keywords:
- 限制范围
- LimitRange
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# 限制范围（LimitRange）

## 概述

LimitRange 是 Kubernetes 命名空间级别的资源默认值和约束机制，为 Pod/Container 自动设置资源的 requests/limits 默认值，并强制最大最小值约束。

## 核心概念/原理

- **默认值注入**：为未设置 requests/limits 的容器自动填充
- **约束强制**：拒绝超出最大/最小范围的请求
- **命名空间级**：每个命名空间独立配置
- **与 ResourceQuota 配合**：防止资源滥用

## 关键机制或特性

- `spec.limits` 定义约束列表
- type: Container/Pod/PersistentVolumeClaim
- default 默认 limits 值
- defaultRequest 默认 requests 值
- max/min 最大最小约束
- maxLimitRequestRatio 限制比率
- 仅对新 Pod 生效（不追溯已有 Pod）

## 使用场景与最佳实践

- 为未设置资源限制的容器提供默认值
- 防止过大或过小的资源请求
- 存储卷的大小约束
- 配合 ResourceQuota 确保配额可用
- 最佳实践：设合理的默认值、max 不超过节点容量、配合 VPA 自动调优

## 参考链接

- https://kubernetes.io/docs/concepts/policy/limit-range/
- https://kubernetes.io/docs/tasks/administer-cluster/manage-resources/

## Related

- [[domain-17-system-foundation/topic-dictionary/configuration/resource-quota.md|ResourceQuota]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/vpa.md|VPA]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/namespace.md|Namespace]]
