# 129 - NGINX Ingress 完整配置指南

> **适用版本**: NGINX Ingress Controller v1.8 - v1.10 | Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01

---

## 一、NGINX Ingress 注解完整参考

### 1.1 代理行为注解 (Proxy Behavior)

| 注解 | 类型 | 默认值 | 说明 | 示例 |
|-----|------|-------|------|------|
| `nginx.ingress.kubernetes.io/proxy-body-size` | string | 1m | 请求体最大大小 | `100m` |
| `nginx.ingress.kubernetes.io/proxy-connect-timeout` | number | 5 | 后端连接超时 (秒) | `10` |
| `nginx.ingress.kubernetes.io/proxy-read-timeout` | number | 60 | 后端读取超时 (秒) | `300` |
| `nginx.ingress.kubernetes.io/proxy-send-timeout` | number | 60 | 后端发送超时 (秒) | `300` |
| `nginx.ingress.kubernetes.io/proxy-next-upstream` | string | error timeout | 重试条件 | `error timeout http_503` |
| `nginx.ingress.kubernetes.io/proxy-next-upstream-timeout` | number | 0 | 重试总超时 (秒) | `30` |
| `nginx.ingress.kubernetes.io/proxy-next-upstream-tries` | number | 3 | 最大重试次数 | `5` |
| `nginx.ingress.kubernetes.io/proxy-request-buffering` | string | on | 请求缓冲 | `off` |
| `nginx.ingress.kubernetes.io/proxy-buffering` | string | on | 响应缓冲 | `off` |
| `nginx.ingress.kubernetes.io/proxy-buffer-size` | string | 4k | 代理缓冲区大小 | `16k` |
| `nginx.ingress.kubernetes.io/proxy-buffers-number` | number | 4 | 代理缓冲区数量 | `8` |
| `nginx.ingress.kubernetes.io/proxy-max-temp-file-size` | string | 1024m | 临时文件最大大小 | `0` |
| `nginx.ingress.kubernetes.io/proxy-http-version` | string | 1.1 | 后端 HTTP 版本 | `1.0` |
| `nginx.ingress.kubernetes.io/proxy-cookie-domain` | string | - | Cookie 域重写 | `localhost example.com` |
| `nginx.ingress.kubernetes.io/proxy-cookie-path` | string | - | Cookie 路径重写 | `/one/ /` |

### 1.2 URL 重写注解 (URL Rewriting)

| 注解 | 类型 | 默认值 | 说明 | 示例 |
|-----|------|-------|------|------|
| `nginx.ingress.kubernetes.io/rewrite-target` | string | - | URL 重写目标 | `/$2` |
| `nginx.ingress.kubernetes.io/use-regex` | bool | false | 启用正则表达式 | `true` |
| `nginx.ingress.kubernetes.io/app-root` | string | - | 应用根目录重定向 | `/app1` |
| `nginx.ingress.kubernetes.io/configuration-snippet` | string | - | 自定义 NGINX 配置 | 见下文 |
| `nginx.ingress.kubernetes.io/server-snippet` | string | - | server 块自定义配置 | 见下文 |

#### URL 重写实操案例

```yaml
# 案例1: 去除路径前缀
# /api/v1/users -> /v1/users
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: rewrite-strip-prefix
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /$2
    nginx.ingress.kubernetes.io/use-regex: "true"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /api(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: api-service
            port:
              number: 8080
---
# 案例2: 添加路径前缀
# /users -> /api/v1/users
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: rewrite-add-prefix
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /api/v1$uri
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
---
# 案例3: 复杂路径重写
# /old/path/xxx -> /new/path/xxx
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: rewrite-complex
  annotations:
    nginx.ingress.kubernetes.io/use-regex: "true"
    nginx.ingress.kubernetes.io/rewrite-target: /new/path/$1
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /old/path/(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: app-service
            port:
              number: 80
```

### 1.3 SSL/TLS 注解 (SSL/TLS)

