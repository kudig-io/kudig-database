# 128 - Ingress Controller 深入剖析

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01

---

## 一、Ingress Controller 架构总览

### 1.1 Ingress Controller 核心组件

| 组件 | 英文名 | 作用 | 实现方式 |
|-----|-------|------|---------|
| **Watch Handler** | 监听处理器 | 监听 K8s API 资源变化 | client-go informer |
| **Config Generator** | 配置生成器 | 生成代理配置 | 模板渲染/动态API |
| **Config Validator** | 配置验证器 | 验证配置正确性 | 语法/语义检查 |
| **Reload Manager** | 重载管理器 | 触发配置重载 | Signal/API 调用 |
| **Proxy Engine** | 代理引擎 | 实际处理流量 | NGINX/Envoy/HAProxy |
| **Health Checker** | 健康检查器 | 监控后端健康 | 主动/被动检查 |
| **Metrics Exporter** | 指标导出器 | 暴露监控指标 | Prometheus 格式 |

### 1.2 Ingress Controller 工作流程

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          Ingress Controller 架构                                │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                        Kubernetes API Server                             │    │
│  └───────────────────────────────┬─────────────────────────────────────────┘    │
│                                  │                                               │
│                    Watch (Ingress/Service/Endpoints/Secret)                      │
│                                  │                                               │
│                                  ▼                                               │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                          Controller Manager                              │    │
│  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐                │    │
│  │  │  Ingress      │  │  Service      │  │  Secret       │                │    │
│  │  │  Informer     │  │  Informer     │  │  Informer     │                │    │
│  │  └───────┬───────┘  └───────┬───────┘  └───────┬───────┘                │    │
│  │          │                  │                  │                         │    │
│  │          └──────────────────┼──────────────────┘                         │    │
│  │                             │                                            │    │
│  │                             ▼                                            │    │
│  │                    ┌─────────────────┐                                   │    │
│  │                    │  Event Queue    │                                   │    │
│  │                    └────────┬────────┘                                   │    │
│  │                             │                                            │    │
│  │                             ▼                                            │    │
│  │                    ┌─────────────────┐                                   │    │
│  │                    │  Sync Handler   │                                   │    │
│  │                    └────────┬────────┘                                   │    │
│  └─────────────────────────────┼────────────────────────────────────────────┘    │
│                                │                                                 │
│                                ▼                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                       Config Generator                                   │    │
│  │  ┌─────────────────────────────────────────────────────────────────┐    │    │
│  │  │  Template Engine (Go Template / Lua / JSON)                     │    │    │
│  │  │    - Server blocks                                              │    │    │
│  │  │    - Location rules                                             │    │    │
│  │  │    - Upstream definitions                                       │    │    │
│  │  │    - TLS configurations                                         │    │    │
│  │  └─────────────────────────────────────────────────────────────────┘    │    │
│  └─────────────────────────────┬────────────────────────────────────────────┘    │
│                                │                                                 │
│                                ▼                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                         Proxy Engine                                     │    │
│  │  ┌─────────────────────────────────────────────────────────────────┐    │    │
│  │  │  NGINX / Envoy / HAProxy / Traefik                              │    │    │
│  │  │    - TLS Termination                                            │    │    │
│  │  │    - Load Balancing                                             │    │    │
│  │  │    - Rate Limiting                                              │    │    │
│  │  │    - Request Routing                                            │    │    │
│  │  └─────────────────────────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、主流 Ingress Controller 全面对比

### 2.1 基础能力对比

