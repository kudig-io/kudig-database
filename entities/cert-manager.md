---
title: cert-manager (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- cert-manager
- envoy
- crd
- operator
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cert-manager 是什么
- 如何 cert-manager
trigger_keywords:
- cert-manager
prerequisites:
- kubectl-basics
- tls-basics
created: "2026-05-23"
---

# cert-manager

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，cert-manager 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/vault.md|[[HashiCorp Vault|vault]]]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[litmus]] — LitmusChaos
- [[pixie]] — Pixie
- [[dapr]] — Dapr
- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 99-cert-manager-tls-guide
- cert-manager
- RELEASE-NOTES-0.12
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.16
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-0.13
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-0.7
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-0.4
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.20
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.15
- [[entities/kubearmor.md|KubeArmor]]
- [[entities/openfga.md|OpenFGA]]
- [[references/release-notes-security|发布说明索引 — 安全]] — Cross-reference
- [[references/kudig-man-pages-index|KUDIG Man Pages Index]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[concepts/cloud-native-defense-in-depth|Cloud Native Defense in Depth]] — Cross-reference
- [[concepts/security-tool-evolution|安全工具演进]] — Cross-reference
- [[skills/ts-ai-ml-workloads|AI/ML 工作负载排查]] — Cross-reference
- [[skills/ts-security-auth|安全认证故障排查]] — Cross-reference
- [[skills/Kubernetes Diagnostic Skills Overview|Kubernetes Diagnostic Skills Overview]] — Cross-reference
- [[skills/ts-control-plane|控制平面故障排查]] — Cross-reference
- [[skills/Agent Orchestration Patterns|Agent Orchestration Patterns for FTA]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/cert-index|Certificate / TLS 证书知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
