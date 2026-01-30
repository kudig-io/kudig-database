# 33 - 服务发现与 DNS 配置 (Service Discovery & DNS)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级

## Kubernetes DNS 架构概览

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        Kubernetes DNS 服务发现架构                                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          应用 Pod (DNS 客户端)                                │   │
│  │                                                                               │   │
│  │  /etc/resolv.conf:                                                           │   │
│  │  nameserver 10.96.0.10                                                       │   │
│  │  search default.svc.cluster.local svc.cluster.local cluster.local            │   │
│  │  options ndots:5                                                              │   │
│  │                                                                               │   │
│  │  DNS 查询: nginx → nginx.default.svc.cluster.local                           │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                      NodeLocal DNSCache (可选)                                │   │
│  │                                                                               │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │  DaemonSet: node-local-dns                                              │ │   │
│  │  │  监听: 169.254.20.10:53                                                 │ │   │
│  │  │  功能: 本地缓存、减少跨节点流量、降低 CoreDNS 负载                       │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          CoreDNS (kube-dns)                                   │   │
│  │                                                                               │   │
│  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐ │   │
│  │  │  kubernetes   │  │    cache      │  │   forward     │  │   prometheus  │ │   │
│  │  │   插件        │  │    插件       │  │    插件       │  │     插件      │ │   │
│  │  │               │  │               │  │               │  │               │ │   │
│  │  │ 解析 Service  │  │ DNS 缓存      │  │ 上游 DNS      │  │ 指标暴露      │ │   │
│  │  │ 和 Pod 记录   │  │ 30s TTL      │  │ 转发          │  │ :9153        │ │   │
│  │  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘ │   │
│  │                                                                               │   │
│  │  Service: kube-dns (10.96.0.10:53)                                           │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│              ┌───────────────────────┼───────────────────────┐                     │
│              │                       │                       │                     │
│              ▼                       ▼                       ▼                     │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────────────┐    │
│  │  Kubernetes     │    │   上游 DNS      │    │     外部 DNS                │    │
│  │  API Server     │    │  (/etc/resolv)  │    │    (云服务商)               │    │
│  │                 │    │                 │    │                             │    │
│  │  • Services     │    │  • 公网域名     │    │  • PrivateZone (阿里云)     │    │
│  │  • Endpoints    │    │  • 内网域名     │    │  • Route53 (AWS)            │    │
│  │  • Pods         │    │                 │    │  • Cloud DNS (GCP)          │    │
│  └─────────────────┘    └─────────────────┘    └─────────────────────────────┘    │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## CoreDNS 配置详解

### 核心配置项

| 配置项 | 默认值 | 说明 | 调优建议 |
|-------|-------|------|---------|
| Corefile 位置 | ConfigMap coredns | CoreDNS 配置 | kubectl edit cm coredns -n kube-system |
| 集群域名 | cluster.local | 集群 DNS 后缀 | 一般不修改 |
| 上游 DNS | /etc/resolv.conf | 外部 DNS 服务器 | 可指定特定 DNS |
| 缓存时间 | 30s | DNS 缓存 TTL | 根据需求调整 |
| 监听端口 | 53 | DNS 服务端口 | 不建议修改 |

### 完整 Corefile 配置示例

