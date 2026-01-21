# Kubernetes Service 核心概念与类型深度解析 (Service Concepts & Types Deep Dive)

> **适用版本**: Kubernetes v1.25 - v1.32  
> **文档版本**: v3.0 | 生产级 Service 完整配置参考  
> **最后更新**: 2026-01  
> **参考文档**: [kubernetes.io/docs/concepts/services-networking/service](https://kubernetes.io/docs/concepts/services-networking/service/)

---

## 1. Service 核心架构 (Architecture Overview)

### 1.1 整体架构图

```
┌─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                              Kubernetes Service Architecture                                     │
├─────────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                                  │
│  ┌────────────────────────────────────────────────────────────────────────────────────────────┐ │
│  │                                   Client Layer                                              │ │
│  │                                                                                             │ │
│  │  ┌───────────────────┐  ┌───────────────────┐  ┌───────────────────┐  ┌─────────────────┐ │ │
│  │  │  Internal Client  │  │  External Client  │  │   DNS Client      │  │  Service Mesh   │ │ │
│  │  │  (Pod in Cluster) │  │  (Browser/CLI)    │  │   Resolution      │  │  (Istio/Linkerd)│ │ │
│  │  │                   │  │                   │  │                   │  │                 │ │ │
│  │  │  ClusterIP:80     │  │  NodePort:30080   │  │  svc.ns.svc       │  │  VirtualService │ │ │
│  │  │  svc-name:port    │  │  LoadBalancer IP  │  │  cluster.local    │  │  DestinationRule│ │ │
│  │  └─────────┬─────────┘  └─────────┬─────────┘  └─────────┬─────────┘  └────────┬────────┘ │ │
│  │            │                      │                      │                     │          │ │
│  └────────────┼──────────────────────┼──────────────────────┼─────────────────────┼──────────┘ │
│               │                      │                      │                     │            │
│               ▼                      ▼                      ▼                     ▼            │
│  ┌────────────────────────────────────────────────────────────────────────────────────────────┐ │
│  │                                Service Layer (API Server)                                   │ │
│  │                                                                                             │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────────────┐  │ │
│  │  │                              Service Object (core/v1)                                │  │ │
│  │  │                                                                                      │  │ │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────┐ │  │ │
│  │  │  │ ClusterIP   │  │ NodePort    │  │LoadBalancer │  │ExternalName │  │ Headless  │ │  │ │
│  │  │  │             │  │             │  │             │  │             │  │           │ │  │ │
│  │  │  │ 10.96.x.x   │  │ 30000-32767 │  │ Cloud LB IP │  │ CNAME DNS   │  │ None      │ │  │ │
│  │  │  │ Internal    │  │ Node-level  │  │ External    │  │ External    │  │ Direct IP │ │  │ │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  └───────────┘ │  │ │
│  │  └─────────────────────────────────────────────────────────────────────────────────────┘  │ │
│  │                                           │                                                │ │
│  │                                           │ selector: app=myapp                           │ │
│  │                                           ▼                                                │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────────────┐  │ │
│  │  │                    EndpointSlice (discovery.k8s.io/v1)                               │  │ │
│  │  │                                                                                      │  │ │
│  │  │  endpoints:                                                                          │  │ │
│  │  │    - addresses: [10.244.1.10]  conditions: {ready: true, serving: true}             │  │ │
│  │  │    - addresses: [10.244.2.11]  conditions: {ready: true, serving: true}             │  │ │
│  │  │    - addresses: [10.244.3.12]  conditions: {ready: false, terminating: true}        │  │ │
│  │  │  ports: [{name: http, port: 8080, protocol: TCP, appProtocol: http}]               │  │ │
│  │  └─────────────────────────────────────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────────────────────────────────────┘ │
│                                           │                                                     │
│                                           │ Watch & Sync (Informer)                            │
│                                           ▼                                                     │
│  ┌────────────────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              kube-proxy / CNI Layer                                         │ │
│  │                                                                                             │ │
│  │  ┌──────────────────────┐  ┌──────────────────────┐  ┌──────────────────────┐            │ │
│  │  │       Node 1         │  │       Node 2         │  │       Node 3         │            │ │
│  │  │                      │  │                      │  │                      │            │ │
│  │  │  ┌────────────────┐ │  │  ┌────────────────┐ │  │  ┌────────────────┐ │            │ │
│  │  │  │   kube-proxy   │ │  │  │   kube-proxy   │ │  │  │   kube-proxy   │ │            │ │
│  │  │  │ ┌────────────┐ │ │  │  │ ┌────────────┐ │ │  │  │ ┌────────────┐ │ │            │ │
│  │  │  │ │  iptables  │ │ │  │  │ │    IPVS    │ │ │  │  │ │  nftables  │ │ │            │ │
│  │  │  │ │  (legacy)  │ │ │  │  │ │ (recommend)│ │ │  │  │ │  (v1.29+)  │ │ │            │ │
│  │  │  │ └────────────┘ │ │  │  │ └────────────┘ │ │  │  │ └────────────┘ │ │            │ │
│  │  │  └────────────────┘ │  │  └────────────────┘ │  │  └────────────────┘ │            │ │
│  │  │                      │  │                      │  │                      │            │ │
│  │  │  Alternative: Cilium eBPF / Calico eBPF (无kube-proxy模式)                          │ │
│  │  └──────────────────────┘  └──────────────────────┘  └──────────────────────┘            │ │
│  └────────────────────────────────────────────────────────────────────────────────────────────┘ │
│                                           │                                                     │
│                                           │ Traffic Forwarding (NAT/DNAT)                      │
│                                           ▼                                                     │
│  ┌────────────────────────────────────────────────────────────────────────────────────────────┐ │
│  │                                   Pod Layer                                                 │ │
│  │                                                                                             │ │
│  │  ┌──────────────────────┐  ┌──────────────────────┐  ┌──────────────────────┐            │ │
│  │  │       Pod 1          │  │       Pod 2          │  │       Pod 3          │            │ │
│  │  │   10.244.1.10:8080   │  │   10.244.2.11:8080   │  │   10.244.3.12:8080   │            │ │
│  │  │                      │  │                      │  │                      │            │ │
│  │  │  ┌────────────────┐ │  │  ┌────────────────┐ │  │  ┌────────────────┐ │            │ │
│  │  │  │   Container    │ │  │  │   Container    │ │  │  │   Container    │ │            │ │
│  │  │  │ readiness: OK  │ │  │  │ readiness: OK  │ │  │  │ terminating    │ │            │ │
│  │  │  └────────────────┘ │  │  └────────────────┘ │  │  └────────────────┘ │            │ │
│  │  └──────────────────────┘  └──────────────────────┘  └──────────────────────┘            │ │
│  └────────────────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 核心组件职责

| 组件 | 英文名 | 职责 | 关键特性 | 版本说明 |
|:---|:---|:---|:---|:---|
| **Service** | Service | 为 Pod 提供稳定访问入口 | 虚拟IP、负载均衡、服务发现 | 所有版本 |
| **Endpoints** | Endpoints | 存储后端 Pod IP 列表 | 自动更新、Watch 机制 | 所有版本(推荐用EndpointSlice) |
| **EndpointSlice** | EndpointSlice | 分片存储端点信息 | 大规模高效、拓扑感知 | v1.21+ GA |
| **kube-proxy** | kube-proxy | 实现 Service 流量转发 | iptables/IPVS/nftables | 所有版本 |
| **CoreDNS** | CoreDNS | 服务名称解析 | A/AAAA/SRV 记录 | v1.13+ 默认 |
| **EndpointSlice Controller** | EndpointSlice Controller | 管理 EndpointSlice 生命周期 | 自动创建、更新、清理 | v1.19+ |
| **Service Controller** | Service Controller | 管理 LoadBalancer 类型 | 云控制器集成 | 所有版本 |

### 1.3 Service 工作流程

```
┌──────────────────────────────────────────────────────────────────────────────────────────┐
│                            Service 创建与流量转发流程                                      │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                           │
│  1. Service 创建                                                                          │
│  ┌─────────────────────────────────────────────────────────────────────────────────────┐ │
│  │  kubectl apply -f service.yaml                                                       │ │
│  │         │                                                                            │ │
│  │         ▼                                                                            │ │
│  │  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────────────────┐ │ │
│  │  │   API Server    │───▶│      etcd       │    │  Service Controller (KCM)       │ │ │
│  │  │  - 验证 Service │    │  - 存储 Service │    │  - 分配 ClusterIP               │ │ │
│  │  │  - 分配 ClusterIP│    │  - 持久化       │    │  - LoadBalancer: 调用云 API     │ │ │
│  │  └─────────────────┘    └─────────────────┘    └─────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                           │
│  2. EndpointSlice 同步                                                                   │
│  ┌─────────────────────────────────────────────────────────────────────────────────────┐ │
│  │  ┌─────────────────────────────┐    ┌─────────────────────────────────────────────┐ │ │
│  │  │  EndpointSlice Controller   │    │           EndpointSlice                     │ │ │
│  │  │                             │    │                                             │ │ │
│  │  │  - Watch Service selector   │───▶│  - 记录匹配 Pod 的 IP/Port                  │ │ │
│  │  │  - Watch Pod (Ready)        │    │  - conditions: ready/serving/terminating   │ │ │
│  │  │  - 更新 EndpointSlice       │    │  - 最多 100 个 endpoint/slice              │ │ │
│  │  └─────────────────────────────┘    └─────────────────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                           │
│  3. kube-proxy 同步规则                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────────────────┐ │
│  │  ┌─────────────────────────────┐    ┌─────────────────────────────────────────────┐ │ │
│  │  │       kube-proxy            │    │         Linux Kernel                        │ │ │
│  │  │                             │    │                                             │ │ │
│  │  │  - Watch Service            │    │  iptables 模式:                             │ │ │
│  │  │  - Watch EndpointSlice      │───▶│    KUBE-SERVICES → KUBE-SVC-xxx → KUBE-SEP  │ │ │
│  │  │  - 生成转发规则             │    │  IPVS 模式:                                 │ │ │
│  │  │                             │    │    ipvsadm -A -t ClusterIP:port -s rr       │ │ │
│  │  └─────────────────────────────┘    └─────────────────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                           │
│  4. 流量转发                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────────────────────┐ │
│  │  Client Pod          kube-proxy Rules           Backend Pod                         │ │
│  │  ┌────────┐         ┌────────────────┐         ┌────────────┐                      │ │
│  │  │        │ ──────▶ │  DNAT:         │ ──────▶ │            │                      │ │
│  │  │ curl   │  dst:   │  ClusterIP:80  │  dst:   │ Container  │                      │ │
│  │  │ svc:80 │ 10.96.  │  → PodIP:8080  │ 10.244. │  :8080     │                      │ │
│  │  └────────┘  0.100  └────────────────┘  1.10   └────────────┘                      │ │
│  └─────────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                           │
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Service 字段完整参考 (Field Reference)

### 2.1 spec 核心字段

| 字段 | 类型 | 必填 | 默认值 | 描述 | 版本 |
|:---|:---|:---:|:---|:---|:---|
| `spec.type` | string | 否 | ClusterIP | Service 类型 | 所有 |
| `spec.selector` | map[string]string | 否 | - | 标签选择器，关联后端 Pod | 所有 |
| `spec.ports` | []ServicePort | 是 | - | 端口配置列表 | 所有 |
| `spec.clusterIP` | string | 否 | 自动分配 | 指定 ClusterIP，"None" 为 Headless | 所有 |
| `spec.clusterIPs` | []string | 否 | 自动分配 | 双栈 IP 列表 | v1.20+ |
| `spec.externalIPs` | []string | 否 | - | 外部 IP 列表 | 所有 |
| `spec.sessionAffinity` | string | 否 | None | 会话亲和性 (None/ClientIP) | 所有 |
| `spec.sessionAffinityConfig` | SessionAffinityConfig | 否 | - | 会话亲和性配置 | v1.7+ |
| `spec.externalName` | string | 条件 | - | ExternalName 类型必填 | 所有 |
| `spec.externalTrafficPolicy` | string | 否 | Cluster | 外部流量策略 (Cluster/Local) | v1.7+ |
| `spec.internalTrafficPolicy` | string | 否 | Cluster | 内部流量策略 (Cluster/Local) | v1.26+ GA |
| `spec.healthCheckNodePort` | int32 | 否 | 自动分配 | externalTrafficPolicy=Local 时健康检查端口 | v1.7+ |
| `spec.publishNotReadyAddresses` | bool | 否 | false | 发布未就绪 Pod 地址 | v1.9+ |
| `spec.ipFamilyPolicy` | string | 否 | SingleStack | IP 协议族策略 | v1.20+ |
| `spec.ipFamilies` | []string | 否 | [IPv4] | IP 协议族列表 | v1.20+ |
| `spec.allocateLoadBalancerNodePorts` | bool | 否 | true | 是否为 LB 分配 NodePort | v1.24+ |
| `spec.loadBalancerClass` | string | 否 | - | LoadBalancer 控制器类 | v1.24+ GA |
| `spec.loadBalancerIP` | string | 否 | - | 指定 LB IP (已弃用) | 弃用 |
| `spec.loadBalancerSourceRanges` | []string | 否 | - | LB 允许的源 CIDR | 所有 |
| `spec.trafficDistribution` | string | 否 | - | 流量分布策略 | v1.30+ Alpha |

### 2.2 ServicePort 字段

| 字段 | 类型 | 必填 | 描述 | 示例 |
|:---|:---|:---:|:---|:---|
| `name` | string | 多端口时必填 | 端口名称 (DNS_LABEL) | http, grpc, metrics |
| `protocol` | string | 否 | 协议 (TCP/UDP/SCTP) | TCP |
| `port` | int32 | 是 | Service 端口 | 80, 443, 9090 |
| `targetPort` | IntOrString | 否 | Pod 端口 (数字或名称) | 8080, "http" |
| `nodePort` | int32 | 否 | NodePort 端口 (30000-32767) | 30080 |
| `appProtocol` | string | 否 | 应用层协议 (v1.20+ GA) | http, https, grpc, h2c |

### 2.3 ipFamilyPolicy 详解

| 值 | 描述 | 行为 | 适用场景 |
|:---|:---|:---|:---|
| `SingleStack` | 单栈模式 | 仅分配一个 IP 族 | 默认，IPv4 或 IPv6 单一环境 |
| `PreferDualStack` | 优先双栈 | 如果集群支持则分配双栈 | 混合环境，兼容性优先 |
| `RequireDualStack` | 要求双栈 | 必须分配双栈，否则失败 | 严格双栈要求 |

### 2.4 externalTrafficPolicy 详解

| 值 | 描述 | 源 IP 保留 | 负载分布 | 适用场景 |
|:---|:---|:---:|:---|:---|
| `Cluster` | 集群范围负载均衡 | 否 (SNAT) | 均匀分布到所有 Pod | 默认，大多数场景 |
| `Local` | 仅本地节点 Pod | 是 | 仅发送到本节点 Pod | 需要源 IP，审计日志 |

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                      externalTrafficPolicy 对比                                          │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                          │
│  Cluster (默认)                               Local                                      │
│  ┌────────────────────────────────────┐      ┌────────────────────────────────────┐    │
│  │                                    │      │                                    │    │
│  │  Client: 1.2.3.4                   │      │  Client: 1.2.3.4                   │    │
│  │         │                          │      │         │                          │    │
│  │         ▼                          │      │         ▼                          │    │
│  │  ┌─────────────┐                   │      │  ┌─────────────┐                   │    │
│  │  │  Node 1     │                   │      │  │  Node 1     │                   │    │
│  │  │  NodePort   │                   │      │  │  NodePort   │                   │    │
│  │  └──────┬──────┘                   │      │  └──────┬──────┘                   │    │
│  │         │                          │      │         │                          │    │
│  │         │ SNAT: Node1 IP           │      │         │ 保留: 1.2.3.4            │    │
│  │         ▼                          │      │         ▼                          │    │
│  │  ┌──────┴──────┐                   │      │  仅转发到 Node 1 上的 Pod          │    │
│  │  │ 转发到任意  │                   │      │  ┌─────────────┐                   │    │
│  │  │ 节点的 Pod  │                   │      │  │  Pod (本地) │                   │    │
│  │  └─────────────┘                   │      │  └─────────────┘                   │    │
│  │                                    │      │                                    │    │
│  │  优点: 负载均衡                    │      │  优点: 源 IP 保留                  │    │
│  │  缺点: 源 IP 丢失                  │      │  缺点: 可能不均衡                  │    │
│  └────────────────────────────────────┘      └────────────────────────────────────┘    │
│                                                                                          │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Service 类型详解 (Service Types)

### 3.1 Service 类型对比矩阵

| 类型 | ClusterIP | NodePort | 外部 LB | DNS 记录 | 访问范围 | 典型场景 |
|:---|:---:|:---:|:---:|:---|:---|:---|
| **ClusterIP** | 自动分配 | - | - | A/AAAA | 集群内部 | 内部微服务通信 |
| **NodePort** | 自动分配 | 30000-32767 | - | A/AAAA | 集群内+节点 | 开发测试、简单暴露 |
| **LoadBalancer** | 自动分配 | 可禁用 | 云 LB | A/AAAA | 集群内+外部 | 生产环境外部访问 |
| **ExternalName** | - | - | - | CNAME | DNS 映射 | 外部服务代理 |
| **Headless** | None | - | - | 多 A 记录 | Pod 直连 | StatefulSet、自定义 LB |

### 3.2 ClusterIP Service (默认类型)

```yaml
# clusterip-service-production.yaml
# 生产级 ClusterIP Service 完整配置
apiVersion: v1
kind: Service
metadata:
  name: backend-api
  namespace: production
  labels:
    app.kubernetes.io/name: backend-api
    app.kubernetes.io/component: api
    app.kubernetes.io/part-of: myapp
    app.kubernetes.io/version: v2.1.0
    app.kubernetes.io/managed-by: kubectl
  annotations:
    # 文档注解
    description: "Backend API service for internal microservice communication"
    owner: "platform-team@example.com"
    oncall: "platform-oncall@example.com"
    
    # Prometheus 服务发现
    prometheus.io/scrape: "true"
    prometheus.io/port: "9091"
    prometheus.io/path: "/metrics"
    prometheus.io/scheme: "http"
    
    # 链路追踪
    opentelemetry.io/inject: "true"
spec:
  type: ClusterIP  # 默认类型，可省略
  
  # 标签选择器
  selector:
    app.kubernetes.io/name: backend-api
    app.kubernetes.io/component: api
  
  # 端口配置
  ports:
    # HTTP API 端口
    - name: http
      protocol: TCP
      port: 80
      targetPort: 8080
      appProtocol: http
    
    # gRPC 端口
    - name: grpc
      protocol: TCP
      port: 9090
      targetPort: grpc  # 使用命名端口
      appProtocol: grpc
    
    # Prometheus metrics 端口
    - name: metrics
      protocol: TCP
      port: 9091
      targetPort: metrics
      appProtocol: http
    
    # 健康检查端口
    - name: health
      protocol: TCP
      port: 8081
      targetPort: 8081
      appProtocol: http
  
  # 双栈配置 (v1.20+)
  ipFamilyPolicy: PreferDualStack
  ipFamilies:
    - IPv4
    - IPv6
  
  # 会话亲和性
  sessionAffinity: None
  
  # 内部流量策略 (v1.26+ GA)
  internalTrafficPolicy: Cluster
---
# 对应的 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend-api
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app.kubernetes.io/name: backend-api
      app.kubernetes.io/component: api
  template:
    metadata:
      labels:
        app.kubernetes.io/name: backend-api
        app.kubernetes.io/component: api
    spec:
      containers:
        - name: api
          image: backend-api:v2.1.0
          ports:
            - name: http
              containerPort: 8080
              protocol: TCP
            - name: grpc
              containerPort: 9090
              protocol: TCP
            - name: metrics
              containerPort: 9091
              protocol: TCP
            - name: health
              containerPort: 8081
              protocol: TCP
          readinessProbe:
            httpGet:
              path: /healthz/ready
              port: health
            initialDelaySeconds: 5
            periodSeconds: 10
            failureThreshold: 3
          livenessProbe:
            httpGet:
              path: /healthz/live
              port: health
            initialDelaySeconds: 15
            periodSeconds: 20
            failureThreshold: 3
          resources:
            requests:
              cpu: 200m
              memory: 256Mi
            limits:
              cpu: 1000m
              memory: 1Gi
```

### 3.3 NodePort Service

```yaml
# nodeport-service-production.yaml
# 生产级 NodePort Service 配置
apiVersion: v1
kind: Service
metadata:
  name: frontend-nodeport
  namespace: production
  labels:
    app.kubernetes.io/name: frontend
    app.kubernetes.io/component: web
  annotations:
    description: "Frontend service exposed via NodePort for development/testing"
spec:
  type: NodePort
  
  selector:
    app.kubernetes.io/name: frontend
    app.kubernetes.io/component: web
  
  ports:
    - name: http
      protocol: TCP
      port: 80
      targetPort: 3000
      nodePort: 30080  # 指定 NodePort (30000-32767)
    
    - name: https
      protocol: TCP
      port: 443
      targetPort: 3443
      nodePort: 30443
  
  # 外部流量策略
  # Local: 保留客户端源 IP，但可能导致不均衡
  # Cluster: 均衡分布，但丢失源 IP
  externalTrafficPolicy: Local
  
  # 健康检查端口 (externalTrafficPolicy=Local 时使用)
  # 云负载均衡器使用此端口检查节点健康状态
  healthCheckNodePort: 32000
```

### 3.4 LoadBalancer Service

```yaml
# loadbalancer-service-aws.yaml
# AWS NLB 生产级配置
apiVersion: v1
kind: Service
metadata:
  name: web-public
  namespace: production
  labels:
    app.kubernetes.io/name: web
    app.kubernetes.io/component: frontend
  annotations:
    # AWS Load Balancer Controller 注解
    service.beta.kubernetes.io/aws-load-balancer-type: "external"
    service.beta.kubernetes.io/aws-load-balancer-nlb-target-type: "ip"
    service.beta.kubernetes.io/aws-load-balancer-scheme: "internet-facing"
    
    # 跨可用区负载均衡
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    
    # 目标组属性
    service.beta.kubernetes.io/aws-load-balancer-target-group-attributes: >-
      deregistration_delay.timeout_seconds=30,
      stickiness.enabled=false,
      proxy_protocol_v2.enabled=false
    
    # 健康检查配置
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-protocol: "HTTP"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-path: "/healthz"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-port: "8081"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-interval: "10"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-timeout: "5"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-healthy-threshold: "2"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-unhealthy-threshold: "2"
    
    # TLS 终止 (NLB 支持 TLS 终止)
    service.beta.kubernetes.io/aws-load-balancer-ssl-cert: "arn:aws:acm:us-west-2:123456789012:certificate/xxx-xxx"
    service.beta.kubernetes.io/aws-load-balancer-ssl-ports: "443"
    service.beta.kubernetes.io/aws-load-balancer-ssl-negotiation-policy: "ELBSecurityPolicy-TLS13-1-2-2021-06"
    
    # 访问日志
    service.beta.kubernetes.io/aws-load-balancer-access-log-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-access-log-s3-bucket-name: "my-lb-logs"
    service.beta.kubernetes.io/aws-load-balancer-access-log-s3-bucket-prefix: "nlb/web-public"
    
    # 外部 DNS 集成
    external-dns.alpha.kubernetes.io/hostname: "api.example.com"
    external-dns.alpha.kubernetes.io/ttl: "300"
spec:
  type: LoadBalancer
  
  # 指定 LoadBalancer 控制器类 (v1.24+)
  loadBalancerClass: service.k8s.aws/nlb
  
  # 不分配 NodePort (NLB IP 模式)
  allocateLoadBalancerNodePorts: false
  
  selector:
    app.kubernetes.io/name: web
    app.kubernetes.io/component: frontend
  
  ports:
    - name: http
      protocol: TCP
      port: 80
      targetPort: 8080
    - name: https
      protocol: TCP
      port: 443
      targetPort: 8443
  
  # 外部流量策略
  externalTrafficPolicy: Local
  
  # 源 IP 范围限制 (可选，建议在安全组配置)
  # loadBalancerSourceRanges:
  #   - 10.0.0.0/8
  #   - 192.168.0.0/16
---
# loadbalancer-service-aliyun.yaml
# 阿里云 SLB 生产级配置
apiVersion: v1
kind: Service
metadata:
  name: web-public-aliyun
  namespace: production
  annotations:
    # 基本配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "internet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.medium"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-charge-type: "paybybandwidth"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-bandwidth: "100"
    
    # 复用已有 SLB (可选)
    # service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "lb-xxx"
    # service.beta.kubernetes.io/alibaba-cloud-loadbalancer-force-override-listeners: "true"
    
    # 健康检查
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/healthz"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-port: "8081"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-healthy-threshold: "2"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-unhealthy-threshold: "2"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "5"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-timeout: "3"
    
    # 调度算法
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-scheduler: "wrr"
    
    # 会话保持
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-persistence-timeout: "0"
    
    # 访问控制
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-acl-status: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-acl-type: "white"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-acl-id: "acl-xxx"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local
  selector:
    app.kubernetes.io/name: web
  ports:
    - name: http
      port: 80
      targetPort: 8080
---
# loadbalancer-service-gcp.yaml
# GCP 负载均衡配置
apiVersion: v1
kind: Service
metadata:
  name: web-public-gcp
  namespace: production
  annotations:
    # 内部负载均衡
    # networking.gke.io/load-balancer-type: "Internal"
    
    # Container-native 负载均衡 (NEG)
    cloud.google.com/neg: '{"ingress": true}'
    
    # 后端配置
    cloud.google.com/backend-config: '{"ports": {"http": "backend-config"}}'
    
    # 全局访问 (仅内部 LB)
    # networking.gke.io/internal-load-balancer-allow-global-access: "true"
spec:
  type: LoadBalancer
  selector:
    app.kubernetes.io/name: web
  ports:
    - name: http
      port: 80
      targetPort: 8080
---
# loadbalancer-service-azure.yaml
# Azure 负载均衡配置
apiVersion: v1
kind: Service
metadata:
  name: web-public-azure
  namespace: production
  annotations:
    # 内部负载均衡
    # service.beta.kubernetes.io/azure-load-balancer-internal: "true"
    # service.beta.kubernetes.io/azure-load-balancer-internal-subnet: "subnet-name"
    
    # 资源组
    service.beta.kubernetes.io/azure-load-balancer-resource-group: "my-rg"
    
    # 健康探测协议
    service.beta.kubernetes.io/azure-load-balancer-health-probe-protocol: "Http"
    service.beta.kubernetes.io/azure-load-balancer-health-probe-request-path: "/healthz"
    
    # TCP 重置 (空闲超时时发送 RST)
    service.beta.kubernetes.io/azure-load-balancer-tcp-idle-timeout: "4"
spec:
  type: LoadBalancer
  selector:
    app.kubernetes.io/name: web
  ports:
    - name: http
      port: 80
      targetPort: 8080
```

### 3.5 ExternalName Service

```yaml
# externalname-service.yaml
# 外部服务 DNS 映射
apiVersion: v1
kind: Service
metadata:
  name: external-database
  namespace: production
  labels:
    app.kubernetes.io/name: external-database
    app.kubernetes.io/component: database
  annotations:
    description: "Maps to external RDS database"
spec:
  type: ExternalName
  externalName: mydb.us-west-2.rds.amazonaws.com
  # 注意: ExternalName 不支持端口映射，客户端需使用实际端口
---
# 外部服务 (带端口映射) - 使用无 selector Service + EndpointSlice
apiVersion: v1
kind: Service
metadata:
  name: legacy-api
  namespace: production
  labels:
    app.kubernetes.io/name: legacy-api
  annotations:
    description: "External legacy API service with manual endpoints"
spec:
  type: ClusterIP
  ports:
    - name: https
      port: 443
      targetPort: 443
      appProtocol: https
    - name: http
      port: 80
      targetPort: 80
      appProtocol: http
  # 无 selector，手动管理 EndpointSlice
---
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: legacy-api-1
  namespace: production
  labels:
    kubernetes.io/service-name: legacy-api
  annotations:
    description: "Manual endpoints for legacy API"
addressType: IPv4
ports:
  - name: https
    appProtocol: https
    protocol: TCP
    port: 443
  - name: http
    appProtocol: http
    protocol: TCP
    port: 80
endpoints:
  - addresses:
      - "203.0.113.10"
    conditions:
      ready: true
      serving: true
      terminating: false
    hostname: legacy-api-1
    zone: us-west-2a
  - addresses:
      - "203.0.113.11"
    conditions:
      ready: true
      serving: true
      terminating: false
    hostname: legacy-api-2
    zone: us-west-2b
```

---

## 4. Headless Service 深度解析 (Headless Service)

### 4.1 Headless Service 架构

```
┌─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                            Headless Service vs Regular Service                                   │
├─────────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                                  │
│  Regular Service (ClusterIP)                     Headless Service (clusterIP: None)            │
│                                                                                                  │
│  ┌─────────────────────────────────────┐        ┌─────────────────────────────────────┐       │
│  │          DNS Query                   │        │          DNS Query                   │       │
│  │   my-svc.ns.svc.cluster.local        │        │   my-svc.ns.svc.cluster.local        │       │
│  └──────────────────┬──────────────────┘        └──────────────────┬──────────────────┘       │
│                     │                                              │                           │
│                     ▼                                              ▼                           │
│  ┌─────────────────────────────────────┐        ┌─────────────────────────────────────┐       │
│  │          DNS Response                │        │          DNS Response                │       │
│  │                                      │        │                                      │       │
│  │  单一 A 记录: 10.96.0.100 (VIP)     │        │  多 A 记录 (所有 Ready Pod):        │       │
│  │                                      │        │    10.244.1.10 (pod-0)              │       │
│  │                                      │        │    10.244.2.11 (pod-1)              │       │
│  │                                      │        │    10.244.3.12 (pod-2)              │       │
│  └──────────────────┬──────────────────┘        └──────────────────┬──────────────────┘       │
│                     │                                              │                           │
│                     ▼                                              ▼                           │
│  ┌─────────────────────────────────────┐        ┌─────────────────────────────────────┐       │
│  │       kube-proxy 负载均衡            │        │     客户端自行选择 (Client-side)    │       │
│  │                                      │        │                                      │       │
│  │  iptables/IPVS 规则:                │        │  应用层负载均衡:                     │       │
│  │  - 随机/轮询选择后端                 │        │  - DNS 轮询                          │       │
│  │  - DNAT 到 Pod IP                   │        │  - gRPC 客户端负载均衡               │       │
│  │                                      │        │  - 自定义策略                        │       │
│  └──────────────────┬──────────────────┘        └──────────────────┬──────────────────┘       │
│                     │                                              │                           │
│                     ▼                                              ▼                           │
│  ┌─────────────────────────────────────┐        ┌─────────────────────────────────────┐       │
│  │         Backend Pods                 │        │         Backend Pods                 │       │
│  │  ┌───────┐ ┌───────┐ ┌───────┐     │        │  ┌───────┐ ┌───────┐ ┌───────┐     │       │
│  │  │Pod-0  │ │Pod-1  │ │Pod-2  │     │        │  │Pod-0  │ │Pod-1  │ │Pod-2  │     │       │
│  │  └───────┘ └───────┘ └───────┘     │        │  └───────┘ └───────┘ └───────┘     │       │
│  └─────────────────────────────────────┘        └─────────────────────────────────────┘       │
│                                                                                                  │
│  StatefulSet + Headless Service DNS 记录:                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────────────────┐  │
│  │  Service DNS (所有 Pod):                                                                  │  │
│  │  └── my-svc.ns.svc.cluster.local → [10.244.1.10, 10.244.2.11, 10.244.3.12]              │  │
│  │                                                                                           │  │
│  │  Pod DNS (单独访问):                                                                      │  │
│  │  ├── pod-0.my-svc.ns.svc.cluster.local → 10.244.1.10                                    │  │
│  │  ├── pod-1.my-svc.ns.svc.cluster.local → 10.244.2.11                                    │  │
│  │  └── pod-2.my-svc.ns.svc.cluster.local → 10.244.3.12                                    │  │
│  │                                                                                           │  │
│  │  SRV 记录 (端口发现):                                                                     │  │
│  │  └── _mysql._tcp.my-svc.ns.svc.cluster.local → 0 100 3306 pod-0.my-svc.ns.svc...       │  │
│  └─────────────────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Headless Service 生产配置

```yaml
# headless-service-mysql.yaml
# MySQL 集群 Headless Service
apiVersion: v1
kind: Service
metadata:
  name: mysql
  namespace: database
  labels:
    app.kubernetes.io/name: mysql
    app.kubernetes.io/component: database
spec:
  clusterIP: None  # Headless Service 关键配置
  
  selector:
    app.kubernetes.io/name: mysql
    app.kubernetes.io/component: database
  
  ports:
    - name: mysql
      port: 3306
      targetPort: 3306
      appProtocol: mysql
    - name: mysqlx
      port: 33060
      targetPort: 33060
    - name: metrics
      port: 9104
      targetPort: 9104
  
  # 发布未就绪地址 (用于初始化场景)
  publishNotReadyAddresses: false
---
# MySQL 只读 Service (正常 ClusterIP)
apiVersion: v1
kind: Service
metadata:
  name: mysql-read
  namespace: database
  labels:
    app.kubernetes.io/name: mysql
    app.kubernetes.io/component: database
spec:
  type: ClusterIP
  selector:
    app.kubernetes.io/name: mysql
    app.kubernetes.io/component: database
  ports:
    - name: mysql
      port: 3306
      targetPort: 3306
---
# StatefulSet
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: database
spec:
  serviceName: mysql  # 关联 Headless Service
  replicas: 3
  podManagementPolicy: Parallel  # 或 OrderedReady
  selector:
    matchLabels:
      app.kubernetes.io/name: mysql
      app.kubernetes.io/component: database
  template:
    metadata:
      labels:
        app.kubernetes.io/name: mysql
        app.kubernetes.io/component: database
    spec:
      terminationGracePeriodSeconds: 30
      containers:
        - name: mysql
          image: mysql:8.0
          ports:
            - name: mysql
              containerPort: 3306
            - name: mysqlx
              containerPort: 33060
          env:
            - name: MYSQL_ROOT_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: root-password
            - name: MYSQL_CLUSTER_NAME
              value: "mysql-cluster"
            - name: MYSQL_SERVER_ID
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
          readinessProbe:
            exec:
              command:
                - bash
                - "-c"
                - "mysqladmin ping -u root -p$MYSQL_ROOT_PASSWORD"
            initialDelaySeconds: 30
            periodSeconds: 10
            timeoutSeconds: 5
          livenessProbe:
            exec:
              command:
                - bash
                - "-c"
                - "mysqladmin ping -u root -p$MYSQL_ROOT_PASSWORD"
            initialDelaySeconds: 60
            periodSeconds: 15
            timeoutSeconds: 5
          resources:
            requests:
              cpu: 500m
              memory: 1Gi
            limits:
              cpu: 2
              memory: 4Gi
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
            - name: config
              mountPath: /etc/mysql/conf.d
        - name: exporter
          image: prom/mysqld-exporter:v0.15.0
          ports:
            - name: metrics
              containerPort: 9104
          env:
            - name: DATA_SOURCE_NAME
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: exporter-dsn
          resources:
            requests:
              cpu: 50m
              memory: 64Mi
            limits:
              cpu: 100m
              memory: 128Mi
      volumes:
        - name: config
          configMap:
            name: mysql-config
  volumeClaimTemplates:
    - metadata:
        name: data
        labels:
          app.kubernetes.io/name: mysql
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 100Gi
```

### 4.3 Headless Service 使用场景矩阵

| 场景 | 描述 | DNS 行为 | 示例应用 | 配置要点 |
|:---|:---|:---|:---|:---|
| **StatefulSet** | 有状态应用 | Pod DNS 名称 | MySQL, PostgreSQL, MongoDB | `serviceName` 必须匹配 |
| **点对点通信** | 节点间直接通信 | 所有 Pod IP | etcd, ZooKeeper, Kafka | 使用 Pod DNS 互相发现 |
| **自定义负载均衡** | 客户端控制 | 所有 Pod IP | gRPC 负载均衡 | 客户端实现 LB 逻辑 |
| **Leader 选举** | 连接特定 Pod | 指定 Pod IP | Raft 集群 | 通过 Pod DNS 连接 Leader |
| **服务发现** | 获取所有实例 | 所有 Pod IP | 集群协调 | DNS A 记录查询 |

---

## 5. EndpointSlice 深度解析 (EndpointSlice)

### 5.1 EndpointSlice 架构

| 特性 | Endpoints (旧) | EndpointSlice (新) |
|:---|:---|:---|
| **引入版本** | v1.0 | v1.16 Alpha, v1.21 GA |
| **存储方式** | 单一对象存储所有端点 | 分片存储，每片最多 100 个端点 |
| **大规模支持** | 差 (单对象过大) | 优 (分片+增量更新) |
| **拓扑信息** | 无 | 支持 zone、hostname |
| **端点状态** | ready/notReady | ready、serving、terminating |
| **推荐使用** | 仅兼容旧系统 | 所有新系统 |

### 5.2 EndpointSlice 结构

```yaml
# endpointslice-example.yaml
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: my-service-abc123
  namespace: production
  labels:
    # 必须标签：关联 Service
    kubernetes.io/service-name: my-service
    # 可选标签
    endpointslice.kubernetes.io/managed-by: endpointslice-controller.k8s.io
  ownerReferences:
    - apiVersion: v1
      kind: Service
      name: my-service
      uid: xxx-xxx
# 地址类型: IPv4, IPv6, FQDN
addressType: IPv4

# 端口列表 (所有端点共享)
ports:
  - name: http
    protocol: TCP
    port: 8080
    appProtocol: http
  - name: grpc
    protocol: TCP
    port: 9090
    appProtocol: grpc

# 端点列表 (最多 100 个)
endpoints:
  # 端点 1
  - addresses:
      - "10.244.1.10"
    conditions:
      ready: true      # Pod Ready
      serving: true    # Pod 可接收流量 (v1.22+)
      terminating: false  # Pod 正在终止 (v1.22+)
    hostname: pod-0
    nodeName: node-1
    zone: us-west-2a
    targetRef:
      kind: Pod
      name: my-service-pod-0
      namespace: production
      uid: xxx-xxx
    deprecatedTopology:  # 已弃用，使用 zone/nodeName
      kubernetes.io/hostname: node-1
  
  # 端点 2
  - addresses:
      - "10.244.2.11"
    conditions:
      ready: true
      serving: true
      terminating: false
    hostname: pod-1
    nodeName: node-2
    zone: us-west-2b
    targetRef:
      kind: Pod
      name: my-service-pod-1
      namespace: production
      uid: xxx-xxx
  
  # 端点 3 (正在终止)
  - addresses:
      - "10.244.3.12"
    conditions:
      ready: false      # 不再 ready
      serving: true     # 但仍可接收流量 (graceful)
      terminating: true # 正在终止
    hostname: pod-2
    nodeName: node-3
    zone: us-west-2c
    targetRef:
      kind: Pod
      name: my-service-pod-2
      namespace: production
      uid: xxx-xxx
```

### 5.3 EndpointSlice 条件状态

| 条件 | 描述 | true | false |
|:---|:---|:---|:---|
| `ready` | Pod 是否 Ready | 通过 readinessProbe | 未通过或 Pod 终止中 |
| `serving` | 是否可接收流量 | Pod 可处理请求 | Pod 不可处理请求 |
| `terminating` | 是否正在终止 | Pod 正在 terminating | Pod 正常运行 |

**状态组合场景**:

| ready | serving | terminating | 场景 | kube-proxy 行为 |
|:---:|:---:|:---:|:---|:---|
| true | true | false | 正常运行 | 包含在负载均衡中 |
| false | true | true | 优雅终止中 | 继续接收流量直到完全终止 |
| false | false | true | 终止完成 | 移除出负载均衡 |
| false | false | false | 未就绪 | 不包含在负载均衡中 |

---

## 6. Service 与 DNS 集成 (DNS Integration)

### 6.1 DNS 记录类型详解

| 记录类型 | 格式 | 用途 | 示例 |
|:---|:---|:---|:---|
| **A/AAAA** | `<svc>.<ns>.svc.<cluster-domain>` | Service ClusterIP | `my-svc.prod.svc.cluster.local → 10.96.0.100` |
| **A (Headless)** | `<svc>.<ns>.svc.<cluster-domain>` | 所有 Pod IP | `my-svc.prod.svc.cluster.local → 10.244.1.10, 10.244.2.11` |
| **SRV** | `_<port>._<proto>.<svc>.<ns>.svc.<cluster-domain>` | 端口和协议发现 | `_http._tcp.my-svc.prod.svc.cluster.local` |
| **Pod A** | `<pod-ip-dashed>.<ns>.pod.<cluster-domain>` | Pod IP 查询 | `10-244-1-10.prod.pod.cluster.local` |
| **StatefulSet Pod** | `<pod>.<svc>.<ns>.svc.<cluster-domain>` | 特定 Pod | `web-0.nginx.prod.svc.cluster.local` |
| **PTR** | `<reversed-ip>.in-addr.arpa` | 反向 DNS | `100.0.96.10.in-addr.arpa → my-svc.prod.svc.cluster.local` |

### 6.2 DNS 配置最佳实践

```yaml
# pod-dns-config.yaml
apiVersion: v1
kind: Pod
metadata:
  name: dns-optimized-pod
  namespace: production
spec:
  containers:
    - name: app
      image: myapp:latest
  
  # DNS 策略
  # ClusterFirst: 优先使用集群 DNS (默认)
  # Default: 继承节点 DNS 配置
  # ClusterFirstWithHostNet: hostNetwork 时使用集群 DNS
  # None: 完全自定义
  dnsPolicy: ClusterFirst
  
  # 自定义 DNS 配置
  dnsConfig:
    # 额外的 nameserver (会添加到列表开头)
    nameservers:
      - 10.96.0.10  # CoreDNS ClusterIP
    
    # 搜索域 (最多 6 个，总字符 <= 256)
    searches:
      - production.svc.cluster.local
      - svc.cluster.local
      - cluster.local
    
    # DNS 选项
    options:
      # ndots: FQDN 判断阈值
      # 小于此值的点数会追加搜索域
      # 例: ndots=5, "my-svc.prod" (1个点) 会追加搜索域
      # 优化: 对于已知 FQDN，使用尾部点 "my-svc.prod.svc.cluster.local."
      - name: ndots
        value: "2"  # 降低默认值 5，减少 DNS 查询
      
      # 单个请求超时 (秒)
      - name: timeout
        value: "2"
      
      # 重试次数
      - name: attempts
        value: "3"
      
      # 使用 TCP (当 UDP 被截断时)
      - name: use-vc
        value: ""
      
      # 单一请求 (禁用 A 和 AAAA 并行查询)
      - name: single-request-reopen
        value: ""
---
# CoreDNS 优化配置
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
        
        # Kubernetes 服务发现
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        
        # 自动路径 (减少搜索域查询)
        autopath @kubernetes
        
        # 负缓存
        # 缓存 NXDOMAIN 响应，减少重复失败查询
        cache {
            success 9984 30
            denial 9984 5
        }
        
        # 上游 DNS
        forward . /etc/resolv.conf {
            max_concurrent 1000
            prefer_udp
        }
        
        # 循环检测
        loop
        
        # 重新加载配置
        reload
        
        # 负载报告
        loadbalance
        
        # Prometheus 指标
        prometheus :9153
    }
