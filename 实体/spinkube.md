---
title: SpinKube [entities]
description: '## 概述'
summary: 'SpinKube 是一个在 Kubernetes 上运行 WebAssembly (Wasm) 微服务和应用的开源平台。它将 Fermyon Spin 框架与 Kubernetes 集成，使开发者能够像部署容器一样部署 Wasm 应用，同时获得更快的启动速度、更小的资源占用和更强的安全隔离。'
category: entities
tags:
- k8s
- cncf
- runtime
- spinkube
- prometheus
- containerd
- gateway
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SpinKube 是什么
- 如何 SpinKube
trigger_keywords:
- SpinKube
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SpinKube

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust, Go

## 概述

SpinKube 是由 Microsoft（Fermyon 团队）开源的 WebAssembly（Wasm）应用运行平台，2024 年加入 CNCF Sandbox。它将 Fermyon Spin 框架与 Kubernetes 集成，使开发者能够像部署容器一样部署 Wasm 应用，同时获得更快的启动速度（毫秒级）、更小的资源占用（MB 级）和更强的安全隔离（Wasm 沙箱）。SpinKube 代表了 Wasm 作为容器补充运行时的方向。

## 核心特性

- **Wasm 原生**: 将 Spin Wasm 应用作为一等公民部署到 Kubernetes
- **极速启动**: Wasm 模块毫秒级启动，适合 Serverless 和事件驱动场景
- **低资源占用**: 每个 Wasm 实例仅几 MB 内存，高密度部署
- **SpinApp CRD**: 通过 CRD 声明式管理 Wasm 应用
- **OCI 分发**: Wasm 应用通过 OCI Artifact 分发，复用标准 Registry
- **containerd shim**: 通过 spin-shim 与 containerd 原生集成

## 架构

SpinKube 由 Spin Operator 和 containerd-shim-spin 组成。Spin Operator 监听 SpinApp CRD，管理 Wasm 应用的副本和调度。containerd-shim-spin 是 containerd 的 OCI Runtime Shim，使 containerd 能够直接运行 Wasm 模块而非容器镜像。当 Pod 指定 RuntimeClass 为 `wasmtime-spin-v2` 时，kubelet 通过 CRI 调用 containerd，containerd 通过 shim 加载 Wasm 模块并在 Wasmtime 运行时中执行。Wasm 应用通过 Spin SDK 访问 Key-Value Store、SQLite、HTTP 等组件能力。

## Kubernetes 集成

SpinKube 通过 RuntimeClass 与 Kubernetes 集成。`runtimeClassName: wasmtime-spin-v2` 指示 kubelet 使用 Wasm 运行时。SpinApp CRD 定义 Wasm 应用的镜像（OCI 引用）、副本数、环境变量和触发器。Operator 将 SpinApp 转换为标准 Deployment + Service。containerd 的 shim 层处理 Wasm 模块加载和执行，对 Kubernetes 控制平面完全透明。支持标准的 HPA、Service 和 Ingress。

## 生产使用场景

1. **Serverless 函数**: 事件驱动的 Wasm 函数，毫秒级冷启动
2. **API 微服务**: 轻量级 HTTP API 服务，高密度部署
3. **边缘计算**: 在资源受限的边缘节点上运行 Wasm 应用
4. **事件处理**: 消息队列消费者的轻量级处理函数

## 安装

```bash
# 安装 Spin Operator
kubectl apply -f https://github.com/spinkube/spin-operator/releases/download/v0.4.0/spin-operator.crds.yaml
kubectl apply -f https://github.com/spinkube/spin-operator/releases/download/v0.4.0/spin-operator.runtime-class.yaml
kubectl apply -f https://github.com/spinkube/spin-operator/releases/download/v0.4.0/spin-operator.deployment.yaml
# 部署 Spin 应用
kubectl apply -f - <<EOF
apiVersion: core.spinkube.dev/v1alpha1
kind: SpinApp
metadata: { name: hello-wasm }
spec:
  image: ghcr.io/spinkube/containerd-shim-spin/examples/spin-rust-hello:v0.4.0
  replicas: 3
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **SpinKube** | K8s 原生 Wasm、CRD 管理 | 较新、生态小 |
| WasmEdge + containerd | CNCF Wasm 运行时 | 需手动集成 |
| Kuasar | 多沙箱运行时 | 通用方案、非 Wasm 专注 |
| 容器 (containerd) | 最成熟、生态最大 | 启动慢、资源占用大 |

## 架构定位

在 CNCF 生态中，SpinKube 属于 **Runtime / WebAssembly** 类别，是 Wasm 在 Kubernetes 上的代表性运行平台。它代表了容器与 Wasm 共存的未来方向。

## 参考链接

- [[containerd]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/autoscaling-strategies.md|autoscaling-strategies]]

## Related

- [[kube-rs]] — kube-rs
- [[02-prometheus-promql-advanced]] — PromQLQL 高级查询|PromQL 高级查询]]
- [[capsule]] — Capsule
- [[spin]] — Spin
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-spinkube-framework
- spinkube
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
