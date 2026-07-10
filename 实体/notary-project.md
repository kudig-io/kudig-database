---
title: Notary Project (entities)
description: '## 概述'
summary: 'Notary Project 提供容器镜像和 OCI 制品的签名、验证规范与工具。它是软件供应链安全的关键组件，通过数字签名确保制品的完整性和来源可信。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- notary-project
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Notary Project 是什么
- 如何 Notary Project
trigger_keywords:
- Notary
- Project
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Notary Project

> **CNCF 状态**: Incubating | **类别**: Supply Chain | **主要语言**: Go

## 概述

Notary Project 提供容器镜像和 OCI 制品的签名、验证规范与工具。它是软件供应链安全的关键组件，通过数字签名确保制品的完整性和来源可信。

## 核心能力

- **标准规范**: OCI 兼容的签名规范
- **Notation CLI**: 签名和验证的命令行工具
- **多种签名方式**: 本地密钥、KMS、硬件令牌
- **信任策略**: 灵活的签名验证策略配置
- **插件架构**: 支持第三方 KMS 和签名服务
- **无侵入**: 签名存储在 OCI 清单，不修改原始镜像

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **密钥管理**: 使用 KMS 服务管理签名密钥，避免本地存储
- **证书轮换**: 定期轮换签名证书，保持短期有效期
- **分层策略**: 生产环境使用 strict，开发环境可用 permissive
- **审计日志**: 记录所有签名和验证操作
- **供应链完整**: 结合 SBOM 和漏洞扫描构建完整供应链安全

## 架构定位

在 CNCF 生态中，notary-project 属于 **Supply Chain** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[实体/vault.md|[[HashiCorp Vault|vault]]]]
- [[kyverno]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[open-cluster-management]] — [[实体/open-cluster-management.md|Open Cluster Management (OCM)]]
- [[cdk8s]] — cdk8s (Cloud Development Kit for Kubernetes)
- [[cloud-custodian]] — Cloud Custodian
- [[kuadrant]] — Kuadrant
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- notary-project
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
