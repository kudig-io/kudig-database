---
title: 11 - API 网关安全体系：认证、鉴权与 WAF
description: '# 11 - API 网关安全体系：认证、鉴权与 WAF'
summary: 'API 网关作为流量入口，是实施纵深防御（Defense in Depth）的核心位置。安全能力从外到内分为多个层次：'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- etcd
- istio
- opa
- redis
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- API 网关安全体系：认证、鉴权与 WAF 是什么
- 如何 API 网关安全体系：认证、鉴权与 WAF
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- API
- 网关安全体系：认证
- 鉴权与
- WAF
- cloud
- native
- api
- gateway
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- etcd-basics
- redis-basics
- tls-basics
- policy-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 11 - API 网关安全体系：认证、鉴权与 WAF

> **文档版本**: v1.0 | **适用版本**: [[Kubernetes|Kubernetes]] 1.25+ | **更新日期**: 2026-03-04 | **关键词**: JWT, OIDC, mTLS, API Key, OPA, WAF, 限流, 零信任, HMAC, Bot 防护

<!-- chunk: 目录 -->## 目录

1. [API 网关安全架构概述](#1-api-网关安全架构概述)
2. [认证模式](#2-认证模式)
3. [鉴权体系](#3-鉴权体系)
4. [WAF 防护](#4-waf-防护)
5. [限流策略](#5-限流策略)
6. [Bot 检测与 DDoS 防护](#6-bot-检测与-ddos-防护)
7. [证书管理](#7-证书管理)
8. [各产品安全能力对比表](#8-各产品安全能力对比表)

---

<!-- chunk: 1. API 网关安全架构概述 -->## 1. API 网关安全架构概述

## 1.1 纵深防御模型

API 网关作为流量入口，是实施纵深防御（Defense in Depth）的核心位置。安全能力从外到内分为多个层次：

```
互联网流量
     │
     ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Layer 1：DDoS / Bot 防护                                            │
│  ├── IP 黑白名单                                                     │
│  ├── 速率限制（IP 维度）                                              │
│  └── Bot 指纹识别 & Challenge                                        │
├─────────────────────────────────────────────────────────────────────┤
│  Layer 2：WAF（Web Application Firewall）                            │
│  ├── OWASP Top 10 规则集（SQLi / XSS / SSRF / RCE）                  │
│  ├── 自定义规则（业务逻辑异常）                                        │
│  └── 请求体深度检测                                                   │
├─────────────────────────────────────────────────────────────────────┤
│  Layer 3：认证（Authentication）                                     │
│  ├── JWT Token 验证                                                  │
│  ├── OIDC / OAuth2 标准流程                                          │
│  ├── mTLS 客户端证书                                                 │
│  ├── API Key / HMAC 签名                                             │
│  └── 多因素认证编排                                                   │
├─────────────────────────────────────────────────────────────────────┤
│  Layer 4：鉴权（Authorization）                                      │
│  ├── RBAC（基于角色的访问控制）                                        │
│  ├── OPA（Open Policy Agent）策略引擎                                │
│  ├── 路由级权限绑定                                                   │
│  └── 资源范围限制（Scope）                                            │
├─────────────────────────────────────────────────────────────────────┤
│  Layer 5：传输加密                                                    │
│  ├── TLS 1.3 终止                                                    │
│  ├── mTLS 双向认证                                                   │
│  └── 证书自动轮转（cert-manager）                                     │
└─────────────────────────────────────────────────────────────────────┘
     │
     ▼
上游微服务（已认证、已授权的请求）
```

## 1.2 零信任在 API 网关的实践

```
传统边界安全模型                    零信任模型
┌─────────────────────┐            ┌─────────────────────────────┐
│   外部（不可信）     │            │  每次请求均需验证身份与权限   │
│   ───────────────   │            │                             │
│   防火墙            │            │  ① 验证调用方身份（JWT/mTLS） │
│   ───────────────   │            │  ② 验证权限（OPA 策略）       │
│   内部（默认可信）   │            │  ③ 验证请求合法性（WAF）      │
│   任意服务互通       │            │  ④ 最小权限原则              │
└─────────────────────┘            │  ⑤ 持续监控异常行为          │
                                   └─────────────────────────────┘
```

---

<!-- chunk: 2. 认证模式 -->## 2. 认证模式

## 2.1 JWT Token 验证

JWT（JSON Web Token）是最常见的无状态认证方案，网关侧负责验签，无需回调认证服务。

**Higress JWT 插件配置**

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: jwt-auth
  namespace: higress-system
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/jwt-auth:latest
  phase: AUTHN
  pluginConfig:
    consumers:
      - name: user-alice
        # RS256 公钥验签（推荐生产使用非对称加密）
        jwks: |
          {
            "keys": [{
              "kty": "RSA",
              "use": "sig",
              "kid": "key-2026",
              "n": "sKqFv8NqA...",
              "e": "AQAB"
            }]
          }
        issuer: "https://auth.example.com"
        audiences: ["api.example.com"]
        token_location: header  # header / query / cookie
        token_header: Authorization
        token_prefix: "Bearer "
    # 全局配置
    clock_skew_seconds: 60
    keep_token: false         # 不向上游转发原始 Token
    strip_bearer_token: true
```

**APISIX JWT 插件配置**

```yaml
# 创建 Consumer
curl -X PUT http://127.0.0.1:9180/apisix/admin/consumers/alice \
  -H 'X-API-KEY: admin-key' \
  -d '{
    "username": "alice",
    "plugins": {
      "jwt-auth": {
        "key": "user-alice",
        "secret": "your-secret-256-bits",
        "algorithm": "HS256",
        "exp": 86400
      }
    }
  }'

# 路由绑定 JWT 插件
curl -X PUT http://127.0.0.1:9180/apisix/admin/routes/1 \
  -H 'X-API-KEY: admin-key' \
  -d '{
    "uri": "/api/v1/*",
    "plugins": {
      "jwt-auth": {
        "hide_credentials": true
      }
    },
    "upstream": {
      "nodes": {"backend:8080": 1}
    }
  }'
```

**JWT 验证流程**

```
客户端                   API 网关                  认证服务（IDP）
   │                        │                           │
   │── GET /api/resource ──►│                           │
   │   Authorization: Bearer│<token>                    │
   │                        │                           │
   │                        │ 1. 解析 JWT Header/Payload │
   │                        │ 2. 验证 exp/nbf/iss/aud   │
   │                        │ 3. 使用本地公钥验证签名    │
   │                        │    （无需远程调用）         │
   │                        │                           │
   │                        │ ✓ 验证通过                 │
   │                        │── 注入 X-Consumer-Name ──►│
   │                        │── 转发请求至上游 ──────────►(upstream)
   │◄─ 200 OK ──────────────│                           │
```

## 2.2 OIDC / OAuth2 标准流程

```
                    OIDC 授权码流程（网关代理模式）
                    
浏览器 / SPA              API 网关              OIDC Provider（如 Keycloak）
    │                       │                          │
    │── GET /app ──────────►│                          │
    │                       │ 检查 Session Cookie       │
    │                       │ ← 未认证                  │
    │◄── 302 redirect ──────│                          │
    │    → IDP /authorize   │                          │
    │                       │                          │
    │─── GET /authorize ────────────────────────────►  │
    │   ?client_id=gw       │                          │
    │   &redirect_uri=...   │                          │
    │   &scope=openid+email │                          │
    │                       │                          │
    │◄── 登录页面 ──────────────────────────────────── │
    │── 提交凭据 ────────────────────────────────────►  │
    │◄── 302 callback?code=xxx ─────────────────────── │
    │                       │                          │
    │── GET /callback?code ►│                          │
    │                       │── POST /token ──────────►│
    │                       │   code + client_secret   │
    │                       │◄─ {access_token, id_token}│
    │                       │   refresh_token           │
    │                       │                          │
    │                       │ 设置加密 Session Cookie   │
    │◄── 302 → /app ────────│                          │
    │── GET /app ──────────►│                          │
    │   Cookie: session=xxx │ 验证 Session             │
    │                       │── 注入用户信息 Header ──►(upstream)
    │◄── 200 OK ────────────│                          │
```

**Higress OIDC 插件配置**

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: oidc-auth
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/oidc:latest
  phase: AUTHN
  pluginConfig:
    provider_url: "https://keycloak.example.com/realms/myrealm"
    client_id: "api-gateway"
    client_secret_ref:
      secretName: oidc-client-secret
      secretKey: client_secret
    redirect_url: "https://api.example.com/oauth2/callback"
    scopes: ["openid", "email", "profile"]
    cookie_name: "_oidc_session"
    cookie_secure: true
    cookie_http_only: true
    session_ttl: 3600
    # 注入用户信息至上游 Header
    set_headers:
      X-User-ID: "sub"
      X-User-Email: "email"
      X-User-Roles: "realm_access.roles"
```

## 2.3 mTLS 客户端证书认证

适用于服务间 API 调用、B2B 集成场景，提供最强的身份绑定保证。

```yaml
# Higress Gateway TLS 配置（启用 mTLS）
apiVersion: networking.istio.io/v1beta1
kind: Gateway
metadata:
  name: mtls-gateway
spec:
  selector:
    higress: higress-system-higress-gateway
  servers:
    - port:
        number: 443
        name: https
        protocol: HTTPS
      tls:
        mode: MUTUAL          # 双向 TLS
        credentialName: gateway-tls-cert
        caCertificates: /etc/ssl/client-ca/ca.crt
        minProtocolVersion: TLSV1_3
      hosts:
        - "api.example.com"

---
# 基于客户端证书 Subject 进行路由
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: mtls-routing
spec:
  hosts:
    - "api.example.com"
  http:
    - matchers:
      - - headers=""
            # 网关提取证书 Subject 注入 Header
            x-forwarded-client-cert:
              regex: ".*CN=partner-a.*"
      route:
        - destination:
            host: partner-a-backend
            port:
              number: 8080
```

## 2.4 API Key 认证

```yaml
# Higress API Key 插件
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: api-key-auth
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/key-auth:latest
  phase: AUTHN
  pluginConfig:
    consumers:
      - name: service-a
        credential: "sk-prod-a1b2c3d4e5f6"
    keys:
      - "x-api-key"      # Header 名称（大小写不敏感）
      - "apikey"         # 备选 Header
    in_query: true       # 也支持 Query 参数 ?apikey=xxx
    hide_credentials: true  # 不向上游透传 Key

# APISIX key-auth 插件
curl -X PUT http://127.0.0.1:9180/apisix/admin/consumers/service-a \
  -d '{
    "username": "service-a",
    "plugins": {
      "key-auth": {
        "key": "sk-prod-a1b2c3d4e5f6"
      }
    }
  }'
```

## 2.5 HMAC 签名认证

HMAC 适用于高安全要求的 API，防止请求被重放或篡改。

```yaml
# HMAC 认证插件配置（APISIX）
plugins:
  hmac-auth:
    access_key: "my-access-key-id"
    secret_key: "my-secret-key"
    signed_headers: ["date", "content-type", "x-custom-header"]
    clock_skew: 300           # 允许 5 分钟时钟偏差
    validate_request_body: true  # 校验请求体 Hash
    encode_uri_params: true
```

**HMAC 签名算法**

```
StringToSign = HTTPMethod + "\n"
             + ContentMD5 + "\n"
             + ContentType + "\n"
             + Date + "\n"
             + CanonicalizedHeaders + "\n"
             + CanonicalizedResource

Signature = Base64(HMAC-SHA256(SecretKey, StringToSign))
Authorization: hmac username="access-key-id", algorithm="hmac-sha256",
               headers="date content-type x-custom-header",
               signature="<Signature>"
```

---

<!-- chunk: 3. 鉴权体系 -->## 3. 鉴权体系

## 3.1 OPA（Open Policy Agent）集成

OPA 提供统一的声明式策略引擎，将鉴权逻辑从应用代码中解耦。

```
鉴权流程
                                          OPA Server
API 网关                                ┌───────────────────┐
  │                                     │                   │
  │── POST /v1/data/authz/allow ───────►│  Rego 策略引擎     │
  │   {                                 │                   │
  │     "input": {                      │  data.authz.allow │
  │       "method": "POST",             │  = true           │
  │       "path": "/api/orders",        │                   │
  │       "user": "alice",              │  ← 策略文件（Rego）│
  │       "roles": ["admin", "seller"], │                   │
  │       "token_claims": {...}         │                   │
  │     }                               │                   │
  │   }                                 │                   │
  │◄── {"result": true} ───────────────│                   │
  │                                     └───────────────────┘
  │
  │── 放行请求至上游
```

**Rego 策略示例**

```rego
# policy/authz.rego
package authz

import future.keywords.if
import future.keywords.in

default allow := false

# 管理员可访问所有路由
allow if {
    "admin" in input.roles
}

# 卖家只能访问 /api/orders 相关接口
allow if {
    "seller" in input.roles
    startswith(input.path, "/api/orders")
    input.method in ["GET", "POST"]
}

# 只读用户只能执行 GET 请求
allow if {
    "readonly" in input.roles
    input.method == "GET"
}

# 禁止访问管理接口（除管理员外）
deny if {
    startswith(input.path, "/api/admin")
    not "admin" in input.roles
}

allow = false if deny
```

## 3.2 路由级细粒度权限绑定

```yaml
# APISIX 路由级 OPA 鉴权配置
routes:
  # 写操作路由：要求 admin 或 editor 角色
  - uri: /api/articles
    methods: [POST, PUT, DELETE]
    plugins:
      jwt-auth: {}
      opa:
        host: "http://opa-service.opa.svc.cluster.local:8181"
        policy: "authz/allow"
        with_body: false
        timeout: 3000
    upstream:
      nodes:
        article-svc:8080: 1

  # 读操作路由：仅需认证
  - uri: /api/articles
    methods: [GET]
    plugins:
      jwt-auth: {}
    upstream:
      nodes:
        article-svc:8080: 1
```

## 3.3 RBAC 模式

```
RBAC 层级结构

  用户 (Subject)
    │
    └── 绑定角色 (Role)
              │
              └── 包含权限 (Permission)
                        │
                        ├── 资源: /api/orders/*
                        ├── 方法: GET, POST
                        └── 条件: 仅访问本人数据

角色示例:
  ┌──────────────┬──────────────────────────────┬──────────────────┐
  │ 角色          │ 允许路径                      │ 允许方法          │
  ├──────────────┼──────────────────────────────┼──────────────────┤
  │ admin        │ /*                            │ ALL              │
  │ developer    │ /api/v1/*, /api/internal/*   │ ALL              │
  │ reader       │ /api/v1/*                     │ GET              │
  │ service-acct │ /api/internal/metrics         │ GET              │
  └──────────────┴──────────────────────────────┴──────────────────┘
```

---

<!-- chunk: 4. WAF 防护 -->## 4. WAF 防护

## 4.1 WAF 在网关中的位置

```
                请求处理流水线（含 WAF）
                
  入站请求
     │
     ▼
 ┌────────────┐    ┌────────────┐    ┌────────────┐    ┌────────────┐
 │  DDoS/IP   │    │    WAF     │    │  认证/鉴权  │    │  业务插件  │
 │  过滤层    │───►│  检测层    │───►│  安全层    │───►│  处理层   │
 │            │    │            │    │            │    │            │
 │ IP黑名单   │    │ OWASP规则  │    │ JWT/mTLS   │    │ 限流/缓存  │
 │ 连接限速   │    │ ModSecurity│    │ OPA鉴权    │    │ 转换/路由  │
 └────────────┘    └────────────┘    └────────────┘    └────────────┘
```

## 4.2 ModSecurity 集成（Higress）

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: modsecurity-waf
  namespace: higress-system
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/modsecurity:latest
  phase: AUTHN     # WAF 在认证之前执行
  priority: 200    # 越高越先执行
  pluginConfig:
    rules:
      # 启用 OWASP CRS（Core Rule Set）
      - "Include @owasp_crs/REQUEST-942-APPLICATION-ATTACK-SQLI.conf"
      - "Include @owasp_crs/REQUEST-941-APPLICATION-ATTACK-XSS.conf"
      - "Include @owasp_crs/REQUEST-931-APPLICATION-ATTACK-RFI.conf"
      - "Include @owasp_crs/REQUEST-932-APPLICATION-ATTACK-RCE.conf"
      - "Include @owasp_crs/REQUEST-944-APPLICATION-ATTACK-JAVA.conf"
    mode: "DETECTION"   # DETECTION（仅检测）or ENFORCEMENT（拦截）
    anomaly_threshold: 5  # 异常评分阈值（超过则拦截）
    custom_rules:
      # 自定义规则：阻止特定 User-Agent
      - |
        SecRule REQUEST_HEADERS:User-Agent "@contains malicious-bot"
        "id:10001,phase:1,deny,status:403,msg:'Blocked bot UA'"
      # 保护 Admin 接口
      - |
        SecRule REQUEST_URI "@beginsWith /api/admin"
        "id:10002,phase:1,chain,deny,status:403"
        SecRule REMOTE_ADDR "!@ipMatch 10.0.0.0/8,172.16.0.0/12"
```

## 4.3 OWASP 核心规则集覆盖

| 规则文件 | 防护类型 | 关键漏洞 |
|---------|---------|---------|
| REQUEST-942 | SQL 注入 | SQLi, 盲注, 时间注入 |
| REQUEST-941 | XSS 跨站脚本 | 反射型 XSS, DOM XSS |
| REQUEST-930 | LFI 本地文件包含 | 路径遍历, ../etc/passwd |
| REQUEST-931 | RFI 远程文件包含 | 远程代码执行入口 |
| REQUEST-932 | 远程代码执行 | Shell 注入, eval 注入 |
| REQUEST-933 | PHP 注入 | PHP 代码执行 |
| REQUEST-934 | Node.js 攻击 | prototype pollution |
| REQUEST-944 | Java 攻击 | Log4Shell, Struts |
| REQUEST-913 | 扫描器检测 | Nikto, sqlmap 特征 |

## 4.4 各产品 WAF 插件对比

| 产品 | WAF 实现 | 规则集 | 检测模式 | 性能影响 |
|------|---------|-------|---------|---------|
| **Higress** | ModSecurity Wasm | OWASP CRS 3.3+ | 检测/拦截 | ~3ms/请求 |
| **APISIX** | 内置 WAF 插件 + 自定义规则 | 部分 OWASP | 拦截 | ~1ms/请求 |
| **Kong** | Kong WAF（Enterprise）| OWASP CRS | 检测/拦截 | ~2ms/请求 |
| **Traefik** | 无原生 WAF，依赖 ModSecurity 中间件 | 手动配置 | 拦截 | ~5ms/请求 |
| **[[Envoy|Envoy]] Gateway** | ext_proc 调用外部 WAF | 取决于外部服务 | 可配置 | 取决于网络 |

---

<!-- chunk: 5. 限流策略 -->## 5. 限流策略

## 5.1 限流算法对比

```
令牌桶算法（Token Bucket）          滑动窗口算法（Sliding Window）
┌─────────────────────────┐         ┌─────────────────────────────┐
│                         │         │                             │
│  桶容量: 100 令牌        │         │  窗口大小: 60s              │
│  补充速率: 10/s          │         │  最大请求: 600              │
│                         │         │                             │
│  t=0: 桶满(100令牌)      │         │  精确统计过去 60s 内请求数   │
│  t=1: 突发30请求，       │         │  无突发容忍，更公平          │
│       消耗30令牌         │         │                             │
│  t=2: 补充10令牌         │         │  实现：Redis ZSET            │
│  允许突发流量            │         │  ZADD key timestamp req_id  │
│                         │         │  ZCOUNT key now-60s now     │
└─────────────────────────┘         └─────────────────────────────┘

固定窗口算法（Fixed Window）
┌─────────────────────────┐
│                         │
│  窗口大小: 60s           │
│  最大请求: 600           │
│                         │
│  实现简单，但存在          │
│  窗口边界突发问题          │
│  (两个窗口交界处可达       │
│   2倍限制)               │
└─────────────────────────┘
```

## 5.2 本地限流 vs 分布式限流

```
本地限流（适合单实例或精度要求不高）

  Gateway Pod-1     Gateway Pod-2     Gateway Pod-3
  ┌──────────┐      ┌──────────┐      ┌──────────┐
  │ limit:   │      │ limit:   │      │ limit:   │
  │ 100 req/s│      │ 100 req/s│      │ 100 req/s│
  │ (内存计数)│      │ (内存计数)│      │ (内存计数)│
  └──────────┘      └──────────┘      └──────────┘
  实际整体限制 = 100 * 3 = 300 req/s（不精确）

分布式限流（适合精确控制）

  Gateway Pod-1     Gateway Pod-2     Gateway Pod-3
  ┌──────────┐      ┌──────────┐      ┌──────────┐
  │          │      │          │      │          │
  │  限流插件 │      │  限流插件 │      │  限流插件 │
  │          │      │          │      │          │
  └────┬─────┘      └────┬─────┘      └────┬─────┘
       │                 │                 │
       └─────────────────┴─────────────────┘
                         │
                    ┌────▼────┐
                    │  Redis  │
                    │ Cluster │
                    │         │
                    │ INCR key│
                    │ EXPIRE  │
                    └─────────┘
  实际整体限制 = 精确 100 req/s
```

## 5.3 Higress 多维度限流配置

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: rate-limit
  namespace: higress-system
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/rate-limit:latest
  pluginConfig:
    # 全局限流
    global:
      qps: 10000
      burst: 5000

    # 按路由限流
    rule_name: "api-rate-limit"
    rule_items:
      # 按消费者（已认证用户）限流
      - limit_by_per_consumer: true
        limit_keys:
          - key: "basic-user"
            token_per_second: 50
            token_per_minute: 1000
          - key: "premium-user"
            token_per_second: 500
            token_per_minute: 10000

      # 按 IP 限流（防 IP 滥用）
      - limit_by_per_ip: true
        limit_keys:
          - key: ".*"    # 所有 IP
            token_per_second: 20
            token_per_minute: 500

      # 按请求 Header 限流（如按 App-ID）
      - limit_by_header: "X-App-ID"
        limit_keys:
          - key: "app-free-tier"
            token_per_second: 10
          - key: "app-enterprise"
            token_per_second: 2000

    # 分布式限流（Redis 后端）
    redis:
      service_name: "redis.redis.svc.cluster.local"
      service_port: 6379
      username: "redis-user"
      password_ref:
        secretName: redis-secret
        key: password
      timeout: 50  # ms
```

## 5.4 APISIX 限流插件对比

```yaml
# limit-count：固定窗口，分布式支持
plugins:
  limit-count:
    count: 100
    time_window: 60
    rejected_code: 429
    key_type: "var"
    key: "remote_addr"   # 按 IP 限流
    policy: "redis"      # local | redis | redis-cluster
    redis_host: "redis.svc"
    redis_port: 6379

---
# limit-req：令牌桶，平滑限流
plugins:
  limit-req:
    rate: 50       # 允许速率（req/s）
    burst: 100     # 突发容量
    key: "remote_addr"
    rejected_code: 429

---
# limit-conn：并发连接数限制
plugins:
  limit-conn:
    conn: 200       # 最大并发连接数
    burst: 50
    default_conn_delay: 0.1
    key: "remote_addr"
```

---

<!-- chunk: 6. Bot 检测与 DDoS 防护 -->## 6. Bot 检测与 DDoS 防护

## 6.1 Bot 指纹识别

```
Bot 检测信号矩阵

  HTTP 特征:
  ├── User-Agent 匹配（爬虫库特征：python-requests, curl, Java/1.8）
  ├── 缺少正常浏览器 Header（Accept-Language, Accept-Encoding）
  ├── 请求频率异常（单 IP 短时高频）
  └── Header 顺序异常（浏览器有固定顺序）

  行为特征:
  ├── 访问路径规律（顺序扫描 /api/v1/user/1, /2, /3...）
  ├── 无 JS 执行能力（无法完成 Challenge）
  ├── Cookie 未携带（Session 为空）
  └── TLS 指纹异常（JA3 指纹与 UA 不匹配）
```

## 6.2 Challenge 机制（Higress）

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: bot-detect
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/bot-detect:latest
  pluginConfig:
    # 可疑行为触发阈值
    suspicious_threshold: 5
    # Challenge 类型
    challenge_type: "js"   # js（JS 计算）/ captcha（图形验证码）
    # IP 黑名单（自动更新）
    ip_blacklist_refresh_interval: 300
    # 已知 Bot UA 列表
    blocked_ua_patterns:
      - "python-requests"
      - "Go-http-client/1.1"
      - "curl/[0-9]"
      - "Scrapy"
    # 白名单（监控 Bot 放行）
    allowed_bots:
      - "Googlebot"
      - "Bingbot"
```

## 6.3 DDoS 防护架构

```
DDoS 防护层次

  云服务商 DDoS 清洗（外层）
  └── 流量清洗中心（T级清洗）
       │
       ▼
  API 网关入口层（中层）
  ├── IP 速率限制：单 IP 100 req/s
  ├── 连接数限制：单 IP 最大 100 并发
  ├── SYN Flood 防护（内核层）
  └── 带宽限速（超 10Mbps/IP 触发）
       │
       ▼
  应用层防护（内层）
  ├── WAF 拦截攻击载荷
  ├── Bot 检测 + Challenge
  └── 限流熔断（保护上游）
```

---

<!-- chunk: 7. 证书管理 -->## 7. 证书管理

## 7.1 cert-manager 集成

```yaml
# 安装 cert-manager
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/latest/download/cert-manager.yaml

# 创建 ClusterIssuer（Let's Encrypt 生产环境）
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: ops@example.com
    privateKeySecretRef:
      name: letsencrypt-prod-key
    solvers:
      # HTTP-01 Challenge（适合有公网 IP 的集群）
      - http01:
          ingress:
            class: higress
      # DNS-01 Challenge（适合内网或通配符证书）
      - dns01:
          route53:
            region: us-east-1
            hostedZoneID: Z1234ABCD
            accessKeyIDSecretRef:
              name: route53-credentials
              key: access-key-id
```

## 7.2 自动证书申请与轮转

```yaml
# Gateway API：自动申请证书
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: prod-gateway
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
spec:
  gatewayClassName: higress
  listeners:
    - name: https
      port: 443
      protocol: HTTPS
      hostname: "api.example.com"
      tls:
        mode: Terminate
        certificateRefs:
          - name: api-example-com-tls
            kind: Secret

---
# cert-manager 自动管理的 Certificate 对象
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: api-example-com-tls
  namespace: higress-system
spec:
  secretName: api-example-com-tls
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
    - api.example.com
    - "*.api.example.com"   # 通配符证书
  renewBefore: 360h         # 到期前 15 天自动续签
  duration: 2160h           # 证书有效期 90 天
```

## 7.3 证书轮转零停机流程

```
时间轴
  t=-30d │  cert-manager 检测到证书将于 30 天后到期
         │
  t=-15d │  自动触发续签请求（renewBefore: 360h）
         │  ├── ACME Challenge 验证域名所有权
         │  └── 获取新证书，存入新 Secret
         │
  t=-14d │  新证书存入 Kubernetes Secret（新版本）
         │  网关控制平面检测到 Secret 更新
         │
  t=-13d │  网关 Worker 热加载新证书
         │  ├── 新 TLS 连接使用新证书
         │  └── 存量连接继续使用旧证书直到断开
         │
  t=0    │  旧证书到期，所有连接已切换到新证书
         │  零停机完成证书轮转
```

---

<!-- chunk: 8. 各产品安全能力对比表 -->## 8. 各产品安全能力对比表

## 8.1 认证能力矩阵

| 认证方式 | Higress | APISIX | Kong | Envoy Gateway | Traefik |
|---------|---------|-------|------|--------------|---------|
| **JWT 验证** | ⭐⭐⭐⭐⭐ 内置 Wasm 插件，支持 RS256/ES256 | ⭐⭐⭐⭐⭐ 内置插件，Consumer 体系完善 | ⭐⭐⭐⭐⭐ 内置 | ⭐⭐⭐⭐ Ext-authz + JWT Filter | ⭐⭐⭐ 中间件支持 |
| **OIDC** | ⭐⭐⭐⭐ Wasm 插件 | ⭐⭐⭐⭐ openid-connect 插件 | ⭐⭐⭐⭐⭐ Enterprise 内置 | ⭐⭐⭐ 需外部鉴权服务 | ⭐⭐⭐ Forward Auth |
| **mTLS** | ⭐⭐⭐⭐⭐ Gateway API 原生支持 | ⭐⭐⭐⭐ 支持 | ⭐⭐⭐⭐ 支持 | ⭐⭐⭐⭐⭐ Envoy 原生强项 | ⭐⭐⭐⭐ 支持 |
| **API Key** | ⭐⭐⭐⭐⭐ Wasm 插件 | ⭐⭐⭐⭐⭐ 内置 | ⭐⭐⭐⭐⭐ 内置 | ⭐⭐⭐ 通过 Ext-authz | ⭐⭐⭐ 中间件 |
| **HMAC** | ⭐⭐⭐ Wasm 插件 | ⭐⭐⭐⭐⭐ 内置 hmac-auth | ⭐⭐⭐⭐ 内置 | ⭐⭐ 自定义实现 | ⭐⭐ 自定义 |

## 8.2 鉴权与 WAF 矩阵

| 能力 | Higress | APISIX | Kong | Envoy Gateway | Traefik |
|------|---------|-------|------|--------------|---------|
| **OPA 集成** | ⭐⭐⭐⭐ Wasm 插件 | ⭐⭐⭐⭐⭐ 原生 opa 插件 | ⭐⭐⭐⭐ 插件支持 | ⭐⭐⭐⭐⭐ Ext-authz 原生 | ⭐⭐ 需外部 |
| **WAF（ModSecurity）** | ⭐⭐⭐⭐⭐ Wasm 插件，OWASP CRS | ⭐⭐⭐ 插件支持 | ⭐⭐⭐⭐ Enterprise WAF | ⭐⭐⭐ 通过 Ext-proc | ⭐⭐ 外部中间件 |
| **IP 黑白名单** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **分布式限流** | ⭐⭐⭐⭐ Redis 后端 | ⭐⭐⭐⭐⭐ Redis/etcd | ⭐⭐⭐⭐⭐ Redis | ⭐⭐⭐ 需 Ext-proc | ⭐⭐⭐ Redis 中间件 |
| **Bot 检测** | ⭐⭐⭐⭐ 内置插件 | ⭐⭐⭐ 有限支持 | ⭐⭐⭐⭐ Enterprise | ⭐⭐ 需外部 | ⭐⭐ 需外部 |
| **cert-manager** | ⭐⭐⭐⭐⭐ Gateway API 原生 | ⭐⭐⭐⭐ 支持 | ⭐⭐⭐⭐ 支持 | ⭐⭐⭐⭐⭐ 原生支持 | ⭐⭐⭐⭐⭐ 原生支持 |

---

<!-- chunk: 参考资料 -->## 参考资料

- [OWASP ModSecurity Core Rule Set](https://coreruleset.org/)
- [Open Policy Agent 官方文档](https://www.openpolicyagent.org/docs/latest/)
- [proxy-wasm JWT 插件（Higress）](https://higress.io/docs/latest/plugins/authentication/jwt-auth/)
- [APISIX 安全插件列表](https://apisix.apache.org/docs/apisix/plugins/jwt-auth/)
- [cert-manager 官方文档](https://cert-manager.io/docs/)
- [OIDC 规范（OpenID Foundation）](https://openid.net/connect/)
- 关联文档：[domain-05-security-compliance 云原生安全](../domain-05-security-compliance/)
- 关联文档：[domain-05-security-compliance 安全基础](../domain-05-security-compliance/)
- 关联文档：[01 - API 网关架构总览](./01-api-gateway-architecture-overview.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[domain-03-networking-traffic/README.md|Domain 03: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
- Domain-40 云原生 API 网关 — 开源项目索引
- 01 - 云原生 API 网关架构总览
- 02 - Kubernetes Gateway API 标准深度解析
- 03 - API 网关选型指南与对比矩阵
- 04 - Higress 云原生 API 网关企业级实践
- 05 - Apache APISIX 企业级 API 网关实践
- 06 - Kong API 网关企业级实践
- 07 - Envoy Gateway 企业级实践
- 08 - Traefik API 网关企业级实践
- 09 - 传统 Ingress 控制器向云原生 API 网关迁移

## See Also

- 09-nginx-ingress-migration-guide
- 10-wasm-plugin-ecosystem
- 12-api-gateway-observability
- 13-api-gateway-performance-benchmarks


<!-- risk-assessed -->
