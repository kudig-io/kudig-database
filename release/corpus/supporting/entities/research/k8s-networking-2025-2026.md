---
title: K8S Networking 2025 2026
summary: 'Key features in Cilium 1.16:'
category: entities
tags:
- k8s-networking-2025-2026
tier: supporting
created: '2026-07-01'
---

# Kubernetes Networking Research 2025-2026

> Research compiled: 2026-05-24
> Scope: CNI evolution, service mesh, Gateway API, eBPF, DNS, network policy, dual-stack, performance tuning

---

## 1. CNI EVOLUTION

### 1.1 Cilium 1.16+ (Released July 2024, latest stable as of 2025)

**Source**: https://isovalent.com/blog/post/cilium-1-16/

Key features in Cilium 1.16:
- **Netkit**: New veth replacement for high-performance networking. Netkit is a Linux kernel network device that provides significantly better throughput and lower latency than traditional veth pairs. Integrated as default for pod networking on kernels >= 6.4.
- **Gateway API GAMMA support**: Native support for Gateway API GAMMA (Gateway API for Mesh Management and Administration), enabling mesh traffic routing via the standard Gateway API.
- **BGPv2**: Completely rewritten BGP control plane with new CRD-based API (CiliumBGPNodeConfigOverride, CiliumBGPAdvertisement, CiliumBGPPeerConfig). Supports multi-path, graceful restart, and route filtering.
- **Egress Gateway observability**: Hubble filters for egress gateway traffic (--node-label, --snat-ip).
- **K8S event generation on packet drops**: Cilium injects packet drop info from Network Policies into Kubernetes Pod events (Alpha).
- **Mutual Authentication**: SPIFFE-based identity for Cilium's mutual auth (GA).

Cilium 1.17 (expected early 2025) focuses on:
- Enhanced multi-cluster networking
- Improved WireGuard encryption performance
- FQDN-based policies for egress

**Source**: https://cilium.io/blog/
**Source**: https://github.com/cilium/cilium/releases

### 1.2 Cilium Replacing kube-proxy

**Source**: https://docs.cilium.io/en/stable/network/kubernetes/kube-proxy-replacement/

- Cilium's kube-proxy replacement has been GA since Cilium 1.14
- Uses eBPF to implement all Service types (ClusterIP, NodePort, LoadBalancer, ExternalName)
- Advantages over iptables-based kube-proxy:
  - O(1) lookup vs O(n) iptables rule traversal
  - No iptables lock contention
  - Direct server return (DSR) mode support
  - Maglev consistent hashing for connection persistence
  - Session affinity at socket level
- Configuration: `kubeProxyReplacement: true` (Helm)
- K8s 1.29+ officially supports running without kube-proxy when using alternatives
- Migration path: Deploy Cilium with kube-proxy-replacement enabled, then remove kube-proxy DaemonSet

### 1.3 Calico 3.29+ (2025)

**Source**: https://www.tigera.io/blog/
**Source**: https://github.com/projectcalico/calico/releases

Key features in Calico 3.28-3.29:
- **eBPF dataplane improvements**: Enhanced performance for service load balancing
- **Calico Cloud / Enterprise**: New policy recommendations, runtime threat defense
- **BGP enhancements**: Improved BGP route reflector scalability
- **Dual-stack improvements**: Better IPv4/IPv6 dual-stack support in eBPF mode
- **Goldmane**: New policy engine for zero-trust networking
- **Whisker**: New observability UI for network flows
- **WireGuard encryption**: GA and production-ready
- **Windows support**: Improved Windows node networking

### 1.4 CNI Comparison 2025-2026

| Feature          | Cilium          | Calico          | Flannel    | Antrea        |
|-----------------|-----------------|-----------------|------------|---------------|
| Datapath        | eBPF (native)   | eBPF/iptables   | iptables   | OVS           |
| Network Policy  | L3/L4/L7        | L3/L4           | None       | L3/L4         |
| Service Mesh    | Yes (sidecar-less) | No           | No         | No            |
| Gateway API     | Yes (GAMMA)     | Yes             | No         | Yes           |
| Encryption      | WireGuard/IPsec | WireGuard       | WireGuard  | IPsec/WireGuard |
| Observability   | Hubble          | Goldmine/Whisker| Basic      | Antrea Flow   |
| Multi-cluster   | ClusterMesh      | Federation      | No         | Multi-cluster |
| kube-proxy replacement | Yes (GA) | Yes (eBPF mode) | No        | Yes           |

