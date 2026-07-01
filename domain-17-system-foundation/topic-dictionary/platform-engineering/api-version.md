---
title: API 版本
description: API Version（API 版本）是 Kubernetes API 的版本标识，表示资源的 API 演进阶段。Kubernetes 使用版本化来管理
  API...
summary: API Version（API 版本）是 Kubernetes API 的版本标识，表示资源的 API 演进阶段。Kubernetes 使用版本化来管理
  API...
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
- API 版本 是什么
- API Version 详解
trigger_keywords:
- API 版本
- API Version
- dictionary
prerequisites:
- kubectl-basics
---



# API 版本

> **英文名**: API Version

## 概述

API Version（API 版本）是 Kubernetes API 的版本标识，表示资源的 API 演进阶段。Kubernetes 使用版本化来管理 API 的变更和兼容性。

## 核心概念/原理

### 版本阶段

| 阶段 | 格式 | 稳定性 |
|------|------|--------|
| Alpha | `v1alpha1`, `v2beta1` | 实验性，可能随时变更 |
| Beta | `v1beta1`, `v2beta2` | 预发布，API 基本稳定 |
| Stable/GA | `v1`, `v2` | 稳定版本，保证向后兼容 |

### 版本格式

API 版本由 API Group + Version 组成：
- 核心组：`v1`（如 Pod、Service）
- 命名组：`apps/v1`（如 Deployment）、`batch/v1`（如 Job）

### API 版本转换

Kubernetes 支持同一资源的多版本存储和自动转换：
```yaml
# 存储版本（etcd 中的版本）
storage: apps/v1
# 请求版本（客户端请求的版本）
request: apps/v1beta1 → 自动转换为 v1 返回
```

## 关键机制或特性

- 使用 `kubectl api-versions` 查看集群支持的所有 API 版本。
- 使用 `kubectl explain <resource>.spec` 查看资源的 API 版本和字段说明。
- 已弃用的 API 版本会在后续版本中被移除。

## 使用场景与最佳实践

- 始终使用 stable 版本的 API（避免 alpha/beta）。
- 升级集群前检查是否有已弃用的 API 版本在使用。
- 使用 `pluto` 或 `kubent` 工具检测已弃用的 API 版本。
- Manifest 中的 `apiVersion` 必须与资源类型匹配。

## 参考链接

- [API Version - Official Documentation](https://kubernetes.io/docs/reference/using-api/)

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/api-group.md|Api Group]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/kind.md|Kind]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/manifest.md|Manifest]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/custom-resource.md|Custom Resource]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/operator-pattern.md|Operator Pattern]]