```

### 6.3 DNS 查询优化

```bash
#!/bin/bash
# dns-optimization-tips.sh

# 1. 使用 FQDN (带尾部点) 避免搜索域追加
# 不优化:
curl http://my-service.production
# 可能触发多次 DNS 查询:
#   my-service.production.default.svc.cluster.local (NXDOMAIN)
#   my-service.production.svc.cluster.local (NXDOMAIN)
#   my-service.production.cluster.local (NXDOMAIN)
#   my-service.production (NXDOMAIN 或成功)

# 优化:
curl http://my-service.production.svc.cluster.local.
# 只触发一次 DNS 查询

# 2. 同命名空间内使用短名称
# 在 production 命名空间内:
curl http://my-service
# 解析为 my-service.production.svc.cluster.local

# 3. 跨命名空间使用 <svc>.<ns> 格式
curl http://my-service.other-ns
# 解析为 my-service.other-ns.svc.cluster.local

# 4. 降低 ndots 值 (在 dnsConfig 中配置)
# 默认 ndots=5，意味着 4 个点以下都会追加搜索域
# 对于大多数应用，ndots=2 足够

# 5. 使用 DNS 缓存 (NodeLocal DNSCache)
# 在每个节点运行本地 DNS 缓存，减少 CoreDNS 负载
```

---

## 7. kube-proxy 模式详解 (kube-proxy Modes)

### 7.1 模式对比

| 特性 | iptables | IPVS | nftables | eBPF (Cilium) |
|:---|:---|:---|:---|:---|
| **引入版本** | v1.2+ | v1.9+ | v1.29+ Alpha | 外部 CNI |
| **默认模式** | v1.2-v1.28 | 可选 | v1.31+ 可选 | 需 Cilium |
| **性能** | 中等 (O(n)) | 高 (O(1)) | 高 | 最高 |
| **规则数量** | 大 (线性增长) | 小 (哈希表) | 中 | 最小 |
| **功能** | 基础 | 丰富 (多种调度算法) | 基础 | 最丰富 |
| **调试难度** | 低 | 中 | 低 | 高 |
| **依赖** | iptables | ipvs 内核模块 | nftables | eBPF |
| **推荐场景** | 小规模/兼容性 | 生产环境 | 新集群 | 高性能 |

### 7.2 kube-proxy 配置

```yaml
# kube-proxy-configmap.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    apiVersion: kubeproxy.config.k8s.io/v1alpha1
    kind: KubeProxyConfiguration
    
    # 绑定地址
    bindAddress: 0.0.0.0
    
    # 健康检查
    healthzBindAddress: 0.0.0.0:10256
    metricsBindAddress: 0.0.0.0:10249
    
    # 代理模式: iptables, ipvs, nftables (v1.29+)
    mode: ipvs
    
    # iptables 模式配置
    iptables:
      masqueradeAll: false
      masqueradeBit: 14
      minSyncPeriod: 1s
      syncPeriod: 30s
      # 本地化 Service (v1.22+)
      localhostNodePorts: true
    
    # IPVS 模式配置
    ipvs:
      # 调度算法
      # rr: 轮询 (默认)
      # lc: 最少连接
      # dh: 目标地址哈希
      # sh: 源地址哈希
      # sed: 最短期望延迟
      # nq: 永不排队
      scheduler: rr
      
      # 同步周期
      syncPeriod: 30s
      minSyncPeriod: 2s
      
      # TCP 超时
      tcpTimeout: 0s
      tcpFinTimeout: 0s
      udpTimeout: 0s
      
      # 严格 ARP (MetalLB 需要)
      strictARP: true
      
      # 排除 CIDR (不代理这些地址)
      excludeCIDRs: []
    
    # nftables 模式配置 (v1.29+)
    nftables:
      masqueradeAll: false
      masqueradeBit: 14
      minSyncPeriod: 1s
      syncPeriod: 30s
    
    # conntrack 配置
    conntrack:
      maxPerCore: 32768
      min: 131072
      tcpEstablishedTimeout: 86400s  # 24h
      tcpCloseWaitTimeout: 3600s     # 1h
    
    # 节点端口地址
    # 空表示所有本地地址
    nodePortAddresses: []
    
    # 配置同步模式
    configSyncPeriod: 15m
    
    # 检测本地流量
    detectLocalMode: ClusterCIDR
    # detectLocal:
    #   bridgeInterface: ""
    #   interfaceNamePrefix: ""
    
    # 日志级别
    logging:
      flushFrequency: 5s
      verbosity: 0
