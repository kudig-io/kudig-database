---
title: Tinkerbell [entities]
description: '## 概述'
summary: 'Tinkerbell 是一个裸金属服务器自动化配置（provisioning）框架，用于在物理服务器上自动安装操作系统和执行配置任务。它替代传统的 PXE/Cobbler 方案，通过声明式的工作流定义和容器化的操作步骤实现裸金属服务器的云原生式管理。'
category: entities
tags:
- k8s
- cncf
- metal
- tinkerbell
- prometheus
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tinkerbell 是什么
- 如何 Tinkerbell
trigger_keywords:
- Tinkerbell
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Tinkerbell

> **CNCF 状态**: Sandbox | **类别**: Metal | **主要语言**: Go

## 概述

Tinkerbell 是一个 CNCF 沙箱项目，由 Equinix Metal（原 Packet）开源，是一个专为裸机服务器设计的自动化配置和部署平台。它在 Kubernetes 上运行，提供从服务器发现、操作系统安装到工作负载部署的全流程自动化。Tinkerbell 特别适合边缘计算、私有数据中心和裸机 Kubernetes 集群的场景，解决了物理服务器从零到可用状态的自动化问题。

## Key Features（核心能力）

- **裸机自动化**：通过 iPXE 网络启动实现物理服务器的全自动 OS 安装
- **Workflow 引擎**：基于 Action 的工作流定义，支持自定义部署步骤
- **Hardware 管理**：通过 Hardware CRD 管理物理服务器元数据和状态
- **Template 系统**：可复用的部署模板，支持多种 OS 镜像
- **DHCP/PXE 服务**：内置 DHCP 和 TFTP 服务，支持网络启动
- **K8s 原生**：所有资源以 CRD 形式管理

## 架构与工作原理

Tinkerbell 架构包含多个微服务组件：Boots 提供 DHCP 和 iPXE 引导服务；Hegel 提供基于硬件元数据的 metadata 服务（类似云厂商的 metadata API）；Tink Server 是核心控制器，管理 Workflow 和 Template；Tink Worker 运行在目标裸机上的临时容器中，执行 Workflow 中的 Action。所有组件作为 K8s Pod 运行，通过 CRD 管理硬件和工作流。

## K8s 集成

Tinkerbell 本身运行在 Kubernetes 上，所有组件以 Deployment/DaemonSet 形式部署。Hardware、Template、Workflow 通过 CRD 定义。Tink Controller 监听 Workflow CRD 并协调执行。在裸机 K8s 集群场景中，Tinkerbell 负责节点的初始 OS 安装和 K8s 组件部署，可与 Cluster API 的 CAPMVM provider 集成实现裸机节点的自动扩缩容。

## 生产用例

- **裸机 K8s 集群**：自动化部署物理服务器上的 Kubernetes 节点
- **边缘计算**：远程批量配置边缘数据中心的物理服务器
- **私有云建设**：替代传统裸机管理工具（如 Foreman/MaaS）
- **OS 批量部署**：大规模数据中心的操作系统自动化安装

## 安装与快速开始

```bash
helm repo add tinkerbell https://tinkerbell.github.io/helm
helm install tinkerbell tinkerbell/tinkerbell-stack -n tinkerbell --create-namespace
```

## 对比替代方案

相比 MAAS（Canonical 的裸机管理工具），Tinkerbell 是 K8s 原生的且更轻量。相比 Foreman，Tinkerbell 专注于自动化工作流而非完整的 IT 资产管理。

## Related

- [[headlamp]] — Headlamp
- [[实体/cncf-orchestration.md|cncf-orchestration]] — CNCF 编排与应用管理项目全景
- [[prometheus]] — Prometheus
- [[interlink]] — InterLink
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tinkerbell
- [[实体/cncf-edge-ai.md|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
