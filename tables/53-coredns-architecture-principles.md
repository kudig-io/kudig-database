# 53 - CoreDNS 架构与核心原理 (Architecture & Principles)

> **适用版本**: CoreDNS 1.8.0+ / Kubernetes v1.25-v1.32 | **最后更新**: 2026-01

---

## 一、CoreDNS 架构概览 (Architecture Overview)

### 1.1 设计哲学

CoreDNS 采用**模块化插件架构**，每个功能以独立插件实现，通过链式调用完成DNS请求处理。

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          CoreDNS 整体架构                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────┐    ┌─────────────────────────────────────────────────┐   │
│   │  DNS Query  │───▶│              CoreDNS Server                      │   │
│   │  (UDP/TCP)  │    │  ┌─────────────────────────────────────────┐    │   │
│   └─────────────┘    │  │         Server Block Manager            │    │   │
│                      │  │  ┌───────────┬───────────┬───────────┐  │    │   │
│                      │  │  │ Zone 1    │ Zone 2    │ Zone N    │  │    │   │
│                      │  │  │ :53       │ :53       │ :5353     │  │    │   │
│                      │  │  └─────┬─────┴─────┬─────┴─────┬─────┘  │    │   │
│                      │  └────────┼───────────┼───────────┼────────┘    │   │
│                      │           │           │           │              │   │
│                      │           ▼           ▼           ▼              │   │
│                      │  ┌─────────────────────────────────────────┐    │   │
│                      │  │            Plugin Chain                 │    │   │
│                      │  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐       │    │   │
│                      │  │  │log  │→│cache│→│k8s  │→│fwd  │→...   │    │   │
│                      │  │  └─────┘ └─────┘ └─────┘ └─────┘       │    │   │
│                      │  └─────────────────────────────────────────┘    │   │
│                      └─────────────────────────────────────────────────┘   │
│                                        │                                    │
│                                        ▼                                    │
│   ┌─────────────┐              ┌─────────────┐                             │
│   │ DNS Response│◀─────────────│   Response  │                             │
│   └─────────────┘              │  Generator  │                             │
│                                └─────────────┘                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 核心组件详解

| 组件 | 英文 | 职责 | 关键特性 |
|:---|:---|:---|:---|
| **CoreDNS Server** | Server | 监听DNS端口,管理请求生命周期 | 支持UDP/TCP/DoT/DoH,并发处理 |
| **Server Block** | Server Block | 配置单元,绑定端口和区域 | 一个Corefile可有多个Server Block |
| **Zone** | Zone | DNS域名空间划分 | 支持正向/反向解析区域 |
| **Plugin Chain** | Plugin Chain | 有序插件序列 | 顺序执行,支持短路返回 |
| **Plugin** | Plugin | 功能模块 | 50+内置插件,支持自定义 |
| **Middleware** | Middleware | 请求/响应拦截器 | 日志、监控、重写等 |

### 1.3 请求处理流程详解

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        DNS 请求处理流程                                       │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────┐     ┌─────────────────────────────────────────────────────┐    │
│  │ Client  │────▶│ 1. 协议/端口匹配                                     │    │
│  │ Query   │     │    - UDP:53 / TCP:53 / TLS:853 / HTTPS:443          │    │
│  └─────────┘     └─────────────────────┬───────────────────────────────┘    │
│                                        │                                     │
│                                        ▼                                     │
│                  ┌─────────────────────────────────────────────────────┐    │
│                  │ 2. Zone 最长后缀匹配                                  │    │
│                  │    Query: api.prod.svc.cluster.local                │    │
│                  │    Match: cluster.local (优先于 . 根区域)            │    │
│                  └─────────────────────┬───────────────────────────────┘    │
│                                        │                                     │
│                                        ▼                                     │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │ 3. Plugin Chain 执行                                                   │  │
│  │                                                                        │  │
│  │   ┌──────┐   ┌──────┐   ┌──────┐   ┌──────┐   ┌──────┐   ┌──────┐   │  │
│  │   │ log  │──▶│errors│──▶│cache │──▶│  k8s │──▶│forward──▶│ ... │   │  │
│  │   └──────┘   └──────┘   └──┬───┘   └──┬───┘   └──┬───┘   └──────┘   │  │
│  │                            │          │          │                    │  │
│  │                     Cache  │   K8s    │  Forward │                    │  │
│  │                      Hit   │  Found   │   to     │                    │  │
│  │                       │    │    │     │ Upstream │                    │  │
│  │                       ▼    │    ▼     │    │     │                    │  │
│  │                  ┌────────┐│┌────────┐│    ▼     │                    │  │
│  │                  │Response│││Response││┌────────┐│                    │  │
│  │                  └────┬───┘│└────┬───┘││Upstream││                    │  │
│  │                       │    │     │    ││  DNS   ││                    │  │
│  │                       │    │     │    │└────┬───┘│                    │  │
│  │                       │    │     │    │     │    │                    │  │
│  └───────────────────────┼────┼─────┼────┼─────┼────┼────────────────────┘  │
│                          │    │     │    │     │    │                        │
│                          ▼    ▼     ▼    ▼     ▼    ▼                        │
│                  ┌─────────────────────────────────────┐                    │
│                  │          Response to Client         │                    │
│                  └─────────────────────────────────────┘                    │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 1.4 插件执行模式

