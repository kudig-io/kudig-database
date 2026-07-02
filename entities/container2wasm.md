---
title: container2wasm (entities)
description: '## 概述'
summary: 'container2wasm 是一个将 Linux 容器镜像转换为 WebAssembly (WASM) 模块的工具。它通过嵌入 Linux 内核模拟器（基于 Bochs x86 模拟器或 TinyEMU RISC-V 模拟器），使原本为 x86_64/aarch64 编译的容器镜像能够在任何支持 WASM 的环境中运行，'
category: entities
tags:
- k8s
- cncf
- runtime
- container2wasm
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- container2wasm 是什么
- 如何 container2wasm
trigger_keywords:
- container2wasm
prerequisites:
- kubectl-basics
---



# container2wasm

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

container2wasm 是一个将 Linux 容器镜像转换为 WebAssembly (WASM) 模块的工具。它通过嵌入 Linux 内核模拟器（基于 Bochs x86 模拟器或 TinyEMU RISC-V 模拟器），使原本为 x86_64/aarch64 编译的容器镜像能够在任何支持 WASM 的环境中运行，包括浏览器、边缘设备和 WASM 运行时（如 Wasmtime、Was...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **选择 RISC-V**: 对于体积敏感的场景，使用 `--target-arch=riscv64` 生成更小的 WASM
- **精简镜像**: 使用 Alpine 等轻量镜像减少转换后的 WASM 体积
- **外部层**: 对大型镜像使用 `--assets-to-external-bundle` 分离层数据
- **浏览器优化**: 预加载 WASM 模块并使用 [[Service|Service]] Worker 缓存
- **网络隔离**: 生产环境谨慎配置网络代理，避免安全风险

## 架构定位

在 CNCF 生态中，container2wasm 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]]

## Related

- [[kubevirt]] — KubeVirt
- [[wasmcloud]] — wasmCloud
- [[spiderpool]] — Spiderpool
- [[ratify]] — Ratify
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- container2wasm
- [[entities/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
