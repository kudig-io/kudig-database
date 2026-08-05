---
title: eBPF 网络应用实战
description: 'Cilium CNI 高级配置、XDP 负载均衡、TC 流量控制与高性能 Service Mesh'
summary: 'Cilium CNI 高级配置、XDP 负载均衡、TC 流量控制与高性能 Service Mesh'
category: specialized-tech
tags:
- ebpf
- cilium
- xdp
- katran
- service-mesh
- cni
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Cilium CNI 是什么
- 如何配置 Cilium 高级网络策略
- XDP 负载均衡如何工作
trigger_keywords:
- cilium
- cni
- xdp
- katran
- service-mesh
- ebpf
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# eBPF 网络应用实战

## 1. Cilium CNI 架构

Cilium 基于 eBPF 的 Kubernetes CNI 实现，替代 iptables/IPVS：

```
Pod → eBPF Datapath → 网络策略 → Service 负载均衡 → 目标 Pod
  │                      │              │
  │                      └── L3/L4/L7   └── Maglev/随机/轮询
  └── veth pair / ipvlan
```

核心特性：

| 特性 | 说明 |
|------|------|
| **eBPF Datapath** | 替代 iptables，O(1) 性能 |
| **Network Policy** | L3/L4/L7 网络策略 |
| **Service Mesh** | Sidecar-free Service Mesh |
| **Hubble** | 网络可观测平台 |
| **Cluster Mesh** | 多集群网络 |

## 2. Cilium 安装与配置

### 2.1 基础安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 安装
helm repo add cilium https://helm.cilium.io/
helm repo update

helm install cilium cilium/cilium \
  --namespace kube-system \
  --set kubeProxyReplacement=strict \
  --set k8sServiceHost=10.0.0.10 \
  --set k8sServicePort=6443 \
  --set hubble.enabled=true \
  --set hubble.relay.enabled=true \
  --set hubble.ui.enabled=true
```
### 2.2 高级配置

```yaml
# values-cilium.yaml
kubeProxyReplacement: strict
k8sServiceHost: "10.0.0.10"
k8sServicePort: "6443"

# eBPF 配置
bpf:
  hostLegacyRouting: false
  masquerade: true
  tproxy: true
  preallocateMaps: true

# IPAM 配置
ipam:
  mode: "kubernetes"
  operator:
    clusterPoolIPv4PodCIDR: "10.244.0.0/16"
    clusterPoolIPv4MaskSize: "24"

# Hubble 可观测
hubble:
  enabled: true
  listenAddress: ":4244"
  metrics:
    enabled:
      - dns
      - drop
      - tcp
      - flow
      - icmp
      - http
  relay:
    enabled: true
  ui:
    enabled: true

# 网络策略
policyEnforcement: "default"
policyAuditMode: false

# 高级特性
enableIPv4Masquerade: true
enableIPv6Masquerade: false
enableHostLegacyRouting: false
tunnel: "disabled"    # native routing 模式
autoDirectNodeRoutes: true
```

### 2.3 带宽管理（BBR）

```yaml
# 启用 BBR 拥塞控制
bandwidthManager:
  enabled: true
  bbr: true
```

## 3. Cilium Network Policy

### 3.1 L3/L4 策略

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: backend-policy
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
            - port: "8080"
              protocol: TCP
    - fromEndpoints:
        - matchLabels:
            app: monitoring
      toPorts:
        - ports:
            - port: "9090"
              protocol: TCP
  egress:
    - toEndpoints:
        - matchLabels:
            app: database
      toPorts:
        - ports:
            - port: "5432"
              protocol: TCP
    - toFQDNs:
        - matchName: "api.external.com"
      toPorts:
        - ports:
            - port: "443"
              protocol: TCP
```

### 3.2 L7 策略（HTTP）

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-l7-policy
spec:
  endpointSelector:
    matchLabels:
      app: api-gateway
  ingress:
    - fromEndpoints:
        - matchLabels:
            app: frontend
      toPorts:
        - ports:
            - port: "8080"
              protocol: TCP
        - rules:
            http:
              - method: GET
                path: "/api/v1/.*"
              - method: POST
                path: "/api/v1/orders"
              - method: GET
                path: "/healthz"