| 注解 | 类型 | 默认值 | 说明 | 示例 |
|-----|------|-------|------|------|
| `nginx.ingress.kubernetes.io/ssl-redirect` | bool | true | HTTP 重定向到 HTTPS | `false` |
| `nginx.ingress.kubernetes.io/force-ssl-redirect` | bool | false | 强制 HTTPS (即使终止) | `true` |
| `nginx.ingress.kubernetes.io/ssl-passthrough` | bool | false | TLS 透传 | `true` |
| `nginx.ingress.kubernetes.io/backend-protocol` | string | HTTP | 后端协议 | `HTTPS` / `GRPC` / `GRPCS` |
| `nginx.ingress.kubernetes.io/ssl-ciphers` | string | - | SSL 加密套件 | `ECDHE-RSA-AES128-GCM-SHA256` |
| `nginx.ingress.kubernetes.io/ssl-prefer-server-ciphers` | bool | true | 优先服务器加密套件 | `false` |
| `nginx.ingress.kubernetes.io/auth-tls-secret` | string | - | 客户端 CA 证书 Secret | `namespace/secret` |
| `nginx.ingress.kubernetes.io/auth-tls-verify-client` | string | - | 客户端证书验证 | `on` / `optional` |
| `nginx.ingress.kubernetes.io/auth-tls-verify-depth` | number | 1 | 证书链验证深度 | `2` |
| `nginx.ingress.kubernetes.io/auth-tls-pass-certificate-to-upstream` | bool | false | 传递证书到后端 | `true` |
| `nginx.ingress.kubernetes.io/proxy-ssl-secret` | string | - | 后端 TLS 证书 | `namespace/secret` |
| `nginx.ingress.kubernetes.io/proxy-ssl-verify` | string | off | 验证后端证书 | `on` |
| `nginx.ingress.kubernetes.io/proxy-ssl-verify-depth` | number | 1 | 后端证书验证深度 | `2` |
| `nginx.ingress.kubernetes.io/proxy-ssl-name` | string | - | 后端 SNI 名称 | `backend.internal` |
| `nginx.ingress.kubernetes.io/proxy-ssl-server-name` | string | off | 启用 SNI | `on` |

### 1.4 认证注解 (Authentication)

| 注解 | 类型 | 默认值 | 说明 | 示例 |
|-----|------|-------|------|------|
| `nginx.ingress.kubernetes.io/auth-type` | string | - | 认证类型 | `basic` / `digest` |
| `nginx.ingress.kubernetes.io/auth-secret` | string | - | 认证凭证 Secret | `basic-auth-secret` |
| `nginx.ingress.kubernetes.io/auth-secret-type` | string | auth-file | Secret 类型 | `auth-map` |
| `nginx.ingress.kubernetes.io/auth-realm` | string | Authentication Required | 认证域 | `Admin Area` |
| `nginx.ingress.kubernetes.io/auth-url` | string | - | 外部认证 URL | `http://auth-svc/verify` |
| `nginx.ingress.kubernetes.io/auth-method` | string | - | 外部认证方法 | `GET` / `POST` |
| `nginx.ingress.kubernetes.io/auth-signin` | string | - | 登录页面 URL | `http://auth.example.com/login` |
| `nginx.ingress.kubernetes.io/auth-signin-redirect-param` | string | rd | 重定向参数名 | `redirect_to` |
| `nginx.ingress.kubernetes.io/auth-response-headers` | string | - | 传递的响应头 | `X-User,X-Token` |
| `nginx.ingress.kubernetes.io/auth-request-redirect` | string | - | 认证请求重定向 | `$scheme://$host$request_uri` |
| `nginx.ingress.kubernetes.io/auth-cache-key` | string | - | 认证缓存键 | `$remote_user$http_x_token` |
| `nginx.ingress.kubernetes.io/auth-cache-duration` | string | 200 202 401 5m | 缓存时长 | `200 10m` |
| `nginx.ingress.kubernetes.io/auth-always-set-cookie` | bool | false | 始终设置 Cookie | `true` |

#### 认证配置实操案例

