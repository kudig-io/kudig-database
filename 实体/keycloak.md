---
title: Keycloak [entities]
description: '## 概述'
summary: 'Keycloak 是开源的身份和访问管理（IAM）解决方案，提供单点登录（SSO）、身份联合、用户管理和细粒度授权功能。它支持 OpenID Connect、OAuth 2.0 和 SAML 2.0 标准协议。'
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
last_updated: 2026-07
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Keycloak

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Java

## 概述

Keycloak 是由 Red Hat 开源的身份和访问管理（IAM）解决方案，2023 年加入 CNCF Incubating。它提供单点登录（SSO）、身份联合、用户管理和细粒度授权功能，支持 OpenID Connect（OIDC）、OAuth 2.0 和 SAML 2.0 标准协议。Keycloak 是企业级 IAM 领域最流行的开源方案之一，被广泛应用于微服务、API 网关和 Kubernetes 集群的身份认证场景。

## 核心特性

- **单点登录 (SSO)**: 一次登录访问多个应用，支持 Web 和移动端
- **身份联合**: 集成 LDAP、Active Directory、社交登录（Google/GitHub/Microsoft）
- **标准协议**: OpenID Connect 1.0、OAuth 2.0、SAML 2.0
- **多租户**: Realm 隔离的多租户架构，每个租户独立管理
- **细粒度授权**: 基于角色（RBAC）、资源（UMA）和策略的访问控制
- **用户管理**: 用户注册、密码策略、多因素认证（OTP/WebAuthn）

## 架构

Keycloak 基于 Java（Quarkus）构建。核心组件包括：Auth Server（处理认证和授权请求）、Realm Manager（管理多租户 Realm）、Identity Brokering（OIDC/SAML 身份联合）、User Federation（LDAP/AD 用户同步）、Token Generator（签发 JWT/OAuth Token）。数据层使用关系型数据库（PostgreSQL、MySQL、MariaDB）存储用户和配置。Keycloak 可以集群部署，通过分布式 Infinispan 缓存实现 Session 和 Token 共享。

## Kubernetes 鿟成

Keycloak 通过 Keycloak Operator 或 Helm Chart 部署到 Kubernetes。Operator 通过 Keycloak CRD 管理实例生命周期。在 K8s 场景中，Keycloak 常作为 OIDC Provider 与 API Server 集成——配置 `--oidc-issuer-url` 使 kubectl 通过 Keycloak 认证。Ingress/API Gateway（如 Envoy、Traefik）可集成 Keycloak 实现集群入口的统一认证。支持通过 ProtocolMapper 为 K8s ServiceAccount 映射 RBAC 角色。

## 生产使用场景

1. **统一身份认证**: 为所有内部应用提供 SSO 和集中式用户管理
2. **K8s API 认证**: 作为 Kubernetes API Server 的 OIDC Provider
3. **API 网关认证**: 在 API Gateway 层集成 Keycloak 进行请求认证
4. **多租户 SaaS**: 使用 Realm 隔离为不同租户提供独立的身份管理

## 安装

```bash
# Helm 安装
helm repo add bitnami https://charts.bitnami.com/bitnami
helm install keycloak bitnami/keycloak \
  --set auth.adminUser=admin \
  --set auth.adminPassword=secure-password \
  --set global.postgresql.auth.postgresPassword=pg-password
# 或使用 Operator
kubectl apply -f https://raw.githubusercontent.com/keycloak/keycloak-k8s-resources/latest/kubernetes/keycloaks.k8s.keycloak.org-v1.yml
kubectl apply -f - <<EOF
apiVersion: k8s.keycloak.org/v2alpha1
kind: Keycloak
spec:
  instances: 2
  db: { vendor: postgres, host: pg-svc }
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Keycloak** | CNCF Incubating、Red Hat 支持 | Java 资源开销大 |
| Dex | 轻量级、K8s 原生 | 功能少（仅 OIDC 桥接） |
| Authentik | Python、灵活 | 社区较小 |
| Auth0 | SaaS、零运维 | 商业产品 |

## 架构定位

在 CNCF 生态中，Keycloak 属于 **Security / IAM** 类别，是开源 IAM 领域的标杆项目。它在 Kubernetes 身份认证生态中扮演 OIDC Provider 的核心角色。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[operator-pattern]]
- [[概念/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[概念/secrets-management.md|secrets-management]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[实体/argocd.md|[[ArgoCD|argocd]]]] — ArgoCD
- [[ovn-kubernetes]] — OVN-Kubernetes
- [[vitess]] — Vitess
- [[argo]] — Argo Workflows
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- keycloak
- [[实体/pixie.md|Pixie]]
- [[实体/kuberhealthy.md|Kuberhealthy]]
- [[实体/kubescape.md|Kubescape]]
- [[实体/perses.md|Perses]]
- [[实体/03-prometheus-ha-deployment.md|Prometheus 高可用部署]]
- [[实体/trickster.md|Trickster]]
- [[实体/distribution.md|Distribution]]
- [[实体/hami.md|HAMI]]
- [[实体/06-containerd-observability.md|containerd 可观测性]]
- [[实体/kubeelasti.md|KubeElastic]]
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference


<!-- risk-assessed -->
