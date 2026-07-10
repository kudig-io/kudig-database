---
title: Inclavare Containers (entities)
description: '## 概述'
summary: 'Inclavare Containers 是一个基于硬件可信执行环境 (TEE) 的机密容器项目。它利用 Intel SGX、ARM TrustZone 等硬件安全技术，在隔离的 Enclave 中运行容器工作负载，保护数据和代码的机密性和完整性。即使宿主机操作系统或 Hypervisor 被攻破，Enclave 内的数据也不会泄露。'
category: entities
tags:
- k8s
- cncf
- security
- inclavare-containers
- prometheus
- containerd
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Inclavare Containers 是什么
- 如何 Inclavare Containers
trigger_keywords:
- Inclavare
- Containers
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Inclavare Containers

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go, Rust, C

## 概述

Inclavare Containers 是一个基于硬件可信执行环境 (TEE) 的机密容器项目。它利用 Intel SGX、ARM TrustZone 等硬件安全技术，在隔离的 Enclave 中运行容器工作负载，保护数据和代码的机密性和完整性。即使宿主机操作系统或 Hypervisor 被攻破，Enclave 内的数据也不会泄露。Inclavare Containers 兼容 OCI 标...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **EPC 内存规划**: SGX EPC 内存有限（通常 128-256 MB），合理规划应用内存使用
- **最小化 TCB**: 减少 Enclave 内的代码量，降低可信计算基（TCB）复杂度
- **远程证明**: 生产环境中始终启用远程证明，验证 Enclave 的真实性
- **密钥管理**: 使用远程证明后的安全通道获取密钥，不要硬编码密钥
- **性能调优**: 减少 Enclave 进出（ECALL/OCALL）次数，降低上下文切换开销

## 架构定位

在 CNCF 生态中，inclavare-containers 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[containerd]]
- [[pod-lifecycle]]

## Related

- [[atlantis]] — Atlantis
- [[实体/tetragon.md|[[Tetragon|tetragon]]]] — Tetragon
- [[submariner]] — Submariner
- deployment]] — Prometheus 高可用部署
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- inclavare-containers
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