| 模式 | 说明 | 示例插件 | 行为 |
|:---|:---|:---|:---|
| **Authoritative** | 权威响应 | kubernetes, file, hosts | 直接生成响应,终止链 |
| **Passthrough** | 传递模式 | log, prometheus, errors | 处理后传递给下一个插件 |
| **Fallthrough** | 回退模式 | kubernetes (配置fallthrough) | 无法处理时传递 |
| **Terminal** | 终止模式 | forward | 作为最后一环处理 |

---

## 二、Kubernetes 集成架构 (Kubernetes Integration)

### 2.1 部署架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     CoreDNS in Kubernetes 部署架构                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        kube-system Namespace                         │   │
│  │                                                                      │   │
│  │  ┌──────────────────┐    ┌──────────────────────────────────────┐   │   │
│  │  │   ConfigMap      │    │        Deployment: coredns           │   │   │
│  │  │   coredns        │    │  ┌──────────────────────────────┐    │   │   │
│  │  │  ┌────────────┐  │    │  │     ReplicaSet (replicas=2)  │    │   │   │
│  │  │  │  Corefile  │──┼────┼─▶│  ┌────────┐    ┌────────┐    │    │   │   │
│  │  │  └────────────┘  │    │  │  │ Pod 1  │    │ Pod 2  │    │    │   │   │
│  │  └──────────────────┘    │  │  │coredns │    │coredns │    │    │   │   │
│  │                          │  │  └────┬───┘    └────┬───┘    │    │   │   │
│  │  ┌──────────────────┐    │  └───────┼────────────┼─────────┘    │   │   │
│  │  │   Service        │    └──────────┼────────────┼──────────────┘   │   │
│  │  │   kube-dns       │               │            │                  │   │
│  │  │  ClusterIP:      │◀──────────────┴────────────┘                  │   │
│  │  │  10.96.0.10:53   │                                               │   │
│  │  └──────────────────┘                                               │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                           │                                                 │
│                           │ DNS Query                                       │
│                           ▼                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      Application Namespace                           │   │
│  │                                                                      │   │
│  │  ┌──────────────────────────────────────────────────────────────┐   │   │
│  │  │                        Pod                                    │   │   │
│  │  │  ┌────────────────────────────────────────────────────────┐  │   │   │
│  │  │  │  /etc/resolv.conf                                      │  │   │   │
│  │  │  │  nameserver 10.96.0.10                                 │  │   │   │
│  │  │  │  search default.svc.cluster.local svc.cluster.local    │  │   │   │
│  │  │  │         cluster.local                                  │  │   │   │
│  │  │  │  options ndots:5                                       │  │   │   │
│  │  │  └────────────────────────────────────────────────────────┘  │   │   │
│  │  └──────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Kubernetes 资源清单

