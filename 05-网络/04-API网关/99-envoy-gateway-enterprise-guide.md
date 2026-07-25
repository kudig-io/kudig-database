---
title: Envoy Gateway 企业级 API Gateway 实践指南
description: '# Envoy Gateway 企业级 API Gateway 实践指南'
summary: 'Ingress (单一资源)              Gateway (基础设施定义)'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- prometheus
- istio
- cilium
- helm
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
- Envoy Gateway 企业级 API Gateway 实践指南 是什么
- 如何 Envoy Gateway 企业级 API Gateway 实践指南
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- Envoy
- Gateway
- 企业级
- API
- Gateway
- 实践指南
- cloud
- native
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- cilium-basics
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




# [[Envoy|Envoy]] Gateway 企业级 API Gateway 实践指南

> **适用版本**: Envoy Gateway v1.3 / Gateway API v1.2  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [一、架构演进](#一架构演进)
- [二、安装部署](#二安装部署)
- [三、Gateway 与 Listener 配置](#三gateway-与-listener-配置)
- [四、HTTPRoute 流量路由](#四httproute-流量路由)
- [五、TLS 终止与 mTLS](#五tls-终止与-mtls)
- [六、速率限制与熔断](#六速率限制与熔断)
- [七、认证与授权](#七认证与授权)
- [八、可观测性集成](#八可观测性集成)
- [九、Envoy Gateway vs 传统 [[Ingress|Ingress]]](#九envoy-gateway-vs-传统-ingress)

---

<!-- chunk: 一、架构演进 -->## 一、架构演进

```
Ingress 时代 (v1)          →    Gateway API 时代 (v2)
                                   
Ingress (单一资源)              Gateway (基础设施定义)
  ├─ 规则混杂                    ├─ GatewayClass (实现选择)
  ├─ 注解驱动                    ├─ Gateway (监听器配置)
  ├─ 实现锁定                    ├─ HTTPRoute (应用路由)
  └─ 无法共享                    ├─ TLSRoute/GRPCRoute/TCPRoute
                                 └─ 角色分离 (平台 vs 应用)

Envoy Gateway 定位
├── 基于 Envoy Proxy (CNCF 毕业项目)
├── 原生 Gateway API 实现
├── 轻量级控制平面
├── 不依赖 Istio 控制平面
└── 适合: 纯网关场景 (不需要服务网格)
```

## Gateway API 角色模型

| 角色 | 管理资源 | 关注点 |
|:---|:---|:---|
| 基础设施管理员 | GatewayClass, Gateway | 端口、协议、TLS、IP |
| 集群运维 | Gateway (共享) | 多租户、策略、监控 |
| 应用开发者 | HTTPRoute, [[Service|Service]] | 路由规则、流量分配 |

---

<!-- chunk: 二、安装部署 -->## 二、安装部署

## 2.1 Helm 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm repo add eg https://charts.envoyproxy.io
helm repo update

helm install eg eg/gateway-helm \
  --namespace envoy-gateway-system \
  --create-namespace \
  --version v1.3.0
```
## 2.2 生产级配置

```yaml
# values-envoy-gateway.yaml
deployment:
  replicas: 2
  resources:
    requests:
      cpu: 500m
      memory: 256Mi
    limits:
      cpu: 2000m
      memory: 1Gi

service:
  type: LoadBalancer
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-scheme: "internet-facing"

envoyGateway:
  gateway:
    controllerName: gateway.envoyproxy.io/gatewayclass-controller
  
  logging:
    level:
      default: info
      gateway: info

# Prometheus 监控
observability:
  metrics:
    prometheus:
      enable: true
      scrapeInterval: 30s
```

## 2.3 GatewayClass 创建

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: envoy-gw
spec:
  controllerName: gateway.envoyproxy.io/gatewayclass-controller
  parametersRef:
    group: gateway.envoyproxy.io
    kind: EnvoyProxy
    name: custom-proxy-config
    namespace: envoy-gateway-system
```

---

<!-- chunk: 三、Gateway 与 Listener 配置 -->## 三、Gateway 与 Listener 配置

## 3.1 基础 Gateway

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: public-gateway
  namespace: ingress
spec:
  gatewayClassName: envoy-gw
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
    hostname: "*.example.com"
    tls:
      mode: Terminate
      certificateRefs:
      - kind: Secret
        name: wildcard-example-com
        namespace: ingress
    allowedRoutes:
      namespaces:
        from: All
  
  # 管理面 (仅内部)
  - name: https-internal
    protocol: HTTPS
    port: 443
    hostname: "admin.example.com"
    tls:
      mode: Terminate
      certificateRefs:
      - kind: Secret
        name: admin-example-com
    allowedRoutes:
      namespaces:
        from: Selector
        selector:
          matchLabels:
            gateway-access: admin
```

## 3.2 EnvoyProxy 自定义配置

```yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: EnvoyProxy
metadata:
  name: custom-proxy-config
  namespace: envoy-gateway-system
spec:
  provider:
    type: Kubernetes
    kubernetes:
      envoyService:
        type: LoadBalancer
      envoyHpa:
        minReplicas: 2
        maxReplicas: 10
        metrics:
        - resource:
            name: cpu
            target:
              averageUtilization: 70
              type: Utilization
          type: Resource
```

---

<!-- chunk: 四、HTTPRoute 流量路由 -->## 四、HTTPRoute 流量路由

## 4.1 基础路由

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: app-routes
  namespace: production
spec:
  parentRefs:
  - name: public-gateway
    namespace: ingress
    sectionName: https
  hostnames:
    - api.example.com
  rules:
  # 路径路由
  - matches:
    - path:
        type: PathPrefix
        value: /v1/users
    backendRefs:
    - name: user-service
      port: 8080
      weight: 100
  
  # 头部分流 (A/B 测试)
  - matches:
    - path:
        type: PathPrefix
        value: /v1/orders
      headers:
      - name: x-canary
        value: "true"
    backendRefs:
    - name: order-service-canary
      port: 8080
      weight: 100
  
  # 默认路由 (蓝绿发布)
  - matches:
    - path:
        type: PathPrefix
        value: /v1/orders
    backendRefs:
    - name: order-service-stable
      port: 8080
      weight: 90
    - name: order-service-canary
      port: 8080
      weight: 10
  
  # 重定向
  - matches:
    - path:
        type: Exact
        value: /old-endpoint
    filters:
    - type: URLRedirect
      urlRedirect:
        path: /v2/new-endpoint
        statusCode: 301
```

---

<!-- chunk: 五、TLS 终止与 mTLS -->## 五、TLS 终止与 mTLS

## 5.1 TLS 终止

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: tls-gateway
spec:
  gatewayClassName: envoy-gw
  listeners:
  - name: https
    protocol: HTTPS
    port: 443
    tls:
      mode: Terminate
      certificateRefs:
      - kind: Secret
        name: example-com-cert
```

## 5.2 上游 mTLS

```yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: SecurityPolicy
metadata:
  name: upstream-mtls
  namespace: production
spec:
  targetRefs:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: app-routes
  mtls:
    clientCertificateRef:
      kind: Secret
      name: client-cert
```

---

<!-- chunk: 六、速率限制与熔断 -->## 六、速率限制与熔断

## 6.1 本地速率限制

```yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: SecurityPolicy
metadata:
  name: rate-limit-policy
  namespace: production
spec:
  targetRefs:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: api-routes
  rateLimit:
    type: Local
    local:
      rules:
      - limit:
          requests: 100
          unit: Second
        clientSelectors:
        - sourceIP:
            value: 0.0.0.0/0
      - limit:
          requests: 10
          unit: Second
        clientSelectors:
        - path:
            value: /api/login
            type: Exact
```

## 6.2 重试与超时

```yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: BackendTrafficPolicy
metadata:
  name: retry-policy
  namespace: production
spec:
  targetRefs:
  - group: ""
    kind: Service
    name: user-service
  retry:
    numRetries: 3
    perRetry:
      timeout: 5s
      backOff:
        baseInterval: 100ms
        maxInterval: 10s
    retryOn:
      gatewayError: true
      connectFailure: true
      retriableStatusCodes: [503, 504]
  timeout:
    request: 30s
```

---

<!-- chunk: 七、认证与授权 -->## 七、认证与授权

## 7.1 JWT 验证

```yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: SecurityPolicy
metadata:
  name: jwt-auth
  namespace: production
spec:
  targetRefs:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: protected-api
  jwt:
    providers:
    - name: keycloak
      remoteJWKS:
        uri: https://auth.example.com/realms/production/protocol/openid-connect/certs
        fetchTimeoutSeconds: 5
      claimToHeaders:
      - header: x-user-id
        claim: sub
      - header: x-user-email
        claim: email
```

## 7.2 基础认证

```yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: SecurityPolicy
metadata:
  name: basic-auth
  namespace: production
spec:
  targetRefs:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: admin-api
  basicAuth:
    users:
      name: basic-auth-users
```

---

<!-- chunk: 八、可观测性集成 -->## 八、可观测性集成

## 8.1 Access Log 配置

```yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: EnvoyProxy
metadata:
  name: logging-config
spec:
  telemetry:
    accessLog:
      settings:
      - format:
          type: JSON
          json:
            time: "%START_TIME%"
            method: "%REQ(:METHOD)%"
            path: "%REQ(X-ENVOY-ORIGINAL-PATH?:PATH)%"
            protocol: "%PROTOCOL%"
            response_code: "%RESPONSE_CODE%"
            duration: "%DURATION%"
            upstream_service_time: "%RESP(X-ENVOY-UPSTREAM-SERVICE-TIME)%"
            forwarded_for: "%REQ(X-FORWARDED-FOR)%"
            user_agent: "%REQ(USER-AGENT)%"
            request_id: "%REQ(X-REQUEST-ID)%"
            authority: "%REQ(:AUTHORITY)%"
            upstream_host: "%UPSTREAM_HOST%"
        destinations:
        - type: File
          file:
            path: /dev/stdout
```

## 8.2 Prometheus Metrics

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: envoy-gateway-metrics
  namespace: monitoring
spec:
  namespaceSelector:
    matchNames:
      - envoy-gateway-system
  selector:
    matchLabels:
      control-plane: envoy-gateway
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
```

| 关键指标 | 含义 |
|:---|:---|
| envoy_cluster_upstream_rq_total | 上游请求总数 |
| envoy_cluster_upstream_rq_time_bucket | 请求延迟分布 |
| envoy_http_downstream_rq_total | 下游请求总数 |

---

<!-- chunk: 九、Envoy Gateway vs 传统 Ingress -->## 九、Envoy Gateway vs 传统 Ingress

| 特性 | Ingress (NGINX) | Envoy Gateway |
|:---|:---|:---|
| 标准 | 实现碎片化 | Gateway API (统一标准) |
| 角色分离 | 无 | Gateway/Route 分离 |
| 多租户 | 注解隔离 | 原生 Namespace 隔离 |
| TLS | 注解配置 | 原生 TLS 配置 |
| 路由 | 基于路径 | 路径/头/方法/权重 |
| 流量分割 | 有限 | 原生权重路由 |
| 协议 | HTTP/1.1, HTTP/2 | HTTP/1.1, HTTP/2, HTTP/3, gRPC |
| 可扩展性 | Lua/NJS | WASM, ExtProc |
| 学习曲线 | 低 | 中等 |

## 迁移路径

```
现有 Ingress
    |
    ▼
评估 Gateway API 实现
    |
    ├── 选择 Envoy Gateway / Istio / Cilium / NGINX Gateway Fabric
    |
    ▼
并行部署 Gateway
    |
    ├── 新应用使用 Gateway API
    ├── 存量应用逐步迁移
    |
    ▼
废弃 Ingress
    └── 完全切换至 Gateway API
```

---

<!-- chunk: 参考链接 -->## 参考链接

- [Envoy Gateway 文档](https://gateway.envoyproxy.io/)
- [Gateway API 官方](https://gateway-api.sigs.k8s.io/)
- [Envoy Proxy 文档](https://www.envoyproxy.io/docs/)
- [Envoy Gateway GitHub](https://github.com/envoyproxy/gateway)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[05-网络/README.md|Domain 03: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
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

- 13-api-gateway-performance-benchmarks
- 14-api-gateway-production-operations
- 01-api-gateway-architecture-overview
- 02-kubernetes-gateway-api-deep-dive

```

<!-- risk-assessed -->
