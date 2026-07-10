---
title: Capsule (entities)
description: '## 概述'
summary: 'Capsule 是一个 Kubernetes 多租户框架，允许在单个集群中实现多租户隔离。它通过 Tenant CRD 将多个命名空间组织为逻辑单元，为每个租户提供隔离的资源配额、网络策略和 RBAC 控制。与传统的每租户一集群方案相比，Capsule 显著降低了运维复杂度和成本。'
category: entities
tags:
- k8s
- cncf
- policy
- capsule
- prometheus
- ingress
- rbac
- networkpolicy
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
- Capsule 是什么
- 如何 Capsule
trigger_keywords:
- Capsule
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Capsule

> **CNCF 状态**: Sandbox | **类别**: Policy | **主要语言**: Go

## 概述

Capsule 是一个 Kubernetes 多租户框架，允许在单个集群中实现多租户隔离。它通过 Tenant CRD 将多个命名空间组织为逻辑单元，为每个租户提供隔离的资源配额、网络策略和 RBAC 控制。与传统的每租户一集群方案相比，Capsule 显著降低了运维复杂度和成本。

## 核心能力

- **多租户隔离**: 单集群内实现强隔离的多租户
- **命名空间聚合**: 将多个命名空间归属到单个租户
- **资源配额**: 租户级别的资源限制和配额
- **网络隔离**: 自动应用 [[NetworkPolicy|NetworkPolicy]] 实现租户隔离
- **RBAC 管理**: 租户所有者自助管理命名空间
- **自定义策略**: 限制 NodePort、[[Ingress|Ingress]]、存储类等

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **租户规划**: 按团队或项目划分租户
- **配额设置**: 合理设置资源配额防止滥用
- **网络隔离**: 默认启用租户间网络隔离
- **镜像限制**: 限制容器镜像来源
- **Proxy 使用**: 使用 Capsule Proxy 提升用户体验
- **审计日志**: 启用 Kubernetes 审计追踪租户操作

## 架构定位

在 CNCF 生态中，capsule 属于 **Policy** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/networkpolicy.md|networkpolicy]]
- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]

## Related

- [[buildpacks]] — Cloud Native Buildpacks
- [[kube-rs]] — kube-rs
- [[02-prometheus-promql-advanced]] — PromQL 高级查询
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- capsule
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