| 资源类型 | 名称 | 命名空间 | 用途 |
|:---|:---|:---|:---|
| Deployment | coredns | kube-system | 管理CoreDNS Pod副本 |
| Service | kube-dns | kube-system | 提供ClusterIP供集群内DNS解析 |
| ConfigMap | coredns | kube-system | 存储Corefile配置 |
| ServiceAccount | coredns | kube-system | Pod身份认证 |
| ClusterRole | system:coredns | - | RBAC权限定义 |
| ClusterRoleBinding | system:coredns | - | 绑定SA和ClusterRole |

### 2.3 DNS 解析类型

| 解析类型 | 格式 | 示例 | 返回值 |
|:---|:---|:---|:---|
| **Service A记录** | `<svc>.<ns>.svc.<zone>` | nginx.default.svc.cluster.local | ClusterIP |
| **Service SRV记录** | `_<port>._<proto>.<svc>.<ns>.svc.<zone>` | _http._tcp.nginx.default.svc.cluster.local | 端口+主机名 |
| **Headless Service** | `<svc>.<ns>.svc.<zone>` | mysql-headless.db.svc.cluster.local | 所有Pod IP列表 |
| **StatefulSet Pod** | `<pod>.<svc>.<ns>.svc.<zone>` | mysql-0.mysql-headless.db.svc.cluster.local | 特定Pod IP |
| **Pod A记录** | `<pod-ip-dashed>.<ns>.pod.<zone>` | 10-244-1-5.default.pod.cluster.local | Pod IP |
| **ExternalName** | `<svc>.<ns>.svc.<zone>` | ext-db.default.svc.cluster.local | CNAME到外部域名 |

### 2.4 resolv.conf 配置详解

```
# Pod 内 /etc/resolv.conf 示例
nameserver 10.96.0.10
search default.svc.cluster.local svc.cluster.local cluster.local
options ndots:5
```

| 参数 | 值 | 说明 | 影响 |
|:---|:---|:---|:---|
| **nameserver** | 10.96.0.10 | kube-dns Service ClusterIP | 所有DNS查询发送到此地址 |
| **search** | default.svc... | 搜索域列表(最多6个) | 短名称自动补全后缀 |
| **ndots** | 5 | 点号阈值 | <5个点视为短名称,触发search |
| **timeout** | 5 (默认) | 查询超时秒数 | 单次查询等待时间 |
| **attempts** | 2 (默认) | 重试次数 | 超时后重试次数 |
| **rotate** | - | 轮询nameserver | 多nameserver时负载均衡 |
| **single-request** | - | 串行A/AAAA查询 | 解决某些NAT问题 |

### 2.5 ndots 与 DNS 查询次数

| 查询域名 | 点数 | ndots=5行为 | 实际查询序列 |
|:---|:---:|:---|:---|
| `nginx` | 0 | 短名称,应用search | nginx.default.svc.cluster.local → nginx.svc.cluster.local → nginx.cluster.local → nginx. |
| `nginx.default` | 1 | 短名称 | nginx.default.default.svc.cluster.local → ... |
| `www.google.com` | 2 | 短名称(!) | www.google.com.default.svc.cluster.local → ... → www.google.com. |
| `api.v1.prod.company.com` | 4 | 短名称 | 先尝试search后缀 |
| `api.v1.prod.company.io.` | 5 | FQDN(末尾有.) | 直接查询,不应用search |

**生产建议**: 外部域名务必使用FQDN格式(末尾加`.`)避免不必要的DNS查询。

---

## 三、插件链机制深度解析 (Plugin Chain In-Depth)

### 3.1 插件分类

| 类别 | 职责 | 插件示例 | 执行特点 |
|:---|:---|:---|:---|
| **数据源插件** | 提供DNS记录 | kubernetes, file, etcd, hosts | 权威响应 |
| **缓存插件** | 缓存响应 | cache | 命中则短路返回 |
| **转发插件** | 转发查询 | forward, proxy | 终止链,等待上游 |
| **修改插件** | 修改请求/响应 | rewrite, template | 传递给下一个 |
| **监控插件** | 记录/暴露指标 | log, prometheus, trace | 传递给下一个 |
| **安全插件** | 访问控制 | acl, dnssec | 可能拒绝请求 |
| **辅助插件** | 辅助功能 | errors, health, ready, reload | 传递给下一个 |