| 控制器 | 代理引擎 | 开发语言 | 许可证 | 维护方 | 首次发布 |
|-------|---------|---------|-------|-------|---------|
| **NGINX Ingress (K8s)** | NGINX | Go | Apache-2.0 | Kubernetes 社区 | 2016 |
| **NGINX Ingress (F5)** | NGINX Plus | Go | Apache-2.0 | F5/NGINX | 2018 |
| **Traefik** | Traefik | Go | MIT | Traefik Labs | 2016 |
| **HAProxy Ingress** | HAProxy | Go | Apache-2.0 | HAProxy 社区 | 2017 |
| **Contour** | Envoy | Go | Apache-2.0 | VMware/CNCF | 2018 |
| **Kong Ingress** | Kong/NGINX | Go | Apache-2.0 | Kong Inc | 2018 |
| **Ambassador/Emissary** | Envoy | Python/Go | Apache-2.0 | Ambassador Labs | 2017 |
| **ALB Ingress (阿里云)** | ALB | Go | - | 阿里云 | 2020 |
| **AWS ALB Ingress** | AWS ALB | Go | Apache-2.0 | AWS | 2018 |
| **GCE Ingress** | Google Cloud LB | Go | Apache-2.0 | Google | 2016 |
| **Skipper** | Skipper | Go | Apache-2.0 | Zalando | 2016 |
| **Voyager** | HAProxy | Go | Apache-2.0 | AppsCode | 2017 |

### 2.2 功能特性对比

| 特性 | NGINX (K8s) | Traefik | HAProxy | Contour | Kong | ALB (阿里云) |
|-----|------------|---------|---------|---------|------|-------------|
| **协议支持** |
| HTTP/1.1 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| HTTP/2 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| HTTP/3 (QUIC) | ✅ (实验) | ✅ | ✅ | ❌ | ❌ | ✅ |
| gRPC | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| WebSocket | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| TCP/UDP | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **TLS 功能** |
| TLS 终止 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| TLS 透传 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| mTLS | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| SNI 路由 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| OCSP Stapling | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **路由能力** |
| 基于主机 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 基于路径 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 基于 Header | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 基于 Cookie | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| 基于查询参数 | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| 正则路径 | ✅ | ✅ | ✅ | ❌ | ✅ | ❌ |
| **流量管理** |
| 负载均衡 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 会话亲和 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 流量分割 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 金丝雀发布 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 蓝绿部署 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| A/B 测试 | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| 熔断 | ❌ | ✅ | ✅ | ✅ | ✅ | ❌ |
| 重试 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **安全功能** |
| 限流 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| IP 黑白名单 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Basic Auth | ✅ | ✅ | ✅ | ❌ | ✅ | ❌ |
| OAuth/OIDC | 插件 | ✅ | ❌ | ❌ | ✅ | ❌ |
| JWT 验证 | 插件 | 插件 | ❌ | ❌ | ✅ | ❌ |
| WAF | ModSecurity | 插件 | ❌ | ❌ | 插件 | ✅ |
| CORS | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **自定义 CRD** |
| 专用 CRD | ❌ | IngressRoute | ❌ | HTTPProxy | KongIngress | AlbConfig |
| Gateway API | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

### 2.3 性能与资源对比

| 指标 | NGINX (K8s) | Traefik | HAProxy | Contour (Envoy) | Kong |
|-----|------------|---------|---------|-----------------|------|
| **吞吐量 (RPS)** | 50,000+ | 30,000+ | 100,000+ | 50,000+ | 40,000+ |
| **延迟 (P99)** | <10ms | <15ms | <5ms | <10ms | <15ms |
| **并发连接** | 10,000+ | 10,000+ | 50,000+ | 10,000+ | 10,000+ |
| **内存占用** | 200-500MB | 100-300MB | 100-200MB | 300-600MB | 300-600MB |
| **CPU 使用** | 中 | 低 | 低 | 中 | 中-高 |
| **启动时间** | 2-5s | 1-3s | 1-2s | 3-5s | 5-10s |
| **配置重载** | 热重载 | 动态 | 热重载 | 动态 (xDS) | 动态 |
| **重载耗时** | 100-500ms | 0ms | <100ms | 0ms | <100ms |
| **连接保持** | 短暂丢失 | 无影响 | 无影响 | 无影响 | 短暂丢失 |

### 2.4 运维与生态对比

