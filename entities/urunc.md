---
title: urunc
description: '## 概述'
summary: 'urunc 是一个符合 OCI 标准的容器运行时，专门用于在 Kubernetes 中运行 Unikernel 应用。Unikernel 是将应用与最小化操作系统库编译为单一镜像的技术，具有极小的攻击面、亚毫秒级启动时间和极低的内存占用。urunc 将 Unikernel 打包为 OCI 镜像，'
category: entities
tags:
- k8s
- cncf
- runtime
- urunc
- containerd
- cri-o
- gateway
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- urunc 是什么
- 如何 urunc
trigger_keywords:
- urunc
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# urunc

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

urunc 是一个符合 OCI 标准的容器运行时，专门用于在 Kubernetes 中运行 Unikernel 应用。Unikernel 是将应用与最小化操作系统库编译为单一镜像的技术，具有极小的攻击面、亚毫秒级启动时间和极低的内存占用。urunc 将 Unikernel 打包为 OCI 镜像，使其能够通过标准的容器工作流（containerd、CRI-O）在 Kubernetes 上部署和管理。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **应用选择**: Unikernel 最适合单一功能的微服务，不适合复杂的多进程应用
- **VMM 选择**: Firecracker 适合高密度部署，QEMU 适合功能丰富的场景
- **镜像构建**: 使用 Unikraft 的 kraft 工具简化 Unikernel 的构建流程
- **资源配置**: Unikernel 内存需求远低于容器，合理设置 limits 节省资源
- **混合部署**: 在同一集群中通过 RuntimeClass 混合部署容器和 Unikernel

## 架构定位

在 CNCF 生态中，urunc 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[containerd]]
- [[concepts/container-runtime-comparison.md|container-runtime-comparison]]
- [[pod-lifecycle]]

## Related

- [[dex]] — Dex
- [[kgateway]] — kgateway
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd
- [[cri-o]] — CRI-O

- urunc
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.1
- RELEASE-NOTES-0.0
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- [[entities/flatcar.md|[[Flatcar Container Linux|Flatcar Container Linux]]ux 生产环境速查卡|Linux]]]]
- [[entities/composefs.md|composefs]]
- [[entities/04-containerd-upgrade-migration.md|containerd 升级迁移]]
- [[entities/wasmedge.md|WasmEdge]]
- [[entities/spinkube.md|SpinKube]]
- [[entities/05-containerd-windows-support.md|containerd Windows 支持]]
- [[entities/02-containerd-v2-features.md|containerd 2.0 新特性]]
- [[entities/08-containerd-multi-tenant.md|containerd 多租户]]
- [[entities/k0s.md|K0s]]
- [[entities/03-containerd-security-hardening.md|containerd 安全加固]]
- [[entities/bootc.md|bootc]]
- [[entities/container2wasm.md|container2wasm]]
- [[entities/kubean.md|Kubean]]
- [[entities/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/topic-index/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
