---
title: Crossplane 资源组合
description: Crossplane Composition 是 Crossplane 的组合式基础设施管理特性，通过 CompositeResourceDefinition（...
summary: Crossplane Composition 是 Crossplane 的组合式基础设施管理特性，通过 CompositeResourceDefinition（...
category: dictionary
tags:
- k8s
- glossary
- multi-cloud
- crossplane
- iac
tier: peripheral
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Crossplane 资源组合 是什么
- Crossplane Composition 详解
trigger_keywords:
- Crossplane 资源组合
- Crossplane Composition
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Crossplane 资源组合（Crossplane Composition）

## 概述

Crossplane Composition 是 Crossplane 的组合式基础设施管理特性，通过 CompositeResourceDefinition（XRD）和 Composition 将底层云资源抽象为面向平台用户的高级 API。

## 核心概念/原理

- **抽象层**：将底层云资源包装为平台 API
- **XRD**：CompositeResourceDefinition 定义新资源类型
- **Composition**：定义 XRD 到具体资源的映射
- **多 Provider**：AWS/Azure/GCP/K8s 等 Provider

## 关键机制或特性

- XRD 定义面向用户的抽象 API
- Composition 定义资源组合和转换逻辑
- Composition Functions 可编程的转换逻辑
- Patch Sets 声明式参数传递
- Multiple Compositions 支持
- 环境配置（EnvironmentConfig）
- Usage（资源使用记录）

## 使用场景与最佳实践

- 内部开发平台（IDP）的基础设施 API
- 多云资源的统一管理接口
- 自助式基础设施服务
- 基础设施的标准化和合规
- 最佳实践：合理的抽象层级、版本演进策略、充分的测试

## 参考链接

- https://docs.crossplane.io/latest/concepts/compositions/
- https://github.com/crossplane/crossplane

## Related

- [[17-系统基础/06-知识字典/platform-engineering/crossplane.md|Crossplane]]
- [[17-系统基础/06-知识字典/platform-engineering/backstage.md|Backstage]]
- [[17-系统基础/06-知识字典/multi-cloud/cluster-api.md|Cluster API]]


<!-- risk-assessed -->
