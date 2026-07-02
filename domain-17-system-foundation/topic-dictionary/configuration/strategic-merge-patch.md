---
title: 策略合并补丁
description: Strategic Merge Patch 是 Kubernetes 特有的 JSON 合并策略，针对列表类型提供智能合并（按 key 合并而非替换），是
  ku...
summary: Strategic Merge Patch 是 Kubernetes 特有的 JSON 合并策略，针对列表类型提供智能合并（按 key 合并而非替换），是
  ku...
category: dictionary
tags:
- k8s
- glossary
- configuration
- patch
- api
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 策略合并补丁 是什么
- Strategic Merge Patch 详解
trigger_keywords:
- 策略合并补丁
- Strategic Merge Patch
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 策略合并补丁（Strategic Merge Patch）

## 概述

Strategic Merge Patch 是 Kubernetes 特有的 JSON 合并策略，针对列表类型提供智能合并（按 key 合并而非替换），是 kubectl apply 和 K8s 控制器的默认补丁策略。

## 核心概念/原理

- **K8s 特有**：区别于标准 JSON Merge Patch
- **列表合并**：按 patchStrategy 定义的 key 合并列表元素
- **默认策略**：kubectl apply 使用此策略
- **CRD 支持**：通过 kubebuilder 注解定义

## 关键机制或特性

- `$patch: replace` 替换整个字段
- `$patch: delete` 删除字段
- `$patch: merge` 合并（默认）
- patchStrategy: merge（按 key 合并列表）
- patchMergeKey: 合并的标识字段（如 name/port）
- 保留列表（retainKeys）策略
- 与 JSON Patch（RFC 6902）和 Server-Side Apply 对比

## 使用场景与最佳实践

- kubectl apply 的底层合并逻辑
- Operator 的状态合并
- 声明式配置的部分更新
- kubectl patch 命令使用
- 最佳实践：了解 patchStrategy、复杂更新用 SSA、避免意外覆盖

## 参考链接

- https://kubernetes.io/docs/tasks/manage-kubernetes-objects/update-api-object-kubectl-patch/
- https://github.com/kubernetes/community/blob/master/contributors/devel/sig-api-machinery/strategic-merge-patch.md

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|kubectl]]
- [[domain-17-system-foundation/topic-dictionary/configuration/server-side-apply.md|Server-Side Apply]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kustomize.md|Kustomize]]


<!-- risk-assessed -->
