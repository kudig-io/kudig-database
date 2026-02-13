# 网络核心组件

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [Kubernetes Networking](https://kubernetes.io/docs/concepts/services-networking/)

## 网络架构全景图

```
┌─────────────────────────────────────────────────────────────┐
│                     外部流量入口                             │
│  Internet → DNS → CDN/WAF → Cloud LB (SLB/ALB/NLB)         │
└────────────────────────┬────────────────────────────────────┘
                         │
                         v
┌─────────────────────────────────────────────────────────────┐
│                  Kubernetes Ingress层                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Nginx Ingress│  │ Gateway API  │  │ ALB Ingress  │      │
│  │ Controller   │  │ (HTTPRoute)  │  │ Controller   │      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
└─────────┼──────────────────┼──────────────────┼─────────────┘
          │                  │                  │
          └──────────────────┴──────────────────┘
                         │
                         v
┌─────────────────────────────────────────────────────────────┐
│                  Kubernetes Service层                        │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐           │
│  │ ClusterIP  │  │ NodePort   │  │LoadBalancer│           │
│  │ (内部LB)   │  │ (端口映射) │  │ (云LB集成) │           │
│  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘           │
└────────┼─────────────────┼─────────────────┼────────────────┘
         │                 │                 │
         └─────────────────┴─────────────────┘
                         │
                         v
┌─────────────────────────────────────────────────────────────┐
│                    CNI网络层                                 │
│  ┌──────────────────────────────────────────────────┐      │
│  │        Pod Network (10.244.0.0/16)               │      │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐          │      │
│  │  │  Pod    │  │  Pod    │  │  Pod    │          │      │
│  │  │ eth0    │  │ eth0    │  │ eth0    │          │      │
│  │  │10.244.x │  │10.244.y │  │10.244.z │          │      │
│  │  └────┬────┘  └────┬────┘  └────┬────┘          │      │
│  └───────┼────────────┼────────────┼───────────────┘      │
│          │            │            │                       │
│     ┌────┴────────────┴────────────┴─────┐                │
│     │      CNI Bridge / veth / ENI       │                │
│     │    (Flannel/Calico/Cilium/Terway)  │                │
│     └────────────────┬───────────────────┘                │
└──────────────────────┼────────────────────────────────────┘
                       │
                       v
            ┌──────────────────────┐
            │   物理网络 / VPC     │
            │   (Node Network)     │
            └──────────────────────┘
```

---

## Service 核心类型深度解析

### 1. ClusterIP - 内部服务发现

#### 架构价值
- **产品视角**: 微服务内部通信的唯一标准入口，屏蔽后端Pod变更
- **运维视角**: 自动负载均衡，无需额外配置，故障自愈
- **架构视角**: 服务网格的基础，支持灰度、熔断等高级治理

#### 生产配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: backend-service
  namespace: production
  labels:
    app: backend
    tier: api
    env: production
  annotations:
    # 服务描述
    description: "核心业务API服务"
    owner: "backend-team@company.com"
    # Prometheus监控
    prometheus.io/scrape: "true"
    prometheus.io/port: "9090"
    prometheus.io/path: "/metrics"
spec:
  type: ClusterIP
  # 固定ClusterIP(灾备需要)
  clusterIP: 10.96.100.50
  # 会话亲和性(保持用户会话)
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 3小时
  # 服务拓扑感知(v1.27+ GA)
  topologyKeys:
    - "kubernetes.io/hostname"      # 优先同节点
    - "topology.kubernetes.io/zone" # 其次同可用区
    - "*"                            # 最后跨区域
  # 端口配置
  ports:
    - name: http
      protocol: TCP
      port: 80        # Service暴露端口
      targetPort: 8080 # Pod实际监听端口
    - name: https
      protocol: TCP
      port: 443
      targetPort: 8443
    - name: metrics
      protocol: TCP
      port: 9090
      targetPort: 9090
  # Pod选择器
  selector:
    app: backend
    version: v2.5.0  # 精确版本控制
  # 发布升级策略(与Deployment配合)
  publishNotReadyAddresses: false  # 仅路由到Ready的Pod
```

#### 会话亲和性场景

| 场景 | 配置 | 原因 |
|------|------|------|
| 有状态WebSocket | `sessionAffinity: ClientIP` | 保持长连接到同一Pod |
| 本地缓存应用 | `sessionAffinity: ClientIP` | 提升缓存命中率 |
| 无状态REST API | `sessionAffinity: None` | 负载均衡更均匀 |
| gRPC长连接 | 使用Headless Service | 客户端直连Pod |

#### 拓扑感知路由 (Topology Aware Hints)

```yaml
# 启用拓扑感知(v1.27+ GA)
apiVersion: v1
kind: Service
metadata:
  name: cache-service
  annotations:
    service.kubernetes.io/topology-aware-hints: "auto"
spec:
  type: ClusterIP
  selector:
    app: redis-cache
  ports:
    - port: 6379
  # kube-proxy自动优化路由
  # 优先选择同可用区的Endpoint，降低跨AZ延迟和成本
```

**生产效果**:
- 延迟降低 40-60%
- 跨AZ流量成本降低 70%
- 适用于跨多可用区部署的缓存、数据库等延迟敏感服务

---

### 2. NodePort - 端口直通模式

#### 架构场景
- **产品视角**: 快速暴露服务给外部系统，无需云LB成本
- **运维视角**: 测试环境快速验证，生产环境**不推荐**
- **架构视角**: 配合物理LB做4层负载均衡

#### 生产配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: test-service
  namespace: staging
spec:
  type: NodePort
  # 源IP保留(关键)
  externalTrafficPolicy: Local
  # 健康检查端口(externalTrafficPolicy=Local时必需)
  healthCheckNodePort: 32000
  ports:
    - name: http
      protocol: TCP
      port: 80
      targetPort: 8080
      nodePort: 30080  # 固定端口(30000-32767)
  selector:
    app: test-app
```

#### externalTrafficPolicy 对比

| 策略 | 源IP | 负载均衡 | 跨节点跳转 | 适用场景 |
|------|------|---------|-----------|---------|
| **Cluster**(默认) | SNAT转换(丢失) | 全局均衡 | 有 | 无IP要求 |
| **Local** | 保留客户端IP | 仅本节点Pod | 无 | 审计日志、IP白名单 |

**生产实践**:
```bash
# 验证源IP保留
kubectl logs -f nginx-pod | grep "X-Forwarded-For"
# externalTrafficPolicy=Local 时应显示真实客户端IP

# 监控不均衡问题
kubectl get endpoints test-service -o jsonpath='{.subsets[*].addresses[*].ip}'
# Local策略下仅返回本节点Pod，可能导致负载不均
```

---

### 3. LoadBalancer - 云原生负载均衡

#### 架构价值
- **产品视角**: 生产环境唯一推荐的外部暴露方式，自动化、高可用
- **运维视角**: 一键对接云厂商LB，自动健康检查、流量切换
- **架构视角**: 支持多可用区容灾、自动弹性伸缩

#### 阿里云ACK完整配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: frontend-service
  namespace: production
  annotations:
    # === 基础配置 ===
    # 复用已有SLB(推荐，避免重复创建)
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "lb-bp1xxxxxxxx"
    
    # SLB规格(如果新建)
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s3.medium"
    
    # === 网络配置 ===
    # 内网/公网
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "internet"
    
    # 计费方式
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-charge-type: "paybytraffic"
    
    # === 安全配置 ===
    # 白名单(多个CIDR用逗号分隔)
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-acl-id: "acl-bp1xxxxxxxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-acl-type: "white"
    
    # === 健康检查 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/health"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-timeout: "5"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-healthy-threshold: "3"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-unhealthy-threshold: "2"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "2"
    
    # === 会话保持 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-persistence-timeout: "1800"
    
    # === 证书配置(HTTPS) ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cert-id: "cert-xxxxxxxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-protocol-port: "https:443"
    
    # === 后端配置 ===
    # 后端服务器权重
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-backend-label: "zone=cn-hangzhou-h"
    
    # 连接超时
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-connection-drain-timeout: "30"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local  # 保留源IP
  sessionAffinity: ClientIP
  ports:
    - name: https
      protocol: TCP
      port: 443
      targetPort: 8443
    - name: http
      protocol: TCP
      port: 80
      targetPort: 8080
  selector:
    app: frontend
    tier: web
```

#### 成本优化策略

| 策略 | 方法 | 节省 |
|------|------|------|
| **复用SLB** | 指定loadbalancer-id | 避免重复创建，节省80% |
| **按流量计费** | paybytraffic | 低流量业务节省50% |
| **共享带宽包** | 配置带宽包ID | 多SLB共享，节省60% |
| **Ingress替代** | 单SLB+Nginx Ingress | 多服务共享，节省90% |

---

### 4. Headless Service - 直连模式

#### 架构场景
- **产品视角**: 有状态服务(数据库、缓存)的标准配置
- **运维视角**: StatefulSet必需，支持DNS直接解析Pod IP
- **架构视角**: 服务发现不经过kube-proxy，性能最优

#### StatefulSet + Headless Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: mysql
  namespace: database
spec:
  clusterIP: None  # Headless标志
  selector:
    app: mysql
  ports:
    - name: mysql
      port: 3306
      targetPort: 3306
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: database
spec:
  serviceName: mysql  # 关联Headless Service
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          ports:
            - containerPort: 3306
              name: mysql
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 100Gi
```

#### DNS解析特性

```bash
# 普通ClusterIP Service DNS
backend-service.production.svc.cluster.local -> 10.96.100.50 (Service ClusterIP)

# Headless Service DNS
mysql.database.svc.cluster.local -> 10.244.1.5, 10.244.2.10, 10.244.3.15 (Pod IPs)

# StatefulSet Pod DNS
mysql-0.mysql.database.svc.cluster.local -> 10.244.1.5
mysql-1.mysql.database.svc.cluster.local -> 10.244.2.10
mysql-2.mysql.database.svc.cluster.local -> 10.244.3.15
```

**应用场景**:
- MySQL/PostgreSQL主从集群
- Redis Cluster分片
- Elasticsearch集群
- Kafka Broker集群
- Cassandra/MongoDB副本集

---

## Ingress 完整方案对比

### 方案选型决策树

```
是否需要南北向流量管理?
├─ 否 → 使用 ClusterIP + Service Mesh(Istio)
└─ 是 → 继续
    ├─ 使用云厂商K8s?
    │   ├─ ACK → ALB Ingress Controller (推荐)
    │   ├─ EKS → AWS Load Balancer Controller
    │   └─ GKE → GCE Ingress Controller
    └─ 自建/多云
        ├─ 简单HTTP路由 → Nginx Ingress Controller
        ├─ 复杂治理 → Gateway API + Envoy Gateway
        └─ 高性能场景 → Traefik / Caddy
```

### 1. Nginx Ingress Controller

#### 生产部署架构

```yaml
# Helm安装(高可用)
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm install ingress-nginx ingress-nginx/ingress-nginx \
  --namespace ingress-nginx --create-namespace \
  --set controller.replicaCount=3 \
  --set controller.resources.requests.cpu=1000m \
  --set controller.resources.requests.memory=1Gi \
  --set controller.service.type=LoadBalancer \
  --set controller.service.externalTrafficPolicy=Local \
  --set controller.metrics.enabled=true \
  --set controller.podAnnotations."prometheus\.io/scrape"=true
```

#### 企业级Ingress配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: production-ingress
  namespace: production
  annotations:
    # === 基础配置 ===
    kubernetes.io/ingress.class: nginx
    
    # === SSL配置 ===
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
    
    # === 速率限制 ===
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/limit-connections: "50"
    
    # === 客户端配置 ===
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    
    # === 白名单 ===
    nginx.ingress.kubernetes.io/whitelist-source-range: "10.0.0.0/8,172.16.0.0/12"
    
    # === CORS配置 ===
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://example.com"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, OPTIONS"
    
    # === 自定义响应头 ===
    nginx.ingress.kubernetes.io/configuration-snippet: |
      more_set_headers "X-Frame-Options: DENY";
      more_set_headers "X-Content-Type-Options: nosniff";
      more_set_headers "X-XSS-Protection: 1; mode=block";
    
    # === 金丝雀发布 ===
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"  # 10%流量到新版本
spec:
  tls:
    - hosts:
        - api.example.com
      secretName: api-tls-cert
  rules:
    - host: api.example.com
      http:
        paths:
          # API v2版本
          - path: /v2
            pathType: Prefix
            backend:
              service:
                name: backend-v2
                port:
                  number: 80
          # API v1版本(兼容)
          - path: /v1
            pathType: Prefix
            backend:
              service:
                name: backend-v1
                port:
                  number: 80
          # 默认路由
          - path: /
            pathType: Prefix
            backend:
              service:
                name: backend-v2
                port:
                  number: 80
```

#### 金丝雀发布完整流程

```yaml
# 步骤1: 生产Ingress(100%流量)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: production
spec:
  rules:
    - host: api.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: backend-v1
                port:
                  number: 80
---
# 步骤2: 创建Canary Ingress(10%流量到v2)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: production-canary
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"
spec:
  rules:
    - host: api.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: backend-v2
                port:
                  number: 80
---
# 步骤3: 观察监控指标，逐步提升权重
# 10% -> 30% -> 50% -> 100%

# 步骤4: 切换完成后删除Canary Ingress，更新生产Ingress
```

---

### 2. Gateway API (下一代标准)

#### 架构优势

| 维度 | Ingress | Gateway API |
|------|---------|-------------|
| **角色分离** | 单一配置 | 基础设施团队(Gateway) + 应用团队(HTTPRoute) |
| **可扩展性** | 注解驱动 | CRD原生，类型安全 |
| **多协议** | HTTP/HTTPS | HTTP/HTTPS/gRPC/TCP/UDP |
| **跨命名空间路由** | 不支持 | 原生支持 |
| **后端引用** | Service | Service/ServiceImport(多集群) |

#### 生产配置示例

```yaml
# 步骤1: 基础设施团队配置Gateway
apiVersion: gateway.networking.k8s.io/v1beta1
kind: GatewayClass
metadata:
  name: production-gateway-class
spec:
  controllerName: envoyproxy.io/gateway-controller
---
apiVersion: gateway.networking.k8s.io/v1beta1
kind: Gateway
metadata:
  name: production-gateway
  namespace: infra
spec:
  gatewayClassName: production-gateway-class
  listeners:
    - name: http
      protocol: HTTP
      port: 80
      allowedRoutes:
        namespaces:
          from: All  # 允许所有命名空间的应用路由
    - name: https
      protocol: HTTPS
      port: 443
      tls:
        certificateRefs:
          - kind: Secret
            name: wildcard-tls
      allowedRoutes:
        namespaces:
          from: All
---
# 步骤2: 应用团队配置HTTPRoute
apiVersion: gateway.networking.k8s.io/v1beta1
kind: HTTPRoute
metadata:
  name: backend-route
  namespace: production
spec:
  parentRefs:
    - name: production-gateway
      namespace: infra
  hostnames:
    - "api.example.com"
  rules:
    - matches:
        - path:
            type: PathPrefix
            value: /api/v2
      backendRefs:
        - name: backend-v2
          port: 80
          weight: 90
        - name: backend-v2-canary
          port: 80
          weight: 10  # 金丝雀10%流量
      filters:
        - type: RequestHeaderModifier
          requestHeaderModifier:
            add:
              - name: X-Gateway
                value: production
```

---

### 3. ALB Ingress Controller (ACK专属)

#### 架构特点
- **深度集成**: ALB原生支持，性能无损耗
- **智能路由**: 基于Header/Cookie的灰度发布
- **成本优化**: 单ALB承载多服务，节省90%成本
- **安全增强**: WAF/DDoS防护一键开启

#### ACK ALB完整配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: alb-ingress
  namespace: production
  annotations:
    # === ALB基础配置 ===
    kubernetes.io/ingress.class: alb
    
    # 复用已有ALB
    alb.ingress.kubernetes.io/load-balancer-id: "alb-xxxxxxxx"
    
    # ALB规格(如果新建)
    alb.ingress.kubernetes.io/load-balancer-edition: "Standard"
    
    # === 监听器配置 ===
    alb.ingress.kubernetes.io/listen-ports: '[{"HTTP": 80}, {"HTTPS": 443}]'
    alb.ingress.kubernetes.io/cert-id: "cert-xxxxxxxx"
    
    # === 后端配置 ===
    alb.ingress.kubernetes.io/backend-protocol: "HTTP"
    alb.ingress.kubernetes.io/healthcheck-enabled: "true"
    alb.ingress.kubernetes.io/healthcheck-path: "/health"
    alb.ingress.kubernetes.io/healthcheck-protocol: "HTTP"
    alb.ingress.kubernetes.io/healthcheck-timeout-seconds: "5"
    alb.ingress.kubernetes.io/healthcheck-interval-seconds: "2"
    alb.ingress.kubernetes.io/healthy-threshold-count: "3"
    alb.ingress.kubernetes.io/unhealthy-threshold-count: "2"
    
    # === 流量策略 ===
    alb.ingress.kubernetes.io/traffic-limit-qps: "1000"
    
    # === 基于Header的灰度 ===
    alb.ingress.kubernetes.io/canary: "true"
    alb.ingress.kubernetes.io/canary-by-header: "X-Canary-Version"
    alb.ingress.kubernetes.io/canary-by-header-value: "v2"
spec:
  rules:
    - host: api.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: backend
                port:
                  number: 80
```

---

## CNI 插件深度对比

### CNI选型矩阵

| CNI | 网络模型 | 性能 | NetworkPolicy | 跨主机通信 | ACK推荐 | 适用规模 |
|-----|---------|------|---------------|-----------|---------|---------|
| **Terway** | VPC路由/ENI | ⭐⭐⭐⭐⭐ | ✅ (Cilium后端) | VPC原生 | ✅ 强烈推荐 | 大规模生产 |
| **Flannel** | VXLAN/host-gw | ⭐⭐⭐ | ❌ (需Calico补充) | 封装/路由 | ❌ | 测试环境 |
| **Calico** | BGP/IPIP | ⭐⭐⭐⭐ | ✅ (原生强大) | BGP路由 | ⚠️ 特定场景 | 中大规模 |
| **Cilium** | eBPF | ⭐⭐⭐⭐⭐ | ✅ (L3-L7) | VXLAN/Geneve | ✅ 新集群 | 未来趋势 |
| **Weave Net** | UDP封装 | ⭐⭐ | ✅ | 封装 | ❌ | 已停维护 |

---

### 1. Terway (ACK生产标准)

#### 架构模式

```
┌─────────────────────────────────────────┐
│           Terway架构图                   │
│                                         │
│  ┌───────────────────────────────────┐ │
│  │  Pod (ENI模式)                    │ │
│  │  ┌─────────────────────────────┐  │ │
│  │  │  eth0: 172.16.1.10          │  │ │
│  │  │  (直接分配VPC ENI)           │  │ │
│  │  └─────────────────────────────┘  │ │
│  │         │                          │ │
│  │         v                          │ │
│  │  ┌─────────────────────────────┐  │ │
│  │  │  Node ENI池                  │  │ │
│  │  │  (预分配弹性网卡)            │  │ │
│  │  └─────────────────────────────┘  │ │
│  └───────────────────────────────────┘ │
│                                         │
│  ┌───────────────────────────────────┐ │
│  │  Pod (ENIIP模式)                  │ │
│  │  ┌─────────────────────────────┐  │ │
│  │  │  eth0: 172.16.1.20          │  │ │
│  │  │  (共享ENI的辅助IP)           │  │ │
│  │  └─────────────────────────────┘  │ │
│  │         │                          │ │
│  │         v                          │ │
│  │  ┌─────────────────────────────┐  │ │
│  │  │  veth pair + bridge         │  │ │
│  │  └─────────────────────────────┘  │ │
│  └───────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

#### 模式对比

| 模式 | Pod IP来源 | 性能 | 节点Pod密度 | 安全组 | 适用场景 |
|------|-----------|------|------------|--------|---------|
| **ENI独占** | 独立弹性网卡 | 无损耗(等同ECS) | 低(受ENI配额限制) | Pod级别 | 数据库、缓存等高性能应用 |
| **ENI-IP共享** | ENI辅助IP | 轻微损耗(<5%) | 高(突破ENI限制) | 节点级别 | Web应用、微服务 |
| **VPC路由** | VPC路由表 | 中等(经过veth) | 中等 | 节点级别 | 混合场景 |

#### 生产配置

```yaml
# Terway ConfigMap (ACK默认)
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "max_pool_size": 25,  # ENI预热池大小
      "min_pool_size": 10,  # 最小保留ENI数
      "eni_tags": {
        "app": "production"
      },
      "security_group": "sg-xxxxxx",
      "vswitches": {
        "cn-hangzhou-h": ["vsw-xxxxxx"],
        "cn-hangzhou-i": ["vsw-yyyyyy"]
      }
    }