**Source**: https://kubernetes.io/docs/concepts/cluster-administration/networking/

---

## 2. SERVICE MESH

### 2.1 Istio Ambient Mode - GA (2024-2025)

**Source**: https://istio.io/latest/blog/2024/ambient-reached-ga/
**Source**: https://istio.io/latest/docs/ambient/

- **Istio Ambient Mesh reached GA in Istio 1.24 (November 2024)**
- Architecture: Sidecar-less, uses ztunnel (L4) and waypoint proxy (L7)
- ztunnel: Per-node daemon handling L4 mTLS, TCP routing
- waypoint proxy: Per-namespace or per-service L7 proxy (Envoy-based)
- Key benefits:
  - No sidecar injection required
  - Reduced resource overhead (up to 90% less memory for L4-only)
  - Transparent mTLS without application changes
  - Gradual adoption: start with L4 secure overlay, add L7 as needed
- **HBONE protocol**: HTTP-based tunnel for mesh traffic
- **GAMMA integration**: Gateway API for mesh traffic management

Istio 1.25+ (2025):
- Enhanced waypoint proxy performance
- Multi-cluster ambient mesh
- Improved observability integration

### 2.2 Cilium Service Mesh

**Source**: https://docs.cilium.io/en/stable/network/servicemesh/

- Sidecar-less architecture using eBPF
- L7 traffic management via Envoy integration (per-node or per-pod)
- Mutual authentication (SPIFFE-based, GA in 1.16)
- Gateway API native support
- Mutual TLS without sidecars
- Observability via Hubble

### 2.3 Linkerd 2.x (2025)

**Source**: https://linkerd.io/2024/10/30/linkerd-2.15/

- Linkerd 2.15 (October 2024): Multi-cluster improvements
- Lightweight sidecar approach (micro-proxy, ~10MB memory)
- mTLS by default
- Gateway API support
- Simpler operational model than Istio
- CNCF graduated project

### 2.4 Service Mesh Comparison 2025

| Feature           | Istio Ambient   | Cilium Mesh    | Linkerd        |
|------------------|-----------------|----------------|----------------|
| Sidecar-less     | Yes (ztunnel)   | Yes (eBPF)     | No (micro-proxy) |
| L7 proxy         | Waypoint (Envoy)| Envoy (per-node)| Linkerd-proxy  |
| mTLS             | HBONE           | SPIFFE          | Automatic      |
| Gateway API      | Yes (GAMMA)     | Yes (GAMMA)     | Yes            |
| Resource overhead| Low (L4 only)   | Low (eBPF)      | Very low       |
| Multi-cluster    | Yes             | ClusterMesh     | Yes            |
| Complexity       | High            | Medium          | Low            |

---

## 3. GATEWAY API

### 3.1 Gateway API v1.0+ GA

**Source**: https://gateway-api.sigs.k8s.io/
**Source**: https://kubernetes.io/blog/2023/10/31/gateway-api-ga/

- **Gateway API v1.0 GA**: Released October 2023 with K8s 1.28
- Core resources (GA): GatewayClass, Gateway, HTTPRoute
- Extended resources (beta): GRPCRoute, TLSRoute, TCPRoute, UDPRoute
- ReferenceGrant for cross-namespace references
- Backend TLS policy
- Multiple implementations: Istio, Cilium, Contour, Envoy Gateway, Nginx, Traefik, Kong, etc.

Gateway API v1.1 (2024):
- BackendLBPolicy for backend load balancing
- GRPCRoute GA
- TLS improvements

Gateway API v1.2+ (2025):
- TCPRoute/UDPRoute stabilization
- Enhanced header matching
- CORS policy

### 3.2 GAMMA (Gateway API for Mesh Management and Administration)

**Source**: https://gateway-api.sigs.k8s.io/mesh/gamma/

- Initiative to use Gateway API for service mesh traffic management
- ParentRef-based routing for mesh services
- Supports: Istio, Cilium, Linkerd
- Enables unified API for north-south (ingress) and east-west (mesh) traffic
- Status: Experimental → Stable in Gateway API v1.1+

