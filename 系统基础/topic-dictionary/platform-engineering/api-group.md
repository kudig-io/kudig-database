---
title: API 组
description: API Group 是 Kubernetes 中将相关 API 资源组织在一起的逻辑分组机制。不同的功能模块通过不同的 API Group
  暴露，便于管理和版本...
summary: API Group 是 Kubernetes 中将相关 API 资源组织在一起的逻辑分组机制。不同的功能模块通过不同的 API Group 暴露，便于管理和版本...
category: dictionary
tags:
- k8s
- glossary
- api
- platform
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- API 组 是什么
- API Group 详解
trigger_keywords:
- API 组
- API Group
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# API 组

> **英文名**: API Group

## 概述

API Group 是 Kubernetes 中将相关 API 资源组织在一起的逻辑分组机制。不同的功能模块通过不同的 API Group 暴露，便于管理和版本控制。

## 核心概念/原理

### 核心概念

- **核心组（Core Group）**：`""` 或 `v1`，包含 Pod、Service、ConfigMap、Secret 等基础资源。
- **命名组（Named Groups）**：如 `apps/v1`（Deployment、StatefulSet）、`batch/v1`（Job、CronJob）、`networking.k8s.io/v1`（Ingress、NetworkPolicy）。

### 常用 API Group

| API Group | 资源示例 |
|-----------|---------|
| `""` (core) | Pod, Service, ConfigMap, Secret, Node |
| `apps` | Deployment, StatefulSet, DaemonSet, ReplicaSet |
| `batch` | Job, CronJob |
| `networking.k8s.io` | Ingress, NetworkPolicy, IngressClass |
| `rbac.authorization.k8s.io` | Role, ClusterRole, RoleBinding |
| `storage.k8s.io` | StorageClass, CSIDriver |

## 关键机制或特性

- 通过 `kubectl api-resources` 查看所有可用的 API Group 和资源。
- API Group 支持多个版本共存（如 `v1`、`v1beta1`）。
- CRD 使用自定义的 API Group。

## 使用场景与最佳实践

- 创建 CRD 时使用 `yourcompany.io` 格式的 API Group。
- 了解 API Group 有助于正确编写 RBAC 规则和 Manifest。

## 参考链接

- [API Group - Official Documentation](https://kubernetes.io/docs/reference/using-api/)

## Related

- [[系统基础/topic-dictionary/platform-engineering/api-version.md|Api Version]]
- [[系统基础/topic-dictionary/platform-engineering/kind.md|Kind]]
- [[系统基础/topic-dictionary/platform-engineering/manifest.md|Manifest]]
- [[系统基础/topic-dictionary/platform-engineering/custom-resource.md|Custom Resource]]
- [[系统基础/topic-dictionary/platform-engineering/operator-pattern.md|Operator Pattern]]


<!-- risk-assessed -->
