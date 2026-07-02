---
title: ko (entities)
description: '## 概述'
summary: 'ko 是一个快速的 Go 应用容器镜像构建和部署工具。它无需 Docker 或 Dockerfile，直接从 Go 源码构建 OCI 兼容的容器镜像，并推送到容器注册表。ko 的核心理念是简化 Go 应用的容器化流程，实现极速构建和部署。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- ko
- docker
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
- ko 是什么
- 如何 ko
trigger_keywords:
- ko
prerequisites:
- kubectl-basics
---



# ko

> **CNCF 状态**: Sandbox | **类别**: Ci/Cd | **主要语言**: Go

## 概述

ko 是一个快速的 Go 应用容器镜像构建和部署工具。它无需 Docker 或 Dockerfile，直接从 Go 源码构建 OCI 兼容的容器镜像，并推送到容器注册表。ko 的核心理念是简化 Go 应用的容器化流程，实现极速构建和部署。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **基础镜像**: 使用 distroless 或 chainguard/static 作为基础镜像减少攻击面
- **多平台**: 生产构建使用 `--platform=linux/amd64,linux/arm64` 支持多架构
- **CI 集成**: 在 CI/CD 中使用 ko 替代 Docker build，消除 Docker daemon 依赖
- **镜像签名**: 启用 SBOM 生成和 cosign 签名，确保供应链安全
- **YAML 管理**: 使用 `ko://` 前缀引用 Go 应用，简化镜像版本管理
- **编译优化**: 在 .ko.yaml 中配置 `-trimpath -s -w` 减小二进制大小

## 架构定位

在 CNCF 生态中，ko 属于 **Ci/Cd** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[xregistry]] — xRegistry
- [[carvel]] — Carvel
- [[holmesgpt]] — HolmesGPT
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/cli-tools-evolution.md|[[CLI 工具演进|CLI 工具演进]]]] — Cross-reference
- [[entities/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
