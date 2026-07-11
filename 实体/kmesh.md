---
title: Kmesh (entities)
description: '## 概述'
summary: 'Kmesh 是一个基于 eBPF 和可编程内核的无 Sidecar 服务网格，在内核空间实现流量治理能力。与传统 Sidecar 模式（如 Istio/Envoy）不同，Kmesh 将 L4/L7 流量管理逻辑下沉到操作系统内核，消除了 Sidecar 代理带来的额外延迟和资源开销，同时保持与 Istio 控制平面的兼容性。'
category: entities
tags:
- k8s
- cncf
- networking
- kmesh
- istio
- envoy
- cilium
- crd
- operator
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kmesh 是什么
- 如何 Kmesh
trigger_keywords:
- Kmesh
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kmesh

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, C

## 概述

Kmesh 是由华为开源的基于 eBPF 的无 Sidecar 服务网格，2023 年加入 CNCF Sandbox。与传统 Sidecar 模式（如 Istio/Envoy）不同，Kmesh 将 L4/L7 流量管理逻辑下沉到操作系统内核空间，利用 eBPF 和可编程内核技术在 socket 层和 cgroup 层实现流量治理，消除了 Sidecar 代理带来的额外延迟和资源开销，同时保持与 Istio 控制平面的兼容性。

## 核心特性

- **无 Sidecar**: eBPF 内核级流量治理，无需注入 Sidecar 容器
- **低延迟**: 内核空间处理，消除用户态代理的上下文切换开销
- **L4 治理**: 基于 sockmap 和 skmsg 实现 TCP 层流量路由
- **L7 治理**: 通过 waypoint 代理实现 HTTP/gRPC 层路由
- **Istio 兼容**: 复用 Istio 控制平面（istiod）和 xDS API
- **混合模式**: 可与 Istio Sidecar 共存，按命名空间选择模式

## 架构

Kmesh 分为数据平面和控制平面。数据平面在节点内核中运行 eBPF 程序：kmesh-cni 在容器创建时设置 cgroup 和 socket 映射；sockmap eBPF 程序在内核 TCP 栈中拦截和路由流量； waypoint 代理（Envoy）处理需要 L7 治理的流量。控制平面 kmesh-daemon 从 istiod 接收 xDS 配置（监听器、集群、路由），编译为 eBPF map 配置注入内核。L4 流量在内核直接处理，L7 流量重定向到 waypoint。无需为每个 Pod 注入 Sidecar，大幅减少资源消耗。

## Kubernetes 集成

Kmesh 通过 DaemonSet 部署在每个节点上，以特权模式运行以加载 eBPF 程序。与 Istio 控制平面（istiod）集成，复用 VirtualService、DestinationRule 等 CRD 定义治理策略。通过命名空间标签（如 `istio.io/dataplane-mode=kmesh`）选择启用 Kmesh 而非 Sidecar 模式。支持标准的 Kubernetes Service 和 NetworkPolicy。

## 生产使用场景

1. **高性能服务网格**: 对延迟敏感的场景（如金融交易），消除 Sidecar 开销
2. **大规模集群**: 减少 Sidecar 带来的内存和 CPU 消耗（每 Pod 节省 ~100MB）
3. **渐进迁移**: 从 Sidecar 模式逐步迁移到无 Sidecar 模式
4. **边缘计算**: 资源受限场景下使用轻量级网格能力

## 安装

```bash
# 前置: 确保 Istio 控制平面已安装
istioctl install --set profile=minimal
# 安装 Kmesh
kubectl apply -f https://raw.githubusercontent.com/kmesh-net/kmesh/deploy/yaml/kmesh.yaml
# 启用命名空间的 Kmesh 模式
kubectl label namespace default istio.io/dataplane-mode=kmesh
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Kmesh** | 无 Sidecar、内核级低延迟 | 内核版本要求高（>=5.10）、较新 |
| Cilium Service Mesh | eBPF 原生、成熟 | L7 能力有限 |
| Istio Ambient Mesh | Istio 官方无 Sidecar 方案 | 仍需 ztunnel + waypoint |
| Sidecar (Envoy) | 功能最全面、生态最大 | 资源开销大 |

## 架构定位

在 CNCF 生态中，Kmesh 属于 **Networking / Service Mesh** 类别，代表了 Sidecar-less 服务网格的技术方向。它与 Istio 控制平面深度兼容。

## 参考链接

- [[istio]]
- [[cilium]]
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[pod-lifecycle]]

## Related

- [[kured]] — Kured (KUbernetes REboot Daemon)
- [[opengemini]] — openGemini
- [[istio]] — Istio
- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kmesh
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
