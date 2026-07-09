---
title: OpenChoreo (entities)
description: '## 概述'
summary: 'OpenChoreo 是一个云原生的内部开发者平台 (IDP) 框架，提供开箱即用的开发者自助服务门户。它基于 Kubernetes 构建，为开发团队提供应用创建、部署、监控的统一界面，同时让平台团队可以通过声明式配置定义黄金路径 (Golden Path) 和治理策略。'
category: entities
tags:
- k8s
- cncf
- platform
- openchoreo
- prometheus
- grafana
- argocd
- flux
- opa
- crd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenChoreo 是什么
- 如何 OpenChoreo
trigger_keywords:
- OpenChoreo
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenChoreo

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

OpenChoreo 是一个云原生的内部开发者平台 (IDP) 框架，提供开箱即用的开发者自助服务门户。它基于 Kubernetes 构建，为开发团队提供应用创建、部署、监控的统一界面，同时让平台团队可以通过声明式配置定义黄金路径 (Golden Path) 和治理策略。OpenChoreo 旨在简化 [[concepts/platform-engineering-sre.md|Platform Engineering]] 的实施复杂度。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **模板标准化**: 为不同技术栈创建标准化的应用模板
- **渐进式策略**: 从宽松的黄金路径规则开始，逐步收紧
- **自助为主**: 尽量让开发者通过 Portal 完成所有操作，减少工单
- **可观测性**: 确保每个应用都有统一的监控和日志入口
- **版本控制**: 所有平台配置都纳入 Git 版本控制

## 架构定位

在 CNCF 生态中，openchoreo 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[flux]]
- [[entities/argocd.md|[[ArgoCD|argocd]]]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]

## Related

- [[copa]] — Copa (Copacetic)
- [[nats]] — NATS
- [[paralus]] — Paralus
- [[hexa]] — Hexa
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openchoreo
- [[entities/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