```yaml
# 案例1: Basic Auth 认证
# 1. 创建密码文件
# htpasswd -c auth admin
# kubectl create secret generic basic-auth --from-file=auth

apiVersion: v1
kind: Secret
metadata:
  name: basic-auth
  namespace: default
type: Opaque
data:
  # admin:$apr1$xxx (htpasswd 生成)
  auth: YWRtaW46JGFwcjEkSDBHWS5CN3IkVnlDV2ZYREJSUGhQMXY3SGNPV0FlLgo=
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: basic-auth-ingress
  annotations:
    nginx.ingress.kubernetes.io/auth-type: basic
    nginx.ingress.kubernetes.io/auth-secret: basic-auth
    nginx.ingress.kubernetes.io/auth-realm: "Admin Area - Authentication Required"
spec:
  ingressClassName: nginx
  rules:
  - host: admin.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: admin-service
            port:
              number: 80
---
# 案例2: 外部认证服务 (OAuth2 Proxy / Authelia)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: external-auth-ingress
  annotations:
    # 外部认证 URL
    nginx.ingress.kubernetes.io/auth-url: "http://oauth2-proxy.auth-system.svc.cluster.local/oauth2/auth"
    # 登录页面
    nginx.ingress.kubernetes.io/auth-signin: "https://auth.example.com/oauth2/start?rd=$scheme://$host$request_uri"
    # 传递用户信息头
    nginx.ingress.kubernetes.io/auth-response-headers: "X-Auth-Request-User,X-Auth-Request-Email,X-Auth-Request-Groups"
    # 缓存认证结果
    nginx.ingress.kubernetes.io/auth-cache-key: "$cookie_oauth2_proxy"
    nginx.ingress.kubernetes.io/auth-cache-duration: "200 202 10m, 401 5m"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: protected-app
            port:
              number: 80
```

### 1.5 限流注解 (Rate Limiting)

| 注解 | 类型 | 默认值 | 说明 | 示例 |
|-----|------|-------|------|------|
| `nginx.ingress.kubernetes.io/limit-rps` | number | - | 每秒请求数限制 | `100` |
| `nginx.ingress.kubernetes.io/limit-rpm` | number | - | 每分钟请求数限制 | `1000` |
| `nginx.ingress.kubernetes.io/limit-connections` | number | - | 并发连接数限制 | `10` |
| `nginx.ingress.kubernetes.io/limit-rate` | number | - | 响应速率限制 (KB/s) | `100` |
| `nginx.ingress.kubernetes.io/limit-rate-after` | number | - | 限速前的数据量 (KB) | `1024` |
| `nginx.ingress.kubernetes.io/limit-whitelist` | string | - | 限流白名单 CIDR | `10.0.0.0/8,192.168.0.0/16` |
| `nginx.ingress.kubernetes.io/global-rate-limit` | number | - | 全局限流 RPS | `1000` |
| `nginx.ingress.kubernetes.io/global-rate-limit-window` | string | 1s | 全局限流窗口 | `1m` |
| `nginx.ingress.kubernetes.io/global-rate-limit-key` | string | $remote_addr | 限流键 | `$http_x_api_key` |
| `nginx.ingress.kubernetes.io/global-rate-limit-ignored-cidrs` | string | - | 忽略的 CIDR | `10.0.0.0/8` |

#### 限流配置实操案例

