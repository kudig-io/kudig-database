---
title: containerd 多租户
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- runtime
- 08-containerd-multi-tenant
- prometheus
- grafana
- containerd
- networkpolicy
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
- containerd 多租户 是什么
- 如何 containerd 多租户
trigger_keywords:
- containerd
- 多租户
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd 多租户

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

Containerd 多租户实践是关于在共享的 containerd 运行时上安全地运行多个租户工作负载的方法论。通过 Namespace 隔离、运行时类（RuntimeClass）、命名空间配额、镜像隔离和节点级安全策略，实现多租户环境下 containerd 的安全运维。该实践涵盖容器运行时隔离（runc vs. Kata Containers）、镜像仓库隔离、CRI 代理（如 CRI-O Shim）以及容器运行时安全加固等关键技术。

## Key Features（核心能力）

- **Containerd Namespace 隔离**：通过 containerd namespace 机制实现镜像和容器元数据隔离
- **RuntimeClass 支持**：通过 RuntimeClass 为不同租户分配不同的容器运行时（runc/Kata/gVisor）
- **镜像仓库策略**：通过镜像签名验证和准入策略限制可拉取的镜像来源
- **资源限制**：通过 CRI 和 cgroups 实现容器级别的 CPU/内存/IO 限制
- **审计日志**：记录容器创建、启动、销毁等操作，支持租户级行为审计
- **安全上下文**：强制非 root 用户运行、只读根文件系统等安全策略

## 架构与工作原理

多租户隔离通过多层机制实现：Kubernetes Namespace 提供逻辑隔离；RuntimeClass 通过 kubelet 为不同租户的 Pod 分配不同的底层运行时（如安全敏感租户使用 Kata Containers）；containerd 的 namespace 机制隔离镜像和容器元数据；CRI proxy 可在 kubelet 和 containerd 之间增加一层策略执行。节点上通过 AppArmor/SELinux/seccomp profile 进一步限制容器行为。

## K8s 集成

在 K8s 中，多租户隔离通过 Pod Security Admission、NetworkPolicy、RBAC、ResourceQuota、LimitRange 等机制实现。containerd 层面的多租户实践补充了这些 API 级别控制，通过 RuntimeClass 指定低层运行时，通过节点级安全策略限制容器行为。CRI 镜像策略可限制镜像仓库来源，防止租户运行不受信任的镜像。

## 生产用例

- **共享集群多租户**：多个团队共享同一 K8s 集群但需要工作负载隔离
- **SaaS 平台**：为客户提供隔离的容器运行环境
- **安全合规环境**：通过 Kata Containers 或 gVisor 提供硬件级隔离
- **开发测试平台**：为不同项目提供隔离但共享基础设施的容器环境

## 安装与快速开始

```bash
# 查看 containerd namespaces
ctr namespace list

# 创建隔离 namespace
ctr namespace create tenant-a

# 配置 RuntimeClass for Kata
kubectl apply -f - <<EOF
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-containers
handler: kata
EOF
```

## 对比替代方案

相比虚拟机级别的多租户隔离，containerd 多租户实践更轻量但隔离性较弱。结合 Kata Containers 可获得接近 VM 级别的隔离强度。

## Related

- [[k0s]] — K0s
- [[kubeedge]] — KubeEdge
- [[telepresence]] — Telepresence
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 08-containerd-multi-tenant


<!-- risk-assessed -->
