# Kubernetes Service 核心概念与类型 (Service Concepts & Types)

> **适用版本**: Kubernetes v1.25 - v1.32  
> **文档版本**: v2.0 | 生产级 Service 配置参考  
> **最后更新**: 2026-01

## Service 核心架构

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                         Kubernetes Service Architecture                                 │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              Client Layer                                          │ │
│  │                                                                                    │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐                   │ │
│  │  │ Internal Client │  │ External Client │  │ DNS Client      │                   │ │
│  │  │ (Pod)           │  │ (Browser/API)   │  │ (Service Name)  │                   │ │
│  │  │                 │  │                 │  │                 │                   │ │
│  │  │ my-svc:80       │  │ node-ip:30080   │  │ my-svc.ns.svc   │                   │ │
│  │  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘                   │ │
│  │           │                    │                    │                             │ │
│  │           │                    │                    │ DNS Resolution              │ │
│  │           │                    │                    ▼                             │ │
│  │           │                    │           ┌─────────────────┐                   │ │
│  │           │                    │           │    CoreDNS      │                   │ │
│  │           │                    │           │                 │                   │ │
│  │           │                    │           │ A/AAAA/SRV      │                   │ │
│  │           │                    │           │ Records         │                   │ │
│  │           │                    │           └────────┬────────┘                   │ │
│  │           │                    │                    │                             │ │
│  └───────────┼────────────────────┼────────────────────┼─────────────────────────────┘ │
│              │                    │                    │                               │
│              ▼                    ▼                    ▼                               │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              Service Layer                                         │ │
│  │                                                                                    │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                        Service Object                                        │ │ │
│  │  │                                                                              │ │ │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │ │ │
│  │  │  │ ClusterIP   │  │ NodePort    │  │ LoadBalancer│  │ExternalName │        │ │ │
│  │  │  │             │  │             │  │             │  │             │        │ │ │
│  │  │  │ 10.96.0.x   │  │ 30000-32767 │  │ Cloud LB IP │  │ CNAME DNS   │        │ │ │
│  │  │  │ Internal    │  │ Node-level  │  │ External    │  │ External    │        │ │ │
│  │  │  │ only        │  │ access      │  │ access      │  │ mapping     │        │ │ │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘        │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  │                                    │                                              │ │
│  │                                    │ selector: app=myapp                         │ │
│  │                                    ▼                                              │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │                     EndpointSlice (discovery.k8s.io/v1)                      │ │ │
│  │  │                                                                              │ │ │
│  │  │  endpoints:                                                                  │ │ │
│  │  │    - addresses: [10.244.1.10]  conditions: {ready: true, serving: true}     │ │ │
│  │  │    - addresses: [10.244.2.11]  conditions: {ready: true, serving: true}     │ │ │
│  │  │    - addresses: [10.244.3.12]  conditions: {ready: true, serving: true}     │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                    │
│                                    │ Watch & Sync                                      │
│                                    ▼                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              kube-proxy Layer                                      │ │
│  │                                                                                    │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐                   │ │
│  │  │    Node 1       │  │    Node 2       │  │    Node 3       │                   │ │
│  │  │                 │  │                 │  │                 │                   │ │
│  │  │  kube-proxy     │  │  kube-proxy     │  │  kube-proxy     │                   │ │
│  │  │  ┌───────────┐ │  │  ┌───────────┐ │  │  ┌───────────┐ │                   │ │
│  │  │  │ iptables  │ │  │  │ iptables  │ │  │  │ iptables  │ │                   │ │
│  │  │  │    or     │ │  │  │    or     │ │  │  │    or     │ │                   │ │
│  │  │  │   IPVS    │ │  │  │   IPVS    │ │  │  │   IPVS    │ │                   │ │
│  │  │  └───────────┘ │  │  └───────────┘ │  │  └───────────┘ │                   │ │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘                   │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                    │
│                                    │ Traffic Forwarding                                │
│                                    ▼                                                    │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              Pod Layer                                             │ │
│  │                                                                                    │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐                   │ │
│  │  │    Pod 1        │  │    Pod 2        │  │    Pod 3        │                   │ │
│  │  │  10.244.1.10    │  │  10.244.2.11    │  │  10.244.3.12    │                   │ │
│  │  │                 │  │                 │  │                 │                   │ │
│  │  │  ┌───────────┐ │  │  ┌───────────┐ │  │  ┌───────────┐ │                   │ │
│  │  │  │ Container │ │  │  │ Container │ │  │  │ Container │ │                   │ │
│  │  │  │ :8080     │ │  │  │ :8080     │ │  │  │ :8080     │ │                   │ │
│  │  │  └───────────┘ │  │  └───────────┘ │  │  └───────────┘ │                   │ │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘                   │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

