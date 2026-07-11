---
title: KubeClipper [entities]
description: '## 概述'
summary: 'KubeClipper 是一个轻量级的 Kubernetes 集群全生命周期管理平台，提供 Web UI 和 CLI 工具，支持在物理机、虚拟机和云主机上快速部署和管理 Kubernetes 集群。'
category: entities
tags:
- k8s
- cncf
- platform
- kubeclipper
- etcd
- prometheus
- grafana
- cilium
- containerd
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeClipper 是什么
- 如何 KubeClipper
trigger_keywords:
- KubeClipper
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeClipper

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

KubeClipper 是一个轻量级的 Kubernetes 集群全生命周期管理平台，由九州云（99Cloud）开源开发，2022 年加入 CNCF 沙箱。它提供 Web UI 和 CLI 工具，支持在物理机、虚拟机和云主机上快速部署和管理 Kubernetes 集群。KubeClipper 采用 Agent 架构，无需依赖 Ansible 或 SSH 密钥分发，通过自研的 kc-agent 管理节点。它支持离线部署（air-gapped）、集群扩缩容、版本升级、备份恢复、组件管理（CNI、CSI、监控）等完整的集群运维能力。KubeClipper 特别适合私有云、国产化和离线场景下的 Kubernetes 集群管理。

## 核心能力

- **全生命周期管理**: 创建、扩缩容、升级、备份恢复、卸载 Kubernetes 集群
- **离线部署**: 预打包离线镜像，支持完全断网环境下的集群部署
- **多组件管理**: 集成 Cilium、Calico、Containerd、Harbor、Prometheus 等常用组件
- **Web UI + CLI**: 图形化界面降低运维门槛，同时提供 kcctl CLI 工具
- **Agent 架构**: 无需 Ansible/SSH，kc-Agent 通过 gRPC 与控制中心通信
- **多架构支持**: 支持 x86_64 和 ARM64 架构

## 架构

KubeClipper 采用中心化控制 + Agent 模式：

- **kc-server**: 控制中心，提供 API Server、Web UI 和调度逻辑，数据存储在内置 etcd
- **kc-agent**: 部署在每个被管理节点上的守护进程，执行具体的安装和运维操作
- **gRPC 通信**: kc-server 与 kc-agent 之间通过 gRPC 双向通信，无需 SSH
- **Cluster CRD**: 以 Kubernetes CRD 方式声明集群期望状态
- **Step Pipeline**: 运维操作被分解为有序的 Step（安装依赖、配置 etcd、部署控制面等）
- **离线包管理**: 统一的离线包仓库，支持按需下载和缓存

管理流程：`用户 (UI/CLI) → kc-server → CRD 期望状态 → kc-agent (执行) → 节点配置`

## K8s 集成

KubeClipper 的管理面本身就是一个 Kubernetes API 服务器（内置 etcd），通过 CRD（`Cluster`、`Node`）声明式管理目标集群。kc-agent 部署在被管理节点上，接收并执行安装、升级等操作指令。每个运维操作被分解为原子 Step（如安装 containerd、初始化 etcd、配置 CNI），按 Pipeline 有序执行。支持与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中各种 CNI（Cilium、Calico）、CSI 和 Ingress Controller 集成。

## 生产场景

1. **私有云 K8s 部署**: 在裸金属服务器上批量部署和管理多个生产级 Kubernetes 集群
2. **国产化信创环境**: 支持麒麟 OS、鲲鹏 ARM 等国产化基础设施
3. **离线环境部署**: 在完全断网的机房环境中部署 Kubernetes 集群
4. **多集群运维**: 统一管理开发、测试、生产多套 Kubernetes 集群

## 安装

```bash
# 下载 kcctl CLI
curl -sfL https://oss.kubeclipper.io/kcctl-install.sh | KC_VERSION=v1.4.0 bash -

# 初始化 KubeClipper 管理节点
kcctl deploy --user root --passwd $SERVER_PASSWORD --pkg kc.tar.gz \
  --ip 10.0.0.1

# 访问 Web UI
echo "https://$(hostname -I | awk '{print $1}')"

# 创建集群
kcctl create cluster --name prod-cluster --master 10.0.0.2,10.0.0.3,10.0.0.4 \
  --worker 10.0.0.5,10.0.0.6 --cni calico
```

## 对比

| 特性 | KubeClipper | Kubespray | KubeKey | Rancher |
|------|-------------|-----------|---------|---------|
| 离线部署 | ✅ 原生 | ⚠️ 需配置 | ✅ | ⚠️ 有限 |
| Agent 架构 | ✅ 自研 | ❌ Ansible | ❌ Ansible | ✅ |
| Web UI | ✅ | ❌ | ❌ | ✅ |
| CNCF 状态 | Sandbox | 非 CNCF | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，KubeClipper 属于 **Platform** 类别，为云原生应用提供轻量级集群生命周期管理能力。

## 参考链接

- [[etcd]]
- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[cilium]]
- [[containerd]]
- networking.md|cilium-ebpf-networking]]

## Related

- [[kmesh]] — Kmesh
- [[kpt]] — kpt
- [[logging-operator]] — Loggingng Operator|Logging Operator]]
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubeclipper
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
