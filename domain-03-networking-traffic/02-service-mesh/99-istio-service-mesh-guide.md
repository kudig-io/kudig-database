---
title: Istio 企业级服务网格入门指南
description: '# Istio 企业级服务网格入门指南'
summary: '# Istio 企业级服务网格入门指南'
category: service-mesh-microservices
tags:
- k8s
- service-mesh
- istio
- envoy
- microservices
- prometheus
- grafana
- jaeger
- helm
- hpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 开发工程师
estimated_read_time: 5min
intent_queries:
- Istio 企业级服务网格入门指南 是什么
- 如何 Istio 企业级服务网格入门指南
- Kubernetes 26 service mesh microservices 最佳实践
trigger_keywords:
- Istio
- 企业级服务网格入门指南
- service
- mesh
- microservices
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- tls-basics
- logging-basics
- tracing-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-05-security-compliance/
  label: '相关知识域: domain-05-security-compliance'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/service-fta.md
  label: '故障树: service'
---



# [[Istio|Istio]] 企业级服务网格入门指南

> **适用版本**: Istio v1.29.0
> **最后更新**: 2026-04-24
> **难度**: 中级 → 高级

---

<!-- chunk: 概述 -->## 概述

Istio 是功能最全面的开源服务网格平台，提供流量管理、安全通信、可观测性和策略执行等四大核心能力。作为 CNCF 毕业项目（2023年），Istio 已被全球数以千计的企业用于生产环境，是服务网格领域的事实标准。本指南从零开始，覆盖 Istio 的两种架构模式（Sidecar 和 Ambient）、安装部署、流量管理、安全加固、可观测性配置、多集群部署、性能调优和故障排查。所有配置均基于 Istio v1.29，可直接用于生产环境。

Istio 的核心设计理念是将分布式系统中的通信关注点从应用代码中分离出来，通过在基础设施层提供透明代理来实现服务间通信的统一治理。这使得开发者无需在业务代码中嵌入服务发现、负载均衡、重试、超时、加密等逻辑，而是通过声明式的 YAML 配置来管理这些横切关注点。2026年 Istio 的重大发展包括 Ambient Mesh 模式的 GA 发布、Gateway API 的全面支持，以及持续的性能优化和可观测性增强。

## 架构图

```mermaid
graph TB
    subgraph "Sidecar 模式 (成熟稳定)"
        POD_S[Pod] --> APP_S[App Container]
        POD_S --> ENV_S[Envoy Sidecar<br/>Ingress:15006 / Egress:15001]
        POD_S --> INIT_S[istio-init<br/>iptables 流量拦截]
    end

    subgraph "Ambient 模式 (2026 GA)"
        POD_A[Pod 无Sidecar] --> APP_A[App Container<br/>无侵入]
        NODE_A[Node] --> ZT_A[ztunnel DaemonSet<br/>L4 mTLS/路由<br/>Rust ~50MB/节点]
        NS_A[Namespace] --> WP_A[Waypoint Proxy<br/>L7 策略/可观测性<br/>Envoy 按需部署]
    end

    subgraph "控制平面 (istiod)"
        PILOT[Pilot<br/>服务发现 + xDS配置下发]
        CITADEL[Citadel<br/>证书签发 + 轮换]
        GALLEY[Galley (内置)<br/>配置验证 + 转换]
    end

    PILOT --> ENV_S
    PILOT --> ZT_A
    PILOT --> WP_A
    CITADEL --> ENV_S
    CITADEL --> ZT_A
```

---

<!-- chunk: 一、架构模式选择 -->## 一、架构模式选择

## 1.1 Sidecar 模式 (传统成熟)

```
Pod
├── App Container
├── istio-proxy (Envoy Sidecar)
│   ├── Ingress Listener (15006) — 拦截入站流量
│   ├── Egress Listener (15001) — 拦截出站流量
│   └── Prometheus Listener (15090) — 指标导出
└── init container (istio-init): iptables 规则设置
```

