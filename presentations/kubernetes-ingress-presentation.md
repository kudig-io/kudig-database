# Kubernetes Ingress 从入门到实战

> **适用环境**: 阿里云专有云 & 公共云 | **重点产品**: ACK | **版本**: Kubernetes v1.25-v1.32  
> **文档类型**: PPT演示文稿内容 | **目标受众**: 开发者、运维工程师、架构师  

---

## 目录

1. [Ingress 基础概念](#1-ingress-基础概念)
2. [Ingress 控制器详解](#2-ingress-控制器详解)
3. [阿里云环境实践](#3-阿里云环境实践)
4. [ACK 产品集成](#4-ack-产品集成)
5. [高级特性与配置](#5-高级特性与配置)
6. [生产最佳实践](#6-生产最佳实践)
7. [监控与故障排查](#7-监控与故障排查)
8. [总结与Q&A](#8-总结与qa)

---

## 1. Ingress 基础概念

### 1.1 什么是 Ingress？

**核心定义**
- Kubernetes 中管理集群外部访问 HTTP/HTTPS 路由的核心组件
- 提供 L7 负载均衡、TLS 终止、虚拟主机等功能
- 解耦服务暴露与应用部署

**关键特性**
- ✅ 基于主机名和路径的路由
- ✅ TLS/SSL 终止
- ✅ 负载均衡
- ✅ 金丝雀发布支持
- ✅ 限流和认证

### 1.2 为什么需要 Ingress？

**没有 Ingress 的痛点**
```
❌ 每个服务需要独立 LoadBalancer，成本高
❌ 无法基于域名/路径路由
❌ TLS 需要单独配置
❌ 难以实现流量治理
❌ 缺乏统一的安全策略
```

**使用 Ingress 的优势**
```
✅ 多服务共享统一入口，降低成本
✅ 灵活的 L7 路由规则
✅ 集中管理 TLS 证书
✅ 支持金丝雀、蓝绿部署
✅ 统一安全策略实施
```

### 1.3 Ingress 核心架构

```
[客户端] → [云负载均衡器] → [Ingress Controller] → [Service] → [Pods]
   ↑           ↑                    ↑                 ↑          ↑
 外网访问   阿里云SLB/ALB      NGINX/Traefik等     服务发现    应用实例
```

**核心组件**
- **Ingress Resource**: 定义路由规则的 YAML 资源
- **Ingress Controller**: 实现路由规则的控制器
- **IngressClass**: 指定控制器类型的资源
- **Backend Service**: 实际提供服务的后端

### 1.4 Ingress vs 其他访问方式对比

| 方案 | 协议层 | 适用场景 | 成本 | 复杂度 |
|------|--------|----------|------|--------|
| **ClusterIP** | L4 | 集群内部通信 | 低 | 简单 |
| **NodePort** | L4 | 开发测试 | 中 | 简单 |
| **LoadBalancer** | L4 | 单服务暴露 | 高 | 简单 |
| **Ingress** | L7 | 多服务HTTP路由 | 低 | 中等 |
| **Service Mesh** | L4/L7 | 微服务治理 | 高 | 复杂 |

---

## 2. Ingress 控制器详解

### 2.1 主流控制器对比

| 控制器 | 代理引擎 | 性能 | 学习曲线 | ACK支持 |
|--------|----------|------|----------|---------|
| **NGINX Ingress** | NGINX | 高 | 中 | ✅ |
| **Traefik** | Traefik | 中-高 | 低 | - |
| **HAProxy** | HAProxy | 很高 | 中 | - |
| **Contour** | Envoy | 高 | 中 | - |
| **ALB Ingress** | 阿里云ALB | 很高 | 低 | ✅ 原生 |
| **Kong** | Kong/NGINX | 高 | 高 | - |

### 2.2 Ingress API 结构

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: example-ingress
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
spec:
  ingressClassName: nginx  # 指定控制器类型
  tls:                     # TLS 配置
  - hosts:
    - app.example.com
    secretName: app-tls
  rules:                   # 路由规则
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
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-service
            port:
              number: 80
```

### 2.3 路径匹配类型

| 类型 | 匹配规则 | 示例 | 适用场景 |
|------|----------|------|----------|
| **Exact** | 完全匹配 | `/health` | 健康检查端点 |
| **Prefix** | 前缀匹配 | `/api/*` | 服务路由 |
| **ImplementationSpecific** | 控制器特定 | 正则表达式 | 高级路由 |

### 2.4 IngressClass 机制

```yaml
# 定义 IngressClass
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx
  annotations:
    ingressclass.kubernetes.io/is-default-class: "true"
spec:
  controller: k8s.io/ingress-nginx

---
# 使用特定 IngressClass
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
spec:
  ingressClassName: nginx  # 明确指定
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

## 3. 阿里云环境实践

### 3.1 专有云 vs 公共云差异

| 特性 | 专有云 (Apsara Stack) | 公共云 (ACK) |
|------|---------------------|-------------|
| **网络环境** | 私有网络 | 公网+私网 |
| **负载均衡** | SLB内网 | SLB公网/内网 |
| **安全管控** | 本地化策略 | 云安全中心 |
| **运维模式** | 本地运维 | 托管运维 |
| **证书管理** | 本地CA | 云证书服务 |

### 3.2 负载均衡器选择策略

#### CLB (传统型负载均衡)
```
适用场景: TCP/UDP协议
优势: 成熟稳定，成本较低
限制: 不支持HTTP高级特性
```

#### NLB (网络型负载均衡)
```
适用场景: 高性能TCP/UDP
优势: 超低延迟，超高并发
限制: 仅支持四层协议
```

#### ALB (应用型负载均衡)
```
适用场景: HTTP/HTTPS应用
优势: 七层路由，丰富特性
限制: 成本相对较高
```

### 3.3 网络规划建议

**专有云环境配置**
```yaml
# 推荐网络配置
VPC网段: 10.0.0.0/8
Pod网段: 172.20.0.0/16
Service网段: 172.21.0.0/16
Ingress IP: 10.x.x.x (内网SLB)
```

**公共云环境配置**
```yaml
# ACK推荐配置
VPC: 自动创建或复用现有
Pod CIDR: 172.20.0.0/16
Service CIDR: 172.21.0.0/20
Ingress IP: 公网SLB或EIP
```

### 3.4 安全组配置

```yaml
# Ingress Controller安全组配置
apiVersion: v1
kind: Service
metadata:
  name: ingress-nginx-controller
  annotations:
    # 绑定安全组
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-security-group-id: "sg-xxxxxxxxx"
    
    # 访问控制
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-enable: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-type: "white"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-list: "192.168.0.0/16,10.0.0.0/8"
spec:
  type: LoadBalancer
  ports:
  - name: http
    port: 80
    targetPort: 80
  - name: https
    port: 443
    targetPort: 443
  selector:
    app.kubernetes.io/name: ingress-nginx
    app.kubernetes.io/component: controller
```

---

## 4. ACK 产品集成

### 4.1 ALB Ingress 原生集成

**ALB Ingress 优势**
- ✅ 免运维的云原生负载均衡
- ✅ 自动弹性伸缩
- ✅ 丰富的七层路由功能
- ✅ 原生支持金丝雀发布
- ✅ 集成WAF和安全防护

**ALB Ingress 配置示例**
```yaml
# ALB Ingress 配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: alb-ingress
  annotations:
    # 指定使用ALB控制器
    kubernetes.io/ingress.class: alb
    
    # 负载均衡配置
    alb.ingress.kubernetes.io/address-type: internet
    alb.ingress.kubernetes.io/vswitch-ids: "vsw-xxx1,vsw-xxx2"
    
    # HTTPS重定向
    alb.ingress.kubernetes.io/ssl-redirect: "true"
    
    # 健康检查
    alb.ingress.kubernetes.io/healthcheck-enabled: "true"
    alb.ingress.kubernetes.io/healthcheck-path: "/health"
    alb.ingress.kubernetes.io/healthcheck-protocol: "HTTP"
    alb.ingress.kubernetes.io/healthcheck-interval-seconds: "5"
    
    # 金丝雀发布
    alb.ingress.kubernetes.io/canary: "true"
    alb.ingress.kubernetes.io/canary-by-header: "X-Canary"
    alb.ingress.kubernetes.io/canary-weight: "10"
    
    # 限流配置
    alb.ingress.kubernetes.io/traffic-limit-qps: "1000"
spec:
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
            name: app-service
            port:
              number: 80
```

### 4.2 ACK托管版集成优势

| 特性 | ACK托管版 | 自建Ingress |
|------|-----------|-------------|
| **运维复杂度** | 零运维 | 需要专业运维 |
| **成本** | 按量付费 | 固定资源成本 |
| **可靠性** | 99.95% SLA | 依赖自身保障 |
| **功能更新** | 自动更新 | 手动升级 |
| **安全防护** | 集成WAF | 需额外配置 |

### 4.3 多环境部署策略

```yaml
# 环境差异化配置
# 开发环境 - 使用内网SLB降低成本
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: dev-ingress
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
spec:
  # ... 其他配置

---
# 生产环境 - 使用公网ALB保证性能
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: prod-ingress
  annotations:
    kubernetes.io/ingress.class: alb
    alb.ingress.kubernetes.io/address-type: "internet"
spec:
  # ... 其他配置
```

---

## 5. 高级特性与配置

### 5.1 金丝雀发布

**基于权重的金丝雀**
```yaml
# 稳定版本 (90%流量)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-stable
spec:
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
# 金丝雀版本 (10%流量)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"
spec:
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

**基于Header的金丝雀**
```yaml
# 内部测试流量路由
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary-header
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-by-header: "X-Test-User"
    nginx.ingress.kubernetes.io/canary-by-header-value: "internal"
spec:
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-beta
            port:
              number: 80
```

### 5.2 TLS/SSL 配置

**基础TLS配置**
```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tls-ingress
  annotations:
    # 强制HTTPS
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    # HSTS配置
    nginx.ingress.kubernetes.io/hsts: "true"
    nginx.ingress.kubernetes.io/hsts-max-age: "31536000"
spec:
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls-secret
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

**cert-manager 自动证书管理**
```yaml
# ClusterIssuer 配置
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    email: admin@example.com
    server: https://acme-v02.api.letsencrypt.org/directory
    privateKeySecretRef:
      name: letsencrypt-prod-account-key
    solvers:
    - http01:
        ingress:
          class: nginx

---
# 自动创建证书的Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: auto-tls-ingress
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
spec:
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls  # cert-manager自动创建
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

### 5.3 限流与安全配置

**基础限流配置**
```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: rate-limited-ingress
  annotations:
    # 每秒请求数限制
    nginx.ingress.kubernetes.io/limit-rps: "100"
    # 并发连接数限制
    nginx.ingress.kubernetes.io/limit-connections: "20"
    # IP白名单
    nginx.ingress.kubernetes.io/limit-whitelist: "10.0.0.0/8,192.168.0.0/16"
spec:
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

**Basic Auth 认证**
```yaml
# 创建认证Secret
apiVersion: v1
kind: Secret
metadata:
  name: basic-auth
type: Opaque
data:
  auth: <base64-encoded-htpasswd>

---
# 配置Basic Auth
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: protected-ingress
  annotations:
    nginx.ingress.kubernetes.io/auth-type: basic
    nginx.ingress.kubernetes.io/auth-secret: basic-auth
    nginx.ingress.kubernetes.io/auth-realm: "Authentication Required"
spec:
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

### 5.4 CORS 跨域配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: cors-ingress
  annotations:
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://app.example.com,https://admin.example.com"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, OPTIONS"
    nginx.ingress.kubernetes.io/cors-allow-headers: "DNT,Keep-Alive,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range,Authorization"
    nginx.ingress.kubernetes.io/cors-allow-credentials: "true"
    nginx.ingress.kubernetes.io/cors-max-age: "86400"
spec:
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

---

## 6. 生产最佳实践

### 6.1 高可用部署架构

```
┌─────────────────────────────────────────────────────────────┐
│                    生产级Ingress架构                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │   DNS/WAF   │    │   CDN边缘   │    │  监控告警    │     │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘     │
│         │                  │                  │            │
│         ▼                  ▼                  ▼            │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              阿里云负载均衡器 (ALB/SLB)                │   │
│  │              多可用区部署，自动故障转移                 │   │
│  └───────────────────────┬───────────────────────────────┘   │
│                          │                                   │
│        ┌─────────────────┼─────────────────┐               │
│        │                 │                 │               │
│        ▼                 ▼                 ▼               │
│  ┌───────────┐   ┌───────────┐   ┌───────────┐            │
│  │ Ingress   │   │ Ingress   │   │ Ingress   │            │
│  │ Controller│   │ Controller│   │ Controller│            │
│  │ (Zone A)  │   │ (Zone B)  │   │ (Zone C)  │            │
│  └───────────┘   └───────────┘   └───────────┘            │
│        │                 │                 │               │
│        └─────────────────┼─────────────────┘               │
│                          │                                   │
│                          ▼                                   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                   后端服务                            │   │
│  │  Service Mesh / 直连Pod / 传统服务发现                  │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 6.2 完整生产配置示例

```yaml
# 生产级Ingress完整配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: production-ingress
  namespace: production
  labels:
    app: myapp
    environment: production
  annotations:
    # --- TLS/SSL 配置 ---
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
    nginx.ingress.kubernetes.io/hsts: "true"
    nginx.ingress.kubernetes.io/hsts-max-age: "31536000"
    
    # --- 代理配置 ---
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-buffer-size: "16k"
    
    # --- 重试配置 ---
    nginx.ingress.kubernetes.io/proxy-next-upstream: "error timeout http_502 http_503 http_504"
    nginx.ingress.kubernetes.io/proxy-next-upstream-tries: "3"
    nginx.ingress.kubernetes.io/proxy-next-upstream-timeout: "30"
    
    # --- 限流配置 ---
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/limit-connections: "50"
    nginx.ingress.kubernetes.io/limit-whitelist: "10.0.0.0/8,192.168.0.0/16"
    
    # --- CORS 配置 ---
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://www.example.com,https://app.example.com"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, OPTIONS"
    nginx.ingress.kubernetes.io/cors-allow-credentials: "true"
    
    # --- 安全头 ---
    nginx.ingress.kubernetes.io/configuration-snippet: |
      add_header X-Frame-Options "SAMEORIGIN" always;
      add_header X-Content-Type-Options "nosniff" always;
      add_header X-XSS-Protection "1; mode=block" always;
      add_header Referrer-Policy "strict-origin-when-cross-origin" always;
      add_header X-Request-ID $req_id always;
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    - api.example.com
    secretName: production-tls
  rules:
  # API路由
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
  # Web应用路由
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

### 6.3 监控告警配置

**关键监控指标**
- ✅ 请求成功率 (>99%)
- ✅ 5xx错误率 (<1%)
- ✅ P99延迟 (<500ms)
- ✅ QPS和并发连接数
- ✅ 证书过期时间预警
- ✅ 配置重载成功率

**Prometheus告警规则**
```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ingress-alerts
  namespace: monitoring
spec:
  groups:
  - name: ingress-availability
    rules:
    # 5xx错误率过高
    - alert: IngressHighErrorRate
      expr: |
        sum(rate(nginx_ingress_controller_requests{status=~"5.."}[5m])) 
        / sum(rate(nginx_ingress_controller_requests[5m])) > 0.01
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress 5xx错误率过高"
    
    # 证书即将过期
    - alert: IngressCertificateExpiring
      expr: |
        (nginx_ingress_controller_ssl_expire_time_seconds - time()) < 604800
      for: 1h
      labels:
        severity: critical
      annotations:
        summary: "Ingress证书即将过期"
```

### 6.4 安全加固措施

**网络安全配置**
```yaml
# NetworkPolicy限制Ingress访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: ingress-network-policy
  namespace: ingress-nginx
spec:
  podSelector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 仅允许负载均衡器访问
  - from:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          app: load-balancer
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
  egress:
  # 允许访问后端服务
  - to:
    - namespaceSelector:
        matchLabels:
          name: production
    ports:
    - protocol: TCP
      port: 8080
```

**安全组最佳实践**
- ✅ 最小权限原则：仅开放必要端口
- ✅ 源IP限制：使用白名单控制访问
- ✅ 定期审查：定期检查安全组规则
- ✅ 日志审计：启用访问日志记录

---

## 7. 监控与故障排查

### 7.1 关键监控指标

| 指标类别 | 指标名称 | 正常范围 | 告警阈值 |
|----------|----------|----------|----------|
| **可用性** | 请求成功率 | >99% | <99% |
| **可用性** | 5xx错误率 | <1% | >1% |
| **性能** | P99延迟 | <500ms | >1s |
| **性能** | QPS | 根据容量规划 | 接近上限 |
| **资源** | CPU使用率 | <80% | >80% |
| **资源** | 内存使用率 | <80% | >80% |
| **健康** | 配置重载成功率 | 100% | <100% |

### 7.2 常见故障诊断流程

```
┌─────────────────────────────────────────────────────────────┐
│                    Ingress故障诊断流程                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  1. 确认故障现象                                             │
│     ├── 完全无法访问？                                       │
│     ├── 部分请求失败？                                       │
│     ├── 响应缓慢？                                          │
│     └── 证书错误？                                          │
│                                                              │
│  2. 检查Ingress Controller                                  │
│     ├── kubectl get pods -n ingress-nginx                   │
│     ├── kubectl describe pod <controller-pod>               │
│     └── kubectl logs <controller-pod>                       │
│                                                              │
│  3. 验证Ingress配置                                         │
│     ├── kubectl get ingress -A                              │
│     ├── kubectl describe ingress <name>                     │
│     └── 检查配置语法和路由规则                               │
│                                                              │
│  4. 检查后端服务                                            │
│     ├── kubectl get svc,endpoints <service>                 │
│     ├── kubectl get pods -l <selector>                      │
│     └── 测试后端连通性                                      │
│                                                              │
│  5. 网络连通性测试                                          │
│     ├── DNS解析测试                                         │
│     ├── 端口连通性测试                                       │
│     └── 负载均衡器状态检查                                   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 7.3 常见问题与解决方案

| 问题 | 症状 | 可能原因 | 解决方案 |
|------|------|----------|----------|
| **502 Bad Gateway** | 请求返回502 | 后端服务不可达 | 检查后端Pod状态和Service配置 |
| **503 Service Unavailable** | 请求返回503 | 无可用后端实例 | 扩容后端服务或检查健康检查 |
| **504 Gateway Timeout** | 请求超时 | 后端响应过慢 | 增加超时配置或优化后端性能 |
| **404 Not Found** | 路径不存在 | 路由规则不匹配 | 检查Ingress path配置 |
| **证书错误** | HTTPS握手失败 | 证书过期或不匹配 | 更新证书或检查证书配置 |
| **重定向循环** | ERR_TOO_MANY_REDIRECTS | 配置冲突 | 调整ssl-redirect配置 |

### 7.4 诊断命令速查

```bash
# Ingress Controller状态检查
kubectl get pods -n ingress-nginx
kubectl describe pod -n ingress-nginx <pod-name>
kubectl logs -n ingress-nginx <pod-name> --tail=100 -f

# Ingress资源配置检查
kubectl get ingress -A
kubectl describe ingress <name> -n <namespace>
kubectl get events --field-selector involvedObject.name=<ingress>

# 后端服务检查
kubectl get svc,endpoints <service-name> -n <namespace>
kubectl get pods -l <selector> -n <namespace>
kubectl exec -it <pod> -- curl localhost:<port>/health

# TLS证书检查
kubectl get secret <tls-secret> -n <namespace> -o yaml
kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout

# 网络连通性测试
kubectl run tmp-debug --rm -it --image=curlimages/curl -- curl -v http://<service>.<namespace>.svc.cluster.local:<port>/
kubectl run tmp-netshoot --rm -it --image=nicolaka/netshoot -- /bin/bash
```

---

## 8. 总结与Q&A

### 8.1 核心要点回顾

**Ingress的价值**
- ✅ 统一的HTTP/HTTPS入口
- ✅ 灵活的路由规则配置
- ✅ 集中的TLS证书管理
- ✅ 丰富的流量治理能力
- ✅ 成熟的生态系统支持

**阿里云环境最佳实践**
- 🎯 专有云使用内网SLB降低成本
- 🎯 公共云优先选择ALB获得最佳性能
- 🎯 合理配置安全组和访问控制
- 🎯 启用监控告警确保稳定性
- 🎯 实施金丝雀发布降低风险

### 8.2 常见问题解答

**Q: 如何选择合适的Ingress Controller？**
A: 根据需求选择：
- 通用场景：NGINX Ingress
- 高性能要求：HAProxy或ALB
- 动态配置：Traefik
- 云原生环境：优先选择云厂商原生方案

**Q: Ingress如何实现高可用？**
A: 
- 部署多个Controller副本
- 跨可用区分布
- 配置反亲和性
- 使用云负载均衡器

**Q: 如何优化Ingress性能？**
A:
- 启用HTTP/2和压缩
- 合理配置缓存策略
- 优化代理缓冲区设置
- 启用连接复用

**Q: 专有云环境下如何配置外部访问？**
A:
- 通过NodePort或LoadBalancer Service
- 配置内网SLB结合反向代理
- 使用VPN或专线打通网络

**Q: 如何实现蓝绿部署？**
A:
- 使用不同的Service后端
- 通过Ingress权重调整流量
- 配合外部负载均衡器切换

### 8.3 学习资源推荐

**官方文档**
- Kubernetes Ingress文档: https://kubernetes.io/docs/concepts/services-networking/ingress/
- 阿里云ACK文档: https://help.aliyun.com/product/85222.html
- NGINX Ingress Controller: https://kubernetes.github.io/ingress-nginx/

**相关技术**
- Service Mesh服务网格
- NetworkPolicy网络安全
- cert-manager证书管理
- Prometheus监控系统

**实践建议**
1. 从简单场景开始，逐步增加复杂功能
2. 充分测试后再上线生产环境
3. 建立完善的监控告警体系
4. 定期回顾和优化配置

---

**感谢聆听！欢迎提问交流**

*本文档基于Kubernetes v1.25-v1.32版本编写，适用于阿里云专有云和公共云环境*