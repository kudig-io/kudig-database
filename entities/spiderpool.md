---
title: Spiderpool (entities)
description: '## 概述'
summary: 'Spiderpool 是一个 Kubernetes 的 Underlay 网络 IPAM (IP Address Management) 解决方案，专为数据中心和云原生环境设计。它支持固定 IP、多网卡、双栈网络等高级特性，能够与多种 CNI 插件无缝集成，特别适合需要 Pod 与物理网络直接通信的场景。'
category: entities
tags:
- k8s
- cncf
- networking
- spiderpool
- cilium
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
- Spiderpool 是什么
- 如何 Spiderpool
trigger_keywords:
- Spiderpool
prerequisites:
- kubectl-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Spiderpool

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Spiderpool 是一个 Kubernetes 的 Underlay 网络 IPAM (IP Address Management) 解决方案，专为数据中心和云原生环境设计。它支持固定 IP、多网卡、双栈网络等高级特性，能够与多种 CNI 插件无缝集成，特别适合需要 Pod 与物理网络直接通信的场景。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，spiderpool 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[cilium]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]

## Related

- [[openfunction]] — OpenFunction
- [[kubevirt]] — KubeVirt
- [[wasmcloud]] — wasmCloud
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- spiderpool
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
