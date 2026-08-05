---
title: Keylime (entities)
description: '## 概述'
summary: 'Keylime 是一个基于 TPM (Trusted Platform Module) 的远程引导完整性验证和运行时完整性监控系统。它利用硬件 TPM 芯片提供加密度量，持续验证节点的引导过程和运行时状态是否被篡改，适用于零信任安全架构中的节点信任验证。'
category: entities
tags:
- k8s
- cncf
- security
- keylime
- argocd
- containerd
- rook
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Keylime 是什么
- 如何 Keylime
trigger_keywords:
- Keylime
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Keylime

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Rust, Python

## 概述

Keylime 是一个基于 TPM (Trusted Platform Module) 的远程引导完整性验证和运行时完整性监控系统。它利用硬件 TPM 芯片提供加密度量，持续验证节点的引导过程和运行时状态是否被篡改，适用于零信任安全架构中的节点信任验证。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **TPM 确认**: 确保节点配备 TPM 2.0 芯片并在 BIOS 中启用
- **IMA 启用**: 配置 Linux IMA 策略实现运行时完整性监控
- **允许列表**: 维护准确的文件 hash 允许列表，定期更新
- **撤销操作**: 配置验证失败时的自动响应（K8s cordon/drain）
- **密钥管理**: 利用 Keylime 的安全密钥分发替代手动密钥部署

## 架构定位

在 CNCF 生态中，keylime 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/argocd.md|[[ArgoCD|argocd]]]]
- [[operator-pattern]]

## Related

- [[32-发布/package/2026-07-02_18-40/corpus/core/entities/01-containerd-v2-features]] — [[containerd|containerd]]rd 2.0 新特性|containerd 2.0 新特性]]
- [[karmada]] — Karmada
- [[rook]] — Rook
- [[microcks]] — Microcks
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- keylime

<!-- risk-assessed -->
