---
title: bootc (entities)
description: '## 概述'
summary: 'bootc 是一个基于 OCI 容器镜像的 Linux 系统启动和升级工具，将容器镜像作为操作系统的部署单元。它允许使用标准的容器构建工具（如 Dockerfile）来定义和构建可启动的 Linux 系统，并通过事务性更新机制实现系统的原子升级和回滚。bootc 将容器工作流的优势（镜像注册中心、版本标签、CI/CD 流水线）引入操作系统管理领域。'
category: entities
tags:
- k8s
- cncf
- runtime
- bootc
- docker
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- bootc 是什么
- 如何 bootc
trigger_keywords:
- bootc
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# bootc

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

bootc 是一个基于 OCI 容器镜像的 Linux 系统启动和升级工具，将容器镜像作为操作系统的部署单元。它允许使用标准的容器构建工具（如 Dockerfile）来定义和构建可启动的 Linux 系统，并通过事务性更新机制实现系统的原子升级和回滚。bootc 将容器工作流的优势（镜像注册中心、版本标签、CI/CD 流水线）引入操作系统管理领域。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **基础镜像选择**: 使用官方 bootc 基础镜像（Fedora/CentOS bootc）作为起点
- **分层构建**: 将通用配置放在基础层，应用特定配置放在上层
- **CI/CD 集成**: 将系统镜像构建集成到 CI/CD 流水线，自动测试和发布
- **版本标签**: 使用语义版本标签管理系统镜像，保留回滚路径
- **最小化镜像**: 只安装必要的包，减小镜像大小和攻击面

## 架构定位

在 CNCF 生态中，bootc 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[pod-lifecycle]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[entities/cncf-edge-ai.md|cncf-edge-ai]] — CNCF 边缘计算与 AI/ML 项目全景
- [[confidential-containers]] — Confidential Containersrs (CoCo)|Confidential Containers (CoCo)]]
- [[k8sgpt]] — K8sGPT
- [[trickster]] — Trickster
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- bootc
- [[entities/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[entities/tetragon.md|Tetragon]] — Cross-reference


<!-- risk-assessed -->
