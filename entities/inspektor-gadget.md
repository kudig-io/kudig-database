---
title: Inspektor Gadget [entities]
description: '## 概述'
summary: '## 概述'
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
last_updated: 2026-05
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



---
title: "Inspektor Gadget"
category: entities
summary: "Inspektor Gadget 是一组基于 eBPF 的工具集合 ("gadgets")，用于调试和检查 Kubernetes 集群中的应用程序。它利用 eBPF 在内核级别收集数据，提供对容器和 Pod 的深入可观测性，无需修改应用程序代码或添加 sidecar。"
tags: k8s, cncf, observability, inspektor-gadget]
sources: ["docs/domain-19-landscape-references/sandbox/inspektor-gadget/inspektor-gadget.md", "domain-19-landscape-references/sandbox/inspektor-gadget/inspektor-gadget.md"]
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

Inspektor Gadget 是一组基于 eBPF 的工具集合 ("gadgets")，用于调试和检查 Kubernetes 集群中的应用程序。它利用 eBPF 在内核级别收集数据，提供对容器和 Pod 的深入可观测性，无需修改应用程序代码或添加 sidecar。

## 核心能力

- **eBPF 驱动**: 利用 eBPF 实现低开销的内核级可观测性
- **Kubernetes 感知**: 自动关联容器和 Pod 元数据
- **多种 Gadgets**: 网络、进程、文件系统、安全等多领域工具
- **本地和远程**: 支持 kubectl 插件和独立 CLI
- **可编程**: 支持自定义 eBPF 程序
- **跨平台**: 支持 Linux 内核 5.4+

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **资源开销**: eBPF 程序运行在内核空间，开销极低
- **权限控制**: Gadget DaemonSet 需要特权权限
- **过滤优化**: 使用过滤器减少数据量
- **安全审计**: 使用 capabilities 和 seccomp 追踪进行安全审计
- **故障排查**: 结合多个 gadget 进行综合分析
- **内核版本**: 确保内核版本 >= 5.4

## 架构定位

在 CNCF 生态中，inspektor-gadget 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[prometheus-grafana]]
- [[observability-pillars]]
- [[cilium-ebpf-networking]]
- [[pod-lifecycle]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
