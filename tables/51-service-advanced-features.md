# Service 高级特性与应用案例 (Service Advanced Features)

> **适用版本**: Kubernetes v1.25 - v1.32  
> **文档版本**: v2.0 | 生产级 Service 高级特性参考  
> **最后更新**: 2026-01

## Service 高级特性架构

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                        Service Advanced Features Architecture                           │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              Traffic Policies                                      │ │
│  │                                                                                    │ │
│  │  ┌─────────────────────────┐      ┌─────────────────────────┐                    │ │
│  │  │  externalTrafficPolicy  │      │  internalTrafficPolicy  │                    │ │
│  │  │                         │      │                         │                    │ │
│  │  │  ┌───────────────────┐ │      │  ┌───────────────────┐ │                    │ │
│  │  │  │ Cluster (default) │ │      │  │ Cluster (default) │ │                    │ │
│  │  │  │ • 负载均衡优先    │ │      │  │ • 跨节点转发      │ │                    │ │
│  │  │  │ • 源 IP 丢失      │ │      │  │ • 均匀分布        │ │                    │ │
│  │  │  └───────────────────┘ │      │  └───────────────────┘ │                    │ │
│  │  │  ┌───────────────────┐ │      │  ┌───────────────────┐ │                    │ │
│  │  │  │ Local             │ │      │  │ Local             │ │                    │ │
│  │  │  │ • 保留源 IP       │ │      │  │ • 仅本地节点      │ │                    │ │
│  │  │  │ • 可能负载不均    │ │      │  │ • 低延迟          │ │                    │ │
│  │  │  └───────────────────┘ │      │  └───────────────────┘ │                    │ │
│  │  └─────────────────────────┘      └─────────────────────────┘                    │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                         │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                            Session Affinity                                        │ │
│  │                                                                                    │ │
│  │  Client IP → Hash → Same Pod (within timeout)                                     │ │
│  │                                                                                    │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│  │  │  Request 1 (IP: 10.0.0.1)  ───────────────────────────────► Pod A          │ │ │
│  │  │  Request 2 (IP: 10.0.0.1)  ───────────────────────────────► Pod A (same)   │ │ │
│  │  │  Request 3 (IP: 10.0.0.2)  ───────────────────────────────► Pod B          │ │ │
│  │  │  Request 4 (IP: 10.0.0.1)  ───────────────────────────────► Pod A (same)   │ │ │
│  │  └─────────────────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                         │
│  ┌───────────────────────────────────────────────────────────────────────────────────┐ │
│  │                        Topology Aware Routing                                      │ │
│  │                                                                                    │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐                   │ │
│  │  │   Zone A        │  │   Zone B        │  │   Zone C        │                   │ │
│  │  │                 │  │                 │  │                 │                   │ │
│  │  │  Client ──────► │  │                 │  │                 │                   │ │
│  │  │       │         │  │                 │  │                 │                   │ │
│  │  │       ▼         │  │                 │  │                 │                   │ │
│  │  │  ┌─────────┐   │  │  ┌─────────┐   │  │  ┌─────────┐   │                   │ │
│  │  │  │ Pod A-1 │◄──│──│──│ Pod B-1 │   │  │  │ Pod C-1 │   │                   │ │
│  │  │  │ (local) │   │  │  │         │   │  │  │         │   │                   │ │
│  │  │  └─────────┘   │  │  └─────────┘   │  │  └─────────┘   │                   │ │
│  │  │  ┌─────────┐   │  │  ┌─────────┐   │  │  ┌─────────┐   │                   │ │
│  │  │  │ Pod A-2 │◄──│  │  │ Pod B-2 │   │  │  │ Pod C-2 │   │                   │ │
│  │  │  │ (local) │   │  │  │         │   │  │  │         │   │                   │ │
│  │  │  └─────────┘   │  │  └─────────┘   │  │  └─────────┘   │                   │ │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘                   │ │
│  │                                                                                    │ │
│  │  优先路由到同 Zone 的 Pod，减少跨 AZ 流量和延迟                                   │ │
│  └───────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

## 流量策略详解

### externalTrafficPolicy 对比

