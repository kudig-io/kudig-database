---
title: CNI 网络插件故障排查
description: '# 03 - CNI 网络插件故障排查 (CNI Network Plugin Troubleshooting)'
summary: '# 03 - CNI 网络插件故障排查 (CNI Network Plugin Troubleshooting)'
category: troubleshooting
tags:
- cni
- calico
- cilium
- flannel
- network
- pod-cidr
- ipam
- kubelet
- prometheus
- istio
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- CNI 不工作
- Pod 没有 IP
- 跨节点网络不通
- 网络插件崩溃
trigger_keywords:
- CNI
- 网络插件故障排查
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- kafka-basics
- tls-basics
k8s_versions:
- 1.25
- 1.26
- 1.27
- 1.28
- 1.29
- 1.3
- 1.31
- 1.32
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
---



# 03 - CNI 网络插件故障排查 (CNI Network Plugin Troubleshooting)
difficulty: "advanced"
related_docs:
  - path: "../domain-03-networking-traffic/02-cni-architecture-fundamentals.md"
    type: "depth"
    desc: "CNI 架构与核心原理"
  - path: "../domain-10-troubleshooting-diagnostics/topic-fta/list/dns-fta.md"
    type: "fta"
    desc: "DNS 故障树"
  - path: "../domain-10-troubleshooting-diagnostics/topic-fta/list/terway-fta.md"
    type: "fta"
    desc: "Terway 故障树"
---

