---
title: Capsule 多租户管理
description: Capsule 是 CNCF Sandbox 项目，为 Kubernetes 提供轻量级多租户管理，通过 Tenant CRD 实现命名空间级别的资源隔离和策略...
summary: Capsule 是 CNCF Sandbox 项目，为 Kubernetes 提供轻量级多租户管理，通过 Tenant CRD 实现命名空间级别的资源隔离和策略...
category: dictionary
tags:
- k8s
- glossary
- security
- multi-tenancy
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Capsule 多租户管理 是什么
- Capsule 详解
trigger_keywords:
- Capsule 多租户管理
- Capsule
- dictionary
prerequisites:
- kubernetes
---



# Capsule 多租户管理（Capsule）

## 概述

Capsule 是 CNCF Sandbox 项目，为 Kubernetes 提供轻量级多租户管理，通过 Tenant CRD 实现命名空间级别的资源隔离和策略管理，无需引入额外的控制面组件。

## 核心概念/原理

- **轻量多租户**：通过 CRD 和 Admission Webhook 实现，无需额外控制面
- **命名空间隔离**：每个租户拥有独立的命名空间集合
- **策略继承**：租户级策略自动应用到其所有命名空间
- **CNCF Sandbox**：Clastix 主导开发

## 关键机制或特性

- Tenant CRD 定义租户及其命名空间
- NetworkPolicy 自动注入（租户间隔离）
- ResourceQuota / LimitRange 按租户管理
- 存储类限制（每租户可用 StorageClass）
- Ingress 类限制（每租户可用 IngressClass）
- 节点选择器限制（NodeSelector 按租户隔离）

## 使用场景与最佳实践

- 企业内部的 K8s 多租户管理
- 开发团队的资源隔离
- SaaS 平台的租户管理
- 共享集群的安全隔离
- 替代 vCluster / OCM 的轻量方案

## 参考链接

- https://capsule.clastix.io/
- https://github.com/clastix/capsule

## Related

- [[系统基础/知识字典/security/rbac.md|RBAC]]
- [[系统基础/知识字典/networking/networkpolicy.md|NetworkPolicy]]
- [[系统基础/知识字典/security/opa.md|OPA]]
