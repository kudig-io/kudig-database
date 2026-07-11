---
title: Lima (entities)
description: '## 概述'
summary: 'Lima（Linux virtual Machine）是 macOS 和 Linux 上的轻量级 Linux VM 管理工具。'
category: entities
tags:
- k8s
- cncf
- runtime
- lima
- containerd
- docker
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
- Lima 是什么
- 如何 Lima
trigger_keywords:
- Lima
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Lima

> **CNCF 状态**: Incubating | **类别**: Runtime | **主要语言**: Go

## 概述

Lima（Linux virtual Machine）是 macOS 和 Linux 上的轻量级 Linux 虚拟机管理工具，由 Rancher/NavILT 等社区推动，2023 年加入 CNCF 孵化。它类似于 Windows 上的 WSL2，提供自动文件共享、端口转发和 containerd 集成，是 Docker Desktop 的开源替代方案。Lima 底层使用 Apple Hypervisor（macOS）或 QEMU（Linux），在 macOS 上运行 Linux 虚拟机，自动配置主机-VM 之间的文件共享（9p/virtiofs）和端口转发。它内置 containerd 和 nerdctl，可以直接运行容器而无需 Docker daemon。Lima 还是 Rancher Desktop、Colima、Finch 等容器开发工具的底层引擎。

## 核心能力

- **自动文件共享**: 主机目录（$HOME）自动挂载到 VM，支持 9p/virtiofs/sshfs
- **自动端口转发**: VM 端口自动映射到主机，无需手动配置
- **containerd 集成**: 内置 containerd 和 nerdctl，可直接运行容器
- **多架构支持**: AMD64 和 ARM64（Apple Silicon 原生支持）
- **多发行版**: Ubuntu、Debian、Fedora、Alpine、Arch Linux 等
- **模板系统**: 预配置 YAML 模板快速启动（docker、k3s、k8s、podman 等）

## 架构

Lima 采用简洁的 VM 管理架构：

- **limactl**: CLI 工具，管理 VM 的创建、启动、停止和删除
- **QEMU/Hypervisor**: 底层虚拟化引擎（macOS 使用 Apple Hypervisor.framework）
- **lima.yaml**: VM 配置文件，定义 CPU、内存、磁盘、挂载、端口转发等
- **guestagent**: VM 内运行的代理，负责端口转发和文件共享协调
- **nerdctl/containerd**: VM 内置的容器运行时（可选 Docker 兼容模式）
- **cloud-init**: VM 首次启动时执行初始化配置

工作流：`limactl start → 创建 VM → cloud-init → containerd ready → nerdctl run`

## K8s 集成

Lima 通过模板系统提供 Kubernetes 集成。`limactl start --name=k8s template://k8s` 启动一个预装 kubeadm 的 VM，自动初始化单节点 Kubernetes 集群。`limactl start template://k3s` 则提供更轻量的 k3s 集群。端口转发自动将 Kubernetes API Server（6443）映射到主机，lima 提供的 kubeconfig 可直接使用 kubectl 连接。Lima VM 中可以部署容器运行时与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 兼容，支持标准 Kubernetes 工作负载。

## 生产场景

1. **本地容器开发**: macOS 开发者使用 Lima 替代 Docker Desktop 运行容器
2. **本地 Kubernetes 集群**: 使用 Lima 模板快速启动 k3s/k8s 单节点集群进行开发测试
3. **CI/CD 构建环境**: 在 macOS CI runner 上使用 Lima 构建 Linux 容器镜像
4. **多架构构建**: 在 Apple Silicon 上使用 Lima 运行 AMD64 VM 进行跨架构构建

## 安装

```bash
# 安装 Lima
brew install lima
# 或
curl -L https://github.com/lima-vm/lima/releases/latest/download/lima-$(uname -s)-$(uname -m).tar.gz | tar xz -C /usr/local

# 启动默认 VM（内置 containerd + nerdctl）
limactl start

# 运行容器
lima nerdctl run -d --name web -p 8080:80 nginx:alpine

# 启动 k3s 集群
limactl start --name=k3s template://k3s
export KUBECONFIG=$(limactl show k3s --format '{{.Dir}}/copied-from-guest/kubeconfig.yaml')
kubectl get nodes

# 启动 Docker 兼容模式
limactl start template://docker
docker context use lima-default
```

## 对比

| 特性 | Lima | Docker Desktop | Colima | Rancher Desktop |
|------|------|---------------|--------|-----------------|
| 开源 | ✅ | ❌ | ✅ | ✅ |
| 底层引擎 | QEMU/Hypervisor | Hypervisor | Lima | Lima |
| 多架构 | ✅ | ⚠️ | ✅ | ✅ |
| CNCF 状态 | Incubating | 非 CNCF | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，Lima 属于 **Runtime** 类别，为云原生开发提供轻量级 Linux VM 管理能力。

## 参考链接

- [[containerd]]
- [[pod-lifecycle]]

## Related

- [[tikv]] — TiKV
- [[k8gb]] — K8GB
- [[docker]] — Docker
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- lima
- [[实体/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