| 特性 | Cluster (默认) | Local |
|------|---------------|-------|
| **流量路径** | 可跨节点转发 | 仅本地节点 Pod |
| **源 IP** | 被 SNAT 替换 | 保留原始 IP |
| **负载均衡** | 均匀分布 | 可能不均匀 |
| **网络跳数** | 可能多一跳 | 最少跳数 |
| **延迟** | 较高 | 较低 |
| **健康检查** | 标准 | 需要 NodePort 健康检查 |
| **Pod 不在节点** | 正常转发 | 流量丢弃 |
| **适用场景** | 通用场景 | 需要源 IP 的应用 |

### externalTrafficPolicy 流量路径

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                    externalTrafficPolicy Flow Comparison                            │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  Cluster Mode (默认):                                                              │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                                                                              │   │
│  │  External        LoadBalancer      Node 1           Node 2                  │   │
│  │  Client          (Cloud)           (no pods)        (has pods)              │   │
│  │                                                                              │   │
│  │    │                │                │                │                      │   │
│  │    │ src: 1.2.3.4   │                │                │                      │   │
│  │    └───────────────►│                │                │                      │   │
│  │                     │ src: 1.2.3.4   │                │                      │   │
│  │                     └───────────────►│                │                      │   │
│  │                                      │ src: Node1 IP  │   ◄── SNAT!         │   │
│  │                                      └───────────────►│                      │   │
│  │                                                       │                      │   │
│  │                                                       ▼                      │   │
│  │                                                    ┌──────┐                  │   │
│  │                                                    │ Pod  │                  │   │
│  │                                                    │ sees │                  │   │
│  │                                                    │Node1 │ ◄── 源 IP 丢失   │   │
│  │                                                    │ IP   │                  │   │
│  │                                                    └──────┘                  │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                     │
│  Local Mode:                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                                                                              │   │
│  │  External        LoadBalancer      Node 1           Node 2                  │   │
│  │  Client          (Cloud)           (no pods)        (has pods)              │   │
│  │                                                                              │   │
│  │    │                │                │                │                      │   │
│  │    │ src: 1.2.3.4   │                │                │                      │   │
│  │    └───────────────►│                │                │                      │   │
│  │                     │                │                │                      │   │
│  │                     │ Health check   │                │                      │   │
│  │                     │ Node1: FAIL ──►│ (no local pod) │                      │   │
│  │                     │ Node2: PASS ──────────────────►│                      │   │
│  │                     │                                │                      │   │
│  │                     │ src: 1.2.3.4                   │                      │   │
│  │                     └───────────────────────────────►│                      │   │
│  │                                                       │                      │   │
│  │                                                       ▼                      │   │
│  │                                                    ┌──────┐                  │   │
│  │                                                    │ Pod  │                  │   │
│  │                                                    │ sees │                  │   │
│  │                                                    │1.2.3.│ ◄── 源 IP 保留   │   │
│  │                                                    │  4   │                  │   │
│  │                                                    └──────┘                  │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### externalTrafficPolicy 配置示例

```yaml
# external-traffic-policy-examples.yaml
---
# 保留源 IP 的 LoadBalancer Service
apiVersion: v1
kind: Service
metadata:
  name: web-preserve-source-ip
  namespace: production
  annotations:
    # AWS NLB 推荐配置
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local  # 保留源 IP
  
  selector:
    app: web
  
  ports:
    - name: http
      port: 80
      targetPort: 8080
    - name: https
      port: 443
      targetPort: 8443
  
  # 健康检查端口 (Local 模式自动使用)
  healthCheckNodePort: 32000
---
# 确保 Pod 均匀分布以配合 Local 策略
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web
  namespace: production
spec:
  replicas: 6
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      # 确保每个节点至少有一个 Pod
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: kubernetes.io/hostname
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: web
      
      # 或使用 Pod Anti-Affinity
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    app: web
                topologyKey: kubernetes.io/hostname
      
      containers:
        - name: web
          image: nginx:latest
          ports:
            - containerPort: 8080
            - containerPort: 8443
```

### internalTrafficPolicy 配置

```yaml
# internal-traffic-policy.yaml
apiVersion: v1
kind: Service
metadata:
  name: cache-local
  namespace: production
spec:
  type: ClusterIP
  
  # 内部流量策略 (v1.26+ GA)
  internalTrafficPolicy: Local  # 优先本地 Pod
  
  selector:
    app: cache
  
  ports:
    - port: 6379
      targetPort: 6379
---
# DaemonSet 部署的本地缓存
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: cache
  namespace: production
spec:
  selector:
    matchLabels:
      app: cache
  template:
    metadata:
      labels:
        app: cache
    spec:
      containers:
        - name: redis
          image: redis:7-alpine
          ports:
            - containerPort: 6379
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 512Mi
```

