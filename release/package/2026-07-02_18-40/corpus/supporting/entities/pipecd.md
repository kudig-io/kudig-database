---
title: PipeCD [entities]
description: '## 概述'
summary: 'PipeCD 是一个统一的持续交付平台，为 Kubernetes、Terraform、CloudRun、Lambda、ECS 等多种应用平台提供一致的 GitOps 部署体验。它采用控制平面（Control Plane）+ 代理（Piped）架构，支持渐进式交付策略（金丝雀、蓝绿、滚动）和自动回滚。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- pipecd
- prometheus
- grafana
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- PipeCD 是什么
- 如何 PipeCD
trigger_keywords:
- PipeCD
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PipeCD

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

PipeCD 是一个统一的持续交付平台，为 Kubernetes、Terraform、CloudRun、Lambda、ECS 等多种应用平台提供一致的 GitOps 部署体验。它采用控制平面（Control Plane）+ 代理（Piped）架构，支持渐进式交付策略（金丝雀、蓝绿、滚动）和自动回滚。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **渐进式交付**: 所有生产部署使用金丝雀或蓝绿策略，避免一次性全量发布
- **自动分析**: 配置 Prometheus 指标分析，在金丝雀阶段自动检测异常
- **审批门控**: 关键阶段设置 WAIT_APPROVAL，确保人工确认
- **Piped 隔离**: 每个环境/集群部署独立的 Piped，缩小爆炸半径
- **Secret 管理**: 使用 Sealed [[Secrets|Secrets]] 或 SOPS 加密 Git 中的敏感配置
- **多集群**: 通过 Piped 代理实现多集群部署，无需直连集群 API

## 架构定位

在 CNCF 生态中，pipecd 属于 **CI/CD** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[concepts/gitops-principles.md|gitops-principles]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[cubefs]] — CubeFS
- [[artifact-hub]] — Artifact Hub
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[sops]] — SOPS (Secrets OPerationS)

- pipecd
- [[entities/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
