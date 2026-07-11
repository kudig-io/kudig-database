---
title: containerd Windows 支持
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- runtime
- 05-containerd-windows-support
- containerd
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd Windows 支持 是什么
- 如何 containerd Windows 支持
trigger_keywords:
- containerd
- Windows
- 支持
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd Windows 支持

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

Containerd Windows 支持是 containerd 运行时的核心功能之一，使 containerd 能够在 Windows Server 2019/2022 上运行 Windows 容器。随着 Kubernetes 在 Windows 工作负载场景中的需求增长，containerd 从 v1.6 开始全面支持 Windows 平台，替代了之前的 Docker EE 作为 K8s 的默认容器运行时。Windows 支持包括 Windows 进程隔离容器和 Hyper-V 隔离容器两种模式，支持 .NET 应用、IIS、SQL Server 等 Windows 原生工作负载在 K8s 中运行。

## Key Features（核心能力）

- **Windows 进程隔离**：支持 Windows Server Containers（进程级隔离），轻量高效
- **Hyper-V 隔离**：通过 Hyper-V 虚拟化提供更强的隔离边界，兼容不同内核版本
- **GMSA 支持**：支持 Group Managed Service Accounts 实现 Active Directory 域认证
- **RunHCS 运行时**：基于 Windows Host Compute Service (HCS) 的运行时实现
- **镜像格式兼容**：支持 Docker 镜像格式和 OCI 镜像格式
- **网络支持**：集成 Windows CNI 插件，支持 overlay 网络和 L2bridge 网络

## 架构与工作原理

Containerd 在 Windows 上的架构与 Linux 类似，但使用 runhcs 替代 runc 作为低层运行时。runhcs 通过 Windows HCS (Host Compute Service) API 创建和管理容器。containerd-shim-runhcs-v1 作为 shim 进程，负责容器进程的生命周期管理。镜像层通过 Windows Container Storage (WCStorage) 管理，支持 NTFS 和 SAS 磁盘作为容器存储后端。

## K8s 集成

在 Kubernetes 中，containerd 通过 CRI (Container Runtime Interface) 与 kubelet 交互。Windows 节点需要运行 kubelet、kube-proxy 和 containerd，通过 taint/toleration 机制将 Windows Pod 调度到 Windows 节点。Pod 网络通过 CNI 插件（如 Calico for Windows、Antrea）配置，Service 和 Ingress 支持 Windows 兼容的代理规则。

## 生产用例

- **Windows 遗留应用现代化**：将 ASP.NET、WCF 等 Windows 应用迁移到 K8s 平台
- **混合 Linux/Windows 集群**：在同一集群中运行 Linux 和 Windows 工作负载
- **SQL Server 容器化**：在 K8s 中运行容器化 SQL Server 实例
- **CI/CD 构建节点**：提供 Windows 容器化的构建和测试环境

## 安装与快速开始

```bash
# Windows Server 2022 安装 containerd
curl.exe -L https://github.com/containerd/containerd/releases/download/v1.7.0/containerd-1.7.0-windows-amd64.tar.gz -o containerd.tar.gz
tar -xzf containerd.tar.gz
./containerd.exe --register-service
```

## 对比替代方案

相比 Docker EE，containerd Windows 支持更轻量且原生集成 CRI。相比 Hyper-V VM，Windows 容器启动更快但隔离性稍弱。

## Related

- [[microcks]] — Microcks
- [[keylime]] — Keylime
- [[openebs]] — OpenEBS
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 05-containerd-windows-support


<!-- risk-assessed -->
