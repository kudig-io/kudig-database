---
title: Cloud Native Buildpacks
description: '## 概述'
summary: 'Cloud Native Buildpacks (CNB) 将应用源代码转换为 OCI 容器镜像，无需编写 Dockerfile。它自动检测应用类型、安装依赖、配置运行环境，简化了容器化流程并提高镜像安全性。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- buildpacks
- docker
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cloud Native Buildpacks 是什么
- 如何 Cloud Native Buildpacks
trigger_keywords:
- Cloud
- Native
- Buildpacks
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cloud Native [[Buildpacks|Buildpacks]]

> **CNCF 状态**: Incubating | **类别**: Ci/Cd | **主要语言**: Go

## 概述

Cloud Native Buildpacks (CNB) 将应用源代码转换为 OCI 容器镜像，无需编写 Dockerfile。它自动检测应用类型、安装依赖、配置运行环境，简化了容器化流程并提高镜像安全性。

## 核心能力

- **自动检测**: 智能识别应用语言和框架
- **模块化**: Buildpack 可组合、可复用
- **Rebasing**: 无需重新构建即可更新基础镜像
- **SBOM 生成**: 自动生成软件物料清单
- **缓存优化**: 分层缓存加速构建
- **多平台**: 支持 AMD64、ARM64 等架构

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **选择合适的 Builder**: tiny 用于生产（最小镜像），full 用于调试
- **利用缓存**: 使用 cache-image 加速 CI/CD 构建
- **Rebase 更新**: 定期 rebase 以获取安全补丁
- **SBOM 集成**: 将 SBOM 纳入供应链安全流程
- **版本锁定**: 使用 project.toml 锁定 buildpack 版本

## 架构定位

在 CNCF 生态中，buildpacks 属于 **Ci/Cd** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[kepler]] — Kepler
- [[kubestellar]] — KubeStellar
- [[athenz]] — Athenz
- [[metallb]] — MetalLB
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- buildpacks
- [[entities/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference
- [[生态参考/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