---

## 会话亲和性 (Session Affinity)

### 会话亲和性工作原理

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        Session Affinity Mechanism                                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  sessionAffinity: ClientIP                                                         │
│                                                                                     │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                           kube-proxy                                         │   │
│  │                                                                              │   │
│  │  Affinity Table (per Service):                                              │   │
│  │  ┌─────────────────────────────────────────────────────────────────────┐   │   │
│  │  │  Client IP    │  Backend Pod      │  Last Access    │  Expires      │   │   │
│  │  │───────────────┼───────────────────┼─────────────────┼───────────────│   │   │
│  │  │  10.0.0.1     │  10.244.1.10:8080 │  12:00:00       │  15:00:00     │   │   │
│  │  │  10.0.0.2     │  10.244.2.11:8080 │  12:05:00       │  15:05:00     │   │   │
│  │  │  10.0.0.3     │  10.244.1.10:8080 │  12:10:00       │  15:10:00     │   │   │
│  │  └─────────────────────────────────────────────────────────────────────┘   │   │
│  │                                                                              │   │
│  │  Flow:                                                                       │   │
│  │  1. Client 10.0.0.1 → Service VIP                                           │   │
│  │  2. Check affinity table for 10.0.0.1                                       │   │
│  │  3. If found and not expired → route to cached backend                      │   │
│  │  4. If not found → select backend, update table                             │   │
│  │  5. If expired → remove entry, select new backend                           │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                     │
│  iptables 实现 (Cluster mode):                                                     │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │  -A KUBE-SVC-XXX -m recent --name affinity-XXX --rcheck --seconds 10800     │   │
│  │      --reap -j KUBE-SEP-TARGET                                               │   │
│  │  -A KUBE-SVC-XXX -m statistic --mode random --probability 0.33333           │   │
│  │      -j KUBE-SEP-AAA                                                         │   │
│  │  ...                                                                         │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                     │
│  IPVS 实现:                                                                        │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │  ipvsadm -A -t 10.96.0.100:80 -s rr -p 10800                                │   │
│  │                                   └── persistence timeout (seconds)          │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 会话亲和性配置示例

```yaml
# session-affinity-examples.yaml
---
# 基于 ClientIP 的会话亲和性
apiVersion: v1
kind: Service
metadata:
  name: stateful-app
  namespace: production
spec:
  type: ClusterIP
  
  # 启用会话亲和性
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      # 会话保持时间 (秒)
      # 默认: 10800 (3小时)
      # 最大: 86400 (24小时)
      timeoutSeconds: 3600  # 1小时
  
  selector:
    app: stateful-app
  
  ports:
    - port: 80
      targetPort: 8080
---
# WebSocket 应用 (需要长连接会话保持)
apiVersion: v1
kind: Service
metadata:
  name: websocket-app
  namespace: production
  annotations:
    # Nginx Ingress WebSocket 支持
    nginx.ingress.kubernetes.io/proxy-read-timeout: "3600"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "3600"
spec:
  type: ClusterIP
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 7200  # 2小时
  selector:
    app: websocket-app
  ports:
    - port: 80
      targetPort: 8080
---
# 游戏服务器 (长会话)
apiVersion: v1
kind: Service
metadata:
  name: game-server
  namespace: production
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local  # 保留源 IP + 会话亲和
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 86400  # 24小时 (最大值)
  selector:
    app: game-server
  ports:
    - name: game
      port: 7777
      targetPort: 7777
      protocol: UDP
```

### 会话亲和性注意事项

| 场景 | 问题 | 解决方案 |
|------|------|---------|
| **NAT 后多客户端** | 多个客户端共享同一出口 IP | 使用应用层会话管理 (Redis) |
| **Pod 重启** | 会话中断 | 使用外部状态存储 |
| **滚动更新** | 部分会话中断 | 使用 PDB + 优雅终止 |
| **负载不均** | 热门客户端集中到单个 Pod | 监控 + 动态扩容 |
| **超时配置** | 会话过期 | 根据业务调整 timeout |

---

## 拓扑感知路由 (Topology Aware Routing)