| 方面 | NGINX (K8s) | Traefik | HAProxy | Contour | Kong | ALB (阿里云) |
|-----|------------|---------|---------|---------|------|-------------|
| **学习曲线** | 中 | 低 | 中 | 中 | 高 | 低 |
| **文档质量** | 优秀 | 优秀 | 良好 | 良好 | 优秀 | 良好 |
| **社区活跃度** | 非常高 | 高 | 中 | 中 | 高 | 中 |
| **GitHub Stars** | 16k+ | 47k+ | 1k+ | 3k+ | 37k+ | - |
| **商业支持** | F5 | Traefik Labs | HAProxy Tech | VMware | Kong Inc | 阿里云 |
| **企业版** | NGINX Plus | Traefik EE | HAProxy EE | - | Kong EE | 原生 |
| **Prometheus 集成** | ✅ | ✅ | ✅ | ✅ | ✅ | 云监控 |
| **分布式追踪** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **日志格式** | 可定制 | JSON | 可定制 | JSON | JSON | 云日志 |
| **Helm Chart** | 官方 | 官方 | 社区 | 官方 | 官方 | - |

---

## 三、各控制器深入配置

### 3.1 NGINX Ingress Controller

#### 部署方式

```yaml
# Helm 部署 NGINX Ingress Controller
# helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
# helm install ingress-nginx ingress-nginx/ingress-nginx -f values.yaml

# values.yaml 核心配置
controller:
  name: controller
  image:
    repository: registry.k8s.io/ingress-nginx/controller
    tag: "v1.10.0"
  
  # 副本数与高可用
  replicaCount: 3
  
  # 资源配置
  resources:
    requests:
      cpu: 100m
      memory: 128Mi
    limits:
      cpu: 1000m
      memory: 512Mi
  
  # 部署策略
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
  
  # 反亲和性
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchExpressions:
          - key: app.kubernetes.io/component
            operator: In
            values:
            - controller
        topologyKey: kubernetes.io/hostname
  
  # 服务类型
  service:
    type: LoadBalancer
    externalTrafficPolicy: Local  # 保留源 IP
    annotations:
      service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: slb.s2.medium
  
  # 配置参数
  config:
    # 工作进程
    worker-processes: "auto"
    worker-connections: "65535"
    # 超时设置
    proxy-connect-timeout: "10"
    proxy-read-timeout: "60"
    proxy-send-timeout: "60"
    # 请求体大小
    proxy-body-size: "100m"
    # 日志格式
    log-format-upstream: '$remote_addr - $request_id - [$time_local] "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent" $request_length $request_time [$proxy_upstream_name] [$proxy_alternative_upstream_name] $upstream_addr $upstream_response_length $upstream_response_time $upstream_status $req_id'
    # 启用指标
    enable-prometheus-metrics: "true"
    # SSL 配置
    ssl-protocols: "TLSv1.2 TLSv1.3"
    ssl-ciphers: "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384"
  
  # 指标
  metrics:
    enabled: true
    serviceMonitor:
      enabled: true
  
  # 准入控制器
  admissionWebhooks:
    enabled: true
```

#### ConfigMap 全局配置

| 配置项 | 默认值 | 说明 | 生产建议 |
|-------|-------|------|---------|
| `worker-processes` | auto | 工作进程数 | CPU 核数 |
| `worker-connections` | 16384 | 单进程最大连接 | 65535 |
| `keep-alive` | 75 | Keep-alive 超时 (秒) | 75 |
| `keep-alive-requests` | 1000 | 单连接最大请求数 | 10000 |
| `proxy-connect-timeout` | 5 | 后端连接超时 (秒) | 5-10 |
| `proxy-read-timeout` | 60 | 后端读取超时 (秒) | 60-300 |
| `proxy-send-timeout` | 60 | 后端发送超时 (秒) | 60 |
| `proxy-body-size` | 1m | 请求体大小限制 | 根据业务 |
| `proxy-buffer-size` | 4k | 代理缓冲区大小 | 8k-16k |
| `ssl-protocols` | TLSv1.2 TLSv1.3 | 支持的 TLS 版本 | TLSv1.2 TLSv1.3 |
| `ssl-session-cache` | true | SSL 会话缓存 | true |
| `ssl-session-timeout` | 10m | SSL 会话超时 | 10m |
| `use-gzip` | true | 启用 gzip 压缩 | true |
| `gzip-level` | 1 | gzip 压缩级别 | 4-6 |
| `enable-brotli` | false | 启用 Brotli 压缩 | true |
| `use-forwarded-headers` | false | 使用 X-Forwarded-* 头 | 按需开启 |
| `compute-full-forwarded-for` | false | 完整转发链 | true |
| `enable-real-ip` | true | 启用真实 IP | true |