### 3.2 最佳插件顺序

```
.:53 {
    # 1. 错误处理 (必须最先)
    errors
    
    # 2. 日志记录
    log
    
    # 3. 健康检查
    health {
        lameduck 5s
    }
    ready
    
    # 4. 监控指标
    prometheus :9153
    
    # 5. 循环检测
    loop
    
    # 6. 名称重写 (在缓存之前)
    rewrite name suffix .old.local .new.local
    
    # 7. 缓存 (在权威解析之前!)
    cache 30
    
    # 8. 权威解析
    kubernetes cluster.local in-addr.arpa ip6.arpa {
        pods insecure
        fallthrough in-addr.arpa ip6.arpa
        ttl 30
    }
    
    # 9. 本地hosts
    hosts {
        10.0.0.1 custom.local
        fallthrough
    }
    
    # 10. 转发到上游 (最后)
    forward . /etc/resolv.conf {
        max_concurrent 1000
    }
    
    # 11. 配置热重载
    reload
    
    # 12. 负载均衡
    loadbalance
}
```

### 3.3 插件顺序错误示例

**错误1: cache在kubernetes之后**
```
# 错误配置
kubernetes cluster.local
cache 30  # 永远不会缓存kubernetes的响应!
```

**错误2: rewrite在cache之后**
```
# 错误配置  
cache 30
rewrite name ...  # 重写后的查询无法利用缓存
```

**错误3: forward在kubernetes之前**
```
# 错误配置
forward . 8.8.8.8  # 所有查询都转发了!
kubernetes cluster.local  # 永远不会执行
```

### 3.4 插件间数据传递

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Plugin Chain 数据流                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────┐                                                               │
│   │ Request │                                                               │
│   │ Context │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐               │
│   └────┬────┘                                              │               │
│        │                                                   │               │
│        ▼                                                   ▼               │
│   ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐               │
│   │  log    │───▶│  cache  │───▶│  k8s    │───▶│ forward │               │
│   └────┬────┘    └────┬────┘    └────┬────┘    └────┬────┘               │
│        │              │              │              │                      │
│        │ ctx.SetVar   │ Cache Hit?   │ K8s Found?   │ Forward             │
│        │ "client_ip"  │     │        │    │         │ to Upstream         │
│        │              │     ▼        │    ▼         │    │                 │
│        │              │  ┌─────┐     │ ┌─────┐      │    ▼                 │
│        │              │  │Return│    │ │Return│     │ ┌──────┐            │
│        │              │  └─────┘     │ └─────┘      │ │Upstream│           │
│        │              │     │        │    │         │ │Response│           │
│        │              │     │        │    │         │ └───┬───┘            │
│        │              │     │        │    │         │     │                 │
│   ┌────┴────┐    ┌────┴────┐    ┌────┴────┐    ┌────┴─────┐               │
│   │Response │◀───│Response │◀───│Response │◀───│Response  │               │
│   │ (logged)│    │ (cached)│    │ (k8s)   │    │(upstream)│               │
│   └─────────┘    └─────────┘    └─────────┘    └──────────┘               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 四、高可用架构设计 (High Availability)

### 4.1 多副本部署

| 配置项 | 推荐值 | 说明 |
|:---|:---|:---|
| replicas | 2-3 | 小型集群2个,大型集群3个 |
| podAntiAffinity | preferredDuringScheduling | 分散到不同节点 |
| PodDisruptionBudget | minAvailable: 1 | 保证至少1个可用 |
| resources.requests.cpu | 100m | CPU请求 |
| resources.requests.memory | 70Mi | 内存请求 |
| resources.limits.memory | 170Mi | 内存限制 |