```

### 3.3 DNS 策略

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: dns-policy
spec:
  endpointSelector:
    matchLabels:
      app: backend
  egress:
    - toEndpoints:
        - matchLabels:
            "k8s:io.kubernetes.pod.namespace": kube-system
            "k8s:k8s-app": kube-dns
      toPorts:
        - ports:
            - port: "53"
              protocol: UDP
          rules:
            dns:
              - matchPattern: "*.production.svc.cluster.local"
              - matchPattern: "*.external.com"
    - toFQDNs:
        - matchName: "db.external.com"
      toPorts:
        - ports:
            - port: "5432"
              protocol: TCP
```

## 4. XDP 负载均衡

### 4.1 XDP 程序示例

```c
// xdp_lb.c - 简单的 XDP 负载均衡器
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 256);
    __type(key, __u32);     // 目标 IP
    __type(value, __u32);   // 后端 IP
} backends SEC(".maps");

SEC("xdp")
int xdp_load_balancer(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;

    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;

    struct iphdr *iph = (void *)(eth + 1);
    if ((void *)(iph + 1) > data_end)
        return XDP_PASS;

    // 只处理 TCP
    if (iph->protocol != IPPROTO_TCP)
        return XDP_PASS;

    // 查找后端
    __u32 vip = iph->daddr;
    __u32 *backend = bpf_map_lookup_elem(&backends, &vip);
    if (!backend)
        return XDP_PASS;

    // 替换目标 IP
    iph->daddr = *backend;

    // 重新计算校验和
    iph->check = 0;
    iph->check = bpf_csum_diff(0, 0, (__be32 *)iph, sizeof(*iph), 0);

    // 修改 MAC 地址（简化）
    // ...

    return XDP_TX;
}
```

### 4.2 Katran（Facebook XDP 负载均衡器）

```bash
# Katran 架构
# 用户态：管理后端池、健康检查、配置更新
# 内核态：XDP 程序处理每个数据包

# Katran 核心特性：
# - Maglev 一致性哈希
# - GUE/GIP 封装
# - 健康检查
# - DDoS 防护
```

### 4.3 XDP 与 Cilium 集成

```yaml
# Cilium 使用 XDP 加速 Service 负载均衡
# values-cilium.yaml
loadBalancer:
  algorithm: maglev    # 一致性哈希
  acceleration: native # XDP 加速
```

## 5. TC 流量控制

### 5.1 TC 与 eBPF 集成

```c
// tc_mark.c - 使用 TC eBPF 标记流量
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

SEC("tc")
int tc_mark_priority(struct __sk_buff *skb) {
    void *data = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;

    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return TC_ACT_OK;

    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return TC_ACT_OK;

    struct iphdr *iph = (void *)(eth + 1);
    if ((void *)(iph + 1) > data_end)
        return TC_ACT_OK;

    // 标记高优先级流量
    if (iph->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp = (void *)(iph + 1);
        if ((void *)(tcp + 1) > data_end)
            return TC_ACT_OK;

        // HTTPS 流量标记为高优先级
        if (tcp->dest == bpf_htons(443)) {
            skb->priority = 100;
        }
    }

    return TC_ACT_OK;
}
```

### 5.2 TC 命令配置

```bash
# 加载 eBPF TC 程序
tc qdisc add dev eth0 clsact
tc filter add dev eth0 ingress bpf da obj tc_mark.o sec tc
tc filter add dev eth0 egress bpf da obj tc_mark.o sec tc

# 查看已加载的 TC 程序
tc filter show dev eth0 ingress
```

## 6. IPVS 替代方案

### 6.1 Cilium 替代 kube-proxy

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 禁用 kube-proxy，使用 Cilium eBPF
helm upgrade cilium cilium/cilium \
  --namespace kube-system \
  --set kubeProxyReplacement=strict \
  --set k8sServiceHost=10.0.0.10 \
  --set k8sServicePort=6443

