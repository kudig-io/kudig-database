---
title: werf [entities]
description: '## 概述'
summary: 'werf 是一个一致且可复现的 CI/CD 交付工具，将 Git 作为唯一真相来源，集成了镜像构建、镜像发布、Helm 部署和清理策略。werf 提供从源码到部署的完整流水线，特别强调构建的可复现性和基于内容的标签策略。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- werf
- helm
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
- werf 是什么
- 如何 werf
trigger_keywords:
- werf
prerequisites:
- kubectl-basics
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# werf

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

werf 是一个一致且可复现的 CI/CD 交付工具，将 Git 作为唯一真相来源，集成了镜像构建、镜像发布、Helm 部署和清理策略。werf 提供从源码到部署的完整流水线，特别强调构建的可复现性和基于内容的标签策略。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

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
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[pipecd]] — PipeCD
- [[hami]] — HAMI
- [[open-policy-containers]] — [[实体/open-policy-containers.md|Open Policy Containers (OPCR)]]
- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- werf
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
