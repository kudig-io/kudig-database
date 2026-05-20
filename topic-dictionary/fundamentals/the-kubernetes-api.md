---
title: Kubernetes API
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- rbac
- operator
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes API 是什么
- 如何 Kubernetes API
trigger_keywords:
- Kubernetes
- API
- dictionary
title_en: The Kubernetes Api
---


# Kubernetes API

## 概述

Kubernetes API 是查询和操作 Kubernetes 中对象状态的核心机制。Kubernetes 控制平面的核心是 API 服务器及其暴露的 HTTP API。用户、集群内部的不同部分以及外部组件都通过 API 服务器相互通信。

## 核心概念/原理

### API 服务器的作用

API 服务器（kube-apiserver）暴露 HTTP API，允许最终用户、集群的不同部分和外部组件相互通信。大多数操作可以通过 `kubectl` 命令行工具或其他命令行工具（如 `kubeadm`）执行，这些工具底层都调用 API。也可以直接使用 REST 调用访问 API，Kubernetes 还提供了一组客户端库供开发者使用。

### API 规范发布机制

每个 Kubernetes 集群都会发布其所服务 API 的规范。Kubernetes 使用两种机制来发布这些 API 规范：

1. **Discovery API**：提供 Kubernetes API 的简要信息，包括 API 名称、资源、版本和支持的操作。它是对可用资源的简要摘要，不详细说明具体资源的 schema。
2. **Kubernetes OpenAPI Document**：为所有 Kubernetes API 端点提供完整的 OpenAPI v2.0 和 v3.0 schema。OpenAPI v3 是首选方法，因为它提供了更全面和准确的 API 视图，包含所有可用的 API 路径以及每个端点上每个操作所消耗和产生的所有资源。

### API 版本控制

Kubernetes 支持多个 API 版本，每个版本位于不同的 API 路径（如 `/api/v1` 或 `/apis/rbac.authorization.k8s.io/v1alpha1`）。版本控制是在 API 级别而非资源或字段级别进行的，以确保 API 呈现清晰一致的系统资源视图。

API 服务器透明地处理不同 API 版本之间的转换：所有不同版本实际上都是同一持久化数据的不同表示形式。

### API 兼容性承诺

- 对于达到 GA（一般可用性，通常为 v1）的官方 Kubernetes API，Kubernetes 强烈承诺保持兼容性。
- 对于官方 Kubernetes API 的 beta 版本，Kubernetes 确保数据可以通过 GA API 版本进行转换和访问。
- 新 API 资源和字段可以频繁添加，但删除资源或字段需要遵循 API 弃用策略。

## 关键机制或特性

- **聚合发现（Aggregated Discovery）**：自 Kubernetes v1.30 [stable] 起，默认启用。通过两个端点（`/api` 和 `/apis`）发布集群支持的所有资源，大幅减少获取发现数据的请求数量。
- **OpenAPI v3**：自 Kubernetes v1.27 [stable] 起，默认启用。提供按 Kubernetes 组版本划分的 OpenAPI v3 规范，使用带有 hash 的相对 URL 以改善客户端缓存。
- **Protobuf 序列化**：Kubernetes 还实现了基于 Protobuf 的替代序列化格式，主要用于集群内部通信。

## 使用场景

- 开发自定义控制器和 Operator，通过客户端库与 Kubernetes API 交互。
- 构建自动化工具和平台，动态发现集群支持的 API 资源和版本。
- 在 CI/CD 流水线中使用 `kubectl apply --dry-run=server` 进行服务端验证。

## 最佳实践/注意事项

- 优先使用官方客户端库，而不是直接构造 REST 调用，以获得更好的类型安全和错误处理。
- 关注 API 弃用策略，及时将 beta API 迁移到稳定版本。
- 利用 OpenAPI v3 和 Discovery API 构建与 Kubernetes 自动互操作的工具。

## 参考链接

- [The Kubernetes API - Official Documentation](https://kubernetes.io/docs/concepts/overview/kubernetes-api/)
