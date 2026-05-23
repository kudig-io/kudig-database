---
title: werf [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- ci-cd
- werf
- helm
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- werf 是什么
- 如何 werf
trigger_keywords:
- werf
prerequisites:
- kubectl-basics
- helm-basics
created: "2026-05-23"
---

# werf

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

werf 是一个一致且可复现的 CI/CD 交付工具，将 Git 作为唯一真相来源，集成了镜像构建、镜像发布、Helm 部署和清理策略。werf 提供从源码到部署的完整流水线，特别强调构建的可复现性和基于内容的标签策略。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Giterminism**: 保持 werf.yaml 中所有配置来自 Git，确保构建可复现
- **Stage 依赖**: 使用 stageDependencies 精确控制缓存失效范围
- **基于内容的标签**: 使用默认的 content-based 标签策略确保部署与构建一致
- **自动清理**: 在 CI 中定期运行 `werf cleanup` 清理未使用的镜像
- **Helm values 分离**: 为不同环境维护独立的 values 文件
- **资源跟踪**: 利用 werf 的增强 Helm 部署监控资源就绪状态

## 架构定位

在 CNCF 生态中，werf 属于 **CI/CD** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[concepts/ci-cd-pipeline-patterns|ci-cd-pipeline-patterns]]

## Related

- [[pipecd]] — PipeCD
- [[hami]] — HAMI
- [[open-policy-containers]] — [[entities/open-policy-containers|Open Policy Containers (OPCR)]]
- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- werf
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
