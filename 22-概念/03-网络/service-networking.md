---
title: Service Networking
description: '- [[22-概念/03-网络/cni-networking-model.md|cni-networking-model]] — CNI 网络模型与插件对比'
summary: '- [[22-概念/03-网络/cni-networking-model.md|cni-networking-model]] — CNI 网络模型与插件对比'
category: concepts
tags:
- k8s
- networking
- service
- kube-proxy
- load-balancing
- dns
- cilium
- coredns
- ingress
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Service Networking 是什么
- 如何 Service Networking
trigger_keywords:
- Service
- Networking
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Service|Service]] Networking

## Service Types

| Type | Purpose | Use Case |
|------|---------|----------|
| **ClusterIP** | Internal virtual IP | Default; microservice-to-microservice communication |
| **NodePort** | Expose on each node's IP:port | External access without cloud load balancer |
| **LoadBalancer** | Cloud provider LB integration | Production external traffic (SLB/ALB/NLB) |
| **ExternalName** | DNS CNAME to external name | Access external services by DNS alias |

## Service Discovery

Clients discover services via DNS:
- **FQDN**: `my-svc.my-ns.svc.cluster.local`
- **CoreDNS** resolves to ClusterIP
- **kube-proxy** routes ClusterIP to backend [[Pods|Pods]]

## Load Balancing Modes

| Mode | Latency | Throughput | Service Scale | Recommended |
|------|---------|------------|---------------|-------------|
| **iptables** | High | Low | <1000 Services | Small clusters |
| **IPVS** | Medium | High | >1000 Services | Production clusters |
| **eBPF (Cilium)** | Lowest | Highest | Unlimited | High-performance, modern kernels |

## EndpointSlice

Since Kubernetes v1.21, EndpointSlice replaces Endpoints as the scalable way to track Service backends. EndpointSlice supports:
- Up to 100 endpoints per slice (vs 1000 in Endpoints)
- Multiple address types (IPv4, IPv6, FQDN)
- Topology-aware routing

## [[Ingress|Ingress]] and Gateway API

- **Ingress**: L7 HTTP/HTTPS routing with TLS termination (nginx, ALB, etc.)
- **Gateway API**: Next-generation successor to Ingress with richer routing, multi-tenant support, and standardized resource types (HTTPRoute, TCPRoute, etc.)

## 源码实现分析

### Service 流量路径

```
Client Pod
    │
    ├── DNS 查询: my-svc.my-ns.svc.cluster.local
    │       └── CoreDNS 返回 ClusterIP (10.96.0.100)
    │
    ├── 连接 ClusterIP:80
    │       │
    │       ├── iptables 模式:
    │       │     KUBE-SERVICES → KUBE-SVC-XXX → KUBE-SEP-XXX (DNAT)
    │       │
    │       ├── IPVS 模式:
    │       │     虚拟服务器 → 真实服务器 (加权轮询)
    │       │
    │       └── eBPF (Cilium):
    │             内核层直接转发，无 iptables
    │
    └── 到达 Backend Pod
```

### EndpointSlice 示例

```yaml
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: my-svc-abc12
  labels:
    kubernetes.io/service-name: my-svc
addressType: IPv4
ports:
- name: http
  port: 8080
  protocol: TCP
endpoints:
- addresses: ["10.244.1.5"]
  conditions:
    ready: true
  nodeName: node-1
  zone: us-east-1a
- addresses: ["10.244.2.8"]
  conditions:
    ready: true
  nodeName: node-2
  zone: us-east-1b
```

## 源码实现分析

### kube-proxy IPVS 模式实现