```yaml
# 案例1: 基础限流
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: rate-limited-ingress
  annotations:
    # 每秒最多 100 个请求
    nginx.ingress.kubernetes.io/limit-rps: "100"
    # 最多 20 个并发连接
    nginx.ingress.kubernetes.io/limit-connections: "20"
    # 内部 IP 白名单不限流
    nginx.ingress.kubernetes.io/limit-whitelist: "10.0.0.0/8,172.16.0.0/12"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
---
# 案例2: 分级限流 (不同路径不同限制)
# API 路径严格限流
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-rate-limit
  annotations:
    nginx.ingress.kubernetes.io/limit-rps: "50"
    nginx.ingress.kubernetes.io/limit-connections: "10"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
---
# 静态资源宽松限流
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: static-rate-limit
  annotations:
    nginx.ingress.kubernetes.io/limit-rps: "500"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /static
        pathType: Prefix
        backend:
          service:
            name: static-service
            port:
              number: 80
---
# 案例3: 基于 API Key 的限流
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-key-rate-limit
  annotations:
    # 使用 API Key 作为限流键
    nginx.ingress.kubernetes.io/global-rate-limit: "1000"
    nginx.ingress.kubernetes.io/global-rate-limit-window: "1m"
    nginx.ingress.kubernetes.io/global-rate-limit-key: "$http_x_api_key"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

### 1.6 金丝雀/灰度发布注解 (Canary Deployment)

| 注解 | 类型 | 默认值 | 说明 | 示例 |
|-----|------|-------|------|------|
| `nginx.ingress.kubernetes.io/canary` | bool | false | 启用金丝雀 | `true` |
| `nginx.ingress.kubernetes.io/canary-weight` | number | 0 | 流量权重 (0-100) | `10` |
| `nginx.ingress.kubernetes.io/canary-weight-total` | number | 100 | 权重总数 | `100` |
| `nginx.ingress.kubernetes.io/canary-by-header` | string | - | 基于 Header 的金丝雀 | `X-Canary` |
| `nginx.ingress.kubernetes.io/canary-by-header-value` | string | - | Header 匹配值 | `true` |
| `nginx.ingress.kubernetes.io/canary-by-header-pattern` | string | - | Header 正则匹配 | `.*test.*` |
| `nginx.ingress.kubernetes.io/canary-by-cookie` | string | - | 基于 Cookie 的金丝雀 | `canary` |

#### 金丝雀发布实操案例

```yaml
# 案例1: 基于权重的金丝雀发布
# 稳定版本 Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-stable
  namespace: production
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v1
            port:
              number: 80
---
# 金丝雀版本 Ingress (10% 流量)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
---
# 案例2: 基于 Header 的金丝雀 (内部测试)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary-header
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-by-header: "X-Canary"
    nginx.ingress.kubernetes.io/canary-by-header-value: "true"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
---
# 案例3: 基于 Cookie 的金丝雀 (用户分组)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary-cookie
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-by-cookie: "canary"
    # cookie 值为 "always" 时路由到金丝雀版本
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
---
# 案例4: 组合条件金丝雀 (Header 优先级最高)
# 优先级: canary-by-header > canary-by-cookie > canary-weight
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary-combined
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    # Header 匹配时 100% 路由到金丝雀
    nginx.ingress.kubernetes.io/canary-by-header: "X-Test-User"
    nginx.ingress.kubernetes.io/canary-by-header-pattern: ".*@testing.com$"
    # 其他用户按 5% 权重
    nginx.ingress.kubernetes.io/canary-weight: "5"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
```

### 1.7 跨域 CORS 注解 (Cross-Origin)

| 注解 | 类型 | 默认值 | 说明 | 示例 |
|-----|------|-------|------|------|
| `nginx.ingress.kubernetes.io/enable-cors` | bool | false | 启用 CORS | `true` |
| `nginx.ingress.kubernetes.io/cors-allow-origin` | string | * | 允许的源 | `https://example.com` |
| `nginx.ingress.kubernetes.io/cors-allow-methods` | string | GET, PUT, POST, DELETE, PATCH, OPTIONS | 允许的方法 | `GET,POST,PUT` |
| `nginx.ingress.kubernetes.io/cors-allow-headers` | string | DNT,Keep-Alive,... | 允许的请求头 | `Authorization,Content-Type` |
| `nginx.ingress.kubernetes.io/cors-expose-headers` | string | - | 暴露的响应头 | `X-Custom-Header` |
| `nginx.ingress.kubernetes.io/cors-allow-credentials` | bool | true | 允许凭证 | `false` |
| `nginx.ingress.kubernetes.io/cors-max-age` | number | 1728000 | 预检缓存时间 (秒) | `86400` |

#### CORS 配置实操案例

