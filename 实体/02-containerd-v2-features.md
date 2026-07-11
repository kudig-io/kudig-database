---
title: containerd 2.0 新特性 (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- runtime
- 02-containerd-v2-features
- kubelet
- prometheus
- grafana
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
- containerd 2.0 新特性 是什么
- 如何 containerd 2.0 新特性
trigger_keywords:
- containerd
- '2.0'
- 新特性
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd 2.0 新特性

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

Containerd 2.0 是 containerd 运行时的重大版本更新，于 2024 年底发布。它带来了全新的 Sandboxed API、增强的 Transfer Service、改进的 NRI（Node Resource Interface）、更高效的镜像分发和全面的 K8s 1.29+ 支持。Containerd 2.0 移除了大量已废弃的 API 和功能，精简了代码库，提升了性能和安全性。作为 K8s 最广泛使用的容器运行时，containerd 2.0 是云原生基础设施的重要里程碑。

## Key Features（核心能力）

- **Sandboxed API**：新的沙箱 API 替代旧版 CRI PodSandbox，支持更灵活的沙箱管理
- **Transfer Service**：内置镜像传输服务，支持 Registry 间直接镜像同步
- **NRI 增强**：Node Resource Interface 2.0，支持更丰富的容器运行时扩展
- **镜像分发优化**：支持 ORAS 镜像、Lazy Pulling（Stargz/SOCI）
- **安全提升**：默认启用 Seccomp Profile、移弃用 API 清理
- **性能改进**：更快的容器启动、更低的内存占用

## 架构与工作原理

Containerd 2.0 架构保持了核心的 containerd-shim 模型，但对沙箱管理进行了重构。新的 Sandboxed API 将沙箱（Sandbox）作为一等公民，支持在沙箱层面进行资源隔离和生命周期管理。Transfer Service 作为独立子系统，支持镜像的 Pull/Push/Mount 操作，可通过插件扩展。NRI 2.0 允许第三方插件在容器创建和运行时注入设备、环境变量等配置。runc v2 成为默认 shim。

## K8s 集成

Containerd 2.0 通过 CRI v1 与 kubelet 集成，完全兼容 K8s 1.29+。新的 Sandboxed API 为未来 K8s 的 Pod 级别隔离增强奠定基础。Transfer Service 可用于大规模集群的镜像预分发。NRI 增强使 K8s 节点上的资源管理更灵活（如 GPU 设备注入）。containerd 2.0 移弃的 API 需要 K8s 1.24+ 环境。

## 生产用例

- **K8s 生产运行时升级**：从 containerd 1.7 升级到 2.0 获取性能和安全改进
- **边缘部署**：更低的资源占用适合资源受限的边缘节点
- **安全加固集群**：利用默认 seccomp 和增强的安全特性
- **镜像加速**：利用 Lazy Pulling 和 Transfer Service 加速大规模镜像分发

## 安装与快速开始

```bash
# 升级 containerd 到 2.0
wget https://github.com/containerd/containerd/releases/download/v2.0.0/containerd-2.0.0-linux-amd64.tar.gz
tar -xzf containerd-2.0.0-linux-amd64.tar.gz -C /usr/local
systemctl restart containerd
```

## 对比替代方案

相比 containerd 1.7，2.0 更精简、更安全、性能更好，但需要 K8s 1.29+。相比 CRI-O，containerd 2.0 功能更丰富且社区更活跃。

## Related

- [[k3s]] — k3s 轻量级 Kubernetes
- [[实体/virtual-kubelet.md|kubelet]]]] — Virtual Kubelet
- [[kudo]] — KUDO
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 02-containerd-v2-features


<!-- risk-assessed -->