```
# CoreDNS Corefile 完整配置
.:53 {
    # 错误日志
    errors
    
    # 健康检查端点 :8080/health
    health {
       lameduck 5s
    }
    
    # 就绪检查端点 :8181/ready
    ready
    
    # Kubernetes 服务发现
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       # Pod 记录模式: disabled | insecure | verified
       pods insecure
       
       # 当查询不匹配时继续到下一个插件
       fallthrough in-addr.arpa ip6.arpa
       
       # DNS 记录 TTL
       ttl 30
       
       # 指定 kubeconfig (集群外运行时)
       # kubeconfig /path/to/kubeconfig
    }
    
    # Prometheus 指标端点 :9153/metrics
    prometheus :9153
    
    # DNS 转发到上游 DNS
    forward . /etc/resolv.conf {
       # 最大并发连接数
       max_concurrent 1000
       
       # 健康检查间隔
       health_check 5s
       
       # 优先使用 TCP (某些云环境需要)
       # prefer_udp
       
       # 指定策略: random | round_robin | sequential
       policy random
    }
    
    # DNS 缓存
    cache 30 {
       # 成功响应缓存
       success 9984 30
       
       # 否定响应缓存
       denial 9984 5
       
       # 预取 (提前刷新即将过期的记录)
       prefetch 10 1m 10%
    }
    
    # 循环检测 (防止转发循环)
    loop
    
    # 配置热加载
    reload
    
    # 负载均衡 (轮询 A/AAAA 记录)
    loadbalance
}

# 特定域名转发到内部 DNS
example.com:53 {
    errors
    cache 30
    forward . 10.0.0.53 10.0.0.54
}

# 阿里云 PrivateZone 集成
internal.mycompany.com:53 {
    errors
    cache 60
    forward . 100.100.2.136 100.100.2.138
}
```

### CoreDNS 插件详解

| 插件 | 功能 | 配置示例 | 说明 |
|-----|------|---------|------|
| **kubernetes** | K8s 服务发现 | `kubernetes cluster.local` | 核心插件 |
| **forward** | DNS 转发 | `forward . 8.8.8.8 8.8.4.4` | 上游 DNS |
| **cache** | DNS 缓存 | `cache 60` | 提高性能 |
| **loop** | 循环检测 | `loop` | 防止死循环 |
| **reload** | 配置热加载 | `reload` | 无需重启 |
| **health** | 健康检查 | `health :8080` | 存活探针 |
| **ready** | 就绪检查 | `ready :8181` | 就绪探针 |
| **prometheus** | 监控指标 | `prometheus :9153` | 指标暴露 |
| **errors** | 错误日志 | `errors` | 错误记录 |
| **log** | 查询日志 | `log` | 调试用 |
| **hosts** | 本地 hosts | `hosts /etc/hosts` | 自定义记录 |
| **rewrite** | 重写规则 | `rewrite name...` | DNS 重写 |
| **template** | 模板响应 | `template...` | 动态响应 |
| **loadbalance** | 负载均衡 | `loadbalance` | 轮询记录 |
| **autopath** | 自动路径 | `autopath @kubernetes` | 减少查询 |

### CoreDNS 高级配置

```
# 带 autopath 的优化配置 (减少 DNS 查询次数)
.:53 {
    errors
    health {
       lameduck 5s
    }
    ready
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
       ttl 30
    }
    
    # autopath 自动添加搜索域，减少客户端查询
    autopath @kubernetes
    
    prometheus :9153
    forward . /etc/resolv.conf {
       max_concurrent 1000
    }
    cache 30
    loop
    reload
    loadbalance
}

# 自定义 hosts 记录
.:53 {
    hosts {
       10.0.0.100 myservice.internal
       10.0.0.101 database.internal
       fallthrough
    }
    
    kubernetes cluster.local
    forward . /etc/resolv.conf
    cache 30
}

# DNS 重写规则
.:53 {
    rewrite name substring old-domain.com new-domain.com
    rewrite name regex (.*)\.old\.example\.com {1}.new.example.com
    
    kubernetes cluster.local
    forward . /etc/resolv.conf
    cache 30
}
```

## DNS 策略详解

| 策略 | 说明 | resolv.conf 来源 | 适用场景 |
|-----|------|-----------------|---------|
| **ClusterFirst** | 优先集群 DNS | 集群 DNS (kube-dns) | 默认策略，推荐 |
| **ClusterFirstWithHostNet** | hostNetwork 优先集群 | 集群 DNS | hostNetwork Pod |
| **Default** | 继承节点 DNS | 节点 /etc/resolv.conf | 需要节点 DNS |
| **None** | 自定义 DNS | Pod dnsConfig 指定 | 完全自定义 |

### Pod DNS 配置示例