### 4.2 生产级部署配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: coredns
  namespace: kube-system
  labels:
    k8s-app: kube-dns
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
      maxSurge: 1
  selector:
    matchLabels:
      k8s-app: kube-dns
  template:
    metadata:
      labels:
        k8s-app: kube-dns
    spec:
      priorityClassName: system-cluster-critical
      serviceAccountName: coredns
      tolerations:
        - key: "CriticalAddonsOnly"
          operator: "Exists"
        - key: "node-role.kubernetes.io/control-plane"
          effect: NoSchedule
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: k8s-app
                  operator: In
                  values:
                  - kube-dns
              topologyKey: kubernetes.io/hostname
      containers:
      - name: coredns
        image: coredns/coredns:1.11.1
        imagePullPolicy: IfNotPresent
        resources:
          limits:
            memory: 170Mi
          requests:
            cpu: 100m
            memory: 70Mi
        args: [ "-conf", "/etc/coredns/Corefile" ]
        volumeMounts:
        - name: config-volume
          mountPath: /etc/coredns
          readOnly: true
        ports:
        - containerPort: 53
          name: dns
          protocol: UDP
        - containerPort: 53
          name: dns-tcp
          protocol: TCP
        - containerPort: 9153
          name: metrics
          protocol: TCP
        securityContext:
          allowPrivilegeEscalation: false
          capabilities:
            add:
            - NET_BIND_SERVICE
            drop:
            - all
          readOnlyRootFilesystem: true
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
            scheme: HTTP
          initialDelaySeconds: 60
          timeoutSeconds: 5
          successThreshold: 1
          failureThreshold: 5
        readinessProbe:
          httpGet:
            path: /ready
            port: 8181
            scheme: HTTP
      dnsPolicy: Default
      volumes:
      - name: config-volume
        configMap:
          name: coredns
          items:
          - key: Corefile
            path: Corefile
---
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

### 4.3 AutoScaling 配置

```yaml
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
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Pods
        value: 1
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
      - type: Pods
        value: 2
        periodSeconds: 60
```

---

## 五、集群规模与容量规划 (Capacity Planning)

### 5.1 规模参考表

| 集群规模 | 节点数 | Pod数 | CoreDNS副本 | CPU Request | Memory Request | 缓存大小 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|
| **小型** | <50 | <1000 | 2 | 100m | 70Mi | 10000 |
| **中型** | 50-200 | 1000-5000 | 3 | 200m | 128Mi | 30000 |
| **大型** | 200-1000 | 5000-20000 | 5 | 500m | 256Mi | 50000 |
| **超大型** | >1000 | >20000 | 7+ | 1000m | 512Mi | 100000 |

### 5.2 QPS 估算

| 因素 | 计算方式 | 示例 |
|:---|:---|:---|
| 基础QPS | Pod数 × 每Pod平均QPS | 5000 × 2 = 10000 QPS |
| 峰值系数 | 基础QPS × 3-5倍 | 10000 × 4 = 40000 QPS |
| 缓存命中率 | 通常60-80% | 实际后端QPS = 40000 × 0.3 = 12000 |
| 单Pod处理能力 | ~30000 QPS (取决于配置) | 需要至少1个Pod |

### 5.3 资源计算公式

```
CPU (millicores) = (集群Pod数 / 1000) × 50m + 100m (基础)
Memory (Mi) = (缓存条目数 / 10000) × 20Mi + 70Mi (基础)
副本数 = max(2, ceil(预期QPS / 20000))
```

---

## 六、监控指标体系 (Monitoring Metrics)

### 6.1 关键Prometheus指标

| 指标名称 | 类型 | 说明 | 告警阈值建议 |
|:---|:---|:---|:---|
| `coredns_dns_requests_total` | Counter | 总请求数 | - |
| `coredns_dns_responses_total` | Counter | 总响应数(按rcode) | SERVFAIL > 1% |
| `coredns_dns_request_duration_seconds` | Histogram | 请求延迟 | P99 > 100ms |
| `coredns_dns_request_size_bytes` | Histogram | 请求大小 | - |
| `coredns_dns_response_size_bytes` | Histogram | 响应大小 | - |
| `coredns_cache_hits_total` | Counter | 缓存命中 | 命中率 < 50% |
| `coredns_cache_misses_total` | Counter | 缓存未命中 | - |
| `coredns_cache_size` | Gauge | 当前缓存条目数 | - |
| `coredns_forward_requests_total` | Counter | 转发请求数 | - |
| `coredns_forward_responses_total` | Counter | 转发响应数 | - |
| `coredns_forward_request_duration_seconds` | Histogram | 转发延迟 | P99 > 500ms |
| `coredns_kubernetes_dns_programming_duration_seconds` | Histogram | K8s同步延迟 | P99 > 5s |
| `coredns_panics_total` | Counter | panic次数 | > 0 |
| `coredns_plugin_enabled` | Gauge | 启用的插件 | - |
| `process_resident_memory_bytes` | Gauge | 内存使用 | > limits 80% |