## Service 核心概念

### 核心组件详解

| 组件 | 描述 | 作用 | 版本说明 |
|------|------|------|---------|
| **Service** | 为 Pod 集合提供稳定访问入口的抽象 | 定义访问策略和负载均衡 | 所有版本 |
| **ClusterIP** | Service 的虚拟 IP 地址 | 集群内部稳定访问点 | 所有版本 |
| **Selector** | 标签选择器 | 动态关联后端 Pod | 所有版本 |
| **Endpoints** | Pod IP 和端口列表 (旧版) | kube-proxy 配置转发规则 | 已弃用 |
| **EndpointSlice** | 分片的端点对象 (新版) | 大规模集群更高效 | v1.21+ 默认 |
| **kube-proxy** | 节点上的网络代理 | 实现 Service 的流量转发 | 所有版本 |
| **CoreDNS** | 集群 DNS 服务器 | 服务名称解析 | v1.13+ 默认 |

### Service 字段详解

| 字段 | 类型 | 必填 | 描述 | 示例值 |
|------|------|------|------|--------|
| `spec.type` | string | 否 | Service 类型 | ClusterIP, NodePort, LoadBalancer, ExternalName |
| `spec.selector` | map | 否 | 标签选择器 | `app: myapp` |
| `spec.ports` | []ServicePort | 是 | 端口配置列表 | `[{port: 80, targetPort: 8080}]` |
| `spec.clusterIP` | string | 否 | 指定 ClusterIP | None (Headless), 10.96.0.100 |
| `spec.clusterIPs` | []string | 否 | 双栈 IP 列表 | `[10.96.0.100, fd00::100]` |
| `spec.sessionAffinity` | string | 否 | 会话亲和性 | None, ClientIP |
| `spec.externalTrafficPolicy` | string | 否 | 外部流量策略 | Cluster, Local |
| `spec.internalTrafficPolicy` | string | 否 | 内部流量策略 | Cluster, Local |
| `spec.ipFamilyPolicy` | string | 否 | IP 协议族策略 | SingleStack, PreferDualStack, RequireDualStack |
| `spec.loadBalancerIP` | string | 否 | 指定 LB IP (已弃用) | 1.2.3.4 |
| `spec.loadBalancerClass` | string | 否 | LB 控制器类 | service.k8s.aws/nlb |
| `spec.externalName` | string | 否 | 外部 DNS 名称 | my.database.example.com |
| `spec.healthCheckNodePort` | int32 | 否 | 健康检查端口 | 30000-32767 |
| `spec.allocateLoadBalancerNodePorts` | bool | 否 | 是否分配 NodePort | true/false |
| `spec.publishNotReadyAddresses` | bool | 否 | 发布未就绪端点 | true/false |

---

## Service 类型详解

### Service 类型对比矩阵

| 类型 | 访问范围 | ClusterIP | NodePort | 外部 LB | DNS | 适用场景 |
|------|---------|-----------|----------|---------|-----|---------|
| **ClusterIP** | 集群内部 | ✅ | ❌ | ❌ | A记录 | 内部服务通信 |
| **NodePort** | 集群内+节点 | ✅ | ✅ | ❌ | A记录 | 开发测试环境 |
| **LoadBalancer** | 集群内+节点+外部 | ✅ | ✅ | ✅ | A记录 | 生产公网访问 |
| **ExternalName** | DNS 映射 | ❌ | ❌ | ❌ | CNAME | 外部服务代理 |
| **Headless** | 直接 Pod IP | None | ❌ | ❌ | 多 A 记录 | StatefulSet |

### ClusterIP Service