---
# IPVS 调度算法详解
# kube-proxy IPVS scheduler comparison
#
# | 算法 | 参数 | 描述 | 适用场景 |
# |-----|------|------|---------|
# | rr | roundrobin | 轮询分发 | 通用场景 (默认) |
# | lc | leastconn | 最少连接优先 | 长连接服务 |
# | dh | destinationhash | 目标IP哈希 | 需要目标亲和 |
# | sh | sourcehash | 源IP哈希 | 需要源亲和 |
# | sed | shortestexpecteddelay | 最短期望延迟 | 异构后端 |
# | nq | neverqueue | 永不排队 | 低延迟要求 |
```

### 7.3 IPVS 调度算法详解

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                              IPVS 调度算法对比                                           │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                          │
│  1. Round Robin (rr) - 轮询                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │  请求 1 → Pod A                                                                  │   │
│  │  请求 2 → Pod B                                                                  │   │
│  │  请求 3 → Pod C                                                                  │   │
│  │  请求 4 → Pod A (循环)                                                           │   │
│  │                                                                                   │   │
│  │  优点: 简单，均衡                                                                │   │
│  │  缺点: 不考虑后端负载                                                            │   │
│  │  场景: 无状态服务，后端能力相近                                                  │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
│  2. Least Connection (lc) - 最少连接                                                    │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │  当前连接: Pod A(10), Pod B(5), Pod C(8)                                         │   │
│  │  新请求 → Pod B (连接数最少)                                                     │   │
│  │                                                                                   │   │
│  │  优点: 自动平衡负载                                                              │   │
│  │  缺点: 不适合短连接                                                              │   │
│  │  场景: 数据库连接池，WebSocket                                                   │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
│  3. Source Hash (sh) - 源地址哈希                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │  Client 1.2.3.4 → hash(1.2.3.4) → Pod A (始终)                                  │   │
│  │  Client 5.6.7.8 → hash(5.6.7.8) → Pod B (始终)                                  │   │
│  │                                                                                   │   │
│  │  优点: 会话保持                                                                  │   │
│  │  缺点: 后端变化时重新分布                                                        │   │
│  │  场景: 需要会话亲和但不支持 cookie                                               │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
│  4. Destination Hash (dh) - 目标地址哈希                                                │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │  访问 /api/v1 → hash(/api/v1) → Pod A                                           │   │
│  │  访问 /api/v2 → hash(/api/v2) → Pod B                                           │   │
│  │                                                                                   │   │
│  │  优点: 缓存友好                                                                  │   │
│  │  缺点: 需要 L7 支持                                                              │   │
│  │  场景: CDN，代理缓存                                                             │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 8. 生产环境配置示例 (Production Examples)

### 8.1 完整微服务架构

```yaml
# production-microservice-stack.yaml
# API Gateway Service
apiVersion: v1
kind: Service
metadata:
  name: api-gateway
  namespace: production
  labels:
    app.kubernetes.io/name: api-gateway
    app.kubernetes.io/component: gateway
    app.kubernetes.io/part-of: myapp
  annotations:
    prometheus.io/scrape: "true"
    prometheus.io/port: "9091"
    external-dns.alpha.kubernetes.io/hostname: "api.example.com"