```yaml
# 完整的 Pod DNS 配置
apiVersion: v1
kind: Pod
metadata:
  name: dns-example
  namespace: default
spec:
  # DNS 策略
  dnsPolicy: None
  
  # 自定义 DNS 配置
  dnsConfig:
    # DNS 服务器列表
    nameservers:
      - 10.96.0.10      # CoreDNS ClusterIP
      - 8.8.8.8         # 外部 DNS (备用)
    
    # 搜索域
    searches:
      - default.svc.cluster.local
      - svc.cluster.local
      - cluster.local
      - mycompany.com   # 自定义搜索域
    
    # DNS 选项
    options:
      # 点数阈值 (域名中少于 ndots 个点时，先尝试搜索域)
      - name: ndots
        value: "5"
      
      # 查询超时 (秒)
      - name: timeout
        value: "2"
      
      # 重试次数
      - name: attempts
        value: "3"
      
      # 单请求重开 (解决某些 conntrack 问题)
      - name: single-request-reopen
      
      # 使用 TCP (某些环境 UDP 不稳定时)
      # - name: use-vc
  
  containers:
    - name: app
      image: nginx
---
# ClusterFirst 策略 (默认，推荐)
apiVersion: v1
kind: Pod
metadata:
  name: clusterfirst-example
spec:
  dnsPolicy: ClusterFirst
  containers:
    - name: app
      image: nginx
---
# hostNetwork Pod 使用 ClusterFirstWithHostNet
apiVersion: v1
kind: Pod
metadata:
  name: hostnetwork-example
spec:
  hostNetwork: true
  dnsPolicy: ClusterFirstWithHostNet
  containers:
    - name: app
      image: nginx
```

## 服务 DNS 记录格式

### DNS 记录类型矩阵

| 服务类型 | 记录格式 | 示例 | 说明 |
|---------|---------|------|------|
| **ClusterIP** | `<svc>.<ns>.svc.<domain>` | `nginx.default.svc.cluster.local` | 返回 ClusterIP |
| **Headless** | `<svc>.<ns>.svc.<domain>` | `nginx-headless.default.svc.cluster.local` | 返回所有 Pod IP |
| **Headless Pod** | `<pod>.<svc>.<ns>.svc.<domain>` | `nginx-0.nginx-headless.default.svc.cluster.local` | 返回特定 Pod IP |
| **ExternalName** | `<svc>.<ns>.svc.<domain>` | `external-db.default.svc.cluster.local` | 返回 CNAME |
| **SRV** | `_<port>._<proto>.<svc>.<ns>.svc.<domain>` | `_http._tcp.nginx.default.svc.cluster.local` | 返回端口和主机 |

### DNS 记录示例

```bash
# ==================== ClusterIP Service ====================

# Service 定义
kubectl create service clusterip nginx --tcp=80:80

# DNS 查询
nslookup nginx.default.svc.cluster.local

# 结果: 返回 ClusterIP
# Server:    10.96.0.10
# Address:   10.96.0.10#53
# Name:      nginx.default.svc.cluster.local
# Address:   10.96.123.45

# ==================== Headless Service ====================

# Service 定义 (clusterIP: None)
kubectl apply -f - <<EOF
apiVersion: v1
kind: Service
metadata:
  name: nginx-headless
spec:
  clusterIP: None
  selector:
    app: nginx
  ports:
    - port: 80
EOF

# DNS 查询
nslookup nginx-headless.default.svc.cluster.local

# 结果: 返回所有 Pod IP
# Name:      nginx-headless.default.svc.cluster.local
# Address:   10.244.1.10
# Address:   10.244.2.11
# Address:   10.244.3.12

# ==================== StatefulSet Pod DNS ====================

# StatefulSet 会创建稳定的 Pod DNS 记录
# Pod 格式: <statefulset-name>-<ordinal>

nslookup nginx-0.nginx-headless.default.svc.cluster.local
# 结果: 10.244.1.10

nslookup nginx-1.nginx-headless.default.svc.cluster.local
# 结果: 10.244.2.11

# ==================== ExternalName Service ====================

kubectl apply -f - <<EOF
apiVersion: v1
kind: Service
metadata:
  name: external-db
spec:
  type: ExternalName
  externalName: db.external.mycompany.com
EOF

nslookup external-db.default.svc.cluster.local
# 结果: CNAME db.external.mycompany.com

# ==================== SRV 记录 ====================

# 查询带命名端口的 Service 的 SRV 记录
dig SRV _http._tcp.nginx.default.svc.cluster.local

# 结果:
# _http._tcp.nginx.default.svc.cluster.local. 30 IN SRV 0 100 80 nginx.default.svc.cluster.local.
```