```

#### 性能测试数据

| 指标 | ENI模式 | ENIIP模式 | Flannel VXLAN |
|------|---------|----------|---------------|
| **带宽(Gbps)** | 9.8 | 9.5 | 8.3 |
| **延迟(ms)** | 0.15 | 0.18 | 0.45 |
| **PPS(万)** | 120 | 110 | 85 |
| **CPU开销(%)** | 2 | 5 | 15 |

---

### 2. Cilium (eBPF新标准)

#### 核心优势
- **性能极致**: eBPF内核旁路，无需iptables规则
- **可观测性**: Hubble提供L3-L7可视化
- **安全增强**: 零信任网络，API级别策略

#### 安装部署

```bash
# Helm安装
helm repo add cilium https://helm.cilium.io/
helm install cilium cilium/cilium --version 1.14.5 \
  --namespace kube-system \
  --set kubeProxyReplacement=strict \
  --set k8sServiceHost=<KUBE_API_SERVER_IP> \
  --set k8sServicePort=6443 \
  --set hubble.relay.enabled=true \
  --set hubble.ui.enabled=true
```

#### NetworkPolicy L7示例

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-policy
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: backend
  ingress:
    - fromEndpoints:
        - matchLabels:
            app: frontend
      toPorts:
        - ports:
            - port: "80"
              protocol: TCP
          rules:
            http:
              - method: "GET"
                path: "/api/v2/.*"
              - method: "POST"
                path: "/api/v2/orders"
                headers:
                  - "X-API-Key: .*"
```

