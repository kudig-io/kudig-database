---
title: Cedar (entities)
description: '## 概述'
summary: 'Cedar 是一个由 AWS 开发的开源策略语言和评估引擎，用于定义和执行细粒度的访问控制策略。它专为应用程序的授权决策设计，提供人类可读的策略语法、形式化验证工具和高性能的策略评估引擎。'
category: entities
tags:
- k8s
- cncf
- orchestration
- cedar
- argocd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cedar 是什么
- 如何 Cedar
trigger_keywords:
- Cedar
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cedar

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Rust

## 概述

Cedar 是由 AWS 开发并捐赠给 CNCF 沙箱的策略语言和评估引擎，专为云原生授权和访问控制设计。它提供了一种声明式的、可分析的策略语言，让开发者能够以简洁的语法定义细粒度的访问控制策略。Cedar 的设计灵感来源于 Amazon Verified Permissions 和 IAM 策略语言，但提供了更强的表达能力和形式化验证能力。Cedar 已在 AWS Verified Permissions、Okta 等产品中获得商业应用。

## Key Features（核心能力）

- **声明式策略语言**：使用类自然语言语法定义权限策略，可读性高
- **高性能评估引擎**：Rust 实现的策略评估引擎，支持亚毫秒级决策
- **细粒度访问控制**：支持基于属性（ABAC）和基于角色（RBAC）的混合模型
- **形式化验证**：策略可进行形式化分析和自动推理验证
- **Schema 驱动**：通过 Schema 定义实体和操作，实现策略类型安全
- **可审计性**：策略评估日志支持合规审计和调试

## 架构与工作原理

Cedar 架构由三部分组成：Schema 定义实体类型（Principal, Resource, Action）和层级关系；Policy 以 permit/forbid 子句定义授权规则；Evaluator 引擎接收请求（Principal, Action, Resource, Context）并返回 Allow/Deny 决策。策略编译为高效的中间表示（IR），评估时通过图遍历检查权限传播路径。

## K8s 集成

Cedar 可集成到 Kubernetes Admission Webhook 中作为授权策略引擎，替代或增强 RBAC。通过 ValidatingWebhook 拦截 API 请求，使用 Cedar 策略进行细粒度授权决策。相比 K8s 原生 RBAC，Cedar 能表达更复杂的条件策略（如基于资源标签、时间、地理区域的访问控制）。

## 生产用例

- **细粒度 API 授权**：超越 RBAC，实现基于资源属性和环境条件的访问控制
- **多租户权限隔离**：在共享集群中为不同租户定义定制化授权策略
- **应用层授权**：微服务间调用的细粒度权限控制
- **合规审计**：策略可分析和可审计性满足合规要求

## 安装与快速开始

```bash
# Rust crate
cargo add cedar-policy

# CLI
cargo install cedar-policy-cli
```

## 对比替代方案

相比 OPA/Rego，Cedar 提供更强的类型安全和形式化验证能力。相比 K8s 原生 RBAC，Cedar 能表达更丰富的条件策略。相比 AWS IAM Policy，Cedar 是开源的、可本地部署的。

## Related

- [[clusternet]] — Clusternet
- [[kubeslice]] — KubeSlice
- [[hyperlight]] — Hyperlight
- [[kubescape]] — Kubescape
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cedar
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference


<!-- risk-assessed -->