### 3.2 Traefik

#### 部署配置

```yaml
# Traefik Helm values.yaml
# helm repo add traefik https://traefik.github.io/charts
# helm install traefik traefik/traefik -f values.yaml

deployment:
  replicas: 3

resources:
  requests:
    cpu: "100m"
    memory: "128Mi"
  limits:
    cpu: "500m"
    memory: "512Mi"

# 入口点配置
ports:
  web:
    port: 8000
    exposedPort: 80
    protocol: TCP
  websecure:
    port: 8443
    exposedPort: 443
    protocol: TCP
    tls:
      enabled: true

# 提供者配置
providers:
  kubernetesIngress:
    enabled: true
    allowExternalNameServices: true
  kubernetesCRD:
    enabled: true
    allowCrossNamespace: true

# 日志配置
logs:
  general:
    level: INFO
    format: json
  access:
    enabled: true
    format: json

# 指标
metrics:
  prometheus:
    entryPoint: metrics
    addEntryPointsLabels: true
    addServicesLabels: true

# 追踪
tracing:
  jaeger:
    samplingServerURL: http://jaeger-agent:5778/sampling
    localAgentHostPort: jaeger-agent:6831

# 中间件
additionalArguments:
  - "--api.dashboard=true"
  - "--ping=true"
  - "--entrypoints.web.http.redirections.entrypoint.to=websecure"
  - "--entrypoints.web.http.redirections.entrypoint.scheme=https"
```

#### Traefik IngressRoute (CRD)

```yaml
# Traefik 专用 CRD，更强大的路由能力
apiVersion: traefik.io/v1alpha1
kind: IngressRoute
metadata:
  name: app-route
  namespace: production
spec:
  entryPoints:
    - websecure
  routes:
    - match: Host(`app.example.com`) && PathPrefix(`/api`)
      kind: Rule
      priority: 100
      middlewares:
        - name: rate-limit
        - name: strip-prefix
      services:
        - name: api-service
          port: 8080
          weight: 90
        - name: api-canary
          port: 8080
          weight: 10
    - match: Host(`app.example.com`)
      kind: Rule
      services:
        - name: web-service
          port: 80
  tls:
    secretName: app-tls
---
# 中间件定义
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: rate-limit
spec:
  rateLimit:
    average: 100
    burst: 200
---
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: strip-prefix
spec:
  stripPrefix:
    prefixes:
      - /api
```

### 3.3 Contour (Envoy)

#### 部署配置

```yaml
# Contour 配置
# helm repo add bitnami https://charts.bitnami.com/bitnami
# helm install contour bitnami/contour -f values.yaml

contour:
  replicaCount: 2
  resources:
    requests:
      cpu: 100m
      memory: 128Mi
    limits:
      cpu: 500m
      memory: 512Mi

envoy:
  replicaCount: 3
  resources:
    requests:
      cpu: 100m
      memory: 128Mi
    limits:
      cpu: 1000m
      memory: 512Mi
  
  service:
    type: LoadBalancer
    externalTrafficPolicy: Local

# 配置文件
configInline:
  gateway:
    controllerName: projectcontour.io/gateway-controller
  rateLimitService:
    extensionService: projectcontour/ratelimit
  accesslog-format: json
  json-fields:
    - "@timestamp"
    - "authority"
    - "bytes_received"
    - "bytes_sent"
    - "duration"
    - "method"
    - "path"
    - "protocol"
    - "request_id"
    - "response_code"
    - "upstream_cluster"
    - "upstream_host"
    - "user_agent"
    - "x_forwarded_for"
```

#### HTTPProxy (Contour CRD)

