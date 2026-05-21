---
title: Metal3
description: 'summary: "Metal3（Metal Kubed）提供裸金属基础设施的 Kubernetes 原生管理能力。它基于 Cluster API 实现裸金属服务器的自动发现、配置和生命周期管理，实现"裸金属即服务"。"'
category: general
tags:
- k8s
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Metal3 是什么
- 如何 Metal3
trigger_keywords:
- Metal3
prerequisites:
- kubectl-basics
---

---
title: "Metal3"
category: entities
summary: "Metal3（Metal Kubed）提供裸金属基础设施的 Kubernetes 原生管理能力。它基于 Cluster API 实现裸金属服务器的自动发现、配置和生命周期管理，实现"裸金属即服务"。"
tags: [k8s, cncf, metal, metal3-io]
sources: ["docs/domain-19-landscape-references/incubating/metal3-io/metal3-io.md", "domain-19-landscape-references/incubating/metal3-io/metal3-io.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: draft
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# Metal3

> **CNCF 状态**: Incubating | **类别**: Metal | **主要语言**: Go

## 概述

Metal3（Metal Kubed）提供裸金属基础设施的 Kubernetes 原生管理能力。它基于 Cluster API 实现裸金属服务器的自动发现、配置和生命周期管理，实现"裸金属即服务"。

## 核心能力

- **Kubernetes 原生**: CRD 方式管理裸金属服务器
- **Cluster API 集成**: 统一的集群生命周期管理
- **自动发现**: 通过 IPMI/Redfish 发现服务器
- **配置管理**: 自动化操作系统安装和配置
- **生命周期管理**: 开机、关机、重装、回收
- **无代理**: 使用 BMC 协议，无需在服务器安装代理

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **BMC 网络**: 确保管理集群可访问所有 BMC
- **镜像管理**: 使用 HTTP 服务器托管操作系统镜像
- **硬件标签**: 使用标签区分不同硬件配置
- **DHCP 配置**: 配置 PXE 启动所需的 DHCP
- **监控**: 监控配置进度和 BMC 连接状态

## 架构定位

在 CNCF 生态中，metal3-io 属于 **Metal** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[crd-custom-resources]]
- [[operator-pattern]]
- [[controller-pattern]]
- [[secrets-management]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