**优点**: 功能完整（L3-L7）、社区案例丰富、WASM 扩展支持、成熟稳定
**缺点**: 额外资源开销 (~100MB/Pod)、Pod 启动延迟 (+3-8s)、运维复杂度高

## 1.2 Ambient Mesh (无 Sidecar — 2026 GA)

```
Node
├── ztunnel (DaemonSet): L4 处理、mTLS、HBONE隧道 (Rust, ~50MB/节点)
└── waypoint proxy (按需Deployment): L7 策略、可观测性、流量管理 (Envoy)

Pod (无 Sidecar — 零侵入)
└── App Container (原生网络，通过iptables重定向到节点级ztunnel)
```

**优点**: 更低资源占用 (~85%节省)、更快启动、更简单运维、零侵入
**状态**: v1.29 GA，生产可用

## 1.3 选型建议

| 场景 | 推荐模式 | 原因 |
|:---|:---|:---|
| 全新部署 | Ambient Mesh | 更低资源、零侵入 |
| 资源极度敏感 | Ambient Mesh | 节点级共享代理 |
| 已大量 Sidecar 投入 | 渐进迁移至 Ambient | 平滑迁移路径 |
| 需要完整 L7 功能 | Sidecar 或 Ambient + Waypoint | 两种都支持 |
| 遗留系统兼容性 | Sidecar (更成熟) | 更多社区案例 |
| VM + K8s 混合 | Sidecar (VM 支持) | Ambient 暂不支持VM |
| 需要 WASM 扩展 | Sidecar 或 Waypoint | 需要L7代理 |
| 大规模集群 (>1000 Pod) | Ambient Mesh | 资源节省最显著 |

---

<!-- chunk: 二、安装部署 -->## 二、安装部署

## 2.1 istioctl 安装 (推荐)

```bash
# 下载安装 istioctl
curl -L https://istio.io/downloadIstio | sh -
cd istio-1.29.0
export PATH=$PWD/bin:$PATH

# 查看可用的安装配置
istioctl profile list
# default     - 推荐生产环境
# demo        - 学习和演示
# minimal     - 最小化安装
# ambient     - Ambient Mesh模式
# empty       - 空配置
# preview     - 预览特性
# external    - 外部控制平面

# Sidecar 模式 (生产)
istioctl install --set profile=default \
  --set values.global.proxy.holdApplicationUntilProxyStarts=true \
  --set values.meshConfig.defaultConfig.holdApplicationUntilProxyStarts=true \
  --set values.meshConfig.accessLogFile=/dev/stdout \
  --set values.meshConfig.accessLogEncoding=JSON \
  --set values.meshConfig.outboundTrafficPolicy.mode=REGISTRY_ONLY \
  --set values.meshConfig.defaultConfig.tracing.zipkin.address=zipkin.istio-system:9411 \
  --set values.meshConfig.defaultConfig.tracing.sampling=10.0 \
  -y

# Ambient 模式 (生产)
istioctl install --set profile=ambient \
  --set values.global.proxy.holdApplicationUntilProxyStarts=true \
  --set values.meshConfig.accessLogFile=/dev/stdout \
  --set values.meshConfig.accessLogEncoding=JSON \
  -y

# 验证安装
istioctl verify-install
# ✔ Istio core installed
# ✔ Istiod installed
# ✔ Ingress gateways installed
# ✔ Installation complete
```

## 2.2 Helm 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 添加 Helm 仓库
helm repo add istio https://istio-release.storage.googleapis.com/charts
helm repo update

# 创建命名空间
kubectl create namespace istio-system

# 安装 Istio base CRD
helm install istio-base istio/base -n istio-system --wait

# 安装 istiod 控制平面
helm install istiod istio/istiod -n istio-system --wait \
  --set global.proxy.holdApplicationUntilProxyStarts=true \
  --set meshConfig.accessLogFile=/dev/stdout \
  --set meshConfig.accessLogEncoding=JSON