```yaml
# HTTPProxy - Contour 专用 CRD
apiVersion: projectcontour.io/v1
kind: HTTPProxy
metadata:
  name: app-proxy
  namespace: production
spec:
  virtualhost:
    fqdn: app.example.com
    tls:
      secretName: app-tls
      minimumProtocolVersion: "1.2"
  routes:
    # API 路由 - 带重试和超时
    - conditions:
        - prefix: /api
      services:
        - name: api-service
          port: 8080
          weight: 90
        - name: api-canary
          port: 8080
          weight: 10
      timeoutPolicy:
        response: 60s
        idle: 120s
      retryPolicy:
        count: 3
        perTryTimeout: 10s
      loadBalancerPolicy:
        strategy: WeightedLeastRequest
      healthCheckPolicy:
        path: /health
        intervalSeconds: 5
        timeoutSeconds: 2
        unhealthyThresholdCount: 3
        healthyThresholdCount: 2
    # 默认路由
    - services:
        - name: web-service
          port: 80
---
# 限流配置
apiVersion: projectcontour.io/v1alpha1
kind: RateLimitPolicy
metadata:
  name: api-rate-limit
spec:
  virtualHostRateLimits:
    local:
      requests: 100
      unit: second
      burst: 200
```

### 3.4 Kong Ingress Controller

#### 部署配置

```yaml
# Kong Helm values.yaml
# helm repo add kong https://charts.konghq.com
# helm install kong kong/kong -f values.yaml

image:
  repository: kong
  tag: "3.6"

env:
  database: "off"  # DB-less 模式
  nginx_worker_processes: "2"
  proxy_access_log: /dev/stdout
  admin_access_log: /dev/stdout
  proxy_error_log: /dev/stderr
  admin_error_log: /dev/stderr
  prefix: /kong_prefix/

ingressController:
  enabled: true
  installCRDs: false
  resources:
    requests:
      cpu: 100m
      memory: 128Mi
    limits:
      cpu: 500m
      memory: 512Mi

proxy:
  enabled: true
  type: LoadBalancer
  http:
    enabled: true
    containerPort: 8000
    servicePort: 80
  tls:
    enabled: true
    containerPort: 8443
    servicePort: 443

resources:
  requests:
    cpu: 500m
    memory: 512Mi
  limits:
    cpu: 2000m
    memory: 2Gi
```

#### Kong 插件配置

```yaml
# Kong 全局插件 - 限流
apiVersion: configuration.konghq.com/v1
kind: KongClusterPlugin
metadata:
  name: global-rate-limiting
  annotations:
    kubernetes.io/ingress.class: kong
  labels:
    global: "true"
plugin: rate-limiting
config:
  minute: 1000
  policy: local
---
# Kong Ingress 特定配置
apiVersion: configuration.konghq.com/v1
kind: KongIngress
metadata:
  name: api-config
proxy:
  protocol: http
  path: /
  connect_timeout: 10000
  read_timeout: 60000
  write_timeout: 60000
  retries: 3
route:
  strip_path: true
  preserve_host: true
upstream:
  algorithm: round-robin
  hash_on: none
  healthchecks:
    active:
      http_path: /health
      healthy:
        interval: 5
        successes: 2
      unhealthy:
        interval: 5
        http_failures: 3
---
# 应用插件
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-ingress
  annotations:
    konghq.com/override: api-config
    konghq.com/plugins: global-rate-limiting
spec:
  ingressClassName: kong
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

### 3.5 ALB Ingress Controller (阿里云)

#### 部署与配置

```yaml
# ALB Ingress Controller 组件配置 (通过 ACK 控制台安装)
# AlbConfig - 定义 ALB 实例
apiVersion: alibabacloud.com/v1
kind: AlbConfig
metadata:
  name: production-alb
spec:
  config:
    name: production-alb
    addressType: Internet
    zoneMappings:
      - vSwitchId: vsw-xxx1
        zoneId: cn-hangzhou-a
      - vSwitchId: vsw-xxx2
        zoneId: cn-hangzhou-b
    accessLogConfig:
      logProject: k8s-log
      logStore: alb-access-log
    deletionProtectionEnabled: true
    addressAllocatedMode: Dynamic
    loadBalancerBillingConfig:
      payType: PayAsYouGo
---
# IngressClass
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: alb
spec:
  controller: ingress.k8s.alibabacloud/alb
  parameters:
    apiGroup: alibabacloud.com
    kind: AlbConfig
    name: production-alb