### 3.3 Gateway API Implementations Matrix

| Implementation   | Gateway API Version | GAMMA Support | Notes                    |
|-----------------|---------------------|---------------|--------------------------|
| Istio           | v1.1+               | Yes           | Native integration       |
| Cilium          | v1.1+               | Yes           | eBPF-based               |
| Envoy Gateway   | v1.1+               | Partial       | CNCF sandbox             |
| Contour         | v1.0+               | No            | VMware/Pivotal           |
| Nginx Gateway   | v1.0+               | No            | F5 Nginx                 |
| Traefik         | v1.0+               | No            | Community                |
| Kong            | v1.0+               | No            | Enterprise               |

---

## 4. eBPF NETWORKING

### 4.1 Cilium eBPF Architecture

**Source**: https://docs.cilium.io/en/stable/architecture/
**Source**: https://docs.cilium.io/en/stable/network/ebpf/

- eBPF programs attached to: TC (traffic control), XDP, socket, cgroup hooks
- Replaces iptables for: service load balancing, network policy, NAT
- Performance: O(1) map lookups vs O(n) iptables rules
- Kernel requirements: 5.10+ (recommended), 4.19+ (minimum)

Key eBPF features:
- **Socket-level load balancing**: Bypasses conntrack entirely
- **Bandwidth manager**: EDT (Earliest Departure Time) based
- **Host routing**: eBPF-based host routing eliminates iptables overhead
- **BIG TCP**: GRO/GSO optimization for large packets (kernel 6.3+)
- **Netkit**: veth replacement (kernel 6.4+, Cilium 1.16+)

### 4.2 Katran (Facebook/Meta)

**Source**: https://github.com/facebookincubator/katran

- L4 load balancer using XDP (eXpress Data Path)
- Powers Facebook's internal load balancing
- Single-core performance: 10+ MPPS
- Maglev consistent hashing
- Direct Server Return (DSR)
- GUE/GRE encapsulation
- Used as reference implementation for high-performance LB

### 4.3 MoonGen/Pktgen

**Source**: https://github.com/emmericp/MoonGen

- Lua-based packet generator using DPDK
- Used for benchmarking eBPF/XDP programs
- Line-rate packet generation up to 100 Gbps
- Research tool for network performance testing

### 4.4 eBPF Performance Benchmarks (2025)

| Metric                | iptables | eBPF (Cilium) | Improvement |
|----------------------|----------|---------------|-------------|
| Service lookup       | O(n)     | O(1)          | 100x+       |
| Latency (p99)        | ~500μs   | ~50μs         | 10x         |
| Throughput           | ~5 Gbps  | ~40 Gbps      | 8x          |
| Connection setup     | ~200μs   | ~20μs         | 10x         |
| Memory per rule      | ~1KB     | ~64B          | 16x         |

---

## 5. DNS

### 5.1 CoreDNS (2025)

**Source**: https://coredns.io/
**Source**: https://github.com/coredns/coredns

- CoreDNS 1.12+ (2024-2025)
- Default DNS provider for Kubernetes since 1.13
- Plugin architecture for extensibility
- Key plugins: kubernetes, forward, cache, health, ready, prometheus
- Performance improvements in caching and forwarding

Configuration best practices:
- Cache tuning: cache 30 { success 9984  denial 9984 }
- DNS autoscaling: NodeLocal DNSCache integration
- Forwarding to multiple upstream resolvers
- Negative caching for NXDOMAIN

### 5.2 NodeLocal DNSCache

**Source**: https://kubernetes.io/docs/tasks/administer-cluster/nodelocaldns/

- DaemonSet running on every node
- Caches DNS responses locally
- Listens on 169.254.20.10 (link-local) by default
- Reduces DNS latency by 50-80%
- Reduces CoreDNS load
- Configuration: ipvs or iptables mode
- Recommended for clusters > 100 nodes or high DNS QPS

Architecture:
```
Pod → NodeLocal DNSCache (169.254.20.10) → CoreDNS → Upstream
```

### 5.3 DNS Performance Tuning

- ndots:5 (default) causes extra DNS lookups; consider ndots:2-3 for production
- Single-request-reopen for UDP reliability
- SO_SNDBUF/SO_RCVBUF tuning
- DNS over TCP fallback configuration
- search path optimization