# 安装 Ingress Gateway
helm install istio-ingressgateway istio/gateway -n istio-system --wait \
  --set service.type=LoadBalancer \
  --set service.annotations."service\.beta\.kubernetes\.io/aws-load-balancer-type"=nlb

# 验证
helm list -n istio-system
kubectl get pods -n istio-system -o wide
```

## 2.3 命名空间注入

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# Sidecar 自动注入
kubectl label namespace default istio-injection=enabled
kubectl label namespace production istio-injection=enabled

# Ambient 模式加入数据平面
kubectl label namespace default istio.io/dataplane-mode=ambient
kubectl label namespace production istio.io/dataplane-mode=ambient

# 验证命名空间标签
kubectl get ns -L istio-injection -L istio.io/dataplane-mode
# NAME              STATUS   AGE   ISTIO-INJECTION   DATAPOINT-MODE
# default           Active   30d   enabled
# production        Active   30d                     ambient
# istio-system      Active   30d

# 部署 Bookinfo 示例应用
kubectl apply -f samples/bookinfo/platform/kube/bookinfo.yaml
kubectl apply -f samples/bookinfo/networking/bookinfo-gateway.yaml

# 验证部署
kubectl get pods -o wide
kubectl get svc
```

---

<!-- chunk: 三、流量管理 -->## 三、流量管理

## 3.1 VirtualService 与 DestinationRule

```yaml
apiVersion: networking.istio.io/v1
kind: VirtualService
metadata:
  name: reviews-route
  namespace: default
spec:
  hosts:
  - reviews
  http:
  - name: user-specific-routing
    match:
    - headers:
        end-user:
          exact: jason
    route:
    - destination:
        host: reviews
        subset: v2
    timeout: 5s
  - name: canary-routing
    route:
    - destination:
        host: reviews
        subset: v1
      weight: 75
    - destination:
        host: reviews
        subset: v2
      weight: 25
    retries:
      attempts: 3
      perTryTimeout: 2s
      retryOn: 5xx,reset,connect-failure,refused-stream
    timeout: 10s
    fault:
      delay:
        percentage:
          value: 0.1
        fixedDelay: 5s
---
apiVersion: networking.istio.io/v1
kind: DestinationRule
metadata:
  name: reviews-destination
  namespace: default
spec:
  host: reviews
  trafficPolicy:
    connectionPool:
      tcp:
        maxConnections: 100
        connectTimeout: 5s
        tcpKeepalive:
          time: 7200s
          interval: 75s
      http:
        http1MaxPendingRequests: 50
        http2MaxRequests: 100
        maxRequestsPerConnection: 10
        maxRetries: 3
        idleTimeout: 60s
    outlierDetection:
      consecutive5xxErrors: 5
      interval: 30s
      baseEjectionTime: 60s
      maxEjectionPercent: 50
      minHealthPercent: 25
    loadBalancer:
      simple: LEAST_CONN
      localityLbSetting:
        enabled: true
        failover:
          - from: us-west-2
            to: us-east-1
  subsets:
  - name: v1
    labels:
      version: v1
    trafficPolicy:
      loadBalancer:
        simple: ROUND_ROBIN
  - name: v2
    labels:
      version: v2
    trafficPolicy:
      loadBalancer:
        simple: LEAST_REQUEST
```

## 3.2 Gateway API (推荐新标准)

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: istio-gateway-class
spec:
  controllerName: istio.io/gateway-controller
---
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: external-gateway
  namespace: istio-system
  annotations:
    cert-manager.io/issuer: letsencrypt-prod
spec:
  gatewayClassName: istio-gateway-class
  listeners:
  - name: https
    protocol: HTTPS
    port: 443
    tls:
      mode: Terminate
      certificateRefs:
      - name: example-cert
    allowedRoutes:
      namespaces:
        from: All
  - name: http
    protocol: HTTP
    port: 80
    allowedRoutes:
      namespaces:
        from: All
