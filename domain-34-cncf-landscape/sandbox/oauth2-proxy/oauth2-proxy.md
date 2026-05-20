---
title: OAuth2 Proxy
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- docker
- opa
- redis
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- OAuth2 Proxy 是什么
- 如何 OAuth2 Proxy
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- OAuth2
- Proxy
- cncf
- landscape
---


# OAuth2 Proxy

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://oauth2-proxy.github.io/oauth2-proxy/ |
| **GitHub** | https://github.com/oauth2-proxy/oauth2-proxy |
| **许可证** | MIT |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

OAuth2 Proxy 是一个反向代理，提供基于 OAuth2/OIDC 协议的身份认证功能。它充当应用程序前的认证网关，支持 Google、GitHub、Azure AD、Keycloak 等多种身份提供商（IdP），使后端应用无需自行实现认证逻辑即可获得统一的身份验证能力。

### 核心特性

- **多 Provider 支持**: Google, GitHub, GitLab, Azure AD, Keycloak, OIDC 等 20+ 提供商
- **反向代理模式**: 支持独立反向代理和 Nginx/Traefik auth_request 子请求模式
- **会话管理**: Cookie、Redis、文件系统等多种会话存储后端
- **令牌传递**: 将 ID Token、Access Token 通过 Header 传递给上游服务
- **细粒度授权**: 基于邮箱、域名、组的访问控制
- **Kubernetes 原生**: 与 Ingress Controller 深度集成
- **刷新令牌**: 自动刷新过期令牌，保持用户会话

---

## 架构设计

```
┌──────────┐     ┌──────────────────────┐     ┌──────────────┐
│          │     │    OAuth2 Proxy       │     │              │
│  User    │────►│                       │────►│  Upstream    │
│  Browser │     │  ┌────────────────┐  │     │  Application │
│          │◄────│  │ Auth Middleware │  │◄────│              │
└──────────┘     │  └────────┬───────┘  │     └──────────────┘
                 │           │          │
                 │  ┌────────┴───────┐  │
                 │  │ Session Store  │  │
                 │  │ (Cookie/Redis) │  │
                 │  └────────┬───────┘  │
                 └───────────┼──────────┘
                             │
                    ┌────────┴────────┐
                    │  Identity       │
                    │  Provider       │
                    │  (OIDC/OAuth2)  │
                    │  Keycloak/      │
                    │  Google/GitHub  │
                    └─────────────────┘
```

### 认证流程

```
1. User ──► OAuth2 Proxy ──► 检查 Session Cookie
2. 无有效 Session ──► 302 重定向到 IdP 登录页
3. 用户在 IdP 完成认证 ──► 回调 OAuth2 Proxy
4. OAuth2 Proxy 验证令牌 ──► 创建 Session ──► 设置 Cookie
5. 请求携带 Cookie ──► OAuth2 Proxy 验证 ──► 转发到上游 + 注入 Headers
```

---

## 快速开始

### Docker 部署

```bash
docker run -d --name oauth2-proxy \
  -p 4180:4180 \
  quay.io/oauth2-proxy/oauth2-proxy:latest \
  --provider=github \
  --email-domain="*" \
  --upstream=http://backend:8080 \
  --http-address=0.0.0.0:4180 \
  --cookie-secret=$(openssl rand -base64 32 | tr -- '+/' '-_') \
  --client-id=<GITHUB_CLIENT_ID> \
  --client-secret=<GITHUB_CLIENT_SECRET>
```

### Kubernetes Deployment

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: oauth2-proxy
  namespace: auth
spec:
  replicas: 2
  selector:
    matchLabels:
      app: oauth2-proxy
  template:
    metadata:
      labels:
        app: oauth2-proxy
    spec:
      containers:
        - name: oauth2-proxy
          image: quay.io/oauth2-proxy/oauth2-proxy:v7.6.0
          ports:
            - containerPort: 4180
              name: http
            - containerPort: 44180
              name: metrics
          args:
            - --provider=oidc
            - --oidc-issuer-url=https://keycloak.example.com/realms/myrealm
            - --email-domain=*
            - --upstream=http://backend-svc:8080
            - --http-address=0.0.0.0:4180
            - --metrics-address=0.0.0.0:44180
            - --cookie-secure=true
            - --cookie-samesite=lax
            - --set-xauthrequest=true
            - --pass-access-token=true
            - --session-store-type=redis
            - --redis-connection-url=redis://redis-svc:6379
          envFrom:
            - secretRef:
                name: oauth2-proxy-secrets
---
apiVersion: v1
kind: Secret
metadata:
  name: oauth2-proxy-secrets
  namespace: auth