### 拓扑感知工作原理

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                     Topology Aware Routing (TAR) Mechanism                          │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  Prerequisites:                                                                     │
│  • Nodes labeled with topology.kubernetes.io/zone                                  │
│  • Service annotated with service.kubernetes.io/topology-mode: Auto                │
│  • Sufficient endpoints in each zone                                               │
│                                                                                     │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                        Control Plane Processing                              │   │
│  │                                                                              │   │
│  │  1. EndpointSlice Controller calculates zone distribution                   │   │
│  │  2. For each zone, generate hints for endpoints                             │   │
│  │  3. kube-proxy receives EndpointSlice with hints                            │   │
│  │  4. kube-proxy programs rules using only local zone endpoints               │   │
│  │                                                                              │   │
│  │  EndpointSlice with Topology Hints:                                         │   │
│  │  ┌───────────────────────────────────────────────────────────────────────┐ │   │
│  │  │  endpoints:                                                            │ │   │
│  │  │    - addresses: ["10.244.1.10"]                                       │ │   │
│  │  │      zone: "us-west-2a"                                               │ │   │
│  │  │      hints:                                                            │ │   │
│  │  │        forZones:                                                       │ │   │
│  │  │          - name: "us-west-2a"  ◄── 此端点服务于 us-west-2a 区域       │ │   │
│  │  │    - addresses: ["10.244.2.11"]                                       │ │   │
│  │  │      zone: "us-west-2b"                                               │ │   │
│  │  │      hints:                                                            │ │   │
│  │  │        forZones:                                                       │ │   │
│  │  │          - name: "us-west-2b"                                         │ │   │
│  │  └───────────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                     │
│  Zone Traffic Distribution:                                                        │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                                                                              │   │
│  │  us-west-2a                    us-west-2b                    us-west-2c     │   │
│  │  ┌─────────────────┐          ┌─────────────────┐          ┌─────────────┐ │   │
│  │  │                 │          │                 │          │             │ │   │
│  │  │  Client A ─────►│          │  Client B ─────►│          │  Client C   │ │   │
│  │  │       │         │          │       │         │          │       │     │ │   │
│  │  │       ▼         │          │       ▼         │          │       ▼     │ │   │
│  │  │  ┌─────────┐   │          │  ┌─────────┐   │          │  No local   │ │   │
│  │  │  │ Pod A-1 │◄──│          │  │ Pod B-1 │◄──│          │  endpoints  │ │   │
│  │  │  └─────────┘   │          │  └─────────┘   │          │       │     │ │   │
│  │  │  ┌─────────┐   │          │  ┌─────────┐   │          │       │     │ │   │
│  │  │  │ Pod A-2 │◄──│          │  │ Pod B-2 │◄──│          │       ▼     │ │   │
│  │  │  └─────────┘   │          │  └─────────┘   │          │  Failover   │ │   │
│  │  │                 │          │                 │          │  to other   │ │   │
│  │  │  100% local     │          │  100% local     │          │  zones      │ │   │
│  │  │  traffic        │          │  traffic        │          │             │ │   │
│  │  └─────────────────┘          └─────────────────┘          └─────────────┘ │   │
│  │                                                                              │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                     │
│  Benefits:                                                                         │
│  • Reduced cross-zone network traffic (cost savings)                              │
│  • Lower latency (same-zone routing)                                              │
│  • Automatic failover when zone has no healthy endpoints                          │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 拓扑感知路由配置

```yaml
# topology-aware-routing.yaml
---
# 启用拓扑感知路由的 Service
apiVersion: v1
kind: Service
metadata:
  name: backend-api
  namespace: production
  annotations:
    # 启用拓扑感知路由 (v1.27+ 推荐)
    service.kubernetes.io/topology-mode: Auto
    
    # 旧版注解 (v1.21-v1.26)
    # service.kubernetes.io/topology-aware-hints: Auto
spec:
  type: ClusterIP
  selector:
    app: backend-api
  ports:
    - port: 80
      targetPort: 8080
---
# 多区域部署的 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend-api
  namespace: production
spec:
  replicas: 9  # 每个 AZ 至少 3 个
  selector:
    matchLabels:
      app: backend-api
  template:
    metadata:
      labels:
        app: backend-api
    spec:
      # 确保跨区域均匀分布
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: backend-api
      
      containers:
        - name: api
          image: backend-api:latest
          ports:
            - containerPort: 8080
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
---
# 验证拓扑感知生效
# kubectl get endpointslices -l kubernetes.io/service-name=backend-api -o yaml
# 查看 endpoints[].hints.forZones 字段
```