```yaml
# 完整 CORS 配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: cors-ingress
  annotations:
    nginx.ingress.kubernetes.io/enable-cors: "true"
    # 允许多个域名
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://app.example.com,https://admin.example.com"
    # 允许的 HTTP 方法
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, PATCH, OPTIONS"
    # 允许的请求头
    nginx.ingress.kubernetes.io/cors-allow-headers: "DNT,Keep-Alive,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range,Authorization,X-API-Key"
    # 暴露的响应头
    nginx.ingress.kubernetes.io/cors-expose-headers: "Content-Length,Content-Range,X-Request-ID"
    # 允许携带凭证
    nginx.ingress.kubernetes.io/cors-allow-credentials: "true"
    # 预检请求缓存 24 小时
    nginx.ingress.kubernetes.io/cors-max-age: "86400"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

### 1.8 会话亲和注解 (Session Affinity)

| 注解 | 类型 | 默认值 | 说明 | 示例 |
|-----|------|-------|------|------|
| `nginx.ingress.kubernetes.io/affinity` | string | - | 亲和类型 | `cookie` |
| `nginx.ingress.kubernetes.io/affinity-mode` | string | balanced | 亲和模式 | `persistent` |
| `nginx.ingress.kubernetes.io/affinity-canary-behavior` | string | sticky | 金丝雀亲和行为 | `legacy` |
| `nginx.ingress.kubernetes.io/session-cookie-name` | string | INGRESSCOOKIE | Cookie 名称 | `SERVERID` |
| `nginx.ingress.kubernetes.io/session-cookie-path` | string | / | Cookie 路径 | `/app` |
| `nginx.ingress.kubernetes.io/session-cookie-max-age` | number | - | Cookie 最大存活时间 | `3600` |
| `nginx.ingress.kubernetes.io/session-cookie-expires` | number | - | Cookie 过期时间 | `3600` |
| `nginx.ingress.kubernetes.io/session-cookie-change-on-failure` | bool | false | 失败时更换 Cookie | `true` |
| `nginx.ingress.kubernetes.io/session-cookie-samesite` | string | - | SameSite 属性 | `Strict` / `Lax` / `None` |
| `nginx.ingress.kubernetes.io/session-cookie-conditional-samesite-none` | bool | false | 条件 SameSite=None | `true` |

#### 会话亲和实操案例

```yaml
# 基于 Cookie 的会话亲和
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: sticky-session-ingress
  annotations:
    nginx.ingress.kubernetes.io/affinity: "cookie"
    nginx.ingress.kubernetes.io/affinity-mode: "persistent"
    nginx.ingress.kubernetes.io/session-cookie-name: "SERVERID"
    nginx.ingress.kubernetes.io/session-cookie-max-age: "3600"
    nginx.ingress.kubernetes.io/session-cookie-path: "/"
    nginx.ingress.kubernetes.io/session-cookie-samesite: "Strict"
    nginx.ingress.kubernetes.io/session-cookie-change-on-failure: "true"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: stateful-app
            port:
              number: 80
```

### 1.9 负载均衡注解 (Load Balancing)

| 注解 | 类型 | 默认值 | 说明 | 示例 |
|-----|------|-------|------|------|
| `nginx.ingress.kubernetes.io/load-balance` | string | round_robin | 负载均衡算法 | `ewma` / `least_conn` |
| `nginx.ingress.kubernetes.io/upstream-hash-by` | string | - | 一致性哈希键 | `$request_uri` |
| `nginx.ingress.kubernetes.io/upstream-hash-by-subset` | bool | false | 子集哈希 | `true` |
| `nginx.ingress.kubernetes.io/upstream-hash-by-subset-size` | number | 3 | 子集大小 | `5` |
| `nginx.ingress.kubernetes.io/upstream-vhost` | string | - | 上游虚拟主机 | `backend.local` |

### 1.10 自定义配置注解 (Custom Configuration)

| 注解 | 类型 | 说明 | 作用域 |
|-----|------|------|-------|
| `nginx.ingress.kubernetes.io/configuration-snippet` | string | location 块自定义配置 | location |
| `nginx.ingress.kubernetes.io/server-snippet` | string | server 块自定义配置 | server |
| `nginx.ingress.kubernetes.io/stream-snippet` | string | stream 块自定义配置 | stream |
| `nginx.ingress.kubernetes.io/modsecurity-snippet` | string | ModSecurity 自定义规则 | location |

#### 自定义配置实操案例

```yaml
# 案例1: 自定义响应头
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: custom-headers-ingress
  annotations:
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # 安全响应头
      add_header X-Frame-Options "SAMEORIGIN" always;
      add_header X-Content-Type-Options "nosniff" always;
      add_header X-XSS-Protection "1; mode=block" always;
      add_header Referrer-Policy "strict-origin-when-cross-origin" always;
      add_header Content-Security-Policy "default-src 'self'; script-src 'self' 'unsafe-inline'" always;
      
      # 自定义业务头
      add_header X-Request-ID $request_id always;
      add_header X-Backend-Server $upstream_addr always;
