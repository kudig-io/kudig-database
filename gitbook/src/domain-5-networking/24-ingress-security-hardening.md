# 132 - Ingress 安全加固与防护

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01

---

## 一、Ingress 安全威胁概览

### 1.1 常见攻击类型

| 攻击类型 | 英文 | 描述 | 风险等级 |
|---------|------|------|---------|
| **DDoS 攻击** | Distributed Denial of Service | 大量请求导致服务不可用 | 高 |
| **SQL 注入** | SQL Injection | 通过输入执行恶意 SQL | 高 |
| **XSS 攻击** | Cross-Site Scripting | 注入恶意脚本 | 高 |
| **CSRF 攻击** | Cross-Site Request Forgery | 伪造用户请求 | 中 |
| **路径遍历** | Path Traversal | 访问受限目录 | 高 |
| **HTTP 走私** | HTTP Request Smuggling | 利用解析差异 | 高 |
| **慢速攻击** | Slowloris | 耗尽连接资源 | 中 |
| **暴力破解** | Brute Force | 猜测密码/密钥 | 中 |
| **信息泄露** | Information Disclosure | 暴露敏感信息 | 中 |
| **中间人攻击** | Man-in-the-Middle | 截获/篡改通信 | 高 |

### 1.2 Ingress 安全防护层次

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                            Ingress 安全防护层次                                  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │  Layer 1: 网络层防护 (Network Security)                                  │   │
│  │  - 云防火墙 / 安全组                                                     │   │
│  │  - DDoS 防护                                                            │   │
│  │  - IP 黑白名单                                                          │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                       │                                         │
│                                       ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │  Layer 2: 传输层防护 (Transport Security)                                │   │
│  │  - TLS 1.2/1.3 加密                                                     │   │
│  │  - 证书验证                                                             │   │
│  │  - HSTS                                                                 │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                       │                                         │
│                                       ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │  Layer 3: 应用层防护 (Application Security)                              │   │
│  │  - WAF (Web Application Firewall)                                       │   │
│  │  - 限流 / 速率限制                                                      │   │
│  │  - 认证 / 授权                                                          │   │
│  │  - 输入验证                                                             │   │
│  │  - 安全响应头                                                           │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                       │                                         │
│                                       ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │  Layer 4: 后端防护 (Backend Security)                                    │   │
│  │  - 网络策略 (NetworkPolicy)                                             │   │
│  │  - 服务间 mTLS                                                          │   │
│  │  - 最小权限原则                                                         │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、IP 访问控制

### 2.1 IP 白名单配置