---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: frontend-route
  namespace: default
spec:
  parentRefs:
  - name: external-gateway
    namespace: istio-system
  hostnames:
  - "bookinfo.example.com"
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /productpage
    backendRefs:
    - name: productpage
      port: 9080
  - matches:
    - path:
        type: PathPrefix
        value: /api
    backendRefs:
    - name: productpage
      port: 9080
      weight: 90
    - name: productpage-canary
      port: 9080
      weight: 10
```

## 3.3 流量镜像

```yaml
apiVersion: networking.istio.io/v1
kind: VirtualService
metadata:
  name: reviews-mirror
  namespace: default
spec:
  hosts:
  - reviews
  http:
  - route:
    - destination:
        host: reviews
        subset: v1
      weight: 100
    mirror:
      host: reviews
      subset: v2
    mirrorPercentage:
      value: 10
```

## 3.4 故障注入

```yaml
apiVersion: networking.istio.io/v1
kind: VirtualService
metadata:
  name: ratings-fault
  namespace: default
spec:
  hosts:
  - ratings
  http:
  - fault:
      delay:
        percentage:
          value: 10
        fixedDelay: 5s
      abort:
        percentage:
          value: 5
        httpStatus: 500
    route:
    - destination:
        host: ratings
        subset: v1
```

---

<!-- chunk: 四、安全加固 -->## 四、安全加固

## 4.1 全局 mTLS (STRICT 模式)

```yaml
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT
---
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: legacy-allow
  namespace: legacy
spec:
  mtls:
    mode: PERMISSIVE
```

## 4.2 授权策略 (AuthorizationPolicy)

```yaml
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: deny-all
  namespace: production
spec:
  action: DENY
---
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: frontend-policy
  namespace: production
spec:
  selector:
    matchLabels:
      app: frontend
  action: ALLOW
  rules:
  - from:
    - source:
        namespaces: ["istio-system"]
        principals: ["cluster.local/ns/istio-system/sa/istio-ingressgateway-service-account"]
    to:
    - operation:
        methods: ["GET", "POST"]
        paths: ["/api/*"]
  - from:
    - source:
        principals: ["cluster.local/ns/production/sa/backend"]
    to:
    - operation:
        methods: ["GET"]
        paths: ["/health", "/ready"]
  - from:
    - source:
        principals: ["cluster.local/ns/production/sa/admin-service"]
    to:
    - operation:
        methods: ["GET", "POST", "PUT", "DELETE"]
        paths: ["/admin/*"]
    when:
    - key: request.headers[x-user-role]
      values: ["admin"]
---
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: internal-only
  namespace: production
spec:
  selector:
    matchLabels:
      app: backend
  action: ALLOW
  rules:
  - from:
    - source:
        namespaces: ["production"]
        principals: ["cluster.local/ns/production/sa/*"]
```

## 4.3 JWT 认证

```yaml
apiVersion: security.istio.io/v1beta1
kind: RequestAuthentication
metadata:
  name: jwt-auth
  namespace: production
spec:
  selector:
    matchLabels:
      app: frontend
  jwtRules:
  - issuer: "https://accounts.google.com"
    jwksUri: "https://www.googleapis.com/oauth2/v3/certs"
    audiences: ["my-client-id"]
    forwardOriginalToken: true
  - issuer: "https://auth.company.com"
    jwksUri: "https://auth.company.com/.well-known/jwks.json"
    audiences: ["api.company.com"]
    forwardOriginalToken: true
---
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: require-jwt
  namespace: production
spec:
  selector:
    matchLabels:
      app: frontend
  action: ALLOW
  rules:
  - from:
    - source:
        requestPrincipals: ["*"]
  - from:
    - source:
        namespaces: ["istio-system"]