---

## CoreDNS 深度优化

### 架构配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
            lameduck 5s
        }
        ready
        
        # Kubernetes插件
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        
        # Prometheus监控
        prometheus :9153
        
        # 缓存(关键优化)
        cache 30
        
        # 循环检测
        loop
        
        # 自动重载
        reload
        
        # 负载均衡
        loadbalance
        
        # 上游DNS
        forward . /etc/resolv.conf {
            max_concurrent 1000
            expire 10s
            policy random
        }
    }
    
    # 自定义域名
    company.internal:53 {
        errors
        cache 300
        forward . 10.0.0.1 10.0.0.2
    }
```

### 性能优化

| 优化项 | 配置 | 效果 |
|-------|------|------|
| **缓存** | `cache 30` | 减少90%重复查询 |
| **NodeLocal DNS** | DaemonSet部署 | 延迟降低70% |
| **并发限制** | `max_concurrent 1000` | 防止DNS洪水 |
| **自动扩容** | HPA配置 | 高峰平稳 |

### NodeLocal DNS Cache

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-local-dns
  namespace: kube-system
spec:
  selector:
    matchLabels:
      k8s-app: node-local-dns
  template:
    metadata:
      labels:
        k8s-app: node-local-dns
    spec:
      priorityClassName: system-node-critical
      hostNetwork: true
      dnsPolicy: Default
      containers:
        - name: node-cache
          image: registry.k8s.io/dns/k8s-dns-node-cache:1.22.20
          args:
            - -localip
            - 169.254.20.10  # 本地缓存IP
            - -conf
            - /etc/coredns/Corefile
          resources:
            requests:
              cpu: 25m
              memory: 25Mi
```

