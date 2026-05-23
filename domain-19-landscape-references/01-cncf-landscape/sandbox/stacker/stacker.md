---
title: Stacker (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- image
- stacker
- containerd
- docker
- crd
- operator
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Stacker 是什么
- 如何 Stacker
trigger_keywords:
- Stacker
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Stacker

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

Stacker 是一个无需特权即可构建 OCI 容器镜像的工具。它使用声明式的 YAML 文件（stacker.yaml）定义镜像层，通过 overlay 文件系统构建镜像，无需 Docker daemon 或 root 权限。Stacker 支持可复现构建、内容寻址层缓存和多阶段构建，特别适合 CI/CD 流水线中的安全镜像构建。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **非特权环境**: 在 CI/CD 中使用非 root 用户运行 stacker 构建
- **层缓存**: 利用 stacker 的层缓存加速 CI/CD 流水线中的重复构建
- **多阶段**: 使用多阶段构建减小最终镜像体积
- **锁定版本**: 在 from 中使用摘要而非标签锁定基础镜像版本
- **签名**: 构建后对镜像签名，确保供应链安全

## 架构定位

在 CNCF 生态中，stacker 属于 **Image** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/ci-cd-pipeline-patterns|ci-cd-pipeline-patterns]]

## Related

- [[ratify]] — Ratify
- [[container2wasm]] — container2wasm
- [[entities/06-containerd-observability|observability]]]] — [[containerd|containerd]]rd 可观测性|containerd 可观测性]]
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- stacker
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
