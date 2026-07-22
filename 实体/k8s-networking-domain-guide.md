---
title: Kubernetes Networking Domain Guide
description: Kubernetes Networking Domain Guide — Kubernetes 生产运维知识库
summary: Kubernetes Networking Domain Guide — Kubernetes 生产运维知识库
category: references
tags:
- k8s
- networking
- 网络
- service
- cni
- ingress
- dns
- reference
- cilium
- flannel
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Networking Domain Guide 是什么
- 如何 Kubernetes Networking Domain Guide
trigger_keywords:
- Kubernetes
- Networking
- Domain
- Guide
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Networking Domain Guide

## Source

Distilled from 网络 (39 documents, Kubernetes v1.28-v1.32).

## Networking Model

1. **Pod-to-Pod**: Every Pod gets unique IP; pods communicate without NAT across nodes
2. **Service**: Stable virtual IP (ClusterIP) with DNS name, load balances to Pod endpoints
3. **Ingress**: L7 HTTP/HTTPS routing with TLS termination
4. **NetworkPolicy**: Pod-level firewall for ingress/egress traffic

## CNI Plugins

| Plugin | Type | Features |
|--------|------|----------|
| **Calico** | BGP | NetworkPolicy, BGP peering, IPIP/VXLAN |
| **Cilium** | eBPF | L7 policies, identity-aware, observability |
| **Flannel** | Overlay | Simple, minimal, WireGuard encryption |
| **Terway** | ENI | Alibaba Cloud native, high throughput |

## Service Types

| Type | Scope | Use |
|------|-------|-----|
| ClusterIP | Internal | Default microservice communication |
| NodePort | Node IP:port | External access without cloud LB |
| LoadBalancer | Cloud LB | Production external traffic |
| ExternalName | DNS CNAME | External service alias |

## kube-proxy Modes

| Mode | Performance | Scale |
|------|------------|-------|
| iptables | Low latency overhead, linear rule growth | <1000 Services |
| IPVS | High throughput, kernel-level LB | >1000 Services |
| eBPF | Lowest latency, bypasses TCP/IP stack | Unlimited |

## Ingress vs Gateway API

- **Ingress**: Mature, widely adopted, HTTP/HTTPS routing
- **Gateway API**: Next-generation, multi-protocol (HTTP, TCP, UDP, gRPC), multi-tenant, role-separated

## 运维操作

```bash
# 🟢 检查 CNI 插件状态
kubectl get pods -n kube-system -l k8s-app=calico-node  # Calico
kubectl get pods -n kube-system -l k8s-app=cilium  # Cilium
kubectl get pods -n kube-system -l app=flannel  # Flannel

# 🟢 检查 Service 和 Endpoints
kubectl get svc -A
kubectl get endpoints -A
kubectl get endpointslice -A

# 🟢 检查 DNS 解析
kubectl exec -it <pod> -- nslookup kubernetes.default.svc.cluster.local
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=20

# 🟢 检查 NetworkPolicy
kubectl get networkpolicy -A
kubectl describe networkpolicy <name> -n <ns>

# 🟢 检查 Ingress/Gateway
kubectl get ingress -A
kubectl get gateway,httproute -A

# 🟢 网络诊断
kubectl run debug --rm -it --image=nicolaka/netshoot -- bash
# 在 debug pod 内：
tcpdump -i eth0 -nn port 80 -c 10
ip route show
iptables -t nat -L -n | head -20

# 🟢 检查 kube-proxy 模式
kubectl get cm kube-proxy -n kube-system -o yaml | grep mode
# 或检查 IPVS 规则
ipvsadm -Ln | head -20
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 间不通 | CNI 异常/NetworkPolicy | `kubectl exec pod -- ping <ip>` | 检查 CNI Pod/策略 |
| Service 无响应 | Endpoints 为空 | `kubectl get endpoints <svc>` | 检查 selector/label |
| DNS 解析失败 | CoreDNS 异常 | `kubectl logs -n kube-system -l k8s-app=kube-dns` | 重启 CoreDNS/检查配置 |
| 外部无法访问 | Ingress/LB 配置错误 | `kubectl describe ingress` | 检查 Ingress 规则/后端 |
| 跨节点 Pod 不通 | 隧道/路由问题 | `ip route`; `tcpdump` | 检查 CNI 配置/MTU |
| 间歇性超时 | conntrack 表满/网络抨动 | `conntrack -C`; `dmesg` | 增大 conntrack/检查网络 |

### 排查流程

```
K8s 网络异常
├── Pod 无法访问？
│   ├── 同节点 Pod 互通？→ veth/bridge 问题
│   ├── 跨节点 Pod 互通？→ CNI 隧道/路由问题
│   └── Pod 到 Service？→ kube-proxy/Endpoints
├── DNS 异常？
│   ├── CoreDNS Pod 运行？
│   ├── ndots 配置导致多余查询？
│   └── 上游 DNS 可达？
├── 外部访问异常？
│   ├── Ingress 规则正确？
│   ├── 后端 Service/Endpoints 正常？
│   └── TLS 证书有效？
└── 性能问题？
    ├── MTU 不匹配？→ 检查隧道 MTU
    ├── conntrack 表满？→ 增大 nf_conntrack_max
    └── kube-proxy 规则过多？→ 切换 IPVS/eBPF
```

## 生产案例

### 案例1：MTU 不匹配导致跨节点通信异常

- **场景**：Pod 跨节点通信时大包丢失，小包正常；curl 小文件成功但大文件超时
- **排查**：`ping -s 1400 -M do <pod-ip>` 失败；VXLAN 隧道减少了 50 字节 MTU
- **方案**：设置 CNI MTU = 物理网卡 MTU - 50（VXLAN）或 - 60（Geneve）
- **效果**：跨节点通信恢复正常

### 案例2：Service 数量增长导致 iptables 性能下降

- **场景**：集群 5000+ Service，Pod 网络延迟从 1ms 增加到 20ms
- **排查**：`iptables -t nat -L | wc -l` 显示 50000+ 规则；每次数据包需线性匹配
- **方案**：切换 kube-proxy 到 IPVS 模式；或部署 Cilium 使用 eBPF 替代 iptables
- **效果**：网络延迟回落到 1ms，与 Service 数量无关

## 检查清单

- [ ] CNI 插件 Pod 在所有节点运行
- [ ] CoreDNS 副本数 >= 2 且配置了 PDB
- [ ] NetworkPolicy 已部署（生产命名空间）
- [ ] MTU 配置与网络环境匹配
- [ ] conntrack 表大小足够（高并发场景）
- [ ] Ingress/Gateway 配置了 TLS
- [ ] 网络监控已配置（延迟/丢包/带宽）
- [ ] kube-proxy 模式适合集群规模

## Related

- [[reference|#reference Hub]] — tag hub

- [[cilium]] — Cilium
- [[grpc]] — gRPC
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/service-networking.md|service-networking]] — Service Networking
- [[概念/service-networking.md|Service Networking]]
- [[实体/cni-plugins.md|CNI Plugins]]
- [[实体/networkpolicy.md|NetworkPolicy]]


<!-- risk-assessed -->
