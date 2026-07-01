---
title: Downward API 元数据注入
description: Downward API 是 Kubernetes 将 Pod/Container 元数据（名称、命名空间、标签、资源限制等）注入到容器内部的机制，支持环境变量...
summary: Downward API 是 Kubernetes 将 Pod/Container 元数据（名称、命名空间、标签、资源限制等）注入到容器内部的机制，支持环境变量...
category: dictionary
tags:
- k8s
- glossary
- configuration
- env
- volume
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Downward API 元数据注入 是什么
- Downward API 详解
trigger_keywords:
- Downward API 元数据注入
- Downward API
- dictionary
prerequisites:
- kubernetes
---



# Downward API 元数据注入（Downward API）

## 概述

Downward API 是 Kubernetes 将 Pod/Container 元数据（名称、命名空间、标签、资源限制等）注入到容器内部的机制，支持环境变量和 Volume 文件两种方式。

## 核心概念/原理

- **元数据暴露**：将 Pod 自身信息注入到容器
- **双通道**：环境变量（env.valueFrom.fieldRef）和 Volume
- **只读**：容器只能读取，不能修改
- **实时性**：Volume 方式支持标签/注解变更的自动更新

## 关键机制或特性

- fieldRef 注入 Pod 元数据（name/namespace/uid/labels/annotations）
- resourceFieldRef 注入资源信息（limits.cpu/requests.memory）
- DownwardAPIVolumeFile 写入 Volume 文件
- 支持的字段：metadata.name/namespace/labels/annotations、spec.nodeName/nodeIP、status.podIP/hostIP
- Volume 方式支持热更新
- 环境变量方式需重启生效

## 使用场景与最佳实践

- 容器内获取自身 Pod 信息
- 日志标签注入（pod_name/namespace）
- 服务自注册的 IP 获取
- 资源感知的自适应配置
- 最佳实践：热更新用 Volume、简单值用 env、避免循环依赖

## 参考链接

- https://kubernetes.io/docs/concepts/workloads/pods/downward-api/
- https://kubernetes.io/docs/tasks/inject-data-application/downward-api-volume-expose-pod-information/

## Related

- [[domain-17-system-foundation/topic-dictionary/configuration/env.md|Environment Variables]]
- [[domain-17-system-foundation/topic-dictionary/configuration/configmap.md|ConfigMap]]
- [[domain-17-system-foundation/topic-dictionary/workloads/pod.md|Pod]]
