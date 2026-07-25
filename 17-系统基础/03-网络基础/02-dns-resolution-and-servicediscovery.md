---
title: DNS 解析与服务发现
description: DNS 协议原理、解析流程、CoreDNS 架构、K8s 服务发现、DNS 调优、故障排查、NodeLocal DNSCache
summary: DNS 完整知识，覆盖协议原理、CoreDNS 配置、K8s DNS 策略、ndots 优化、NodeLocal DNSCache、故障排查
category: knowledge
tags:
- networking
- dns
- coredns
- service-discovery
- kubernetes
domain: 系统基础
difficulty: intermediate
audience:
- SRE
- 平台工程师
- 开发工程师
---

# DNS 解析与服务发现

> DNS 是 Kubernetes 服务发现的核心机制。理解 DNS 解析流程、CoreDNS 配置和常见故障模式，是排查服务通信问题的关键技能。

## DNS 协议基础

### DNS 查询类型

| 类型 | 用途 | K8s 场景 |
|------|------|----------|
| A | 域名 → IPv4 | Service ClusterIP |
| AAAA | 域名 → IPv6 | 双栈集群 |
| CNAME | 域名 → 域名 | ExternalName Service |
| SRV | 服务端口发现 | Headless Service |
| PTR | IP → 域名 | 反向解析 |
| TXT | 文本记录 | 验证/元数据 |
| MX | 邮件交换 | 非 K8s |

### DNS 解析完整流程

```
应用调用 getaddrinfo("nginx.default.svc.cluster.local")
    │
    ▼
检查 /etc/hosts（Pod 内由 kubelet 管理）
    │ 未命中
    ▼
读取 /etc/resolv.conf
    nameserver 10.96.0.10
    search default.svc.cluster.local svc.cluster.local cluster.local
    options ndots:5
    │
    ▼
构造查询：nginx.default.svc.cluster.local
    │ (ndots=5: 域名中点号 < 5，先追加 search 域)
    ▼
发送 UDP 查询到 10.96.0.10:53 (CoreDNS)
    │
    ▼
CoreDNS 处理：
    ├── kubernetes 插件匹配 *.cluster.local → 直接解析
    ├── 缓存命中 → 返回缓存
    └── 外部域名 → forward 到上游 DNS
    │
    ▼
返回 A 记录 (如 10.96.1.100)
    │
    ▼
应用获得 IP，发起 TCP 连接
```

### DNS 报文结构

```
┌─────────────────────────────────────┐
│           Header (12 bytes)          │
│  ID | Flags | QDCOUNT | ANCOUNT     │
│  NSCOUNT | ARCOUNT                   │
├─────────────────────────────────────┤
│           Question                   │
│  QNAME | QTYPE | QCLASS             │
├─────────────────────────────────────┤
│           Answer (RR)                │
│  NAME | TYPE | CLASS | TTL | RDLENGTH | RDATA │
├─────────────────────────────────────┤
│           Authority (RR)             │
├─────────────────────────────────────┤
│           Additional (RR)            │
└─────────────────────────────────────┘
```

### DNS over UDP vs TCP

| 场景 | 协议 | 说明 |
|------|------|------|
| 普通查询 | UDP:53 | 响应 < 512B |
| 大响应 | TCP:53 | EDNS0 或截断后重试 |
| Zone Transfer | TCP:53 | AXFR/IXFR |
| DNS over TLS | TCP:853 | 加密 |
| DNS over HTTPS | TCP:443 | 加密+伪装 |

## Kubernetes DNS 架构

### K8s DNS 规范

每个 Service 自动获得 DNS 记录：
```
<service-name>.<namespace>.svc.<cluster-domain>
```

默认 cluster-domain = `cluster.local`

### DNS 记录类型详解

| 资源 | DNS 记录 | 示例 |
|------|----------|------|
| ClusterIP Service | A → ClusterIP | `nginx.default.svc.cluster.local → 10.96.1.1` |
| Headless Service | A → 所有 Pod IP | 多条 A 记录 |
| StatefulSet Pod | A → Pod IP | `web-0.nginx-headless.default.svc.cluster.local` |
| ExternalName | CNAME → 外部域名 | `db.default.svc.cluster.local → rds.aws.com` |
| NodePort/LB | A → ClusterIP | 同 ClusterIP |
| Pod (默认) | A (dashed IP) | `10-244-1-5.default.pod.cluster.local` |

### Pod DNS 策略

```yaml
spec:
  dnsPolicy: ClusterFirst  # 默认
  # 可选值:
  # - ClusterFirst: 先查集群 DNS，再查上游
  # - ClusterFirstWithHostNet: hostNetwork Pod 也用集群 DNS
  # - Default: 使用节点 /etc/resolv.conf
  # - None: 完全由 dnsConfig 指定
  dnsConfig:
    nameservers:
    - 169.254.20.10  # NodeLocal DNSCache
    searches:
    - default.svc.cluster.local
    - svc.cluster.local
    - cluster.local
    options:
    - name: ndots
      value: "5"
    - name: single-request-reopen
    - name: timeout
      value: "1"
    - name: attempts
      value: "2"
```

