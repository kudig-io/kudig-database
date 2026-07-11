---
title: Container Runtime (entities)
description: Container Runtime — Kubernetes 生产运维知识库
summary: Container Runtime — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- container
- runtime
- containerd
- cri-o
- cri
- kubelet
- docker
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Container Runtime 是什么
- 如何 Container Runtime
trigger_keywords:
- Container
- Runtime
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Container Runtime

## 概述

Container Runtime 是运行容器的底层软件组件，负责镜像管理、容器生命周期管理和资源隔离。在 Kubernetes 生态中，容器运行时通过 CRI（Container Runtime Interface）与 kubelet 交互。主流容器运行时包括 containerd（CNCF 毕业）、CRI-O 和 cri-dockerd。容器运行时是 Kubernetes 节点组件的核心，决定了 Pod 的启动速度、隔离性和资源效率。

## 核心特性

- **CRI 标准接口**: gRPC API（RuntimeService + ImageService）与 kubelet 解耦
- **OCI 运行时**: 底层调用 runc/crun/kata 等 OCI Runtime 创建容器
- **镜像管理**: Pull/Push/List/Delete OCI 兼容镜像
- **Snapshot 管理**: OverlayFS、btrfs 等联合文件系统管理镜像层
- **RuntimeClass**: 允许 Pod 选择不同的容器运行时
- **多沙箱**: 支持 gVisor、Kata Containers、Wasm 等隔离级别

## 架构

容器运行时架构分为高层和低层。高层运行时（containerd、CRI-O）实现 CRI 接口，管理 Pod Sandbox 和容器生命周期。kubelet 通过 CRI gRPC 调用高层运行时的 `RunPodSandbox`、`CreateContainer`、`StartContainer` 方法。高层运行时负责镜像拉取、存储管理和网络配置。低层运行时（runc、crun、kata-runtime）通过 OCI Runtime Specification 实际创建和运行容器进程。containerd 通过 containerd-shim 管理每个容器进程。

## CRI 接口

CRI（Container Runtime Interface）是 kubelet 与容器运行时之间的 gRPC API。RuntimeService 管理容器生命周期（PodSandbox 和 Container 的创建/启动/停止/删除）。ImageService 管理镜像操作（PullImage/ListImages/RemoveImage）。CRI 的标准化使 Kubernetes 可以在不修改核心代码的情况下支持不同的容器运行时。

## 运行时选择对比

| 运行时 | 优势 | 劣势 | 适用场景 |
|--------|------|------|----------|
| **containerd** | 轻量、高性能、CNCF 毕业 | 调试需 nerdctl | 通用生产 |
| **CRI-O** | K8s 专用、最小依赖 | 功能较少 | 纯 K8s 环境 |
| **cri-dockerd** | Docker 兼容 | 重、已弃用 | 兼容旧系统 |

## Kubernetes 集成

Kubernetes v1.24 移除了内置的 dockershim，CRI 成为唯一的容器运行时接口。节点上 kubelet 通过 Unix Socket 连接容器运行时（containerd: /run/containerd/containerd.sock）。RuntimeClass CRD 允许为不同 Pod 选择不同的运行时——标准容器使用 `runc`/`crun`，安全敏感工作负载使用 `gVisor`/`kata`，Wasm 工作负载使用 `wasmtime`。containerd 通过配置文件（config.toml）管理运行时、镜像仓库和插件。

## 生产使用场景

1. **通用容器**: 使用 containerd + runc 运行标准工作负载
2. **安全隔离**: 使用 Kata Containers 运行不可信工作负载
3. **多租户**: 使用 gVisor 提供更强的进程隔离
4. **WASM 应用**: 使用 wasmtime-spin 运行 WebAssembly 模块

## 安装

```bash
# containerd 安装（通常由 kubeadm/k0s 自动安装）
apt install containerd
# 配置 SystemdCgroup
containerd config default | tee /etc/containerd/config.toml
sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
# RuntimeClass
kubectl apply -f - <<EOF
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata: { name: kata }
handler: kata
EOF
```

## 相关链接

- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[kubernetes]] — Kubernetes
- [[docker]] — Docker
- [[实体/kubelet.md|kubelet]]

## Related

- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[docker]] — Docker
- [[实体/kubelet.md|kubelet]]
- [[pod-lifecycle|Pod Lifecycle]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]

- 21-container-runtime-deep-dive
- 15-container-runtime-interfaces
- [[故障诊断/高级排障/02-node-components/03-container-runtime-troubleshooting.md|03-container-runtime-troubleshooting]]

<!-- risk-assessed -->
