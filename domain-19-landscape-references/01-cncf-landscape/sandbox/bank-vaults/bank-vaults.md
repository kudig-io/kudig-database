---
title: Bank-Vaults (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- bank-vaults
- etcd
- prometheus
- crd
- operator
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Bank-Vaults 是什么
- 如何 Bank-Vaults
trigger_keywords:
- Bank-Vaults
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
created: "2026-05-23"
---

# Bank-Vaults

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

Bank-Vaults 是一套围绕 HashiCorp Vault 构建的 Kubernetes 原生密钥管理工具集。它提供 Vault Operator 自动化部署和管理 Vault 集群、Webhook 自动注入密钥到 Pod 环境变量和文件、以及多种云 KMS 后端的自动解封能力。Bank-Vaults 大幅简化了在 Kubernetes 环境中使用 Vault 进行密钥管理的复杂度。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **HA 部署**: 生产环境至少 3 节点 Raft 集群，确保 Vault 高可用
- **KMS 解封**: 使用云 KMS 自动解封，避免手动解封操作
- **最小权限**: 每个应用使用独立的 Vault Role 和 Policy，遵循最小权限原则
- **密钥路径规范**: 按命名空间/应用组织密钥路径，如 `secret/data/{namespace}/{app}`
- **审计日志**: 启用 Vault 审计日志，监控密钥访问行为

## 架构定位

在 CNCF 生态中，bank-vaults 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/vault|vault]]
- [[deployment]]
- [[entities/crd-custom-resources|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[submariner]] — Submariner
- [[03-prometheus-ha-deployment]] — [[Prometheus|Prometheus]]us 高可用部署|Prometheus 高可用部署]]
- [[inclavare-containers]] — Inclavare Containers
- [[entities/vault|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- bank-vaults
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