spec:
  type: LoadBalancer
  loadBalancerClass: service.k8s.aws/nlb
  externalTrafficPolicy: Local
  selector:
    app.kubernetes.io/name: api-gateway
  ports:
    - name: http
      port: 80
      targetPort: 8080
    - name: https
      port: 443
      targetPort: 8443
    - name: metrics
      port: 9091
      targetPort: 9091
---
# User Service (内部)
apiVersion: v1
kind: Service
metadata:
  name: user-service
  namespace: production
  labels:
    app.kubernetes.io/name: user-service
    app.kubernetes.io/component: backend
spec:
  type: ClusterIP
  selector:
    app.kubernetes.io/name: user-service
  ports:
    - name: grpc
      port: 9090
      targetPort: 9090
      appProtocol: grpc
    - name: http
      port: 80
      targetPort: 8080
      appProtocol: http
    - name: metrics
      port: 9091
      targetPort: 9091
---
# Order Service (内部)
apiVersion: v1
kind: Service
metadata:
  name: order-service
  namespace: production
spec:
  type: ClusterIP
  selector:
    app.kubernetes.io/name: order-service
  ports:
    - name: grpc
      port: 9090
      targetPort: 9090
      appProtocol: grpc
    - name: metrics
      port: 9091
      targetPort: 9091
