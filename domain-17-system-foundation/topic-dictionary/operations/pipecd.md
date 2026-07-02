---
title: PipeCD 持续交付
description: PipeCD 是 Cybozu 开源的 CNCF Sandbox 持续交付平台，支持 Kubernetes、ECS、Lambda、Terraform
  等多种部署...
summary: PipeCD 是 Cybozu 开源的 CNCF Sandbox 持续交付平台，支持 Kubernetes、ECS、Lambda、Terraform
  等多种部署...
category: dictionary
tags:
- k8s
- glossary
- operations
- ci-cd
- gitops
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- PipeCD 持续交付 是什么
- PipeCD 详解
trigger_keywords:
- PipeCD 持续交付
- PipeCD
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PipeCD 持续交付（PipeCD）

## 概述

PipeCD 是 Cybozu 开源的 CNCF Sandbox 持续交付平台，支持 Kubernetes、ECS、Lambda、Terraform 等多种部署目标的统一 GitOps 管理，提供金丝雀、蓝绿等高级部署策略。

## 核心概念/原理

- **多目标**：统一支持 K8s/ECS/Lambda/Cloud Run/Terraform
- **GitOps**：以 Git 仓库为唯一配置源
- **高级策略**：金丝雀、蓝绿、渐进式交付
- **CNCF Sandbox**：Cybozu 主导

## 关键机制或特性

- Application CRD 定义部署目标
- Analysis 自动化分析（Prometheus/DataDog/Stackdriver）
- 渐进式交付（Canary / Blue-Green / Rolling）
- Web UI 可视化管理
- 多集群 / 多环境管理
- Encryption 敏感配置加密
- Notification 集成（Slack/Teams）

## 使用场景与最佳实践

- 多平台（K8s + Serverless + VM）的统一 CD
- 需要渐进式交付的生产部署
- GitOps 实践中的持续交付
- 多团队 / 多环境的部署管理
- 自动化分析驱动的安全发布

## 参考链接

- https://pipecd.dev/
- https://github.com/pipe-cd/pipecd

## Related

- [[domain-17-system-foundation/topic-dictionary/operations/argo.md|Argo]]
- [[domain-17-system-foundation/topic-dictionary/operations/flux.md|Flux]]
- [[domain-17-system-foundation/topic-dictionary/operations/flagger.md|Flagger]]


<!-- risk-assessed -->
