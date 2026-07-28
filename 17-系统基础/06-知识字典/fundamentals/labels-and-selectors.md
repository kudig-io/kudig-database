---
title: 标签和选择器
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- daemonset
- job
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 标签和选择器 是什么
- 如何 标签和选择器
trigger_keywords:
- 标签和选择器
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 标签和选择器

## 概述

标签（Labels）是附加到对象（如 Pod）上的键/值对，用于指定对用户有意义的相关标识属性。与注解不同，标签可用于组织和选择对象的子集。标签选择器（Label Selectors）是 [[kubernetes|Kubernetes]] 中核心的分组原语。

## 核心概念/原理

### 标签

- 标签在创建时可以附加到对象，之后也可以随时添加和修改。
- 每个对象的标签键必须唯一。
- 标签适用于高效的查询和监听，是 UI 和 CLI 的理想选择。非标识信息应使用注解记录。

**语法规则**：
- 键由可选前缀和名称组成，用 `/` 分隔。
- 名称段最多 63 个字符，以字母数字开头和结尾，中间可包含 `-`、`_`、`.` 和字母数字。
- 前缀必须是 DNS 子域名，最多 253 个字符。`kubernetes.io/` 和 `k8s.io/` 前缀保留给 Kubernetes 核心组件使用。
- 值最多 63 个字符（可为空），非空时以字母数字开头和结尾。

### 标签选择器

客户端可以通过标签选择器识别一组对象。API 目前支持两种类型的选择器：

#### 基于相等性的要求（Equality-based）

支持 `=`、`==`、`!=` 运算符。例如：
- `environment=production`：选择标签键为 `environment` 且值为 `production` 的资源。
- `tier!=frontend`：选择标签键为 `tier` 且值不是 `frontend` 的资源，以及没有 `tier` 标签的资源。

多个要求用逗号分隔，表示逻辑 **AND** 关系。

#### 基于集合的要求（Set-based）

支持 `in`、`notin` 和 `exists`（仅键标识符）运算符。例如：
- `environment in (production, qa)`：选择 `environment` 值为 `production` 或 `qa` 的资源。
- `tier notin (frontend, backend)`：选择 `tier` 值不是 `frontend` 或 `backend` 的资源，以及没有 `tier` 标签的资源。
- `partition`：选择带有 `partition` 标签的资源（不检查值）。
- `!partition`：选择没有 `partition` 标签的资源。

## 关键机制或特性

- **API 对象中的集合引用**：[[service|Service]] 和 [[replicationcontroller|ReplicationController]] 使用基于相等性的选择器；Deployment、ReplicaSet、DaemonSet、Job 等较新的资源同时支持 `matchLabels` 和 `matchExpressions`（支持集合-based 要求）。
- **节点选择**：Pod 可以通过 `nodeSelector` 使用标签选择器约束可调度到的节点集合。
- **列表和监听过滤**：`kubectl get pods -l environment=production,tier=frontend`

## 使用场景

- 区分不同环境（dev、qa、production）。
- 区分不同发布轨道（stable、canary）。
- 区分多层应用架构（frontend、backend、cache）。
- 按客户或分区组织资源。

## 最佳实践/注意事项

- 多标签比单标签更能有效区分资源集合。
- 对于工具化和自动化，优先使用 `app.kubernetes.io/name` 等推荐标签，而非简单的 `app` 标签。
- 避免在选择器中依赖逻辑 OR 操作（标签选择器不支持 OR），应通过结构化的标签设计来满足需求。

## 参考链接

- [Labels and Selectors - Official Documentation](https://kubernetes.io/docs/concepts/overview/working-with-objects/labels/)

## Related

- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
