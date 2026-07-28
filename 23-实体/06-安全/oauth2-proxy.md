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

## 安装与配置

```bash
# Helm 安装 OAuth2 Proxy
helm repo add oauth2-proxy https://oauth2-proxy.github.io/manifests
helm install oauth2-proxy oauth2-proxy/oauth2-proxy \
  -n oauth2-proxy --create-namespace \
  --set config.clientID="your-client-id" \
  --set config.clientSecret="your-client-secret" \
  --set config.cookieSecret="$(openssl rand -base64 32 | head -c 32 | base64)" \
  --set extraArgs.provider=oidc \
  --set extraArgs.oidc-issuer-url="https://keycloak.example.com/realms/myrealm" \
  --set extraArgs.email-domains="*" \
  --set extraArgs.cookie-secure=true \
  --set extraArgs.cookie-httponly=true
# 验证部署
kubectl get pods -n oauth2-proxy
```

```yaml
# Ingress 集成（Nginx Ingress auth-url 模式）
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: grafana-protected
  annotations:
    nginx.ingress.kubernetes.io/auth-url: "https://oauth2-proxy.oauth2-proxy.svc/oauth2/auth"
    nginx.ingress.kubernetes.io/auth-signin: "https://auth.example.com/oauth2/start?rd=https://$host$request_uri"
    nginx.ingress.kubernetes.io/auth-response-headers: "X-Auth-Request-Email,X-Auth-Request-User"
spec:
  rules:
  - host: grafana.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: grafana
            port:
              number: 3000
```

```yaml
# Redis 会话存储配置（多副本部署）
extraArgs:
  session-store-type: redis
  redis-connection-url: redis://redis.oauth2-proxy.svc:6379
  cookie-refresh: 1h
  cookie-expire: 24h
```

## 运维操作

```bash
# 🟢 查看 OAuth2 Proxy 状态
kubectl get pods -n oauth2-proxy
kubectl logs -n oauth2-proxy -l app.kubernetes.io/name=oauth2-proxy --tail=50

# 🟢 检查认证指标
curl -s http://oauth2-proxy:44180/metrics | grep oauth2_proxy

# 🟢 测试认证端点
curl -I https://auth.example.com/oauth2/auth

# 🟡 重启 OAuth2 Proxy
kubectl rollout restart deployment/oauth2-proxy -n oauth2-proxy

# 🟡 更新配置
helm upgrade oauth2-proxy oauth2-proxy/oauth2-proxy -n oauth2-proxy --set ...

# 🔴 卸载 OAuth2 Proxy（所有受保护应用失去认证）
helm uninstall oauth2-proxy -n oauth2-proxy
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 无限重定向循环 | Cookie 域名不匹配/secure 设置错误 | 检查浏览器 Cookie | 调整 cookie-domain 和 cookie-secure |
| 403 Forbidden | 邮箱域/组不匹配 | `kubectl logs -n oauth2-proxy` | 检查 email-domains 和 allowed-group |
| 会话过期频繁 | Cookie 有效期过短 | 检查 cookie-expire 配置 | 增加 cookie-expire 和 cookie-refresh |
| 多副本会话丢失 | 未使用 Redis 存储 | 检查 session-store-type | 配置 Redis 会话存储 |
| IdP 回调失败 | redirect-url 配置错误 | 检查 IdP 应用配置 | 核对 redirect-url 与 IdP 回调地址 |

```
排查流程：
├─ 认证失败
│  ├─ 检查 OAuth2 Proxy 日志
│  ├─ 验证 IdP 配置（clientID/secret）
│  └─ 检查 redirect-url 是否正确
├─ 会话问题
│  ├─ 检查 Cookie 设置（domain/secure/httponly）
│  ├─ 多副本检查 Redis 连接
│  └─ 检查 cookie-expire 配置
└─ 访问控制问题
   ├─ 检查 email-domains 配置
   ├─ 检查 allowed-group 配置
   └─ 验证 IdP 返回的 groups claim
```

## 生产案例

### 案例 1：内部工具统一 SSO

- **场景**: 20+ 内部工具（Grafana/Kibana/Jenkins/ArgoCD）需要统一认证
- **排查**: 各工具独立认证管理复杂，无法统一离职回收
- **方案**: OAuth2 Proxy + Keycloak OIDC，所有工具通过 Ingress auth-url 保护
- **效果**: 统一 SSO 登录，离职一键回收所有工具访问权限

### 案例 2：基于组的细粒度访问控制

- **场景**: 不同团队只能访问各自的 Dashboard，生产环境仅 SRE 可访问
- **排查**: 传统方案需要每个工具单独配置权限
- **方案**: OAuth2 Proxy allowed-group + Keycloak 组管理，Ingress 级别控制
- **效果**: 统一组权限管理，新工具接入仅需添加 Ingress 注解

## 对比

| 维度 | OAuth2 Proxy | Authelia | vouch-proxy | Pomerium |
|------|-------------|----------|-------------|----------|
| 多 IdP | ✅ | ✅ | ✅ | ✅ |
| Redis 会话 | ✅ | ✅ | ❌ | ✅ |
| 零信任 | ❌ | ❌ | ❌ | ✅ |
| 社区活跃度 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |
| 适用场景 | 通用 SSO | 家庭/小团队 | 轻量 | 企业零信任 |

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]

## Related

- [[drasi]] — Drasi
- [[containerssh]] — ContainerSSH
- [[modelpack]] — ModelPack
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[keycloak]] — Keycloak

- oauth2-proxy
- [[23-实体/cncf-security.md|[[23-实体/15-参考与索引/cncf-security|CNCF 安全与合规项目全景]]]] — Cross-reference


<!-- risk-assessed -->