# 03 - CNI 网络插件故障排查 (CNI Network Plugin Troubleshooting)

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25-v1.32 | **最后更新**: 2026-03 | **参考**: [CNI Specification](https://github.com/containernetworking/cni)

---


<!-- chunk: 网络排查知识体系索引 (Network Troubleshooting Knowledge Index) -->
## 网络排查知识体系索引 (Network Troubleshooting Knowledge Index)

> 本索引汇总项目中 **所有网络排查相关文档**，按知识层次分类，方便快速定位。路径均相对于项目根目录。

### A. 网络基础原理

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| 网络协议栈 | [01-network-protocols-stack.md](../domain-03-networking-traffic/01-network-protocols-stack.md) | OSI/TCP-IP模型、数据封装、Linux网络栈、Netfilter/iptables链路、conntrack、veth/bridge、Overlay网络 |
| TCP/UDP 协议深度解析 | [02-tcp-udp-deep-dive.md](../domain-03-networking-traffic/02-tcp-udp-deep-dive.md) | TCP状态机、拥塞控制、BBR算法、UDP特性、性能调优 |
| DNS 原理与配置 | [03-dns-principles-configuration.md](../domain-03-networking-traffic/03-dns-principles-configuration.md) | DNS递归/迭代查询、记录类型、DNSSEC、缓存机制 |
| 负载均衡技术 | [04-load-balancing-technologies.md](../domain-03-networking-traffic/04-load-balancing-technologies.md) | L4/L7负载均衡、LVS/IPVS、Keepalived高可用 |
| 网络安全基础 | [05-network-security-fundamentals.md](../domain-03-networking-traffic/05-network-security-fundamentals.md) | iptables防护、WireGuard VPN、DDoS防御 |
| SDN 与网络虚拟化 | [06-sdn-network-virtualization.md](../domain-03-networking-traffic/06-sdn-network-virtualization.md) | OVS/Linux Bridge、VXLAN/GENEVE/GRE、Overlay网络 |
| Linux 网络配置 | [04-linux-networking-configuration.md](../domain-17-system-foundation/04-linux-networking-configuration.md) | ip命令、NetworkManager、Netplan、内核参数调优 |
| Docker 网络深度解析 | [04-docker-networking-deep-dive.md](../domain-13-container-runtime/04-docker-networking-deep-dive.md) | bridge/host/overlay网络模式、容器网络基础 |

### B. Kubernetes 网络架构与 CNI

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| 网络核心组件 | [01-network-architecture-overview.md](../domain-03-networking-traffic/01-network-architecture-overview.md) | K8s网络模型、Service/DNS/Ingress全景、拓扑感知 |
| CNI 容器网络接口深度解析 | [23-container-network-deep-dive.md](../domain-01-cluster-fundamentals/23-container-network-deep-dive.md) | CNI规范、VXLAN/IPIP/BGP原理、Calico/Cilium架构 |
| CNI 架构与核心原理 | [02-cni-architecture-fundamentals.md](../domain-03-networking-traffic/02-cni-architecture-fundamentals.md) | CNI接口规范、插件链、IPAM、生产配置 |
| CNI 插件深度对比 | [03-cni-plugins-comparison.md](../domain-03-networking-traffic/03-cni-plugins-comparison.md) | Calico/Flannel/Cilium/Terway功能性能对比 |
| Flannel 完整指南 | [04-flannel-complete-guide.md](../domain-03-networking-traffic/04-flannel-complete-guide.md) | VXLAN/Host-GW模式、配置与调优 |
| Terway 高级指南 | [05-terway-advanced-guide.md](../domain-03-networking-traffic/05-terway-advanced-guide.md) | 阿里云ENI/ENIIP模式、VPC网络集成 |
| Terway 实例 CRUD 操作 | [37-terway-resources-crud-operations.md](../domain-03-networking-traffic/37-terway-resources-crud-operations.md) | Terway资源管理、网络策略检查 |
| Cilium CNI 架构与部署 | [03-cilium-cni-architecture.md](../domain-03-networking-traffic/03-cilium-cni-architecture.md) | eBPF数据平面、BPF Maps、安全身份、部署配置 |

### C. Service 与服务发现

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| Service 核心概念与类型 | [06-service-concepts-types.md](../domain-03-networking-traffic/06-service-concepts-types.md) | ClusterIP/NodePort/LoadBalancer/ExternalName |
| Service 实现机制 | [07-service-implementation-details.md](../domain-03-networking-traffic/07-service-implementation-details.md) | iptables/IPVS规则链路、Endpoints控制器 |
| 服务拓扑与端点切片 | [08-service-topology-aware.md](../domain-03-networking-traffic/08-service-topology-aware.md) | EndpointSlice、拓扑感知路由 |
| Kube-proxy 模式与性能 | [09-kube-proxy-modes-performance.md](../domain-03-networking-traffic/09-kube-proxy-modes-performance.md) | iptables/IPVS/nftables模式、性能基准、调优 |
| Service 高级特性 | [10-service-advanced-features.md](../domain-03-networking-traffic/10-service-advanced-features.md) | 会话保持、ExternalTrafficPolicy、多端口Service |

### D. DNS 与 CoreDNS

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| DNS 服务发现与 CoreDNS 调优 | [11-dns-service-discovery-coredns.md](../domain-03-networking-traffic/11-dns-service-discovery-coredns.md) | K8s DNS规范、CoreDNS配置、NodeLocal DNSCache |
| 服务发现与 DNS 配置 | [12-dns-service-discovery.md](../domain-03-networking-traffic/12-dns-service-discovery.md) | DNS记录类型、search域、ndots配置 |
| CoreDNS 架构与核心原理 | [13-coredns-architecture-principles.md](../domain-03-networking-traffic/13-coredns-architecture-principles.md) | 插件链架构、请求处理流程 |
| CoreDNS Corefile 配置详解 | [14-coredns-configuration-corefile.md](../domain-03-networking-traffic/14-coredns-configuration-corefile.md) | Corefile语法、zone配置、上游转发 |
| CoreDNS 插件完整参考 | [15-coredns-plugins-reference.md](../domain-03-networking-traffic/15-coredns-plugins-reference.md) | 内置/外部插件、cache/forward/log等 |

### E. NetworkPolicy 网络策略

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| NetworkPolicy 深度实践指南 | [16-networkpolicy-deep-practice.md](../domain-03-networking-traffic/16-networkpolicy-deep-practice.md) | 零信任策略、多层隔离、微服务策略 |
| NetworkPolicy 高级配置 | [17-network-policy-advanced.md](../domain-03-networking-traffic/17-network-policy-advanced.md) | Egress策略、CIDR规则、高级选择器 |
| 网络加密与 mTLS | [18-network-encryption-mtls.md](../domain-03-networking-traffic/18-network-encryption-mtls.md) | WireGuard加密、mTLS配置 |
| Cilium 网络策略 L3/L4/L7 | [04-cilium-network-policy.md](../domain-03-networking-traffic/04-cilium-network-policy.md) | CiliumNetworkPolicy、FQDN过滤、HTTP/Kafka L7策略 |
| 网络安全策略与零信任架构 | [02-network-security-policies.md](../domain-05-security-compliance/02-network-security-policies.md) | 零信任网络、策略设计模式 |
| 网络安全纵深防御体系 | [18-network-defense-depth.md](../domain-05-security-compliance/18-network-defense-depth.md) | 多层防御、入侵检测、安全审计 |
| NetworkPolicy YAML 参考 | [22-networkpolicy-reference.md](../domain-18-manifests-patterns/22-networkpolicy-reference.md) | 完整YAML模板库 |

### F. Ingress 与流量管理

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| Ingress 基础概念 | [19-ingress-fundamentals.md](../domain-03-networking-traffic/19-ingress-fundamentals.md) | Ingress资源规范、路径匹配、TLS |
| Ingress Controller 深入剖析 | [20-ingress-controller-deep-dive.md](../domain-03-networking-traffic/20-ingress-controller-deep-dive.md) | NGINX/Traefik/HAProxy Controller原理 |
| NGINX Ingress 完整配置指南 | [21-nginx-ingress-complete-guide.md](../domain-03-networking-traffic/21-nginx-ingress-complete-guide.md) | annotations、自定义配置、限流 |
| Ingress TLS 与证书管理 | [22-ingress-tls-certificate.md](../domain-03-networking-traffic/22-ingress-tls-certificate.md) | cert-manager、ACME自动证书 |
| Ingress 高级路由 | [23-ingress-advanced-routing.md](../domain-03-networking-traffic/23-ingress-advanced-routing.md) | 金丝雀发布、蓝绿部署、A/B测试 |
| Ingress 安全加固 | [24-ingress-security-hardening.md](../domain-03-networking-traffic/24-ingress-security-hardening.md) | WAF、速率限制、IP白名单 |
| Ingress 监控与故障排查 | [25-ingress-monitoring-troubleshooting.md](../domain-03-networking-traffic/25-ingress-monitoring-troubleshooting.md) | Prometheus指标、日志分析、常见问题 |
| Ingress 生产最佳实践 | [26-ingress-production-best-practices.md](../domain-03-networking-traffic/26-ingress-production-best-practices.md) | 高可用部署、Kustomize环境管理 |
| Gateway API 配置 | [35-gateway-api-overview.md](../domain-03-networking-traffic/35-gateway-api-overview.md) | Gateway/HTTPRoute/GRPCRoute |
| Ingress 和 API Gateway 对比 | [36-api-gateway-patterns.md](../domain-03-networking-traffic/36-api-gateway-patterns.md) | Ingress vs API Gateway选型 |

### G. Egress、Service Mesh 与多集群

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| Egress 流量管理 | [29-egress-traffic-management.md](../domain-03-networking-traffic/29-egress-traffic-management.md) | Egress Gateway、SNAT、出站策略 |
| Service Mesh 深度解析 | [30-service-mesh-deep-dive.md](../domain-03-networking-traffic/30-service-mesh-deep-dive.md) | Sidecar注入、iptables劫持、Envoy配置 |
| Cilium Service Mesh | [05-cilium-service-mesh.md](../domain-03-networking-traffic/05-cilium-service-mesh.md) | eBPF替代Sidecar、mTLS/SPIFFE、Gateway API |
| 多集群网络联邦 | [31-multi-cluster-federation.md](../domain-03-networking-traffic/31-multi-cluster-federation.md) | Karmada/Submariner/ClusterMesh |
| 多集群网络互联 | [32-multi-cluster-networking.md](../domain-03-networking-traffic/32-multi-cluster-networking.md) | Submariner/Cilium ClusterMesh/Istio多集群 |

### H. 网络故障排查 (核心排查文档)

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| **CNI 网络插件故障排查** | **本文** | CNI诊断流程、Pod网络不通、DNS解析、跨节点通信 |
| 网络连通性故障排查 | [25-network-connectivity-troubleshooting.md](25-network-connectivity-troubleshooting.md) | Pod-to-Pod/Pod-to-Node/Node-to-Node场景排查、数据路径分析、iptables TRACE |
| DNS 故障排查 | [26-dns-troubleshooting.md](26-dns-troubleshooting.md) | CoreDNS状态检查、解析失败、DNS延迟、上游DNS |
| Ingress 故障排查 | [15-ingress-troubleshooting.md](15-ingress-troubleshooting.md) | Controller状态、502/504错误、TLS问题 |
| NetworkPolicy 故障排查 | [16-networkpolicy-troubleshooting.md](16-networkpolicy-troubleshooting.md) | 策略不生效、CNI兼容性、规则调试 |
| Service 全面故障排查 | [10-service-comprehensive-troubleshooting.md](10-service-comprehensive-troubleshooting.md) | Endpoints异常、LoadBalancer挂起、端口映射 |
| CNI 故障排查与优化 | [27-cni-troubleshooting-optimization.md](../domain-03-networking-traffic/27-cni-troubleshooting-optimization.md) | IP池管理、veth诊断、多跳tcpdump、conntrack |
| CoreDNS 故障排查与优化 | [28-coredns-troubleshooting-optimization.md](../domain-03-networking-traffic/28-coredns-troubleshooting-optimization.md) | 解析慢、OOM、插件异常、性能优化 |
| 网络故障诊断与链路排查 | [33-network-troubleshooting.md](../domain-03-networking-traffic/33-network-troubleshooting.md) | 数据路径总览、场景速查、MTU诊断、内核参数 |
| 网络性能调优 | [34-network-performance-tuning.md](../domain-03-networking-traffic/34-network-performance-tuning.md) | CNI性能配置、网卡队列、IRQ亲和性 |

### I. 结构化深度排查指南

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| CNI 深度排查 | [01-cni-troubleshooting.md](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md) | 系统化排查方法论、conntrack/iptables TRACE/eBPF诊断、生产案例 |
| CoreDNS/DNS 排查指南 | [02-dns-troubleshooting.md](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md) | DNS全链路排查、NodeLocal DNSCache、CoreDNS调优 |
| Service 与 Ingress 排查指南 | [03-service-ingress-troubleshooting.md](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md) | kube-proxy规则链路、Service DNAT、Ingress Controller诊断 |
| NetworkPolicy 深度排查 | [04-networkpolicy-troubleshooting.md](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting.md) | 零信任治理、Calico/Cilium策略追踪、iptables规则映射 |
| Service Mesh (Istio) 排查 | [05-service-mesh-istio-troubleshooting.md](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md) | xDS推送、Envoy配置、mTLS证书、Ambient Mesh |
| Gateway API 排查 | [06-gateway-api-troubleshooting.md](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/06-gateway-api-troubleshooting.md) | Gateway状态机、路由优先级、BackendTLSPolicy |

### J. eBPF 与网络可观测性

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| Hubble 网络可观测性 | [07-hubble-network-observability.md](../domain-03-networking-traffic/07-hubble-network-observability.md) | Flow观测、Hubble UI、网络拓扑可视化 |
| 网络性能优化（生产运维） | [20-network-performance-optimization.md](../domain-06-observability/20-network-performance-optimization.md) | Calico/Cilium生产调优、eBPF模式、Service Mesh优化 |

### K. 参考资料与事件

| 文档 | 路径 | 核心内容 |
|------|------|----------|
| Service 与网络事件 | [10-service-networking-events.md](../domain-17-system-foundation/10-service-networking-events.md) | K8s网络相关Event解读 |
| NetworkPolicy 异常 FTA 树 | [networkpolicy-fta.md](../domain-10-troubleshooting-diagnostics/topic-fta/list/networkpolicy-fta.md) | 故障树分析 |
| 网络硬件技术 | [08-network-hardware-technology.md](../domain-17-system-foundation/08-network-hardware-technology.md) | 高速网卡、RDMA、智能网卡 |
| 网络硬件故障排查 | [13-network-hardware-troubleshooting.md](../domain-17-system-foundation/13-network-hardware-troubleshooting.md) | 网卡诊断、光模块问题、ethtool |

### 排查场景快速导航

| 排查场景 | 首选文档 | 深度参考 |
|---------|---------|----------|
| **Pod-to-Pod 不通（同节点）** | [33-网络诊断](../domain-03-networking-traffic/33-network-troubleshooting.md) | [25-网络连通性](25-network-connectivity-troubleshooting.md)、[CNI深度排查](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md) |
| **Pod-to-Pod 不通（跨节点）** | [25-网络连通性](25-network-connectivity-troubleshooting.md) | [27-CNI排查优化](../domain-03-networking-traffic/27-cni-troubleshooting-optimization.md)、[CNI深度排查](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md) |
| **Pod-to-Node 不通** | [25-网络连通性](25-network-connectivity-troubleshooting.md) | 本文、[网络协议栈](../domain-03-networking-traffic/01-network-protocols-stack.md) |
| **Node-to-Node 不通** | [25-网络连通性](25-network-connectivity-troubleshooting.md) | [33-网络诊断](../domain-03-networking-traffic/33-network-troubleshooting.md)、[Linux网络配置](../domain-17-system-foundation/04-linux-networking-configuration.md) |
| **Pod-to-Service 不通** | [10-Service排查](10-service-comprehensive-troubleshooting.md) | [Service与Ingress排查](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md) |
| **DNS 解析失败** | [26-DNS排查](26-dns-troubleshooting.md) | [DNS深度排查](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md)、[CoreDNS优化](../domain-03-networking-traffic/28-coredns-troubleshooting-optimization.md) |
| **Ingress 访问异常** | [15-Ingress排查](15-ingress-troubleshooting.md) | [Service与Ingress排查](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md)、[Ingress监控](../domain-03-networking-traffic/25-ingress-monitoring-troubleshooting.md) |
| **NetworkPolicy 不生效** | [16-NP排查](16-networkpolicy-troubleshooting.md) | [NP深度排查](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/04-networkpolicy-troubleshooting.md) |
| **CNI 插件问题** | 本文 | [27-CNI排查优化](../domain-03-networking-traffic/27-cni-troubleshooting-optimization.md)、[CNI深度排查](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md) |
| **Service Mesh 问题** | [Istio排查](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md) | [Service Mesh解析](../domain-03-networking-traffic/30-service-mesh-deep-dive.md)、[Cilium Mesh](../domain-03-networking-traffic/05-cilium-service-mesh.md) |
| **Gateway API 问题** | [Gateway API排查](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/06-gateway-api-troubleshooting.md) | [Gateway API配置](../domain-03-networking-traffic/35-gateway-api-overview.md) |
| **网络性能问题** | [34-网络性能调优](../domain-03-networking-traffic/34-network-performance-tuning.md) | [生产网络优化](../domain-06-observability/20-network-performance-optimization.md) |
| **conntrack 表满** | [27-CNI排查优化](../domain-03-networking-traffic/27-cni-troubleshooting-optimization.md) | [CNI深度排查](../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md)、[网络协议栈](../domain-03-networking-traffic/01-network-protocols-stack.md) |
| **MTU/分片问题** | [33-网络诊断](../domain-03-networking-traffic/33-network-troubleshooting.md) | [CNI容器网络深度](../domain-01-cluster-fundamentals/23-container-network-deep-dive.md) |

---
<!-- chunk: 1. CNI 网络故障诊断总览 (CNI Diagnosis Overview) -->
## 1. CNI 网络故障诊断总览 (CNI Diagnosis Overview)

### 1.1 常见网络问题类型

| 问题类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **Pod网络不通** | Pod无法ping通其他Pod/Service | 应用间通信中断 | P0 - 紧急 |
| **DNS解析失败** | nslookup失败、域名无法解析 | 服务发现异常 | P1 - 高 |
| **跨节点通信失败** | 同集群不同节点Pod无法互访 | 集群网络分割 | P1 - 高 |
| **网络策略失效** | NetworkPolicy规则不生效 | 安全边界破坏 | P2 - 中 |
| **IP地址耗尽** | CNI IP池分配完、Pod卡在ContainerCreating | 新Pod无法创建 | P1 - 高 |
| **MTU问题** | 数据包分片、连接超时 | 网络性能下降 | P2 - 中 |

### 1.2 CNI 网络架构概览

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      CNI 网络故障诊断架构                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                          应用Pod层面                                   │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │   Pod A     │  │   Pod B     │  │   Pod C     │  │   Pod D     │  │  │
│  │  │ 10.244.1.2  │  │ 10.244.2.3  │  │ 10.244.1.4  │  │ 10.244.3.5  │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │  │  │  │                                     │
│        ┌─────────────────────┘  │  │  └─────────────────────┐             │
│        │                        │  │                        │             │
│        ▼                        ▼  ▼                        ▼             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    CNI插件实现层 (Calico/Flannel/Cilium等)           │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │   Calico    │  │  Flannel    │  │   Cilium    │  │    Terway   │  │  │
│  │  │ (BGP/IPAM)  │  │ (VXLAN)     │  │ (eBPF)      │  │ (阿里云CNI) │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    网络虚拟化层                                        │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │   veth pair │  │   VXLAN     │  │   eBPF      │  │   ENI       │  │  │
│  │  │ (容器接口)   │  │ (隧道封装)   │  │ (内核旁路)   │  │ (弹性网卡)   │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    主机网络栈                                          │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │   iptables  │  │   ipvs      │  │   routing   │  │   policy    │  │  │
│  │  │ (NAT规则)   │  │ (负载均衡)   │  │ (路由表)    │  │ (路由策略)   │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    物理网络层                                          │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                   │  │
│  │  │   交换机     │  │   路由器     │  │   防火墙     │                   │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                   │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 2. Pod 网络不通故障排查 (Pod Network Connectivity Issues) -->
## 2. Pod 网络不通故障排查 (Pod Network Connectivity Issues)

### 2.1 故障诊断流程

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      Pod 网络不通诊断流程                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Pod A 无法访问 Pod B                                                      │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 1: 检查Pod状态和网络配置                         │                 │
│   │ kubectl get pod -o wide                              │                 │
│   │ kubectl describe pod <pod-name>                      │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── Pod未Running ──▶ 转Pod故障排查                              │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 2: 检查Pod网络接口                               │                 │
│   │ kubectl exec -it <pod> -- ip addr show               │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── 无网络接口 ──▶ CNI插件问题                                  │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 3: 检查节点CNI组件                               │                 │
│   │ kubectl get pods -n kube-system -l k8s-app=calico-node│                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── CNI Pod异常 ──▶ 检查CNI Pod日志                             │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 4: 检查主机网络配置                              │                 │
│   │ ip route show                                        │                 │
│   │ iptables -t nat -L                                   │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── 路由缺失 ──▶ 检查CNI路由配置                                │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 5: 跨节点连通性测试                              │                 │
│   │ ping <other-node-pod-ip>                             │                 │
│   └──────────────────────────────────────────────────────┘                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 详细诊断命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# ========== 1. Pod网络状态检查 ==========

# 检查Pod网络状态
kubectl get pods -o wide --all-namespaces | grep -v "Running"

# 查看Pod网络接口
kubectl exec -it <pod-name> -n <namespace> -- ip addr show

# 检查Pod路由表
kubectl exec -it <pod-name> -n <namespace> -- ip route show

# ========== 2. CNI插件状态检查 ==========

# 检查CNI Pod状态 (以Calico为例)
kubectl get pods -n kube-system -l k8s-app=calico-node -o wide

# 检查CNI配置
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/*.conflist

# 检查CNI二进制文件
ls -la /opt/cni/bin/

# ========== 3. 节点网络检查 ==========

# 检查节点路由表
ip route show

# 检查ARP表
arp -a

# 检查网络接口状态
ip link show

# 检查iptables规则
iptables -t nat -L -n -v | head -20

# ========== 4. 跨节点连通性测试 ==========

# 测试Pod到Pod连通性
kubectl run debug-pod --rm -it --image=busybox -- sh
# 在Pod内执行: ping <other-pod-ip>

# 测试节点间连通性
ping <other-node-ip>

# 检查UDP端口连通性 (VXLAN常用4789端口)
nc -uzv <other-node-ip> 4789

# ========== 5. CNI日志分析 ==========

# 查看CNI Pod日志
kubectl logs -n kube-system -l k8s-app=calico-node --tail=100

# 查看kubelet CNI相关日志
journalctl -u kubelet | grep -i cni

# 查看CNI调用错误
grep "CNI failed" /var/log/messages
```

### 2.3 veth pair 深度诊断

每个 Pod 通过 veth pair 连接到宿主机网络命名空间。当 Pod 网络异常时，需要逐层检查 veth pair 状态。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# ========== 1. 定位 Pod 对应的宿主机 veth 接口 ==========

# 方法一：通过 Pod 内 eth0 的 ifindex 找到宿主机侧 veth
POD_IFINDEX=$(kubectl exec -it <pod-name> -n <namespace> -- cat /sys/class/net/eth0/iflink)
ip link show | grep "^${POD_IFINDEX}:"

# 方法二：通过 crictl 获取容器 PID，再用 nsenter 查看
CONTAINER_ID=$(crictl ps --name <container-name> -q)
PID=$(crictl inspect $CONTAINER_ID | jq '.info.pid')
nsenter -t $PID -n ip link show eth0
# 然后在宿主机上找到对应的 veth
ip link show | grep "if${POD_IFINDEX}"

# ========== 2. 检查 veth pair 状态 ==========

# 确认 veth 接口 UP 且无错误
ip -s link show <veth-name>
# 关注: RX/TX errors, dropped, overrun

# 使用 ethtool 查看详细丢包统计
ethtool -S <veth-name>
# 关键字段: tx_dropped, rx_dropped, tx_errors

# ========== 3. 检查 veth 连接的 bridge/路由 ==========

# Flannel 模式：检查 cni0 bridge
bridge link show | grep <veth-name>
bridge fdb show br cni0

# Calico 路由模式：检查路由表中的 veth 条目
ip route show | grep <veth-name>
# 应有类似: 10.244.1.5 dev caliXXXX scope link
```

### 2.4 iptables 链路追踪技术

当 Pod 网络不通但 veth pair 状态正常时，问题可能出在 iptables 规则上。使用 TRACE 功能可以逐条追踪数据包经过的规则链。

```bash
# ========== 1. 启用 iptables TRACE ==========

# 加载 nf_log_ipv4 模块（如需要）
modprobe nf_log_ipv4

# 对特定源/目标 IP 的数据包启用追踪
iptables -t raw -A PREROUTING -s <src-pod-ip> -d <dst-pod-ip> -j TRACE
iptables -t raw -A OUTPUT -s <src-pod-ip> -d <dst-pod-ip> -j TRACE

# ========== 2. 查看 TRACE 输出 ==========

# TRACE 日志会输出到内核日志
dmesg -w | grep TRACE
# 或
tail -f /var/log/kern.log | grep TRACE

# 输出格式示例:
# TRACE: raw:PREROUTING:policy:2 IN=cali12345 OUT= SRC=10.244.1.5 DST=10.244.2.8 ...
# TRACE: nat:PREROUTING:rule:1 IN=cali12345 ...
# TRACE: filter:FORWARD:rule:3 IN=cali12345 OUT=cali67890 ...

# 分析要点:
# - 观察数据包经过了哪些链（PREROUTING -> FORWARD -> POSTROUTING）
# - 找到最后一条 TRACE 记录 → 该规则可能是 DROP/REJECT 的位置
# - 注意 IN/OUT 接口变化，确认数据包是否正确路由

# ========== 3. Kubernetes 关键 iptables 链 ==========

# kube-proxy 相关链
iptables -t nat -L KUBE-SERVICES -n -v | head -30    # Service ClusterIP 入口
iptables -t nat -L KUBE-NODEPORTS -n -v              # NodePort 入口
iptables -t nat -L KUBE-SVC-XXXXXX -n -v             # 特定 Service 的后端选择
iptables -t nat -L KUBE-SEP-XXXXXX -n -v             # 特定 Endpoint 的 DNAT

# CNI 相关链（Calico 示例）
iptables -t filter -L cali-FORWARD -n -v              # Calico FORWARD 链
iptables -t filter -L cali-fw-caliXXXX -n -v          # 特定 Pod 的 from-workload 链
iptables -t filter -L cali-tw-caliXXXX -n -v          # 特定 Pod 的 to-workload 链

# ========== 4. 清理 TRACE 规则（调试完成后必须清理）==========

iptables -t raw -D PREROUTING -s <src-pod-ip> -d <dst-pod-ip> -j TRACE
iptables -t raw -D OUTPUT -s <src-pod-ip> -d <dst-pod-ip> -j TRACE
```

### 2.5 conntrack 连接跟踪诊断

Linux 内核通过 conntrack（连接跟踪）实现有状态防火墙和 NAT。Kubernetes 的 Service DNAT/SNAT 完全依赖 conntrack，表满或条目异常会导致严重的网络问题。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
# ========== 1. conntrack 基础检查 ==========

# 查看当前连接跟踪表条目数
sysctl net.netfilter.nf_conntrack_count

# 查看最大连接数限制
sysctl net.netfilter.nf_conntrack_max

# 查看使用率
echo "$(sysctl -n net.netfilter.nf_conntrack_count) / $(sysctl -n net.netfilter.nf_conntrack_max)" | bc -l

# ========== 2. 查看特定 Pod 的连接状态 ==========

# 按源 IP 过滤（从 Pod 发出的连接）
conntrack -L -s <pod-ip> 2>/dev/null | head -20

# 按目标 IP 过滤（到 Pod 的连接）
conntrack -L -d <pod-ip> 2>/dev/null | head -20

# 实时监控新建/销毁事件
conntrack -E

# ========== 3. conntrack 表满诊断 ==========

# 检查内核日志中的 conntrack 报错
dmesg | grep "nf_conntrack: table full"

# 查看 conntrack 统计信息
conntrack -S
# 关注: insert_failed, drop, early_drop
# insert_failed > 0 表示表满导致新连接被丢弃

# ========== 4. conntrack 调优 ==========

# 临时调大（生效立即）
sysctl -w net.netfilter.nf_conntrack_max=262144
sysctl -w net.netfilter.nf_conntrack_buckets=65536

# 永久配置
cat >> /etc/sysctl.d/99-conntrack.conf << EOF
net.netfilter.nf_conntrack_max = 262144
net.netfilter.nf_conntrack_buckets = 65536
net.netfilter.nf_conntrack_tcp_timeout_established = 86400
net.netfilter.nf_conntrack_tcp_timeout_time_wait = 30
EOF
sysctl -p /etc/sysctl.d/99-conntrack.conf
```

### 2.6 常见错误及解决方案

| 错误信息 | 可能原因 | 解决方案 |
|---------|---------|---------|
| `failed to set up sandbox container` | CNI插件未就绪 | 检查CNI Pod状态，重启CNI组件 |
| `no IP addresses available` | IP地址池耗尽 | 扩展IP池或清理未使用IP |
| `network plugin is not ready` | CNI初始化失败 | 检查CNI配置文件和权限 |
| `dial tcp: lookup xxx on 10.96.0.10:53` | DNS解析失败 | 检查CoreDNS和网络策略 |
| `connection refused` | 网络策略阻止 | 检查NetworkPolicy配置 |
| `nf_conntrack: table full, dropping packet` | conntrack 表满 | 调大 nf_conntrack_max |
| `RTNETLINK answers: File exists` | veth/路由冲突 | 清理残留网络接口，重启 CNI |

---

<!-- chunk: 3. DNS 解析故障排查 (DNS Resolution Issues) -->
## 3. DNS 解析故障排查 (DNS Resolution Issues)

### 3.1 DNS 故障诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# ========== 1. CoreDNS状态检查 ==========

# 检查CoreDNS Pod状态
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# 检查CoreDNS服务
kubectl get svc -n kube-system kube-dns -o wide

# 检查CoreDNS配置
kubectl get configmap coredns -n kube-system -o yaml

# ========== 2. DNS解析测试 ==========

# 从Pod内测试DNS解析
kubectl run dns-test --rm -it --image=busybox -- sh
# 在Pod内执行:
# nslookup kubernetes.default
# nslookup google.com

# 测试集群内Service解析
kubectl exec -it <pod> -- nslookup <service>.<namespace>.svc.cluster.local

# ========== 3. DNS配置检查 ==========

# 检查Pod的resolv.conf
kubectl exec -it <pod> -- cat /etc/resolv.conf

# 检查kubelet DNS配置
cat /var/lib/kubelet/config.yaml | grep -A5 dns

# 检查集群DNS配置
kubectl cluster-info | grep dns
```

### 3.2 常见DNS问题

| 问题类型 | 症状 | 解决方案 |
|---------|------|---------|
| **CoreDNS Pod CrashLoopBackOff** | DNS服务不可用 | 检查CoreDNS日志，调整资源配置 |
| **解析超时** | DNS查询慢 | 检查上游DNS服务器，优化配置 |
| **解析失败** | 域名无法解析 | 检查NetworkPolicy，确认53端口开放 |
| **解析不一致** | 不同Pod解析结果不同 | 检查CoreDNS副本数和服务发现 |

---

<!-- chunk: 4. 跨节点网络通信问题 (Cross-Node Network Issues) -->
## 4. 跨节点网络通信问题 (Cross-Node Network Issues)

### 4.1 网络互通性检查

```bash
# ========== 1. 隧道网络检查 (VXLAN/Overlay) ==========

# 检查VXLAN接口
ip link show type vxlan

# 检查VXLAN转发数据库
bridge fdb show | grep vxlan

# 检查UDP端口连通性
nc -uzv <node-ip> 4789  # VXLAN端口

# ========== 2. BGP路由检查 (Calico等) ==========

# 检查BGP邻居状态
calicoctl node status

# 检查路由表
ip route show | grep bird

# 查看BGP路由
calicoctl get workloadEndpoint -o wide

# ========== 3. 网络策略影响 ==========

# 检查NetworkPolicy
kubectl get networkpolicy --all-namespaces

# 检查特定命名空间的策略
kubectl get networkpolicy -n <namespace> -o yaml

# ========== 4. MTU问题排查 ==========

# 检查接口MTU
ip link show

# 测试不同大小的数据包
ping -M do -s 1400 <destination-ip>  # 不分片测试
ping -M do -s 1500 <destination-ip>  # 可能分片

# 检查iptables规则中的MTU处理
iptables -t mangle -L -n -v
```

---

<!-- chunk: 5. CNI IP地址管理问题 (IPAM Issues) -->
## 5. CNI IP地址管理问题 (IPAM Issues)

### 5.1 IP地址耗尽问题

```bash
# ========== 1. IP使用情况检查 ==========

# 检查节点IP分配情况
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.pods}{"\n"}{end}'

# 检查Pod CIDR分配
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'

# ========== 2. CNI IP池状态 ==========

# Calico IP池检查
calicoctl get ippool -o wide

# 检查已分配IP
calicoctl ipam show

# 检查IP冲突
calicoctl ipam check

# ========== 3. IP回收操作 ==========

# 清理未使用的IP (谨慎操作)
calicoctl ipam release --ip=<ip-address>

# 扩展IP池
calicoctl create -f - <<EOF
apiVersion: projectcalico.org/v3
kind: IPPool
metadata:
  name: new-pool
spec:
  cidr: 10.245.0.0/16
  blockSize: 26
  natOutgoing: true
EOF
```

### 5.2 IPAM配置优化

```yaml
# Calico IP池配置示例
apiVersion: projectcalico.org/v3
kind: IPPool
metadata:
  name: default-ipv4-ippool
spec:
  cidr: 10.244.0.0/16
  blockSize: 26  # 每个块64个IP，适合大多数场景
  ipipMode: Never  # 根据网络环境选择
  natOutgoing: true
  disabled: false
```

---

<!-- chunk: 6. 不同CNI插件特有问题 (CNI-Specific Issues) -->
## 6. 不同CNI插件特有问题 (CNI-Specific Issues)

### 6.1 Calico 故障排查

```bash
# ========== Calico专用诊断 ==========

# 安装calicoctl
curl -O -L https://github.com/projectcalico/calicoctl/releases/download/v3.26.0/calicoctl
chmod +x calicoctl

# 检查Calico节点状态
./calicoctl node status

# 检查BGP配置
./calicoctl get bgpconfig -o yaml

# 检查 Felix配置
./calicoctl get felixconfig -o yaml

# 查看路由信息
./calicoctl get workloadEndpoint -o wide

# 检查网络策略
./calicoctl get networkPolicy -o wide
```

### 6.2 Flannel 故障排查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# ========== Flannel专用诊断 ==========

# 检查Flannel Pod日志
kubectl logs -n kube-system -l app=flannel

# 检查Flannel子网配置
kubectl exec -n kube-system -l app=flannel -- cat /run/flannel/subnet.env

# 检查VXLAN接口
ip link show flannel.1

# 检查Flannel网络配置
cat /etc/kube-flannel/net-conf.json
```

---

<!-- chunk: 7. 生产环境应急处理 (Production Emergency Response) -->
## 7. 生产环境应急处理 (Production Emergency Response)

### 7.1 网络问题紧急诊断脚本

```bash
#!/bin/bash
# cni-network-emergency-check.sh

echo "=== CNI 网络紧急诊断 ==="
echo "时间: $(date)"
echo ""

# 1. 检查CNI组件状态
echo "1. CNI组件状态:"
kubectl get pods -n kube-system -l k8s-app=calico-node 2>/dev/null || \
kubectl get pods -n kube-system -l app=flannel 2>/dev/null || \
echo "❌ 未找到CNI组件"

# 2. 检查Pod网络状态
echo -e "\n2. 异常Pod统计:"
kubectl get pods --all-namespaces --field-selector=status.phase!=Running | wc -l

# 3. 检查CoreDNS
echo -e "\n3. CoreDNS状态:"
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# 4. 网络连通性测试
echo -e "\n4. 节点间连通性测试:"
for node in $(kubectl get nodes -o jsonpath='{.items[*].status.addresses[?(@.type=="InternalIP")].address}'); do
  echo -n "$node: "
  timeout 3 ping -c 1 $node >/dev/null 2>&1 && echo "✅" || echo "❌"
done

# 5. 检查IP使用情况
echo -e "\n5. IP使用概况:"
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.pods}{"\n"}{end}' | head -10

echo -e "\n=== 诊断完成 ==="
```

### 7.2 故障处理优先级

| 问题类型 | 响应时间 | 处理步骤 |
|---------|---------|---------|
| **全集群网络中断** | 15分钟内 | 1. 确认CNI组件状态 2. 检查节点网络 3. 应急重启 |
| **DNS解析失败** | 30分钟内 | 1. 检查CoreDNS 2. 验证网络策略 3. 重启CoreDNS |
| **跨节点通信失败** | 1小时内 | 1. 检查隧道/BGP 2. 验证路由配置 3. 调整网络设置 |
| **IP地址耗尽** | 2小时内 | 1. 扩展IP池 2. 清理僵尸Pod 3. 优化IP回收 |

---

<!-- chunk: 8. 预防措施与最佳实践 (Prevention & Best Practices) -->
## 8. 预防措施与最佳实践 (Prevention & Best Practices)

### 8.1 监控告警配置

```yaml
# Prometheus网络监控告警
groups:
- name: network.rules
  rules:
  # CNI组件不可用
  - alert: CNIComponentDown
    expr: up{job=~"calico|flannel|cilium"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "CNI组件 {{ $labels.job }} 不可用"
  
  # CoreDNS异常
  - alert: CoreDNSHealthError
    expr: coredns_health_request_duration_seconds_count{status!="success"} > 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS健康检查失败"
  
  # 网络延迟高
  - alert: NetworkLatencyHigh
    expr: histogram_quantile(0.99, rate(container_network_latency_seconds_bucket[5m])) > 0.1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "网络延迟超过100ms"
```

### 8.2 运维检查清单

- [ ] 定期检查CNI组件健康状态
- [ ] 监控IP地址池使用率（阈值80%）
- [ ] 验证跨节点网络连通性
- [ ] 检查CoreDNS解析成功率
- [ ] 审查NetworkPolicy配置变更
- [ ] 测试网络故障恢复流程
- [ ] 保持CNI插件版本更新

---

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-10-troubleshooting-diagnostics MOC
- [[domain-10-troubleshooting-diagnostics/README.md|Domain-12 故障排查 (Troubleshooting)]]
- Domain-12 故障排查 — 开源项目索引
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/01-control-plane-apiserver-troubleshooting.md|API Server 故障排查]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/02-control-plane-etcd-troubleshooting.md|etcd 故障排查]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/04-storage-csi-troubleshooting.md|CSI 存储驱动故障排查]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/05-pod-pending-diagnosis.md|Pod Pending 状态深度诊断]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/06-node-notready-diagnosis.md|Node NotReady 状态深度诊断]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/07-oom-memory-diagnosis.md|OOM 和内存问题诊断]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/08-pod-comprehensive-troubleshooting.md|Pod 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/09-node-comprehensive-troubleshooting.md|Node 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/10-service-comprehensive-troubleshooting.md|Service 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/01-control-plane-apiserver-troubleshooting.md|01-control-plane-apiserver-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/02-control-plane-etcd-troubleshooting.md|02-control-plane-etcd-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/04-storage-csi-troubleshooting.md|04-storage-csi-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/05-pod-pending-diagnosis.md|05-pod-pending-diagnosis]]

## Related

- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]

```