---
# Redis (Headless for Sentinel)
apiVersion: v1
kind: Service
metadata:
  name: redis
  namespace: production
spec:
  clusterIP: None
  selector:
    app.kubernetes.io/name: redis
  ports:
    - name: redis
      port: 6379
      targetPort: 6379
    - name: sentinel
      port: 26379
      targetPort: 26379
---
# Redis 只读 Service
apiVersion: v1
kind: Service
metadata:
  name: redis-readonly
  namespace: production
spec:
  type: ClusterIP
  selector:
    app.kubernetes.io/name: redis
    role: replica
  ports:
    - name: redis
      port: 6379
      targetPort: 6379
---
# PostgreSQL Primary (Headless)
apiVersion: v1
kind: Service
metadata:
  name: postgres
  namespace: production
spec:
  clusterIP: None
  selector:
    app.kubernetes.io/name: postgres
  ports:
    - name: postgres
      port: 5432
      targetPort: 5432
---
# PostgreSQL 只读 Service
apiVersion: v1
kind: Service
metadata:
  name: postgres-readonly
  namespace: production
spec:
  type: ClusterIP
  selector:
    app.kubernetes.io/name: postgres
    role: replica
  ports:
    - name: postgres
      port: 5432
      targetPort: 5432
---
# PodDisruptionBudget for API Gateway
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: api-gateway-pdb
  namespace: production
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app.kubernetes.io/name: api-gateway
```

### 8.2 gRPC 服务负载均衡

```yaml
# grpc-service-loadbalancing.yaml
# gRPC 服务需要特殊处理，因为 HTTP/2 连接复用
apiVersion: v1
kind: Service
metadata:
  name: grpc-service
  namespace: production
  labels:
    app.kubernetes.io/name: grpc-service
  annotations:
    # Istio: 指定协议
    # service.istio.io/canonical-name: grpc-service