---
# Ingress 配置
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: alb-ingress
  annotations:
    # 监听配置
    alb.ingress.kubernetes.io/listen-ports: '[{"HTTP": 80}, {"HTTPS": 443}]'
    # HTTPS 重定向
    alb.ingress.kubernetes.io/ssl-redirect: "true"
    # 健康检查
    alb.ingress.kubernetes.io/healthcheck-enabled: "true"
    alb.ingress.kubernetes.io/healthcheck-path: "/health"
    alb.ingress.kubernetes.io/healthcheck-protocol: "HTTP"
    alb.ingress.kubernetes.io/healthcheck-interval-seconds: "5"
    alb.ingress.kubernetes.io/healthy-threshold-count: "2"
    alb.ingress.kubernetes.io/unhealthy-threshold-count: "3"
    # 后端协议
    alb.ingress.kubernetes.io/backend-protocol: "HTTP"
    # 会话保持
    alb.ingress.kubernetes.io/sticky-session: "true"
    alb.ingress.kubernetes.io/sticky-session-type: "Insert"
    alb.ingress.kubernetes.io/cookie-timeout: "1800"
    # 限流
    alb.ingress.kubernetes.io/traffic-limit-qps: "1000"
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

#### ALB 注解参考

| 注解 | 说明 | 示例值 |
|-----|------|-------|
| `alb.ingress.kubernetes.io/address-type` | 地址类型 | `Internet` / `Intranet` |
| `alb.ingress.kubernetes.io/listen-ports` | 监听端口 | `'[{"HTTP": 80}, {"HTTPS": 443}]'` |
| `alb.ingress.kubernetes.io/ssl-redirect` | HTTPS 重定向 | `"true"` |
| `alb.ingress.kubernetes.io/backend-protocol` | 后端协议 | `HTTP` / `HTTPS` / `gRPC` |
| `alb.ingress.kubernetes.io/healthcheck-enabled` | 启用健康检查 | `"true"` |
| `alb.ingress.kubernetes.io/healthcheck-path` | 健康检查路径 | `/health` |
| `alb.ingress.kubernetes.io/traffic-limit-qps` | QPS 限制 | `"1000"` |
| `alb.ingress.kubernetes.io/sticky-session` | 会话保持 | `"true"` |
| `alb.ingress.kubernetes.io/canary` | 金丝雀发布 | `"true"` |
| `alb.ingress.kubernetes.io/canary-weight` | 金丝雀权重 | `"10"` |
| `alb.ingress.kubernetes.io/canary-by-header` | Header 金丝雀 | `X-Canary` |
| `alb.ingress.kubernetes.io/order` | 规则优先级 | `"100"` |

---

## 四、选型决策矩阵

### 4.1 场景推荐

| 场景 | 首选 | 备选 | 理由 |
|-----|------|------|------|
| **通用场景/入门** | NGINX Ingress | Traefik | 文档完善，社区活跃 |
| **高性能要求** | HAProxy | NGINX Ingress | HAProxy 原生高性能 |
| **动态配置/微服务** | Traefik | Contour | 零停机配置更新 |
| **API 网关需求** | Kong | Ambassador | 丰富的插件生态 |
| **服务网格集成** | Contour | Istio Gateway | Envoy 原生支持 |
| **阿里云 ACK** | ALB Ingress | NGINX Ingress | 云原生，免运维 |
| **AWS EKS** | AWS ALB Ingress | NGINX Ingress | 原生集成 |
| **多租户隔离** | Contour | Traefik | HTTPProxy 支持委派 |
| **金丝雀/灰度发布** | Traefik | NGINX Ingress | 原生流量分割 |
| **企业级 WAF** | Kong EE | ALB (云 WAF) | 内置 WAF 能力 |

### 4.2 决策流程图

```
开始
  │
  ├─→ 使用云服务？
  │      │
  │      ├─ 是: 阿里云 → ALB Ingress
  │      │       AWS → AWS ALB Ingress
  │      │       GCP → GCE Ingress
  │      │
  │      └─ 否 ↓
  │
  ├─→ 需要 API 网关功能？
  │      │
  │      ├─ 是 → Kong Ingress
  │      │
  │      └─ 否 ↓
  │
  ├─→ 追求极致性能？
  │      │
  │      ├─ 是 → HAProxy Ingress
  │      │
  │      └─ 否 ↓
  │
  ├─→ 需要零停机配置？
  │      │
  │      ├─ 是 → Traefik 或 Contour
  │      │
  │      └─ 否 ↓
  │
  ├─→ 计划使用服务网格？
  │      │
  │      ├─ 是 → Contour (Envoy) 或 Istio Gateway
  │      │
  │      └─ 否 ↓
  │
  └─→ 默认选择 → NGINX Ingress (社区版)
```

