---
title: InterLink (entities)
description: '## 概述'
summary: 'InterLink 是一个 Virtual Kubelet 提供者实现，允许将 Kubernetes Pod 调度到远程 HPC（高性能计算）和云计算基础设施上执行。'
category: entities
tags:
- k8s
- cncf
- edge
- interlink
- kubelet
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
- InterLink 是什么
- 如何 InterLink
trigger_keywords:
- InterLink
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# InterLink

> **CNCF 状态**: Sandbox | **类别**: Edge | **主要语言**: Go

## 概述

InterLink 是一个 Virtual Kubelet 提供者实现，由 CS Italy（意大利国家核物理研究院）等科研机构推动，2023 年加入 CNCF 沙箱。它允许将 Kubernetes Pod 调度到远程 HPC（高性能计算）和云计算基础设施上执行。InterLink 通过标准的 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 将传统 HPC 集群（Slurm、HTCondor）和云计算平台作为 Kubernetes 的扩展计算资源，使科研人员和工程师能够使用熟悉的 Kubernetes 工作流提交和管理 HPC 作业。InterLink 会在远程 HPC 节点上将容器转换为 Singularity/Apptainer 格式运行，解决了 HPC 环境不支持 Docker/root 权限的限制。

## 核心能力

- **Virtual Kubelet 集成**: 将远程 HPC/云资源注册为 Kubernetes 虚拟节点
- **多调度器支持**: Slurm、HTCondor、OpenStack、AWS Batch 等后端
- **容器转换**: 自动将 OCI 容器镜像转换为 Singularity/Apptainer 格式（HPC 兼容）
- **Pod 生命周期管理**: 将 Kubernetes Pod 状态映射到 HPC 作业状态
- **数据管理**: 支持将 PVC 挂载到 HPC 共享文件系统
- **资源映射**: 虚拟节点容量反映 HPC 集群实际可用资源

## 架构

InterLink 架构分为云端控制面和边缘执行面两层：

- **InterLink Controller**: 部署在 Kubernetes 控制面集群，实现 Virtual Kubelet Provider 接口
- **Virtual Node**: 在 Kubernetes 中注册的虚拟节点，代表远程 HPC 集群
- **InterLinker (Tunnel)**: 建立云到 HPC 集群的安全隧道连接
- **Sidecar (HPC 侧)**: 部署在 HPC 登录节点，接收 Pod 定义并转换为 Slurm 脚本
- **Singularity Runtime**: 在 HPC 计算节点上以 Singularity/Apptainer 方式运行容器

执行流程：`Kubernetes Pod → InterLink (Virtual Kubelet) → Slurm 脚本 → HPC 计算节点 → Singularity 容器`

## K8s 集成

InterLink 通过 Virtual Kubelet 框架与 Kubernetes 深度集成。它在集群中注册一个或多个虚拟节点，节点的 capacity 反映远程 HPC 集群的可用资源。当 Pod 被调度到虚拟节点时，InterLink Controller 将 Pod spec 转换为 HPC 作业脚本（如 Slurm SBATCH），通过 SSH/API 提交到 HPC 调度器。Pod 状态（Pending → Running → Succeeded/Failed）映射到 HPC 作业状态。支持通过 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 nodeSelector、tolerations 和 affinity 控制哪些 Pod 被调度到 HPC。

## 生产场景

1. **HPC 作业管理**: 科研人员通过 kubectl 提交 Slurm 作业，无需学习 Slurm 语法
2. **混合计算池**: 将本地 Kubernetes 集群与远程超算中心组成统一计算池
3. **大规模 AI 训练**: 将 GPU 密集型训练任务调度到 HPC 集群的 GPU 节点
4. **弹性计算扩展**: Kubernetes 集群资源不足时，自动将作业溢出到 HPC/云

## 安装

```bash
# 安装 Virtual Kubelet 和 InterLink
helm repo add interlink https://intertwin-eu.github.io/interLink/
helm install interlink interlink/interlink -n interlink-system --create-namespace

# 配置 HPC 后端连接
kubectl create secret generic hpc-credentials \
  --from-file=ssh-key=$HOME/.ssh/id_rsa -n interlink-system

# 部署虚拟节点（指向 HPC 集群）
kubectl apply -f - <<EOF
apiVersion: v1
kind: Node
metadata:
  name: hpc-cluster
  labels:
    kubernetes.io/hostname: hpc-cluster
spec:
  taints:
  - key: hpc
    value: "true"
    effect: NoSchedule
EOF
```

## 对比

| 特性 | InterLink | Virtual Kubelet (ACI) | Kueue |
|------|-----------|-----------------------|-------|
| HPC 支持 | ✅ Slurm/HTCondor | ❌ 仅 ACI | ⚠️ 有限 |
| 容器转换 | ✅ Singularity | ❌ | ❌ |
| 批处理调度 | ✅ | ❌ | ✅ |
| 适用场景 | 科研 HPC | Azure 云 | K8s 内调度 |

## 架构定位

在 CNCF 生态中，InterLink 属于 **Edge** 类别，为云原生应用提供 HPC 计算资源集成能力。

## 参考链接

- [[deployment]]
- [[pod-lifecycle]]
- [[实体/kubelet.md|kubelet]]

## Related

- [[实体/cncf-orchestration.md|cncf-orchestration]] — CNCF 编排与应用管理项目全景
- [[prometheus]] — Prometheus
- [[实体/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[virtual-kubelet]] — Virtual Kubelet

- interlink
- [[实体/akri.md|Akri]]
- [[实体/openyurt.md|OpenYurt]]
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