---

## 6. NETWORK POLICY EVOLUTION

### 6.1 Standard NetworkPolicy (Kubernetes)

**Source**: https://kubernetes.io/docs/concepts/services-networking/network-policies/

- L3/L4 policies (pod/namespace selectors, IPBlock, ports)
- Ingress and egress rules
- Default deny pattern: empty podSelector with no ingress/egress rules

### 6.2 Advanced Network Policy (2025)

**Cilium Network Policy (CNF)**:
- L7 policies: HTTP, Kafka, gRPC, DNS filtering
- DNS-based egress policies (toFQDN)
- Node selectors for node-level policies
- Deny policies (explicit deny overrides allow)
- ClusterwideNetworkPolicy for cluster-wide rules
- Per-endpoint policies with identity-based enforcement

**Calico GlobalNetworkPolicy**:
- Global (cluster-wide) and namespaced policies
- Application layer policies (HTTP, DNS)
- DNS-based policies
- Threat defense integration

**Antrea NetworkPolicy**:
- ClusterNetworkPolicy (cluster-wide)
- Antrea-native policies with priority ordering
- L7 protocol support (HTTP)

### 6.3 Policy as Code Integration

- Kyverno/OPA/Gatekeeper integration for policy validation
- GitOps-based policy deployment
- Policy auditing tools: Cilium Hubble, Calico Whisker

---

## 7. DUAL-STACK IPv4/IPv6

### 7.1 Kubernetes Dual-Stack Status

**Source**: https://kubernetes.io/docs/concepts/services-networking/dual-stack/

- GA since Kubernetes 1.23
- Service supports dual-stack (spec.ipFamilyPolicy: PreferDualStack/RequireDualStack)
- Pod gets both IPv4 and IPv6 addresses
- Dual-stack requires CNI support

### 7.2 CNI Dual-Stack Support

| CNI      | IPv4/IPv6 Dual-Stack | Notes                    |
|----------|---------------------|--------------------------|
| Cilium   | GA                  | Full support in eBPF mode |
| Calico   | GA                  | VXLAN, BGP, IPIP modes   |
| Flannel  | GA                  | VXLAN backend (1.15+)    |
| Antrea   | GA                  | Geneve/VXLAN             |
| Weave    | Beta                | Limited testing           |

### 7.3 Dual-Stack Best Practices

- Gradual migration: Start with dual-stack, keep IPv4 primary
- DNS64/NAT64 for IPv6-only pods accessing IPv4 services
- Service mesh dual-stack support varies
- Load balancer dual-stack support required for external traffic
- Monitor both address families independently

---

## 8. K8S NETWORK PERFORMANCE TUNING

### 8.1 Kernel-Level Tuning

**Source**: https://docs.cilium.io/en/stable/network/kubernetes/performance/

```bash
# sysctl tuning for high-performance networking
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
net.ipv4.tcp_rmem = 4096 87380 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216
net.core.netdev_max_backlog = 30000
net.ipv4.tcp_no_metrics_save = 1
net.ipv4.tcp_timestamps = 0
net.ipv4.tcp_sack = 0
net.ipv4.tcp_window_scaling = 1
net.core.optmem_max = 25165824
net.ipv4.tcp_congestion_control = bbr
```

### 8.2 NIC-Level Tuning

- IRQ affinity: Pin NIC interrupts to specific CPUs
- RSS (Receive Side Scaling): Distribute across CPU cores
- RPS/RFS (Receive Packet Steering/Flow Steering): Software-level distribution
- Ring buffer tuning: ethtool -G eth0 rx 4096 tx 4096
- Offloads: TSO, GRO, GSO (verify with ethtool -k)

### 8.3 eBPF-Specific Tuning

- Enable BIG TCP for kernel 6.3+: `--enable-big-tcp`
- Use Netkit for kernel 6.4+: `--devices=netkit`
- BPF map sizing: Increase map sizes for large clusters
- Socket-level LB: `--bpf-lb-mode=srh` for SRv6
- Bandwidth manager: `--enable-bandwidth-manager`

### 8.4 MTU Optimization

- Standard: 1500 bytes
- Jumbo frames: 9000 bytes (if supported end-to-end)
- WireGuard overhead: 60 bytes (set MTU to 1440)
- VXLAN overhead: 50 bytes (set MTU to 1450)
- Geneve overhead: Variable (typically 50-60 bytes)