### 拓扑感知条件和限制

| 条件 | 说明 |
|------|------|
| **节点标签** | 必须有 `topology.kubernetes.io/zone` 标签 |
| **最小端点数** | 每个区域至少 3 个端点 (可配置) |
| **端点分布** | 端点在各区域分布相对均匀 |
| **Service 类型** | ClusterIP, NodePort, LoadBalancer |
| **协议** | TCP 和 UDP |

---

## 无选择器 Service (Services without Selectors)

### 无选择器 Service 架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                    Services without Selectors Architecture                          │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  Use Cases:                                                                        │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                                                                              │   │
│  │  1. External Database        2. Cross-Cluster Service  3. Migration Bridge  │   │
│  │                                                                              │   │
│  │  ┌───────────────┐           ┌───────────────┐        ┌───────────────┐    │   │
│  │  │   K8s Pod     │           │   Cluster A   │        │   Legacy App  │    │   │
│  │  │               │           │               │        │   (VM)        │    │   │
│  │  │ db-svc:3306 ──┼──────────►│ ┌───────────┐ │        │               │    │   │
│  │  │               │           │ │ Service B │ │        │ 192.168.1.100 │    │   │
│  │  └───────────────┘           │ └───────────┘ │        └───────────────┘    │   │
│  │         │                    └───────────────┘               ▲              │   │
│  │         │                           │                        │              │   │
│  │         ▼                           ▼                        │              │   │
│  │  ┌───────────────┐           ┌───────────────┐        ┌───────────────┐    │   │
│  │  │ EndpointSlice │           │ EndpointSlice │        │ EndpointSlice │    │   │
│  │  │               │           │               │        │               │    │   │
│  │  │ RDS endpoint  │           │ Cluster B IPs │        │ VM IP address │    │   │
│  │  │ 10.0.1.50     │           │ 10.1.0.x      │        │ 192.168.1.100 │    │   │
│  │  └───────────────┘           └───────────────┘        └───────────────┘    │   │
│  │         │                           │                        │              │   │
│  │         ▼                           ▼                        ▼              │   │
│  │  ┌───────────────┐           ┌───────────────┐        ┌───────────────┐    │   │
│  │  │  External DB  │           │  Remote       │        │  Legacy App   │    │   │
│  │  │  (RDS/Cloud)  │           │  Cluster B    │        │  (Gradual     │    │   │
│  │  │               │           │  Pods         │        │   Migration)  │    │   │
│  │  └───────────────┘           └───────────────┘        └───────────────┘    │   │
│  │                                                                              │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 无选择器 Service 配置示例

```yaml
# service-without-selector.yaml
---
# 1. 外部数据库代理
apiVersion: v1
kind: Service
metadata:
  name: external-mysql
  namespace: production
spec:
  type: ClusterIP
  ports:
    - port: 3306
      targetPort: 3306
  # 无 selector
---
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: external-mysql-1
  namespace: production
  labels:
    kubernetes.io/service-name: external-mysql
addressType: IPv4
ports:
  - name: ""
    appProtocol: mysql
    protocol: TCP
    port: 3306
endpoints:
  - addresses:
      - "10.0.1.50"  # RDS Primary
    conditions:
      ready: true
  - addresses:
      - "10.0.1.51"  # RDS Replica
    conditions:
      ready: true
---
# 2. 外部 API 服务代理
apiVersion: v1
kind: Service
metadata:
  name: external-api
  namespace: production
spec:
  type: ClusterIP
  ports:
    - name: https
      port: 443
      targetPort: 443
---
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: external-api-1
  namespace: production
  labels:
    kubernetes.io/service-name: external-api
addressType: IPv4
ports:
  - name: https
    appProtocol: https
    protocol: TCP
    port: 443
endpoints:
  - addresses:
      - "203.0.113.10"
    conditions:
      ready: true
  - addresses:
      - "203.0.113.11"
    conditions:
      ready: true
---
# 3. 跨集群服务发现
apiVersion: v1
kind: Service
metadata:
  name: remote-service
  namespace: production
spec:
  type: ClusterIP
  ports:
    - port: 8080
      targetPort: 8080
---
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: remote-service-1
  namespace: production
  labels:
    kubernetes.io/service-name: remote-service
addressType: IPv4
ports:
  - port: 8080
    protocol: TCP
endpoints:
  - addresses:
      - "10.1.0.10"  # Remote cluster Pod IP
    conditions:
      ready: true
    zone: "us-east-1a"
  - addresses:
      - "10.1.0.11"
    conditions:
      ready: true
    zone: "us-east-1b"
```

