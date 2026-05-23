---
title: Parsec
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- parsec
- argocd
- ingress
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Parsec 是什么
- 如何 Parsec
trigger_keywords:
- Parsec
prerequisites:
- kubectl-basics
- gitops-basics
created: "2026-05-23"
---

# Parsec

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Rust

## 概述

Parsec 是一个平台安全抽象层，为应用程序提供统一的 API 来访问底层硬件安全模块（HSM）、可信平台模块（TPM）和其他加密硬件。它通过 IPC 机制（Unix Domain Socket）对外提供统一的加密操作接口，屏蔽了不同安全硬件的差异，使应用无需关心底层使用的是 TPM 2.0、PKCS#11 HSM 还是 Arm TrustZone。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **硬件优先**: 生产环境优先使用 TPM 或 HSM 后端，开发环境可用 Mbed Crypto
- **密钥命名**: 使用有意义的密钥名称，包含应用和用途信息
- **权限控制**: 通过 Unix Socket 权限和 SELinux 策略控制 Parsec 访问
- **备份策略**: HSM 后端的密钥需要配合 HSM 自身的备份机制
- **监控**: 监控 Parsec 服务的可用性和操作延迟

## 架构定位

在 CNCF 生态中，parsec 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/argocd.md|[[ArgoCD|argocd]]]]
- [[entities/vault.md|[[HashiCorp Vault|vault]]]]
- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[entities/emissary-ingress.md|ingress]]]] — Emissary-Ingress
- [[kubevela]] — KubeVela
- [[piraeus-datastore]] — [[Piraeus Datastore|Piraeus Datastore]]
- [[k8up]] — K8up
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- parsec
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