### ndots 参数详解

```
ndots:5 含义：域名中 "." 的数量 < 5 时，先尝试追加 search 域

查询 "nginx.default.svc.cluster.local" (4个点 < 5):
  1. nginx.default.svc.cluster.local.default.svc.cluster.local  ← 无效
  2. nginx.default.svc.cluster.local.svc.cluster.local          ← 无效
  3. nginx.default.svc.cluster.local.cluster.local              ← 无效
  4. nginx.default.svc.cluster.local.                           ← 成功！

查询 "www.google.com" (2个点 < 5):
  1. www.google.com.default.svc.cluster.local  ← 无效
  2. www.google.com.svc.cluster.local          ← 无效
  3. www.google.com.cluster.local              ← 无效
  4. www.google.com.                           ← 成功！
```

**性能影响：** 每个外部域名查询产生 3 次无效 DNS 请求！

**优化方案：**
```yaml
# 方案1: 域名末尾加点（跳过 search）
url: "http://www.google.com."

# 方案2: 降低 ndots
dnsConfig:
  options:
  - name: ndots
    value: "2"

# 方案3: 使用 FQDN 格式（点数 >= ndots）
# nginx.default.svc.cluster.local 有4个点，ndots=5 仍会追加
# 但如果 ndots=2，则直接查询不追加
```

## CoreDNS 深度解析

### CoreDNS 架构

```
┌─────────────────────────────────────────────┐
│                CoreDNS Pod                    │
│                                              │
│  ┌─────────────────────────────────────┐    │
│  │          Plugin Chain                │    │
│  │                                      │    │
│  │  errors → health → ready             │    │
│  │  → kubernetes (集群内解析)           │    │
│  │  → prometheus (指标)                 │    │
│  │  → forward (上游转发)                │    │
│  │  → cache (缓存)                      │    │
│  │  → loop (环路检测)                   │    │
│  │  → reload (热加载)                   │    │
│  │  → loadbalance (负载均衡)            │    │
│  └─────────────────────────────────────┘    │
│                                              │
│  监听: UDP:53, TCP:53                        │
│  指标: :9153/metrics                         │
└─────────────────────────────────────────────┘
```

### CoreDNS 插件详解

| 插件 | 功能 | 配置要点 |
|------|------|----------|
| kubernetes | 集群内 Service/Pod 解析 | `pods insecure`, `ttl 30` |
| forward | 转发到上游 DNS | `max_concurrent 1000` |
| cache | 缓存 DNS 响应 | `success 9984 30`, `denial 9984 5` |
| loop | 检测转发环路 | 启动时检测 |
| reload | 热加载 Corefile | 30s 检查一次 |
| loadbalance | 轮询 A/AAAA 记录 | 默认启用 |
| autopath | 优化 search 路径 | 减少无效查询 |
| template | 自定义响应 | 拦截特定域名 |

### CoreDNS 生产配置

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
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        prometheus :9153
        forward . /etc/resolv.conf {
            max_concurrent 1000
            policy sequential
        }
        cache 30 {
            success 9984 30
            denial 9984 5
            prefetch 10 60m 10%
        }
        loop
        reload
        loadbalance
    }
```

### CoreDNS 自动扩缩

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
  - type: Pods
    pods:
      metric:
        name: coredns_dns_request_duration_seconds
      target:
        type: AverageValue
        averageValue: "100m"
```

## NodeLocal DNSCache

### 架构

```
Pod → 169.254.20.10:53 (NodeLocal DNSCache, 本节点 DaemonSet)
    │
    ├── 缓存命中 → 直接返回
    │
    ├── 集群内域名 → 转发到 CoreDNS (10.96.0.10)
    │
    └── 外部域名 → 转发到上游 DNS
```

### 部署配置

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-local-dns
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: node-cache
        image: registry.k8s.io/dns/k8s-dns-node-cache:1.22.28
        args:
        - "-localip"
        - "169.254.20.10"
        - "-conf"
        - "/etc/Corefile"
        - "-upstreamsvc"
        - "kube-dns"
        securityContext:
          privileged: true
        ports:
        - containerPort: 53
          name: dns
          protocol: UDP
        - containerPort: 53
          name: dns-tcp
          protocol: TCP
```

### 优势

| 指标 | 无 NodeLocal | 有 NodeLocal |
|------|-------------|-------------|
| DNS 延迟 | 1-5ms (跨节点) | < 0.1ms (本地) |
| 丢包风险 | 有 (conntrack) | 极低 |
| CoreDNS 负载 | 高 | 大幅降低 |
| 故障域 | 集群级 | 节点级 |

## DNS 故障排查

### 排查流程

```
1. 确认症状
   ├── NXDOMAIN: 域名不存在
   ├── SERVFAIL: 服务器错误
   ├── Timeout: 无响应
   └── 错误 IP: 解析到错误地址
       │
2. 检查 Pod DNS 配置
   kubectl exec <pod> -- cat /etc/resolv.conf
       │
3. 测试解析
   kubectl exec <pod> -- nslookup <domain>
   kubectl exec <pod> -- dig <domain> +short
       │
