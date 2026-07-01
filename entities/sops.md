---
title: SOPS (Secrets OPerationS)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- supply-chain
- sops
- prometheus
- argocd
- flux
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SOPS (Secrets OPerationS) 是什么
- 如何 SOPS (Secrets OPerationS)
trigger_keywords:
- SOPS
- Secrets
- OPerationS
prerequisites:
- kubectl-basics
- prometheus-basics
- gitops-basics
created: "2026-05-23"
---

# [[SOPS|SOPS]] ([[Secrets|Secrets]] OPerationS)

> **CNCF 状态**: Sandbox | **类别**: Supply Chain | **主要语言**: Go

## 概述

SOPS (Secrets OPerationS) 是一个加密文件编辑器，支持 YAML、JSON、ENV 和 BINARY 格式。它使用 AWS KMS、GCP KMS、Azure Key Vault、HashiCorp Vault 或 PGP 密钥对文件中的值进行加密，而保持键名明文，便于版本控制和代码审查。SOPS 是 GitOps 工作流中管理敏感信息的核心工具。

## 核心能力

- **多格式支持**: YAML、JSON、ENV、INI、BINARY
- **键值分离**: 只加密值，保留键名可读
- **多 KMS 后端**: AWS KMS、GCP KMS、Azure、Vault、age、PGP
- **多密钥加密**: 同时使用多个密钥加密
- **审计日志**: 加密/解密操作审计
- **GitOps 友好**: 加密文件可安全提交到 Git

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **age 优先**: 新项目推荐使用 age 而非 PGP
- **密钥轮换**: 定期轮换加密密钥
- **.sops.yaml**: 始终配置 .sops.yaml 简化使用
- **Git 集成**: 使用 git-diff 配置显示加密差异
- **CI/CD**: 在 pipeline 中使用 KMS 而非本地密钥
- **最小权限**: 按环境和团队划分加密密钥

## 架构定位

在 CNCF 生态中，sops 属于 **Supply Chain** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[flux]]
- [[entities/argocd.md|argocd]]
- [[entities/vault.md|vault]]
- [[deployment]]
- [[concepts/controller-pattern.md|controller-pattern]]

## Related

- [[prometheus]] — Prometheus
- [[interlink]] — InterLink
- [[tinkerbell]] — Tinkerbell
- [[entities/vault.md|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- sops
- [[entities/ratify.md|Ratify]]
- [[concepts/IaC x 多集群管理.md|基础设施即代码 x 多集群管理]] — Cross-reference
- [[concepts/GitOps x 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
