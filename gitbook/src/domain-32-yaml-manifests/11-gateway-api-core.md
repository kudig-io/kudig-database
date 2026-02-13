# 11 - Gateway API 核心资源 YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 + Gateway API v1.0+ | **最后更新**: 2026-02  
> **相关领域**: [域5-网络](../domain-5-networking/) | **前置知识**: Service, Ingress  
> **关联配置**: [12-高级路由](./12-gateway-api-advanced-routes.md) | [Ingress参考](./08-ingress-all-classes.md)

---

## 📋 目录

1. [API 概述与版本](#api-概述与版本)
2. [GatewayClass 配置](#gatewayclass-配置)
3. [Gateway 配置](#gateway-配置)
4. [HTTPRoute 配置](#httproute-配置)
5. [HTTPRouteMatch 匹配规则](#httproutematch-匹配规则)
6. [HTTPRouteFilter 流量处理](#httproutefilter-流量处理)
7. [内部实现原理](#内部实现原理)
8. [生产实战案例](#生产实战案例)
9. [版本兼容性与最佳实践](#版本兼容性与最佳实践)

---

## API 概述与版本

### 基本信息

| 属性 | 值 |
|------|-----|
| **API Group** | `gateway.networking.k8s.io` |
| **核心资源** | GatewayClass, Gateway, HTTPRoute |
| **稳定版本** | v1 (自 Gateway API v1.0.0) |
| **安装方式** | `kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.0.0/standard-install.yaml` |

### Gateway API 演进

```yaml
# Gateway API 版本标准分类
# - Standard Channel (GA/稳定):
#   - GatewayClass, Gateway, HTTPRoute
#   - ReferenceGrant (v1beta1)
# - Experimental Channel (实验性):
#   - GRPCRoute, TCPRoute, TLSRoute, UDPRoute
#   - BackendTLSPolicy (v1alpha2)
```

### 与 Ingress 的核心差异

| 维度 | Ingress | Gateway API |
|------|---------|-------------|
| **角色分离** | 单一资源 | GatewayClass(集群管理员) + Gateway(平台运维) + Route(开发者) |
| **协议支持** | HTTP/HTTPS | HTTP, HTTPS, gRPC, TCP, TLS, UDP |
| **路由能力** | 基础路径/主机匹配 | Header, Query, Method, 权重路由, 镜像 |
| **跨命名空间** | 不支持 | ReferenceGrant 授权 |
| **扩展性** | Annotation 依赖实现 | 标准化 Filter/Policy |

---

## GatewayClass 配置

### 字段规格表

| 字段路径 | 类型 | 必填 | 版本 | 说明 |
|----------|------|------|------|------|
| `spec.controllerName` | string | ✅ | v1 | 控制器标识符 (如 `istio.io/gateway-controller`) |
| `spec.parametersRef` | object | ❌ | v1 | 控制器特定配置引用 |
| `spec.parametersRef.group` | string | ✅ | v1 | 参数资源 API 组 |
| `spec.parametersRef.kind` | string | ✅ | v1 | 参数资源类型 |
| `spec.parametersRef.name` | string | ✅ | v1 | 参数资源名称 |
| `spec.parametersRef.namespace` | string | ❌ | v1 | 参数资源命名空间(集群级参数可选) |
| `spec.description` | string | ❌ | v1 | 人类可读描述 |

### 基础示例

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: istio-gateway
spec:
  # 控制器名称 - 必须与安装的网关控制器匹配
  controllerName: istio.io/gateway-controller
  
  # 可选：控制器特定的配置参数引用
  # parametersRef:
  #   group: networking.istio.io
  #   kind: GatewayParameters
  #   name: default-params
  
  description: "Istio-based gateway for production traffic"
```

### 多实现示例

```yaml
---
# 1. Istio Gateway Controller
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: istio
spec:
  controllerName: istio.io/gateway-controller
---
# 2. Nginx Gateway Fabric
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: nginx
spec:
  controllerName: gateway.nginx.org/nginx-gateway-controller
---
# 3. Envoy Gateway
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: envoy
spec:
  controllerName: gateway.envoyproxy.io/gatewayclass-controller
---
# 4. Traefik
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: traefik
spec:
  controllerName: traefik.io/gateway-controller
```

### 带参数的 GatewayClass

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: shared-gateway
spec:
  controllerName: example.net/gateway-controller
  
  # 引用集群级配置参数
  parametersRef:
    group: config.example.net
    kind: GatewayConfig
    name: shared-config
---
# 控制器特定的配置示例 (取决于实现)
apiVersion: config.example.net/v1
kind: GatewayConfig
metadata:
  name: shared-config
spec:
  # 示例：资源限制
  resources:
    requests:
      cpu: "1"
      memory: 1Gi
  # 示例：日志级别
  logging:
    level: info
```

---

## Gateway 配置

### 字段规格表

| 字段路径 | 类型 | 必填 | 版本 | 说明 |
|----------|------|------|------|------|
| `spec.gatewayClassName` | string | ✅ | v1 | 关联的 GatewayClass 名称 |
| `spec.listeners[]` | array | ✅ | v1 | 监听器配置列表 |
| `spec.listeners[].name` | string | ✅ | v1 | 监听器唯一名称 |
| `spec.listeners[].hostname` | string | ❌ | v1 | 主机名匹配(支持通配符 `*.example.com`) |
| `spec.listeners[].port` | int32 | ✅ | v1 | 监听端口 (1-65535) |
| `spec.listeners[].protocol` | string | ✅ | v1 | 协议: HTTP, HTTPS, TLS, TCP, UDP |
| `spec.listeners[].tls` | object | ❌ | v1 | TLS 配置(HTTPS/TLS 协议必需) |
| `spec.listeners[].tls.mode` | string | ❌ | v1 | Terminate(终止), Passthrough(透传) |
| `spec.listeners[].tls.certificateRefs[]` | array | ❌ | v1 | TLS 证书引用(Secret) |
| `spec.listeners[].allowedRoutes` | object | ❌ | v1 | 允许绑定的路由规则 |
| `spec.listeners[].allowedRoutes.namespaces.from` | string | ❌ | v1 | All, Same(默认), Selector |
| `spec.listeners[].allowedRoutes.kinds[]` | array | ❌ | v1 | 允许的路由类型(默认HTTPRoute) |
| `spec.addresses[]` | array | ❌ | v1 | 显式指定网关地址 |
| `spec.infrastructure` | object | ❌ | v1.1+ | 基础设施配置(Annotations/Labels) |

### 基础 HTTP Gateway

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: http-gateway
  namespace: gateway-system
spec:
  # 关联 GatewayClass
  gatewayClassName: istio
  
  listeners:
  - name: http
    # 监听端口
    port: 80
    # 协议类型
    protocol: HTTP
    # 可选：限定主机名
    # hostname: "*.example.com"
    
    # 允许的路由绑定规则
    allowedRoutes:
      # 允许来自所有命名空间的 HTTPRoute
      namespaces:
        from: All
```

### 生产级 HTTPS Gateway

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: production-gateway
  namespace: gateway-infra
spec:
  gatewayClassName: istio
  
  listeners:
  # 1. HTTP 监听器 (自动重定向到 HTTPS)
  - name: http
    port: 80
    protocol: HTTP
    allowedRoutes:
      namespaces:
        from: All
  
  # 2. HTTPS 监听器 - 单证书
  - name: https-main
    port: 443
    protocol: HTTPS
    hostname: "api.example.com"
    tls:
      # TLS 终止模式
      mode: Terminate
      # 引用 Secret 中的 TLS 证书
      certificateRefs:
      - kind: Secret
        name: api-tls-cert
        namespace: gateway-infra  # 跨命名空间需 ReferenceGrant
    allowedRoutes:
      namespaces:
        from: Selector
        selector:
          matchLabels:
            gateway-access: "true"
  
  # 3. HTTPS 监听器 - 通配符域名
  - name: https-wildcard
    port: 443
    protocol: HTTPS
    hostname: "*.apps.example.com"
    tls:
      mode: Terminate
      certificateRefs:
      - name: wildcard-tls-cert
    allowedRoutes:
      namespaces:
        from: Same
      kinds:
      - kind: HTTPRoute
```

### 多端口多协议 Gateway

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: multi-protocol-gateway
  namespace: gateway-system
spec:
  gatewayClassName: envoy
  
  listeners:
  # HTTP 应用流量
  - name: web-http
    port: 80
    protocol: HTTP
    allowedRoutes:
      namespaces:
        from: All
  
  # HTTPS 应用流量
  - name: web-https
    port: 443
    protocol: HTTPS
    hostname: "*.myapp.com"
    tls:
      mode: Terminate
      certificateRefs:
      - name: myapp-tls
    allowedRoutes:
      namespaces:
        from: All
  
  # gRPC 服务 (需要 Experimental Channel)
  # - name: grpc-service
  #   port: 9090
  #   protocol: HTTPS
  #   tls:
  #     mode: Terminate
  #     certificateRefs:
  #     - name: grpc-tls
  #   allowedRoutes:
  #     kinds:
  #     - kind: GRPCRoute
  
  # TLS Passthrough (透传到后端)
  - name: tls-passthrough
    port: 8443
    protocol: TLS
    hostname: "secure.backend.com"
    tls:
      mode: Passthrough
    allowedRoutes:
      kinds:
      - kind: TLSRoute
```

### Gateway 基础设施配置 (v1.1+)

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: infra-gateway
  namespace: gateway-system
spec:
  gatewayClassName: istio
  
  # Gateway API v1.1+ 特性
  infrastructure:
    # 传递给底层 Service 的 Annotations
    annotations:
      service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
      service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    
    # 传递给底层 Pod 的 Labels
    labels:
      app: gateway
      version: v1.1
  
  listeners:
  - name: http
    port: 80
    protocol: HTTP
```

### 显式地址分配

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: fixed-ip-gateway
  namespace: gateway-system
spec:
  gatewayClassName: nginx
  
  # 显式指定网关地址 (云环境中预分配的 IP)
  addresses:
  - type: IPAddress
    value: "203.0.113.42"
  
  listeners:
  - name: http
    port: 80
    protocol: HTTP
```

---

## HTTPRoute 配置

### 字段规格表

| 字段路径 | 类型 | 必填 | 版本 | 说明 |
|----------|------|------|------|------|
| `spec.parentRefs[]` | array | ✅ | v1 | 绑定的 Gateway 列表 |
| `spec.parentRefs[].name` | string | ✅ | v1 | Gateway 名称 |
| `spec.parentRefs[].namespace` | string | ❌ | v1 | Gateway 命名空间(默认同命名空间) |
| `spec.parentRefs[].sectionName` | string | ❌ | v1 | Gateway 的特定 listener |
| `spec.hostnames[]` | array | ❌ | v1 | 主机名列表(支持通配符) |
| `spec.rules[]` | array | ✅ | v1 | 路由规则列表 |
| `spec.rules[].matches[]` | array | ❌ | v1 | 匹配条件(为空则匹配所有) |
| `spec.rules[].filters[]` | array | ❌ | v1 | 流量处理过滤器 |
| `spec.rules[].backendRefs[]` | array | ✅ | v1 | 后端服务引用 |
| `spec.rules[].backendRefs[].name` | string | ✅ | v1 | Service 名称 |
| `spec.rules[].backendRefs[].port` | int32 | ✅ | v1 | Service 端口 |
| `spec.rules[].backendRefs[].weight` | int32 | ❌ | v1 | 流量权重(默认1) |
| `spec.rules[].timeouts` | object | ❌ | v1.2+ | 超时配置 |
| `spec.rules[].timeouts.request` | duration | ❌ | v1.2+ | 请求超时(如"30s") |
| `spec.rules[].timeouts.backendRequest` | duration | ❌ | v1.2+ | 后端请求超时 |

### 基础路由

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: basic-route
  namespace: default
spec:
  # 绑定到 Gateway
  parentRefs:
  - name: production-gateway
    namespace: gateway-infra
  
  # 主机名匹配
  hostnames:
  - "api.example.com"
  
  rules:
  # 规则1: 所有请求路由到 backend 服务
  - backendRefs:
    - name: backend-svc
      port: 8080
```

### 路径匹配路由

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: path-based-route
  namespace: app-team
spec:
  parentRefs:
  - name: http-gateway
    namespace: gateway-system
  
  hostnames:
  - "myapp.example.com"
  
  rules:
  # 规则1: /api 路径前缀 -> API 服务
  - matches:
    - path:
        type: PathPrefix
        value: /api
    backendRefs:
    - name: api-service
      port: 8080
  
  # 规则2: /static 精确匹配 -> 静态文件服务
  - matches:
    - path:
        type: Exact
        value: /static
    backendRefs:
    - name: static-service
      port: 80
  
  # 规则3: /assets/* 正则匹配 -> CDN 服务
  - matches:
    - path:
        type: RegularExpression
        value: /assets/.*\\.jpg
    backendRefs:
    - name: cdn-service
      port: 8080
  
  # 规则4: 默认路由(无 matches)
  - backendRefs:
    - name: default-backend
      port: 8080
```

### 多条件匹配

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: advanced-match-route
  namespace: default
spec:
  parentRefs:
  - name: production-gateway
    namespace: gateway-infra
  
  rules:
  # 规则: 同时匹配路径、Header、Method、Query
  - matches:
    - path:
        type: PathPrefix
        value: /api/v2
      # HTTP 方法匹配
      method: POST
      # Header 匹配
      headers:
      - name: X-API-Version
        value: "2.0"
      # Query 参数匹配
      queryParams:
      - name: format
        value: json
    
    backendRefs:
    - name: api-v2-service
      port: 8080
```

### 权重路由(金丝雀发布)

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: canary-route
  namespace: production
spec:
  parentRefs:
  - name: production-gateway
    namespace: gateway-infra
  
  hostnames:
  - "app.example.com"
  
  rules:
  - backendRefs:
    # 90% 流量到稳定版本
    - name: app-v1
      port: 8080
      weight: 90
    
    # 10% 流量到金丝雀版本
    - name: app-v2
      port: 8080
      weight: 10
```

### 跨命名空间路由

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: cross-namespace-route
  namespace: team-a
spec:
  parentRefs:
  # 引用其他命名空间的 Gateway
  - name: shared-gateway
    namespace: gateway-system
  
  rules:
  - backendRefs:
    # 同命名空间的 Service
    - name: local-service
      port: 8080
    
    # 跨命名空间的 Service (需要 ReferenceGrant 授权)
    - name: shared-service
      namespace: shared-services
      port: 9090
---
# 必需：授权 team-a 命名空间引用 shared-services 中的 Service
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-team-a
  namespace: shared-services
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: team-a
  to:
  - group: ""
    kind: Service
```

### 带超时配置 (v1.2+)

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: timeout-route
  namespace: default
spec:
  parentRefs:
  - name: production-gateway
  
  rules:
  - backendRefs:
    - name: slow-backend
      port: 8080
    
    # 超时配置 (Gateway API v1.2+)
    timeouts:
      # 整体请求超时(包括重试)
      request: "30s"
      # 单次后端请求超时
      backendRequest: "10s"
```

---

## HTTPRouteMatch 匹配规则

### 匹配类型完整示例

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: match-examples
  namespace: default
spec:
  parentRefs:
  - name: production-gateway
  
  rules:
  # ==================== 路径匹配 ====================
  # 1. 前缀匹配 (PathPrefix)
  - matches:
    - path:
        type: PathPrefix
        value: /api/v1
    backendRefs:
    - name: api-v1
      port: 8080
  
  # 2. 精确匹配 (Exact)
  - matches:
    - path:
        type: Exact
        value: /health
    backendRefs:
    - name: health-check
      port: 8080
  
  # 3. 正则匹配 (RegularExpression)
  - matches:
    - path:
        type: RegularExpression
        value: /users/[0-9]+
    backendRefs:
    - name: user-service
      port: 8080
  
  # ==================== Header 匹配 ====================
  # 4. 精确 Header 值
  - matches:
    - headers:
      - type: Exact
        name: X-Custom-Header
        value: "production"
    backendRefs:
    - name: prod-backend
      port: 8080
  
  # 5. 正则 Header 值
  - matches:
    - headers:
      - type: RegularExpression
        name: User-Agent
        value: "Mozilla.*"
    backendRefs:
    - name: browser-optimized
      port: 8080
  
  # ==================== Query 参数匹配 ====================
  # 6. 精确 Query 参数
  - matches:
    - queryParams:
      - type: Exact
        name: version
        value: "2.0"
    backendRefs:
    - name: v2-backend
      port: 8080
  
  # 7. 正则 Query 参数
  - matches:
    - queryParams:
      - type: RegularExpression
        name: id
        value: "[a-f0-9]{8}"
    backendRefs:
    - name: uuid-handler
      port: 8080
  
  # ==================== HTTP 方法匹配 ====================
  # 8. GET 请求
  - matches:
    - method: GET
      path:
        type: PathPrefix
        value: /data
    backendRefs:
    - name: read-service
      port: 8080
  
  # 9. POST/PUT 写操作
  - matches:
    - method: POST
      path:
        type: PathPrefix
        value: /data
    - method: PUT
      path:
        type: PathPrefix
        value: /data
    backendRefs:
    - name: write-service
      port: 8080
```

### 复杂组合匹配

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: complex-match
  namespace: default
spec:
  parentRefs:
  - name: production-gateway
  
  rules:
  # 匹配逻辑: (path AND method AND headers AND queryParams)
  - matches:
    # Match 1: Admin API with token
    - path:
        type: PathPrefix
        value: /admin
      method: POST
      headers:
      - name: Authorization
        value: "Bearer .*"
        type: RegularExpression
      queryParams:
      - name: action
        value: "write"
    
    # Match 2: Public API
    - path:
        type: PathPrefix
        value: /public
    
    # 满足任一 match 则路由到此后端
    backendRefs:
    - name: admin-backend
      port: 8080
```

---

## HTTPRouteFilter 流量处理

### Filter 类型表

| Filter 类型 | 说明 | 用途 |
|-------------|------|------|
| `RequestHeaderModifier` | 修改请求头 | 添加认证信息、追踪ID |
| `ResponseHeaderModifier` | 修改响应头 | 添加 CORS、安全头 |
| `RequestRedirect` | HTTP 重定向 | HTTPS 跳转、域名迁移 |
| `URLRewrite` | URL 重写 | 路径改写、服务聚合 |
| `RequestMirror` | 流量镜像 | 生产流量复制到测试环境 |
| `ExtensionRef` | 自定义扩展 | 实现特定的 Filter 逻辑 |

### RequestHeaderModifier

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: header-modifier-route
  namespace: default
spec:
  parentRefs:
  - name: production-gateway
  
  rules:
  - filters:
    # 修改请求头
    - type: RequestHeaderModifier
      requestHeaderModifier:
        # 添加新 Header
        add:
        - name: X-Request-ID
          value: "uuid-12345"
        - name: X-Forwarded-Proto
          value: "https"
        
        # 设置 Header (覆盖已存在的)
        set:
        - name: X-Custom-Header
          value: "gateway-value"
        
        # 删除 Header
        remove:
        - "X-Internal-Debug"
        - "X-Legacy-Header"
    
    backendRefs:
    - name: backend-service
      port: 8080
```

### ResponseHeaderModifier

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: response-header-route
  namespace: default
spec:
  parentRefs:
  - name: production-gateway
  
  rules:
  - filters:
    # 修改响应头
    - type: ResponseHeaderModifier
      responseHeaderModifier:
        add:
        # CORS 头
        - name: Access-Control-Allow-Origin
          value: "*"
        - name: Access-Control-Allow-Methods
          value: "GET, POST, OPTIONS"
        
        # 安全头
        - name: Strict-Transport-Security
          value: "max-age=31536000; includeSubDomains"
        - name: X-Content-Type-Options
          value: "nosniff"
        - name: X-Frame-Options
          value: "DENY"
        
        remove:
        - "Server"
        - "X-Powered-By"
    
    backendRefs:
    - name: api-service
      port: 8080
```

### RequestRedirect

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: redirect-route
  namespace: default
spec:
  parentRefs:
  - name: production-gateway
  
  hostnames:
  - "old-domain.com"
  
  rules:
  # HTTP -> HTTPS 重定向
  - matches:
    - path:
        type: PathPrefix
        value: /
    filters:
    - type: RequestRedirect
      requestRedirect:
        scheme: https
        statusCode: 301
  
  ---
  # 域名迁移重定向
  - matches:
    - path:
        type: PathPrefix
        value: /
    filters:
    - type: RequestRedirect
      requestRedirect:
        hostname: new-domain.com
        statusCode: 301
  
  ---
  # 路径重定向
  - matches:
    - path:
        type: PathPrefix
        value: /old-api
    filters:
    - type: RequestRedirect
      requestRedirect:
        path:
          type: ReplaceFullPath
          replaceFullPath: /new-api
        statusCode: 302
```

### URLRewrite

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: rewrite-route
  namespace: default
spec:
  parentRefs:
  - name: production-gateway
  
  rules:
  # 路径前缀替换
  - matches:
    - path:
        type: PathPrefix
        value: /api/v1
    filters:
    - type: URLRewrite
      urlRewrite:
        path:
          type: ReplacePrefixMatch
          replacePrefixMatch: /v1
    backendRefs:
    - name: api-service
      port: 8080
  
  ---
  # 完整路径替换
  - matches:
    - path:
        type: Exact
        value: /healthz
    filters:
    - type: URLRewrite
      urlRewrite:
        path:
          type: ReplaceFullPath
          replaceFullPath: /health/check
    backendRefs:
    - name: health-service
      port: 8080
  
  ---
  # 主机名重写
  - matches:
    - path:
        type: PathPrefix
        value: /external
    filters:
    - type: URLRewrite
      urlRewrite:
        hostname: internal-service.cluster.local
    backendRefs:
    - name: internal-service
      port: 8080
```

### RequestMirror

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: mirror-route
  namespace: default
spec:
  parentRefs:
  - name: production-gateway
  
  rules:
  - filters:
    # 流量镜像: 将生产流量复制到测试环境
    - type: RequestMirror
      requestMirror:
        backendRef:
          name: test-backend
          port: 8080
    
    # 主流量仍发送到生产环境
    backendRefs:
    - name: production-backend
      port: 8080
```

### 多 Filter 组合

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: multi-filter-route
  namespace: default
spec:
  parentRefs:
  - name: production-gateway
  
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    
    # Filter 按顺序执行
    filters:
    # 1. URL 重写
    - type: URLRewrite
      urlRewrite:
        path:
          type: ReplacePrefixMatch
          replacePrefixMatch: /v2
    
    # 2. 修改请求头
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        - name: X-API-Gateway
          value: "gateway-v2"
    
    # 3. 流量镜像
    - type: RequestMirror
      requestMirror:
        backendRef:
          name: analytics-backend
          port: 9090
    
    # 4. 修改响应头
    - type: ResponseHeaderModifier
      responseHeaderModifier:
        add:
        - name: X-Response-Time
          value: "ms-timestamp"
    
    backendRefs:
    - name: api-backend
      port: 8080
```

---

## 内部实现原理

### 角色分离模型

```yaml
# Gateway API 的三层角色分离设计

# 【层1: 集群管理员】
# 职责: 安装网关控制器, 定义基础设施策略
# 资源: GatewayClass
---
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: production-gw
spec:
  controllerName: istio.io/gateway-controller

---
# 【层2: 平台运维/SRE】
# 职责: 创建网关实例, 配置监听器, 管理证书
# 资源: Gateway
---
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: shared-gateway
  namespace: gateway-infra
spec:
  gatewayClassName: production-gw
  listeners:
  - name: https
    port: 443
    protocol: HTTPS
    tls:
      certificateRefs:
      - name: wildcard-cert

---
# 【层3: 应用开发者】
# 职责: 定义路由规则, 绑定后端服务
# 资源: HTTPRoute, GRPCRoute 等
---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: my-app-route
  namespace: app-team
spec:
  parentRefs:
  - name: shared-gateway
    namespace: gateway-infra
  rules:
  - backendRefs:
    - name: my-app-svc
      port: 8080
```

### 路由绑定机制

```yaml
# Gateway 和 Route 的绑定流程

# 1. HTTPRoute 声明 parentRefs 绑定 Gateway
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: app-route
  namespace: team-a
spec:
  parentRefs:
  - name: shared-gateway
    namespace: gateway-system
    sectionName: https-listener  # 可选: 绑定特定 listener
  
  hostnames:
  - "app.example.com"
  
  rules:
  - backendRefs:
    - name: app-svc
      port: 8080

# 2. Gateway 通过 allowedRoutes 控制绑定权限
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: shared-gateway
  namespace: gateway-system
spec:
  gatewayClassName: istio
  listeners:
  - name: https-listener
    port: 443
    protocol: HTTPS
    tls:
      certificateRefs:
      - name: tls-cert
    
    # 绑定规则
    allowedRoutes:
      # 允许来自特定命名空间的路由
      namespaces:
        from: Selector
        selector:
          matchLabels:
            gateway-access: "enabled"
      
      # 允许的路由类型
      kinds:
      - kind: HTTPRoute
      - kind: GRPCRoute

# 3. 命名空间需要标签授权
apiVersion: v1
kind: Namespace
metadata:
  name: team-a
  labels:
    gateway-access: "enabled"

# 4. 如果需要跨命名空间引用资源, 需要 ReferenceGrant
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-team-a
  namespace: gateway-system
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: team-a
  to:
  - group: ""
    kind: Service
```

### 主机名匹配优先级

```yaml
# Gateway API 主机名匹配规则 (从高到低优先级)

# 优先级 1: 精确匹配
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: exact-hostname
spec:
  parentRefs:
  - name: gateway
  hostnames:
  - "api.example.com"  # 精确匹配优先级最高
  rules:
  - backendRefs:
    - name: api-exact
      port: 8080

---
# 优先级 2: 最长通配符前缀匹配
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: longest-wildcard
spec:
  hostnames:
  - "*.api.example.com"  # 匹配 foo.api.example.com
  rules:
  - backendRefs:
    - name: api-wildcard
      port: 8080

---
# 优先级 3: 较短通配符匹配
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: short-wildcard
spec:
  hostnames:
  - "*.example.com"  # 匹配 *.example.com 但优先级低于上面
  rules:
  - backendRefs:
    - name: wildcard
      port: 8080

---
# 优先级 4: 无主机名 (catch-all)
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: catch-all
spec:
  # 无 hostnames 字段, 匹配所有未被其他路由处理的主机名
  rules:
  - backendRefs:
    - name: default-backend
      port: 8080
```

### 控制器实现机制

```yaml
# Gateway API 控制器的工作原理

# 1. 控制器监听 Gateway 资源
#    - 创建底层负载均衡器 (如 LoadBalancer Service)
#    - 分配外部 IP 地址
#    - 配置 TLS 证书

# 2. 控制器监听 HTTPRoute 资源
#    - 解析路由规则
#    - 配置数据平面 (Envoy, Nginx, Istio 等)
#    - 更新负载均衡器配置

# 示例: Istio Gateway Controller 创建的底层资源
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: my-gateway
  namespace: default
spec:
  gatewayClassName: istio
  listeners:
  - name: http
    port: 80
    protocol: HTTP

# Istio 控制器会自动创建:
# 1. Deployment (istio-gateway pod)
# 2. Service (LoadBalancer type)
# 3. ConfigMap (Envoy 配置)

# 查看自动创建的资源:
# kubectl get svc,deploy,configmap -l gateway.networking.k8s.io/gateway-name=my-gateway
```

---

## 生产实战案例

### 案例1: 多租户 Gateway 架构

```yaml
# 场景: SaaS 平台为每个租户提供独立域名, 共享网关基础设施

# 【平台层: Gateway】
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: saas-platform-gateway
  namespace: platform-infra
spec:
  gatewayClassName: istio
  
  listeners:
  # HTTP -> HTTPS 重定向
  - name: http
    port: 80
    protocol: HTTP
    allowedRoutes:
      namespaces:
        from: All
  
  # HTTPS 主监听器
  - name: https
    port: 443
    protocol: HTTPS
    hostname: "*.saas-platform.com"
    tls:
      mode: Terminate
      certificateRefs:
      - name: wildcard-tls
    allowedRoutes:
      namespaces:
        from: Selector
        selector:
          matchLabels:
            tenant: "true"

---
# 【租户 A: 命名空间 + HTTPRoute】
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a
  labels:
    tenant: "true"
---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: tenant-a-route
  namespace: tenant-a
spec:
  parentRefs:
  - name: saas-platform-gateway
    namespace: platform-infra
  
  hostnames:
  - "tenant-a.saas-platform.com"
  
  rules:
  # HTTP -> HTTPS 重定向
  - matches:
    - path:
        type: PathPrefix
        value: /
    filters:
    - type: RequestRedirect
      requestRedirect:
        scheme: https
        statusCode: 301
  
  # HTTPS 流量
  - matches:
    - path:
        type: PathPrefix
        value: /
    filters:
    # 注入租户标识
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        - name: X-Tenant-ID
          value: "tenant-a"
    backendRefs:
    - name: tenant-a-app
      port: 8080

---
# 【租户 B: 独立命名空间】
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-b
  labels:
    tenant: "true"
---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: tenant-b-route
  namespace: tenant-b
spec:
  parentRefs:
  - name: saas-platform-gateway
    namespace: platform-infra
  
  hostnames:
  - "tenant-b.saas-platform.com"
  
  rules:
  - filters:
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        - name: X-Tenant-ID
          value: "tenant-b"
    backendRefs:
    - name: tenant-b-app
      port: 8080
```

### 案例2: 金丝雀发布 + A/B 测试

```yaml
# 场景: 新版本灰度发布, 同时支持 Header 定向测试

apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: canary-release
  namespace: production
spec:
  parentRefs:
  - name: production-gateway
    namespace: gateway-infra
  
  hostnames:
  - "app.example.com"
  
  rules:
  # 规则1: Beta 测试用户 -> v2 版本 (100% 流量)
  - matches:
    - headers:
      - name: X-Beta-User
        value: "true"
    backendRefs:
    - name: app-v2
      port: 8080
      weight: 100
  
  # 规则2: 内部员工 -> v2 版本 (100% 流量)
  - matches:
    - headers:
      - name: X-Employee-ID
        type: RegularExpression
        value: ".*"
    backendRefs:
    - name: app-v2
      port: 8080
  
  # 规则3: 普通用户 -> 灰度发布 (10% v2, 90% v1)
  - backendRefs:
    - name: app-v1
      port: 8080
      weight: 90
    - name: app-v2
      port: 8080
      weight: 10

---
# 监控金丝雀指标 (Prometheus)
apiVersion: v1
kind: Service
metadata:
  name: app-v2
  namespace: production
  labels:
    app: myapp
    version: v2
  annotations:
    prometheus.io/scrape: "true"
    prometheus.io/path: "/metrics"
    prometheus.io/port: "8080"
spec:
  selector:
    app: myapp
    version: v2
  ports:
  - port: 8080
```

### 案例3: 生产流量镜像 + 测试环境

```yaml
# 场景: 将生产流量镜像到测试环境, 验证新版本功能

apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: traffic-mirror
  namespace: production
spec:
  parentRefs:
  - name: production-gateway
    namespace: gateway-infra
  
  hostnames:
  - "api.example.com"
  
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api/v2
    
    filters:
    # 镜像到测试环境 (异步, 不影响生产响应)
    - type: RequestMirror
      requestMirror:
        backendRef:
          name: api-v2-test
          namespace: testing
          port: 8080
    
    # 修改镜像请求的 Header (可选)
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        - name: X-Mirror-Request
          value: "true"
    
    # 主流量仍发送到生产环境
    backendRefs:
    - name: api-v2-production
      port: 8080

---
# 测试环境 Service
apiVersion: v1
kind: Service
metadata:
  name: api-v2-test
  namespace: testing
spec:
  selector:
    app: api
    version: v2
    environment: test
  ports:
  - port: 8080

---
# 跨命名空间授权
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-mirror-to-test
  namespace: testing
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: production
  to:
  - group: ""
    kind: Service
    name: api-v2-test
```

### 案例4: API 网关聚合多服务

```yaml
# 场景: 单一网关入口聚合多个微服务

apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: api-gateway
  namespace: api-platform
spec:
  gatewayClassName: nginx
  
  listeners:
  - name: https
    port: 443
    protocol: HTTPS
    hostname: "api.company.com"
    tls:
      certificateRefs:
      - name: api-tls-cert

---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: api-aggregation
  namespace: api-platform
spec:
  parentRefs:
  - name: api-gateway
  
  hostnames:
  - "api.company.com"
  
  rules:
  # 用户服务
  - matches:
    - path:
        type: PathPrefix
        value: /users
    filters:
    - type: URLRewrite
      urlRewrite:
        path:
          type: ReplacePrefixMatch
          replacePrefixMatch: /api/v1/users
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        - name: X-Service
          value: "user-service"
    backendRefs:
    - name: user-service
      namespace: services
      port: 8080
  
  # 订单服务
  - matches:
    - path:
        type: PathPrefix
        value: /orders
    filters:
    - type: URLRewrite
      urlRewrite:
        path:
          type: ReplacePrefixMatch
          replacePrefixMatch: /api/v1/orders
    backendRefs:
    - name: order-service
      namespace: services
      port: 8081
  
  # 支付服务 (高安全性要求)
  - matches:
    - path:
        type: PathPrefix
        value: /payments
    filters:
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        - name: X-Request-ID
          value: "uuid-generated"
        - name: X-Forwarded-Proto
          value: "https"
    backendRefs:
    - name: payment-service
      namespace: secure-services
      port: 8443
  
  # 认证服务
  - matches:
    - path:
        type: PathPrefix
        value: /auth
    backendRefs:
    - name: auth-service
      namespace: services
      port: 9090

---
# 跨命名空间授权
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-api-platform
  namespace: services
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: api-platform
  to:
  - group: ""
    kind: Service
```

---

## 版本兼容性与最佳实践

### Gateway API 版本演进

| Gateway API 版本 | Kubernetes 版本 | 主要特性 | 状态 |
|------------------|-----------------|----------|------|
| v1.0.0 | v1.25+ | GatewayClass, Gateway, HTTPRoute(Standard) | GA |
| v1.1.0 | v1.27+ | `infrastructure`, BackendTLSPolicy(Experimental) | GA |
| v1.2.0 | v1.29+ | `timeouts`, Session Persistence(Experimental) | GA |
| v1.3.0 | v1.31+ | 增强 GRPC 支持, 改进 TLS 配置 | GA |

### Kubernetes 版本支持

```yaml
# Gateway API 需要单独安装 CRD

# 标准版 (Standard Channel - GA 资源)
kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.0.0/standard-install.yaml

# 实验版 (Experimental Channel - 包含 GRPCRoute, TCPRoute 等)
kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.0.0/experimental-install.yaml

# 验证安装
kubectl get crd | grep gateway
# 应输出:
# gatewayclasses.gateway.networking.k8s.io
# gateways.gateway.networking.k8s.io
# httproutes.gateway.networking.k8s.io
# referencegrants.gateway.networking.k8s.io
```

### 最佳实践

#### 1. 命名空间隔离策略

```yaml
# 推荐: 三层命名空间架构
# - gateway-system: 放置 Gateway 资源(平台团队管理)
# - app-team-*: 放置 HTTPRoute 资源(应用团队管理)
# - shared-services: 共享基础服务

apiVersion: v1
kind: Namespace
metadata:
  name: gateway-system
  labels:
    role: infrastructure
---
apiVersion: v1
kind: Namespace
metadata:
  name: app-team-1
  labels:
    team: team1
    gateway-access: "enabled"
---
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: shared-gateway
  namespace: gateway-system
spec:
  gatewayClassName: istio
  listeners:
  - name: https
    port: 443
    protocol: HTTPS
    tls:
      certificateRefs:
      - name: wildcard-cert
    allowedRoutes:
      namespaces:
        from: Selector
        selector:
          matchLabels:
            gateway-access: "enabled"
```

#### 2. TLS 证书管理

```yaml
# 推荐: 使用 cert-manager 自动管理证书

# 1. 安装 cert-manager
# kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.13.0/cert-manager.yaml

# 2. 创建 ClusterIssuer
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-prod
    solvers:
    - http01:
        gatewayHTTPRoute:
          parentRefs:
          - name: shared-gateway
            namespace: gateway-system

---
# 3. 创建 Certificate (自动生成 Secret)
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: api-tls-cert
  namespace: gateway-system
spec:
  secretName: api-tls-cert
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
  - api.example.com
  - "*.api.example.com"

---
# 4. Gateway 引用自动生成的 Secret
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: secure-gateway
  namespace: gateway-system
spec:
  gatewayClassName: istio
  listeners:
  - name: https
    port: 443
    protocol: HTTPS
    tls:
      certificateRefs:
      - name: api-tls-cert
```

#### 3. 可观测性配置

```yaml
# 推荐: 集成 OpenTelemetry/Prometheus 监控

apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: monitored-gateway
  namespace: gateway-system
  annotations:
    # 启用访问日志
    istio.io/accessLogging: "true"
    # Prometheus 指标
    prometheus.io/scrape: "true"
    prometheus.io/port: "15020"
spec:
  gatewayClassName: istio
  listeners:
  - name: https
    port: 443
    protocol: HTTPS

---
# HTTPRoute 添加追踪标识
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: traced-route
  namespace: default
spec:
  parentRefs:
  - name: monitored-gateway
    namespace: gateway-system
  
  rules:
  - filters:
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        # 传播追踪上下文
        - name: X-Request-ID
          value: "${request_id}"
        - name: X-B3-TraceId
          value: "${trace_id}"
    backendRefs:
    - name: app-service
      port: 8080
```

#### 4. 安全加固

```yaml
# 推荐: 配置严格的访问控制和安全头

apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: secure-route
  namespace: production
spec:
  parentRefs:
  - name: production-gateway
    namespace: gateway-system
  
  hostnames:
  - "app.example.com"
  
  rules:
  # 强制 HTTPS
  - matches:
    - path:
        type: PathPrefix
        value: /
    filters:
    # 添加安全响应头
    - type: ResponseHeaderModifier
      responseHeaderModifier:
        add:
        # HSTS
        - name: Strict-Transport-Security
          value: "max-age=31536000; includeSubDomains; preload"
        # 防止 MIME 嗅探
        - name: X-Content-Type-Options
          value: "nosniff"
        # 防止点击劫持
        - name: X-Frame-Options
          value: "DENY"
        # CSP
        - name: Content-Security-Policy
          value: "default-src 'self'; script-src 'self' 'unsafe-inline'"
        # 推荐政策
        - name: Referrer-Policy
          value: "strict-origin-when-cross-origin"
        
        remove:
        # 移除服务器指纹信息
        - "Server"
        - "X-Powered-By"
    
    backendRefs:
    - name: app-service
      port: 8080
```

### FAQ

#### Q1: Gateway API 与 Ingress 如何选择?

**A:** 选择标准:
- **新项目**: 优先使用 Gateway API (更灵活, 未来趋势)
- **简单场景**: Ingress 足够 (HTTP/HTTPS 基础路由)
- **多协议**: Gateway API (支持 gRPC, TCP, UDP)
- **多团队**: Gateway API (角色分离模型)
- **遗留系统**: 保持 Ingress (迁移成本高)

#### Q2: 如何平滑从 Ingress 迁移到 Gateway API?

**A:** 迁移策略:
```yaml
# 1. 并行运行: 保留 Ingress, 创建等效的 HTTPRoute
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: legacy-ingress
spec:
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
              number: 8080
---
# 等效的 Gateway API 配置
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: migrated-route
spec:
  parentRefs:
  - name: new-gateway
  hostnames:
  - "app.example.com"
  rules:
  - backendRefs:
    - name: app-service
      port: 8080

# 2. 逐步切换 DNS 到新网关
# 3. 验证流量后删除 Ingress
```

#### Q3: 如何调试 HTTPRoute 不生效?

**A:** 诊断步骤:
```bash
# 1. 检查 Gateway 状态
kubectl get gateway -A
kubectl describe gateway <gateway-name> -n <namespace>

# 2. 检查 HTTPRoute 状态
kubectl get httproute -A
kubectl describe httproute <route-name> -n <namespace>

# 3. 查看 Route 绑定状态
kubectl get httproute <route-name> -n <namespace> -o yaml | grep -A 10 status

# 4. 验证 allowedRoutes 配置
kubectl get gateway <gateway-name> -n <namespace> -o jsonpath='{.spec.listeners[*].allowedRoutes}'

# 5. 检查控制器日志
kubectl logs -n gateway-system -l app=gateway-controller
```

#### Q4: 权重路由的流量分配精确吗?

**A:** 权重是**目标比例**, 不是绝对保证:
- 小流量下可能有偏差 (如 10 个请求 90/10 分配)
- 大流量下趋向精确 (如 10000 个请求)
- 依赖控制器实现 (Envoy/Nginx 负载均衡算法)
- 建议监控实际分配比例

#### Q5: RequestMirror 会影响主流量性能吗?

**A:** 镜像是**异步非阻塞**:
- 主请求不等待镜像响应
- 镜像失败不影响主流量
- 会增加网关出口带宽消耗
- 建议镜像到独立集群/命名空间

---

## 相关资源

### 官方文档
- Gateway API 官网: https://gateway-api.sigs.k8s.io/
- API 参考: https://gateway-api.sigs.k8s.io/reference/spec/
- 实现列表: https://gateway-api.sigs.k8s.io/implementations/

### 控制器实现
- Istio Gateway: https://istio.io/latest/docs/tasks/traffic-management/ingress/gateway-api/
- Nginx Gateway Fabric: https://github.com/nginxinc/nginx-gateway-fabric
- Envoy Gateway: https://gateway.envoyproxy.io/
- Traefik: https://doc.traefik.io/traefik/routing/providers/kubernetes-gateway/

### 本知识库相关文档
- [12 - Gateway API 高级路由](./12-gateway-api-advanced-routes.md)
- [Ingress 完整配置参考](./08-ingress-all-classes.md)
- [Service YAML 参考](./02-service-all-types.md)
- [Gateway API 深度解析](../domain-5-networking/35-gateway-api-overview.md)

---

**最后更新**: 2026-02 | **维护者**: Kudig.io 社区 | **反馈**: [GitHub Issues](https://github.com/kudig-io/kudig-database)
