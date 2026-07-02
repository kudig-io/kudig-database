---
title: Ratify (entities)
description: '## 概述'
summary: 'Ratify 是一个容器镜像供应链安全验证框架，用作 Kubernetes 准入控制器，在 Pod 创建时验证容器镜像的签名、SBOM、漏洞扫描报告等供应链工件（Artifacts）。它与 Gatekeeper/OPA 集成，通过可插拔的验证器架构支持 Notary v2 签名、Cosign 签名、SBOM 验证、漏洞报告检查等多种供应链安全策略。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- ratify
- opa
- crd
- operator
- wasm
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Ratify 是什么
- 如何 Ratify
trigger_keywords:
- Ratify
prerequisites:
- kubectl-basics
- policy-basics
---



# Ratify

> **CNCF 状态**: Sandbox | **类别**: Supply Chain | **主要语言**: Go

## 概述

Ratify 是一个容器镜像供应链安全验证框架，用作 Kubernetes 准入控制器，在 Pod 创建时验证容器镜像的签名、SBOM、漏洞扫描报告等供应链工件（Artifacts）。它与 Gatekeeper/OPA 集成，通过可插拔的验证器架构支持 Notary v2 签名、Cosign 签名、SBOM 验证、漏洞报告检查等多种供应链安全策略。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **渐进实施**: 先用 dryrun 模式观察，确认无误后再切换到 deny 模式
- **多验证器**: 组合使用签名验证 + SBOM 验证 + 漏洞检查实现深度防御
- **证书管理**: 使用 Kubernetes Secret 管理验证证书，配置自动轮换
- **命名空间隔离**: 先在生产命名空间启用强制验证，逐步扩展
- **缓存配置**: 合理配置 Ratify 的验证结果缓存，平衡安全性和性能

## 架构定位

在 CNCF 生态中，ratify 属于 **Supply Chain** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[kyverno]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kubevirt]] — KubeVirt
- [[wasmcloud]] — wasmCloud
- [[spiderpool]] — Spiderpool
- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- ratify
- [[entities/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