### 动态更新 EndpointSlice

```bash
#!/bin/bash
# update-external-endpoints.sh
# 动态更新外部服务端点

SERVICE_NAME="external-mysql"
NAMESPACE="production"

# 获取新的端点 IP (例如从云 API)
get_rds_endpoints() {
    # 示例: 从 AWS RDS 获取端点
    aws rds describe-db-instances \
        --db-instance-identifier my-rds \
        --query 'DBInstances[0].Endpoint.Address' \
        --output text
}

# 更新 EndpointSlice
update_endpoints() {
    local primary_ip="$1"
    local replica_ip="$2"
    
    cat <<EOF | kubectl apply -f -
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: ${SERVICE_NAME}-1
  namespace: ${NAMESPACE}
  labels:
    kubernetes.io/service-name: ${SERVICE_NAME}
addressType: IPv4
ports:
  - name: mysql
    protocol: TCP
    port: 3306
endpoints:
  - addresses:
      - "${primary_ip}"
    conditions:
      ready: true
  - addresses:
      - "${replica_ip}"
    conditions:
      ready: true
EOF
}

# 主函数
main() {
    local primary=$(get_rds_endpoints)
    local replica="${primary/primary/replica}"
    
    update_endpoints "$primary" "$replica"
    echo "Endpoints updated: $primary, $replica"
}

main
```

---

## 生产环境最佳实践

### Service 配置检查清单

| 检查项 | 说明 | 建议配置 |
|-------|------|---------|
| **流量策略** | 是否需要保留源 IP | Web 应用使用 Local |
| **会话亲和** | 是否需要粘性会话 | 有状态应用启用 ClientIP |
| **拓扑感知** | 是否需要区域路由 | 多 AZ 部署启用 |
| **健康检查** | Local 策略需要 | 配置 healthCheckNodePort |
| **Pod 分布** | 配合 Local 策略 | 使用 topologySpreadConstraints |
| **超时配置** | 会话超时时间 | 根据业务需求调整 |
| **监控告警** | Service 健康监控 | 配置 Prometheus 规则 |

### 监控指标

```yaml
# service-monitoring.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: service-advanced-rules
  namespace: monitoring
spec:
  groups:
    - name: service.advanced
      rules:
        - alert: ServiceTopologyHintsNotPopulated
          expr: |
            kube_endpointslice_info{topology_aware_hints_configured="true"} 
            unless 
            kube_endpointslice_hints_populated == 1
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "Topology hints not populated"
        
        - alert: LocalTrafficPolicyNoLocalEndpoints
          expr: |
            kube_service_spec_external_traffic_policy{external_traffic_policy="Local"} 
            * on(service, namespace) 
            (count by (service, namespace) (kube_endpoint_address_available) == 0)
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Service with Local policy has no local endpoints on some nodes"
```

---

## 版本变更记录

| K8s版本 | 变更内容 | 影响 |
|--------|---------|------|
| v1.32 | 拓扑感知路由增强 | 更智能的区域路由 |
| v1.31 | Service 内部流量策略改进 | 更灵活配置 |
| v1.30 | EndpointSlice 性能优化 | 大规模更高效 |
| v1.27 | topology-mode 注解 | 替代旧的 hints 注解 |
| v1.26 | internalTrafficPolicy GA | 内部流量策略稳定 |
| v1.25 | 拓扑感知路由 Beta | 跨 AZ 优化 |

---

> **参考文档**:  
> - [Service Traffic Policy](https://kubernetes.io/docs/concepts/services-networking/service/#traffic-policies)
> - [Topology Aware Routing](https://kubernetes.io/docs/concepts/services-networking/topology-aware-routing/)
> - [EndpointSlices](https://kubernetes.io/docs/concepts/services-networking/endpoint-slices/)

---

*Kusheet - Kubernetes 知识速查表项目*
