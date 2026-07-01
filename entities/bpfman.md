---
title: bpfman (entities)
description: '## 概述'
summary: '## 概述'
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
last_updated: 2026-05
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



# bpfman

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Rust

## 概述

bpfman 是一个 eBPF 程序管理器，提供系统守护进程和 Kubernetes Operator，用于集中加载、管理和监控 eBPF 程序。它解决了多个应用同时使用 eBPF 时的管理混乱问题，提供统一的 eBPF 程序生命周期管理、多程序共享挂载点、权限控制和可观测性，使 eBPF 程序的部署和运维更加安全和可控。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **OCI 打包**: 将 eBPF bytecode 打包为 OCI 镜像，通过 Registry 管理版本
- **优先级设置**: 合理设置程序优先级，确保关键程序优先执行
- **节点选择器**: 使用 nodeSelector 控制 eBPF 程序的部署范围
- **监控**: 监控 bpfman 暴露的指标，跟踪 eBPF 程序的加载状态和错误
- **内核版本**: 确保节点内核版本支持所需的 eBPF 程序类型

## 架构定位

在 CNCF 生态中，bpfman 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[cilium]]
- [[entities/argocd.md|[[ArgoCD|argocd]]]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]

## Related

- [[opentofu]] — OpenTofu
- [[cartography]] — Cartography
- [[46-terway-performance-tuning]] — Terway 性能调优
- [[volcano]] — Volcano
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- bpfman
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
