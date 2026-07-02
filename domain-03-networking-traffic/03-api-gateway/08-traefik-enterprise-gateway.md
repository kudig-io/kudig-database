---
title: 08 - Traefik API 网关企业级实践
description: '# 08 - Traefik API 网关企业级实践'
summary: '5. [IngressRoute CRD 路由配置](#5-ingressroute-crd-路由配置)'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- prometheus
- helm
- docker
- ceph
- hpa
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
- Traefik API 网关企业级实践 是什么
- 如何 Traefik API 网关企业级实践
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- Traefik
- API
- 网关企业级实践
- cloud
- native
- api
- gateway
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- prometheus-basics
- tls-basics
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




# 08 - Traefik API 网关企业级实践

> **文档版本**: v1.0 | **适用版本**: Traefik v3.x, [[Kubernetes|Kubernetes]] 1.24+ | **更新日期**: 2026-03-04 | **关键词**: Traefik, IngressRoute, Middleware, ACME, Let's Encrypt, Hub

<!-- chunk: 目录 -->## 目录

1. [项目概述](#1-项目概述)
2. [核心架构](#2-核心架构)
3. [部署安装](#3-部署安装)
4. [Provider 模型](#4-provider-模型)
5. [IngressRoute CRD 路由配置](#5-ingressroute-crd-路由配置)
6. [Middleware 中间件体系](#6-middleware-中间件体系)
7. [TLS 自动化](#7-tls-自动化)
8. Gateway API 支持](#8-gateway-api-支持)
9. [Traefik Hub](#9-traefik-hub)
10. [生产部署建议](#10-生产部署建议)

---

<!-- chunk: 1. 项目概述 -->## 1. 项目概述

Traefik 是由 Traefik Labs（前 Containous）开发的云原生反向代理和负载均衡器，以 **Go 语言**原生实现，以极低的资源占用和动态配置能力著称。

## 核心特点

- **轻量高效**：单二进制文件，内存占用极低（生产环境 50~200MB）
- **动态配置**：自动发现服务变化，零重启配置热更新
- **内置 Let's Encrypt**：自动申请、续期 TLS 证书，无需外部证书管理器
- **多 Provider 支持**：Kubernetes、Docker、Consul、文件等 20+ 数据源
- **内置监控**：自带 Web Dashboard，Prometheus 指标开箱即用

## 产品线

| 产品 | 定位 | 许可 |
|------|------|------|
| **Traefik Proxy** | 开源反向代理/网关核心 | MIT |
| **Traefik Enterprise** | 企业版（高级路由、集群模式） | 商业许可 |
| **Traefik Hub** | API 管理与发布平台（SaaS） | 免费/付费分层 |

## v2 vs v3 主要变化

| 特性 | Traefik v2 | Traefik v3 |
|------|-----------|-----------|
| Gateway API | 实验性支持 | v1 正式支持 |
| SPIFFE/SPIRE | 不支持 | 内置支持 |
| Kubernetes Service LB | 基础 | 增强（IPAM） |
| gRPC 路由 | 通过 TCP 处理 | 原生 HTTP/2 |
| Wasm 中间件 | 不支持 | 实验性支持 |

---

<!-- chunk: 2. 核心架构 -->## 2. 核心架构

## Traefik 流量处理模型

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────────┐
│                       Traefik 核心架构                               │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  外部流量                                                            │
│     │                                                               │
│     ▼                                                               │
│  ┌──────────────────────────────────────────────────────┐           │
│  │                   EntryPoints（入口点）                │           │
│  │   :80 (web)   :443 (websecure)   :8080 (traefik)     │           │
│  └──────────────┬───────────────────────────────────────┘           │
│                 │                                                   │
│                 ▼                                                   │
│  ┌──────────────────────────────────────────────────────┐           │
│  │                    Routers（路由器）                   │           │
│  │                                                      │           │
│  │  规则匹配（Rule）:                                     │           │
│  │  Host(`api.example.com`) && PathPrefix(`/api`)        │           │
│  │  Headers(`X-Service`, `backend`)                     │           │
│  └──────────────┬───────────────────────────────────────┘           │
│                 │ 匹配命中                                            │
│                 ▼                                                   │
│  ┌──────────────────────────────────────────────────────┐           │
│  │                 Middlewares（中间件）                  │           │
│  │  [认证] → [限流] → [Header改写] → [压缩] → [重试]      │           │
│  └──────────────┬───────────────────────────────────────┘           │
│                 │                                                   │
│                 ▼                                                   │
│  ┌──────────────────────────────────────────────────────┐           │
│  │                  Services（服务）                      │           │
│  │  LoadBalancer → Backend Pod 1 / Pod 2 / Pod 3        │           │
│  └──────────────────────────────────────────────────────┘           │
│                                                                     │
│  ┌──────────────────────────────────────────────────────┐           │
│  │                  Providers（提供者）                   │           │
│  │  Kubernetes CRD │ Ingress │ Docker │ File │ Consul   │           │
│  └──────────────────────────────────────────────────────┘           │
└─────────────────────────────────────────────────────────────────────┘
```
## 自动服务发现流程

```
Kubernetes API
      │
      │（Watch Events: Service/Ingress/IngressRoute 变化）
      ▼
  Traefik Provider
      │
      │（动态配置更新，无需重启）
      ▼
  Router/Middleware/Service 配置热更新
      │
      ▼
  流量即时切换（零停机）
```

---

<!-- chunk: 3. 部署安装 -->## 3. 部署安装

## Helm 安装（推荐）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 添加 Helm 仓库
helm repo add traefik https://traefik.github.io/charts
helm repo update

# 查看默认 values
helm show values traefik/traefik > traefik-values.yaml
```
## 生产 values 配置

```yaml
# traefik-values-prod.yaml
deployment:
  replicas: 3
  podAnnotations:
    prometheus.io/scrape: "true"
    prometheus.io/port: "9100"

# 入口点配置
ports:
  web:
    port: 80
    redirectTo:
      port: websecure
  websecure:
    port: 443
    tls:
      enabled: true
  traefik:
    port: 9000
    expose:
      default: false  # 生产禁止暴露 Dashboard

# 服务配置
service:
  type: LoadBalancer
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"

# 访问日志
logs:
  access:
    enabled: true
    format: json

# 指标
metrics:
  prometheus:
    entryPoint: metrics
    addEntryPointsLabels: true
    addServicesLabels: true
    addRoutersLabels: true

# RBAC（需要访问 K8s CRD）
rbac:
  enabled: true

# 持久化（用于 ACME 证书存储）
persistence:
  enabled: true
  size: 128Mi
  storageClass: "fast-ssd"

# 资源限制
resources:
  requests:
    cpu: "200m"
    memory: "128Mi"
  limits:
    cpu: "1"
    memory: "512Mi"

# 亲和性（跨 Zone 分布）
affinity:
  podAntiAffinity:
    requiredDuringSchedulingIgnoredDuringExecution:
    - labelSelector:
        matchLabels:
          app.kubernetes.io/name: traefik
      topologyKey: topology.kubernetes.io/zone
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装
helm install traefik traefik/traefik \
  --namespace traefik \
  --create-namespace \
  -f traefik-values-prod.yaml \
  --version 32.x.x

# 验证
kubectl get pods -n traefik
kubectl get svc -n traefik
```
---

<!-- chunk: 4. Provider 模型 -->## 4. Provider 模型

Traefik 通过 **Provider** 抽象与不同平台集成，每个 Provider 负责从特定数据源读取路由配置。

## Provider 对比

| Provider | 配置来源 | 使用场景 |
|---------|---------|---------|
| `kubernetescrd` | IngressRoute / Middleware CRD | Kubernetes 原生，功能最完整 |
| `kubernetesingress` | 标准 Ingress 资源 | 与现有 Ingress 兼容 |
| `kubernetesgateway` | Gateway API CRD | 标准化接口（v3 正式支持） |
| `docker` | Docker labels | 容器场景（Swarm/Compose） |
| `file` | 静态 YAML/TOML 文件 | 调试和静态配置 |
| `consul` | Consul Key-Value | HashiCorp 生态 |

## 多 Provider 同时启用

```yaml
# traefik-configmap.yaml（静态配置）
apiVersion: v1
kind: ConfigMap
metadata:
  name: traefik-config
  namespace: traefik
data:
  traefik.yaml: |
    providers:
      kubernetesCRD:
        enabled: true
        allowCrossNamespace: false        # 安全：禁止跨命名空间引用
        allowExternalNameServices: false
        ingressClass: "traefik"
      kubernetesIngress:
        enabled: true
        ingressClass: "traefik"
        allowExternalNameServices: false
      kubernetesGateway:
        enabled: true
        experimentalChannel: false
      file:
        directory: /etc/traefik/dynamic
        watch: true
    
    entryPoints:
      web:
        address: ":80"
        http:
          redirections:
            entrypoint:
              to: websecure
              scheme: https
      websecure:
        address: ":443"
        http3:
          advertisedPort: 443
      metrics:
        address: ":9100"
    
    api:
      dashboard: true
      insecure: false    # 生产须关闭 insecure 模式
    
    certificatesResolvers:
      letsencrypt:
        acme:
          email: ops@example.com
          storage: /data/acme.json
          tlsChallenge: {}
```

---

<!-- chunk: 5. IngressRoute CRD 路由配置 -->## 5. IngressRoute CRD 路由配置

## 基础 IngressRoute

```yaml
# ingressroute-basic.yaml
apiVersion: traefik.io/v1alpha1
kind: IngressRoute
metadata:
  name: api-route
  namespace: default
spec:
  entryPoints:
  - websecure
  routes:
  - match: Host(`api.example.com`) && PathPrefix(`/v1`)
    kind: Rule
    priority: 10
    services:
    - name: api-service
      port: 8080
      weight: 100
    middlewares:
    - name: rate-limit-middleware
    - name: auth-middleware
  - match: Host(`api.example.com`) && PathPrefix(`/health`)
    kind: Rule
    priority: 20    # 优先级更高
    services:
    - name: health-service
      port: 8080
  tls:
    certResolver: letsencrypt
    domains:
    - main: api.example.com
```

## 高级路由规则

```yaml
# ingressroute-advanced.yaml
apiVersion: traefik.io/v1alpha1
kind: IngressRoute
metadata:
  name: advanced-routes
  namespace: default
spec:
  entryPoints:
  - websecure
  routes:
  # Header 条件路由（金丝雀）
  - match: Host(`app.example.com`) && Headers(`X-Canary`, `true`)
    kind: Rule
    services:
    - name: app-canary
      port: 8080

  # 正则路径匹配
  - match: Host(`app.example.com`) && PathRegexp(`^/api/v[0-9]+/.*`)
    kind: Rule
    services:
    - name: api-versioned-service
      port: 8080

  # 加权负载均衡（流量分割）
  - match: Host(`app.example.com`) && PathPrefix(`/`)
    kind: Rule
    services:
    - name: app-v1
      port: 8080
      weight: 80
    - name: app-v2
      port: 8080
      weight: 20

  # TCP 路由（IngressRouteTCP）
  tls:
    secretName: app-tls-secret
```

## IngressRouteTCP（四层路由）

```yaml
# ingressroute-tcp.yaml
apiVersion: traefik.io/v1alpha1
kind: IngressRouteTCP
metadata:
  name: postgres-route
  namespace: default
spec:
  entryPoints:
  - tcp-5432
  routes:
  - match: HostSNI(`db.example.com`)
    services:
    - name: postgres-service
      port: 5432
  tls:
    passthrough: true    # TLS 直通（不解密）
```

---

<!-- chunk: 6. Middleware 中间件体系 -->## 6. Middleware 中间件体系

Traefik 中间件是流量处理管道的核心，支持链式组合。

## 中间件全景图

```
请求 ──▶ [IP白名单] ──▶ [基础认证] ──▶ [限流] ──▶ [Header改写]
      ──▶ [URL重写] ──▶ [压缩] ──▶ [重试] ──▶ [熔断] ──▶ 后端服务
响应 ◀── [压缩] ◀── [Header添加] ◀── 后端服务
```

## 常用中间件配置

```yaml
# middlewares-collection.yaml

# 1. 限流（RateLimit）
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: rate-limit
  namespace: default
spec:
  rateLimit:
    average: 100      # 平均速率（请求/秒）
    burst: 200        # 突发容量
    period: 1s
    sourceCriterion:
      ipStrategy:
        depth: 1      # 从 X-Forwarded-For 取第一个 IP

---
# 2. 基础认证（BasicAuth）
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: basic-auth
  namespace: default
spec:
  basicAuth:
    secret: basic-auth-secret    # Secret 中存 htpasswd 格式

---
# 3. Header 修改（Headers）
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: security-headers
  namespace: default
spec:
  headers:
    frameDeny: true
    browserXssFilter: true
    contentTypeNosniff: true
    forceSTSHeader: true
    stsIncludeSubdomains: true
    stsPreload: true
    stsSeconds: 31536000
    customRequestHeaders:
      X-Gateway: "traefik"
    customResponseHeaders:
      X-Content-Type-Options: "nosniff"
      X-Frame-Options: "DENY"

---
# 4. 压缩（Compress）
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: compress
  namespace: default
spec:
  compress:
    minResponseBodyBytes: 1024
    excludedContentTypes:
    - image/jpeg
    - image/png
    - image/gif

---
# 5. 重试（Retry）
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: retry
  namespace: default
spec:
  retry:
    attempts: 3
    initialInterval: 100ms

---
# 6. 熔断（CircuitBreaker）
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: circuit-breaker
  namespace: default
spec:
  circuitBreaker:
    expression: "ResponseCodeRatio(500, 600, 0, 600) > 0.25 || NetworkErrorRatio() > 0.1"
    checkPeriod: 10s
    fallbackDuration: 30s
    recoveryDuration: 10s

---
# 7. IP 白名单（IPAllowList）
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: ip-allowlist
  namespace: default
spec:
  ipAllowList:
    sourceRange:
    - "10.0.0.0/8"
    - "192.168.0.0/16"
    - "172.16.0.0/12"
    ipStrategy:
      depth: 1

---
# 8. URL 重写（StripPrefix/ReplacePath）
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: strip-prefix
  namespace: default
spec:
  stripPrefix:
    prefixes:
    - /api/v1
    forceSlash: true

---
# 9. 转发认证（ForwardAuth）
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: forward-auth
  namespace: default
spec:
  forwardAuth:
    address: "http://auth-service.default.svc.cluster.local/auth/verify"
    trustForwardHeader: true
    authResponseHeaders:
    - X-Auth-User
    - X-Auth-Role
    authRequestHeaders:
    - Authorization
    - Cookie
```

## 中间件链（MiddlewareChain）

```yaml
# middleware-chain.yaml
apiVersion: traefik.io/v1alpha1
kind: Middleware
metadata:
  name: api-chain
  namespace: default
spec:
  chain:
    middlewares:
    - name: ip-allowlist
    - name: rate-limit
    - name: forward-auth
    - name: security-headers
    - name: compress
```

---

<!-- chunk: 7. TLS 自动化 -->## 7. TLS 自动化

## ACME/Let's Encrypt 配置

Traefik 内置 ACME 客户端，支持三种验证方式：

| 验证方式 | 原理 | 适用场景 |
|---------|------|---------|
| `tlsChallenge` | 通过 443 端口验证 | 简单场景（需要 80 端口重定向） |
| `httpChallenge` | 通过 80 端口 HTTP 响应验证 | 标准 HTTP 挑战 |
| `dnsChallenge` | 通过 DNS TXT 记录验证 | 通配符证书、内网服务 |

## DNS 验证（推荐生产使用）

```yaml
# traefik-acme-dns.yaml
certificatesResolvers:
  letsencrypt-dns:
    acme:
      email: ops@example.com
      storage: /data/acme.json
      dnsChallenge:
        provider: alidns          # 阿里云 DNS
        delayBeforeCheck: 30s
        resolvers:
        - "8.8.8.8:53"
        - "1.1.1.1:53"
  
  letsencrypt-staging:            # 测试用 Staging 环境
    acme:
      email: ops@example.com
      storage: /data/acme-staging.json
      caServer: https://acme-staging-v02.api.letsencrypt.org/directory
      dnsChallenge:
        provider: alidns
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 配置 DNS Provider 凭证（以阿里云为例）
kubectl create secret generic traefik-dns-credentials \
  -n traefik \
  --from-literal=ALICLOUD_ACCESS_KEY=xxx \
  --from-literal=ALICLOUD_SECRET_KEY=yyy \
  --from-literal=ALICLOUD_REGION_ID=cn-hangzhou
```
## 通配符证书配置

```yaml
# ingressroute-wildcard.yaml
apiVersion: traefik.io/v1alpha1
kind: IngressRoute
metadata:
  name: wildcard-route
  namespace: default
spec:
  entryPoints:
  - websecure
  routes:
  - match: HostRegexp(`{subdomain:[a-z]+}.example.com`)
    kind: Rule
    services:
    - name: multi-tenant-service
      port: 8080
  tls:
    certResolver: letsencrypt-dns
    domains:
    - main: "example.com"
      sans:
      - "*.example.com"
```

## 自定义证书（Secret 方式）

```yaml
# ingressroute-custom-cert.yaml
apiVersion: traefik.io/v1alpha1
kind: IngressRoute
metadata:
  name: custom-cert-route
  namespace: default
spec:
  entryPoints:
  - websecure
  routes:
  - match: Host(`secure.example.com`)
    kind: Rule
    services:
    - name: secure-service
      port: 8080
  tls:
    secretName: secure-example-com-tls   # TLS Secret
```

---

<!-- chunk: 8. Gateway API 支持 -->## 8. Gateway API 支持

Traefik v3 正式支持 Gateway API v1（稳定渠道），可通过 Gateway API 标准接口管理路由。

## 启用 Gateway API Provider

```yaml
# traefik-static-config.yaml
providers:
  kubernetesGateway:
    enabled: true
```

## GatewayClass 配置

```yaml
# traefik-gatewayclass.yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: traefik
spec:
  controllerName: traefik.io/gateway-controller
```

## Gateway 和 HTTPRoute（标准写法）

```yaml
# traefik-gateway.yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: traefik-gateway
  namespace: default
spec:
  gatewayClassName: traefik
  listeners:
  - name: http
    port: 80
    protocol: HTTP
    allowedRoutes:
      namespaces:
        from: Same
  - name: https
    port: 443
    protocol: HTTPS
    tls:
      mode: Terminate
      certificateRefs:
      - kind: Secret
        name: app-tls-secret
    allowedRoutes:
      namespaces:
        from: All

---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: app-route
  namespace: default
spec:
  parentRefs:
  - name: traefik-gateway
    sectionName: https
  hostnames:
  - "app.example.com"
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    backendRefs:
    - name: api-service
      port: 8080
```

## Traefik 私有 CRD vs Gateway API 对比

| 功能 | IngressRoute (Traefik CRD) | HTTPRoute (Gateway API) |
|------|--------------------------|------------------------|
| 中间件链 | ✅ 原生支持 | ❌ 不支持（需 ExtensionRef） |
| 流量权重 | ✅ 内置 weight | ✅ backendRefs weight |
| Header 路由 | ✅ 完整表达式 | ✅ 标准 matches |
| TCP/UDP 路由 | ✅ IngressRouteTCP/UDP | ✅ TCPRoute/UDPRoute |
| 通配符证书 | ✅ ACME 内置 | 需外部证书管理器 |
| 可移植性 | ❌ 厂商锁定 | ✅ 标准可移植 |

---

<!-- chunk: 9. Traefik Hub -->## 9. Traefik Hub

Traefik Hub 是 Traefik Labs 提供的 API 管理和发布平台，分为免费层和付费层。

## 核心能力

```
┌──────────────────────────────────────────────────────────────┐
│                     Traefik Hub 功能                          │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────────┐   ┌─────────────────┐                   │
│  │   API Portal    │   │  API Catalog    │                   │
│  │  开发者门户      │   │  API 文档自动生成 │                   │
│  └─────────────────┘   └─────────────────┘                   │
│                                                              │
│  ┌─────────────────┐   ┌─────────────────┐                   │
│  │  Access Control │   │  API Analytics  │                   │
│  │  API 访问管理    │   │  使用量统计分析   │                   │
│  └─────────────────┘   └─────────────────┘                   │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐     │
│  │                  AI Gateway（新增）                   │     │
│  │  Token 消耗追踪 / LLM 负载均衡 / 语义缓存               │     │
│  └─────────────────────────────────────────────────────┘     │
└──────────────────────────────────────────────────────────────┘
```

## 安装 Hub Agent

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 获取 Hub Token（在 hub.traefik.io 注册后获得）
export HUB_TOKEN="your-hub-token"

# 安装 Hub Agent（与 Traefik 集成）
helm upgrade traefik traefik/traefik \
  --namespace traefik \
  --set hub.token=$HUB_TOKEN \
  --set hub.enabled=true
```
## API 发布配置

```yaml
# hub-api.yaml
apiVersion: hub.traefik.io/v1alpha1
kind: API
metadata:
  name: user-api
  namespace: default
  labels:
    app: user-service
spec:
  openApiSpec:
    url: "http://user-service.default.svc.cluster.local:8080/openapi.json"
  pathPrefix: /api/users

---
apiVersion: hub.traefik.io/v1alpha1
kind: APIVersion
metadata:
  name: user-api-v1
  namespace: default
spec:
  apiName: user-api
  title: "User API v1"
  release: "1.0.0"

---
# API 访问计划
apiVersion: hub.traefik.io/v1alpha1
kind: APIAccess
metadata:
  name: basic-plan
  namespace: default
spec:
  apis:
  - name: user-api
  groups:
  - developers
  operationFilter:
    include:
    - path: /api/users/{id}
      methods: [GET]
    - path: /api/users
      methods: [GET]
```

---

<!-- chunk: 10. 生产部署建议 -->## 10. 生产部署建议

## 高可用架构

```
                ┌─────────────────────────────────────┐
                │         Traefik HA 部署              │
                └─────────────────────────────────────┘

  Zone A              Zone B              Zone C
  ┌──────────┐        ┌──────────┐        ┌──────────┐
  │ Traefik  │        │ Traefik  │        │ Traefik  │
  │  Pod     │        │  Pod     │        │  Pod     │
  └────┬─────┘        └────┬─────┘        └────┬─────┘
       │                   │                   │
       └───────────────────┼───────────────────┘
                           │
                ┌──────────▼──────────┐
                │   LoadBalancer      │
                │   (NLB/CLB)         │
                └─────────────────────┘
  
  注意：多副本时 ACME 证书需配置共享存储（PVC）或使用外部存储
  推荐：cert-manager + Let's Encrypt 替代内置 ACME（多副本场景）
```

## ACME 多副本方案

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 方案 A：共享 PVC（ReadWriteMany）
# 使用 NFS 或 CephFS 存储 acme.json

# 方案 B：外部 Secret 存储（推荐）
# 配置 Traefik 使用 Kubernetes Secret 存储证书
# 或使用 cert-manager 管理证书，Traefik 直接引用 Secret

# 安装 cert-manager（推荐生产多副本方案）
helm install cert-manager jetstack/cert-manager \
  --namespace cert-manager \
  --create-namespace \
  --set installCRDs=true
```
## 资源规划与 HPA

```yaml
# hpa-traefik.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: traefik-hpa
  namespace: traefik
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: traefik
  minReplicas: 3
  maxReplicas: 15
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Pods
    pods:
      metric:
        name: traefik_entrypoint_requests_total
      target:
        type: AverageValue
        averageValue: 1000
```

## 监控告警规则

```yaml
# prometheusrule-traefik.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: traefik-alerts
  namespace: traefik
spec:
  groups:
  - name: traefik
    rules:
    - alert: TraefikHighErrorRate
      expr: |
        sum(rate(traefik_router_requests_total{code=~"5.."}[5m]))
        / sum(rate(traefik_router_requests_total[5m])) > 0.05
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "Traefik 5xx 错误率过高 ({{ $value | humanizePercentage }})"
    
    - alert: TraefikHighLatency
      expr: |
        histogram_quantile(0.99,
          sum(rate(traefik_router_request_duration_seconds_bucket[5m])) by (le, router)
        ) > 2
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Traefik P99 延迟超过 2 秒"
    
    - alert: TraefikDown
      expr: up{job="traefik"} == 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "Traefik 实例下线"
```

## 生产检查清单

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ✅ 检查 Traefik 版本
kubectl exec -n traefik -it deploy/traefik -- traefik version

# ✅ 检查 Dashboard（仅内网访问）
kubectl port-forward -n traefik pod/traefik-xxx 9000:9000
# 访问 http://localhost:9000/dashboard/

# ✅ 验证 IngressRoute 状态
kubectl get ingressroute -A

# ✅ 检查 ACME 证书状态
kubectl exec -n traefik deploy/traefik -- cat /data/acme.json | jq '.letsencrypt.Certificates[].domain'

# ✅ 查看 Traefik 日志
kubectl logs -n traefik -l app.kubernetes.io/name=traefik -f --tail=100

# ✅ 检查 Middleware 是否生效
kubectl get middleware -A
kubectl describe middleware rate-limit -n default
```
---

<!-- chunk: 参考资料 -->## 参考资料

- [Traefik 官方文档](https://doc.traefik.io/traefik/)
- [Traefik v3 迁移指南](https://doc.traefik.io/traefik/migration/v2-to-v3/)
- [Traefik Hub 文档](https://doc.traefik.io/traefik-hub/)
- [Gateway API 集成文档](https://doc.traefik.io/traefik/providers/kubernetes-gateway/)
- [ACME 配置参考](https://doc.traefik.io/traefik/https/acme/)
- 本域相关文档：
  - [01 - API 网关架构总览](./01-api-gateway-architecture-overview.md)
  - [02 - Kubernetes Gateway API 深度解析](./02-kubernetes-gateway-api-deep-dive.md)
  - [03 - API 网关选型指南](./03-api-gateway-selection-guide.md)
  - [09 - 传统 Ingress 迁移指南](./09-nginx-ingress-migration-guide.md)
  - [domain-5: Ingress 控制器概述](../domain-03-networking-traffic/19-ingress-fundamentals.html)

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
- 09 - 传统 Ingress 控制器向云原生 API 网关迁移
- 10 - Wasm 插件生态与开发实践

## See Also

- 06-kong-enterprise-gateway
- 07-envoy-gateway-enterprise
- 09-nginx-ingress-migration-guide
- 10-wasm-plugin-ecosystem


<!-- risk-assessed -->
