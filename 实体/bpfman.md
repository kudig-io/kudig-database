---
title: bpfman (entities)
description: '## 概述'
summary: 'bpfman 是一个 eBPF 程序管理器，提供系统守护进程和 Kubernetes Operator，用于集中加载、管理和监控 eBPF 程序。'
category: entities
tags:
- k8s
- cncf
- networking
- bpfman
- cilium
- argocd
- crd
- operator
- ebpf
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- bpfman 是什么
- 如何 bpfman
trigger_keywords:
- bpfman
prerequisites:
- kubectl-basics
- gitops-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# bpfman

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Rust

## 概述

bpfman 是一个 eBPF 程序管理器，由 Red Hat 推动开发，2023 年加入 CNCF 沙箱。它提供系统守护进程和 Kubernetes Operator，用于集中加载、管理和监控 eBPF 程序。bpfman 解决了多个应用（如 Cilium、Falco、Tetragon）同时使用 eBPF 时的管理混乱问题——这些程序各自加载 eBPF bytecode，可能导致 hook 冲突和资源竞争。bpfman 提供统一的 eBPF 程序生命周期管理、多程序共享挂载点、权限控制和可观测性。它支持将 eBPF bytecode 打包为 OCI 镜像通过 Registry 分发，实现了 eBPF 程序的云原生部署和版本管理。

## 核心能力

- **统一 eBPF 管理**: 集中加载、卸载和监控所有 eBPF 程序，避免冲突
- **OCI 镜像分发**: 将 eBPF bytecode 打包为 OCI 镜像，通过 Registry 管理版本
- **Kubernetes Operator**: 通过 CRD 声明式管理 eBPF 程序部署
- **程序类型支持**: XDP、TC、Tracepoint、Kprobe、Uprobe、Perf Event 等
- **优先级控制**: 为多个 eBPF 程序设置执行优先级，确保关键程序优先
- **可观测性**: 暴露 Prometheus 指标，跟踪 eBPF 程序加载状态和错误

## 架构

bpfman 采用守护进程 + Operator 双模式架构：

- **bpfman (守护进程)**: 运行在每个节点上的系统守护进程，负责实际的 eBPF 程序加载和管理
- **bpfman-agent**: 运行在节点上的 gRPC 接口，接收来自 Kubernetes 的指令
- **bpfman-operator**: Kubernetes Operator，通过 CRD 管理集群范围的 eBPF 程序部署
- **BpfProgram CRD**: 声明式定义 eBPF 程序（bytecode 来源、挂载点、参数）
- **OCI Image**: eBPF bytecode 以 OCI 镜像格式存储在 Registry 中
- **Map 管理**: 共享 eBPF Maps，支持多程序间通信

管理流程：`BpfProgram CRD → Operator → bpfman-agent → bpfman → 加载 eBPF bytecode 到内核`

## K8s 集成

bpfman 通过 Kubernetes Operator 深度集成集群管理。bpfman-operator 监听 `BpfProgram` CRD，将期望的 eBPF 程序分发到目标节点（通过 nodeSelector/affinity 控制）。每个节点运行 bpfman 守护进程和 bpfman-agent，agent 通过 gRPC 与 Operator 通信。eBPF bytecode 以 OCI 镜像形式存储在 Registry（如 Harbor）中，节点拉取后通过 bpfman 加载到内核。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 DaemonSet 模式类似，确保所有目标节点都运行指定的 eBPF 程序。

## 生产场景

1. **安全监控部署**: 统一部署 Tetragon/Falco 类安全 eBPF 程序，避免与 CNI 的 eBPF 程序冲突
2. **网络观测**: 部署 XDP/TC eBPF 程序进行网络流量监控和过滤
3. **性能分析**: 动态部署 perf event eBPF 程序进行性能 profiling，无需重启节点
4. **多团队 eBPF 共存**: 不同团队（网络、安全、可观测性）的 eBPF 程序通过 bpfman 统一管理

## 安装

```bash
# 安装 bpfman Operator
kubectl apply -f https://github.com/bpfman/bpfman/releases/latest/download/bpfman-operator.yaml

# 部署 eBPF 程序（OCI 镜像方式）
kubectl apply -f - <<EOF
apiVersion: bpfman.io/v1alpha1
kind: BpfProgram
metadata:
  name: xdp-drop-example
spec:
  type: xdp
  interfaceSelector:
    primary: true
  bytecode: quay.io/bpfman/xdp-drop:latest
  section: drop
  mapownerselector:
    matchLabels:
      myapp: drop-map
EOF

# 查看已加载的 eBPF 程序
kubectl get bpfprograms -A
```

## 对比

| 特性 | bpfman | Cilium (内置) | Inspektor Gadget | bpfd |
|------|--------|---------------|-----------------|------|
| 统一管理 | ✅ 多源 | ❌ 仅自身 | ⚠️ 工具集 | ✅ |
| OCI 分发 | ✅ | ❌ | ❌ | ✅ |
| K8s Operator | ✅ | ❌ | ✅ | ✅ |
| 语言 | Rust | Go | Go | Rust |

## 架构定位

在 CNCF 生态中，bpfman 属于 **Networking** 类别，为云原生应用提供统一的 eBPF 程序生命周期管理能力。

## 参考链接

- [[cilium]]
- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]]

## Related

- [[opentofu]] — OpenTofu
- [[cartography]] — Cartography
- [[46-terway-performance-tuning]] — Terway 性能调优
- [[volcano]] — Volcano
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- bpfman
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
