---
title: cdk8s 声明式 K8s CDK
description: cdk8s（Cloud Development Kit for Kubernetes）是 CNCF Sandbox 项目，允许使用 TypeScript/Pyt...
summary: cdk8s（Cloud Development Kit for Kubernetes）是 CNCF Sandbox 项目，允许使用 TypeScript/Pyt...
category: dictionary
tags:
- k8s
- glossary
- tooling
- cdk
- configuration
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cdk8s 声明式 K8s CDK 是什么
- cdk8s 详解
trigger_keywords:
- cdk8s 声明式 K8s CDK
- cdk8s
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# cdk8s 声明式 K8s CDK（cdk8s）

## 概述

cdk8s（Cloud Development Kit for Kubernetes）是 CNCF Sandbox 项目，允许使用 TypeScript/Python/Java/Go 等编程语言定义 Kubernetes 资源，编译为标准 YAML 清单。

## 核心概念/原理

- **编程式定义**：用编程语言（非 YAML）定义 K8s 资源
- **类型安全**：利用编程语言的类型系统检查配置
- **CNCF Sandbox**：AWS CDK 团队主导
- **多语言**：TypeScript/Python/Java/Go

## 关键机制或特性

- Constructs 组件模型（可复用资源组合）
- Charts 图表（K8s 资源集合）
- Apps 应用（Chart 集合）
- cdk8s import 导入 CRD 类型
- Helm Chart 集成（cdk8s-plus）
- cdk8s synth 合成 YAML 输出
- cdk8s-plus 高级抽象库

## 使用场景与最佳实践

- 复杂 K8s 配置的编程化管理
- 需要类型安全的配置定义
- 配置模板的复用和组合
- Helm/Kustomize 的编程式替代
- Infrastructure as Code 的统一

## 参考链接

- https://cdk8s.io/
- https://github.com/cdk8s-team/cdk8s

## Related

- [[系统基础/知识字典/tooling/helm.md|Helm]]
- [[系统基础/知识字典/tooling/kustomize.md|Kustomize]]
- [[系统基础/知识字典/configuration/kcl.md|KCL]]


<!-- risk-assessed -->