type: Opaque
stringData:
  OAUTH2_PROXY_CLIENT_ID: "my-client-id"
  OAUTH2_PROXY_CLIENT_SECRET: "my-client-secret"
  OAUTH2_PROXY_COOKIE_SECRET: "base64-encoded-32-byte-secret"
```

---

## 配置详解

### Nginx Ingress auth_request 模式

```yaml
# OAuth2 Proxy Service
apiVersion: v1
kind: Service
metadata:
  name: oauth2-proxy
  namespace: auth
spec:
  selector:
    app: oauth2-proxy
  ports:
    - port: 4180
      targetPort: http
---
# 受保护的 Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: protected-app
  annotations:
    nginx.ingress.kubernetes.io/auth-url: "https://oauth2.example.com/oauth2/auth"
    nginx.ingress.kubernetes.io/auth-signin: "https://oauth2.example.com/oauth2/start?rd=$scheme://$host$escaped_request_uri"
    nginx.ingress.kubernetes.io/auth-response-headers: "X-Auth-Request-User,X-Auth-Request-Email,X-Auth-Request-Groups,Authorization"
spec:
  rules:
    - host: app.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: backend-svc
                port:
                  number: 8080
```

### Traefik ForwardAuth 模式

```yaml
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: oauth2-proxy-auth
spec:
  forwardAuth:
    address: http://oauth2-proxy.auth.svc.cluster.local:4180/oauth2/auth
    trustForwardHeader: true
    authResponseHeaders:
      - X-Auth-Request-User
      - X-Auth-Request-Email
      - X-Auth-Request-Groups
      - Authorization
---
apiVersion: traefik.io/v1alpha1
kind: IngressRoute
metadata:
  name: protected-app
spec:
  routes:
    - match: Host(`app.example.com`)
      kind: Rule
      middlewares:
        - name: oauth2-proxy-auth
      services:
        - name: backend-svc
          port: 8080
```

### 高级 Provider 配置

```yaml
# Keycloak OIDC 配置
args:
  - --provider=oidc
  - --oidc-issuer-url=https://keycloak.example.com/realms/myrealm
  - --allowed-group=/admin,/developers  # Keycloak 组授权
  - --oidc-groups-claim=groups
  - --code-challenge-method=S256  # PKCE 支持
  - --skip-provider-button=true  # 跳过选择页直接跳转 IdP

# Azure AD 配置
args:
  - --provider=azure
  - --azure-tenant=<TENANT_ID>
  - --oidc-issuer-url=https://login.microsoftonline.com/<TENANT_ID>/v2.0
  - --allowed-group=<GROUP_OBJECT_ID>

# Google 配置
args:
  - --provider=google
  - --google-admin-email=admin@example.com
  - --google-group=engineering@example.com
  - --google-service-account-json=/etc/oauth2-proxy/sa.json
```

---

## 高级功能

### Redis 会话存储 (HA)

```yaml
args:
  - --session-store-type=redis
  - --redis-use-sentinel=true
  - --redis-sentinel-master-name=mymaster
  - --redis-sentinel-connection-urls=redis://sentinel-0:26379,redis://sentinel-1:26379,redis://sentinel-2:26379
  - --redis-password=<REDIS_PASSWORD>
```

### 多上游路由

```yaml
args:
  - --upstream=http://app1:8080/app1/
  - --upstream=http://app2:8080/app2/
  - --upstream=file:///var/www/static/#/static/
```

### API 令牌认证

```yaml
# 支持 Bearer Token 认证（无浏览器场景）
args:
  - --skip-jwt-bearer-tokens=true
  - --extra-jwt-issuers=https://keycloak.example.com/realms/myrealm=audience1
```

---

## 监控

### Prometheus 指标

| 指标 | 说明 |
|:---|:---|
| `oauth2_proxy_requests_total` | 请求总数 |
| `oauth2_proxy_response_duration_seconds` | 响应时间分布 |
| `oauth2_proxy_api_requests_total` | API 请求计数 |
| `oauth2_proxy_auth_success_total` | 认证成功次数 |

---

## 最佳实践

1. **Cookie 安全**: 生产环境启用 `cookie-secure=true` 和 `cookie-httponly=true`
2. **会话存储**: 多副本部署使用 Redis 共享会话，避免 Cookie-only 的大小限制
3. **PKCE**: 启用 `code-challenge-method=S256` 增强授权码流安全性
4. **令牌传递**: 使用 `set-xauthrequest=true` 让上游获取用户信息
5. **组授权**: 使用 `allowed-group` 实现基于组的细粒度访问控制
6. **监控告警**: 监控 `oauth2_proxy_auth_success_total` 异常变化检测潜在攻击

---

## 参考资源

- [OAuth2 Proxy 官方文档](https://oauth2-proxy.github.io/oauth2-proxy/)
- [OAuth2 Proxy GitHub](https://github.com/oauth2-proxy/oauth2-proxy)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
