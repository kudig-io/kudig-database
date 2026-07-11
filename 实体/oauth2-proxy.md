---
title: OAuth2 Proxy [entities]
description: '## 概述'
summary: 'OAuth2 Proxy 是一个反向代理，提供基于 OAuth2/OIDC 协议的身份认证功能。它充当应用程序前的认证网关，支持 Google、GitHub、Azure AD、Keycloak 等多种身份提供商（IdP），使后端应用无需自行实现认证逻辑即可获得统一的身份验证能力。'
category: entities
tags:
- k8s
- cncf
- security
- oauth2-proxy
- prometheus
- grafana
- redis
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
- OAuth2 Proxy 是什么
- 如何 OAuth2 Proxy
trigger_keywords:
- OAuth2
- Proxy
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OAuth2 Proxy

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

OAuth2 Proxy 是由 Bitly 发起、后由 OAuth2 Proxy 社区维护的开源反向代理认证网关。它充当应用程序前的**认证屏障**，基于 OAuth2/OpenID Connect（OIDC）协议与外部身份提供商（IdP）交互，验证用户身份后才将请求转发到后端应用。这使得后端应用无需自行实现 OAuth2/OIDC 认证逻辑即可获得 SSO 能力。

OAuth2 Proxy 支持主流 IdP：Google、GitHub、GitLab、Azure AD（Entra ID）、Keycloak、Login.gov、Nextcloud、Bitbucket、LinkedIn 等。它通过 Cookie 存储会话信息，也支持 Redis 作为分布式会话存储实现多副本部署。广泛用于 Grafana、Kibana、内部 Dashboard 等无内置认证的 Web 应用的 SSO 保护。

## Key Features

- **OAuth2/OIDC 认证**：支持标准 OAuth2 Authorization Code Flow 和 OIDC
- **多 IdP 支持**：Google、GitHub、GitLab、Azure AD、Keycloak、OIDC 通用提供商
- **Cookie 会话**：加密 Cookie 存储用户会话，支持配置有效期
- **Redis 会话存储**：多副本部署使用 Redis 共享会话
- **PKCE 支持**：支持 S256 code challenge 增强安全性
- **细粒度访问控制**：基于邮箱域、GitHub 组织/团队、OIDC groups 的访问控制

## Architecture

OAuth2 Proxy 作为反向代理部署在后端应用前面。当未认证用户访问时，Proxy 重定向到 IdP 的授权页面。用户在 IdP 完成认证后，IdP 回调 Proxy，Proxy 获取 Access Token 和用户信息，创建加密 Cookie 会话，重定向用户回原请求 URL。后续请求携带 Cookie，Proxy 验证 Cookie 并将请求（附带 `X-Forwarded-User` 等头）转发到后端。

## K8s 集成

在 Kubernetes 中，OAuth2 Proxy 通常作为 Ingress 的 auth-url 后端部署。Nginx Ingress Controller 通过 `nginx.ingress.kubernetes.io/auth-url` annotation 将认证委托给 OAuth2 Proxy。也支持通过 Service Mesh（Istio EnvoyFilter）或 Gateway API 将认证策略注入到流量路径中。Helm Chart 提供一键部署。

## 生产部署要点

- **Cookie 安全**：生产环境启用 `cookie-secure=true` 和 `cookie-httponly=true`
- **会话存储**：多副本部署使用 Redis 共享会话，避免 Cookie-only 的大小限制
- **PKCE**：启用 `code-challenge-method=S256` 增强授权码流安全性
- **令牌传递**：使用 `set-xauthrequest=true` 让上游获取用户信息
- **组授权**：使用 `allowed-group` 实现基于组的细粒度访问控制
- **监控告警**：监控 `oauth2_proxy_auth_success_total` 异常变化检测潜在攻击

## 生产场景

1. **内部 Dashboard SSO**：保护 Grafana、Kibana、内部管理后台，强制 SSO 认证
2. **API 认证代理**：为无认证能力的 API 添加 OAuth2 认证层
3. **多团队访问控制**：基于 GitHub 组织/AD 组控制不同团队的访问权限
4. **零信任网络**：作为零信任架构中的身份验证网关

## 安装

```bash
# Helm 安装 OAuth2 Proxy
helm repo add oauth2-proxy https://oauth2-proxy.github.io/manifests
helm install oauth2-proxy oauth2-proxy/oauth2-proxy \
  -n oauth2-proxy --create-namespace \
  --set config.clientID="your-client-id" \
  --set config.clientSecret="your-client-secret" \
  --set config.cookieSecret="$(openssl rand -base64 32)" \
  --set extraArgs.provider=github \
  --set extraArgs.email-domains="*"

# Ingress 集成（Nginx Ingress）
kubectl annotate ingress my-dashboard \
  nginx.ingress.kubernetes.io/auth-url="https://oauth2-proxy.oauth2-proxy.svc/oauth2/auth" \
  nginx.ingress.kubernetes.io/auth-signin="https://auth.example.com/oauth2/start?rd=https://$host$request_uri"
```

## 对比

| 特性 | OAuth2 Proxy | Authelia | vouch-proxy | Keycloak Gatekeeper |
|------|-------------|----------|-------------|-------------------|
| 多 IdP | ✅ | ✅ | ✅ | ⚠️ Keycloak only |
| Redis 会话 | ✅ | ✅ | ❌ | ⚠️ |
| Helm Chart | ✅ | ✅ | ❌ | ⚠️ |
| 社区活跃度 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐ |

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[drasi]] — Drasi
- [[containerssh]] — ContainerSSH
- [[modelpack]] — ModelPack
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[keycloak]] — Keycloak

- oauth2-proxy
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference


<!-- risk-assessed -->
