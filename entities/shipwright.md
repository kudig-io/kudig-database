---
title: Shipwright (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- ci-cd
- shipwright
- cri-o
- crd
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Shipwright 是什么
- 如何 Shipwright
trigger_keywords:
- Shipwright
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Shipwright

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

Shipwright 是一个在 Kubernetes 上构建容器镜像的框架。它抽象了底层构建工具的差异，通过统一的 CRD API 支持 Buildpacks, Buildah, BuildKit, Kaniko 等多种构建策略。开发者只需定义源码位置和目标镜像，Shipwright 自动管理构建过程。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **策略选择**: Go 应用用 [[ko|ko]]，通用语言用 Buildpacks，需要精细控制用 Buildah
- **镜像缓存**: 配置构建缓存加速重复构建
- **安全**: 使用 Kaniko/Buildah 避免特权容器构建
- **CI 集成**: 在 CI/CD pipeline 中创建 BuildRun 实现自动化构建
- **多平台**: 使用支持多平台的策略构建 arm64/amd64 镜像
- **资源限制**: 为构建 Pod 设置合理的 CPU/内存限制

## 架构定位

在 CNCF 生态中，shipwright 属于 **CI/CD** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/storage-model.md|storage-model]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kubescape]] — Kubescape
- [[cedar]] — Cedar
- [[cri-o]] — CRI-O
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[buildpacks]] — Cloud Native Buildpacks

- shipwright
- [[entities/atlantis.md|Atlantis]]
- [[entities/dalec.md|Dalec]]
- [[entities/werf.md|werf]]
- [[entities/pipecd.md|PipeCD]]
- [[entities/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
