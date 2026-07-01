---
title: Keycloak [entities]
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- keycloak
- prometheus
- grafana
- argocd
- containerd
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
- Keycloak 是什么
- 如何 Keycloak
trigger_keywords:
- Keycloak
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---



# Keycloak

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Java

## 概述

Keycloak 是开源的身份和访问管理（IAM）解决方案，提供单点登录（SSO）、身份联合、用户管理和细粒度授权功能。它支持 OpenID Connect、OAuth 2.0 和 SAML 2.0 标准协议。

## 核心能力

- **单点登录 (SSO)**: 一次登录访问多个应用
- **身份联合**: 集成 LDAP、Active Directory、社交登录
- **标准协议**: OpenID Connect、OAuth 2.0、SAML 2.0
- **多租户**: Realm 隔离的多租户架构
- **细粒度授权**: 基于角色、资源、策略的访问控制
- **高可用**: 支持集群部署和数据库复制

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **生产安全**: 强制 HTTPS，配置合适的 Token 过期时间
- **密码策略**: 启用密码复杂度、历史记录、暴力破解保护
- **多因素认证**: 为敏感操作启用 OTP/WebAuthn
- **会话管理**: 配置会话超时，启用 SSO Session Idle
- **审计日志**: 启用 Event Logging，集成 SIEM
- **备份恢复**: 定期备份 Realm 配置和数据库

## 架构定位

在 CNCF 生态中，keycloak 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[operator-pattern]]
- [[concepts/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[entities/argocd.md|[[ArgoCD|argocd]]]] — ArgoCD
- [[ovn-kubernetes]] — OVN-Kubernetes
- [[vitess]] — Vitess
- [[argo]] — Argo Workflows
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- keycloak
- [[entities/pixie.md|Pixie]]
- [[entities/kuberhealthy.md|Kuberhealthy]]
- [[entities/kubescape.md|Kubescape]]
- [[entities/perses.md|Perses]]
- [[entities/03-prometheus-ha-deployment.md|Prometheus 高可用部署]]
- [[entities/trickster.md|Trickster]]
- [[entities/distribution.md|Distribution]]
- [[entities/hami.md|HAMI]]
- [[entities/06-containerd-observability.md|containerd 可观测性]]
- [[entities/kubeelasti.md|KubeElastic]]
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
