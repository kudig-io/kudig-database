---
title: 自定义资源 CRD
description: 'Custom Resource（CR）是 Kubernetes 的核心扩展机制，通过 CRD（CustomResourceDefinition）注册自定义资源类...'
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- crd
- extension
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 自定义资源 CRD 是什么
- Custom Resource 详解
trigger_keywords:
- 自定义资源 CRD
- Custom Resource
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# 自定义资源 CRD（Custom Resource）

## 概述

Custom Resource（CR）是 Kubernetes 的核心扩展机制，通过 CRD（CustomResourceDefinition）注册自定义资源类型，将任意领域模型纳入 K8s 的声明式 API 管理体系。

## 核心概念/原理

- **CRD**：CustomResourceDefinition 定义新资源类型
- **CR**：Custom Resource 是 CRD 的实例
- **声明式 API**：CR 遵循 K8s 的声明式管理范式
- **Operator 模式**：CR + Controller = Operator

## 关键机制或特性

- CRD YAML 定义资源 schema（OpenAPI v3）
- API Group/Version/Kind 注册到 API Server
- 验证（Validation）通过 OpenAPI schema
- 子资源（status/scale）支持
- Webhook（准入控制和转换）
- Finalizers 生命周期管理
- 版本管理和转换（conversion webhook）

## 使用场景与最佳实践

- 平台能力的 API 化（数据库/消息队列/证书）
- 运维自动化（备份策略/巡检任务）
- 业务模型的 K8s 化（工单/配置中心）
- Operator 开发的基础
- 最佳实践：版本演进、向后兼容、Status 子资源、条件（Conditions）

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/api-extension/custom-resources/
- https://book.kubebuilder.io/

## Related
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/operator-pattern.md|Operator Pattern]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes.md|Kubernetes]]
