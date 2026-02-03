# 59 - Egress流量管理

## Egress流量架构概述

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Kubernetes Egress流量管理架构                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                        Kubernetes Cluster                            │    │
│  │  ┌────────────────────────────────────────────────────────────────┐ │    │
│  │  │                     Application Namespace                       │ │    │
│  │  │  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐    │ │    │
│  │  │  │  Pod A   │   │  Pod B   │   │  Pod C   │   │  Pod D   │    │ │    │
│  │  │  │ (web)    │   │ (api)    │   │ (worker) │   │ (batch)  │    │ │    │
│  │  │  └────┬─────┘   └────┬─────┘   └────┬─────┘   └────┬─────┘    │ │    │
│  │  │       │              │              │              │          │ │    │
│  │  │       └──────────────┴──────────────┴──────────────┘          │ │    │
│  │  │                              │                                 │ │    │
│  │  │                   ┌──────────▼──────────┐                     │ │    │
│  │  │                   │   NetworkPolicy     │                     │ │    │
│  │  │                   │   (L3/L4控制)       │                     │ │    │
│  │  │                   └──────────┬──────────┘                     │ │    │
│  │  └──────────────────────────────┼────────────────────────────────┘ │    │
│  │                                 │                                   │    │
│  │  ┌──────────────────────────────┼────────────────────────────────┐ │    │
│  │  │              Service Mesh Layer (Optional)                     │ │    │
│  │  │  ┌───────────────────────────▼───────────────────────────┐   │ │    │
│  │  │  │              Egress Gateway (Istio/Cilium)             │   │ │    │
│  │  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │   │ │    │
│  │  │  │  │ TLS终止/发起│  │  L7路由     │  │  访问控制   │    │   │ │    │
│  │  │  │  └─────────────┘  └─────────────┘  └─────────────┘    │   │ │    │
│  │  │  └───────────────────────────┬───────────────────────────┘   │ │    │
│  │  └──────────────────────────────┼────────────────────────────────┘ │    │
│  │                                 │                                   │    │
│  │  ┌──────────────────────────────┼────────────────────────────────┐ │    │
│  │  │                    CNI Layer                                   │ │    │
│  │  │  ┌──────────────┐  ┌────────▼───────┐  ┌──────────────┐      │ │    │
│  │  │  │ Calico/Cilium│  │  Egress IP     │  │  SNAT/NAT    │      │ │    │
│  │  │  │    Policy    │  │   Allocation   │  │   Gateway    │      │ │    │
│  │  │  └──────────────┘  └────────┬───────┘  └──────────────┘      │ │    │
│  │  └──────────────────────────────┼────────────────────────────────┘ │    │
│  └─────────────────────────────────┼─────────────────────────────────┘ │    │
│                                    │                                    │    │
│  ┌─────────────────────────────────▼─────────────────────────────────┐ │    │
│  │                      Cloud NAT/Gateway Layer                       │ │    │
│  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐         │ │    │
│  │  │  NAT Gateway  │  │  Internet GW  │  │  VPN Gateway  │         │ │    │
│  │  │  (固定IP出口) │  │  (直接出口)   │  │ (私有链路)    │         │ │    │
│  │  └───────┬───────┘  └───────┬───────┘  └───────┬───────┘         │ │    │
│  └──────────┼──────────────────┼──────────────────┼──────────────────┘ │    │
└─────────────┼──────────────────┼──────────────────┼──────────────────────┘    │
              │                  │                  │                            
              ▼                  ▼                  ▼                            
┌─────────────────────────────────────────────────────────────────────────────┐
│                          External Services                                   │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐            │
│  │ Public API │  │ SaaS服务   │  │ 合作伙伴API │  │ 数据中心   │            │
│  │ (Google,   │  │ (Stripe,   │  │ (B2B集成)  │  │ (私有云)   │            │
│  │  AWS, etc) │  │  Twilio)   │  │            │  │            │            │
│  └────────────┘  └────────────┘  └────────────┘  └────────────┘            │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Egress流量控制方案对比

| 方案 | 功能 | 复杂度 | 性能影响 | 适用场景 | 控制粒度 | 监控能力 |
|-----|------|-------|---------|---------|---------|---------|
| NetworkPolicy | 基础出站控制 | 低 | 极低 | L3/L4限制 | IP/端口级 | 基础 |
| Egress Gateway | 统一出口IP | 中 | 低 | IP白名单场景 | IP级 | 中等 |
| NAT Gateway | 云原生NAT | 低 | 低 | 云环境 | VPC级 | 云监控 |
| Service Mesh | L7控制 | 高 | 中等 | 精细控制 | URL/Header级 | 全面 |
| Cilium | eBPF增强控制 | 中 | 极低 | 高性能场景 | L3-L7全栈 | 深度 |
| Calico | 企业级策略 | 中 | 低 | 混合云 | L3/L4级 | 中等 |