```

---

<!-- chunk: 五、可观测性配置 -->## 五、可观测性配置

## 5.1 Telemetry API

```yaml
apiVersion: telemetry.istio.io/v1alpha1
kind: Telemetry
metadata:
  name: default-metrics
  namespace: istio-system
spec:
  metrics:
  - providers:
    - name: prometheus
    overrides:
    - matchers:
      - metric="ALL_METRICS"
      - tagOverrides=""
      - destination_cluster=""
      - value="cluster-1"
      - request_method=""
      - value="request.method"
      - request_host=""
      - value="request.host"
  accessLogging:
  - providers:
    - name: envoy
    filter:
      expression: "response.code >= 400"
  tracing:
  - providers:
    - name: otel-collector
    randomSamplingPercentage: 10.0
    customTags:
      user_id:
        header:
          name: x-user-id
          defaultValue: "unknown"
      environment:
        literal:
          value: "production"
```

## 5.2 Kiali 可视化

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 安装可观测性组件
kubectl apply -f https://raw.githubusercontent.com/istio/istio/release-1.29/samples/addons/kiali.yaml
kubectl apply -f https://raw.githubusercontent.com/istio/istio/release-1.29/samples/addons/prometheus.yaml
kubectl apply -f https://raw.githubusercontent.com/istio/istio/release-1.29/samples/addons/jaeger.yaml
kubectl apply -f https://raw.githubusercontent.com/istio/istio/release-1.29/samples/addons/grafana.yaml
kubectl apply -f https://raw.githubusercontent.com/istio/istio/release-1.29/samples/addons/loki.yaml

# 启动 Kiali Dashboard
istioctl dashboard kiali

# 启动 Grafana Dashboard
istioctl dashboard grafana

# 启动 Jaeger Dashboard
istioctl dashboard jaeger
```

## 5.3 关键 PromQL 查询

```promql
# 服务错误率
rate(istio_requests_total{response_code=~"5.."}[1m]) / rate(istio_requests_total[1m])

# P99 延迟
histogram_quantile(0.99, sum(rate(istio_request_duration_milliseconds_bucket[1m])) by (le, destination_service))

# 服务请求速率
sum(rate(istio_requests_total[1m])) by (destination_service)

# mTLS 流量比例
sum(rate(istio_requests_total{connection_security_policy="mutual_tls"}[1m])) /
sum(rate(istio_requests_total[1m]))

# TCP 连接数
sum(istio_tcp_connections_opened_total) by (destination_service)

# 证书过期时间
istio_cert_expiry_timestamp - time()
```

## 5.4 Prometheus 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: istio-alerts
  namespace: istio-system
spec:
  groups:
  - name: istio.rules
    rules:
    - alert: IstioHighErrorRate
      expr: |
        sum(rate(istio_requests_total{response_code=~"5.."}[1m])) by (destination_service) /
        sum(rate(istio_requests_total[1m])) by (destination_service) > 0.05
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "Error rate above 5% for {{ $labels.destination_service }}"

    - alert: IstioHighLatency
      expr: |
        histogram_quantile(0.99, sum(rate(istio_request_duration_milliseconds_bucket[1m])) by (le, destination_service)) > 1000
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "P99 latency above 1s for {{ $labels.destination_service }}"

    - alert: IstioComponentDown
      expr: up{job=~"istiod|istio-proxy"} == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Istio component {{ $labels.instance }} is down"

    - alert: IstioCertificateExpiringSoon
      expr: istio_cert_expiry_timestamp - time() < 86400 * 7
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "Certificate expiring in less than 7 days"

    - alert: IstioHighConnectionRate
      expr: rate(istio_tcp_connections_opened_total[1m]) > 100
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High TCP connection rate for {{ $labels.destination_service }}"
```

---

<!-- chunk: 六、多集群部署 -->## 六、多集群部署

## 6.1 单网络多集群 (Flat Network)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# Cluster 1
istioctl install --set profile=default \
  --set values.global.multiCluster.clusterName=cluster1 \
  --set values.global.network=network1 -y

# Cluster 2
istioctl install --set profile=default \
  --set values.global.multiCluster.clusterName=cluster2 \
  --set values.global.network=network1 -y

# 交叉注册集群 Secret
istioctl create-remote-secret \
  --context=cluster2 \
  --name=cluster2 | \
  kubectl apply -f - --context=cluster1

istioctl create-remote-secret \
  --context=cluster1 \
  --name=cluster1 | \
  kubectl apply -f - --context=cluster2

# 验证多集群
istioctl proxy-status
```

