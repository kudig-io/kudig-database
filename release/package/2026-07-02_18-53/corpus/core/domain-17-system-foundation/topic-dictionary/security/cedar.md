---
title: Cedar 策略语言
description: Cedar 是 AWS 开源的策略语言，用于定义和执行细粒度授权策略，语法简洁直观，专为应用级权限管理设计，已被 Amazon Verified
  Permiss...
summary: Cedar 是 AWS 开源的策略语言，用于定义和执行细粒度授权策略，语法简洁直观，专为应用级权限管理设计，已被 Amazon Verified
  Permiss...
category: dictionary
tags:
- k8s
- glossary
- security
- policy
- authorization
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cedar 策略语言 是什么
- Cedar 详解
trigger_keywords:
- Cedar 策略语言
- Cedar
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cedar 策略语言（Cedar）

## 概述

Cedar 是 AWS 开源的策略语言，用于定义和执行细粒度授权策略，语法简洁直观，专为应用级权限管理设计，已被 Amazon Verified Permissions 采用。

## 核心概念/原理

- **策略语言**：专为授权决策设计的声明式语言
- **AWS 背景**：Amazon Verified Permissions 的核心引擎
- **形式化验证**：支持策略的形式化验证
- **应用集成**：嵌入到应用中的授权引擎

## 关键机制或特性

- Entity（用户/资源/动作的定义）
- Policy（when/unless 条件的策略规则）
- 层次化资源模型
- 策略组（Policy Set）管理
- 策略评估（is-authorized API）
- 形式化验证工具
- SDK（Rust/Java/Go）

## 使用场景与最佳实践

- 应用的细粒度授权策略
- 多租户 SaaS 的权限管理
- AWS 资源的 IAM 策略
- 替代 OPA 的轻量策略方案
- 需要形式化验证的安全策略

## 参考链接

- https://www.cedarpolicy.com/
- https://github.com/cedar-policy/cedar

## Related

- [[domain-17-system-foundation/知识字典/security/opa.md|OPA]]
- [[domain-17-system-foundation/知识字典/security/openfga.md|OpenFGA]]
- [[domain-17-system-foundation/知识字典/security/kyverno.md|Kyverno]]


<!-- risk-assessed -->