### 6.2 告警规则示例

```yaml
groups:
- name: coredns
  rules:
  - alert: CoreDNSDown
    expr: absent(up{job="coredns"} == 1)
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "CoreDNS is down"
      
  - alert: CoreDNSHighErrorRate
    expr: |
      sum(rate(coredns_dns_responses_total{rcode="SERVFAIL"}[5m])) 
      / 
      sum(rate(coredns_dns_responses_total[5m])) > 0.01
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS error rate > 1%"
      
  - alert: CoreDNSHighLatency
    expr: |
      histogram_quantile(0.99, 
        sum(rate(coredns_dns_request_duration_seconds_bucket[5m])) by (le)
      ) > 0.1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS P99 latency > 100ms"
      
  - alert: CoreDNSLowCacheHitRate
    expr: |
      sum(rate(coredns_cache_hits_total[5m])) 
      / 
      (sum(rate(coredns_cache_hits_total[5m])) + sum(rate(coredns_cache_misses_total[5m]))) < 0.5
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS cache hit rate < 50%"
      
  - alert: CoreDNSForwardLatencyHigh
    expr: |
      histogram_quantile(0.99, 
        sum(rate(coredns_forward_request_duration_seconds_bucket[5m])) by (le)
      ) > 0.5
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS forward P99 latency > 500ms"
```

### 6.3 Grafana Dashboard 关键面板

| 面板名称 | 查询 | 用途 |
|:---|:---|:---|
| **QPS** | `sum(rate(coredns_dns_requests_total[1m]))` | 总请求速率 |
| **Error Rate** | `sum(rate(coredns_dns_responses_total{rcode=~"SERVFAIL\|REFUSED"}[1m]))/sum(rate(coredns_dns_responses_total[1m]))` | 错误率 |
| **Latency P50/P99** | `histogram_quantile(0.99, sum(rate(coredns_dns_request_duration_seconds_bucket[1m])) by (le))` | 延迟分布 |
| **Cache Hit Rate** | `sum(rate(coredns_cache_hits_total[1m]))/(sum(rate(coredns_cache_hits_total[1m]))+sum(rate(coredns_cache_misses_total[1m])))` | 缓存效率 |
| **Forward Latency** | `histogram_quantile(0.99, sum(rate(coredns_forward_request_duration_seconds_bucket[1m])) by (le))` | 上游延迟 |
| **Memory Usage** | `process_resident_memory_bytes{job="coredns"}` | 内存使用 |

---

## 七、与kube-dns对比 (Comparison with kube-dns)

| 特性 | CoreDNS | kube-dns | 说明 |
|:---|:---|:---|:---|
| **架构** | 单进程,插件化 | 多容器(kubedns+dnsmasq+sidecar) | CoreDNS更简洁 |
| **配置** | Corefile (声明式) | ConfigMap + 命令行参数 | CoreDNS更灵活 |
| **扩展性** | 插件系统,易扩展 | 需修改源码 | CoreDNS优势明显 |
| **缓存** | 内置cache插件 | dnsmasq提供 | CoreDNS原生支持 |
| **性能** | 高性能,Go原生 | 依赖dnsmasq | CoreDNS略优 |
| **资源占用** | 单容器,资源少 | 3容器,资源多 | CoreDNS节省约50% |
| **监控** | 原生Prometheus | 需额外配置 | CoreDNS更易集成 |
| **负载均衡** | 内置loadbalance | 依赖dnsmasq | CoreDNS原生支持 |
| **热重载** | reload插件支持 | 需重启Pod | CoreDNS支持无缝更新 |
| **Kubernetes支持** | v1.13+ 默认 | v1.12及之前默认 | CoreDNS是现代标准 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