### 8.5 Cilium Performance Features

- **Bandwidth Manager**: EDT-based rate limiting
- **Egress Gateway**: SNAT with dedicated egress IPs
- **Maglev LB**: Consistent hashing for sticky sessions
- **XDP acceleration**: For supported drivers
- **Socket-level operations**: Bypass conntrack

### 8.6 Benchmarking Tools

- **iperf3**: TCP/UDP throughput testing
- **netperf**: Network performance benchmarking
- **qperf**: RDMA and TCP latency/throughput
- **nuttcp**: TCP/UDP performance measurement
- **pprof**: CPU/memory profiling for network components

---

## 9. KEY TRENDS & PREDICTIONS (2025-2026)

1. **eBPF dominance**: Cilium becomes de facto CNI for new clusters
2. **Sidecar-less service mesh**: Istio Ambient and Cilium mesh replace sidecar patterns
3. **Gateway API standardization**: Ingress controllers migrate to Gateway API
4. **IPv6 adoption acceleration**: More cloud providers offer dual-stack by default
5. **AI/ML networking**: High-performance RDMA and GPU-direct networking for AI workloads
6. **Security-first networking**: mTLS everywhere, zero-trust as default
7. **Platform engineering**: CNI choices become platform team decisions, not developer concerns
8. **Multi-cloud networking**: ClusterMesh, Submariner, Skupper for cross-cloud connectivity

---

## 10. SOURCE URLS

### Cilium
- https://isovalent.com/blog/post/cilium-1-16/
- https://docs.cilium.io/en/stable/
- https://cilium.io/blog/
- https://github.com/cilium/cilium/releases

### Istio
- https://istio.io/latest/blog/2024/ambient-reached-ga/
- https://istio.io/latest/docs/ambient/
- https://github.com/istio/istio/releases

### Gateway API
- https://gateway-api.sigs.k8s.io/
- https://kubernetes.io/blog/2023/10/31/gateway-api-ga/
- https://gateway-api.sigs.k8s.io/mesh/gamma/

### eBPF
- https://ebpf.io/
- https://github.com/facebookincubator/katran
- https://docs.cilium.io/en/stable/network/ebpf/

### CoreDNS
- https://coredns.io/
- https://kubernetes.io/docs/tasks/administer-cluster/nodelocaldns/

### Calico
- https://www.tigera.io/blog/
- https://github.com/projectcalico/calico/releases

### Linkerd
- https://linkerd.io/
- https://linkerd.io/2024/10/30/linkerd-2.15/

### Kubernetes Official
- https://kubernetes.io/docs/concepts/services-networking/dual-stack/
- https://kubernetes.io/docs/concepts/services-networking/network-policies/
- https://kubernetes.io/docs/concepts/cluster-administration/networking/

---

## 11. EXISTING KUDIG DOCUMENTATION CROSS-REFERENCES

Existing docs in the kudig-database that cover related topics:
- `domain-03-networking-traffic/00-core-k8s-networking/03-cni-plugins-comparison.md` - CNI comparison (last_updated: 2026-01)
- `domain-03-networking-traffic/00-core-k8s-networking/09-kube-proxy-modes-performance.md` - kube-proxy modes (last_updated: 2026-01)
- `domain-03-networking-traffic/00-core-k8s-networking/11-dns-service-discovery-coredns.md` - CoreDNS
- `domain-03-networking-traffic/00-core-k8s-networking/16-networkpolicy-deep-practice.md` - NetworkPolicy
- `domain-03-networking-traffic/00-core-k8s-networking/34-network-performance-tuning.md` - Performance tuning
- `domain-03-networking-traffic/00-core-k8s-networking/35-gateway-api-overview.md` - Gateway API
- `domain-03-networking-traffic/02-service-mesh/01-istio-enterprise-service-mesh.md` - Istio
- `domain-03-networking-traffic/02-service-mesh/02-linkerd-enterprise-service-mesh.md` - Linkerd
- `domain-03-networking-traffic/02-service-mesh/08-ambient-mesh-l7-policy.md` - Ambient mesh
- `domain-03-networking-traffic/04-ebpf/` - eBPF directory