**效果**:
- DNS查询延迟从 10ms → 1ms
- CoreDNS负载降低 80%
- 避免conntrack表溢出

---

## 网络策略完整实践

### 零信任网络模型

```yaml
# 步骤1: 默认拒绝所有入口流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
    - Ingress
---
# 步骤2: 仅允许frontend访问backend
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
    - Ingress
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: frontend
      ports:
        - protocol: TCP
          port: 8080
---
# 步骤3: 允许backend访问数据库
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-backend-to-db
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: mysql
  policyTypes:
    - Ingress
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: backend
      ports:
        - protocol: TCP
          port: 3306
---
# 步骤4: 允许所有Pod访问DNS
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns-access
  namespace: production
spec:
  podSelector: {}
  policyTypes:
    - Egress
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              name: kube-system
      ports:
        - protocol: UDP
          port: 53
```

---

## 网络监控与诊断

### Prometheus指标

```yaml
# ServiceMonitor for CoreDNS
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: coredns
  namespace: kube-system
spec:
  selector:
    matchLabels:
      k8s-app: kube-dns
  endpoints:
    - port: metrics
      interval: 30s
---
# 关键告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: network-alerts
spec:
  groups:
    - name: network
      rules:
        - alert: HighDNSErrorRate
          expr: |
            rate(coredns_dns_response_rcode_count_total{rcode="SERVFAIL"}[5m]) > 10
          for: 5m
          annotations:
            summary: "DNS错误率过高"
        
        - alert: ServiceEndpointDown
          expr: |
            kube_endpoint_address_available == 0
          for: 5m
          annotations:
            summary: "Service无可用Endpoint"
```