```yaml
# clusterip-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: backend-service
  namespace: production
  labels:
    app: backend
    tier: api
  annotations:
    # 服务描述
    description: "Backend API service for internal communication"
spec:
  type: ClusterIP  # 默认类型，可省略
  
  # IP 协议族配置 (双栈支持)
  ipFamilyPolicy: PreferDualStack
  ipFamilies:
    - IPv4
    - IPv6
  
  # 标签选择器
  selector:
    app: backend
    version: v1
  
  # 端口配置
  ports:
    - name: http
      protocol: TCP
      port: 80           # Service 端口
      targetPort: 8080   # Pod 容器端口
    - name: grpc
      protocol: TCP
      port: 9090
      targetPort: grpc   # 可以使用命名端口
    - name: metrics
      protocol: TCP
      port: 9091
      targetPort: 9091
  
  # 会话亲和性
  sessionAffinity: None
  
  # 内部流量策略 (v1.26+)
  internalTrafficPolicy: Cluster
```

### NodePort Service

```yaml
# nodeport-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: frontend-nodeport
  namespace: production
  labels:
    app: frontend
spec:
  type: NodePort
  
  selector:
    app: frontend
  
  ports:
    - name: http
      protocol: TCP
      port: 80           # Service ClusterIP 端口
      targetPort: 3000   # Pod 端口
      nodePort: 30080    # 节点端口 (30000-32767)
    - name: https
      protocol: TCP
      port: 443
      targetPort: 3443
      nodePort: 30443
  
  # 外部流量策略
  externalTrafficPolicy: Local  # 保留客户端源 IP
```

### LoadBalancer Service

```yaml
# loadbalancer-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: web-loadbalancer
  namespace: production
  labels:
    app: web
  annotations:
    # AWS NLB 配置
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-scheme: "internet-facing"
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-backend-protocol: "tcp"
    service.beta.kubernetes.io/aws-load-balancer-ssl-cert: "arn:aws:acm:us-west-2:123456789:certificate/xxx"
    service.beta.kubernetes.io/aws-load-balancer-ssl-ports: "443"
    
    # Azure LB 配置示例
    # service.beta.kubernetes.io/azure-load-balancer-internal: "true"
    # service.beta.kubernetes.io/azure-load-balancer-resource-group: "my-rg"
    
    # GCP LB 配置示例
    # cloud.google.com/neg: '{"ingress": true}'
    # networking.gke.io/load-balancer-type: "Internal"
    
    # Alibaba Cloud SLB 配置
    # service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
    # service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.small"
spec:
  type: LoadBalancer
  
  # 指定 LoadBalancer 控制器 (v1.24+)
  loadBalancerClass: service.k8s.aws/nlb
  
  # 不分配 NodePort (仅部分云支持)
  allocateLoadBalancerNodePorts: false
  
  selector:
    app: web
  
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
  
  # 健康检查端口
  healthCheckNodePort: 32000
  
  # 限制客户端 IP 范围
  loadBalancerSourceRanges:
    - 10.0.0.0/8
    - 192.168.0.0/16
```

### ExternalName Service

```yaml
# externalname-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: external-database
  namespace: production
  labels:
    app: database
    type: external
spec:
  type: ExternalName
  # 外部 DNS 名称
  externalName: mydb.us-west-2.rds.amazonaws.com
  # 无需 selector 和 ports
---
# 带端口的 ExternalName (通过 Endpoints)
apiVersion: v1
kind: Service
metadata:
  name: legacy-api
  namespace: production
spec:
  type: ClusterIP
  ports:
    - port: 443
      targetPort: 443
# 无 selector，手动管理 Endpoints
---
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: legacy-api-1
  namespace: production
  labels:
    kubernetes.io/service-name: legacy-api
addressType: IPv4
ports:
  - name: ""
    appProtocol: https
    protocol: TCP
    port: 443
endpoints:
  - addresses:
      - "203.0.113.10"
    conditions:
      ready: true
      serving: true
      terminating: false
  - addresses:
      - "203.0.113.11"
    conditions:
      ready: true
      serving: true
      terminating: false
```

---

## Headless Service (无头服务)

