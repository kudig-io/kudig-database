---
title: OpenKruise [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- orchestration
- openkruise
- statefulset
- daemonset
- job
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenKruise 是什么
- 如何 OpenKruise
trigger_keywords:
- OpenKruise
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# OpenKruise

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

OpenKruise 是 Kubernetes 的增强工作负载套件，提供高级部署、原地升级、Sidecar 管理等能力。它扩展了 Kubernetes 原生工作负载，解决大规模应用管理的痛点问题。

## 核心能力

- **高级工作负载**: CloneSet、Advanced [[StatefulSet|StatefulSet]]、Advanced [[DaemonSet|DaemonSet]]
- **原地升级**: 更新镜像无需重建 Pod
- **Sidecar 管理**: 声明式 Sidecar 注入和独立升级
- **镜像预热**: 提前拉取镜像加速部署
- **容器重启**: 不重建 Pod 的情况下重启容器
- **保护机制**: PodUnavailableBudget 防止误操作

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **原地升级**: 对于无状态应用优先使用原地升级减少调度开销
- **镜像预热**: 大规模发布前使用 ImagePullJob 预热镜像
- **Sidecar 管理**: 使用 SidecarSet 统一管理 Sidecar 版本
- **保护机制**: 配置 PodUnavailableBudget 防止误操作
- **分批发布**: 使用 partition 实现灰度发布

## 架构定位

在 CNCF 生态中，openkruise 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[krkn]] — Krkn
- [[opengitops]] — OpenGitOps
- [[cadence]] — Cadence
- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/openkruise-fta.md|OpenKruise 工作负载异常故障树分析]]
- openkruise
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/openkruise-index|OpenKruise 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