### 网络诊断工具

```bash
# 1. 查看Service Endpoints
kubectl get endpoints -A

# 2. 测试DNS解析
kubectl run -it --rm debug --image=nicolaka/netshoot --restart=Never -- nslookup backend-service.production.svc.cluster.local

# 3. 测试网络连通性
kubectl run -it --rm debug --image=nicolaka/netshoot --restart=Never -- curl -v http://backend-service.production.svc.cluster.local

# 4. 抓包分析
kubectl debug node/node-1 -it --image=nicolaka/netshoot -- tcpdump -i any -w /tmp/capture.pcap port 53

# 5. 查看iptables规则(Service实现)
kubectl debug node/node-1 -it --image=nicolaka/netshoot -- iptables-save | grep backend-service
```

---

## 生产环境网络运维最佳实践

### 高可用网络架构设计

```yaml
# 生产环境网络架构推荐配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: production-network-config
  namespace: kube-system
data:
  # 网络插件高可用配置
  cni-ha-config: |
    {
      "name": "k8s-pod-network",
      "cniVersion": "0.3.1",
      "plugins": [
        {
          "type": "calico",
          "log_level": "info",
          "nodename": "__KUBERNETES_NODE_NAME__",
          "mtu": 1440,
          "ipam": {
            "type": "calico-ipam",
            "assign_ipv4": "true",
            "assign_ipv6": "false"
          },
          "kubernetes": {
            "kubeconfig": "/etc/cni/net.d/calico-kubeconfig"
          },
          # 高可用配置
          "health_checks": {
            "enabled": true,
            "interval": "5s",
            "timeout": "3s"
          }
        }
      ]
    }
  
  # kube-proxy 高性能配置
  kube-proxy-config: |
    apiVersion: kubeproxy.config.k8s.io/v1alpha1
    kind: KubeProxyConfiguration
    mode: "ipvs"
    ipvs:
      scheduler: "rr"
      syncPeriod: "30s"
      minSyncPeriod: "5s"
      excludeCIDRs:
        - "10.0.0.0/8"
    conntrack:
      maxPerCore: 32768
      min: 131072
    clientConnection:
      kubeconfig: "/var/lib/kube-proxy/kubeconfig"
```