spec:
  type: ClusterIP
  selector:
    app.kubernetes.io/name: grpc-service
  ports:
    - name: grpc
      port: 9090
      targetPort: 9090
      appProtocol: grpc  # 重要：指定应用层协议
---
# Headless Service for client-side load balancing
apiVersion: v1
kind: Service
metadata:
  name: grpc-service-headless
  namespace: production
  annotations:
    description: "Headless service for gRPC client-side load balancing"
spec:
  clusterIP: None
  selector:
    app.kubernetes.io/name: grpc-service
  ports:
    - name: grpc
      port: 9090
      targetPort: 9090
      appProtocol: grpc
---
# gRPC 客户端配置示例 (Go)
# client.go:
#
# import (
#     "google.golang.org/grpc"
#     "google.golang.org/grpc/resolver"
#     _ "google.golang.org/grpc/resolver/dns" // DNS resolver
# )
#
# func main() {
#     // 使用 dns:/// 前缀启用 DNS 解析
#     // 使用 Headless Service 获取所有 Pod IP
#     conn, err := grpc.Dial(
#         "dns:///grpc-service-headless.production.svc.cluster.local:9090",
#         grpc.WithDefaultServiceConfig(`{
#             "loadBalancingPolicy": "round_robin",
#             "healthCheckConfig": {
#                 "serviceName": "grpc.health.v1.Health"
#             }
#         }`),
#         grpc.WithInsecure(),
#     )
# }
```

### 8.3 多区域服务拓扑

```yaml
# topology-aware-service.yaml
# 拓扑感知服务路由 (v1.21+)
apiVersion: v1
kind: Service
metadata:
  name: web-service
  namespace: production
  annotations:
    # 拓扑感知提示 (v1.23+)
    # 首选同一 zone 的端点
    service.kubernetes.io/topology-mode: Auto
