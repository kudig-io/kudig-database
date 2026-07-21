---
title: Kubernetes Networking Deep Dive — From Pod to External Traffic Flow
description: K8s 网络深度解析 — Pod 网络模型、Service 实现、Ingress 架构、DNS 机制、网络策略、流量路径全链路
summary: 全面解析 Kubernetes 网络模型，从 Pod 通信到外部流量的完整路径与生产实践
category: reference
tags:
- networking
- service
- ingress
- dns
- network-policy
- traffic-flow
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: networking
---
# Kubernetes 网络深度解析

> 从 Pod 到外部流量的完整网络路径与生产实践。

## K8s 网络模型基本原则

```
┌─────────────────────────────────────────────────────────────┐
│  Kubernetes 网络三大规则                                     │
│                                                             │
│  1. 所有 Pod 可以直接通信（无需 NAT）                        │
│  2. 节点与 Pod 可以直接通信（无需 NAT）                      │
│  3. Pod 看到的自己的 IP 就是别人看到的 IP                    │
│                                                             │
│  实现: CNI 插件（Calico/Cilium/Flannel/Weave）             │
└─────────────────────────────────────────────────────────────┘
```

## Pod 网络（同节点）

```
Node
├── Pod A (10.244.1.2)
│   └── eth0 ← veth-a (宿主机端)
│           ↕
│       Linux Bridge (cbr0) 或 eBPF datapath
│           ↕
│   └── eth0 ← veth-b (宿主机端)
├── Pod B (10.244.1.3)
│
└── 通信路径: Pod A → veth-a → cbr0 → veth-b → Pod B
    （同节点直接通过 bridge/eBPF 转发，无封装）
```

## Pod 网络（跨节点）

```
Node 1                          Node 2
├── Pod A (10.244.1.2)         ├── Pod B (10.244.2.3)
│   └── veth → cbr0            │   └── cbr0 ← veth
│         │                    │         ↑
│    路由/封装                  │    路由/解封装
│         │                    │         ↑
│    eth0 (物理网卡)  ──────────────  eth0
│    (VXLAN/WireGuard/eBPF)    │
└──────────────────────────────└──────────────────────────────

CNI 实现对比:
┌──────────┬───────────┬──────────┬──────────┬──────────┐
│ CNI      │ 封装方式   │ 性能     │ 策略     │ 适用     │
├──────────┼───────────┼──────────┼──────────┼──────────┤
│ Flannel  │ VXLAN     │ 中       │ ❌       │ 简单场景 │
│ Calico   │ IPIP/无   │ 高       │ ✅       │ 企业     │
│ Cilium   │ eBPF/无   │ 最高     │ ✅       │ 高性能   │
│ Weave    │ VXLAN     │ 中       │ ✅       │ 加密需求 │
│ Antrea   │ Geneve    │ 高       │ ✅       │ VMware   │
└──────────┴───────────┴──────────┴──────────┴──────────┘
```

## Service 实现机制

### ClusterIP（iptables 模式）

```
Client Pod → Service ClusterIP:Port
    │
    ▼
iptables DNAT 规则（kube-proxy 维护）
    │
    ├── KUBE-SVC-XXXX (Service 链)
    │   ├── KUBE-SEP-1 → Pod 1 IP:Port (probability 0.33)
    │   ├── KUBE-SEP-2 → Pod 2 IP:Port (probability 0.50)
    │   └── KUBE-SEP-3 → Pod 3 IP:Port (probability 1.00)
    │
    ▼
目标 Pod
```

### IPVS 模式（高性能）

```bash
# 查看 IPVS 规则
ipvsadm -Ln
# TCP  10.96.0.1:443 rr
#   -> 10.244.1.5:6443    Masq  1  0  0
#   -> 10.244.2.5:6443    Masq  1  0  0
#   -> 10.244.3.5:6443    Masq  1  0  0

# IPVS vs iptables
# iptables: O(n) 规则匹配，1000+ Service 时延迟明显
# IPVS: O(1) 哈希查找，适合大规模集群
```

### kube-proxy 模式选择

```yaml
# kube-proxy 配置（IPVS 模式）
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: "ipvs"
ipvs:
  scheduler: "rr"           # 轮询（rr/lc/dh/sh/sed/nq）
  strictARP: true           # MetalLB 需要
  tcpTimeout: 0s
  tcpFinTimeout: 0s
  udpTimeout: 0s
```

## Ingress 流量路径

```
外部客户端
    │
    ▼
DNS (api.example.com → LB IP)
    │
    ▼
云 LB / MetalLB (L4)
    │
    ▼
Ingress Controller Pod (Nginx/Traefik/Envoy)
    │
    ├── TLS 终止（证书）
    ├── 路径路由（/api → api-svc, /web → web-svc）
    ├── 限流/认证（注解配置）
    │
    ▼
Service (ClusterIP)
    │
    ▼
Pod (应用容器)
```