## NodeLocal DNSCache

### 架构说明

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         NodeLocal DNSCache 架构                                      │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  Node                                                                               │
│  ┌────────────────────────────────────────────────────────────────────────────┐    │
│  │                                                                             │    │
│  │  ┌─────────────┐      ┌─────────────────────┐      ┌─────────────────┐    │    │
│  │  │   Pod A     │      │  NodeLocal DNS      │      │    CoreDNS     │    │    │
│  │  │             │      │    Cache            │      │   (ClusterIP)  │    │    │
│  │  │ resolv.conf │      │                     │      │                │    │    │
│  │  │ nameserver  │ ──▶  │  169.254.20.10:53   │ ──▶  │  10.96.0.10   │    │    │
│  │  │ 169.254.20.10│      │                     │      │                │    │    │
│  │  └─────────────┘      │  ┌───────────────┐  │      └────────┬───────┘    │    │
│  │                       │  │ Local Cache   │  │               │            │    │
│  │  ┌─────────────┐      │  │ (内存缓存)    │  │               │            │    │
│  │  │   Pod B     │      │  └───────────────┘  │               │            │    │
│  │  │             │      │                     │               │            │    │
│  │  │ nameserver  │ ──▶  │  功能:             │               │            │    │
│  │  │ 169.254.20.10│      │  • DNS 缓存        │               │            │    │
│  │  └─────────────┘      │  • 减少网络跳转     │               │            │    │
│  │                       │  • 降低 CoreDNS 负载│               │            │    │
│  │                       │  • 避免 conntrack  │               │            │    │
│  │                       │    问题            │               │            │    │
│  │                       └─────────────────────┘               │            │    │
│  │                                                             │            │    │
│  │                                                             ▼            │    │
│  │                                                    ┌─────────────────┐   │    │
│  │                                                    │   上游 DNS      │   │    │
│  │                                                    │  (外部域名)     │   │    │
│  │                                                    └─────────────────┘   │    │
│  └────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘

优势:
1. DNS 请求不离开节点，延迟更低
2. 本地缓存减少 CoreDNS 负载
3. 避免 SNAT conntrack 竞争问题
4. 提高 DNS 查询可靠性
```

### NodeLocal DNSCache 部署

```yaml
# node-local-dns-configmap.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: node-local-dns
  namespace: kube-system
data:
  Corefile: |
    cluster.local:53 {
        errors
        cache {
            success 9984 30
            denial 9984 5
        }
        reload
        loop
        bind 169.254.20.10
        forward . __PILLAR__CLUSTER__DNS__ {
            force_tcp
        }
        prometheus :9253
        health 169.254.20.10:8080
    }
    in-addr.arpa:53 {
        errors
        cache 30
        reload
        loop
        bind 169.254.20.10
        forward . __PILLAR__CLUSTER__DNS__ {
            force_tcp
        }
        prometheus :9253
    }
    ip6.arpa:53 {
        errors
        cache 30
        reload
        loop
        bind 169.254.20.10
        forward . __PILLAR__CLUSTER__DNS__ {
            force_tcp
        }
        prometheus :9253
    }
    .:53 {
        errors
        cache 30
        reload
        loop
        bind 169.254.20.10
        forward . __PILLAR__UPSTREAM__SERVERS__
        prometheus :9253
    }