```yaml
# 只允许特定 IP 访问
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: whitelist-ingress
  annotations:
    # IP 白名单 (CIDR 格式)
    nginx.ingress.kubernetes.io/whitelist-source-range: "10.0.0.0/8,172.16.0.0/12,192.168.0.0/16,203.0.113.0/24"
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

### 2.2 IP 黑名单配置

```yaml
# 禁止特定 IP 访问
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: denylist-ingress
  annotations:
    # IP 黑名单
    nginx.ingress.kubernetes.io/denylist-source-range: "1.2.3.4/32,5.6.7.0/24"
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
```

### 2.3 基于地理位置的访问控制

```yaml
# 使用 configuration-snippet 实现地理位置访问控制
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: geo-restricted-ingress
  annotations:
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # 假设已配置 GeoIP 模块
      # 只允许中国和美国访问
      if ($geoip2_data_country_code !~ ^(CN|US)$) {
        return 403;
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
```

---

## 三、限流与防 DDoS

### 3.1 请求速率限制

| 限流类型 | 注解 | 说明 | 适用场景 |
|---------|------|------|---------|
| **每秒请求数** | `limit-rps` | 限制每秒请求数 | API 保护 |
| **每分钟请求数** | `limit-rpm` | 限制每分钟请求数 | 一般限流 |
| **并发连接数** | `limit-connections` | 限制并发连接 | 连接保护 |
| **响应速率** | `limit-rate` | 限制响应带宽 | 带宽控制 |
| **全局限流** | `global-rate-limit` | 分布式限流 | 集群级别 |

```yaml
# 综合限流配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: rate-limited-ingress
  annotations:
    # 每秒最多 100 个请求
    nginx.ingress.kubernetes.io/limit-rps: "100"
    # 每分钟最多 1000 个请求
    nginx.ingress.kubernetes.io/limit-rpm: "1000"
    # 最多 50 个并发连接
    nginx.ingress.kubernetes.io/limit-connections: "50"
    # 白名单 (不受限流影响)
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
```

### 3.2 分级限流策略

```yaml
# 公共 API - 严格限流
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: public-api-ingress
  annotations:
    nginx.ingress.kubernetes.io/limit-rps: "10"
    nginx.ingress.kubernetes.io/limit-connections: "5"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /public
        pathType: Prefix
        backend:
          service:
            name: public-api
            port:
              number: 8080
---
# 认证 API - 中等限流
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: auth-api-ingress
  annotations:
    nginx.ingress.kubernetes.io/limit-rps: "50"
    nginx.ingress.kubernetes.io/limit-connections: "20"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /v1
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
---
# 内部 API - 宽松限流
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: internal-api-ingress
  annotations:
    nginx.ingress.kubernetes.io/limit-rps: "500"
    nginx.ingress.kubernetes.io/whitelist-source-range: "10.0.0.0/8"
spec:
  ingressClassName: nginx
  rules:
  - host: internal-api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: internal-api
            port:
              number: 8080
```

### 3.3 防慢速攻击配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # 客户端请求体超时
  client-body-timeout: "10"
  # 客户端请求头超时
  client-header-timeout: "10"
  # Keep-alive 超时
  keep-alive: "75"
  # Keep-alive 最大请求数
  keep-alive-requests: "1000"
  # 大请求体缓冲
  large-client-header-buffers: "4 16k"
  # 请求体缓冲区
  client-body-buffer-size: "16k"
```

---

## 四、认证与授权

### 4.1 Basic Auth 认证

```yaml
# 创建认证 Secret
# htpasswd -c auth admin
# kubectl create secret generic basic-auth --from-file=auth

apiVersion: v1
kind: Secret
metadata:
  name: basic-auth
  namespace: default
type: Opaque
data:
  auth: YWRtaW46JGFwcjEkSDBHWS5CN3IkVnlDV2ZYREJSUGhQMXY3SGNPV0FlLgo=
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: basic-auth-ingress
  annotations:
    nginx.ingress.kubernetes.io/auth-type: basic
    nginx.ingress.kubernetes.io/auth-secret: basic-auth
    nginx.ingress.kubernetes.io/auth-realm: "Authentication Required - Admin Area"
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
```

### 4.2 外部认证 (OAuth2/OIDC)

```yaml
# 使用 OAuth2 Proxy 进行外部认证
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: oauth2-protected-ingress
  annotations:
    # 外部认证 URL
    nginx.ingress.kubernetes.io/auth-url: "https://oauth2-proxy.auth.svc.cluster.local/oauth2/auth"
    # 登录页面
    nginx.ingress.kubernetes.io/auth-signin: "https://auth.example.com/oauth2/start?rd=$scheme://$host$escaped_request_uri"
    # 传递用户信息到后端
    nginx.ingress.kubernetes.io/auth-response-headers: "X-Auth-Request-User,X-Auth-Request-Email,X-Auth-Request-Groups,Authorization"
    # 缓存认证结果
    nginx.ingress.kubernetes.io/auth-cache-key: "$cookie_oauth2_proxy"
    nginx.ingress.kubernetes.io/auth-cache-duration: "200 202 10m, 401 1m"
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

### 4.3 mTLS 客户端证书认证

```yaml
# 创建 CA Secret
apiVersion: v1
kind: Secret
metadata:
  name: client-ca
  namespace: default
type: Opaque
data:
  ca.crt: <base64-encoded-ca-certificate>
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: mtls-ingress
  annotations:
    # 客户端 CA 证书
    nginx.ingress.kubernetes.io/auth-tls-secret: "default/client-ca"
    # 强制客户端证书验证
    nginx.ingress.kubernetes.io/auth-tls-verify-client: "on"
    # 证书链验证深度
    nginx.ingress.kubernetes.io/auth-tls-verify-depth: "2"
    # 传递客户端证书信息到后端
    nginx.ingress.kubernetes.io/auth-tls-pass-certificate-to-upstream: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - secure.example.com
    secretName: server-tls
  rules:
  - host: secure.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: secure-service
            port:
              number: 80
```

---

## 五、安全响应头

### 5.1 安全响应头说明

| 响应头 | 作用 | 推荐值 |
|-------|------|-------|
| `X-Frame-Options` | 防止点击劫持 | `SAMEORIGIN` 或 `DENY` |
| `X-Content-Type-Options` | 防止 MIME 类型嗅探 | `nosniff` |
| `X-XSS-Protection` | XSS 过滤器 | `1; mode=block` |
| `Strict-Transport-Security` | 强制 HTTPS | `max-age=31536000; includeSubDomains` |
| `Content-Security-Policy` | 内容安全策略 | 按需配置 |
| `Referrer-Policy` | 控制 Referer 头 | `strict-origin-when-cross-origin` |
| `Permissions-Policy` | 功能权限控制 | 按需配置 |
| `X-Permitted-Cross-Domain-Policies` | Flash 跨域策略 | `none` |

### 5.2 安全响应头配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: secure-headers-ingress
  annotations:
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # 防止点击劫持
      add_header X-Frame-Options "SAMEORIGIN" always;
      
      # 防止 MIME 类型嗅探
      add_header X-Content-Type-Options "nosniff" always;
      
      # XSS 过滤器
      add_header X-XSS-Protection "1; mode=block" always;
      
      # 强制 HTTPS (HSTS)
      add_header Strict-Transport-Security "max-age=31536000; includeSubDomains; preload" always;
      
      # 内容安全策略
      add_header Content-Security-Policy "default-src 'self'; script-src 'self' 'unsafe-inline' 'unsafe-eval'; style-src 'self' 'unsafe-inline'; img-src 'self' data: https:; font-src 'self' data:; connect-src 'self' https://api.example.com" always;
      
      # Referrer 策略
      add_header Referrer-Policy "strict-origin-when-cross-origin" always;
      
      # 功能权限策略
      add_header Permissions-Policy "accelerometer=(), camera=(), geolocation=(), gyroscope=(), magnetometer=(), microphone=(), payment=(), usb=()" always;
      
      # 隐藏服务器信息
      more_clear_headers Server;
      more_clear_headers X-Powered-By;
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
            name: app-service
            port:
              number: 80
```

### 5.3 全局安全响应头配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # 隐藏服务器版本
  server-tokens: "false"
  
  # 隐藏特定响应头
  hide-headers: "X-Powered-By,Server"
  
  # HSTS 配置
  hsts: "true"
  hsts-max-age: "31536000"
  hsts-include-subdomains: "true"
  hsts-preload: "true"
  
  # SSL 配置
  ssl-protocols: "TLSv1.2 TLSv1.3"
  ssl-ciphers: "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384"
```

---

## 六、WAF (Web Application Firewall)

### 6.1 ModSecurity WAF

```yaml
# 启用 ModSecurity
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # 启用 ModSecurity
  enable-modsecurity: "true"
  # 启用 OWASP 核心规则集
  enable-owasp-modsecurity-crs: "true"
  # ModSecurity 配置
  modsecurity-snippet: |
    # 检测模式 (DetectionOnly) 或阻断模式 (On)
    SecRuleEngine On
    
    # 审计日志
    SecAuditEngine RelevantOnly
    SecAuditLogRelevantStatus "^(?:5|4(?!04))"
    SecAuditLogParts ABIJDEFHZ
    SecAuditLogType Serial
    SecAuditLog /var/log/modsec_audit.log
    
    # 请求体处理
    SecRequestBodyAccess On
    SecRequestBodyLimit 13107200
    SecRequestBodyNoFilesLimit 131072
    
    # 响应体处理
    SecResponseBodyAccess On
    SecResponseBodyLimit 1048576
    
    # 文件上传
    SecUploadDir /tmp
    SecUploadKeepFiles Off
    
    # 临时文件
    SecTmpDir /tmp
    SecDataDir /tmp
```

### 6.2 ModSecurity 自定义规则

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: waf-protected-ingress
  annotations:
    nginx.ingress.kubernetes.io/enable-modsecurity: "true"
    nginx.ingress.kubernetes.io/enable-owasp-modsecurity-crs: "true"
    nginx.ingress.kubernetes.io/modsecurity-snippet: |
      # 自定义规则: 阻止特定 User-Agent
      SecRule REQUEST_HEADERS:User-Agent "@contains badbot" \
        "id:10001,phase:1,deny,status:403,msg:'Bad bot detected'"
      
      # 自定义规则: 阻止 SQL 注入尝试
      SecRule ARGS "@detectSQLi" \
        "id:10002,phase:2,deny,status:403,msg:'SQL Injection attempt detected'"
      
      # 自定义规则: 限制请求体大小
      SecRule REQUEST_BODY_LENGTH "@gt 1048576" \
        "id:10003,phase:2,deny,status:413,msg:'Request body too large'"
      
      # 白名单规则: 允许特定路径跳过检查
      SecRule REQUEST_URI "@beginsWith /api/webhook" \
        "id:10004,phase:1,pass,nolog,ctl:ruleEngine=Off"
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
```

### 6.3 OWASP Top 10 防护规则

| 威胁 | 规则 ID 范围 | 防护措施 |
|-----|-------------|---------|
| **注入攻击** | 941xxx | SQL/NoSQL/LDAP/XPath 注入检测 |
| **身份验证** | 942xxx | 弱认证检测 |
| **敏感数据暴露** | 943xxx | 敏感信息泄露检测 |
| **XXE** | 944xxx | XML 外部实体检测 |
| **访问控制** | 945xxx | 访问控制绕过检测 |
| **安全配置** | 946xxx | 错误配置检测 |
| **XSS** | 941xxx | 跨站脚本检测 |
| **反序列化** | 944xxx | 不安全反序列化检测 |
| **组件漏洞** | - | 依赖外部漏洞扫描 |
| **日志监控** | - | 审计日志配置 |

---

## 七、Ingress Controller 安全加固

### 7.1 RBAC 最小权限

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: ingress-nginx-controller
rules:
# 最小必要权限
- apiGroups: [""]
  resources: ["configmaps", "endpoints", "pods", "secrets", "namespaces"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["services"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["networking.k8s.io"]
  resources: ["ingresses", "ingressclasses"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["networking.k8s.io"]
  resources: ["ingresses/status"]
  verbs: ["update"]
- apiGroups: [""]
  resources: ["events"]
  verbs: ["create", "patch"]
# 禁止不必要的权限
# - apiGroups: [""]
#   resources: ["*"]
#   verbs: ["*"]
```

### 7.2 Pod 安全配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
spec:
  template:
    spec:
      # 安全上下文
      securityContext:
        runAsNonRoot: true
        runAsUser: 101
        runAsGroup: 101
        fsGroup: 101
        seccompProfile:
          type: RuntimeDefault
      containers:
      - name: controller
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          capabilities:
            drop:
            - ALL
            add:
            - NET_BIND_SERVICE
        # 资源限制
        resources:
          requests:
            cpu: 100m
            memory: 256Mi
          limits:
            cpu: 1000m
            memory: 512Mi
        # 只读挂载
        volumeMounts:
        - name: tmp
          mountPath: /tmp
        - name: var-run
          mountPath: /var/run
      volumes:
      - name: tmp
        emptyDir: {}
      - name: var-run
        emptyDir: {}
```

### 7.3 NetworkPolicy 隔离

```yaml
# 限制 Ingress Controller 网络访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
spec:
  podSelector:
    matchLabels:
      app.kubernetes.io/component: controller
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 允许来自外部的流量
  - from: []
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
  # 允许来自 Prometheus 的指标抓取
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 10254
  egress:
  # 允许访问 Kubernetes API
  - to:
    - ipBlock:
        cidr: <API-SERVER-IP>/32
    ports:
    - protocol: TCP
      port: 443
  # 允许访问所有 Service
  - to:
    - namespaceSelector: {}
    ports:
    - protocol: TCP
  # 允许 DNS 查询
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
    ports:
    - protocol: UDP
      port: 53
```

---

## 八、注解安全

### 8.1 限制危险注解

```yaml
# ConfigMap 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # 禁用 snippet 注解 (防止注入)
  allow-snippet-annotations: "false"
  
  # 注解值黑名单
  annotation-value-word-blocklist: "load_module,lua_package,_by_lua,root,alias"
```

### 8.2 Snippet 安全风险

| 风险 | 描述 | 缓解措施 |
|-----|------|---------|
| **配置注入** | 恶意注解修改 NGINX 配置 | 禁用 snippet 注解 |
| **文件读取** | 通过 alias/root 读取文件 | 配置黑名单 |
| **代码执行** | Lua 代码执行 | 禁用 Lua 指令 |
| **权限提升** | 修改进程权限 | 使用 securityContext |

```yaml
# 如果必须启用 snippet，限制使用范围
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # 允许 snippet 但限制危险指令
  allow-snippet-annotations: "true"
  annotation-value-word-blocklist: |
    load_module,
    lua_package,
    _by_lua,
    _by_lua_block,
    root,
    alias,
    internal,
    proxy_pass http://unix:,
    proxy_pass https://unix:
```

---

## 九、审计与日志

### 9.1 访问日志配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # JSON 格式日志
  log-format-escape-json: "true"
  log-format-upstream: |
    {
      "time": "$time_iso8601",
      "remote_addr": "$remote_addr",
      "x_forwarded_for": "$proxy_add_x_forwarded_for",
      "request_id": "$req_id",
      "remote_user": "$remote_user",
      "bytes_sent": $bytes_sent,
      "request_time": $request_time,
      "status": $status,
      "request": "$request",
      "request_method": "$request_method",
      "host": "$host",
      "uri": "$uri",
      "http_referrer": "$http_referer",
      "http_user_agent": "$http_user_agent",
      "upstream_addr": "$upstream_addr",
      "upstream_response_time": "$upstream_response_time",
      "upstream_status": "$upstream_status",
      "ssl_protocol": "$ssl_protocol",
      "ssl_cipher": "$ssl_cipher"
    }
```

### 9.2 审计日志告警

```yaml
# Prometheus 告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ingress-security-alerts
  namespace: monitoring
spec:
  groups:
  - name: ingress-security
    rules:
    # 高错误率告警
    - alert: IngressHighErrorRate
      expr: |
        sum(rate(nginx_ingress_controller_requests{status=~"5.."}[5m])) 
        / sum(rate(nginx_ingress_controller_requests[5m])) > 0.05
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress 错误率过高"
        description: "Ingress 5xx 错误率超过 5%: {{ $value | humanizePercentage }}"
    
    # 限流触发告警
    - alert: IngressRateLimitTriggered
      expr: |
        sum(rate(nginx_ingress_controller_requests{status="429"}[5m])) > 10
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "Ingress 限流触发"
        description: "Ingress 限流请求数: {{ $value }}/s"
    
    # 认证失败告警
    - alert: IngressAuthFailures
      expr: |
        sum(rate(nginx_ingress_controller_requests{status="401"}[5m])) > 50
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress 认证失败过多"
        description: "认证失败请求数: {{ $value }}/s，可能存在暴力破解攻击"
    
    # 异常流量告警
    - alert: IngressAbnormalTraffic
      expr: |
        sum(rate(nginx_ingress_controller_nginx_process_requests_total[5m])) 
        > 2 * avg_over_time(sum(rate(nginx_ingress_controller_nginx_process_requests_total[5m]))[1h:5m])
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress 异常流量"
        description: "当前流量是过去 1 小时平均值的 2 倍以上"
```

---

## 十、安全检查清单

### 10.1 部署前检查

| 检查项 | 说明 | 状态 |
|-------|------|------|
| TLS 1.2+ | 禁用低版本 TLS | ☐ |
| 强制 HTTPS | 配置 SSL 重定向 | ☐ |
| 安全响应头 | 配置所有安全头 | ☐ |
| 限流配置 | 配置适当的限流 | ☐ |
| IP 白名单 | 敏感路径访问控制 | ☐ |
| 认证配置 | 管理接口启用认证 | ☐ |
| RBAC 配置 | 最小权限原则 | ☐ |
| NetworkPolicy | 网络隔离 | ☐ |
| Pod 安全 | 非 root 运行 | ☐ |
| 资源限制 | 配置 requests/limits | ☐ |

### 10.2 运行时检查

| 检查项 | 说明 | 状态 |
|-------|------|------|
| 日志监控 | 访问日志收集分析 | ☐ |
| 指标监控 | 错误率、延迟监控 | ☐ |
| 告警配置 | 安全事件告警 | ☐ |
| 证书续期 | 自动证书管理 | ☐ |
| 漏洞扫描 | 定期安全扫描 | ☐ |
| 配置审计 | 定期审计配置 | ☐ |

---

**Ingress 安全原则**: 纵深防御 → 最小权限 → 持续监控 → 快速响应

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
