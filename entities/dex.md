---
title: Dex (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- dex
- prometheus
- grafana
- argocd
- postgresql
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dex 是什么
- 如何 Dex
trigger_keywords:
- Dex
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
created: "2026-05-23"
---

# Dex

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

Dex 是一个身份联合服务，实现 OpenID Connect (OIDC) 协议。它作为身份代理，连接各种身份提供商（LDAP、SAML、GitHub、Google 等），为 Kubernetes 和其他应用提供统一的认证接口。

## 核心能力

- **OIDC 提供商**: 标准 OpenID Connect 实现
- **身份联合**: 连接多种上游身份提供商
- **Kubernetes 集成**: 原生支持 K8s API Server 认证
- **轻量级**: 单二进制文件，资源占用小
- **可扩展**: 支持自定义连接器
- **静态配置**: YAML 配置文件，易于版本控制

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **HTTPS**: 生产环境必须启用 HTTPS
- **存储后端**: 使用 Kubernetes CRD 或 PostgreSQL 持久化
- **密钥轮换**: 定期轮换 signing keys
- **审计日志**: 启用访问日志记录
- **高可用**: 部署多副本 + 共享存储

## 架构定位

在 CNCF 生态中，dex 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/argocd.md|[[ArgoCD|argocd]]]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/storage-model.md|storage-model]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[kubefleet]] — KubeFleet
- [[kuma]] — Kuma
- [[kuberhealthy]] — Kuberhealthy
- [[tokenetes]] — Tokenetes
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- [[domain-06-observability/00-open-source-projects-index.md|00-open-source-projects-index]]
- 00-open-source-projects-index
- 04-cncf-fta-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- [[domain-06-observability/00-open-source-projects-index.md|00-open-source-projects-index]]
- [[domain-06-observability/00-open-source-projects-index.md|00-open-source-projects-index]]
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- [[domain-05-security-compliance/00-open-source-projects-index.md|00-open-source-projects-index]]
- 00-open-source-projects-index
- 00-open-source-projects-index
- [[domain-05-security-compliance/00-open-source-projects-index.md|00-open-source-projects-index]]
- 00-open-source-projects-index
- [[domain-05-security-compliance/00-open-source-projects-index.md|00-open-source-projects-index]]
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- 00-open-source-projects-index
- [[domain-10-troubleshooting-diagnostics/topic-fta/fta-index.md|fta-index]]
- dex
- [[skills/ts-ai-ml-workloads.md|AI/ML 工作负载排查]] — Cross-reference
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