### Nginx Ingress 生产配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-ingress
  namespace: production
  annotations:
    # TLS
    cert-manager.io/cluster-issuer: letsencrypt-prod
    # 限流
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/limit-connections: "50"
    # 超时
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "5"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "30"
    # 缓冲
    nginx.ingress.kubernetes.io/proxy-buffering: "on"
    nginx.ingress.kubernetes.io/proxy-buffer-size: "8k"
    # 安全头
    nginx.ingress.kubernetes.io/configuration-snippet: |
      more_set_headers "X-Frame-Options: DENY";
      more_set_headers "X-Content-Type-Options: nosniff";
spec:
  ingressClassName: nginx
  tls:
    - hosts: ["api.example.com"]
      secretName: api-tls
  rules:
    - host: api.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: api-service
                port:
                  number: 8080
```

## DNS 机制

### CoreDNS 解析流程

```
Pod 内 DNS 查询: my-service.my-ns.svc.cluster.local
    │
    ▼
/etc/resolv.conf (kubelet 注入)
    nameserver 10.96.0.10    ← CoreDNS ClusterIP
    search my-ns.svc.cluster.local svc.cluster.local cluster.local
    options ndots:5
    │
    ▼
CoreDNS Pod (kube-system)
    │
    ├── cluster.local 域 → K8s API 查询 Service/Endpoints
    ├── 外部域 → 上游 DNS (8.8.8.8 / VPC DNS)
    │
    ▼
返回 Service ClusterIP 或 Pod IP (Headless)
```

### DNS 优化配置

```yaml
# CoreDNS ConfigMap 优化
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
        }
        cache 30 {
            success 9984 30
            denial 9984 5
        }
        loop
        reload
        loadbalance
    }
---
# Pod 级 DNS 优化
spec:
  dnsPolicy: ClusterFirst
  dnsConfig:
    options:
      - name: ndots
        value: "2"        # 减少无效 search 查询
      - name: timeout
        value: "2"
      - name: attempts
        value: "2"
```

## NetworkPolicy 实践

### 生产环境策略集

```yaml
# 1. 默认拒绝所有入站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes: ["Ingress"]
---
# 2. 允许 Ingress Controller 访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-from-ingress
  namespace: production
spec:
  podSelector:
    matchLabels:
      expose: "true"
  policyTypes: ["Ingress"]
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: ingress-nginx
---
# 3. 允许应用间通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-api-to-db
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: database
  policyTypes: ["Ingress"]
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: api-server
      ports:
        - port: 5432
          protocol: TCP
---
# 4. 限制出站（仅允许 DNS + 必要服务）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: restrict-egress
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  policyTypes: ["Egress"]
  egress:
    - to:
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - port: 53
          protocol: UDP
        - port: 53
          protocol: TCP
    - to:
        - podSelector:
            matchLabels:
              tier: database
      ports:
        - port: 5432
```

## 故障排查

| 问题 | 诊断 | 解决 |
|------|------|------|
| Pod 间不通 | `kubectl exec -it pod -- ping <target-ip>` | 检查 CNI/路由 |
| Service 不通 | `kubectl get endpoints <svc>` | 检查 selector |
| DNS 失败 | `kubectl exec -it pod -- nslookup kubernetes.default` | 检查 CoreDNS |
| Ingress 502 | `kubectl logs -n ingress-nginx <pod>` | 后端健康/超时 |
| 跨节点不通 | `traceroute <pod-ip>` | MTU/防火墙/CNI |
| 网络策略阻断 | `kubectl get netpol -n <ns>` | 添加允许规则 |
| 连接超时 | `tcpdump -i any port <port>` | 防火墙/安全组 |

## 性能调优

| 调优项 | 方法 | 效果 |
|--------|------|------|
| kube-proxy IPVS | 替代 iptables | 大集群 Service 查找 O(1) |
| eBPF (Cilium) | 替代 iptables 全链路 | 延迟降低 30-50% |
| DNS 缓存 | NodeLocal DNSCache | 减少 CoreDNS 压力 |
| MTU 优化 | 匹配底层网络 MTU | 避免分片 |
| 连接复用 | HTTP Keep-Alive/gRPC | 减少连接建立开销 |
| Service Topology | topologyAwareHints | 优先本地 AZ |

## Related

- [[网络/index.md|网络]]
- [[网络/K8s网络核心/index.md|K8s 网络核心]]
- [[网络/服务网格/index.md|服务网格]]
- [[网络/eBPF/index.md|eBPF]]