---
# node-local-dns-daemonset.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-local-dns
  namespace: kube-system
  labels:
    k8s-app: node-local-dns
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
      tolerations:
        - key: CriticalAddonsOnly
          operator: Exists
        - effect: NoSchedule
          operator: Exists
        - effect: NoExecute
          operator: Exists
      containers:
        - name: node-cache
          image: registry.k8s.io/dns/k8s-dns-node-cache:1.22.28
          resources:
            requests:
              cpu: 25m
              memory: 5Mi
            limits:
              memory: 128Mi
          args:
            - "-localip"
            - "169.254.20.10"
            - "-conf"
            - "/etc/Corefile"
            - "-upstreamsvc"
            - "kube-dns"
          ports:
            - containerPort: 53
              name: dns
              protocol: UDP
            - containerPort: 53
              name: dns-tcp
              protocol: TCP
            - containerPort: 9253
              name: metrics
              protocol: TCP
          livenessProbe:
            httpGet:
              host: 169.254.20.10
              path: /health
              port: 8080
            initialDelaySeconds: 60
            timeoutSeconds: 5
          volumeMounts:
            - mountPath: /run/xtables.lock
              name: xtables-lock
              readOnly: false
            - name: config-volume
              mountPath: /etc/Corefile
              subPath: Corefile
          securityContext:
            capabilities:
              add:
                - NET_ADMIN
      volumes:
        - name: xtables-lock
          hostPath:
            path: /run/xtables.lock
            type: FileOrCreate
        - name: config-volume
          configMap:
            name: node-local-dns
            items:
              - key: Corefile
                path: Corefile
```

### kubelet 配置 (使用 NodeLocal DNS)

```yaml
# kubelet 配置 (通过 KubeletConfiguration)
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
clusterDNS:
  - 169.254.20.10  # NodeLocal DNS IP
clusterDomain: cluster.local
```

## DNS 性能优化

### 优化策略矩阵

| 优化项 | 配置方法 | 效果 | 适用场景 |
|-------|---------|------|---------|
| **启用缓存** | `cache 60` | 减少查询 | 所有场景 |
| **NodeLocal DNS** | 部署 DaemonSet | 降低延迟 | 大规模集群 |
| **减少 ndots** | `ndots: 2` | 减少搜索域尝试 | 频繁外部域名查询 |
| **使用 FQDN** | 完整域名查询 | 避免搜索 | 性能敏感应用 |
| **增加副本** | 多 CoreDNS Pod | 提高吞吐 | 高 QPS 场景 |
| **autopath** | `autopath @kubernetes` | 减少查询次数 | 大量内部服务调用 |
| **prefetch** | `prefetch 10 1m 10%` | 预取即将过期记录 | 热点域名 |

### ndots 优化详解

```
ndots 工作原理:

当查询域名时，如果域名中的点数 < ndots，则先尝试添加搜索域

示例 (ndots=5, 默认值):
  查询 "nginx" (0 个点):
    1. nginx.default.svc.cluster.local
    2. nginx.svc.cluster.local  
    3. nginx.cluster.local
    4. nginx (原始查询)

  查询 "api.example.com" (2 个点):
    1. api.example.com.default.svc.cluster.local
    2. api.example.com.svc.cluster.local
    3. api.example.com.cluster.local
    4. api.example.com (原始查询)

问题: 外部域名查询会产生大量无效查询

优化建议:
  - 内部服务为主: ndots=5 (默认)
  - 大量外部调用: ndots=2
  - 混合场景: ndots=3

配置方法:
spec:
  dnsConfig:
    options:
      - name: ndots
        value: "2"
```

### CoreDNS 扩容配置

```yaml
# coredns-hpa.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: coredns
  namespace: kube-system
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: coredns
  minReplicas: 2
  maxReplicas: 10
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
          averageUtilization: 70
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
        - type: Pods
          value: 2
          periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Pods
          value: 1
          periodSeconds: 120
---
# coredns-pdb.yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: coredns
  namespace: kube-system
spec:
  minAvailable: 1
  selector:
    matchLabels:
      k8s-app: kube-dns