## 6.2 多网络多集群 (Gateway 互连)

```yaml
apiVersion: networking.istio.io/v1beta1
kind: Gateway
metadata:
  name: cross-network-gateway
  namespace: istio-system
spec:
  selector:
    istio: eastwestgateway
  servers:
  - port:
      number: 443
      name: tls
      protocol: TLS
    tls:
      mode: AUTO_PASSTHROUGH
    hosts:
    - "*.local"
```

---

<!-- chunk: 七、性能调优 -->## 七、性能调优

## 7.1 Sidecar 资源限制

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: istio-sidecar-injector
  namespace: istio-system
data:
  values: |
    sidecarInjectorWebhook:
      injectedAnnotations:
        sidecar.istio.io/proxyCPU: "100m"
        sidecar.istio.io/proxyMemory: "128Mi"
        sidecar.istio.io/proxyLimitCPU: "2000m"
        sidecar.istio.io/proxyLimitMemory: "1Gi"
```

## 7.2 Ambient 资源

```yaml
# ztunnel DaemonSet
resources:
  requests:
    cpu: "100m"
    memory: "128Mi"
  limits:
    cpu: "2000m"
    memory: "1Gi"

# Waypoint HPA
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: waypoint-hpa
  namespace: production
spec:
  minReplicas: 2
  maxReplicas: 10
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
```

## 7.3 istiod 调优

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: istiod
  namespace: istio-system
spec:
  template:
    spec:
      containers:
      - name: discovery
        env:
        - name: PILOT_PUSH_THROTTLE_COUNT
          value: "100"
        - name: PILOT_DEBOUNCE_AFTER
          value: "1s"
        - name: PILOT_DEBOUNCE_MAX
          value: "10s"
        - name: PILOT_FILTER_GATEWAY_CLUSTER_CONFIG
          value: "true"
        - name: PILOT_ENABLE_STATUS_WORKLOAD_ENTRY
          value: "true"
        - name: PILOT_EVICTION_INTERVAL
          value: "30s"
        - name: CACHE_LOG
          value: "true"
        resources:
          requests:
            cpu: "500m"
            memory: "2Gi"
          limits:
            cpu: "2000m"
            memory: "4Gi"
```

---

<!-- chunk: 八、故障排查 -->## 八、故障排查

## 8.1 诊断命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash

echo "=== 1. 全面配置分析 ==="
istioctl analyze -A

echo "=== 2. 代理同步状态 ==="
istioctl proxy-status

echo "=== 3. 代理配置检查 ==="
istioctl proxy-config cluster deployment/frontend -n production
istioctl proxy-config route deployment/frontend -n production
istioctl proxy-config endpoint deployment/frontend -n production
istioctl proxy-config listener deployment/frontend -n production
istioctl proxy-config secret deployment/frontend -n production

echo "=== 4. Sidecar 日志 ==="
kubectl logs deployment/frontend -c istio-proxy -n production --tail=100

echo "=== 5. istiod 日志 ==="
kubectl logs -n istio-system -l app=istiod --tail=100 | grep -iE "error|warn"

echo "=== 6. 资源使用 ==="
kubectl top pods -n istio-system
kubectl top pods -n production -l istio.io/rev=default

echo "=== 7. 安全策略 ==="
kubectl get peerauthentication -A
kubectl get authorizationpolicy -A
kubectl get requestauthentication -A

