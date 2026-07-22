---
title: Kubernetes Port Reference
description: Kubernetes Port Reference — Kubernetes 生产运维知识库
summary: Kubernetes Port Reference — Kubernetes 生产运维知识库
category: references
tags:
- k8s
- networking
- ports
- reference
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Port Reference 是什么
- 如何 Kubernetes Port Reference
trigger_keywords:
- Kubernetes
- Port
- Reference
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- cilium-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Port Reference

## Control Plane Ports

| Port | Protocol | Component | Direction | Notes |
|---|---|---|---|---|
| 6443 | TCP | kube-apiserver | Inbound | HTTPS API (default) |
| 2379-2380 | TCP | etcd | Internal | Client/peer communication |
| 10257 | TCP | kube-controller-manager | Inbound | HTTPS metrics |
| 10259 | TCP | kube-scheduler | Inbound | HTTPS metrics |
| 10250 | TCP | kubelet | Internal | Pod/exec, logs, metrics |
| 10248 | TCP | kubelet | Local | Health check (localhost only) |
| 10256 | TCP | kube-proxy | Inbound | Health check endpoint |

## Node Ports

| Port | Protocol | Component | Notes |
|---|---|---|---|
| 10256 | TCP | kube-proxy | Health check endpoint |
| 30000-32767 | TCP/UDP | NodePort Services | Default NodePort range |
| 179 | TCP | BGP (Calico) | BGP peering |
| 4789 | UDP | VXLAN (Calico/Flannel) | Overlay tunnel |
| 8472 | UDP | VXLAN (Flannel) | Default Flannel VXLAN |
| 51820-51821 | UDP | WireGuard (Flannel) | WireGuard encryption |
| 9099 | TCP | Calico Typha | Typha health check |
| 5473 | TCP | Calico Typha | Typha client connection |

## Addon Ports

| Port | Protocol | Addon | Notes |
|---|---|---|---|
| 53 | TCP/UDP | CoreDNS | DNS resolution |
| 9153 | TCP | CoreDNS | Prometheus metrics |
| 9090 | TCP | Prometheus | Web UI |
| 3000 | TCP | Grafana | Dashboard |
| 8080 | TCP | ArgoCD | HTTP API (before redirect) |
| 443 | TCP | ArgoCD | HTTPS UI/API |
| 9093 | TCP | Alertmanager | Alert management |
| 8443 | TCP | Metrics Server | Aggregated API |
| 8080 | TCP | Kubernetes Dashboard | Web UI |
| 20250 | TCP | kube-state-metrics | Telemetry |
| 8080 | TCP | kube-state-metrics | Metrics |
| 9100 | TCP | Node Exporter | Node metrics |
| 4194 | TCP | Hubble UI | Cilium observability |
| 16686 | TCP | Jaeger | Tracing UI |
| 4317-4318 | TCP | OTel Collector | gRPC/HTTP telemetry |

## CNI-Specific Ports

| Port | Protocol | CNI | Notes |
|---|---|---|---|
| 4194 | TCP | Cilium Hubble | Observability UI |
| 4240 | TCP | Cilium | Health server |
| 4244 | TCP | Cilium | Prometheus metrics |
| 6060 | TCP | Cilium Operator | Debug endpoint |
| 9090 | TCP | Calico Felix | Felix metrics |
| 9099 | TCP | Calico Typha | Typha health |
| 5473 | TCP | Calico Typha | Typha client |
| 8080 | TCP | Flannel | flanneld metrics |

## Storage Ports

| Port | Protocol | Component | Notes |
|---|---|---|---|
| 3260 | TCP | iSCSI | iSCSI target |
| 2049 | TCP/UDP | NFS | NFS server |
| 6789 | TCP | Ceph MON | Ceph monitor |
| 3300 | TCP | Ceph MGR | Ceph manager |
| 6800-7300 | TCP | Ceph OSD | Ceph OSD data |
| 9000 | TCP | MinIO | S3 API |
| 9001 | TCP | MinIO | Console UI |
| 10000 | TCP | Rook | Ceph dashboard |

## Service Mesh Ports

| Port | Protocol | Component | Notes |
|---|---|---|---|
| 15000 | TCP | Envoy (Istio) | Admin interface |
| 15001 | TCP | Envoy (Istio) | Outbound listener |
| 15006 | TCP | Envoy (Istio) | Inbound listener |
| 15010 | TCP | istiod | gRPC xDS (plaintext) |
| 15012 | TCP | istiod | gRPC xDS (TLS) |
| 15014 | TCP | istiod | Monitoring |
| 15017 | TCP | istiod | Webhook |
| 15020 | TCP | Envoy (Istio) | Prometheus merge |
| 15090 | TCP | Envoy (Istio) | Prometheus telemetry |
| 4143 | TCP | Linkerd | Proxy incoming |
| 4190 | TCP | Linkerd | Proxy admin |
| 4191 | TCP | Linkerd | Proxy metrics |
| 8086 | TCP | Linkerd | Control plane admin |

