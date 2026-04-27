# Istio 企业级服务网格入门指南

> **适用版本**: Istio v1.29.0  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

## 📋 目录

- [一、架构模式选择](#一架构模式选择)
- [二、安装部署](#二安装部署)
- [三、流量管理](#三流量管理)
- [四、安全加固](#四安全加固)
- [五、可观测性配置](#五可观测性配置)
- [六、多集群部署](#六多集群部署)
- [七、性能调优](#七性能调优)
- [八、常见问题](#八常见问题)

---

## 一、架构模式选择

### 1.1 Sidecar 模式 (传统)

```
Pod
├── App Container
├── istio-proxy (Envoy Sidecar)
│   ├── Ingress Listener (15006)
│   └── Egress Listener (15001)
└── init container (istio-init): iptables 流量拦截
```

**优点**: 功能完整、成熟稳定  
**缺点**: 额外资源开销、Pod 启动延迟

### 1.2 Ambient Mesh (无 Sidecar)

```
Node
├── ztunnel (DaemonSet): L4 处理、mTLS
└── waypoint proxy (按需): L7 策略、可观测性

Pod (无 Sidecar)
└── App Container (原生网络)
```

**优点**: 更低资源占用、更快启动、更简单运维  
**状态**: v1.29 功能完善，生产可用

### 1.3 选型建议

| 场景 | 推荐模式 |
|:---|:---|
| 全新部署 | Ambient Mesh |
| 资源极度敏感 | Ambient Mesh |
| 已大量 Sidecar 投入 | 渐进迁移至 Ambient |
| 需要完整 L7 功能 | Sidecar 或 Ambient + Waypoint |
| 遗留系统兼容性 | Sidecar (更成熟) |

---

## 二、安装部署

### 2.1 istioctl 安装 (推荐)

```bash
# 下载
curl -L https://istio.io/downloadIstio | sh -
cd istio-1.29.0
export PATH=$PWD/bin:$PATH

# 查看配置配置文件
istioctl profile list
# 可选: default, demo, minimal, ambient, empty, preview, external

# 生产环境安装 (Sidecar 模式)
istioctl install --set profile=default \
  --set values.global.proxy.holdApplicationUntilProxyStarts=true \
  --set values.meshConfig.defaultConfig.holdApplicationUntilProxyStarts=true \
  -y

# Ambient 模式安装
istioctl install --set profile=ambient -y
```

### 2.2 Helm 安装

```bash
helm repo add istio https://istio-release.storage.googleapis.com/charts
helm repo update

# 安装基础 CRD
helm install istio-base istio/base -n istio-system --create-namespace

# 安装 istiod
helm install istiod istio/istiod -n istio-system --wait

# 安装 Ingress Gateway (如需要)
helm install istio-ingressgateway istio/gateway -n istio-system
```

### 2.3 命名空间注入

```bash
# Sidecar 自动注入
kubectl label namespace default istio-injection=enabled

# Ambient 模式加入数据平面
kubectl label namespace default istio.io/dataplane-mode=ambient
```

---

## 三、流量管理

### 3.1 VirtualService 与 DestinationRule

```yaml
apiVersion: networking.istio.io/v1
kind: VirtualService
metadata:
  name: reviews-route
spec:
  hosts:
  - reviews
  http:
  - match:
    - headers:
        end-user:
          exact: jason
    route:
    - destination:
        host: reviews
        subset: v2
  - route:
    - destination:
        host: reviews
        subset: v1
      weight: 75
    - destination:
        host: reviews
        subset: v2
      weight: 25
---
apiVersion: networking.istio.io/v1
kind: DestinationRule
metadata:
  name: reviews-destination
spec:
  host: reviews
  trafficPolicy:
    connectionPool:
      tcp:
        maxConnections: 100
      http:
        http1MaxPendingRequests: 50
        maxRequestsPerConnection: 10
    outlierDetection:
      consecutive5xxErrors: 5
      interval: 30s
      baseEjectionTime: 30s
  subsets:
  - name: v1
    labels:
      version: v1
  - name: v2
    labels:
      version: v2
```

### 3.2 Gateway API (推荐新标准)

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: external-gateway
spec:
  gatewayClassName: istio
  listeners:
  - name: https
    protocol: HTTPS
    port: 443
    tls:
      mode: Terminate
      certificateRefs:
      - name: example-cert
---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: frontend-route
spec:
  parentRefs:
  - name: external-gateway
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /
    backendRefs:
    - name: frontend
      port: 8080
```

---

## 四、安全加固

### 4.1 全局 mTLS ( STRICT 模式)

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
# 允许特定命名空间 PERMISSIVE (迁移期)
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: legacy-allow
  namespace: legacy
spec:
  mtls:
    mode: PERMISSIVE
```

### 4.2 授权策略 (AuthorizationPolicy)

```yaml
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: frontend-policy
  namespace: default
spec:
  selector:
    matchLabels:
      app: frontend
  action: ALLOW
  rules:
  - from:
    - source:
        namespaces: ["istio-ingressgateway"]
    to:
    - operation:
        methods: ["GET", "POST"]
        paths: ["/api/*"]
  - from:
    - source:
        principals: ["cluster.local/ns/default/sa/backend"]
    to:
    - operation:
        methods: ["GET"]
        paths: ["/internal/health"]
```

### 4.3 JWT 认证

```yaml
apiVersion: security.istio.io/v1beta1
kind: RequestAuthentication
metadata:
  name: jwt-auth
  namespace: default
spec:
  selector:
    matchLabels:
      app: frontend
  jwtRules:
  - issuer: "https://accounts.google.com"
    jwksUri: "https://www.googleapis.com/oauth2/v3/certs"
    audiences: ["my-client-id"]
---
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: require-jwt
  namespace: default
spec:
  selector:
    matchLabels:
      app: frontend
  action: ALLOW
  rules:
  - from:
    - source:
        requestPrincipals: ["*"]
```

---

## 五、可观测性配置

### 5.1 自动指标、日志、追踪

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
    - match:
        metric: ALL_METRICS
      tagOverrides:
        destination_cluster:
          value: "cluster-1"
  accessLogging:
  - providers:
    - name: envoy
  tracing:
  - providers:
    - name: otel-collector
    randomSamplingPercentage: 10.0
```

### 5.2 Kiali 可视化

```bash
# 安装 Kiali (与 Prometheus 集成)
kubectl apply -f https://raw.githubusercontent.com/istio/istio/release-1.29/samples/addons/kiali.yaml

# 访问
istioctl dashboard kiali
```

---

## 六、多集群部署

### 6.1 单网络多集群 (Flat Network)

```yaml
# cluster-1 安装
istioctl install --set profile=default \
  --set values.global.multiCluster.clusterName=cluster1 \
  --set values.global.network=network1 \
  -y

# 暴露 istiod
kubectl apply -f - <<EOF
apiVersion: networking.istio.io/v1beta1
kind: Gateway
metadata:
  name: cross-network-gateway
spec:
  selector:
    istio: ingressgateway
  servers:
  - port:
      number: 443
      name: tls
      protocol: TLS
    tls:
      mode: AUTO_PASSTHROUGH
    hosts:
    - "*.local"
EOF
```

---

## 七、性能调优

### 7.1 Sidecar 资源限制

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

### 7.2 Ambient 资源

```yaml
# ztunnel DaemonSet 资源
tolerations:
  - operator: Exists
resources:
  requests:
    cpu: "100m"
    memory: "256Mi"
  limits:
    cpu: "2000m"
    memory: "1Gi"
```

---

## 八、常见问题

| 问题 | 原因 | 解决 |
|:---|:---|:---|
| Sidecar 未注入 | 命名空间缺少 label | `kubectl label ns <ns> istio-injection=enabled` |
| mTLS 连接失败 | PeerAuthentication STRICT 但客户端未注入 | 检查双方 Sidecar / Ambient 状态 |
| 503 UH (无健康主机) | DestinationRule outlierDetection 驱逐 | 检查 Pod 健康状态、调整 outlier 阈值 |
| 流量未按 VirtualService 路由 |  Gateway 配置不匹配 | 检查 hosts、Gateway 绑定 |
| Prometheus 缺少指标 | 未启用 stats filter | 检查 Telemetry API 或 EnvoyFilter |
| Ambient L7 策略不生效 | 缺少 waypoint proxy | `istioctl waypoint apply -n <ns>` |

---

## 参考链接

- [Istio 官方文档](https://istio.io/latest/docs/)
- [Ambient Mesh 文档](https://istio.io/latest/docs/ambient/)
- [Gateway API 文档](https://gateway-api.sigs.k8s.io/)
- [Istio 安全最佳实践](https://istio.io/latest/docs/ops/best-practices/security/)
- [Kiali 文档](https://kiali.io/docs/)
