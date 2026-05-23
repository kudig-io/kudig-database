---
title: youki [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- runtime
- youki
- containerd
- cri-o
- crd
- operator
- wasm
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- youki 是什么
- 如何 youki
trigger_keywords:
- youki
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# youki

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

youki 是一个用 Rust 实现的 OCI 容器运行时，作为 runc 的替代品。它完全兼容 OCI Runtime Specification，同时利用 Rust 的内存安全特性减少潜在的安全漏洞。youki 可与 containerd、CRI-O、Podman 等高级容器运行时集成。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **生产评估**: 在非生产环境充分测试后再替换 runc
- **Rootless 模式**: 优先使用 rootless 模式运行容器
- **安全增强**: 利用 Rust 的内存安全减少运行时安全风险
- **Wasm 实验**: 尝试 youki 的 Wasm 运行时特性用于轻量级工作负载
- **版本锁定**: 在生产环境锁定 youki 版本，避免未测试的更新

## 架构定位

在 CNCF 生态中，youki 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[containerd]]
- [[pod-lifecycle]]

## Related

- [[kairos]] — Kairos
- [[kaito]] — KAITO
- [[cri-o]] — CRI-O
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd

- youki
- [[concepts/container-runtime-comparison|[[Container Runtime|Container Runtime]]me Comparison|Container Runtime Comparison]]]] — Cross-reference
- [[concepts/docker-architecture|[[Docker Architecture and Container Runtime|Docker Architecture and Container Runtime]]]] — Cross-reference
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
