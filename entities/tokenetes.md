---
title: Tokenetes (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- tokenetes
- prometheus
- grafana
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tokenetes 是什么
- 如何 Tokenetes
trigger_keywords:
- Tokenetes
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# Tokenetes

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Java / Go

## 概述

Tokenetes（也称为 Vault CRD Operator）是一个 Kubernetes Operator，用于将 HashiCorp Vault 中的密钥自动同步到 Kubernetes [[Secrets|Secrets]]。它通过自定义资源 (CRD) 简化了 Vault 与 Kubernetes 的集成，支持多种认证方式和密钥类型，让开发者能够以声明式方式管理敏感数据。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- weight: 100

## 架构定位

在 CNCF 生态中，tokenetes 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/vault.md|vault]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]

## Related

- [[kuma]] — Kuma
- [[kuberhealthy]] — Kuberhealthy
- [[entities/trivy.md|[[Trivy|trivy]]]] — Trivy
- [[entities/vault.md|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tokenetes
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
