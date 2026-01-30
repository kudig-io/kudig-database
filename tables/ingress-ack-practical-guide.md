# Kubernetes Ingress ACK 实战技术指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **环境**: 阿里云专有云 & 公共云 | **重点产品**: ACK  
> **文档类型**: 技术实践指南 | **目标读者**: 运维工程师、架构师、DevOps工程师  

---

## 目录

1. [技术架构深度解析](#1-技术架构深度解析)
2. [生产级配置模板](#2-生产级配置模板)
3. [阿里云集成详解](#3-阿里云集成详解)
4. [性能优化指导](#4-性能优化指导)
5. [安全加固实践](#5-安全加固实践)
6. [故障排查手册](#6-故障排查手册)
7. [监控告警体系建设](#7-监控告警体系建设)

---

## 1. 技术架构深度解析

### 1.1 Ingress 控制器工作原理

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                            Ingress Controller 工作流程                           │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │ 1. 资源监听阶段 (Watch Phase)                                            │   │
│  │                                                                          │   │
│  │ Ingress Controller 通过 client-go Informer 监听:                          │   │
│  │   • Ingress 资源变化                                                     │   │
│  │   • Service 和 Endpoints 变化                                            │   │
│  │   • TLS Secret 证书更新                                                  │   │
│  │   • ConfigMap 全局配置变更                                               │   │
│  │                                                                          │   │
│  │ ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     │   │
│  │ │   Ingress   │  │   Service   │  │ Endpoints/  │  │   Secret    │     │   │
│  │ │  Watcher    │  │  Watcher    │  │EndpointSlice│  │   Watcher   │     │   │
│  │ └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘     │   │
│  │        │                │                │                │             │   │
│  │        └────────────────┼────────────────┼────────────────┘             │   │
│  │                         │                │                              │   │
│  │                         ▼                ▼                              │   │
│  │                ┌─────────────────────────────────┐                      │   │
│  │                │        Event Queue              │                      │   │
│  │                │      (WorkQueue)                │                      │   │
│  │                └─────────────────┬───────────────┘                      │   │
│  └───────────────────────────────────┼──────────────────────────────────────┘   │
│                                      │                                           │
│                                      ▼                                           │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │ 2. 配置同步阶段 (Sync Phase)                                             │   │
│  │                                                                          │   │
│  │ Sync Handler 处理流程:                                                    │   │
│  │   1. 从 Queue 获取事件                                                   │   │
│  │   2. 获取所有相关的 Ingress 资源                                         │   │
│  │   3. 过滤匹配当前 IngressClass 的 Ingress                                │   │
│  │   4. 解析 Ingress rules 和 annotations                                   │   │
│  │   5. 查询关联的 Service 和 Endpoints                                     │   │
│  │   6. 获取 TLS Secret 中的证书                                            │   │
│  │   7. 构建内部路由模型                                                     │   │
│  └───────────────────────────────────┬──────────────────────────────────────┘   │
│                                      │                                           │
│                                      ▼                                           │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │ 3. 配置生成阶段 (Config Generation)                                      │   │
│  │                                                                          │   │
│  │ 根据控制器类型生成对应的代理配置:                                          │   │
│  │                                                                          │   │
│  │ NGINX Controller:                                                     │   │
│  │   • 模板渲染生成 nginx.conf                                           │   │
│  │   • 支持 Lua 动态更新 Endpoints                                       │   │
│  │                                                                          │   │
│  │ ALB Controller:                                                       │   │
│  │   • 调用阿里云 API 创建/更新 ALB 配置                                 │   │
│  │   • 自动管理监听器、路由规则、证书绑定                                │   │
│  │                                                                          │   │
│  │ Envoy (Contour):                                                      │   │
│  │   • 通过 xDS API 热更新配置                                           │   │
│  │   • 无需重启，零停机配置变更                                          │   │
│  └───────────────────────────────────┬──────────────────────────────────────┘   │
│                                      │                                           │
│                                      ▼                                           │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │ 4. 配置应用阶段 (Apply Phase)                                            │   │
│  │                                                                          │   │
│  │ NGINX:                                                                │   │
│  │   • nginx -t 验证配置语法                                             │   │
│  │   • nginx -s reload 重载配置                                          │   │
│  │   • Endpoints 变更通过 Lua 动态更新                                   │   │
│  │                                                                          │   │
│  │ ALB:                                                                  │   │
│  │   • 异步调用阿里云 API                                                │   │
│  │   • 等待配置生效                                                      │   │
│  │   • 更新 Ingress status                                               │   │
│  │                                                                          │   │
│  │ Envoy:                                                                │   │
│  │   • xDS API 推送新配置                                                │   │
│  • 热更新，毫秒级生效                                                 │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 流量转发路径分析

| 转发模式 | 路径 | 数据流向 | 优点 | 缺点 | 适用场景 |
|----------|------|----------|------|------|----------|
| **Service转发** | Client → LB → IC → Service → kube-proxy → Pod | 传统模式，利用K8s原生负载均衡 | 简单可靠，兼容性好 | 额外跳转，延迟稍高 | 通用场景 |
| **Endpoints直连** | Client → LB → IC → Pod IP | 绕过kube-proxy，直接连接Pod | 延迟更低，性能更好 | 需要控制器支持 | 高性能要求 |
| **EndpointSlice** | Client → LB → IC → Pod IP | 基于EndpointSlice的现代方案 | 大规模集群优化，支持拓扑感知 | 需要v1.21+版本 | 现代集群 |

### 1.3 TLS 终止与证书管理

```
TLS握手完整流程:

1. Client Hello
   ├── 支持的TLS版本 (TLS 1.2/1.3)
   ├── 支持的加密套件
   ├── SNI主机名 (app.example.com)
   └── 其他扩展参数

2. Server Hello
   ├── 选定的TLS版本
   ├── 选定的加密套件
   ├── 服务器证书 (包含公钥)
   └── 密钥交换参数

3. 证书验证
   ├── 检查证书是否过期
   ├── 验证CA签名链
   ├── 检查域名匹配 (CN/SAN)
   └── 查询OCSP状态

4. 密钥交换
   ├── 客户端生成预主密钥
   ├── 用服务器公钥加密预主密钥
   └── 发送给服务器

5. 会话建立
   ├── 双方基于预主密钥生成会话密钥
   ├── 验证Finished消息
   └── 开始加密通信
```

---

## 2. 生产级配置模板

### 2.1 标准Web应用配置

```yaml
# 标准Web应用Ingress配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: webapp-production
  namespace: production
  labels:
    app: webapp
    environment: production
    tier: frontend
  annotations:
    # === TLS/SSL 配置 ===
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
    nginx.ingress.kubernetes.io/hsts: "true"
    nginx.ingress.kubernetes.io/hsts-max-age: "31536000"
    nginx.ingress.kubernetes.io/hsts-include-subdomains: "true"
    nginx.ingress.kubernetes.io/hsts-preload: "true"
    
    # === 代理配置 ===
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-buffer-size: "16k"
    nginx.ingress.kubernetes.io/proxy-buffers-number: "8"
    nginx.ingress.kubernetes.io/proxy-http-version: "1.1"
    
    # === 重试机制 ===
    nginx.ingress.kubernetes.io/proxy-next-upstream: "error timeout http_502 http_503 http_504"
    nginx.ingress.kubernetes.io/proxy-next-upstream-tries: "3"
    nginx.ingress.kubernetes.io/proxy-next-upstream-timeout: "30"
    
    # === 限流配置 ===
    nginx.ingress.kubernetes.io/limit-rps: "200"
    nginx.ingress.kubernetes.io/limit-connections: "100"
    nginx.ingress.kubernetes.io/limit-whitelist: "10.0.0.0/8,192.168.0.0/16"
    
    # === CORS 跨域支持 ===
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://app.example.com,https://admin.example.com"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, PATCH, OPTIONS"
    nginx.ingress.kubernetes.io/cors-allow-headers: "DNT,Keep-Alive,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range,Authorization,X-API-Key"
    nginx.ingress.kubernetes.io/cors-allow-credentials: "true"
    nginx.ingress.kubernetes.io/cors-max-age: "86400"
    
    # === 负载均衡策略 ===
    nginx.ingress.kubernetes.io/load-balance: "ewma"  # 指数加权移动平均
    
    # === 会话保持 ===
    nginx.ingress.kubernetes.io/affinity: "cookie"
    nginx.ingress.kubernetes.io/affinity-mode: "balanced"
    nginx.ingress.kubernetes.io/session-cookie-name: "SERVERID"
    nginx.ingress.kubernetes.io/session-cookie-max-age: "3600"
    nginx.ingress.kubernetes.io/session-cookie-path: "/"
    nginx.ingress.kubernetes.io/session-cookie-samesite: "Strict"
    
    # === 安全响应头 ===
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # 安全头
      add_header X-Frame-Options "SAMEORIGIN" always;
      add_header X-Content-Type-Options "nosniff" always;
      add_header X-XSS-Protection "1; mode=block" always;
      add_header Referrer-Policy "strict-origin-when-cross-origin" always;
      add_header Permissions-Policy "geolocation=(), microphone=(), camera=()" always;
      
      # 业务头
      add_header X-Request-ID $req_id always;
      add_header X-Backend-Server $upstream_addr always;
      add_header X-Response-Time $upstream_response_time always;
      
      # 缓存控制
      location ~* \.(css|js|jpg|jpeg|png|gif|ico|svg)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
      }
      
      # 压缩配置
      gzip on;
      gzip_vary on;
      gzip_min_length 1024;
      gzip_types text/plain text/css application/json application/javascript text/xml application/xml;
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - webapp.example.com
    - www.example.com
    secretName: webapp-tls
  rules:
  - host: webapp.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
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
              number: 3000
  - host: www.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend-service
            port:
              number: 3000
```

### 2.2 API服务配置

```yaml
# API服务专用Ingress配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-production
  namespace: production
  labels:
    app: api
    environment: production
    tier: backend
  annotations:
    # === TLS配置 ===
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/backend-protocol: "HTTP"
    
    # === 严格的代理配置 ===
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "5"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "120"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "120"
    nginx.ingress.kubernetes.io/proxy-buffer-size: "32k"
    nginx.ingress.kubernetes.io/proxy-buffers-number: "16"
    
    # === 严格的限流 ===
    nginx.ingress.kubernetes.io/limit-rps: "1000"
    nginx.ingress.kubernetes.io/limit-connections: "200"
    nginx.ingress.kubernetes.io/limit-rate: "1024"  # 1MB/s
    
    # === API特定配置 ===
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "*"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, PATCH, OPTIONS"
    nginx.ingress.kubernetes.io/cors-allow-headers: "Authorization,Content-Type,X-API-Key,X-Requested-With,Accept,Origin"
    
    # === 认证配置 ===
    nginx.ingress.kubernetes.io/auth-url: "http://auth-service.auth-system.svc.cluster.local/auth"
    nginx.ingress.kubernetes.io/auth-signin: "https://auth.example.com/login"
    nginx.ingress.kubernetes.io/auth-response-headers: "X-User-ID,X-User-Role"
    
    # === 监控集成 ===
    nginx.ingress.kubernetes.io/enable-opentracing: "true"
    nginx.ingress.kubernetes.io/opentracing-trust-incoming-span: "true"
    
    # === 日志增强 ===
    nginx.ingress.kubernetes.io/enable-access-log: "true"
    nginx.ingress.kubernetes.io/log-format-escape-json: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - api.example.com
    secretName: api-tls
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /v1/health
        pathType: Exact
        backend:
          service:
            name: health-check-service
            port:
              number: 8080
      - path: /v1/metrics
        pathType: Exact
        backend:
          service:
            name: metrics-service
            port:
              number: 8080
      - path: /v1/admin
        pathType: Prefix
        backend:
          service:
            name: admin-service
            port:
              number: 8080
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
```

### 2.3 微服务网关配置

```yaml
# 微服务网关模式配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: microservices-gateway
  namespace: gateway
  annotations:
    # === 网关级配置 ===
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    
    # === 路由重写 ===
    nginx.ingress.kubernetes.io/rewrite-target: /$2
    nginx.ingress.kubernetes.io/use-regex: "true"
    
    # === 服务发现 ===
    nginx.ingress.kubernetes.io/service-weight: "user-service=90,user-service-canary=10"
    
    # === 熔断配置 ===
    nginx.ingress.kubernetes.io/proxy-next-upstream: "error timeout invalid_header http_502 http_503 http_504"
    nginx.ingress.kubernetes.io/proxy-next-upstream-tries: "3"
    nginx.ingress.kubernetes.io/proxy-next-upstream-timeout: "10"
    
    # === 网关安全 ===
    nginx.ingress.kubernetes.io/rate-limit-connections: "1000"
    nginx.ingress.kubernetes.io/rate-limit-rps: "500"
    
    # === 请求追踪 ===
    nginx.ingress.kubernetes.io/enable-opentracing: "true"
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # 添加追踪头
      add_header X-Trace-ID $req_id always;
      add_header X-Gateway-Time $upstream_response_time always;
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - gateway.example.com
    secretName: gateway-tls
  rules:
  - host: gateway.example.com
    http:
      paths:
      # 用户服务
      - path: /api/users(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: user-service
            port:
              number: 8080
      
      # 订单服务
      - path: /api/orders(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: order-service
            port:
              number: 8080
      
      # 支付服务
      - path: /api/payments(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: payment-service
            port:
              number: 8080
      
      # 通知服务
      - path: /api/notifications(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: notification-service
            port:
              number: 8080
      
      # 默认路由到API网关
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-gateway
            port:
              number: 8080
```

---

## 3. 阿里云集成详解

### 3.1 ALB Ingress 原生集成

```yaml
# ALB Ingress 完整配置
apiVersion: alibabacloud.com/v1
kind: AlbConfig
metadata:
  name: production-alb
  namespace: kube-system
spec:
  config:
    name: production-alb
    addressType: Internet
    zoneMappings:
    - vSwitchId: vsw-xxx-zone-a
      zoneId: cn-hangzhou-a
    - vSwitchId: vsw-xxx-zone-b
      zoneId: cn-hangzhou-b
    accessLogConfig:
      logProject: k8s-access-logs
      logStore: alb-access-log
    deletionProtectionEnabled: true
    modificationProtectionStatus: ConsoleProtection
    loadBalancerBillingConfig:
      payType: PayAsYouGo
    tags:
    - key: Environment
      value: Production
    - key: Team
      value: Platform

---
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: alb
  annotations:
    ingressclass.kubernetes.io/is-default-class: "true"
spec:
  controller: ingress.k8s.alibabacloud/alb
  parameters:
    apiGroup: alibabacloud.com
    kind: AlbConfig
    name: production-alb

---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: alb-ingress-example
  namespace: production
  annotations:
    # === ALB特定配置 ===
    alb.ingress.kubernetes.io/address-type: internet
    alb.ingress.kubernetes.io/vswitch-ids: "vsw-xxx1,vsw-xxx2"
    
    # === 监听器配置 ===
    alb.ingress.kubernetes.io/listen-ports: '[{"HTTPS": 443}, {"HTTP": 80}]'
    alb.ingress.kubernetes.io/ssl-redirect: "true"
    
    # === 健康检查 ===
    alb.ingress.kubernetes.io/healthcheck-enabled: "true"
    alb.ingress.kubernetes.io/healthcheck-path: "/health"
    alb.ingress.kubernetes.io/healthcheck-protocol: "HTTP"
    alb.ingress.kubernetes.io/healthcheck-port: "8080"
    alb.ingress.kubernetes.io/healthcheck-interval-seconds: "5"
    alb.ingress.kubernetes.io/healthcheck-timeout-seconds: "3"
    alb.ingress.kubernetes.io/healthy-threshold-count: "3"
    alb.ingress.kubernetes.io/unhealthy-threshold-count: "3"
    
    # === 后端配置 ===
    alb.ingress.kubernetes.io/backend-protocol: "HTTP"
    alb.ingress.kubernetes.io/sticky-session: "true"
    alb.ingress.kubernetes.io/sticky-session-type: "Insert"
    alb.ingress.kubernetes.io/cookie-timeout: "1800"
    
    # === 性能配置 ===
    alb.ingress.kubernetes.io/idle-timeout: "60"
    alb.ingress.kubernetes.io/request-timeout: "60"
    
    # === 安全配置 ===
    alb.ingress.kubernetes.io/security-policy: "tls_security_policy_1_2_strict"
    
    # === 金丝雀发布 ===
    alb.ingress.kubernetes.io/canary: "true"
    alb.ingress.kubernetes.io/canary-by-header: "X-Canary"
    alb.ingress.kubernetes.io/canary-weight: "10"
    
    # === 限流配置 ===
    alb.ingress.kubernetes.io/traffic-limit-qps: "10000"
spec:
  ingressClassName: alb
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

### 3.2 SLB/NLB 集成配置

```yaml
# SLB Service 配置示例
apiVersion: v1
kind: Service
metadata:
  name: slb-service
  namespace: production
  annotations:
    # === 负载均衡器类型 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-type: "slb"
    
    # === 基础配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.medium"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "internet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-bandwidth: "100"
    
    # === 安全组配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-security-group-id: "sg-xxxxxxxxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-enable: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-type: "white"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-access-control-list: "192.168.0.0/16,10.0.0.0/8"
    
    # === 健康检查 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "tcp"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-port: "8080"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "5"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-healthy-threshold: "3"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-unhealthy-threshold: "3"
    
    # === 调度算法 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-scheduler: "wrr"
    
    # === 会话保持 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-sticky-session: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-sticky-session-type: "insert"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cookie-timeout: "1800"
    
    # === 删除保护 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-delete-protection: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-modification-protection: "ConsoleProtection"
spec:
  selector:
    app: my-app
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
  - name: https
    port: 443
    targetPort: 8443
    protocol: TCP
  type: LoadBalancer
  externalTrafficPolicy: Local
```

### 3.3 NLB 高性能配置

```yaml
# NLB Service 配置 (适用于高并发场景)
apiVersion: v1
kind: Service
metadata:
  name: nlb-high-performance
  namespace: production
  annotations:
    # === 使用NLB ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-type: "nlb"
    
    # === 高性能规格 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "nlb.s1.small"  # 按量付费
    
    # === 网络配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "internet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-zone-mapping: |
      [
        {"ZoneId": "cn-hangzhou-a", "VSwitchId": "vsw-xxx1"},
        {"ZoneId": "cn-hangzhou-b", "VSwitchId": "vsw-xxx2"}
      ]
    
    # === 健康检查增强 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-timeout: "3"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "2"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-healthy-threshold: "2"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-unhealthy-threshold: "2"
    
    # === 连接优化 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-connection-drain: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-connection-drain-timeout: "300"
    
    # === 安全配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-security-group-id: "sg-nlb-secure"
spec:
  selector:
    app: high-performance-app
  ports:
  - name: tcp-service
    port: 80
    targetPort: 8080
    protocol: TCP
  type: LoadBalancer
  externalTrafficPolicy: Local
```

---

## 4. 性能优化指导

### 4.1 NGINX Controller 性能调优

```yaml
# NGINX Controller 高性能配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # === 工作进程优化 ===
  worker-processes: "auto"  # CPU核心数
  worker-cpu-affinity: "auto"
  worker-shutdown-timeout: "300s"
  worker-rlimit-nofile: "65536"
  
  # === 连接性能优化 ===
  max-worker-connections: "65535"
  max-worker-open-files: "65535"
  reuse-port: "true"
  
  # === Keep-Alive 优化 ===
  keep-alive: "75"
  keep-alive-requests: "10000"
  keep-alive-timeout: "75"
  
  # === 上游连接池优化 ===
  upstream-keepalive-connections: "500"
  upstream-keepalive-requests: "10000"
  upstream-keepalive-timeout: "60"
  
  # === 缓冲区优化 ===
  proxy-buffer-size: "32k"
  proxy-buffers-number: "16"
  proxy-busy-buffers-size: "64k"
  proxy-max-temp-file-size: "0"
  
  # === 超时配置 ===
  proxy-connect-timeout: "10"
  proxy-read-timeout: "60"
  proxy-send-timeout: "60"
  proxy-next-upstream-timeout: "30"
  
  # === 请求体处理 ===
  client-body-buffer-size: "128k"
  client-max-body-size: "100m"
  client-header-buffer-size: "4k"
  large-client-header-buffers: "4 8k"
  
  # === 压缩优化 ===
  use-gzip: "true"
  gzip-level: "6"
  gzip-min-length: "1000"
  gzip-types: "text/plain text/css application/json application/javascript text/xml application/xml application/xml+rss text/javascript application/x-javascript"
  gzip-vary: "on"
  gzip-proxied: "any"
  
  # === HTTP/2 支持 ===
  use-http2: "true"
  http2-max-field-size: "16k"
  http2-max-header-size: "32k"
  
  # === 日志优化 ===
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
      "vhost": "$host",
      "request_proto": "$server_protocol",
      "path": "$uri",
      "request_query": "$args",
      "request_length": $request_length,
      "duration": $request_time,
      "method": "$request_method",
      "http_referrer": "$http_referer",
      "http_user_agent": "$http_user_agent",
      "upstream_addr": "$upstream_addr",
      "upstream_response_time": "$upstream_response_time",
      "upstream_response_length": "$upstream_response_length",
      "upstream_status": "$upstream_status",
      "server_name": "$server_name"
    }
  
  # === 安全优化 ===
  server-tokens: "false"
  hide-headers: "X-Powered-By,Server"
  
  # === 监控指标 ===
  enable-prometheus-metrics: "true"
  metrics-per-host: "true"
  
  # === 地理位置 ===
  enable-geoip: "true"
  geoip-country: "/etc/nginx/geoip/GeoIP.dat"
  geoip-city: "/etc/nginx/geoip/GeoLiteCity.dat"
```

### 4.2 资源请求与限制

```yaml
# Controller 资源优化配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
spec:
  replicas: 3
  selector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
      app.kubernetes.io/component: controller
  template:
    metadata:
      labels:
        app.kubernetes.io/name: ingress-nginx
        app.kubernetes.io/component: controller
    spec:
      containers:
      - name: controller
        image: registry.k8s.io/ingress-nginx/controller:v1.10.0
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
          limits:
            cpu: "2000m"
            memory: "2Gi"
        env:
        - name: LD_PRELOAD
          value: /usr/local/lib/libmimalloc.so  # 内存分配器优化
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          capabilities:
            drop:
            - ALL
            add:
            - NET_BIND_SERVICE
```

### 4.3 HPA 自动扩缩容

```yaml
# 基于指标的自动扩缩容
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: ingress-nginx-hpa
  namespace: ingress-nginx
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: ingress-nginx-controller
  minReplicas: 3
  maxReplicas: 20
  metrics:
  # CPU 使用率
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  
  # 内存使用率
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  
  # 自定义指标 - QPS
  - type: Pods
    pods:
      metric:
        name: nginx_ingress_controller_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"
  
  # 自定义指标 - 连接数
  - type: Pods
    pods:
      metric:
        name: nginx_ingress_controller_connections_active
      target:
        type: AverageValue
        averageValue: "5000"
  
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 100
        periodSeconds: 15
      - type: Pods
        value: 4
        periodSeconds: 15
      selectPolicy: Max
    
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
      selectPolicy: Min
```

---

## 5. 安全加固实践

### 5.1 网络安全策略

```yaml
# 严格网络策略配置
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: ingress-controller-network-policy
  namespace: ingress-nginx
spec:
  podSelector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
      app.kubernetes.io/component: controller
  policyTypes:
  - Ingress
  - Egress
  
  # 入站规则
  ingress:
  # 允许来自负载均衡器的流量
  - from:
    - ipBlock:
        cidr: 0.0.0.0/0  # 或者具体的SLB IP范围
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
  
  # 允许来自监控系统的流量
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 10254  # metrics端口
  
  # 允许节点到控制器的健康检查
  - from:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kubelet
    ports:
    - protocol: TCP
      port: 10254
  
  # 出站规则
  egress:
  # 允许访问后端服务
  - to:
    - namespaceSelector:
        matchLabels:
          ingress-allowed: "true"
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
    - protocol: TCP
      port: 8080
  
  # 允许DNS查询
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
  
  # 允许访问外部认证服务
  - to:
    - ipBlock:
        cidr: 10.0.0.0/8  # 内部网络
    ports:
    - protocol: TCP
      port: 80
    - protocol: TCP
      port: 443
```

### 5.2 TLS 安全配置

```yaml
# 安全的TLS配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # === TLS协议版本 ===
  ssl-protocols: "TLSv1.2 TLSv1.3"
  
  # === 安全加密套件 ===
  ssl-ciphers: |
    ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:
    ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:
    ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:
    DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384
  
  # === 安全选项 ===
  ssl-prefer-server-ciphers: "true"
  ssl-session-cache: "shared:SSL:50m"
  ssl-session-cache-size: "50m"
  ssl-session-timeout: "10m"
  ssl-session-tickets: "false"  # 禁用会话票据提高安全性
  
  # === HSTS 配置 ===
  hsts: "true"
  hsts-max-age: "31536000"  # 1年
  hsts-include-subdomains: "true"
  hsts-preload: "true"
  
  # === OCSP Stapling ===
  enable-ocsp: "true"
  ssl-stapling: "true"
  ssl-stapling-verify: "true"
  
  # === Diffie-Hellman 参数 ===
  ssl-dh-param: "ingress-nginx/dhparam"
```

### 5.3 WAF 集成配置

```yaml
# 阿里云WAF集成
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: waf-protected-ingress
  namespace: production
  annotations:
    # === 基础配置 ===
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    
    # === 自定义安全头 ===
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # WAF相关头
      add_header X-Content-Type-Options "nosniff" always;
      add_header X-Frame-Options "DENY" always;
      add_header X-XSS-Protection "1; mode=block" always;
      add_header Referrer-Policy "strict-origin-when-cross-origin" always;
      add_header Strict-Transport-Security "max-age=31536000; includeSubDomains; preload" always;
      add_header Content-Security-Policy "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data:;" always;
      
      # 防御性配置
      add_header X-Download-Options "noopen" always;
      add_header X-Permitted-Cross-Domain-Policies "none" always;
      
      # 移除危险头
      proxy_hide_header Server;
      proxy_hide_header X-Powered-By;
      
      # 请求大小限制
      client_max_body_size 10m;
      
      # 速率限制
      limit_req_zone $binary_remote_addr zone=login:10m rate=10r/m;
      limit_req_zone $binary_remote_addr zone=api:10m rate=1000r/m;
      
      # 登录接口限流
      location /api/login {
        limit_req zone=login burst=5 nodelay;
        proxy_pass http://login-service;
      }
      
      # API接口限流
      location /api/ {
        limit_req zone=api burst=100 nodelay;
        proxy_pass http://api-service;
      }
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
            name: app-service
            port:
              number: 80
```

---

## 6. 故障排查手册

### 6.1 系统性诊断流程

```bash
#!/bin/bash
# Ingress 故障诊断脚本

echo "=== Ingress 故障诊断报告 ==="
echo "诊断时间: $(date)"
echo "Kubernetes版本: $(kubectl version --short)"
echo

# 1. 检查Ingress Controller状态
echo "1. Ingress Controller 状态检查"
echo "--------------------------------"
kubectl get pods -n ingress-nginx -o wide
echo
kubectl get svc -n ingress-nginx
echo
kubectl describe deployment -n ingress-nginx ingress-nginx-controller | grep -A 10 "Conditions:"
echo

# 2. 检查Ingress资源配置
echo "2. Ingress 资源检查"
echo "--------------------"
kubectl get ingress -A
echo
kubectl get ingressclass
echo
for ingress in $(kubectl get ingress -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'); do
  echo "检查 Ingress: $ingress"
  kubectl describe ingress -n ${ingress%/*} ${ingress#*/} | grep -E "(Events|Warning|Error)"
  echo "---"
done
echo

# 3. 检查后端服务状态
echo "3. 后端服务状态检查"
echo "--------------------"
for svc in $(kubectl get svc -n production -o jsonpath='{.items[*].metadata.name}'); do
  echo "检查 Service: $svc"
  kubectl get endpoints $svc -n production
  echo "---"
done
echo

# 4. 检查配置重载状态
echo "4. 配置重载状态"
echo "---------------"
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- curl -s localhost:10254/metrics | grep "config_last_reload"
echo

# 5. 检查证书状态
echo "5. TLS证书状态"
echo "--------------"
kubectl get secret -n production -o jsonpath='{range .items[?(@.type=="kubernetes.io/tls")]}{.metadata.name}{"\n"}{end}' | while read secret; do
  echo "检查证书: $secret"
  exp_date=$(kubectl get secret $secret -n production -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -enddate | cut -d= -f2)
  exp_timestamp=$(date -d "$exp_date" +%s)
  now_timestamp=$(date +%s)
  days_left=$(( (exp_timestamp - now_timestamp) / 86400 ))
  echo "  过期时间: $exp_date"
  echo "  剩余天数: $days_left"
  if [ $days_left -lt 30 ]; then
    echo "  ⚠️  警告: 证书将在30天内过期"
  fi
  echo "---"
done
echo

# 6. 检查网络连通性
echo "6. 网络连通性测试"
echo "------------------"
kubectl run debug-pod --rm -it --image=curlimages/curl --restart=Never -- curl -I http://app-service.production.svc.cluster.local
echo

# 7. 检查资源使用情况
echo "7. 资源使用情况"
echo "---------------"
kubectl top pods -n ingress-nginx
echo
kubectl get hpa -n ingress-nginx
echo

# 8. 收集关键日志
echo "8. 关键错误日志"
echo "---------------"
kubectl logs -n ingress-nginx -l app.kubernetes.io/component=controller --since=1h | grep -i "error\|warning\|failed" | tail -20
```

### 6.2 常见故障排查命令

```bash
# ========== 基础状态检查 ==========
# 查看所有Ingress资源
kubectl get ingress -A

# 查看Ingress详细信息
kubectl describe ingress <ingress-name> -n <namespace>

# 查看Ingress事件
kubectl get events --field-selector involvedObject.kind=Ingress,involvedObject.name=<ingress-name>

# 查看Ingress Controller Pod状态
kubectl get pods -n ingress-nginx -o wide

# ========== 配置验证 ==========
# 检查NGINX配置语法
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- nginx -t

# 查看生成的NGINX配置
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- cat /etc/nginx/nginx.conf

# 查看特定server配置
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- cat /etc/nginx/nginx.conf | grep -A 50 "server_name example.com"

# ========== 后端服务检查 ==========
# 检查Service和Endpoints
kubectl get svc,endpoints <service-name> -n <namespace>

# 测试后端连通性
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- curl -v http://<service>.<namespace>.svc.cluster.local:<port>/

# 从临时Pod测试
kubectl run tmp-debug --rm -it --image=curlimages/curl -- curl -v http://<service>.<namespace>.svc.cluster.local:<port>/

# ========== TLS证书检查 ==========
# 查看证书Secret
kubectl get secret <tls-secret> -n <namespace> -o yaml

# 验证证书内容
kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout

# 检查证书过期时间
kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -dates -noout

# 测试HTTPS连接
curl -vI https://<domain> 2>&1 | grep -E "SSL|subject|issuer|expire"

# ========== 性能监控 ==========
# 查看控制器资源使用
kubectl top pod -n ingress-nginx

# 查看NGINX状态
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- curl -s localhost:10246/nginx_status

# 查看Prometheus指标
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- curl -s localhost:10254/metrics | grep nginx_ingress

# ========== 网络诊断 ==========
# DNS解析测试
kubectl run tmp-dns --rm -it --image=busybox -- nslookup <service>.<namespace>.svc.cluster.local

# 端口连通性测试
kubectl run tmp-nc --rm -it --image=nicolaka/netshoot -- nc -zv <service> <port>

# 完整网络诊断
kubectl run tmp-netshoot --rm -it --image=nicolaka/netshoot -- /bin/bash
```

### 6.3 故障排除决策树

```
问题诊断决策树:

入口问题 (无法访问服务)
├── DNS解析失败
│   ├── 检查域名解析记录
│   ├── 验证DNS配置
│   └── 检查本地hosts文件
├── 网络连通性问题
│   ├── ping测试负载均衡器IP
│   ├── telnet测试端口连通性
│   └── 检查安全组和防火墙规则
├── 负载均衡器故障
│   ├── 检查SLB/ALB状态
│   ├── 验证后端服务器健康检查
│   └── 检查负载均衡器配置
└── Ingress Controller问题
    ├── 检查Controller Pod状态
    ├── 查看Controller日志
    ├── 验证配置重载状态
    └── 检查资源使用情况

配置问题 (路由不正确)
├── Ingress资源配置错误
│   ├── 验证YAML语法
│   ├── 检查host和path配置
│   ├── 确认IngressClass匹配
│   └── 验证Service名称和端口
├── 后端服务配置问题
│   ├── 检查Service Selector
│   ├── 验证Endpoints状态
│   ├── 测试后端服务可用性
│   └── 检查Pod就绪状态
└── TLS/证书配置错误
    ├── 验证证书Secret存在
    ├── 检查证书有效期
    ├── 验证域名匹配
    └── 测试证书链完整性

性能问题 (响应慢/超时)
├── Controller性能瓶颈
│   ├── 检查CPU/Memory使用率
│   ├── 验证HPA配置
│   ├── 调整资源配置
│   └── 优化NGINX参数
├── 后端服务性能问题
│   ├── 检查后端应用性能
│   ├── 验证数据库连接
│   ├── 分析慢查询日志
│   └── 优化应用代码
└── 网络延迟问题
    ├── 检查跨可用区延迟
    ├── 验证网络策略
    ├── 测试不同路径延迟
    └── 考虑使用就近部署
```

---

## 7. 监控告警体系建设

### 7.1 Prometheus 监控指标

```yaml
# ServiceMonitor 配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: ingress-nginx-controller
  namespace: monitoring
  labels:
    app: ingress-nginx
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
  namespaceSelector:
    matchNames:
    - ingress-nginx
  endpoints:
  - port: metrics
    interval: 15s
    path: /metrics
    relabelings:
    - sourceLabels: [__meta_kubernetes_pod_node_name]
      targetLabel: node
    - sourceLabels: [__meta_kubernetes_namespace]
      targetLabel: namespace
    - sourceLabels: [__meta_kubernetes_pod_name]
      targetLabel: pod
```

### 7.2 关键告警规则

```yaml
# Ingress核心告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ingress-alerts
  namespace: monitoring
spec:
  groups:
  - name: ingress-availability
    rules:
    # Controller不可用
    - alert: IngressControllerDown
      expr: |
        absent(nginx_ingress_controller_nginx_process_connections)
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Ingress Controller不可用"
        description: "Ingress Controller无法获取指标，可能已停止运行"
    
    # 高错误率
    - alert: IngressHighErrorRate
      expr: |
        sum(rate(nginx_ingress_controller_requests{status=~"5.."}[5m])) 
        / sum(rate(nginx_ingress_controller_requests[5m])) > 0.01
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress 5xx错误率过高"
        description: "5xx错误率: {{ $value | humanizePercentage }}"
    
    # 严重错误率
    - alert: IngressCriticalErrorRate
      expr: |
        sum(rate(nginx_ingress_controller_requests{status=~"5.."}[5m])) 
        / sum(rate(nginx_ingress_controller_requests[5m])) > 0.05
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Ingress 5xx错误率严重"
        description: "5xx错误率超过5%: {{ $value | humanizePercentage }}"
  
  - name: ingress-performance
    rules:
    # 高延迟
    - alert: IngressHighLatency
      expr: |
        histogram_quantile(0.99, sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[5m])) by (le, ingress)) > 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress P99延迟过高"
        description: "P99延迟: {{ $value | humanizeDuration }}"
    
    # 高连接数
    - alert: IngressHighConnections
      expr: |
        nginx_ingress_controller_nginx_process_connections{state="active"} > 10000
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Ingress活跃连接数过高"
        description: "活跃连接数: {{ $value }}"
  
  - name: ingress-resources
    rules:
    # 资源使用率过高
    - alert: IngressHighCPU
      expr: |
        sum(rate(container_cpu_usage_seconds_total{namespace="ingress-nginx",container="controller"}[5m])) 
        / sum(kube_pod_container_resource_limits{namespace="ingress-nginx",container="controller",resource="cpu"}) > 0.8
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Ingress Controller CPU使用率过高"
        description: "CPU使用率: {{ $value | humanizePercentage }}"
    
    - alert: IngressHighMemory
      expr: |
        sum(container_memory_working_set_bytes{namespace="ingress-nginx",container="controller"}) 
        / sum(kube_pod_container_resource_limits{namespace="ingress-nginx",container="controller",resource="memory"}) > 0.8
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Ingress Controller内存使用率过高"
        description: "内存使用率: {{ $value | humanizePercentage }}"
  
  - name: ingress-certificates
    rules:
    # 证书即将过期
    - alert: IngressCertificateExpiringSoon
      expr: |
        (nginx_ingress_controller_ssl_expire_time_seconds - time()) < 2592000
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "Ingress证书即将过期"
        description: "证书 {{ $labels.host }} 将在 {{ $value | humanizeDuration }} 后过期"
    
    # 证书紧急过期
    - alert: IngressCertificateExpiringCritical
      expr: |
        (nginx_ingress_controller_ssl_expire_time_seconds - time()) < 604800
      for: 1h
      labels:
        severity: critical
      annotations:
        summary: "Ingress证书即将过期(紧急)"
        description: "证书 {{ $labels.host }} 将在 {{ $value | humanizeDuration }} 后过期，请立即更新！"
```

### 7.3 Grafana 仪表板配置

```json
{
  "dashboard": {
    "title": "Ingress Controller 监控大盘",
    "panels": [
      {
        "title": "总体请求量",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(nginx_ingress_controller_requests[5m]))",
            "legendFormat": "总QPS"
          }
        ]
      },
      {
        "title": "错误率分布",
        "type": "graph",
        "targets": [
          {
            "expr": "sum by (status) (rate(nginx_ingress_controller_requests{status=~\"4..|5..\"}[5m]))",
            "legendFormat": "{{status}}"
          }
        ]
      },
      {
        "title": "响应延迟(P99)",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.99, sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[5m])) by (le, ingress))",
            "legendFormat": "{{ingress}}"
          }
        ]
      },
      {
        "title": "活跃连接数",
        "type": "stat",
        "targets": [
          {
            "expr": "nginx_ingress_controller_nginx_process_connections{state=\"active\"}",
            "legendFormat": "活跃连接"
          }
        ]
      },
      {
        "title": "CPU使用率",
        "type": "gauge",
        "targets": [
          {
            "expr": "sum(rate(container_cpu_usage_seconds_total{namespace=\"ingress-nginx\",container=\"controller\"}[5m])) / sum(kube_pod_container_resource_limits{namespace=\"ingress-nginx\",container=\"controller\",resource=\"cpu\"}) * 100",
            "legendFormat": "CPU使用率"
          }
        ]
      }
    ]
  }
}
```

---

**文档结束**

*本技术指南提供了Kubernetes Ingress在阿里云环境下的完整实践方案，涵盖了从基础配置到高级优化的各个方面。建议结合实际业务场景进行配置调整，并建立完善的监控告警体系确保系统稳定运行。*