### 网络故障应急响应预案

```bash
#!/bin/bash
# network-emergency-response.sh - 生产环境网络故障应急脚本

set -euo pipefail

CLUSTER_NAME="${CLUSTER_NAME:-production}"
LOG_DIR="/var/log/network-emergency-$(date +%Y%m%d-%H%M%S)"

echo "=== Kubernetes 网络紧急诊断工具 ==="
echo "Cluster: ${CLUSTER_NAME}"
echo "Time: $(date)"
echo "Log Directory: ${LOG_DIR}"
echo

mkdir -p "${LOG_DIR}"

# 1. 基础网络连通性检查
echo "[1/8] 检查基础网络连通性..."
{
  echo "=== 基础网络检查 ==="
  echo "时间: $(date)"
  echo "节点网络状态:"
  kubectl get nodes -o wide
  
  echo -e "\nPod网络状态:"
  kubectl get pods -A -o wide | grep -E "(Running|Error|CrashLoopBackOff)"
  
  echo -e "\nService状态:"
  kubectl get svc -A --no-headers | wc -l
} > "${LOG_DIR}/basic-check.log"

# 2. CNI插件状态检查
echo "[2/8] 检查CNI插件状态..."
{
  echo "=== CNI插件检查 ==="
  echo "时间: $(date)"
  
  # 检查Calico状态
  if kubectl get pods -n kube-system | grep -q calico; then
    echo "检测到Calico CNI"
    kubectl get pods -n kube-system | grep calico-node
    kubectl exec -n kube-system -l k8s-app=calico-node -- calicoctl node status 2>/dev/null || echo "calicoctl不可用"
  fi
  
  # 检查Cilium状态
  if kubectl get pods -n kube-system | grep -q cilium; then
    echo "检测到Cilium CNI"
    kubectl get pods -n kube-system | grep cilium
    kubectl exec -n kube-system -l k8s-app=cilium -- cilium status 2>/dev/null || echo "cilium命令不可用"
  fi
} > "${LOG_DIR}/cni-status.log"

# 3. kube-proxy状态检查
echo "[3/8] 检查kube-proxy状态..."
{
  echo "=== kube-proxy检查 ==="
  echo "时间: $(date)"
  
  echo "kube-proxy Pod状态:"
  kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide
  
  echo -e "\nkube-proxy配置:"
  kubectl get cm -n kube-system kube-proxy -o yaml | grep -A 20 "mode:"
  
  echo -e "\nkube-proxy日志采样:"
  kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50
} > "${LOG_DIR}/kube-proxy-status.log"

# 4. DNS服务检查
echo "[4/8] 检查DNS服务..."
{
  echo "=== DNS服务检查 ==="
  echo "时间: $(date)"
  
  echo "CoreDNS Pod状态:"
  kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
  
  echo -e "\nDNS配置:"
  kubectl get cm -n kube-system coredns -o yaml
  
  echo -e "\nDNS解析测试:"
  kubectl run --rm -it dns-test --image=busybox --restart=Never -- \
    nslookup kubernetes.default 2>&1
  
  echo -e "\n外部DNS测试:"
  kubectl run --rm -it dns-test2 --image=busybox --restart=Never -- \
    nslookup google.com 2>&1
} > "${LOG_DIR}/dns-check.log"

# 5. Service和Endpoints检查
echo "[5/8] 检查Service和Endpoints..."
{
  echo "=== Service和Endpoints检查 ==="
  echo "时间: $(date)"
  
  echo "异常Service列表:"
  kubectl get svc -A --no-headers | awk '$5=="<none>" {print $1"/"$2}'
  
  echo -e "\nEndpoints状态统计:"
  kubectl get endpoints -A --no-headers | awk '{print $NF}' | sort | uniq -c
  
  echo -e "\n无Endpoints的Service:"
  kubectl get svc -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\t"}{.spec.clusterIP}{"\t"}{.status.loadBalancer.ingress[*].ip}{"\n"}{end}' | \
    while read ns_svc clusterip lbip; do
      ep_count=$(kubectl get endpoints -n ${ns_svc%/*} ${ns_svc#*/} -o jsonpath='{.subsets[*].addresses[*]}' 2>/dev/null | wc -w)
      if [ "$ep_count" -eq 0 ] && [ "$clusterip" != "<none>" ]; then
        echo "$ns_svc - 无可用Endpoints"
      fi
    done
} > "${LOG_DIR}/service-endpoints-check.log"

# 6. 网络策略检查
echo "[6/8] 检查网络策略..."
{
  echo "=== 网络策略检查 ==="
  echo "时间: $(date)"
  
  echo "NetworkPolicy统计:"
  kubectl get networkpolicy -A --no-headers | wc -l
  
  echo -e "\n各命名空间策略数量:"
  kubectl get networkpolicy -A --no-headers | awk '{print $1}' | sort | uniq -c
  
  echo -e "\n阻断策略示例:"
  kubectl get networkpolicy -A -o wide | head -10
} > "${LOG_DIR}/networkpolicy-check.log"

# 7. 性能指标采集
echo "[7/8] 采集网络性能指标..."
{
  echo "=== 网络性能指标 ==="
  echo "时间: $(date)"
  
  echo "节点网络接口统计:"
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | \
    while read node; do
      echo "--- Node: $node ---"
      kubectl debug node/$node -it --image=nicolaka/netshoot -- ss -i 2>/dev/null | head -20
    done
  
  echo -e "\nconntrack统计:"
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | \
    while read node; do
      echo "--- Node: $node ---"
      kubectl debug node/$node -it --image=nicolaka/netshoot -- \
        cat /proc/sys/net/netfilter/nf_conntrack_count 2>/dev/null || echo "无法读取"
    done
} > "${LOG_DIR}/performance-metrics.log"

# 8. 生成诊断报告
echo "[8/8] 生成诊断报告..."
{
  echo "=========================================="
  echo "        Kubernetes网络诊断报告"
  echo "=========================================="
  echo "集群名称: ${CLUSTER_NAME}"
  echo "诊断时间: $(date)"
  echo "报告目录: ${LOG_DIR}"
  echo
  
  echo "=== 摘要信息 ==="
  echo "总节点数: $(kubectl get nodes --no-headers | wc -l)"
  echo "运行中Pod数: $(kubectl get pods -A --no-headers | grep Running | wc -l)"
  echo "异常Pod数: $(kubectl get pods -A --no-headers | grep -E "(Error|CrashLoopBackOff|Pending)" | wc -l)"
  echo "Service总数: $(kubectl get svc -A --no-headers | wc -l)"
  echo "无Endpoints服务数: $(grep -c "无可用Endpoints" ${LOG_DIR}/service-endpoints-check.log 2>/dev/null || echo 0)"
  echo
  
  echo "=== 关键发现 ==="
  if [ "$(grep -c "SERVFAIL\|NXDOMAIN" ${LOG_DIR}/dns-check.log 2>/dev/null || echo 0)" -gt 0 ]; then
    echo "⚠️  DNS解析存在问题"
  fi
  
  if [ "$(grep -c "CrashLoopBackOff\|Error" ${LOG_DIR}/basic-check.log 2>/dev/null || echo 0)" -gt 5 ]; then
    echo "⚠️  大量Pod处于异常状态"
  fi
  
  if [ "$(grep -c "无可用Endpoints" ${LOG_DIR}/service-endpoints-check.log 2>/dev/null || echo 0)" -gt 0 ]; then
    echo "⚠️  存在Service无Endpoints的情况"
  fi
  
  echo
  echo "=== 建议操作 ==="
  echo "1. 详细日志已保存至: ${LOG_DIR}"
  echo "2. 如需人工分析，请查看各*.log文件"
  echo "3. 紧急情况下可联系SRE团队"
} > "${LOG_DIR}/diagnosis-report.txt"

echo
echo "=== 诊断完成 ==="
echo "详细报告位置: ${LOG_DIR}"
echo "请查看 ${LOG_DIR}/diagnosis-report.txt 获取摘要信息"
echo
echo "常见问题处理建议:"
echo "1. DNS问题: 重启CoreDNS Pod"
echo "2. 网络插件问题: 检查CNI配置和节点状态"
echo "3. Service问题: 检查Endpoints和后端Pod状态"
```