```

## DNS 监控指标

### CoreDNS Prometheus 指标

| 指标 | 类型 | 说明 | 告警阈值 |
|-----|-----|------|---------|
| `coredns_dns_requests_total` | Counter | DNS 请求总数 | - |
| `coredns_dns_responses_total` | Counter | DNS 响应总数 | - |
| `coredns_dns_request_duration_seconds` | Histogram | 请求延迟 | P99 > 100ms |
| `coredns_cache_hits_total` | Counter | 缓存命中 | - |
| `coredns_cache_misses_total` | Counter | 缓存未命中 | - |
| `coredns_forward_requests_total` | Counter | 转发请求数 | - |
| `coredns_forward_responses_total` | Counter | 转发响应数 | - |
| `coredns_panic_count_total` | Counter | Panic 计数 | > 0 |
| `coredns_dns_response_rcode_total` | Counter | 响应码统计 | SERVFAIL > 1% |

### Prometheus 告警规则

```yaml
# dns-alerts.yaml
groups:
  - name: coredns
    interval: 30s
    rules:
      # DNS 请求延迟过高
      - alert: CoreDNSHighLatency
        expr: |
          histogram_quantile(0.99, 
            sum(rate(coredns_dns_request_duration_seconds_bucket[5m])) by (le, server)
          ) > 0.1
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "CoreDNS 请求延迟过高"
          description: "P99 延迟: {{ $value | humanizeDuration }}"

      # DNS 错误率过高
      - alert: CoreDNSErrorsHigh
        expr: |
          sum(rate(coredns_dns_responses_total{rcode="SERVFAIL"}[5m])) 
          / 
          sum(rate(coredns_dns_responses_total[5m])) > 0.01
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "CoreDNS 错误率过高"
          description: "SERVFAIL 比例: {{ $value | humanizePercentage }}"

      # DNS 缓存命中率过低
      - alert: CoreDNSCacheHitRateLow
        expr: |
          sum(rate(coredns_cache_hits_total[5m])) 
          / 
          (sum(rate(coredns_cache_hits_total[5m])) + sum(rate(coredns_cache_misses_total[5m]))) 
          < 0.5
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "CoreDNS 缓存命中率过低"
          description: "缓存命中率: {{ $value | humanizePercentage }}"

      # CoreDNS Pod 不健康
      - alert: CoreDNSDown
        expr: |
          up{job="coredns"} == 0
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "CoreDNS 实例不可用"
          description: "{{ $labels.instance }} 已下线"

      # DNS 转发失败
      - alert: CoreDNSForwardErrors
        expr: |
          sum(rate(coredns_forward_responses_total{rcode="SERVFAIL"}[5m])) > 10
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "CoreDNS 转发错误增加"
          description: "上游 DNS 可能存在问题"
```

## 故障排查指南

### 常见问题与解决方案

| 问题 | 症状 | 排查方法 | 解决方案 |
|-----|------|---------|---------|
| DNS 解析失败 | Pod 无法解析服务名 | 检查 CoreDNS Pod 状态 | 重启 CoreDNS |
| DNS 延迟高 | 应用响应慢 | 检查 CoreDNS 指标 | 扩容/启用缓存 |
| 外部域名解析失败 | 无法访问外部服务 | 检查 forward 配置 | 修复上游 DNS |
| 间歇性失败 | DNS 偶尔超时 | 检查 conntrack | 启用 NodeLocal DNS |
| 搜索域问题 | 域名解析错误 | 检查 resolv.conf | 调整 ndots |

### 故障排查命令

```bash
# ==================== 基础检查 ====================

# 检查 CoreDNS Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100

# 检查 CoreDNS Service
kubectl get svc -n kube-system kube-dns

# 查看 CoreDNS 配置
kubectl get cm coredns -n kube-system -o yaml

# ==================== DNS 解析测试 ====================

# 创建调试 Pod
kubectl run dnsutils --image=registry.k8s.io/e2e-test-images/jessie-dnsutils:1.3 \
  -it --rm --restart=Never -- /bin/bash

# 测试集群内 DNS
nslookup kubernetes.default.svc.cluster.local

# 测试外部 DNS
nslookup google.com

# 测试特定服务
nslookup nginx.default.svc.cluster.local