# 验证
cilium status
cilium service list
```
### 6.2 性能对比

| 方案 | 每秒连接数 | 延迟 P99 | CPU 开销 |
|------|-----------|----------|----------|
| iptables | 50K | 5ms | 高 |
| IPVS | 200K | 2ms | 中 |
| Cilium eBPF | 500K | 0.5ms | 低 |

### 6.3 DSR（Direct Server Return）

```yaml
# 启用 DSR 模式
loadBalancer:
  mode: dsr    # 直接服务器返回
  dsrEncapsulation: geneve
```

## 7. 高性能 Service Mesh

### 7.1 Cilium Service Mesh（Sidecar-free）

```yaml
# 启用 Cilium Service Mesh
kubeProxyReplacement: strict
hubble:
  enabled: true
  relay:
    enabled: true
  ui:
    enabled: true
```

### 7.2 L7 负载均衡

```yaml
apiVersion: cilium.io/v2
kind: CiliumEnvoyConfig
metadata:
  name: envoy-config
  namespace: production
spec:
  services:
    - name: backend
      namespace: production
  backendServices:
    - name: backend
      namespace: production
  resources:
    - "@type": type.googleapis.com/envoy.config.listener.v3.Listener
      name: envoy-l7-listener
      filter_chains:
        - filters:
            - name: envoy.filters.network.http_connection_manager
              typed_config:
                "@type": type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager
                stat_prefix: ingress_http
                route_config:
                  virtual_hosts:
                    - name: backend
                      domains: ["*"]
                      routes:
                        - match:
                            prefix: "/api/v1"
                          route:
                            cluster: backend
                http_filters:
                  - name: envoy.filters.http.router
```

### 7.3 mTLS 加密

```yaml
# 启用 WireGuard 加密
encryption:
  enabled: true
  type: wireguard
  nodeEncryption: true
```

### 7.4 SPIFFE/SPIRE 集成

```yaml
# SPIRE 集成
authentication:
  enabled: true
  mutual:
    spire:
      enabled: true
      install:
        enabled: true
```

## 8. 多集群网络（Cluster Mesh）

### 8.1 配置

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 集群 1
helm install cilium cilium/cilium \
  --namespace kube-system \
  --set cluster.name=cluster1 \
  --set cluster.id=1 \
  --set etcd.enabled=true \
  --set etcd.managed=true

# 集群 2
helm install cilium cilium/cilium \
  --namespace kube-system \
  --set cluster.name=cluster2 \
  --set cluster.id=2 \
  --set etcd.enabled=true \
  --set etcd.managed=true
```
### 8.2 跨集群服务发现

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: cross-cluster-policy
spec:
  endpointSelector:
    matchLabels:
      app: backend
  ingress:
    - fromEndpoints:
        - matchLabels:
            "io.cilium.k8s.namespace.labels.cluster": "cluster2"
            app: frontend
```

## 9. 监控与排障

```bash
# Cilium 状态
cilium status

# 查看 eBPF 程序
cilium bpf lb list
cilium bpf endpoint list

# Hubble 网络流
hubble observe --namespace production --since 1h

# 策略审计
cilium monitor --type drop
cilium monitor --type policy-verdict

# 端到端延迟
cilium connectivity test
```

---

## Related

- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-15-specialized-tech/06-ebpf-programming/01-ebpf-programming-fundamentals|eBPF 开发基础]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-15-specialized-tech/06-ebpf-programming/02-ebpf-observability-tools|eBPF 可观测工具]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-15-specialized-tech/06-ebpf-programming/04-ebpf-security-runtime|eBPF 安全运行时]]

## See Also

- [Cilium 官方文档](https://docs.cilium.io/)
- [Cilium Network Policy](https://docs.cilium.io/en/stable/network/kubernetes/policy/)
- [Hubble](https://docs.cilium.io/en/stable/observability/)


<!-- risk-assessed -->
