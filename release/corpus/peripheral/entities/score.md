---
title: Score (entities)
description: '## 概述'
summary: 'Score 是一个与平台无关的工作负载规范，使开发者能够用统一的格式描述其工作负载需求（容器、资源依赖、环境变量等），然后由 Score 实现工具（score-compose, score-k8s, score-humanitec）将规范翻译为目标平台的原生配置。'
category: entities
tags:
- k8s
- cncf
- orchestration
- score
- crd
- operator
- kubelet
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Score 是什么
- 如何 Score
trigger_keywords:
- Score
prerequisites:
- kubectl-basics
---



# Score

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Score 是一个与平台无关的工作负载规范，使开发者能够用统一的格式描述其工作负载需求（容器、资源依赖、环境变量等），然后由 Score 实现工具（score-compose, score-k8s, score-humanitec）将规范翻译为目标平台的原生配置。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **资源抽象**: 使用 resources 声明依赖，让平台团队决定具体实现
- **环境变量**: 通过 `${resources.xxx}` 引用资源属性，保持可移植性
- **本地开发**: 使用 score-compose 进行本地开发，score-k8s 部署到集群
- **团队协作**: 开发者专注 Score 规范，平台团队维护 provisioners
- **版本控制**: 将 score.yaml 纳入 Git 管理

## 架构定位

在 CNCF 生态中，score 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]]

## Related

- [[meshery]] — Meshery
- [[knative]] — Knative
- [[konveyor]] — Konveyor
- [[bfe]] — BFE
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- score
- [[concepts/scheduling-algorithm.md|[[Scheduling Algorithm|Scheduling Algorithm]]]] — Cross-reference
- [[skills/kubelet-eviction-mechanism.md|kubelet 资源驱逐机制]] — Cross-reference
- [[skills/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]] — Cross-reference
- [[entities/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