### Headless Service 架构

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                         Headless Service Architecture                                   │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  Regular Service (clusterIP: 10.96.0.1)     Headless Service (clusterIP: None)         │
│                                                                                         │
│  ┌─────────────────────────────────┐        ┌─────────────────────────────────┐       │
│  │         DNS Query               │        │         DNS Query               │       │
│  │    my-svc.ns.svc.cluster.local  │        │    my-svc.ns.svc.cluster.local  │       │
│  └─────────────────────────────────┘        └─────────────────────────────────┘       │
│                  │                                          │                          │
│                  ▼                                          ▼                          │
│  ┌─────────────────────────────────┐        ┌─────────────────────────────────┐       │
│  │         DNS Response            │        │         DNS Response            │       │
│  │                                 │        │                                 │       │
│  │  A Record: 10.96.0.1 (VIP)     │        │  A Records (multiple):          │       │
│  │                                 │        │    10.244.1.10 (Pod-0)          │       │
│  │                                 │        │    10.244.2.11 (Pod-1)          │       │
│  │                                 │        │    10.244.3.12 (Pod-2)          │       │
│  └─────────────────────────────────┘        └─────────────────────────────────┘       │
│                  │                                          │                          │
│                  ▼                                          ▼                          │
│  ┌─────────────────────────────────┐        ┌─────────────────────────────────┐       │
│  │       kube-proxy                │        │     Client-side Selection       │       │
│  │       Load Balancing            │        │     (Application decides)       │       │
│  │                                 │        │                                 │       │
│  │  Random/RR selection            │        │  • First IP                     │       │
│  │  to backend Pods                │        │  • Specific Pod by name         │       │
│  │                                 │        │  • Custom load balancing        │       │
│  └─────────────────────────────────┘        └─────────────────────────────────┘       │
│                  │                                          │                          │
│                  ▼                                          ▼                          │
│  ┌─────────────────────────────────┐        ┌─────────────────────────────────┐       │
│  │         Backend Pods            │        │         Backend Pods            │       │
│  │  ┌───────┐ ┌───────┐ ┌───────┐ │        │  ┌───────┐ ┌───────┐ ┌───────┐ │       │
│  │  │Pod-0  │ │Pod-1  │ │Pod-2  │ │        │  │Pod-0  │ │Pod-1  │ │Pod-2  │ │       │
│  │  └───────┘ └───────┘ └───────┘ │        │  └───────┘ └───────┘ └───────┘ │       │
│  └─────────────────────────────────┘        └─────────────────────────────────┘       │
│                                                                                         │
│  DNS Records for Headless Service with StatefulSet:                                    │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐  │
│  │  Service: my-svc.ns.svc.cluster.local                                            │  │
│  │  └── A Records: 10.244.1.10, 10.244.2.11, 10.244.3.12                           │  │
│  │                                                                                   │  │
│  │  Pod DNS Names (StatefulSet specific):                                           │  │
│  │  ├── pod-0.my-svc.ns.svc.cluster.local → 10.244.1.10                            │  │
│  │  ├── pod-1.my-svc.ns.svc.cluster.local → 10.244.2.11                            │  │
│  │  └── pod-2.my-svc.ns.svc.cluster.local → 10.244.3.12                            │  │
│  └─────────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### Headless Service 配置

```yaml
# headless-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
  namespace: database
  labels:
    app: mysql
spec:
  # 关键配置：设置为 None
  clusterIP: None
  
  selector:
    app: mysql
  
  ports:
    - name: mysql
      port: 3306
      targetPort: 3306
  
  # 发布未就绪的端点 (可选)
  publishNotReadyAddresses: true
---
# 配合 StatefulSet 使用
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: database
spec:
  serviceName: mysql-headless  # 关联 Headless Service
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
          env:
            - name: MYSQL_ROOT_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: root-password
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: fast-storage
        resources:
          requests:
            storage: 100Gi
```

### Headless Service 使用场景

| 场景 | 说明 | 示例应用 |
|------|------|---------|
| **StatefulSet** | 需要稳定网络标识的有状态应用 | MySQL, PostgreSQL, MongoDB |
| **分布式系统** | 需要节点间点对点通信 | etcd, ZooKeeper, Kafka |
| **自定义负载均衡** | 客户端需要自行选择后端 | gRPC 负载均衡, 自定义调度 |
| **服务发现** | 直接获取所有 Pod IP 列表 | 集群协调服务 |
| **Leader 选举** | 需要直接连接到特定 Pod | Raft/Paxos 集群 |

