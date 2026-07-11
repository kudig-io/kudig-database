---
title: KubeVirt [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- runtime
- kubevirt
- containerd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeVirt 是什么
- 如何 KubeVirt
trigger_keywords:
- KubeVirt
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeVirt

> **CNCF 状态**: Incubating | **类别**: Runtime | **主要语言**: Go

## 概述

KubeVirt 是一个 CNCF 孵化项目，由 Red Hat 主导开发，旨在将传统虚拟机（VM）工作负载引入 Kubernetes 平台。它允许开发团队在 K8s 集群中以管理容器相同的方式管理虚拟机，使得那些难以容器化的遗留应用（如 Windows 工作负载、需要直接硬件访问的应用）也能享受云原生的编排能力。KubeVirt 通过自定义资源定义（CRD）将 VM 抽象为 Kubernetes 原生对象，无需维护独立的虚拟化管理平台。项目于 2017 年开源，目前已被 Red Hat OpenShift Virtualization、SUSE Rancher 等商业产品采用。

## Key Features（核心能力）

- **VM 即 K8s 资源**：通过 VirtualMachine 和 VirtualMachineInstance CRD 将虚拟机生命周期完全纳入 K8s API 管理
- **容器与 VM 混合编排**：支持在同一个 Pod 中运行 VM 和 sidecar 容器，实现边车模式注入
- **云原生存储与网络**：复用 CSI 存储驱动和 CNI 网络插件，PVC 可直接挂载为 VM 磁盘
- **Live Migration**：支持虚拟机热迁移，在节点维护期间实现零停机工作负载转移
- **GPU/设备直通**：通过 Kubernetes Device Plugin 支持 GPU、SR-IOV 等硬件直通
- **标准化 API**：兼容 libvirt 和 QEMU/KVM，提供稳定的虚拟化抽象层

## 架构与工作原理

KubeVirt 架构由多个组件构成：virt-api 负责 VM 相关 API 的认证和准入控制；virt-controller 管理 VM 对应 Pod 的生命周期；virt-handler 以 DaemonSet 形式运行在每个节点，负责与本地 libvirt 通信；virt-launcher 为每个 VMI 创建专属 Pod，在其中运行 libvirtd 实例管理 QEMU/KVM 虚拟机。CDI（Containerized Data Importer）组件负责从 HTTP、S3、Registry 等数据源导入磁盘镜像到 PVC。所有组件通过 Kubernetes 控制器模式协调状态。

## K8s 集成

KubeVirt 深度集成 Kubernetes 生态系统：VM 以 CRD 形式存在，可通过 kubectl get vmi 管理；使用 K8s 调度器进行节点分配；复用 PVC/StorageClass 提供持久化存储；通过 NetworkPolicy 和 CNI 插件实现网络安全；支持 HPA 和 VPA 进行资源自动伸缩。CDI 组件也是以 Operator 模式部署的 K8s 原生控制器。

## 生产用例

- **遗留应用迁移**：将需要完整 OS 环境的传统应用（如老旧数据库、Windows 应用）迁移到 K8s 平台
- **混合工作负载平台**：在同一集群中同时运行容器化微服务和 VM 工作负载，统一管理
- **开发测试环境**：为需要完整 VM 环境的开发团队提供自助式 K8s 原生虚拟机服务
- **安全隔离场景**：多租户环境中利用 VM 级别的强隔离，满足合规要求

## 安装与快速开始

```bash
kubectl apply -f https://github.com/kubevirt/kubevirt/releases/download/v1.2.0/kubevirt-operator.yaml
kubectl apply -f https://github.com/kubevirt/kubevirt/releases/download/v1.2.0/kubevirt-cr.yaml
# 等待部署完成
kubectl wait --for=condition=Available kv/kubevirt -n kubevirt --timeout=300s
```

## 对比替代方案

相比传统虚拟化管理平台（如 OpenStack、oVirt），KubeVirt 不需要独立的控制平面，完全复用 K8s 基础设施。相比 Kata Containers（轻量级 VM 替代容器运行时），KubeVirt 提供完整的 VM 体验而非容器替代方案。

## Related

- [[carvel]] — Carvel
- [[holmesgpt]] — HolmesGPT
- [[ko]] — ko
- [[openfunction]] — OpenFunction
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubevirt
- index/etcd-index|etcd 知识图谱索引]]


<!-- risk-assessed -->