echo "=== 8. 流量配置 ==="
kubectl get virtualservices -A -o yaml | grep -A5 "hosts:"
kubectl get destinationrules -A -o yaml | grep -A5 "host:"
kubectl get gateways -A

echo "=== 9. 端到端连通性 ==="
kubectl exec -n default deploy/sleep -- \
  curl -s -o /dev/null -w "HTTP %{http_code}, Time: %{time_total}s\n" http://productpage:9080/productpage

echo "=== 10. 证书检查 ==="
istioctl proxy-config secret deployment/frontend -n production

```

## 8.2 常见问题

| 问题 | 原因 | 诊断命令 | 解决方案 |
|:---|:---|:---|:---|
| Sidecar 未注入 | namespace 缺少 label | `kubectl get ns -L istio-injection` | `kubectl label ns <ns> istio-injection=enabled` |
| mTLS 连接失败 | STRICT 但客户端无 Sidecar | `istioctl proxy-config secret` | 确认双方都有 Sidecar/Ambient 或用 PERMISSIVE |
| 503 UH (无健康主机) | outlierDetection 驱逐所有端点 | `istioctl proxy-config endpoint` | 检查 Pod 健康、调整 outlier 阈值 |
| 流量未按 VS 路由 | Gateway 配置不匹配 | `istioctl analyze` | 检查 hosts、Gateway 绑定、namespace |
| Prometheus 缺少指标 | 未配置 Telemetry API | `kubectl get telemetry -A` | 创建 Telemetry 资源启用 Prometheus |
| Ambient L7 不生效 | 缺少 waypoint proxy | `kubectl get pods -l istio.io/waypoint` | `istioctl waypoint apply -n <ns>` |
| 配置推送慢 | istiod 资源不足 | `kubectl top pods -n istio-system` | 增加 istiod CPU/内存 |
| 证书过期 | Citadel 异常 | `istioctl proxy-config secret` | 重启 istiod、检查根证书 |
| EnvoyFilter 冲突 | 自定义过滤器不兼容 | `kubectl get envoyfilter -A` | 检查过滤器优先级和兼容性 |
| 多集群通信失败 | 远程集群 Secret 过期 | `istioctl proxy-status` | 重新创建 remote-secret |

---

<!-- chunk: 参考链接 -->## 参考链接

- [Istio 官方文档](https://istio.io/latest/docs/)
- [Ambient Mesh 文档](https://istio.io/latest/docs/ambient/)
- [Gateway API 文档](https://gateway-api.sigs.k8s.io/)
- [Istio 安全最佳实践](https://istio.io/latest/docs/ops/best-practices/security/)
- [Kiali 文档](https://kiali.io/docs/)
- [Istio 性调优指南](https://istio.io/latest/docs/ops/best-practices/performance/)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-03-networking-traffic MOC
- [[domain-03-networking-traffic/README.md|Domain 03: 企业级服务网格与微服务治理 (Enterprise Service Mesh & Microser...]]
- Domain-26 服务网格与微服务 — 开源项目索引
- Istio 企业级服务网格架构与实践
- Linkerd 企业级服务网格深度实践
- Consul Connect 企业级服务网格管理
- Envoy Proxy 企业级服务网格数据平面深度实践
- Dapr (Distributed Application Runtime) Enterprise 深度实践
- Traefik Mesh Enterprise Service Mesh 深度实践
- 服务网格对比与选型决策指南
- Istio Ambient Mesh 与 L7 策略深度实践
- 微服务弹性模式深度实践 — Circuit Breaker, Retry, Timeout, Bulkhead, Rat...

## See Also

- 09-microservice-resilience-patterns
- 10-api-gateway-service-mesh-integration
- 99-linkerd-service-mesh-guide
- 99-spring-cloud-kubernetes-service-mesh-guide

## Related

- [[domain-19-landscape-references/topic-index/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]

```