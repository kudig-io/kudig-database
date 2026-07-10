---
title: composefs (entities)
description: '## 概述'
summary: 'composefs 是一个 Linux 文件系统，设计用于高效挂载和共享容器镜像层。它结合了 EROFS（只读文件系统）作为元数据存储和 fs-verity 提供内容校验，实现了容器镜像的可验证挂载。composefs 允许多个容器镜像共享相同内容的文件块（基于内容寻址的对象存储），大幅减少磁盘空间占用，同时通过 fs-verity 确保镜像内容的完整性。'
category: entities
tags:
- k8s
- cncf
- runtime
- composefs
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
- composefs 是什么
- 如何 composefs
trigger_keywords:
- composefs
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# composefs

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: C

## 概述

composefs 是一个 Linux 文件系统，设计用于高效挂载和共享容器镜像层。它结合了 EROFS（只读文件系统）作为元数据存储和 fs-verity 提供内容校验，实现了容器镜像的可验证挂载。composefs 允许多个容器镜像共享相同内容的文件块（基于内容寻址的对象存储），大幅减少磁盘空间占用，同时通过 fs-verity 确保镜像内容的完整性。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **启用 fs-verity**: 生产环境启用 fs-verity 确保镜像文件完整性
- **对象存储规划**: 合理规划对象存储目录的容量和文件系统
- **与 podman 配合**: 在容器主机上启用 composefs 显著减少磁盘占用
- **内核版本**: 确保内核支持 EROFS 和 fs-verity (5.4+)

## 架构定位

在 CNCF 生态中，composefs 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[概念/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[containerssh]] — ContainerSSH
- [[modelpack]] — ModelPack
- [[oauth2-proxy]] — OAuth2 Proxy
- [[schemahero]] — SchemaHero
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- composefs
- [[实体/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference


<!-- risk-assessed -->
