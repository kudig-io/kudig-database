---
title: ORAS (OCI Registry As Storage)
description: 'summary: "ORAS (OCI Registry As Storage) 是一个用于将 OCI 工件推送到和拉取自 OCI 兼容仓库的工具和库。它允许使用容器镜像仓库存储任意类型的工件，如
  Helm Chart、WASM 模块、策略文件、签名等，实现 "anything as OCI artifacts" 的理念。"'
summary: 'summary: "ORAS (OCI Registry As Storage) 是一个用于将 OCI 工件推送到和拉取自 OCI 兼容仓库的工具和库。它允许使用容器镜像仓库存储任意类型的工件，如
  Helm Chart、WASM 模块、策略文件、签名等，实现 "anything as OCI artifacts" 的理念。"'
category: general
tags:
- k8s
- helm
- crd
- operator
- wasm
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
- ORAS (OCI Registry As Storage) 是什么
- 如何 ORAS (OCI Registry As Storage)
trigger_keywords:
- ORAS
- OCI
- Registry
- As
- Storage
prerequisites:
- kubectl-basics
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "[[ORAS|ORAS]] (OCI Registry As Storage)"
category: entities
summary: "ORAS (OCI Registry As Storage) 是一个用于将 OCI 工件推送到和拉取自 OCI 兼容仓库的工具和库。它允许使用容器镜像仓库存储任意类型的工件，如 Helm Chart、WASM 模块、策略文件、签名等，实现 "anything as OCI artifacts" 的理念。"
tags: k8s, cncf, image, oras]
sources: ["docs/生态参考/sandbox/oras/oras.md", "生态参考/sandbox/oras/oras.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: reference
base_confidence: 0.7
---

# ORAS (OCI Registry As Storage)

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

ORAS (OCI Registry As Storage) 是一个用于将 OCI 工件推送到和拉取自 OCI 兼容仓库的工具和库。它允许使用容器镜像仓库存储任意类型的工件，如 Helm Chart、WASM 模块、策略文件、签名等，实现 "anything as OCI artifacts" 的理念。

## 核心能力

- **任意工件**: 将任意文件存储为 OCI 工件
- **OCI 兼容**: 支持所有 OCI 兼容仓库
- **CLI 和库**: 提供 CLI 工具和 Go/Python 库
- **Manifest 操作**: 查看和管理 OCI manifest
- **多平台**: 支持 Linux、macOS、Windows
- **引用支持**: OCI Reference Types 关联工件

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Media Type**: 为工件定义明确的 media type
- **标签管理**: 使用语义化版本标签
- **引用关联**: 使用 OCI Reference Types 关联签名、SBOM
- **仓库兼容**: 确认目标仓库支持 OCI 工件

## 架构定位

在 CNCF 生态中，oras 属于 **Image** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[storage-model]]

## Related

- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[生态参考/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
