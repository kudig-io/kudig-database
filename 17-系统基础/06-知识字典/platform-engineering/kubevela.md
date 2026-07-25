---
title: KubeVela 应用交付
description: KubeVela 是阿里巴巴开源的 CNCF 孵化项目，基于 OAM（Open Application Model）的现代应用交付平台，提供声明式、可扩展、面向...
summary: KubeVela 是阿里巴巴开源的 CNCF 孵化项目，基于 OAM（Open Application Model）的现代应用交付平台，提供声明式、可扩展、面向...
category: dictionary
tags:
- k8s
- glossary
- platform-engineering
- oam
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
- KubeVela 应用交付 是什么
- KubeVela 详解
trigger_keywords:
- KubeVela 应用交付
- KubeVela
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KubeVela 应用交付（KubeVela）

## 概述

KubeVela 是阿里巴巴开源的 CNCF 孵化项目，基于 OAM（Open Application Model）的现代应用交付平台，提供声明式、可扩展、面向最终用户的应用管理能力。

## 核心概念/原理

- **OAM 实现**：Open Application Model 的参考实现
- **可扩展**：CUE 语言定义可复用的组件和工作流
- **CNCF 孵化**：阿里巴巴主导
- **多集群**：支持多集群应用分发

## 关键机制或特性

- Application CRD 定义应用
- ComponentDefinition / TraitDefinition 组件扩展
- Workflow 步骤定义（部署/检查/通知等）
- 多集群环境管理
- Helm/Kustomize/Terraform 集成
- VelaUX 可视化管理界面
- Addon 插件市场

## 使用场景与最佳实践

- 平台团队的 IDP 底层引擎
- 复杂应用的多集群交付
- OAM 标准的应用管理
- 开发者自助服务平台
- GitOps 应用交付

## 参考链接

- https://kubevela.io/
- https://github.com/kubevela/kubevela

## Related

- [[17-系统基础/06-知识字典/platform-engineering/crossplane.md|Crossplane]]
- [[17-系统基础/06-知识字典/operations/argo.md|Argo]]
- [[17-系统基础/06-知识字典/platform-engineering/score.md|Score]]


<!-- risk-assessed -->