## Egress流量控制层次

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        Egress流量控制层次                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Layer 7 (应用层)                                                        │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │  • URL路径过滤 (/api/v1/*, /public/*)                              │ │
│  │  • HTTP方法控制 (GET, POST, PUT, DELETE)                           │ │
│  │  • Header检查 (Authorization, Content-Type)                         │ │
│  │  • 请求体检测 (敏感数据过滤)                                        │ │
│  │  • TLS版本控制 (TLS 1.2+)                                           │ │
│  │  工具: Istio, Envoy, Kong, nginx                                    │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
│  Layer 4 (传输层)                                                        │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │  • 端口控制 (80, 443, 3306, 5432)                                  │ │
│  │  • 协议限制 (TCP, UDP)                                              │ │
│  │  • 连接数限制                                                       │ │
│  │  • 带宽控制                                                         │ │
│  │  工具: NetworkPolicy, iptables, nftables                            │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
│  Layer 3 (网络层)                                                        │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │  • IP地址/CIDR控制                                                  │ │
│  │  • 命名空间隔离                                                     │ │
│  │  • Pod选择器                                                        │ │
│  │  • 出口IP分配                                                       │ │
│  │  工具: NetworkPolicy, Calico, Cilium                                │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
│  Infrastructure (基础设施层)                                             │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │  • NAT Gateway配置                                                  │ │
│  │  • VPC路由表                                                        │ │
│  │  • 安全组/防火墙规则                                                │ │
│  │  • VPN/专线连接                                                     │ │
│  │  工具: 云提供商控制台, Terraform                                    │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## NetworkPolicy Egress规则

### 基础Egress策略

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: egress-control
  namespace: production
  labels:
    policy-type: egress
    environment: production
  annotations:
    description: "生产环境出站流量控制策略"
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
  - Egress
  egress:
  # 允许访问内部数据库服务
  - to:
    - podSelector:
        matchLabels:
          app: database
          tier: backend
    ports:
    - protocol: TCP
      port: 5432
  
  # 允许访问Redis缓存
  - to:
    - podSelector:
        matchLabels:
          app: redis
    ports:
    - protocol: TCP
      port: 6379
  
  # 允许访问DNS (kube-dns/CoreDNS)
  - to:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
  
  # 允许访问特定外部API
  - to:
    - ipBlock:
        cidr: 203.0.113.0/24
        except:
        - 203.0.113.128/25  # 排除特定子网
    ports:
    - protocol: TCP
      port: 443
```

### 命名空间级别Egress策略

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: namespace-egress-policy
  namespace: frontend
spec:
  podSelector: {}  # 选择命名空间内所有Pod
  policyTypes:
  - Egress
  egress:
  # 允许访问backend命名空间
  - to:
    - namespaceSelector:
        matchLabels:
          name: backend
    ports:
    - protocol: TCP
      port: 8080
  
  # 允许访问monitoring命名空间
  - to:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 9090  # Prometheus
    - protocol: TCP
      port: 3000  # Grafana
  
  # 允许DNS
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
```

### 默认拒绝所有出站

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production
  annotations:
    description: "默认拒绝所有出站流量，需要显式允许"
spec:
  podSelector: {}  # 选择所有Pod
  policyTypes:
  - Egress
  # 空的egress数组意味着拒绝所有出站
  egress: []
---
# 配套的允许策略 - 允许DNS
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
```

### 外部服务CIDR访问控制

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: external-services-egress
  namespace: production
spec:
  podSelector:
    matchLabels:
      role: api-consumer
  policyTypes:
  - Egress
  egress:
  # AWS服务IP范围 (需要定期更新)
  - to:
    - ipBlock:
        cidr: 52.94.0.0/20      # AWS S3
    - ipBlock:
        cidr: 54.239.0.0/17     # AWS DynamoDB
    ports:
    - protocol: TCP
      port: 443
  
  # Google Cloud服务
  - to:
    - ipBlock:
        cidr: 142.250.0.0/15    # Google APIs
    ports:
    - protocol: TCP
      port: 443
  
  # 支付网关
  - to:
    - ipBlock:
        cidr: 91.218.229.0/24   # Stripe
    ports:
    - protocol: TCP
      port: 443
  
  # DNS服务
  - to:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
```

---

## Cilium Egress Gateway

### Cilium Helm配置启用Egress Gateway

```yaml
# cilium-values.yaml
# Helm安装: helm upgrade --install cilium cilium/cilium -f cilium-values.yaml -n kube-system
kubeProxyReplacement: strict
k8sServiceHost: "API_SERVER_IP"
k8sServicePort: "6443"

# 启用Egress Gateway功能
egressGateway:
  enabled: true
  installRoutes: true

# 启用BPF masquerading
bpf:
  masquerade: true
  hostRouting: true

# 启用策略审计
policyAuditMode: false

# IP池管理
ipam:
  mode: kubernetes
  operator:
    clusterPoolIPv4PodCIDRList:
    - "10.244.0.0/16"

# 监控集成
prometheus:
  enabled: true
  serviceMonitor:
    enabled: true

hubble:
  enabled: true
  relay:
    enabled: true
  ui:
    enabled: true
  metrics:
    enabled:
    - dns
    - drop
    - tcp
    - flow
    - icmp
    - http

# 节点初始化
nodeinit:
  enabled: true
  bootstrapFile: "/tmp/cilium-bootstrap-time"
```

### CiliumEgressGatewayPolicy配置

```yaml
# Cilium Egress Gateway策略
apiVersion: cilium.io/v2
kind: CiliumEgressGatewayPolicy
metadata:
  name: egress-external-api
  labels:
    policy-group: external-access
spec:
  # 源选择器 - 指定哪些Pod使用此Egress Gateway
  selectors:
  - podSelector:
      matchLabels:
        io.kubernetes.pod.namespace: production
        app: api-client
        egress-gateway: external
  
  # 目标CIDR - 指定通过Egress Gateway的目标
  destinationCIDRs:
  - "203.0.113.0/24"    # 外部API服务器
  - "198.51.100.0/24"   # 合作伙伴网络
  
  # 排除的目标 - 不通过Gateway的流量
  excludedCIDRs:
  - "10.0.0.0/8"        # 内部网络
  - "172.16.0.0/12"     # Docker网络
  - "192.168.0.0/16"    # 私有网络
  
  # Egress Gateway配置
  egressGateway:
    nodeSelector:
      matchLabels:
        node-role.kubernetes.io/egress-gateway: "true"
        topology.kubernetes.io/zone: "zone-a"
    # 固定出口IP
    egressIP: 10.0.100.50
    
---
# 高可用Egress Gateway配置
apiVersion: cilium.io/v2
kind: CiliumEgressGatewayPolicy
metadata:
  name: egress-ha-policy
spec:
  selectors:
  - podSelector:
      matchLabels:
        io.kubernetes.pod.namespace: critical-services
        tier: backend
  
  destinationCIDRs:
  - "0.0.0.0/0"  # 所有外部流量
  
  excludedCIDRs:
  - "10.0.0.0/8"
  - "172.16.0.0/12"
  - "192.168.0.0/16"
  - "100.64.0.0/10"  # CGNAT
  
  egressGateway:
    nodeSelector:
      matchLabels:
        egress-gateway: "ha-pool"
    # 使用IP池实现高可用
    # 当主节点故障时自动切换到备用节点
```

### Cilium节点标签配置

```bash
#!/bin/bash
# configure-egress-nodes.sh
# 配置Egress Gateway节点

# 标记Egress Gateway节点
kubectl label node egress-node-1 \
  node-role.kubernetes.io/egress-gateway=true \
  egress-gateway=ha-pool \
  topology.kubernetes.io/zone=zone-a

kubectl label node egress-node-2 \
  node-role.kubernetes.io/egress-gateway=true \
  egress-gateway=ha-pool \
  topology.kubernetes.io/zone=zone-b

# 验证节点标签
kubectl get nodes -l egress-gateway=ha-pool -o wide

# 查看Egress Gateway状态
cilium egress list

# 查看BPF egress映射
cilium bpf egress list

# 查看策略状态
cilium policy get --all
```

### Cilium L7 Egress策略

```yaml
# Cilium L7 Egress网络策略
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: l7-egress-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: api-client
  egress:
  # L3/L4规则 + L7 HTTP规则
  - toEndpoints:
    - matchLabels:
        app: backend-api
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:
        - method: "GET"
          path: "/api/v1/.*"
        - method: "POST"
          path: "/api/v1/orders"
          headers:
          - 'Content-Type: application/json'
  
  # 允许访问外部FQDN
  - toFQDNs:
    - matchName: "api.stripe.com"
    - matchPattern: "*.googleapis.com"
    toPorts:
    - ports:
      - port: "443"
        protocol: TCP
      rules:
        http:
        - method: "POST"
          path: "/v1/charges"
  
  # DNS出站规则
  - toEndpoints:
    - matchLabels:
        k8s:io.kubernetes.pod.namespace: kube-system
        k8s-app: kube-dns
    toPorts:
    - ports:
      - port: "53"
        protocol: UDP
      rules:
        dns:
        - matchName: "api.stripe.com"
        - matchPattern: "*.googleapis.com"
        - matchPattern: "*.cluster.local"
```

---

## Istio Egress Gateway

### Istio Egress Gateway部署

```yaml
# Istio Egress Gateway IstioOperator配置
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
metadata:
  name: istio-egress
  namespace: istio-system
spec:
  profile: default
  
  components:
    # Egress Gateway组件配置
    egressGateways:
    - name: istio-egressgateway
      enabled: true
      k8s:
        # 副本数和高可用
        replicaCount: 2
        
        # 资源配置
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 2000m
            memory: 1024Mi
        
        # HPA配置
        hpaSpec:
          minReplicas: 2
          maxReplicas: 5
          metrics:
          - type: Resource
            resource:
              name: cpu
              targetAverageUtilization: 80
        
        # 节点亲和性
        affinity:
          nodeAffinity:
            requiredDuringSchedulingIgnoredDuringExecution:
              nodeSelectorTerms:
              - matchExpressions:
                - key: node-role.kubernetes.io/egress
                  operator: In
                  values:
                  - "true"
          # Pod反亲和性确保高可用
          podAntiAffinity:
            preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    istio: egressgateway
                topologyKey: kubernetes.io/hostname
        
        # Service配置
        service:
          ports:
          - port: 80
            name: http
          - port: 443
            name: https
          - port: 15443
            name: tls
        
        # Pod注解
        podAnnotations:
          prometheus.io/scrape: "true"
          prometheus.io/port: "15020"
  
  # 全局Mesh配置
  meshConfig:
    # 默认出站流量策略
    outboundTrafficPolicy:
      mode: REGISTRY_ONLY  # 只允许注册的外部服务
    
    # 访问日志
    accessLogFile: /dev/stdout
    accessLogFormat: |
      [%START_TIME%] "%REQ(:METHOD)% %REQ(X-ENVOY-ORIGINAL-PATH?:PATH)% %PROTOCOL%"
      %RESPONSE_CODE% %RESPONSE_FLAGS% %BYTES_RECEIVED% %BYTES_SENT%
      %DURATION% "%REQ(X-FORWARDED-FOR)%" "%REQ(USER-AGENT)%"
      "%REQ(X-REQUEST-ID)%" "%REQ(:AUTHORITY)%" "%UPSTREAM_HOST%"
```

### Gateway资源配置

```yaml
# Egress Gateway资源定义
apiVersion: networking.istio.io/v1
kind: Gateway
metadata:
  name: egress-gateway
  namespace: istio-system
spec:
  selector:
    istio: egressgateway
  servers:
  # HTTPS/TLS透传
  - port:
      number: 443
      name: tls
      protocol: TLS
    hosts:
    - api.external.com
    - "*.googleapis.com"
    - api.stripe.com
    tls:
      mode: PASSTHROUGH
  
  # HTTPS终止 (用于需要修改请求的场景)
  - port:
      number: 8443
      name: https-terminate
      protocol: HTTPS
    hosts:
    - internal-proxy.external.com
    tls:
      mode: SIMPLE
      credentialName: egress-tls-cert
  
  # HTTP出口
  - port:
      number: 80
      name: http
      protocol: HTTP
    hosts:
    - "*.example.com"
```

### VirtualService和ServiceEntry配置

```yaml
# ServiceEntry - 定义外部服务
apiVersion: networking.istio.io/v1
kind: ServiceEntry
metadata:
  name: external-api
  namespace: istio-system
spec:
  hosts:
  - api.external.com
  ports:
  - number: 443
    name: https
    protocol: TLS
  resolution: DNS
  location: MESH_EXTERNAL
---
# VirtualService - 路由到Egress Gateway
apiVersion: networking.istio.io/v1
kind: VirtualService
metadata:
  name: external-api-routing
  namespace: istio-system
spec:
  hosts:
  - api.external.com
  gateways:
  - mesh            # 来自mesh内部的流量
  - egress-gateway  # 来自egress gateway的流量
  
  tls:
  # 从mesh内部到egress gateway
  - match:
    - gateways:
      - mesh
      port: 443
      sniHosts:
      - api.external.com
    route:
    - destination:
        host: istio-egressgateway.istio-system.svc.cluster.local
        port:
          number: 443
      weight: 100
  
  # 从egress gateway到外部服务
  - match:
    - gateways:
      - egress-gateway
      port: 443
      sniHosts:
      - api.external.com
    route:
    - destination:
        host: api.external.com
        port:
          number: 443
      weight: 100
---
# DestinationRule - 配置TLS设置
apiVersion: networking.istio.io/v1
kind: DestinationRule
metadata:
  name: external-api-dr
  namespace: istio-system
spec:
  host: api.external.com
  trafficPolicy:
    tls:
      mode: SIMPLE
      sni: api.external.com
    connectionPool:
      tcp:
        maxConnections: 100
        connectTimeout: 30s
      http:
        h2UpgradePolicy: UPGRADE
        http1MaxPendingRequests: 1024
        http2MaxRequests: 1024
    outlierDetection:
      consecutive5xxErrors: 5
      interval: 30s
      baseEjectionTime: 60s
      maxEjectionPercent: 100
```

### 多外部服务完整配置

```yaml
# 批量ServiceEntry配置
apiVersion: networking.istio.io/v1
kind: ServiceEntry
metadata:
  name: google-apis
  namespace: istio-system
spec:
  hosts:
  - "*.googleapis.com"
  - "accounts.google.com"
  - "oauth2.googleapis.com"
  ports:
  - number: 443
    name: https
    protocol: HTTPS
  - number: 80
    name: http
    protocol: HTTP
  resolution: DNS
  location: MESH_EXTERNAL
---
apiVersion: networking.istio.io/v1
kind: ServiceEntry
metadata:
  name: payment-services
  namespace: istio-system
spec:
  hosts:
  - "api.stripe.com"
  - "*.paypal.com"
  - "api.braintreegateway.com"
  ports:
  - number: 443
    name: https
    protocol: HTTPS
  resolution: DNS
  location: MESH_EXTERNAL
---
apiVersion: networking.istio.io/v1
kind: ServiceEntry
metadata:
  name: monitoring-services
  namespace: istio-system
spec:
  hosts:
  - "*.datadoghq.com"
  - "*.newrelic.com"
  - "*.pagerduty.com"
  ports:
  - number: 443
    name: https
    protocol: HTTPS
  resolution: DNS
  location: MESH_EXTERNAL
---
# 统一VirtualService
apiVersion: networking.istio.io/v1
kind: VirtualService
metadata:
  name: all-external-services
  namespace: istio-system
spec:
  hosts:
  - "*.googleapis.com"
  - "api.stripe.com"
  - "*.datadoghq.com"
  gateways:
  - mesh
  - egress-gateway
  tls:
  - match:
    - gateways:
      - mesh
      port: 443
    route:
    - destination:
        host: istio-egressgateway.istio-system.svc.cluster.local
        port:
          number: 443
```

### Istio AuthorizationPolicy控制

```yaml
# 基于源的Egress授权
apiVersion: security.istio.io/v1
kind: AuthorizationPolicy
metadata:
  name: egress-authz
  namespace: istio-system
spec:
  selector:
    matchLabels:
      istio: egressgateway
  action: ALLOW
  rules:
  # 允许production命名空间访问外部API
  - from:
    - source:
        namespaces:
        - production
        principals:
        - cluster.local/ns/production/sa/api-service
    to:
    - operation:
        hosts:
        - "api.stripe.com"
        - "*.googleapis.com"
        ports:
        - "443"
  
  # 允许monitoring命名空间访问监控服务
  - from:
    - source:
        namespaces:
        - monitoring
    to:
    - operation:
        hosts:
        - "*.datadoghq.com"
        - "*.newrelic.com"
        ports:
        - "443"
---
# 拒绝未授权访问
apiVersion: security.istio.io/v1
kind: AuthorizationPolicy
metadata:
  name: deny-unauthorized-egress
  namespace: istio-system
spec:
  selector:
    matchLabels:
      istio: egressgateway
  action: DENY
  rules:
  - from:
    - source:
        notNamespaces:
        - production
        - staging
        - monitoring
    to:
    - operation:
        notHosts:
        - "*.cluster.local"
```

---

## 云提供商Egress配置

### AWS EKS NAT Gateway

```yaml
# Terraform配置 AWS NAT Gateway
# nat-gateway.tf

resource "aws_nat_gateway" "egress" {
  count         = length(var.availability_zones)
  allocation_id = aws_eip.nat[count.index].id
  subnet_id     = aws_subnet.public[count.index].id

  tags = {
    Name        = "eks-egress-nat-${count.index}"
    Environment = var.environment
    ManagedBy   = "terraform"
  }

  depends_on = [aws_internet_gateway.main]
}

resource "aws_eip" "nat" {
  count  = length(var.availability_zones)
  domain = "vpc"

  tags = {
    Name = "eks-egress-eip-${count.index}"
  }
}

# 路由表配置
resource "aws_route_table" "private" {
  count  = length(var.availability_zones)
  vpc_id = aws_vpc.main.id

  route {
    cidr_block     = "0.0.0.0/0"
    nat_gateway_id = aws_nat_gateway.egress[count.index].id
  }

  tags = {
    Name = "eks-private-rt-${count.index}"
  }
}

# 输出NAT Gateway公网IP
output "nat_gateway_ips" {
  value = aws_eip.nat[*].public_ip
}
```

```yaml
# EKS Pod Egress IP注解 (使用AWS VPC CNI)
apiVersion: v1
kind: Pod
metadata:
  name: egress-enabled-pod
  annotations:
    # 使用自定义网络配置
    k8s.amazonaws.com/pod-networking-config: |
      {
        "securityGroups": ["sg-12345678"],
        "subnetIds": ["subnet-private-1", "subnet-private-2"]
      }
spec:
  containers:
  - name: app
    image: myapp:latest
---
# 使用Security Group for Pods实现精细控制
apiVersion: vpcresources.k8s.aws/v1beta1
kind: SecurityGroupPolicy
metadata:
  name: egress-sg-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      egress-tier: external
  securityGroups:
    groupIds:
    - sg-egress-allowed  # 允许出站的安全组
```

### Azure AKS NAT Gateway

```yaml
# Terraform配置 Azure NAT Gateway
# aks-nat-gateway.tf

resource "azurerm_public_ip" "nat" {
  name                = "aks-egress-nat-ip"
  location            = azurerm_resource_group.main.location
  resource_group_name = azurerm_resource_group.main.name
  allocation_method   = "Static"
  sku                 = "Standard"
  zones               = ["1", "2", "3"]
}

resource "azurerm_nat_gateway" "main" {
  name                    = "aks-egress-nat"
  location                = azurerm_resource_group.main.location
  resource_group_name     = azurerm_resource_group.main.name
  sku_name                = "Standard"
  idle_timeout_in_minutes = 10
  zones                   = ["1"]
}

resource "azurerm_nat_gateway_public_ip_association" "main" {
  nat_gateway_id       = azurerm_nat_gateway.main.id
  public_ip_address_id = azurerm_public_ip.nat.id
}

resource "azurerm_subnet_nat_gateway_association" "main" {
  subnet_id      = azurerm_subnet.aks_nodes.id
  nat_gateway_id = azurerm_nat_gateway.main.id
}

# AKS集群配置
resource "azurerm_kubernetes_cluster" "main" {
  name                = "aks-cluster"
  location            = azurerm_resource_group.main.location
  resource_group_name = azurerm_resource_group.main.name
  dns_prefix          = "aks"

  default_node_pool {
    name           = "default"
    node_count     = 3
    vm_size        = "Standard_D2s_v3"
    vnet_subnet_id = azurerm_subnet.aks_nodes.id
  }

  network_profile {
    network_plugin     = "azure"
    network_policy     = "calico"
    outbound_type      = "userAssignedNATGateway"
    service_cidr       = "10.0.0.0/16"
    dns_service_ip     = "10.0.0.10"
  }

  identity {
    type = "SystemAssigned"
  }
}
```

### GCP GKE Cloud NAT

```yaml
# Terraform配置 GCP Cloud NAT
# gke-cloud-nat.tf

resource "google_compute_router" "router" {
  name    = "gke-egress-router"
  region  = var.region
  network = google_compute_network.vpc.name
}

resource "google_compute_router_nat" "nat" {
  name                               = "gke-egress-nat"
  router                             = google_compute_router.router.name
  region                             = google_compute_router.router.region
  nat_ip_allocate_option             = "MANUAL_ONLY"
  nat_ips                            = google_compute_address.nat[*].self_link
  source_subnetwork_ip_ranges_to_nat = "LIST_OF_SUBNETWORKS"

  subnetwork {
    name                    = google_compute_subnetwork.gke_nodes.name
    source_ip_ranges_to_nat = ["ALL_IP_RANGES"]
  }

  log_config {
    enable = true
    filter = "ERRORS_ONLY"
  }

  # TCP连接配置
  tcp_established_idle_timeout_sec = 1200
  tcp_transitory_idle_timeout_sec  = 30
  
  # UDP配置
  udp_idle_timeout_sec = 30
  
  # ICMP配置
  icmp_idle_timeout_sec = 30

  # 端口分配
  min_ports_per_vm                    = 64
  max_ports_per_vm                    = 65536
  enable_endpoint_independent_mapping = true
}

resource "google_compute_address" "nat" {
  count  = 2
  name   = "gke-egress-nat-ip-${count.index}"
  region = var.region
}

# GKE集群配置
resource "google_container_cluster" "primary" {
  name     = "gke-cluster"
  location = var.region

  # 私有集群配置
  private_cluster_config {
    enable_private_nodes    = true
    enable_private_endpoint = false
    master_ipv4_cidr_block  = "172.16.0.0/28"
  }

  # 网络配置
  network    = google_compute_network.vpc.name
  subnetwork = google_compute_subnetwork.gke_nodes.name

  ip_allocation_policy {
    cluster_secondary_range_name  = "pods"
    services_secondary_range_name = "services"
  }

  # 网络策略
  network_policy {
    enabled  = true
    provider = "CALICO"
  }
}
```

### 阿里云ACK NAT Gateway

```yaml
# Terraform配置 阿里云 NAT Gateway
# ack-nat-gateway.tf

resource "alicloud_nat_gateway" "egress" {
  vpc_id               = alicloud_vpc.main.id
  nat_gateway_name     = "ack-egress-nat"
  payment_type         = "PayAsYouGo"
  vswitch_id           = alicloud_vswitch.public.id
  nat_type             = "Enhanced"
  network_type         = "internet"
}

resource "alicloud_eip_address" "nat" {
  count                = 2
  address_name         = "ack-egress-eip-${count.index}"
  bandwidth            = "200"
  internet_charge_type = "PayByTraffic"
}

resource "alicloud_eip_association" "nat" {
  count         = 2
  allocation_id = alicloud_eip_address.nat[count.index].id
  instance_id   = alicloud_nat_gateway.egress.id
  instance_type = "Nat"
}

resource "alicloud_snat_entry" "main" {
  snat_table_id     = alicloud_nat_gateway.egress.snat_table_ids
  source_vswitch_id = alicloud_vswitch.private.id
  snat_ip           = join(",", alicloud_eip_address.nat[*].ip_address)
}

# ACK集群配置
resource "alicloud_cs_managed_kubernetes" "cluster" {
  name                 = "ack-cluster"
  cluster_spec         = "ack.pro.small"
  worker_vswitch_ids   = [alicloud_vswitch.private.id]
  pod_vswitch_ids      = [alicloud_vswitch.pod.id]
  new_nat_gateway      = false
  service_cidr         = "172.21.0.0/20"
  
  addons {
    name = "terway-eniip"
  }
}
```

```yaml
# ACK Pod EIP注解配置
apiVersion: v1
kind: Pod
metadata:
  name: pod-with-eip
  annotations:
    # 自动分配EIP
    k8s.aliyun.com/pod-eip: "true"
    k8s.aliyun.com/eip-bandwidth: "10"
    k8s.aliyun.com/eip-common-bandwidth-package-id: "cbwp-xxx"
    k8s.aliyun.com/eip-internet-charge-type: "PayByTraffic"
spec:
  containers:
  - name: app
    image: myapp:latest
---
# 使用NAT网关的Pod
apiVersion: v1
kind: Pod
metadata:
  name: pod-with-nat
  annotations:
    k8s.aliyun.com/pod-eip: "true"
    k8s.aliyun.com/eip-bindingtype: "NAT"
spec:
  containers:
  - name: app
    image: myapp:latest
```

---

## Calico Egress配置

### Calico BGP Egress IP

```yaml
# Calico BGP配置
apiVersion: projectcalico.org/v3
kind: BGPConfiguration
metadata:
  name: default
spec:
  # 日志级别
  logSeverityScreen: Info
  
  # 节点间mesh
  nodeToNodeMeshEnabled: true
  
  # AS号
  asNumber: 64512
  
  # Service IP范围
  serviceClusterIPs:
  - cidr: 10.96.0.0/12
  
  # 外部IP范围
  serviceExternalIPs:
  - cidr: 203.0.113.0/24
  
  # Service LoadBalancer IP范围
  serviceLoadBalancerIPs:
  - cidr: 198.51.100.0/24
---
# BGP Peer配置
apiVersion: projectcalico.org/v3
kind: BGPPeer
metadata:
  name: my-global-peer
spec:
  peerIP: 192.168.1.1
  asNumber: 64513
  keepOriginalNextHop: true
```

### Calico Egress Gateway

```yaml
# Calico Egress Gateway IPPool
apiVersion: projectcalico.org/v3
kind: IPPool
metadata:
  name: egress-ippool
spec:
  cidr: 10.10.10.0/24
  natOutgoing: true
  nodeSelector: egress-gateway == "true"
  blockSize: 29
---
# Egress Gateway策略
apiVersion: projectcalico.org/v3
kind: EgressGateway
metadata:
  name: egress-gateway
  namespace: production
spec:
  # 副本数
  replicas: 2
  
  # IP池选择
  ipPool:
    cidr: 10.10.10.0/24
  
  # 外部网络
  externalNetworks:
  - name: external-api
    cidr: 203.0.113.0/24
    
  # 节点选择器
  egressGatewayNode:
    nodeSelector:
      egress-gateway: "true"
---
# Egress策略
apiVersion: projectcalico.org/v3
kind: EgressPolicy
metadata:
  name: egress-to-external
  namespace: production
spec:
  # 源选择
  selector: app == "api-client"
  
  # 目标
  destination:
  - cidr: 203.0.113.0/24
  
  # 使用的Gateway
  gateway:
    name: egress-gateway
    namespace: production
```

### Calico NetworkPolicy Egress

```yaml
# Calico增强NetworkPolicy
apiVersion: projectcalico.org/v3
kind: NetworkPolicy
metadata:
  name: advanced-egress-policy
  namespace: production
spec:
  selector: app == 'backend'
  types:
  - Egress
  
  egress:
  # 允许访问特定域名 (需要DNS策略)
  - action: Allow
    protocol: TCP
    destination:
      domains:
      - "api.stripe.com"
      - "*.googleapis.com"
      ports:
      - 443
  
  # 允许访问内部服务
  - action: Allow
    protocol: TCP
    destination:
      selector: app == 'database'
      ports:
      - 5432
  
  # 允许访问特定CIDR
  - action: Allow
    protocol: TCP
    destination:
      nets:
      - 10.0.0.0/8
      notNets:
      - 10.100.0.0/16  # 排除敏感网段
      ports:
      - 80
      - 443
  
  # DNS出站
  - action: Allow
    protocol: UDP
    destination:
      selector: k8s-app == 'kube-dns'
      namespaceSelector: projectcalico.org/name == 'kube-system'
      ports:
      - 53
---
# 全局网络策略
apiVersion: projectcalico.org/v3
kind: GlobalNetworkPolicy
metadata:
  name: default-deny-egress
spec:
  selector: all()
  types:
  - Egress
  egress:
  # 只允许DNS
  - action: Allow
    protocol: UDP
    destination:
      selector: k8s-app == 'kube-dns'
      namespaceSelector: projectcalico.org/name == 'kube-system'
      ports:
      - 53
  # 其他流量默认拒绝
```

---

## Egress监控与告警

### Prometheus监控规则

```yaml
# Egress监控PrometheusRule
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: egress-monitoring-rules
  namespace: monitoring
  labels:
    prometheus: main
    role: alert-rules
spec:
  groups:
  - name: egress.rules
    interval: 30s
    rules:
    # 出站流量速率
    - record: egress:traffic_bytes_rate:5m
      expr: |
        sum(rate(istio_tcp_sent_bytes_total{
          reporter="source",
          destination_service_namespace!~"kube-.*|istio-.*"
        }[5m])) by (source_workload, source_workload_namespace, destination_service)
    
    # 出站请求速率
    - record: egress:requests_rate:5m
      expr: |
        sum(rate(istio_requests_total{
          reporter="source",
          destination_service_namespace!~"kube-.*|istio-.*"
        }[5m])) by (source_workload, source_workload_namespace, destination_service, response_code)
    
    # 出站延迟P99
    - record: egress:latency_p99:5m
      expr: |
        histogram_quantile(0.99,
          sum(rate(istio_request_duration_milliseconds_bucket{
            reporter="source",
            destination_service_namespace!~"kube-.*|istio-.*"
          }[5m])) by (le, source_workload, destination_service)
        )
    
    # Cilium出站流量
    - record: egress:cilium_forward_bytes:5m
      expr: |
        sum(rate(cilium_forward_bytes_total{direction="EGRESS"}[5m])) 
        by (source_pod, destination)

  - name: egress.alerts
    rules:
    # 异常高出站流量
    - alert: EgressTrafficSpike
      expr: |
        sum(rate(istio_tcp_sent_bytes_total{
          reporter="source",
          destination_service_namespace!~"kube-.*|istio-.*"
        }[5m])) by (source_workload_namespace) 
        > 
        sum(rate(istio_tcp_sent_bytes_total{
          reporter="source",
          destination_service_namespace!~"kube-.*|istio-.*"
        }[1h] offset 1d)) by (source_workload_namespace) * 3
      for: 10m
      labels:
        severity: warning
        category: network
      annotations:
        summary: "出站流量异常增长"
        description: "命名空间 {{ $labels.source_workload_namespace }} 出站流量是昨日同期的3倍以上"
    
    # 出站连接失败率高
    - alert: EgressConnectionFailures
      expr: |
        sum(rate(istio_tcp_connections_closed_total{
          reporter="source",
          destination_service_namespace!~"kube-.*|istio-.*",
          response_flags=~"UF|UO|NR|URX"
        }[5m])) by (source_workload, destination_service)
        /
        sum(rate(istio_tcp_connections_opened_total{
          reporter="source",
          destination_service_namespace!~"kube-.*|istio-.*"
        }[5m])) by (source_workload, destination_service)
        > 0.1
      for: 5m
      labels:
        severity: critical
        category: network
      annotations:
        summary: "出站连接失败率高"
        description: "{{ $labels.source_workload }} 到 {{ $labels.destination_service }} 连接失败率超过10%"
    
    # 未授权出站访问尝试
    - alert: UnauthorizedEgressAttempt
      expr: |
        sum(rate(istio_requests_total{
          reporter="source",
          response_code="403",
          destination_service_namespace!~"kube-.*|istio-.*"
        }[5m])) by (source_workload, destination_service) > 1
      for: 5m
      labels:
        severity: warning
        category: security
      annotations:
        summary: "检测到未授权出站访问尝试"
        description: "{{ $labels.source_workload }} 尝试访问未授权的外部服务 {{ $labels.destination_service }}"
    
    # Egress Gateway不可用
    - alert: EgressGatewayDown
      expr: |
        sum(up{job="istio-egressgateway"}) == 0
        or
        absent(up{job="istio-egressgateway"})
      for: 2m
      labels:
        severity: critical
        category: infrastructure
      annotations:
        summary: "Egress Gateway不可用"
        description: "Istio Egress Gateway实例全部不可用，外部服务访问可能受影响"
    
    # 出站延迟异常
    - alert: EgressLatencyHigh
      expr: |
        histogram_quantile(0.99,
          sum(rate(istio_request_duration_milliseconds_bucket{
            reporter="source",
            destination_service_namespace!~"kube-.*|istio-.*"
          }[5m])) by (le, destination_service)
        ) > 5000
      for: 10m
      labels:
        severity: warning
        category: performance
      annotations:
        summary: "外部服务访问延迟高"
        description: "访问 {{ $labels.destination_service }} 的P99延迟超过5秒"
    
    # NAT Gateway端口耗尽预警
    - alert: NATGatewayPortExhaustion
      expr: |
        (aws_natgateway_active_connection_count / 55000) > 0.8
      for: 5m
      labels:
        severity: warning
        category: infrastructure
      annotations:
        summary: "NAT Gateway连接数接近上限"
        description: "NAT Gateway活跃连接数已达到上限的80%，可能影响新的出站连接"
    
    # DNS查询失败
    - alert: EgressDNSFailures
      expr: |
        sum(rate(coredns_dns_responses_total{rcode="SERVFAIL"}[5m])) 
        / 
        sum(rate(coredns_dns_responses_total[5m])) > 0.01
      for: 5m
      labels:
        severity: warning
        category: network
      annotations:
        summary: "DNS查询失败率升高"
        description: "DNS SERVFAIL响应比例超过1%，可能影响外部服务访问"
```

### Grafana Dashboard配置

```json
{
  "dashboard": {
    "title": "Egress Traffic Dashboard",
    "uid": "egress-traffic",
    "timezone": "browser",
    "panels": [
      {
        "title": "Egress流量概览",
        "type": "stat",
        "gridPos": {"h": 4, "w": 6, "x": 0, "y": 0},
        "targets": [
          {
            "expr": "sum(rate(istio_tcp_sent_bytes_total{reporter=\"source\",destination_service_namespace!~\"kube-.*|istio-.*\"}[5m]))",
            "legendFormat": "Total Egress"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "unit": "Bps",
            "thresholds": {
              "steps": [
                {"value": 0, "color": "green"},
                {"value": 100000000, "color": "yellow"},
                {"value": 500000000, "color": "red"}
              ]
            }
          }
        }
      },
      {
        "title": "按目标服务的出站流量",
        "type": "timeseries",
        "gridPos": {"h": 8, "w": 12, "x": 0, "y": 4},
        "targets": [
          {
            "expr": "sum(rate(istio_tcp_sent_bytes_total{reporter=\"source\",destination_service_namespace!~\"kube-.*|istio-.*\"}[5m])) by (destination_service)",
            "legendFormat": "{{ destination_service }}"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "unit": "Bps",
            "custom": {
              "drawStyle": "line",
              "lineWidth": 1,
              "fillOpacity": 30
            }
          }
        }
      },
      {
        "title": "出站请求成功率",
        "type": "gauge",
        "gridPos": {"h": 4, "w": 6, "x": 6, "y": 0},
        "targets": [
          {
            "expr": "sum(rate(istio_requests_total{reporter=\"source\",destination_service_namespace!~\"kube-.*|istio-.*\",response_code=~\"2..\"}[5m])) / sum(rate(istio_requests_total{reporter=\"source\",destination_service_namespace!~\"kube-.*|istio-.*\"}[5m])) * 100",
            "legendFormat": "Success Rate"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "unit": "percent",
            "min": 0,
            "max": 100,
            "thresholds": {
              "steps": [
                {"value": 0, "color": "red"},
                {"value": 95, "color": "yellow"},
                {"value": 99, "color": "green"}
              ]
            }
          }
        }
      },
      {
        "title": "按命名空间的出站流量分布",
        "type": "piechart",
        "gridPos": {"h": 8, "w": 6, "x": 12, "y": 4},
        "targets": [
          {
            "expr": "sum(rate(istio_tcp_sent_bytes_total{reporter=\"source\",destination_service_namespace!~\"kube-.*|istio-.*\"}[5m])) by (source_workload_namespace)",
            "legendFormat": "{{ source_workload_namespace }}"
          }
        ]
      },
      {
        "title": "出站延迟分布",
        "type": "heatmap",
        "gridPos": {"h": 8, "w": 12, "x": 0, "y": 12},
        "targets": [
          {
            "expr": "sum(rate(istio_request_duration_milliseconds_bucket{reporter=\"source\",destination_service_namespace!~\"kube-.*|istio-.*\"}[5m])) by (le)",
            "legendFormat": "{{ le }}"
          }
        ]
      },
      {
        "title": "Top 10 出站目标",
        "type": "table",
        "gridPos": {"h": 8, "w": 12, "x": 12, "y": 12},
        "targets": [
          {
            "expr": "topk(10, sum(rate(istio_requests_total{reporter=\"source\",destination_service_namespace!~\"kube-.*|istio-.*\"}[5m])) by (destination_service))",
            "format": "table",
            "instant": true
          }
        ],
        "transformations": [
          {
            "id": "organize",
            "options": {
              "renameByName": {
                "destination_service": "目标服务",
                "Value": "请求率 (req/s)"
              }
            }
          }
        ]
      }
    ]
  }
}
```

### 日志收集配置

```yaml
# Fluentd配置 - Egress日志收集
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluentd-egress-config
  namespace: logging
data:
  fluent.conf: |
    <source>
      @type tail
      path /var/log/containers/istio-egressgateway*.log
      pos_file /var/log/fluentd-egress.pos
      tag egress.istio
      <parse>
        @type json
        time_format %Y-%m-%dT%H:%M:%S.%NZ
      </parse>
    </source>
    
    <filter egress.**>
      @type record_transformer
      enable_ruby
      <record>
        cluster_name "#{ENV['CLUSTER_NAME']}"
        egress_type "istio"
        parsed_log ${record["log"].to_s.include?("upstream_cluster") ? JSON.parse(record["log"]) : {}}
      </record>
    </filter>
    
    <filter egress.**>
      @type grep
      <regexp>
        key log
        pattern /upstream_cluster|response_code|destination/
      </regexp>
    </filter>
    
    <match egress.**>
      @type elasticsearch
      host elasticsearch.logging.svc
      port 9200
      index_name egress-logs
      type_name _doc
      logstash_format true
      logstash_prefix egress
      <buffer>
        @type file
        path /var/log/fluentd-egress-buffer
        flush_interval 5s
        retry_max_interval 30
        retry_forever true
      </buffer>
    </match>
```

---

## Egress故障排查

### 排查脚本

```bash
#!/bin/bash
# egress-troubleshoot.sh
# Egress流量故障排查脚本

set -e

POD_NAME=${1:-""}
NAMESPACE=${2:-"default"}
TARGET_HOST=${3:-""}

echo "=========================================="
echo "Egress流量故障排查"
echo "=========================================="

# 检查Pod网络配置
echo -e "\n[1] 检查Pod网络配置"
if [ -n "$POD_NAME" ]; then
    kubectl get pod $POD_NAME -n $NAMESPACE -o yaml | grep -A 20 "status:"
fi

# 检查NetworkPolicy
echo -e "\n[2] 检查NetworkPolicy规则"
kubectl get networkpolicy -n $NAMESPACE -o wide
echo -e "\n--- NetworkPolicy详情 ---"
kubectl get networkpolicy -n $NAMESPACE -o yaml | grep -A 50 "egress:"

# 检查出站连接
echo -e "\n[3] 测试出站连接"
if [ -n "$POD_NAME" ] && [ -n "$TARGET_HOST" ]; then
    echo "测试DNS解析..."
    kubectl exec $POD_NAME -n $NAMESPACE -- nslookup $TARGET_HOST 2>/dev/null || echo "DNS解析失败"
    
    echo -e "\n测试TCP连接..."
    kubectl exec $POD_NAME -n $NAMESPACE -- timeout 10 bash -c "echo | nc -vz $TARGET_HOST 443" 2>&1 || echo "TCP连接失败"
    
    echo -e "\n测试HTTPS连接..."
    kubectl exec $POD_NAME -n $NAMESPACE -- curl -v --connect-timeout 10 https://$TARGET_HOST 2>&1 | head -30
fi

# 检查Cilium状态
echo -e "\n[4] 检查Cilium Egress状态"
if kubectl get crd ciliumegressgatewaypolicies.cilium.io &>/dev/null; then
    echo "Cilium Egress Gateway策略:"
    kubectl get ciliumegressgatewaypolicies -A
    
    echo -e "\n--- BPF Egress映射 ---"
    CILIUM_POD=$(kubectl get pods -n kube-system -l k8s-app=cilium -o jsonpath='{.items[0].metadata.name}')
    kubectl exec -n kube-system $CILIUM_POD -- cilium bpf egress list 2>/dev/null || echo "无法获取BPF映射"
fi

# 检查Istio Egress Gateway
echo -e "\n[5] 检查Istio Egress Gateway状态"
if kubectl get ns istio-system &>/dev/null; then
    echo "Egress Gateway Pods:"
    kubectl get pods -n istio-system -l istio=egressgateway
    
    echo -e "\n--- ServiceEntry ---"
    kubectl get serviceentry -A
    
    echo -e "\n--- VirtualService (Egress) ---"
    kubectl get virtualservice -A -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.gateways[*]}{"\n"}{end}' | grep -i egress
    
    if [ -n "$POD_NAME" ]; then
        echo -e "\n--- Envoy路由配置 ---"
        istioctl proxy-config routes $POD_NAME -n $NAMESPACE --name "$TARGET_HOST" 2>/dev/null || echo "无匹配路由"
    fi
fi

# 检查DNS
echo -e "\n[6] 检查DNS配置"
kubectl get configmap coredns -n kube-system -o yaml | grep -A 20 "Corefile"

# 检查NAT Gateway (如果有云提供商CLI)
echo -e "\n[7] 检查云NAT Gateway状态"
echo "请使用云提供商CLI检查NAT Gateway状态"
echo "  AWS: aws ec2 describe-nat-gateways"
echo "  GCP: gcloud compute routers get-nat-mapping-info"
echo "  Azure: az network nat gateway show"

# 检查iptables规则
echo -e "\n[8] 检查节点iptables规则"
NODE=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.nodeName}' 2>/dev/null)
if [ -n "$NODE" ]; then
    echo "Pod运行在节点: $NODE"
    echo "需要SSH到节点检查: iptables -t nat -L POSTROUTING -v"
fi

# 网络策略验证
echo -e "\n[9] 网络策略效果验证"
if [ -n "$POD_NAME" ]; then
    echo "Pod标签:"
    kubectl get pod $POD_NAME -n $NAMESPACE --show-labels
    
    echo -e "\n匹配的NetworkPolicy:"
    POD_LABELS=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.metadata.labels}')
    kubectl get networkpolicy -n $NAMESPACE -o json | jq --arg labels "$POD_LABELS" '.items[] | select(.spec.podSelector.matchLabels | to_entries | all(. as $e | ($labels | fromjson) | has($e.key) and .[$e.key] == $e.value)) | .metadata.name'
fi

echo -e "\n=========================================="
echo "排查完成"
echo "=========================================="
```

### 常用排查命令

```bash
# 检查出站连接
kubectl exec <pod> -- curl -v --connect-timeout 10 https://api.external.com

# 检查DNS解析
kubectl exec <pod> -- nslookup api.external.com
kubectl exec <pod> -- dig +short api.external.com

# 检查NetworkPolicy
kubectl get networkpolicy -n <namespace> -o yaml
kubectl describe networkpolicy <policy-name> -n <namespace>

# 检查Cilium策略
cilium policy get --all
cilium endpoint list
cilium bpf egress list
cilium monitor --type policy-verdict

# 检查Istio配置
istioctl proxy-config routes <pod> -n <namespace>
istioctl proxy-config clusters <pod> -n <namespace> | grep -i external
istioctl analyze -n <namespace>

# 检查Egress Gateway日志
kubectl logs -n istio-system -l istio=egressgateway --tail=100

# 检查Envoy访问日志
kubectl logs <pod> -c istio-proxy --tail=100 | grep "egress"

# 检查Calico策略
calicoctl get networkpolicy -n <namespace> -o yaml
calicoctl get globalnetworkpolicy -o yaml

# 抓包分析
kubectl debug -it <pod> --image=nicolaka/netshoot -- tcpdump -i any host api.external.com -w /tmp/capture.pcap
```

---

## 最佳实践

### Egress安全清单

| 类别 | 检查项 | 说明 | 优先级 |
|-----|-------|------|-------|
| **默认策略** | 默认拒绝出站 | 实施default-deny NetworkPolicy | 高 |
| | 显式允许必要流量 | 仅允许业务需要的出站连接 | 高 |
| **DNS安全** | DNS策略控制 | 限制可解析的外部域名 | 中 |
| | DNS流量加密 | 使用DoH/DoT | 低 |
| **流量审计** | 访问日志 | 记录所有出站连接 | 高 |
| | 流量监控 | Prometheus metrics | 高 |
| | 异常检测 | 告警异常流量模式 | 中 |
| **出口IP管理** | 固定出口IP | 使用Egress Gateway/NAT | 中 |
| | IP白名单协调 | 与外部服务提供商协调 | 中 |
| **加密** | TLS 1.2+ | 强制使用现代TLS | 高 |
| | 证书验证 | 验证外部服务证书 | 高 |
| **访问控制** | 命名空间隔离 | 按命名空间限制出站 | 高 |
| | 服务账户绑定 | 按SA限制出站权限 | 中 |
| **高可用** | 多副本Gateway | Egress Gateway至少2副本 | 高 |
| | 跨AZ部署 | Gateway跨可用区分布 | 中 |

### 配置模板

```yaml
# 生产环境推荐配置模板
---
# 1. 默认拒绝所有出站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress: []
---
# 2. 允许DNS
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
---
# 3. 允许内部服务通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-internal
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          environment: production
    ports:
    - protocol: TCP
---
# 4. 允许特定外部服务 (通过Egress Gateway)
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-egress-gateway
  namespace: production
spec:
  podSelector:
    matchLabels:
      egress-access: external
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: istio-system
      podSelector:
        matchLabels:
          istio: egressgateway
    ports:
    - protocol: TCP
      port: 443
```

---

## 版本变更记录

| 版本 | 日期 | 变更内容 |
|-----|------|---------|
| v1.0 | 2024-01 | 初始版本，基础Egress控制 |
| v2.0 | 2024-06 | 添加Cilium L7策略支持 |
| v2.1 | 2024-09 | 添加云提供商NAT Gateway配置 |
| v2.2 | 2024-12 | 增强监控和告警规则 |
| v3.0 | 2025-01 | 全面重构，添加架构图和完整配置示例 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
