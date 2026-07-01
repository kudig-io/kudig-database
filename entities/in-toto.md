---
title: in-toto (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- supply-chain
- in-toto
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- in-toto 是什么
- 如何 in-toto
trigger_keywords:
- in-toto
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# in-toto

> **CNCF 状态**: Graduated | **类别**: Supply Chain | **主要语言**: Python, Go

## 概述

description: '## 项目概述'

## 核心能力

- **布局定义**: 定义预期的软件供应链流程
- **链接元数据**: 记录每个构建步骤的输入输出
- **签名验证**: 加密签名保护元数据完整性
- **策略执行**: 验证实际流程符合预期布局
- **SBOM 集成**: 与软件物料清单集成
- **多语言支持**: Python、Go、Java、Rust 实现

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 使用硬件安全模块存储签名密钥
- 配置多个 functionary 分权
- 与 CI/CD 系统集成
- 定期轮换密钥
- 保护 Layout 签名密钥
- 使用阈值签名

## 架构定位

在 CNCF 生态中，in-toto 属于 **Supply Chain** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/security-defense-depth.md|security-defense-depth]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[opentofu]] — OpenTofu
- [[cartography]] — Cartography
- [[volcano]] — Volcano
- [[bpfman]] — bpfman
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- in-toto
- [[entities/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
