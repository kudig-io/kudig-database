---
title: 服务端 Apply SSA
description: Server-Side Apply（SSA）是 Kubernetes 1.22+ GA 的配置管理特性，在 API Server 端执行声明式合并，支持多管理者...
summary: Server-Side Apply（SSA）是 Kubernetes 1.22+ GA 的配置管理特性，在 API Server 端执行声明式合并，支持多管理者...
category: dictionary
tags:
- k8s
- glossary
- configuration
- api
- field-management
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 服务端 Apply SSA 是什么
- Server-Side Apply 详解
trigger_keywords:
- 服务端 Apply SSA
- Server-Side Apply
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 服务端 Apply SSA（Server-Side Apply）

## 概述

Server-Side Apply（SSA）是 Kubernetes 1.22+ GA 的配置管理特性，在 API Server 端执行声明式合并，支持多管理者（Manager）的字段所有权追踪和冲突检测。

## 核心概念/原理

- **服务端合并**：API Server 执行 merge 逻辑
- **字段所有权**：追踪每个字段由哪个 Manager 管理
- **冲突检测**：多个 Manager 修改同一字段时告警
- **GA 特性**：K8s 1.22 起正式可用

## 关键机制或特性

- `fieldManager` 声明管理者身份
- `force` 强制获取字段所有权
- ManagedFields 元数据追踪
- 与 Client-Side Apply（CSA）对比
- `kubectl apply --server-side`
- Controller 的 SSA 模式（controller-gen）
- 与 Strategic Merge Patch 的差异

## 使用场景与最佳实践

- 多控制器的声明式管理
- Controller 开发的最佳实践
- GitOps 工具的配置应用
- 复杂对象的增量更新
- 最佳实践：指定 fieldManager、理解冲突处理、控制器用 SSA

## 参考链接

- https://kubernetes.io/docs/reference/using-api/server-side-apply/
- https://kubernetes.io/blog/2021/08/06/server-side-apply-ga/

## Related

- [[系统基础/topic-dictionary/configuration/strategic-merge-patch.md|Strategic Merge Patch]]
- [[系统基础/topic-dictionary/tooling/kubectl.md|kubectl]]
- [[系统基础/topic-dictionary/operations/argo.md|Argo]]


<!-- risk-assessed -->