spec:
  ingressClassName: nginx
  rules:
  - host: secure.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-service
            port:
              number: 80
---
# 案例2: 自定义缓存配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: caching-ingress
  annotations:
    nginx.ingress.kubernetes.io/server-snippet: |
      # 开启代理缓存
      proxy_cache_path /tmp/nginx-cache levels=1:2 keys_zone=app_cache:10m max_size=1g inactive=60m use_temp_path=off;
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # 静态资源缓存
      location ~* \.(jpg|jpeg|png|gif|ico|css|js|woff2?)$ {
        proxy_cache app_cache;
        proxy_cache_valid 200 7d;
        proxy_cache_valid 404 1m;
        add_header X-Cache-Status $upstream_cache_status;
        proxy_pass http://upstream_balancer;
      }
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
---
# 案例3: 自定义访问控制
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: access-control-ingress
  annotations:
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # IP 访问控制
      allow 10.0.0.0/8;
      allow 192.168.0.0/16;
      deny all;
      
      # 特定 User-Agent 拦截
      if ($http_user_agent ~* (bot|crawler|spider)) {
        return 403;
      }
spec:
  ingressClassName: nginx
  rules:
  - host: internal.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: internal-service
            port:
              number: 80
```

### 1.11 其他常用注解

| 注解 | 类型 | 默认值 | 说明 | 示例 |
|-----|------|-------|------|------|
| `nginx.ingress.kubernetes.io/whitelist-source-range` | string | - | 源 IP 白名单 | `10.0.0.0/8,172.16.0.0/12` |
| `nginx.ingress.kubernetes.io/denylist-source-range` | string | - | 源 IP 黑名单 | `1.2.3.4/32` |
| `nginx.ingress.kubernetes.io/permanent-redirect` | string | - | 永久重定向 URL | `https://new.example.com` |
| `nginx.ingress.kubernetes.io/permanent-redirect-code` | number | 301 | 重定向状态码 | `308` |
| `nginx.ingress.kubernetes.io/temporal-redirect` | string | - | 临时重定向 URL | `https://temp.example.com` |
| `nginx.ingress.kubernetes.io/from-to-www-redirect` | bool | false | www 重定向 | `true` |
| `nginx.ingress.kubernetes.io/server-alias` | string | - | 服务器别名 | `alias.example.com` |
| `nginx.ingress.kubernetes.io/client-body-buffer-size` | string | 8k | 请求体缓冲区 | `1m` |
| `nginx.ingress.kubernetes.io/default-backend` | string | - | 自定义默认后端 | `namespace/service` |
| `nginx.ingress.kubernetes.io/custom-http-errors` | string | - | 自定义错误页面 | `404,503` |
| `nginx.ingress.kubernetes.io/mirror-uri` | string | - | 流量镜像 URI | `/mirror` |
| `nginx.ingress.kubernetes.io/mirror-request-body` | string | on | 镜像请求体 | `off` |
| `nginx.ingress.kubernetes.io/mirror-host` | string | - | 镜像目标主机 | `mirror.internal` |
| `nginx.ingress.kubernetes.io/enable-access-log` | bool | true | 启用访问日志 | `false` |
| `nginx.ingress.kubernetes.io/enable-rewrite-log` | bool | false | 启用重写日志 | `true` |
| `nginx.ingress.kubernetes.io/enable-opentracing` | bool | false | 启用 OpenTracing | `true` |
| `nginx.ingress.kubernetes.io/opentracing-trust-incoming-span` | bool | true | 信任入站 Span | `false` |

---

## 二、ConfigMap 全局配置参考

### 2.1 核心性能配置