# 详细 DNS 查询
dig @10.96.0.10 nginx.default.svc.cluster.local +short
dig @10.96.0.10 nginx.default.svc.cluster.local ANY

# 测试 SRV 记录
dig @10.96.0.10 SRV _http._tcp.nginx.default.svc.cluster.local

# ==================== resolv.conf 检查 ====================

# 查看 Pod 的 DNS 配置
kubectl exec <pod-name> -- cat /etc/resolv.conf

# 查看 DNS 策略
kubectl get pod <pod-name> -o jsonpath='{.spec.dnsPolicy}'

# ==================== 网络连通性检查 ====================

# 测试到 CoreDNS 的连通性
kubectl exec <pod-name> -- nc -zv 10.96.0.10 53

# 测试 UDP DNS
kubectl exec <pod-name> -- dig @10.96.0.10 kubernetes.default.svc.cluster.local

# 测试 TCP DNS
kubectl exec <pod-name> -- dig @10.96.0.10 kubernetes.default.svc.cluster.local +tcp

# ==================== 性能测试 ====================

# DNS 查询延迟测试
kubectl exec <pod-name> -- sh -c 'for i in $(seq 1 100); do 
  start=$(date +%s.%N)
  nslookup nginx.default.svc.cluster.local > /dev/null 2>&1
  end=$(date +%s.%N)
  echo "$end - $start" | bc
done'
```

### DNS 诊断脚本

```bash
#!/bin/bash
# dns-diagnose.sh - Kubernetes DNS 诊断脚本

echo "=== CoreDNS 状态 ==="
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

echo ""
echo "=== CoreDNS Service ==="
kubectl get svc -n kube-system kube-dns

echo ""
echo "=== CoreDNS 配置 ==="
kubectl get cm coredns -n kube-system -o jsonpath='{.data.Corefile}'
echo ""

echo ""
echo "=== DNS 解析测试 ==="
kubectl run dns-test-$RANDOM --image=busybox:1.36 --rm -it --restart=Never -- sh -c '
echo "Testing kubernetes.default.svc.cluster.local..."
nslookup kubernetes.default.svc.cluster.local

echo ""
echo "Testing external domain (google.com)..."
nslookup google.com

echo ""
echo "/etc/resolv.conf:"
cat /etc/resolv.conf
'

echo ""
echo "=== CoreDNS 指标 ==="
kubectl exec -n kube-system $(kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{.items[0].metadata.name}') -- \
  wget -qO- http://localhost:9153/metrics 2>/dev/null | \
  grep -E '^coredns_(dns_requests_total|cache_hits_total|dns_request_duration_seconds)' | head -20

echo ""
echo "=== 最近 DNS 错误日志 ==="
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=20 | grep -i error
```

## ACK DNS 增强功能

| 功能 | 说明 | 配置方式 |
|-----|------|---------|
| **PrivateZone 集成** | 阿里云私有 DNS | CoreDNS forward 配置 |
| **DNS 自动扩缩** | 基于负载扩缩 | Cluster Autoscaler |
| **智能 DNS 缓存** | NodeLocal DNS | ACK 控制台一键部署 |
| **DNS 监控** | 云监控集成 | ARMS/Prometheus |

## DNS 最佳实践清单

- [ ] **启用 NodeLocal DNS**: 大规模集群必备
- [ ] **配置 DNS 缓存**: cache 30 以上
- [ ] **优化 ndots**: 根据应用类型调整
- [ ] **使用 FQDN**: 性能敏感场景
- [ ] **配置 HPA**: CoreDNS 自动扩缩
- [ ] **配置 PDB**: 保证 DNS 可用性
- [ ] **监控告警**: 配置延迟和错误率告警
- [ ] **定期检查**: 审查 DNS 配置和性能
- [ ] **文档记录**: 记录自定义 DNS 配置

## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.25 | CoreDNS 1.9.3+ |
| v1.27 | DNS 插件改进 |
| v1.28 | CoreDNS 1.10+ |
| v1.29 | DNS 查询优化 |
| v1.30 | CoreDNS 1.11+ |
| v1.31 | DNS 缓存改进 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
