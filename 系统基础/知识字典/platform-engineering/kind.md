---
title: 类型
description: Kind（类型）是 Kubernetes 资源对象的类型标识。每个 Manifest 的 `kind` 字段指定了要创建的资源类型，如 Pod、Deployme...
summary: Kind（类型）是 Kubernetes 资源对象的类型标识。每个 Manifest 的 `kind` 字段指定了要创建的资源类型，如 Pod、Deployme...
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
- 类型 是什么
- Kind 详解
trigger_keywords:
- 类型
- Kind
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 类型

> **英文名**: Kind

## 概述

Kind（类型）是 Kubernetes 资源对象的类型标识。每个 Manifest 的 `kind` 字段指定了要创建的资源类型，如 Pod、Deployment、Service 等。

## 核心概念/原理

### Kind 的作用

Kind 在 Manifest 中标识资源类型：

```yaml
apiVersion: apps/v1
kind: Deployment    # 资源类型
metadata:
  name: my-app
```

### Kind 分类

| 类别 | Kind 示例 |
|------|----------|
| 工作负载 | Pod, Deployment, StatefulSet, DaemonSet, Job, CronJob |
| 服务发现 | Service, Ingress, Endpoints |
| 存储 | PersistentVolume, PersistentVolumeClaim, StorageClass |
| 配置 | ConfigMap, Secret |
| 安全 | Role, ClusterRole, RoleBinding, ServiceAccount |
| 集群管理 | Namespace, Node, ResourceQuota, LimitRange |

## 关键机制或特性

- 每个 Kind 属于一个 API Group，通过 `apiVersion` 指定。
- 使用 `kubectl api-resources` 查看所有 Kind 及其对应的 API Group。
- CRD（CustomResourceDefinition）允许创建自定义 Kind。

## 使用场景与最佳实践

- 了解常用 Kind 的 API Group 和版本。
- 使用 `kubectl explain <kind>` 查看 Kind 的详细字段说明。
- 创建 CRD 时遵循 Kind 命名规范（PascalCase，单数/复数）。

## 参考链接

- [Kind - Official Documentation](https://kubernetes.io/docs/reference/kubernetes-api/)

## Related

- [[系统基础/知识字典/platform-engineering/api-group.md|Api Group]]
- [[系统基础/知识字典/platform-engineering/api-version.md|Api Version]]
- [[系统基础/知识字典/platform-engineering/manifest.md|Manifest]]
- [[系统基础/知识字典/platform-engineering/custom-resource.md|Custom Resource]]
- [[系统基础/知识字典/platform-engineering/operator-pattern.md|Operator Pattern]]


<!-- risk-assessed -->