### 网络容量规划指南

```markdown
## 生产环境网络容量规划

### 1. IP地址规划
- **Pod CIDR**: 建议至少 /16 (65,534个地址)
- **Service CIDR**: 建议至少 /17 (32,768个地址)
- **节点CIDR**: 根据实际节点数量规划

### 2. 性能基准
- **单节点Pod密度**: 不超过110个Pod/节点
- **网络带宽**: 每节点至少1Gbps
- **连接数**: 每节点支持至少50,000并发连接

### 3. 监控阈值设置
- **CPU使用率**: >80% 触发告警
- **内存使用率**: >85% 触发告警
- **网络丢包率**: >0.1% 触发告警
- **DNS错误率**: >1% 触发告警

### 4. 故障恢复时间目标
- **RTO (恢复时间目标)**: <5分钟
- **RPO (恢复点目标)**: <1分钟
- **可用性目标**: 99.95%
```

## 生产最佳实践清单

### Service配置

- ✅ 使用`externalTrafficPolicy: Local`保留源IP
- ✅ 启用`topologyKeys`优化跨AZ延迟
- ✅ 配置`sessionAffinity`适配应用特性
- ✅ LoadBalancer类型复用已有云LB
- ✅ 配置健康检查端点和阈值
- ✅ 添加Prometheus监控注解

### Ingress配置

- ✅ 强制HTTPS重定向
- ✅ 配置速率限制防止DDoS
- ✅ 启用CORS和安全响应头
- ✅ 使用cert-manager自动化证书
- ✅ 金丝雀发布验证新版本
- ✅ 配置IP白名单保护敏感接口

### CNI选择

- ✅ ACK环境强制使用Terway
- ✅ 高性能场景选择ENI模式
- ✅ 高密度场景选择ENIIP模式
- ✅ 新集群考虑Cilium(eBPF)
- ✅ 启用NetworkPolicy实施零信任

### DNS优化

- ✅ 部署NodeLocal DNS Cache
- ✅ 配置合理的缓存TTL
- ✅ CoreDNS启用HPA自动扩容
- ✅ 监控DNS错误率和延迟
- ✅ 自定义域名使用专用配置

---

**表格维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)
