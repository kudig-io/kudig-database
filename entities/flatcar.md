---
title: Flatcar Container Linux (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- runtime
- flatcar
- etcd
- containerd
- crd
- operator
- serverless
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Flatcar Container Linux 是什么
- 如何 Flatcar Container Linux
trigger_keywords:
- Flatcar
- Container
- Linux
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# Flatcar Container Linux

> **CNCF 状态**: Incubating | **类别**: Runtime | **主要语言**: Shell, Go

## 概述

Flatcar Container Linux 是为容器优化的不可变 Linux 发行版，是 CoreOS Container Linux 的延续和替代品。它提供最小化、自动更新、安全的容器运行环境。

## 核心能力

- **不可变基础设施**: 只读根文件系统，配置通过 Ignition/Cloud-Init
- **自动更新**: 内置 A/B 分区自动更新机制
- **最小化设计**: 只包含运行容器必需的组件
- **安全加固**: SELinux、只读 rootfs、自动安全补丁
- **多平台支持**: AWS、Azure、GCP、VMware、裸金属等
- **兼容性**: 完全兼容 CoreOS Container Linux

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Ignition 配置**: 使用 Ignition 实现声明式配置
- **自动更新**: 配置更新窗口避免业务高峰
- **协调更新**: 使用 locksmith 协调集群节点更新
- **监控**: 监控更新状态和系统健康
- **LTS 版本**: 生产环境考虑使用 LTS 通道

## 架构定位

在 CNCF 生态中，flatcar 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[containerd]]
- [[concepts/container-runtime-comparison.md|container-runtime-comparison]]
- [[concepts/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[serverless-devs]] — [[entities/serverless-devs.md|Serverless Devs]]
- [[sermant]] — Sermant
- [[loxilb]] — LoxiLB
- [[kube-ovn]] — Kube-OVN
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- flatcar
- [[entities/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index.md|Node 知识图谱索引]]
