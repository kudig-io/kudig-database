---
title: Assets
description: Assets 目录索引
summary: Assets 目录索引
category: index
tags:
- index
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
---


# Assets

> 本页为 `assets` 目录的自动索引。

## 子目录

- assets/presentations/

## 架构图（Mermaid）

### Kubernetes 核心架构

- [Kubernetes Control Plane 架构](architecture-k8s-control-plane.md) — API Server、etcd、Scheduler、Controller Manager、Cloud Controller Manager 及其交互。
- [K8s 网络模型](architecture-k8s-network-model.md) — CNI、kube-proxy、CoreDNS、Service/EndpointSlice、Ingress、Pod 网络。
- [CNI 数据平面对比](architecture-cni-data-plane.md) — iptables vs IPVS vs eBPF 三种数据路径。
- [K8s 存储栈](architecture-storage-stack.md) — PV → PVC → CSI Driver → Storage Backend 挂载时序。
- [RBAC 认证授权流程](architecture-rbac-flow.md) — User/SA → AuthN → AuthZ(RBAC) → Admission → Resource。
- [Operator 调谐循环](architecture-operator-reconcile.md) — Watch → Compare → Act reconcile loop。
- [Pod 生命周期状态机](architecture-pod-lifecycle.md) — Pending → Running → Succeeded/Failed 全状态机。

### 平台与生态

- [Service Mesh 架构](architecture-service-mesh.md) — Sidecar vs Sidecarless（Cilium / Istio Ambient）。
- [可观测性三支柱栈](architecture-observability-stack.md) — Metrics/Logs/Traces 与 OTel Collector 中枢。
- [GitOps 部署流程](architecture-gitops-flow.md) — Git → ArgoCD/Flux → K8s 端到端时序。
- [GPU 共享模型](architecture-gpu-sharing.md) — Time-Slicing vs MPS vs MIG 对比。
- [多集群架构模式](architecture-multi-cluster.md) — Hub-Spoke / Federation / Service Mesh。
- [零信任分层安全](architecture-zero-trust-security.md) — 从供应链到 API 的纵深防御。
- [灾难恢复架构模式](architecture-disaster-recovery.md) — Active-Passive / Active-Active / Pilot Light。
- [平台工程 / IDP](architecture-platform-engineering.md) — 内部开发者平台分层架构。