---

## 多端口 Service 配置

```yaml
# multi-port-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: multi-port-app
  namespace: production
spec:
  type: ClusterIP
  selector:
    app: multi-port-app
  ports:
    # HTTP 端口
    - name: http
      protocol: TCP
      port: 80
      targetPort: 8080
    
    # HTTPS 端口
    - name: https
      protocol: TCP
      port: 443
      targetPort: 8443
    
    # gRPC 端口
    - name: grpc
      protocol: TCP
      port: 9090
      targetPort: grpc  # 使用命名端口
      appProtocol: grpc
    
    # Metrics 端口
    - name: metrics
      protocol: TCP
      port: 9091
      targetPort: metrics
    
    # Admin 端口
    - name: admin
      protocol: TCP
      port: 8081
      targetPort: 8081
    
    # UDP 端口 (如 DNS)
    - name: dns-udp
      protocol: UDP
      port: 53
      targetPort: 53
    
    # TCP DNS 端口
    - name: dns-tcp
      protocol: TCP
      port: 53
      targetPort: 53
```

---

## Service 与 DNS 集成

### DNS 记录类型

| 记录类型 | 格式 | 用途 | 示例 |
|---------|------|------|------|
| **A/AAAA** | `<svc>.<ns>.svc.cluster.local` | Service ClusterIP | `my-svc.default.svc.cluster.local → 10.96.0.1` |
| **SRV** | `_<port>._<proto>.<svc>.<ns>.svc.cluster.local` | 端口和协议发现 | `_http._tcp.my-svc.default.svc.cluster.local` |
| **PTR** | `<ip>.in-addr.arpa` | 反向 DNS 查询 | `1.0.96.10.in-addr.arpa → my-svc.default.svc.cluster.local` |
| **Pod A** | `<pod-ip>.<ns>.pod.cluster.local` | Pod IP 查询 | `10-244-1-10.default.pod.cluster.local` |
| **StatefulSet Pod** | `<pod-name>.<svc>.<ns>.svc.cluster.local` | StatefulSet Pod | `web-0.nginx.default.svc.cluster.local` |

### DNS 配置示例

```yaml
# pod-with-dns-config.yaml
apiVersion: v1
kind: Pod
metadata:
  name: dns-example
  namespace: production
spec:
  containers:
    - name: app
      image: myapp:latest
  dnsPolicy: ClusterFirst  # 默认策略
  dnsConfig:
    nameservers:
      - 10.96.0.10  # CoreDNS ClusterIP
    searches:
      - production.svc.cluster.local
      - svc.cluster.local
      - cluster.local
    options:
      - name: ndots
        value: "5"
      - name: timeout
        value: "2"
      - name: attempts
        value: "3"
```

---

## 生产环境 Service 配置示例

### 完整的生产级 Service