```go
// k8s.io/kubernetes/pkg/proxy/ipvs/proxier.go
// kube-proxy IPVS 模式：为每个 Service 创建虚拟服务器
func (proxier *Proxier) syncProxyRules() {
    // 1. 为每个 ClusterIP 创建 IPVS 虚拟服务器
    for svcName, svc := range proxier.serviceMap {
        vserver := &ipvs.VirtualServer{
            Address:  net.ParseIP(svc.ClusterIP()),
            Port:     uint16(svc.Port()),
            Protocol: string(svc.Protocol()),
            Scheduler: "rr",  // 轮询调度
        }
        proxier.ipvs.AddVirtualServer(vserver)
        
        // 2. 为每个 Endpoint 添加真实服务器
        for _, ep := range proxier.endpointsMap[svcName] {
            realServer := &ipvs.RealServer{
                Address: net.ParseIP(ep.IP()),
                Port:    uint16(ep.Port()),
                Weight:  1,
            }
            proxier.ipvs.AddRealServer(vserver, realServer)
        }
    }
}
```

### Service 网络架构

```
┌───────────────────────────────────────────────────────────┐
│          Service 网络架构                              │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  客户端 Pod                                              │
│    │  curl http://my-svc:80                              │
│    ▼                                                      │
│  CoreDNS: my-svc.default.svc.cluster.local → 10.96.0.100│
│    │                                                      │
│    ▼                                                      │
│  kube-proxy 数据路径 (三选一):                           │
│    ├─ iptables: KUBE-SERVICES → DNAT → Pod IP          │
│    ├─ IPVS: ipvsadm 虚拟服务器 → RealServer           │
│    └─ eBPF (Cilium): 内核层直接转发，无 iptables     │
│    │                                                      │
│    ▼                                                      │
│  目标 Pod (10.244.1.5:8080)                              │
│                                                           │
│  性能对比:                                               │
│  iptables: O(n) 规则匹配，<1000 Service              │
│  IPVS:     O(1) 哈希查找，>1000 Service              │
│  eBPF:     内核层处理，最高性能，无 conntrack        │
└───────────────────────────────────────────────────────────┘
```

### EndpointSlice 控制器（🟢 只读观察）

```bash
# 查看 Service 的 EndpointSlice
kubectl get endpointslices -n default -l kubernetes.io/service-name=my-svc

# 查看详细信息
kubectl get endpointslices my-svc-abc12 -n default -o yaml

# 对比 iptables vs IPVS 模式
kubectl get nodes -o jsonpath='{.items[0].metadata.labels.kubernetes\.io/hostname}'
# 检查 kube-proxy 模式
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=20 | grep -i "using.*mode"
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| ClusterIP 是 Pod IP | ClusterIP 是虚拟 IP，由 kube-proxy 转发 |
| Service 负载均衡是 L7 | Service 是 L4 (TCP/UDP)，L7 需 Ingress |
| NodePort 适合生产 | NodePort 占用节点端口，生产用 LoadBalancer |
| Endpoints 和 EndpointSlice 相同 | EndpointSlice 是分片版本，更可扩展 |
| headless Service 没有用 | headless 用于 StatefulSet 和客户端负载均衡 |

## 面试要点

1. **Service 的四种类型分别适用什么场景？**
   - ClusterIP: 内部服务间通信
   - NodePort: 无云 LB 的外部访问
   - LoadBalancer: 生产环境外部流量
   - ExternalName: 外部服务 DNS 别名

2. **iptables vs IPVS vs eBPF 的区别？**
   - iptables: O(n) 规则匹配，小规模集群
   - IPVS: O(1) 哈希查找，大规模集群
   - eBPF: 内核层处理，最高性能

3. **EndpointSlice 解决了什么问题？**
   - Endpoints 单对象过大 (5000 Pod = 1.5MB)
   - EndpointSlice 分片 (100 endpoints/slice)
   - 支持拓扑感知路由

## Related

- [[22-概念/03-网络/cni-networking-model.md|cni-networking-model]] — CNI 网络模型与插件对比
- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[23-实体/02-K8s核心组件/cni-plugins.md|CNI Plugins]]
- [[coredns|CoreDNS]]
- Kubernetes Network Model
- Ingress Controller

- 10-service-networking-events

<!-- risk-assessed -->