## 端口验证命令

```bash
# 🟢 检查控制平面端口监听
ss -tlnp | grep -E '6443|2379|2380|10257|10259'

# 🟢 检查 kubelet 端口
ss -tlnp | grep 10250

# 🟢 检查 NodePort 范围使用
kubectl get svc -A -o jsonpath='{range .items[?(@.spec.type=="NodePort")]}{.metadata.name}{"\t"}{.spec.ports[*].nodePort}{"\n"}{end}'

# 🟢 检查 CoreDNS 端口
kubectl get svc -n kube-system kube-dns -o yaml | grep -A5 ports
ss -ulnp | grep :53

# 🟢 检查 CNI 端口 (Calico BGP)
ss -tlnp | grep :179
calicoctl node status

# 🟢 检查 CNI 端口 (Cilium)
ss -tlnp | grep -E '4240|4244|4194'
cilium status

# 🟢 检查 Istio 端口
ss -tlnp | grep -E '15000|15001|15006|15012|15017'
kubectl get svc -n istio-system

# 🟢 检查防火墙规则
iptables -L INPUT -n --line-numbers | head -30
nft list ruleset | grep -E '6443|2379|10250'

# 🟢 检查端口连通性
nc -zv <master-ip> 6443
nc -zv <node-ip> 10250
nc -zv <etcd-ip> 2379
```

## Security Notes

- Ports 6443, 10250 must be restricted to cluster-internal access
- etcd ports (2379-2380) should NEVER be exposed outside the control plane
- NodePort range should be planned to avoid conflicts with host services
- Consider using NetworkPolicy to restrict inter-pod traffic
- kubelet port 10250 requires authentication (disable anonymous auth)
- Istio admin port 15000 should be disabled in production (`--concurrency` flag)
- CNI BGP port 179 only needed between cluster nodes
- Metrics ports (9090, 9100, etc.) should be restricted to monitoring namespace

## 防火墙规则模板

```bash
# Master 节点必要端口
iptables -A INPUT -p tcp --dport 6443 -j ACCEPT   # API Server
iptables -A INPUT -p tcp --dport 2379:2380 -s <master-subnet> -j ACCEPT  # etcd
iptables -A INPUT -p tcp --dport 10257 -s <master-subnet> -j ACCEPT  # controller-manager
iptables -A INPUT -p tcp --dport 10259 -s <master-subnet> -j ACCEPT  # scheduler

# Worker 节点必要端口
iptables -A INPUT -p tcp --dport 10250 -s <cluster-subnet> -j ACCEPT  # kubelet
iptables -A INPUT -p tcp --dport 30000:32767 -j ACCEPT  # NodePort
iptables -A INPUT -p tcp --dport 179 -s <cluster-subnet> -j ACCEPT  # BGP (Calico)
iptables -A INPUT -p udp --dport 4789 -s <cluster-subnet> -j ACCEPT  # VXLAN
iptables -A INPUT -p udp --dport 8472 -s <cluster-subnet> -j ACCEPT  # Flannel VXLAN

# 所有节点
iptables -A INPUT -p tcp --dport 10256 -j ACCEPT  # kube-proxy health
```

## 端口冲突排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| API Server 启动失败 | 6443 被占用 | `ss -tlnp \| grep 6443` | 停止占用进程或修改端口 |
| etcd 集群不健康 | 2379/2380 被防火墙阻止 | `nc -zv <peer> 2380` | 开放防火墙规则 |
| Pod 跨节点不通 | VXLAN/BGP 端口被阻止 | `tcpdump -i any port 4789` | 开放 UDP 4789/TCP 179 |
| NodePort 无法访问 | 端口被其他服务占用 | `ss -tlnp \| grep <port>` | 修改 NodePort 范围或释放端口 |
| DNS 解析失败 | 53 端口冲突 | `ss -ulnp \| grep :53` | 停止 systemd-resolved |
| Istio Sidecar 不工作 | 15001/15006 被占用 | `ss -tlnp \| grep 1500` | 检查端口冲突 |

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/argocd.md|argocd]] — ArgoCD
- [[概念/tcp-udp-protocol-stack.md|tcp-udp-protocol-stack]] — TCP/UDP Protocol Stack
- [[概念/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[概念/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[实体/cni-plugins.md|cni-plugins]] — CNI Plugins

<!-- risk-assessed -->