| 配置项 | 默认值 | 说明 | 生产建议 |
|-------|-------|------|---------|
| `worker-processes` | auto | 工作进程数 | `auto` (CPU 核数) |
| `worker-cpu-affinity` | - | CPU 亲和性 | `auto` |
| `worker-shutdown-timeout` | 240s | 工作进程关闭超时 | `300s` |
| `max-worker-connections` | 16384 | 单进程最大连接 | `65535` |
| `max-worker-open-files` | - | 最大打开文件数 | `65535` |
| `keep-alive` | 75 | Keep-alive 超时 | `75` |
| `keep-alive-requests` | 1000 | 单连接最大请求 | `10000` |
| `upstream-keepalive-connections` | 320 | 上游连接池大小 | `320` |
| `upstream-keepalive-requests` | 10000 | 上游连接最大请求 | `10000` |
| `upstream-keepalive-timeout` | 60 | 上游连接超时 | `60` |

### 2.2 日志配置

| 配置项 | 默认值 | 说明 | 示例 |
|-------|-------|------|------|
| `access-log-path` | /var/log/nginx/access.log | 访问日志路径 | `/dev/stdout` |
| `error-log-path` | /var/log/nginx/error.log | 错误日志路径 | `/dev/stderr` |
| `log-format-upstream` | - | 日志格式 | 见下文 |
| `log-format-escape-json` | false | JSON 转义 | `true` |
| `log-format-escape-none` | false | 不转义 | `false` |
| `enable-access-log-for-default-backend` | false | 默认后端日志 | `true` |
| `skip-access-log-urls` | - | 跳过的 URL | `/healthz,/readyz` |

```yaml
# 推荐的 JSON 日志格式
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  log-format-escape-json: "true"
  log-format-upstream: '{"time":"$time_iso8601","request_id":"$req_id","remote_addr":"$remote_addr","x_forwarded_for":"$proxy_add_x_forwarded_for","request_method":"$request_method","host":"$host","uri":"$uri","args":"$args","status":$status,"body_bytes_sent":$body_bytes_sent,"request_time":$request_time,"upstream_response_time":"$upstream_response_time","upstream_addr":"$upstream_addr","http_referer":"$http_referer","http_user_agent":"$http_user_agent"}'
```

### 2.3 SSL/TLS 配置

| 配置项 | 默认值 | 说明 | 生产建议 |
|-------|-------|------|---------|
| `ssl-protocols` | TLSv1.2 TLSv1.3 | 支持的 TLS 版本 | `TLSv1.2 TLSv1.3` |
| `ssl-ciphers` | - | 加密套件 | 见下文 |
| `ssl-prefer-server-ciphers` | true | 优先服务器套件 | `true` |
| `ssl-session-cache` | true | SSL 会话缓存 | `true` |
| `ssl-session-cache-size` | 10m | 缓存大小 | `50m` |
| `ssl-session-timeout` | 10m | 会话超时 | `10m` |
| `ssl-session-tickets` | false | 会话票据 | `false` |
| `ssl-dh-param` | - | DH 参数 Secret | `ingress-nginx/dh-param` |
| `hsts` | true | 启用 HSTS | `true` |
| `hsts-max-age` | 15724800 | HSTS 最大存活 | `31536000` |
| `hsts-include-subdomains` | true | 包含子域 | `true` |
| `hsts-preload` | false | HSTS 预加载 | `true` |
| `ssl-redirect` | true | SSL 重定向 | `true` |

```yaml
# 推荐的 SSL 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  ssl-protocols: "TLSv1.2 TLSv1.3"
  ssl-ciphers: "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384"
  ssl-prefer-server-ciphers: "true"
  ssl-session-cache-size: "50m"
  ssl-session-timeout: "10m"
  ssl-session-tickets: "false"
  hsts: "true"
  hsts-max-age: "31536000"
  hsts-include-subdomains: "true"
  hsts-preload: "true"
```

### 2.4 安全配置

| 配置项 | 默认值 | 说明 | 生产建议 |
|-------|-------|------|---------|
| `enable-modsecurity` | false | 启用 ModSecurity | 按需 |
| `enable-owasp-modsecurity-crs` | false | 启用 OWASP 规则集 | 按需 |
| `modsecurity-snippet` | - | ModSecurity 配置 | - |
| `hide-headers` | - | 隐藏的响应头 | `X-Powered-By,Server` |
| `server-tokens` | false | 服务器版本标识 | `false` |
| `allow-snippet-annotations` | false | 允许 snippet 注解 | 谨慎开启 |
| `annotation-value-word-blocklist` | - | 注解值黑名单 | `load_module,lua_package,_by_lua` |
| `disable-catch-all` | false | 禁用全局匹配 | 按需 |
| `global-allowed-response-headers` | - | 允许的响应头 | - |