spec:
  type: ClusterIP
  selector:
    app.kubernetes.io/name: web
  ports:
    - name: http
      port: 80
      targetPort: 8080
  # 内部流量策略
  internalTrafficPolicy: Cluster
---
# 使用 trafficDistribution (v1.30+ Alpha)
apiVersion: v1
kind: Service
metadata:
  name: web-service-v130
  namespace: production
spec:
  type: ClusterIP
  selector:
    app.kubernetes.io/name: web
  ports:
    - name: http
      port: 80
      targetPort: 8080
  # 流量分布 (v1.30+)
  # PreferClose: 优先同一拓扑区域
  trafficDistribution: PreferClose
---
# EndpointSlice 拓扑信息示例
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: web-service-zone-a
  namespace: production
  labels:
    kubernetes.io/service-name: web-service
addressType: IPv4
ports:
  - name: http
    port: 8080
    protocol: TCP
endpoints:
  - addresses: ["10.244.1.10"]
    zone: us-west-2a  # 拓扑区域
    nodeName: node-1
    conditions:
      ready: true
      serving: true
  - addresses: ["10.244.1.11"]
    zone: us-west-2a
    nodeName: node-2
    conditions:
      ready: true
      serving: true
---
# 多区域 Deployment 配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web
  namespace: production
spec:
  replicas: 6
  selector:
    matchLabels:
      app.kubernetes.io/name: web
  template:
    metadata:
      labels:
        app.kubernetes.io/name: web
    spec:
      # 拓扑分布约束
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app.kubernetes.io/name: web
        - maxSkew: 1
          topologyKey: kubernetes.io/hostname
          whenUnsatisfiable: ScheduleAnyway
          labelSelector:
            matchLabels:
              app.kubernetes.io/name: web
      containers:
        - name: web
          image: web:v1.0.0
          ports:
            - containerPort: 8080
```

---

## 9. 会话亲和性与流量策略 (Session Affinity & Traffic Policy)

### 9.1 会话亲和性配置

```yaml
# session-affinity-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: sticky-service
  namespace: production
  annotations:
    description: "Service with client IP session affinity"
spec:
  type: ClusterIP
  selector:
    app.kubernetes.io/name: sticky-app
  ports:
    - name: http
      port: 80
      targetPort: 8080
  
  # 会话亲和性类型
  # None: 无亲和性 (默认)
  # ClientIP: 基于客户端 IP
  sessionAffinity: ClientIP
  
  # 会话亲和性配置
  sessionAffinityConfig:
    clientIP:
      # 会话保持时间 (秒)
      # 默认: 10800 (3 小时)
      # 最大: 86400 (24 小时)
      timeoutSeconds: 3600  # 1 小时
```

### 9.2 流量策略组合

| 场景 | externalTrafficPolicy | internalTrafficPolicy | 效果 |
|:---|:---|:---|:---|
| **默认** | Cluster | Cluster | 所有流量均匀分布 |
| **保留源 IP** | Local | Cluster | 外部流量保留源 IP，内部正常 |
| **完全本地化** | Local | Local | 所有流量仅发送到本地 Pod |
| **内部优化** | Cluster | Local | 外部正常，内部优先本地 |

---

## 10. 监控与故障排查 (Monitoring & Troubleshooting)

### 10.1 Service 监控指标

| 指标 | 类型 | 来源 | 描述 |
|:---|:---|:---|:---|
| `kube_service_info` | Gauge | kube-state-metrics | Service 基本信息 |
| `kube_service_spec_type` | Gauge | kube-state-metrics | Service 类型 |
| `kube_service_spec_external_ip` | Gauge | kube-state-metrics | 外部 IP |
| `kube_service_status_load_balancer_ingress` | Gauge | kube-state-metrics | LB Ingress 信息 |
| `kube_endpoint_address_available` | Gauge | kube-state-metrics | 可用端点数 |
| `kube_endpoint_address_not_ready` | Gauge | kube-state-metrics | 未就绪端点数 |
| `kube_endpointslice_endpoints` | Gauge | kube-state-metrics | EndpointSlice 端点数 |
| `kubeproxy_sync_proxy_rules_duration_seconds` | Histogram | kube-proxy | 规则同步耗时 |
| `kubeproxy_network_programming_duration_seconds` | Histogram | kube-proxy | 网络编程耗时 |

### 10.2 故障排查命令

```bash
#!/bin/bash
# service-troubleshooting.sh

