---
title: Confidential Containers (CoCo)
description: '## 概述'
summary: 'Confidential Containers (CoCo) 是一个为 Kubernetes 提供机密计算能力的项目，使容器工作负载能够在硬件 TEE（可信执行环境）中运行。通过利用 AMD SEV、Intel TDX、IBM SE 等硬件机密计算技术，CoCo 保护运行中的数据免受云提供商、管理员和其他特权软件的访问。'
category: entities
tags:
- k8s
- cncf
- security
- confidential-containers
- opa
- crd
- operator
- agent
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Confidential Containers (CoCo) 是什么
- 如何 Confidential Containers (CoCo)
trigger_keywords:
- Confidential
- Containers
- CoCo
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Confidential Containers (CoCo)

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Rust, Go

## 概述

Confidential Containers (CoCo) 是一个为 Kubernetes 提供机密计算能力的项目，使容器工作负载能够在硬件 TEE（可信执行环境）中运行。通过利用 AMD SEV、Intel TDX、IBM SE 等硬件机密计算技术，CoCo 保护运行中的数据免受云提供商、管理员和其他特权软件的访问。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **硬件验证**: 部署前确认节点 CPU 支持所需的 TEE 技术并已在 BIOS 中启用
- **镜像加密**: 生产环境始终使用加密容器镜像，密钥通过 KBS 管理
- **证明策略**: 使用 OPA 策略精确定义可接受的 TEE 证据和固件版本
- **密钥轮换**: 定期轮换 KBS 中的加密密钥，配合镜像重新加密
- **网络隔离**: KBS 服务应部署在安全的网络区域，限制访问来源
- **审计日志**: 启用 KBS 的证明审计日志，记录所有密钥分发事件

## 架构定位

在 CNCF 生态中，confidential-containers 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[spire]] — SPIRE
- [[akri]] — Akri
- [[实体/cncf-edge-ai.md|cncf-edge-ai]] — CNCF 边缘计算与 AI/ML 项目全景
- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- confidential-containers
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[实体/tetragon.md|Tetragon]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
