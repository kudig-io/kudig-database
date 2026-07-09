---
title: Carvel (entities)
description: '## 概述'
summary: 'Carvel 是一组专注于 Kubernetes 应用构建、配置和部署的工具集。它采用 Unix 哲学，每个工具专注于单一任务并可组合使用。主要包括 ytt (YAML 模板)、kbld (镜像构建)、kapp (应用部署)、imgpkg (OCI 镜像打包)、vendir (依赖管理) 和 kapp-controller (GitOps)。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- carvel
- crd
- operator
- kubeflow
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Carvel 是什么
- 如何 Carvel
trigger_keywords:
- Carvel
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Carvel

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

Carvel 是一组专注于 Kubernetes 应用构建、配置和部署的工具集。它采用 Unix 哲学，每个工具专注于单一任务并可组合使用。主要包括 ytt (YAML 模板)、kbld (镜像构建)、kapp (应用部署)、imgpkg (OCI 镜像打包)、vendir (依赖管理) 和 kapp-controller (GitOps)。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **模块化配置**: 使用 ytt overlay 分离环境配置
- **镜像锁定**: 使用 kbld 锁定镜像 digest
- **Bundle 打包**: 使用 imgpkg 打包可重定位的应用
- **依赖管理**: 使用 vendir 统一管理外部依赖
- **GitOps**: 使用 kapp-controller 实现声明式部署
- **版本控制**: 锁文件纳入版本控制

## 架构定位

在 CNCF 生态中，carvel 属于 **CI/CD** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/gitops-principles.md|gitops-principles]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[kubeflow]] — Kubeflow
- [[spiffe]] — SPIFFE
- [[kubeelasti]] — [[entities/kubeelasti.md|KubeElastic]]
- [[xregistry]] — xRegistry
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- carvel
- [[entities/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