### 4.3 成本对比

| 控制器 | 软件成本 | 运维复杂度 | 资源消耗 | 总体 TCO |
|-------|---------|-----------|---------|---------|
| **NGINX Ingress (开源)** | 免费 | 中 | 中 | 低 |
| **NGINX Plus** | 商业授权 | 低 | 中 | 中 |
| **Traefik (开源)** | 免费 | 低 | 低 | 低 |
| **Traefik EE** | 商业授权 | 低 | 低 | 中 |
| **HAProxy (开源)** | 免费 | 中 | 低 | 低 |
| **Contour** | 免费 | 中 | 中 | 低 |
| **Kong (开源)** | 免费 | 高 | 中-高 | 中 |
| **Kong EE** | 商业授权 | 中 | 中-高 | 高 |
| **ALB (阿里云)** | 按量付费 | 低 | - | 按流量 |

---

## 五、高可用部署模式

### 5.1 部署模式对比

| 模式 | 优点 | 缺点 | 适用场景 |
|-----|------|------|---------|
| **Deployment** | 灵活扩缩，易于管理 | 需要 Service 暴露 | 通用场景 |
| **DaemonSet** | 每节点部署，低延迟 | 扩缩不灵活 | 高性能要求 |
| **DaemonSet + NodeSelector** | 指定节点，资源可控 | 管理复杂 | 专用边缘节点 |
| **Deployment + HPA** | 自动扩缩容 | HPA 延迟 | 流量波动场景 |

### 5.2 高可用配置示例

```yaml
# NGINX Ingress 高可用部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-ingress-controller
  namespace: ingress-nginx
spec:
  replicas: 3  # 最少 3 副本
  selector:
    matchLabels:
      app.kubernetes.io/name: ingress-nginx
      app.kubernetes.io/component: controller
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 1
  template:
    metadata:
      labels:
        app.kubernetes.io/name: ingress-nginx
        app.kubernetes.io/component: controller
    spec:
      # 服务账号
      serviceAccountName: ingress-nginx
      # 优雅终止
      terminationGracePeriodSeconds: 300
      # 反亲和性 - 分散到不同节点
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app.kubernetes.io/component: controller
            topologyKey: kubernetes.io/hostname
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app.kubernetes.io/component: controller
              topologyKey: topology.kubernetes.io/zone
      # 拓扑分布约束
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app.kubernetes.io/component: controller
      containers:
      - name: controller
        image: registry.k8s.io/ingress-nginx/controller:v1.10.0
        args:
          - /nginx-ingress-controller
          - --election-id=ingress-controller-leader
          - --controller-class=k8s.io/ingress-nginx
          - --configmap=$(POD_NAMESPACE)/ingress-nginx-controller
        lifecycle:
          preStop:
            exec:
              command:
                - /wait-shutdown
        resources:
          requests:
            cpu: 100m
            memory: 256Mi
          limits:
            cpu: 1000m
            memory: 512Mi
        livenessProbe:
          httpGet:
            path: /healthz
            port: 10254
          initialDelaySeconds: 10
          periodSeconds: 10
          timeoutSeconds: 1
          failureThreshold: 5
        readinessProbe:
          httpGet:
            path: /healthz
            port: 10254
          initialDelaySeconds: 10
          periodSeconds: 10
          timeoutSeconds: 1
          failureThreshold: 3
---
# HPA 自动扩缩
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: nginx-ingress-hpa
  namespace: ingress-nginx
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: nginx-ingress-controller
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Pods
        value: 2
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
---
# PodDisruptionBudget
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: nginx-ingress-pdb
  namespace: ingress-nginx
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app.kubernetes.io/component: controller
```

---

**Ingress Controller 选型原则**: 评估业务需求 → 对比功能特性 → 测试性能指标 → 考虑运维成本 → 验证高可用方案

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