4. 检查 CoreDNS
   kubectl get pods -n kube-system -l k8s-app=kube-dns
   kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100
       │
5. 检查 Service/Endpoints
   kubectl get svc -n kube-system kube-dns
   kubectl get endpoints -n kube-system kube-dns
       │
6. 检查网络连通性
   kubectl exec <pod> -- nc -zv 10.96.0.10 53
```

### 常见 DNS 问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 5s 超时 | ndots + search 导致多次查询 | 降低 ndots 或域名加点 |
| NXDOMAIN | Service 不存在/命名错误 | 检查 Service 名称和命名空间 |
| 间歇性失败 | conntrack UDP 竞态 | 部署 NodeLocal DNSCache |
| CoreDNS CrashLoop | 配置错误/上游不可达 | 检查 Corefile 和上游 DNS |
| 解析慢 | CoreDNS 过载 | 扩容 CoreDNS/启用缓存 |
| Pod 重启后 DNS 失败 | resolv.conf 未更新 | 检查 dnsPolicy |

### DNS 诊断命令

```bash
# 🟢 Pod 内 DNS 测试
kubectl exec -it <pod> -- nslookup kubernetes.default.svc.cluster.local
kubectl exec -it <pod> -- dig nginx.default.svc.cluster.local +short
kubectl exec -it <pod> -- dig @10.96.0.10 <domain> A

# 🟢 检查 CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
kubectl top pods -n kube-system -l k8s-app=kube-dns

# 🟢 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50 -f

# 🟢 CoreDNS 指标
kubectl exec -n kube-system <coredns-pod> -- wget -qO- http://localhost:9153/metrics | grep coredns_dns

# 🟢 检查 DNS Service
kubectl get svc -n kube-system kube-dns -o yaml
kubectl get endpoints -n kube-system kube-dns

# 🟡 重启 CoreDNS
kubectl rollout restart deployment coredns -n kube-system

# 🟢 从节点测试 DNS
dig @10.96.0.10 kubernetes.default.svc.cluster.local +short
```

### DNS 监控告警

```yaml
groups:
- name: dns-alerts
  rules:
  - alert: CoreDNSHighLatency
    expr: histogram_quantile(0.99, rate(coredns_dns_request_duration_seconds_bucket[5m])) > 0.1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS P99 延迟超过 100ms"

  - alert: CoreDNSErrors
    expr: rate(coredns_dns_responses_total{rcode="SERVFAIL"}[5m]) > 10
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "CoreDNS SERVFAIL 响应率异常"

  - alert: CoreDNSPodDown
    expr: kube_deployment_status_replicas_available{deployment="coredns", namespace="kube-system"} < 2
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "CoreDNS 可用副本数不足"
```

## 生产案例

### 案例1：ndots 导致外部域名解析慢

**症状：** 应用访问外部 API 延迟 2-3s

**根因：** ndots=5，外部域名 `api.example.com`（2个点）先追加 3 个 search 域查询，每次超时 1s

**解决：**
```yaml
dnsConfig:
  options:
  - name: ndots
    value: "2"
```

### 案例2：UDP conntrack 竞态导致 DNS 间歇失败

**症状：** Pod 启动时 DNS 解析偶发超时

**根因：** 多个 DNS 查询同时发出，conntrack 插入竞态导致丢包

**解决：** 部署 NodeLocal DNSCache，或添加 `single-request-reopen` 选项

### 案例3：CoreDNS OOMKilled

**症状：** 集群 DNS 全部中断

**根因：** 大量 DNS 查询导致缓存膨胀，内存超限

**解决：**
```yaml
resources:
  requests:
    memory: 128Mi
  limits:
    memory: 512Mi
# 调整缓存大小
cache 30 {
    success 4096 30
    denial 2048 5
}
```

## 版本兼容矩阵

| 组件 | 版本 | DNS 相关变化 |
|------|------|-------------|
| Kubernetes | 1.11+ | CoreDNS 替代 kube-dns |
| Kubernetes | 1.14+ | NodeLocal DNSCache GA |
| Kubernetes | 1.22+ | dnsConfig 支持自定义 |
| CoreDNS | 1.9+ | 支持 DNS-over-TLS |
| CoreDNS | 1.11+ | 性能优化 |

## 检查清单

- [ ] 理解 DNS 解析完整流程
- [ ] 掌握 K8s DNS 记录格式
- [ ] 理解 ndots 参数及其性能影响
- [ ] 能配置 CoreDNS Corefile
- [ ] 掌握 DNS 故障排查流程
- [ ] 了解 NodeLocal DNSCache 部署
- [ ] 能配置 DNS 监控告警
- [ ] 理解 DNS 缓存策略

## 参考链接

- [[17-系统基础/03-网络基础/index.md|网络基础总索引]]
- [[17-系统基础/03-网络基础/01-tcp-ip-protocol-stack.md|TCP/IP 协议栈]]
- [[17-系统基础/04-K8s事件/10-service-networking-events.md|Service 网络事件]]
- [[17-系统基础/05-速查卡/networking.md|网络速查卡]]