# ==================== 基本检查 ====================

# 1. 查看 Service 详情
kubectl get svc my-service -n production -o wide
kubectl describe svc my-service -n production

# 2. 检查 Endpoints
kubectl get endpoints my-service -n production
kubectl describe endpoints my-service -n production

# 3. 检查 EndpointSlice
kubectl get endpointslice -n production -l kubernetes.io/service-name=my-service
kubectl describe endpointslice -n production -l kubernetes.io/service-name=my-service

# 4. 检查 Pod 状态和标签
kubectl get pods -n production -l app=my-app -o wide
kubectl get pods -n production -l app=my-app --show-labels

# ==================== 网络连通性测试 ====================

# 5. DNS 解析测试
kubectl run -it --rm debug --image=busybox --restart=Never -- \
  nslookup my-service.production.svc.cluster.local

# 6. Service 连通性测试
kubectl run -it --rm debug --image=curlimages/curl --restart=Never -- \
  curl -v http://my-service.production.svc.cluster.local

# 7. 直接访问 Pod IP
kubectl run -it --rm debug --image=curlimages/curl --restart=Never -- \
  curl -v http://<pod-ip>:8080

# 8. NodePort 测试 (从集群外部)
curl -v http://<node-ip>:30080

# ==================== kube-proxy 检查 ====================

# 9. 查看 kube-proxy 日志
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=100

# 10. 检查 kube-proxy 配置
kubectl get configmap kube-proxy -n kube-system -o yaml

# 11. iptables 规则检查 (在节点上执行)
# 查看 Service 相关的 NAT 规则
iptables -t nat -L KUBE-SERVICES -n -v | grep my-service
iptables -t nat -L KUBE-SVC-XXXX -n -v

# 12. IPVS 规则检查 (在节点上执行)
ipvsadm -Ln | grep -A 5 <ClusterIP>
ipvsadm -Ln --stats | grep <ClusterIP>

# ==================== DNS 排查 ====================

# 13. CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100

# 14. CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# 15. DNS 详细查询
kubectl run -it --rm debug --image=tutum/dnsutils --restart=Never -- \
  dig +short my-service.production.svc.cluster.local

# 16. DNS SRV 记录查询
kubectl run -it --rm debug --image=tutum/dnsutils --restart=Never -- \
  dig +short SRV _http._tcp.my-service.production.svc.cluster.local

# ==================== 高级诊断 ====================

# 17. 使用 netshoot 进行网络诊断
kubectl run -it --rm netshoot --image=nicolaka/netshoot --restart=Never -- bash
# 在 netshoot 容器内:
# tcpdump -i any host <service-ip> -n
# ss -tuln
# netstat -rn

# 18. 检查 conntrack 表
# 在节点上执行
conntrack -L -d <ClusterIP> 2>/dev/null | head

# 19. 端口监听检查 (在 Pod 内)
kubectl exec -it <pod-name> -n production -- ss -tuln

# 20. 查看 Service 事件
kubectl get events -n production --field-selector involvedObject.name=my-service
```

### 10.3 常见问题诊断矩阵

| 问题现象 | 可能原因 | 诊断命令 | 解决方案 |
|:---|:---|:---|:---|
| **Service 无法访问** | Selector 不匹配 | `kubectl get ep <svc>` | 检查 Pod 标签和 Service selector |
| **Endpoints 为空** | Pod 未 Ready | `kubectl get pods -o wide` | 检查 Pod readinessProbe |
| **DNS 解析失败** | CoreDNS 异常 | `kubectl get pods -n kube-system -l k8s-app=kube-dns` | 检查 CoreDNS 状态和日志 |
| **NodePort 无法访问** | 防火墙阻止 | `iptables -L INPUT -n` | 开放 30000-32767 端口 |
| **LoadBalancer Pending** | 无云控制器 | `kubectl describe svc <svc>` | 安装 cloud-controller-manager |
| **源 IP 丢失** | externalTrafficPolicy=Cluster | `kubectl get svc <svc> -o yaml` | 设置 externalTrafficPolicy=Local |
| **连接超时** | NetworkPolicy 阻止 | `kubectl get networkpolicy` | 检查并调整 NetworkPolicy |
| **间歇性失败** | Pod 重启/驱逐 | `kubectl get events` | 检查 Pod 稳定性，增加副本数 |
| **负载不均衡** | 会话亲和性 | `kubectl get svc <svc> -o yaml` | 调整 sessionAffinity 设置 |
| **跨 AZ 延迟高** | 无拓扑感知 | 检查 EndpointSlice zone | 启用拓扑感知路由 |

---

## 11. 版本变更记录 (Version History)

| K8s 版本 | 变更内容 | 特性状态 | 影响 |
|:---|:---|:---|:---|
| **v1.32** | TrafficDistribution 字段增强 | Beta | 更精细的流量控制 |
| **v1.31** | nftables 代理模式 | Beta | 新的代理实现选项 |
| **v1.30** | TrafficDistribution 字段 | Alpha | 流量分布策略 |
| **v1.30** | internalTrafficPolicy | GA | 内部流量策略稳定 |
| **v1.29** | nftables 代理模式 | Alpha | 新代理后端 |
| **v1.29** | Multi-network Service | Alpha | 多网络支持 |
| **v1.28** | Service 端口分配改进 | - | NodePort 分配更灵活 |
| **v1.27** | LoadBalancerClass | GA | LB 控制器类选择稳定 |
| **v1.26** | internalTrafficPolicy | GA | 内部流量策略稳定 |
| **v1.26** | EndpointSlice 增强 | - | 大规模更高效 |
| **v1.25** | 拓扑感知路由 | Beta | 跨 AZ 优化 |
| **v1.24** | LoadBalancerClass | Beta | 多 LB 控制器支持 |
| **v1.24** | allocateLoadBalancerNodePorts | GA | 可禁用 NodePort 分配 |
| **v1.23** | 拓扑感知提示 | Beta | service.kubernetes.io/topology-mode |
| **v1.22** | EndpointSlice 条件字段 | GA | serving, terminating |
| **v1.21** | EndpointSlice | GA | 默认启用 |
| **v1.20** | 双栈 Service | GA | IPv4/IPv6 支持 |

---

## 12. 最佳实践总结 (Best Practices)

### 12.1 Service 配置检查清单

| 检查项 | 推荐配置 | 说明 |
|:---|:---|:---|
| **类型选择** | 内部用 ClusterIP，外部用 LoadBalancer | NodePort 仅用于开发测试 |
| **标签规范** | 使用 app.kubernetes.io/* 标签 | 标准化命名 |
| **端口命名** | 使用有意义的名称 (http, grpc, metrics) | 便于识别和 Istio 协议检测 |
| **appProtocol** | 指定应用层协议 | http, https, grpc, h2c |
| **健康检查** | 配置 readinessProbe | 确保只有健康 Pod 接收流量 |
| **PDB** | 配置 PodDisruptionBudget | 保证服务可用性 |
| **资源限制** | 设置 requests/limits | 防止资源争用 |
| **拓扑分布** | 使用 topologySpreadConstraints | 跨 AZ 高可用 |
| **监控** | 添加 Prometheus 注解 | 服务可观测性 |
| **文档** | 添加 description 注解 | 便于运维理解 |

### 12.2 性能优化建议

| 优化项 | 建议 | 效果 |
|:---|:---|:---|
| **kube-proxy 模式** | 生产使用 IPVS | O(1) 复杂度 |
| **DNS 优化** | 降低 ndots，使用 FQDN | 减少 DNS 查询 |
| **拓扑感知** | 启用拓扑感知路由 | 减少跨 AZ 流量 |
| **连接复用** | gRPC 使用客户端负载均衡 | 避免 HTTP/2 热点 |
| **NodeLocal DNSCache** | 部署本地 DNS 缓存 | 减少 CoreDNS 负载 |

---

> **参考文档**:  
> - [Kubernetes Service 官方文档](https://kubernetes.io/docs/concepts/services-networking/service/)
> - [DNS for Services and Pods](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)
> - [EndpointSlices](https://kubernetes.io/docs/concepts/services-networking/endpoint-slices/)
> - [kube-proxy 配置参考](https://kubernetes.io/docs/reference/config-api/kube-proxy-config.v1alpha1/)
> - [拓扑感知路由](https://kubernetes.io/docs/concepts/services-networking/topology-aware-routing/)

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