```yaml
# production-service-complete.yaml
apiVersion: v1
kind: Service
metadata:
  name: api-gateway
  namespace: production
  labels:
    app: api-gateway
    team: platform
    environment: production
    version: v2.0.0
  annotations:
    # 文档
    description: "API Gateway for external traffic"
    owner: "platform-team@example.com"
    
    # Prometheus 监控
    prometheus.io/scrape: "true"
    prometheus.io/port: "9091"
    prometheus.io/path: "/metrics"
    
    # 外部 DNS 配置
    external-dns.alpha.kubernetes.io/hostname: "api.example.com"
    external-dns.alpha.kubernetes.io/ttl: "300"
    
    # AWS NLB 配置
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-scheme: "internet-facing"
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-target-group-attributes: "deregistration_delay.timeout_seconds=30"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-healthy-threshold: "2"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-unhealthy-threshold: "2"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-interval: "10"
spec:
  type: LoadBalancer
  loadBalancerClass: service.k8s.aws/nlb
  
  selector:
    app: api-gateway
    version: v2
  
  ports:
    - name: http
      protocol: TCP
      port: 80
      targetPort: 8080
    - name: https
      protocol: TCP
      port: 443
      targetPort: 8443
    - name: grpc
      protocol: TCP
      port: 9090
      targetPort: 9090
      appProtocol: grpc
    - name: metrics
      protocol: TCP
      port: 9091
      targetPort: 9091
  
  # 流量策略
  externalTrafficPolicy: Local
  internalTrafficPolicy: Cluster
  
  # 会话亲和性
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 3600
  
  # IP 源范围限制
  loadBalancerSourceRanges:
    - 0.0.0.0/0  # 生产环境应更严格
  
  # 健康检查端口
  healthCheckNodePort: 32100
---
# 对应的 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-gateway
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: api-gateway
      version: v2
  template:
    metadata:
      labels:
        app: api-gateway
        version: v2
    spec:
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: api-gateway
      containers:
        - name: api-gateway
          image: api-gateway:v2.0.0
          ports:
            - name: http
              containerPort: 8080
            - name: https
              containerPort: 8443
            - name: grpc
              containerPort: 9090
            - name: metrics
              containerPort: 9091
          readinessProbe:
            httpGet:
              path: /health/ready
              port: 8080
            initialDelaySeconds: 5
            periodSeconds: 10
          livenessProbe:
            httpGet:
              path: /health/live
              port: 8080
            initialDelaySeconds: 15
            periodSeconds: 20
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: 2
              memory: 2Gi
---
# PodDisruptionBudget
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: api-gateway-pdb
  namespace: production
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: api-gateway
```

---

## 故障排除

### 常见问题诊断命令

```bash
#!/bin/bash
# service-troubleshoot.sh

# 查看 Service 详情
kubectl get svc -n production -o wide
kubectl describe svc my-service -n production

# 查看 Endpoints
kubectl get endpoints my-service -n production
kubectl get endpointslices -l kubernetes.io/service-name=my-service -n production

# 检查 DNS 解析
kubectl run -it --rm debug --image=busybox --restart=Never -- nslookup my-service.production.svc.cluster.local

# 测试 Service 连通性
kubectl run -it --rm debug --image=curlimages/curl --restart=Never -- curl -v http://my-service.production.svc.cluster.local

# 查看 kube-proxy 日志
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=100

# 检查 iptables 规则 (在节点上执行)
iptables -t nat -L KUBE-SERVICES -n | grep my-service

# 检查 IPVS 规则 (在节点上执行)
ipvsadm -Ln | grep -A 5 <ClusterIP>
```

### 常见问题和解决方案

| 问题 | 可能原因 | 解决方案 |
|------|---------|---------|
| Service 无法访问 | Selector 不匹配 | 检查 Pod 标签和 Service selector |
| Endpoints 为空 | Pod 未就绪 | 检查 Pod readinessProbe |
| DNS 解析失败 | CoreDNS 异常 | 检查 CoreDNS Pod 状态 |
| NodePort 无法访问 | 防火墙阻止 | 开放节点端口 30000-32767 |
| LoadBalancer pending | 无云控制器 | 安装或配置 cloud-controller-manager |
| 源 IP 丢失 | externalTrafficPolicy | 设置为 Local |
| 连接超时 | 网络策略限制 | 检查 NetworkPolicy |

---

## 版本变更记录

| K8s版本 | 变更内容 | 影响 |
|--------|---------|------|
| v1.32 | Service 拓扑感知增强 | 更精细的区域路由 |
| v1.31 | LoadBalancer IP 模式改进 | 更灵活的 LB 配置 |
| v1.30 | internalTrafficPolicy GA | 内部流量策略稳定 |
| v1.29 | Multi-network Service alpha | 多网络支持 |
| v1.28 | Service 端口分配改进 | NodePort 更灵活 |
| v1.27 | LoadBalancerClass GA | LB 控制器选择 |
| v1.26 | EndpointSlice 改进 | 大规模更高效 |
| v1.25 | 拓扑感知路由 beta | 跨 AZ 优化 |

---

> **参考文档**:  
> - [Kubernetes Service 官方文档](https://kubernetes.io/docs/concepts/services-networking/service/)
> - [DNS for Services and Pods](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)
> - [EndpointSlices](https://kubernetes.io/docs/concepts/services-networking/endpoint-slices/)

---

*Kusheet - Kubernetes 知识速查表项目*
