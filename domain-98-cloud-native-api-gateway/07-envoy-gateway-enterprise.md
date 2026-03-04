# 07 - Envoy Gateway 企业级实践

> **文档版本**: v1.0 | **适用版本**: Envoy Gateway 1.x, Kubernetes 1.26+ | **更新日期**: 2026-03-04 | **关键词**: Envoy Gateway, Gateway API, xDS, EnvoyPatchPolicy, ExtensionPolicy

## 目录

1. [项目概述](#1-项目概述)
2. [核心架构](#2-核心架构)
3. [部署安装](#3-部署安装)
4. [Gateway API 原生使用](#4-gateway-api-原生使用)
5. [策略扩展体系](#5-策略扩展体系)
6. [EnvoyPatchPolicy 高级配置](#6-envoypatchpolicy-高级配置)
7. [Wasm 扩展](#7-wasm-扩展)
8. [可观测性](#8-可观测性)
9. [生产部署建议](#9-生产部署建议)
10. [与其他网关对比](#10-与其他网关对比)

---

## 1. 项目概述

Envoy Gateway（简称 EG）是 Envoy 社区于 2022 年正式发起的官方 Kubernetes API 网关项目，目标是以 **Gateway API First** 的设计原则，为 Envoy Proxy 提供标准化、云原生的控制平面。

### 核心定位

- **CNCF 官方项目**：与 Envoy Proxy 同属 CNCF 生态，2023 年加入沙箱，2024 年晋升孵化级
- **Gateway API First**：100% 以 Kubernetes Gateway API 为主接口，不引入自有私有 CRD 作为核心路由对象
- **官方背书**：由 Envoy 核心维护者主导，矩阵涵盖 Tetrate、Microsoft、VMware 等主要贡献者
- **轻量控制面**：相比 Istio/Contour 等，控制面极为精简，仅运行单个 `envoy-gateway` 控制器 Pod

### 适用场景

| 场景 | 推荐理由 |
|------|---------|
| 已采用 Gateway API 标准的团队 | 最纯粹的 Gateway API 实现 |
| 需要 Envoy 原生能力（xDS、Wasm） | 可直接通过 EnvoyPatchPolicy 注入原始 xDS |
| 替换 Ingress 走向标准化 | 平滑迁移路径，官方社区支持 |
| 多租户 Kubernetes 平台 | 细粒度命名空间隔离策略 |

### 版本里程碑

| 版本 | 重要特性 |
|------|---------|
| v0.3 | GA 首个稳定版，支持 HTTPRoute/GRPCRoute |
| v0.5 | BackendTLSPolicy、RateLimitPolicy |
| v1.0 | 生产就绪，EnvoyPatchPolicy GA |
| v1.1+ | Wasm 扩展、MergeGateways、多监听器 |

---

## 2. 核心架构

### 整体架构图

```
┌────────────────────────────────────────────────────────────────────┐
│                     Envoy Gateway 架构                              │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌─────────────────────────────────────────────────────┐           │
│  │                   控制平面（Control Plane）            │           │
│  │                                                     │           │
│  │  ┌─────────────────────────────────────────────┐    │           │
│  │  │          envoy-gateway Controller           │    │           │
│  │  │                                             │    │           │
│  │  │  ┌─────────────┐   ┌────────────────────┐   │    │           │
│  │  │  │ K8s Watcher │   │  Gateway API        │   │    │           │
│  │  │  │ (Informers) │──▶│  Translator         │   │    │           │
│  │  │  └─────────────┘   └────────┬───────────┘   │    │           │
│  │  │                             │                │    │           │
│  │  │  ┌──────────────────────────▼─────────────┐  │    │           │
│  │  │  │         xDS Server (IR → xDS)           │  │    │           │
│  │  │  │  Listener / Cluster / Route / Endpoint  │  │    │           │
│  │  │  └──────────────────────────┬─────────────┘  │    │           │
│  │  └─────────────────────────────│───────────────┘    │           │
│  └────────────────────────────────│───────────────────┘           │
│                                   │ xDS gRPC                       │
│  ┌────────────────────────────────▼───────────────────┐           │
│  │               数据平面（Data Plane）                  │           │
│  │                                                    │           │
│  │   ┌──────────────┐  ┌──────────────┐               │           │
│  │   │  Envoy Proxy │  │  Envoy Proxy │  ... (多副本)   │           │
│  │   │  (Pod)       │  │  (Pod)       │               │           │
│  │   └──────┬───────┘  └──────────────┘               │           │
│  └──────────│──────────────────────────────────────────┘           │
│             │                                                      │
│    ┌────────▼────────┐                                             │
│    │  客户端流量入口   │  (LoadBalancer Service / NodePort)           │
│    └─────────────────┘                                             │
│                                                                    │
│  Kubernetes 资源（CRD）:                                             │
│  GatewayClass → Gateway → HTTPRoute / GRPCRoute / TCPRoute         │
│  SecurityPolicy / BackendTLSPolicy / RateLimitPolicy               │
│  EnvoyPatchPolicy / EnvoyExtensionPolicy                           │
└────────────────────────────────────────────────────────────────────┘
```

### xDS 配置流转

```
Kubernetes CRD                IR（内部表示）             xDS 资源
─────────────────────────────────────────────────────────────────────
GatewayClass                                            
   └─ Gateway            ──▶  GatewayIR            ──▶  Listener
         └─ HTTPRoute    ──▶  HTTPRouteIR           ──▶  RouteConfiguration
               └─ BackendRef ──▶  ClusterIR         ──▶  Cluster + Endpoint
```

### 组件说明

| 组件 | 职责 |
|------|------|
| `envoy-gateway` Controller | 监听 K8s 资源，翻译成 xDS，下发给 Envoy |
| `envoy-proxy` DaemonSet/Deployment | 实际处理流量，接受 xDS 动态配置 |
| `certgen` Job | 自动生成控制面与数据面之间的 mTLS 证书 |
| `rate-limit` Deployment（可选） | 独立的全局限速服务（基于 Envoy ratelimit） |

---

## 3. 部署安装

### 前置要求

```bash
# 检查 Kubernetes 版本（需要 1.26+）
kubectl version --short

# 安装 Gateway API CRD（标准渠道）
kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.2.0/standard-install.yaml

# 安装实验渠道 CRD（含 TCPRoute、TLSRoute、GRPCRoute 等）
kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.2.0/experimental-install.yaml
```

### Helm 安装

```bash
# 添加 Helm 仓库
helm repo add envoy-gateway https://charts.envoyproxy.io
helm repo update

# 查看可用版本
helm search repo envoy-gateway/gateway-helm

# 安装 Envoy Gateway（最新稳定版）
helm install eg envoy-gateway/gateway-helm \
  --version v1.2.0 \
  --namespace envoy-gateway-system \
  --create-namespace \
  --wait

# 验证安装
kubectl get pods -n envoy-gateway-system
```

### 快速验证

```bash
# 部署示例应用
kubectl apply -f https://raw.githubusercontent.com/envoyproxy/gateway/main/examples/kubernetes/quickstart.yaml

# 检查 GatewayClass 和 Gateway 状态
kubectl get gatewayclass
kubectl get gateway -n default

# 获取 Gateway 地址
export GATEWAY_HOST=$(kubectl get gateway/eg -n default -o jsonpath='{.status.addresses[0].value}')

# 测试路由
curl -H "Host: www.example.com" http://$GATEWAY_HOST/
```

### EnvoyProxy 自定义配置

```yaml
# envoyproxy-config.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: EnvoyProxy
metadata:
  name: custom-proxy-config
  namespace: envoy-gateway-system
spec:
  provider:
    type: Kubernetes
    kubernetes:
      envoyDeployment:
        replicas: 3
        pod:
          labels:
            app: envoy-gateway-proxy
          annotations:
            prometheus.io/scrape: "true"
            prometheus.io/port: "19001"
        container:
          resources:
            requests:
              cpu: "500m"
              memory: "512Mi"
            limits:
              cpu: "2"
              memory: "2Gi"
      envoyService:
        type: LoadBalancer
        annotations:
          service.beta.kubernetes.io/aws-load-balancer-type: "external"
  logging:
    level:
      default: info
```

---

## 4. Gateway API 原生使用

### GatewayClass 配置

```yaml
# gatewayclass.yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: eg
spec:
  controllerName: gateway.envoyproxy.io/gatewayclass-controller
  parametersRef:
    group: gateway.envoyproxy.io
    kind: EnvoyProxy
    name: custom-proxy-config
    namespace: envoy-gateway-system
```

### Gateway 多监听器配置

```yaml
# gateway-multi-listener.yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: eg
  namespace: default
spec:
  gatewayClassName: eg
  listeners:
  - name: http
    protocol: HTTP
    port: 80
    allowedRoutes:
      namespaces:
        from: All
  - name: https
    protocol: HTTPS
    port: 443
    tls:
      mode: Terminate
      certificateRefs:
      - kind: Secret
        name: tls-secret
    allowedRoutes:
      namespaces:
        from: All
  - name: grpc
    protocol: HTTPS
    port: 8443
    tls:
      mode: Terminate
      certificateRefs:
      - kind: Secret
        name: grpc-tls-secret
```

### HTTPRoute 路由规则

```yaml
# httproute-advanced.yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: app-routes
  namespace: default
spec:
  parentRefs:
  - name: eg
    sectionName: https
  hostnames:
  - "api.example.com"
  rules:
  # 精确路径匹配
  - matches:
    - path:
        type: Exact
        value: /health
    backendRefs:
    - name: health-service
      port: 8080

  # 前缀匹配 + Header 过滤
  - matches:
    - path:
        type: PathPrefix
        value: /api/v1
      headers:
      - name: X-Environment
        value: production
    filters:
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        - name: X-Gateway
          value: envoy-gateway
        remove:
        - X-Internal-Token
    backendRefs:
    - name: api-v1-service
      port: 8080
      weight: 90
    - name: api-v1-canary
      port: 8080
      weight: 10

  # URL 重写
  - matches:
    - path:
        type: PathPrefix
        value: /legacy
    filters:
    - type: URLRewrite
      urlRewrite:
        path:
          type: ReplacePrefixMatch
          replacePrefixMatch: /api/v2
    backendRefs:
    - name: api-v2-service
      port: 8080

  # 重定向
  - matches:
    - path:
        type: PathPrefix
        value: /old-path
    filters:
    - type: RequestRedirect
      requestRedirect:
        scheme: https
        hostname: new.example.com
        path:
          type: ReplacePrefixMatch
          replacePrefixMatch: /new-path
        statusCode: 301
```

### GRPCRoute 配置

```yaml
# grpcroute.yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GRPCRoute
metadata:
  name: grpc-routes
  namespace: default
spec:
  parentRefs:
  - name: eg
    sectionName: grpc
  hostnames:
  - "grpc.example.com"
  rules:
  - matches:
    - method:
        service: com.example.UserService
        method: GetUser
    backendRefs:
    - name: user-grpc-service
      port: 9090
  - matches:
    - method:
        service: com.example.OrderService
    backendRefs:
    - name: order-grpc-service
      port: 9090
```

### TCPRoute 四层路由

```yaml
# tcproute.yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TCPRoute
metadata:
  name: tcp-backend
  namespace: default
spec:
  parentRefs:
  - name: eg
    sectionName: tcp-listener
  rules:
  - backendRefs:
    - name: tcp-service
      port: 5432
```

---

## 5. 策略扩展体系

Envoy Gateway 通过**策略附着（Policy Attachment）**模式在 Gateway API 对象上叠加能力，遵循 GEP-713 规范。

### 策略层级关系

```
GatewayClass
   └── Gateway                ←── ClientTrafficPolicy
         └── HTTPRoute        ←── BackendTrafficPolicy
               └── BackendRef ←── BackendTLSPolicy
                              ←── SecurityPolicy
                              ←── RateLimitPolicy (附着在 HTTPRoute)
```

### SecurityPolicy（认证/授权）

```yaml
# security-policy.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: SecurityPolicy
metadata:
  name: jwt-authn-policy
  namespace: default
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: app-routes
  jwt:
    providers:
    - name: keycloak
      issuer: "https://keycloak.example.com/realms/myrealm"
      audiences:
      - "my-api"
      remoteJWKS:
        uri: "https://keycloak.example.com/realms/myrealm/protocol/openid-connect/certs"
  cors:
    allowOrigins:
    - "https://app.example.com"
    - "https://admin.example.com"
    allowMethods:
    - GET
    - POST
    - PUT
    - DELETE
    allowHeaders:
    - Authorization
    - Content-Type
    maxAge: 3600
```

### RateLimitPolicy（限流）

```yaml
# ratelimit-policy.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: BackendTrafficPolicy
metadata:
  name: ratelimit-policy
  namespace: default
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: app-routes
  rateLimit:
    type: Global        # Global（全局共享）或 Local（每 Pod 独立）
    global:
      rules:
      # 基于 IP 限流
      - clientSelectors:
        - remoteAddress:
            distinct: true
        limit:
          requests: 100
          unit: Minute
      # 基于 Header 限流（按 API Key）
      - clientSelectors:
        - headers:
          - name: X-Api-Key
            type: Distinct
        limit:
          requests: 1000
          unit: Hour
```

### BackendTLSPolicy（后端 TLS）

```yaml
# backend-tls-policy.yaml
apiVersion: gateway.networking.k8s.io/v1alpha3
kind: BackendTLSPolicy
metadata:
  name: backend-tls
  namespace: default
spec:
  targetRefs:
  - group: ""
    kind: Service
    name: secure-backend
    port: 8443
  validation:
    caCertificateRefs:
    - kind: ConfigMap
      name: backend-ca-cert
    hostname: secure-backend.default.svc.cluster.local
```

### ClientTrafficPolicy（客户端流量控制）

```yaml
# client-traffic-policy.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: ClientTrafficPolicy
metadata:
  name: client-traffic
  namespace: default
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: Gateway
    name: eg
  tcpKeepalive:
    probes: 3
    idleTime: "20m"
    interval: "60s"
  headers:
    enableEnvoyHeaders: true
  http1:
    enableTrailers: true
  http2:
    initialStreamWindowSize: 65536
    initialConnectionWindowSize: 1048576
  timeout:
    http:
      requestReceivedTimeout: "10s"
```

---

## 6. EnvoyPatchPolicy 高级配置

`EnvoyPatchPolicy` 允许对 Envoy Gateway 生成的 xDS 配置进行**直接修改**，适用于 EG 尚未通过策略 API 暴露的底层 Envoy 能力。

> ⚠️ **警告**：EnvoyPatchPolicy 绕过了 EG 的抽象层，可能导致升级不兼容，建议仅在无其他方案时使用。

### 架构示意

```
Gateway API CRD
      │
      ▼
EG Translator（生成 Envoy xDS）
      │
      ▼  ← EnvoyPatchPolicy 在此注入 JSON Patch
Patched xDS Config
      │
      ▼
Envoy Proxy（加载最终配置）
```

### 示例：添加自定义 Lua Filter

```yaml
# envoy-patch-lua.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: EnvoyPatchPolicy
metadata:
  name: add-lua-filter
  namespace: default
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: Gateway
    name: eg
  type: JSONPatch
  jsonPatches:
  - type: "type.googleapis.com/envoy.config.listener.v3.Listener"
    name: "default/eg/http"
    operation:
      op: add
      path: "/filter_chains/0/filters/0/typed_config/http_filters/0"
      value:
        name: "envoy.filters.http.lua"
        typed_config:
          "@type": "type.googleapis.com/envoy.extensions.filters.http.lua.v3.LuaPerRoute"
          inline_code: |
            function envoy_on_request(request_handle)
              request_handle:headers():add("X-Request-Time", tostring(os.time()))
            end
```

### 示例：调整 Cluster 连接池参数

```yaml
# envoy-patch-cluster.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: EnvoyPatchPolicy
metadata:
  name: cluster-tuning
  namespace: default
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: Gateway
    name: eg
  type: JSONPatch
  jsonPatches:
  - type: "type.googleapis.com/envoy.config.cluster.v3.Cluster"
    name: "default/backend-service/8080"
    operation:
      op: replace
      path: "/upstream_connection_options/tcp_keepalive/keepalive_probes"
      value: 5
  - type: "type.googleapis.com/envoy.config.cluster.v3.Cluster"
    name: "default/backend-service/8080"
    operation:
      op: add
      path: "/circuit_breakers"
      value:
        thresholds:
        - priority: DEFAULT
          max_connections: 1000
          max_requests: 1000
          max_pending_requests: 500
```

### 验证 Patch 效果

```bash
# 检查 EnvoyPatchPolicy 状态
kubectl get envoypatchpolicy -n default

# 查看实际下发的 xDS 配置（需要访问 EG 管理端口）
kubectl port-forward -n envoy-gateway-system pod/eg-envoy-proxy-xxx 19000:19000

# 查看 Listener 配置
curl http://localhost:19000/config_dump?resource=listener | jq '.configs[].dynamic_listeners'

# 查看 Cluster 配置
curl http://localhost:19000/config_dump?resource=cluster | jq '.configs[].dynamic_active_clusters'
```

---

## 7. Wasm 扩展

Envoy Gateway 通过 `EnvoyExtensionPolicy` 支持加载 WebAssembly（Wasm）插件，实现自定义流量处理逻辑。

### Wasm 插件加载方式

| 方式 | 场景 |
|------|------|
| OCI 镜像（推荐） | 版本管理方便，企业环境首选 |
| HTTP URL | 简单测试场景 |
| ConfigMap | 小型 Wasm 模块 |

### EnvoyExtensionPolicy 配置

```yaml
# envoy-extension-wasm.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: EnvoyExtensionPolicy
metadata:
  name: wasm-auth-plugin
  namespace: default
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: app-routes
  wasm:
  - name: custom-auth
    rootID: "custom_auth_root"
    code:
      type: Image
      image:
        url: "registry.example.com/wasm-plugins/custom-auth:v1.2.0"
        pullSecretRef:
          name: registry-credentials
    config:
      apiKeyHeader: "X-API-Key"
      validationEndpoint: "http://auth-service.default.svc.cluster.local/validate"
    failOpen: false
```

### Wasm 插件开发（Go SDK 示例）

```go
// main.go - 使用 proxy-wasm-go-sdk
package main

import (
    "github.com/tetratelabs/proxy-wasm-go-sdk/proxywasm"
    "github.com/tetratelabs/proxy-wasm-go-sdk/proxywasm/types"
)

func main() {
    proxywasm.SetVMContext(&vmContext{})
}

type vmContext struct{}

func (*vmContext) NewPluginContext(contextID uint32) types.PluginContext {
    return &pluginContext{}
}

type pluginContext struct {
    types.DefaultPluginContext
}

func (ctx *pluginContext) NewHttpContext(contextID uint32) types.HttpContext {
    return &httpContext{contextID: contextID}
}

type httpContext struct {
    types.DefaultHttpContext
    contextID uint32
}

func (ctx *httpContext) OnHttpRequestHeaders(numHeaders int, endOfStream bool) types.Action {
    // 读取请求头
    apiKey, err := proxywasm.GetHttpRequestHeader("X-API-Key")
    if err != nil || apiKey == "" {
        _ = proxywasm.SendHttpResponse(401, nil, []byte("Unauthorized"), -1)
        return types.ActionPause
    }
    // 添加追踪头
    _ = proxywasm.AddHttpRequestHeader("X-Wasm-Processed", "true")
    return types.ActionContinue
}
```

```bash
# 编译 Wasm 模块
tinygo build -o custom-auth.wasm -scheduler=none -target=wasi ./main.go

# 打包为 OCI 镜像
docker build -t registry.example.com/wasm-plugins/custom-auth:v1.2.0 \
  --build-arg WASM_FILE=custom-auth.wasm .
docker push registry.example.com/wasm-plugins/custom-auth:v1.2.0
```

---

## 8. 可观测性

### Prometheus 指标采集

```yaml
# prometheus-podmonitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: PodMonitor
metadata:
  name: envoy-gateway-proxy
  namespace: envoy-gateway-system
spec:
  selector:
    matchLabels:
      app.kubernetes.io/component: proxy
      app.kubernetes.io/managed-by: envoy-gateway
  podMetricsEndpoints:
  - port: metrics
    path: /stats/prometheus
    interval: 15s
    relabelings:
    - sourceLabels: [__meta_kubernetes_pod_name]
      targetLabel: pod
    - sourceLabels: [__meta_kubernetes_namespace]
      targetLabel: namespace
```

### 关键 Envoy 指标

| 指标类别 | 指标名称示例 | 含义 |
|---------|------------|------|
| 请求统计 | `envoy_http_downstream_rq_total` | 下游总请求数 |
| 延迟 | `envoy_http_downstream_rq_time_bucket` | 请求处理延迟分布 |
| 错误率 | `envoy_http_downstream_rq_5xx` | 5xx 错误数 |
| 连接数 | `envoy_listener_downstream_cx_active` | 活跃连接数 |
| 后端健康 | `envoy_cluster_upstream_cx_active` | 上游活跃连接 |
| 限流 | `envoy_http_local_rate_limit_rate_limited` | 限流拦截请求数 |

### 访问日志配置

```yaml
# access-log-policy.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: EnvoyProxy
metadata:
  name: logging-config
  namespace: envoy-gateway-system
spec:
  telemetry:
    accessLog:
      settings:
      - format:
          type: JSON
          json:
            start_time: "%START_TIME%"
            method: "%REQ(:METHOD)%"
            path: "%REQ(X-ENVOY-ORIGINAL-PATH?:PATH)%"
            protocol: "%PROTOCOL%"
            response_code: "%RESPONSE_CODE%"
            duration: "%DURATION%"
            upstream_host: "%UPSTREAM_HOST%"
            x_forwarded_for: "%REQ(X-FORWARDED-FOR)%"
            user_agent: "%REQ(USER-AGENT)%"
            request_id: "%REQ(X-REQUEST-ID)%"
        sinks:
        - type: File
          file:
            path: /dev/stdout
        - type: OpenTelemetry
          openTelemetry:
            host: otel-collector.monitoring.svc.cluster.local
            port: 4317
```

### 分布式追踪

```yaml
# tracing-config.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: EnvoyProxy
metadata:
  name: tracing-config
  namespace: envoy-gateway-system
spec:
  telemetry:
    tracing:
      samplingRate: 10.0    # 采样率 10%
      provider:
        host: jaeger-collector.monitoring.svc.cluster.local
        port: 4317
        type: OpenTelemetry
      customTags:
        environment:
          type: Literal
          literal:
            value: "production"
        pod_name:
          type: Environment
          environment:
            name: POD_NAME
            defaultValue: "unknown"
```

### Grafana 仪表盘推荐

```bash
# 导入 Envoy Gateway 官方 Grafana Dashboard
# Dashboard ID: 20162（Envoy Gateway）

# 安装 kube-prometheus-stack（推荐）
helm install prometheus prometheus-community/kube-prometheus-stack \
  -n monitoring --create-namespace \
  -f https://raw.githubusercontent.com/envoyproxy/gateway/main/charts/gateway-addons-helm/values.yaml
```

---

## 9. 生产部署建议

### 高可用架构

```
                    ┌──────────────────────────────────────┐
                    │          生产 HA 部署架构               │
                    └──────────────────────────────────────┘

  Zone A                Zone B                Zone C
  ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
  │ Envoy Proxy  │      │ Envoy Proxy  │      │ Envoy Proxy  │
  │ (Pod ×2)    │      │ (Pod ×2)    │      │ (Pod ×2)    │
  └──────┬───────┘      └──────┬───────┘      └──────┬───────┘
         │                     │                     │
         └─────────────────────┼─────────────────────┘
                               │
                    ┌──────────▼───────────┐
                    │   LoadBalancer (NLB)  │
                    └──────────────────────┘

  envoy-gateway Controller: 单活（Leader Election via K8s lease）
```

### 资源规划

| 组件 | 请求（Request） | 限制（Limit） | 副本数 |
|------|---------------|-------------|-------|
| envoy-gateway Controller | 100m CPU / 256Mi | 500m CPU / 512Mi | 1（主备） |
| envoy-proxy（小型） | 100m CPU / 128Mi | 1 CPU / 512Mi | 2+ |
| envoy-proxy（中型） | 500m CPU / 512Mi | 2 CPU / 2Gi | 3+ |
| envoy-proxy（大型） | 1 CPU / 1Gi | 4 CPU / 4Gi | 5+ |
| rate-limit service | 100m CPU / 128Mi | 500m CPU / 512Mi | 2 |

### HPA 自动扩缩配置

```yaml
# hpa-envoy-proxy.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: envoy-proxy-hpa
  namespace: default
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: envoy-default-eg-proxy
  minReplicas: 3
  maxReplicas: 20
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
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 25
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 30
      policies:
      - type: Percent
        value: 100
        periodSeconds: 30
```

### PodDisruptionBudget

```yaml
# pdb-envoy-proxy.yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: envoy-proxy-pdb
  namespace: default
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app.kubernetes.io/component: proxy
      gateway.envoyproxy.io/owning-gateway-name: eg
```

### 生产检查清单

```bash
# ✅ 验证 GatewayClass 状态
kubectl get gatewayclass eg -o jsonpath='{.status.conditions[?(@.type=="Accepted")].status}'

# ✅ 验证 Gateway 已分配 IP
kubectl get gateway -A -o wide

# ✅ 检查 HTTPRoute 绑定状态
kubectl get httproute -A -o wide

# ✅ 查看 EG 控制器日志
kubectl logs -n envoy-gateway-system -l app.kubernetes.io/name=gateway -f

# ✅ 查看 Envoy Proxy 访问日志
kubectl logs -n default -l gateway.envoyproxy.io/owning-gateway-name=eg -f

# ✅ 检查 xDS 同步状态
kubectl port-forward -n envoy-gateway-system pod/eg-proxy-xxx 19000:19000
curl http://localhost:19000/ready
```

---

## 10. 与其他网关对比

| 特性 | Envoy Gateway | Higress | APISIX | Kong |
|------|-------------|---------|--------|------|
| **控制面语言** | Go | Go + Java | Lua + Go | Lua + Go |
| **数据面** | Envoy | Envoy | Nginx/OpenResty | Nginx/OpenResty |
| **Gateway API 支持** | ⭐⭐⭐⭐⭐ 原生 | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| **私有 CRD** | 仅扩展策略 | 少量 | 大量 | 大量 |
| **插件生态** | Wasm / EnvoyFilter | Wasm / Lua | Lua / Go | Lua / Go |
| **配置存储** | Kubernetes etcd | Kubernetes etcd | etcd / DB-less | PostgreSQL / DB-less |
| **AI 网关能力** | 基础 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| **社区活跃度** | 高（CNCF 官方） | 高（Alibaba） | 高（Apache） | 高（Kong Inc.） |
| **学习曲线** | 中（需懂 xDS） | 低 | 中 | 中 |

---

## 参考资料

- [Envoy Gateway 官方文档](https://gateway.envoyproxy.io/docs/)
- [Gateway API 规范文档](https://gateway-api.sigs.k8s.io/)
- [Envoy Proxy 官方文档](https://www.envoyproxy.io/docs/)
- [CNCF Envoy Gateway 项目页](https://www.cncf.io/projects/envoy-gateway/)
- [EnvoyPatchPolicy API 参考](https://gateway.envoyproxy.io/docs/api/extension_types/#envoypatchpolicy)
- 本域相关文档：
  - [01 - API 网关架构总览](./01-api-gateway-architecture-overview.md)
  - [02 - Kubernetes Gateway API 深度解析](./02-kubernetes-gateway-api-deep-dive.md)
  - [03 - API 网关选型指南](./03-api-gateway-selection-guide.md)
  - [domain-5: Nginx Ingress 完整指南](../domain-5-networking/21-nginx-ingress-complete-guide.html)
  - [domain-30: 服务网格 Istio（xDS 对比参考）](../domain-30-service-mesh/)
