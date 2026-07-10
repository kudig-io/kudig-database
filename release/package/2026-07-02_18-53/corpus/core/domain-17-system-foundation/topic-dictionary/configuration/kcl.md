---
title: KCL 配置语言
description: KCL（Kusion Configuration Language）是蚂蚁集团开源并捐赠给 CNCF 的配置语言，专为云原生场景设计，提供类型系统、模块化和策略...
summary: KCL（Kusion Configuration Language）是蚂蚁集团开源并捐赠给 CNCF 的配置语言，专为云原生场景设计，提供类型系统、模块化和策略...
category: dictionary
tags:
- k8s
- glossary
- configuration
- language
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
- KCL 配置语言 是什么
- KCL 详解
trigger_keywords:
- KCL 配置语言
- KCL
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KCL 配置语言（KCL）

## 概述

KCL（Kusion Configuration Language）是蚂蚁集团开源并捐赠给 CNCF 的配置语言，专为云原生场景设计，提供类型系统、模块化和策略校验能力，是 YAML/Helm 的编程式替代方案。

## 核心概念/原理

- **编程式配置**：类型系统、循环、条件、函数等编程能力
- **云原生专注**：内置 Kubernetes 模型和校验规则
- **模块复用**：包管理和模块系统支持配置复用
- **CNCF Sandbox**：蚂蚁集团开源

## 关键机制或特性

- 强类型系统（类型推断 + 类型检查）
- 内置 Schema 和 Validation
- KPM 包管理器（类似 pip/go mod）
- 与 Helm/Terraform/Crossplane 等工具集成
- 配置策略检查（Policy as Code）
- IDE 支持（VS Code 插件、LSP）

## 使用场景与最佳实践

- 大规模 Kubernetes 配置的编程化管理
- 替代 Helm/Kustomize 的复杂配置场景
- 配置策略的自动化校验
- 多环境配置的统一管理
- 基础设施即代码（IaC）配置编写

## 参考链接

- https://kcl-lang.io/
- https://github.com/kcl-lang/kcl

## Related

- [[domain-17-system-foundation/知识字典/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/知识字典/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/知识字典/platform-engineering/crossplane.md|Crossplane]]


<!-- risk-assessed -->