---

## 三、完整生产配置示例

### 3.1 生产级 Ingress 完整示例

```yaml
# 生产级完整 Ingress 配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: production-app-ingress
  namespace: production
  labels:
    app: myapp
    environment: production
  annotations:
    # --- TLS/SSL 配置 ---
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
    
    # --- 代理配置 ---
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-buffer-size: "16k"
    nginx.ingress.kubernetes.io/proxy-buffering: "on"
    
    # --- 重试配置 ---
    nginx.ingress.kubernetes.io/proxy-next-upstream: "error timeout http_502 http_503 http_504"
    nginx.ingress.kubernetes.io/proxy-next-upstream-tries: "3"
    nginx.ingress.kubernetes.io/proxy-next-upstream-timeout: "30"
    
    # --- 限流配置 ---
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/limit-connections: "20"
    nginx.ingress.kubernetes.io/limit-whitelist: "10.0.0.0/8"
    
    # --- CORS 配置 ---
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://www.example.com,https://app.example.com"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, OPTIONS"
    nginx.ingress.kubernetes.io/cors-allow-headers: "DNT,Keep-Alive,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range,Authorization"
    nginx.ingress.kubernetes.io/cors-allow-credentials: "true"
    nginx.ingress.kubernetes.io/cors-max-age: "86400"
    
    # --- 负载均衡 ---
    nginx.ingress.kubernetes.io/load-balance: "ewma"
    
    # --- 会话亲和 ---
    nginx.ingress.kubernetes.io/affinity: "cookie"
    nginx.ingress.kubernetes.io/affinity-mode: "persistent"
    nginx.ingress.kubernetes.io/session-cookie-name: "SERVERID"
    nginx.ingress.kubernetes.io/session-cookie-max-age: "3600"
    nginx.ingress.kubernetes.io/session-cookie-samesite: "Strict"
    
    # --- 安全头 ---
    nginx.ingress.kubernetes.io/configuration-snippet: |
      add_header X-Frame-Options "SAMEORIGIN" always;
      add_header X-Content-Type-Options "nosniff" always;
      add_header X-XSS-Protection "1; mode=block" always;
      add_header Referrer-Policy "strict-origin-when-cross-origin" always;
      add_header X-Request-ID $request_id always;
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    - api.example.com
    secretName: production-tls-secret
  rules:
  # API 路由
  - host: api.example.com
    http:
      paths:
      - path: /v1
        pathType: Prefix
        backend:
          service:
            name: api-v1-service
            port:
              number: 8080
      - path: /v2
        pathType: Prefix
        backend:
          service:
            name: api-v2-service
            port:
              number: 8080
  # 前端应用路由
  - host: app.example.com
    http:
      paths:
      - path: /static
        pathType: Prefix
        backend:
          service:
            name: static-service
            port:
              number: 80
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend-service
            port:
              number: 80
```

### 3.2 多环境 Ingress 配置模板

```yaml
# 使用 Kustomize 的多环境配置
# base/ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
spec:
  ingressClassName: nginx
  rules:
  - host: $(HOST)
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
---
# overlays/dev/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
- ../../base
patches:
- patch: |-
    - op: replace
      path: /spec/rules/0/host
      value: app.dev.example.com
    - op: add
      path: /metadata/annotations/nginx.ingress.kubernetes.io~1limit-rps
      value: "1000"
  target:
    kind: Ingress
    name: app-ingress
---
# overlays/prod/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
- ../../base
patches:
- patch: |-
    - op: replace
      path: /spec/rules/0/host
      value: app.example.com
    - op: add
      path: /metadata/annotations/nginx.ingress.kubernetes.io~1limit-rps
      value: "100"
    - op: add
      path: /spec/tls
      value:
        - hosts:
            - app.example.com
          secretName: prod-tls-secret
  target:
    kind: Ingress
    name: app-ingress
```

---

**NGINX Ingress 配置原则**: 安全优先 → 性能调优 → 合理限流 → 完善监控

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
