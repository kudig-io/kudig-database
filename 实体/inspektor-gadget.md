---
title: Inspektor Gadget [entities]
description: '## 概述'
summary: 'Inspektor Gadget 是一组基于 eBPF 的工具集合 ("gadgets")，用于调试和检查 Kubernetes 集群中的应用程序。它利用 eBPF 在内核级别收集数据，提供对容器和 Pod 的深入可观测性，无需修改应用程序代码或添加 sidecar。'
category: general
tags:
- k8s
- prometheus
- grafana
- cilium
- daemonset
- crd
- operator
- ebpf
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Inspektor Gadget 是什么
- 如何 Inspektor Gadget
trigger_keywords:
- Inspektor
- Gadget
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "Inspektor Gadget"
category: entities
summary: "Inspektor Gadget 是一组基于 eBPF 的工具集合 ("gadgets")，用于调试和检查 Kubernetes 集群中的应用程序。它利用 eBPF 在内核级别收集数据，提供对容器和 Pod 的深入可观测性，无需修改应用程序代码或添加 sidecar。"
tags: k8s, cncf, observability, inspektor-gadget]
sources: ["docs/生态参考/sandbox/inspektor-gadget/inspektor-gadget.md", "生态参考/sandbox/inspektor-gadget/inspektor-gadget.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: reference
base_confidence: 0.7
---

# Inspektor Gadget

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go, C (eBPF)

## 概述

Inspektor Gadget 是由 Microsoft（Kinvolk）开源的 eBPF 工具集合（"Gadgets"），用于调试和检查 Kubernetes 集群中的应用程序，2021 年加入 CNCF Sandbox。它利用 eBPF 在内核级别收集数据，提供对容器和 Pod 的深入可观测性，无需修改应用程序代码或添加 Sidecar。Inspektor Gadget 覆盖网络、进程、文件系统、安全等多个调试领域。

## 核心特性

- **eBPF 驱动**: 利用 eBPF 实现低开销的内核级可观测性
- **Kubernetes 感知**: 自动关联容器和 Pod 元数据（命名空间、Pod 名、容器名）
- **丰富 Gadgets**: 网络（DNS、TCP）、进程（exec、fork）、文件系统（open、reads）、安全（capabilities、seccomp）
- **kubectl 插件**: 通过 `kubectl gadget` 命令直接使用
- **可编程**: 支持自定义 eBPF 程序和 Gadgets
- **跨平台**: 支持 Linux 内核 5.4+，兼容多种 CNI 和容器运行时

## 架构

Inspektor Gadget 采用 DaemonSet + CLI 架构。Gadget DaemonSet 以特权模式部署在每个节点上，负责加载 eBPF 程序和收集数据。`kubectl-gadget` CLI 通过 Kubernetes API 与各节点的 Gadget Daemon 通信，下发 Gadgets 配置和收集结果。每个 Gadget 是一个独立的 eBPF 程序，挂载到特定的内核跟踪点（tracepoint、kprobe、uprobe）。当内核触发跟踪点时，eBPF 程序捕获事件数据，附加容器元数据后通过 ringbuffer 传递到用户空间。

## Kubernetes 集成

Inspektor Gadget 通过 DaemonSet 部署在所有节点。CLI 作为 kubectl 插件运行，通过 Kubernetes API 路由请求到目标节点。Gadgets 自动关联 Kubernetes 元数据——将内核事件中的 cgroup ID 映射到 Pod/Container 名称。支持按命名空间、Pod 标签、容器名过滤事件。与 CNI 无关——在内核层采集，兼容 Calico、Cilium、Flannel 等所有网络插件。

## 生产使用场景

1. **网络调试**: 使用 dns gadget 排查 DNS 解析问题，tcpdump gadget 抓取容器流量
2. **安全审计**: 使用 audit-seccomp 生成 seccomp Profile，capabilities gadget 审计容器权限
3. **性能分析**: 使用 profile gadget 生成 CPU 火焰图，分析应用性能瓶颈
4. **文件追踪**: 使用 tracesnoop/fis 监控文件读写操作，排查 IO 问题

## 安装

```bash
# 安装 kubectl 插件
kubectl krew install gadget
# 部署到集群
kubectl gadget deploy
# 使用 Gadgets
kubectl gadget top pods --sort cpu       # Pod CPU 排行
kubectl gadget advise seccomp monitor    # 生成 seccomp Profile
kubectl gadget profile cpu --node node1  # CPU 火焰图
kubectl gadget trace dns --pod web       # DNS 追踪
kubectl gadget trace exec --namespace prod  # 进程执行追踪
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Inspektor Gadget** | 多功能 Gadget 集、kubectl 原生 | 特权 DaemonSet |
| Pixie | 零侵入、PxL 强大 | 仅关注协议层可观测性 |
| Cilium Hubble | eBPF 网络可观测优秀 | 仅 Cilium 环境 |
| bpftrace | eBPF 编程灵活 | 非 K8s 原生 |

## 架构定位

在 CNCF 生态中，Inspektor Gadget 属于 **Observability / Debugging** 类别，是 Kubernetes eBPF 调试工具箱的代表性项目。它与 Pixie、Cilium 互补，更偏向开发和排障场景。

## 参考链接

- [[prometheus-grafana]]
- [[observability-pillars]]
- [[cilium-ebpf-networking]]
- [[pod-lifecycle]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
