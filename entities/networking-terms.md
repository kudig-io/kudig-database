---
title: K8s 网络术语参考
description: '# K8s 网络术语参考'
summary: '# K8s 网络术语参考'
category: references
tags:
- k8s
- dictionary
- networking
- kubelet
- istio
- envoy
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
estimated_read_time: 15min
intent_queries:
- K8s 网络术语参考 是什么
- 如何 K8s 网络术语参考
trigger_keywords:
- K8s
- 网络术语参考
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
---



# K8s 网络术语参考

本页汇总了 **网络** 领域的 17 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[entities/k8s-networking-domain-guide.md|k8s-networking-domain-guide]] | [[entities/k8s-networking-ecosystem.md|k8s-networking-ecosystem]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **多集群网络互联（Cluster Mesh）** | Cluster Mesh | 随着企业 Kubernetes 集群数量从单个增长到数十甚至上百个，**多集群网络互联（Cluster Mesh）** 成为构建统一服务网格和跨集群负载均... |
| **集群网络（Cluster Networking）** | Cluster Networking | 网络是 Kubernetes 的核心组成部分，理解其预期工作方式对于集群管理员至关重要 |
| **DNS for Services and Pods** | Dns For Services And Pods | Kubernetes 通过集群 DNS（通常由 CoreDNS 实现）为 Service 和 Pod 创建 DNS 记录，使工作负载能够通过一致的域名而非... |
| **eBPF 与 Cilium 网络** | Ebpf And Cilium Networking | **eBPF（Extended Berkeley Packet Filter）** 是一项革命性的 Linux 内核技术，允许在不修改内核源码或加载内核模... |
| **EndpointSlices** | Endpointslices | EndpointSlice 是 Kubernetes 自 v1 |
| **Gateway API** | Gateway Api | Gateway API 是 Kubernetes 中用于暴露网络服务的一组扩展 API（以 CustomResourceDefinition 实现），旨在... |
| **Ingress Controllers** | Ingress Controllers | Ingress 资源本身只是声明式的路由配置，**必须有 Ingress Controller 在集群中运行**才能将其转化为实际的流量转发规则 |
| **Ingress** | Ingress | Ingress 是 Kubernetes 中用于管理集群外部 HTTP/HTTPS 访问到内部 Service 的 API 对象 |
| **IPv4/IPv6 dual-stack** | Ipv4 Ipv6 Dual Stack | Kubernetes 支持为 Pod 和 Service 同时分配 IPv4 与 IPv6 地址，实现双栈（Dual-Stack）网络 |
| **Network Policies** | Network Policies | NetworkPolicy 是 Kubernetes 中用于在 OSI 第 3/4 层（IP 地址和端口级别）控制流量的资源对象 |
| **Networking on Windows** | Networking On Windows | Kubernetes 支持在 Windows 节点上运行工作负载，并允许与 Linux 节点混合部署在同一个集群中 |
| **Service ClusterIP allocation** | Service Clusterip Allocation | 在 Kubernetes 中，`ClusterIP` 类型的 Service 会被分配一个集群范围内的虚拟 IP 地址，客户端通过该 IP 访问 Serv... |
| **Service Internal Traffic Policy** | Service Internal Traffic Policy | Service Internal Traffic Policy（Service 内部流量策略）用于控制集群内部发起的流量如何被路由到后端端点 |
| **服务网格（Service Mesh）** | Service Mesh | **服务网格（Service Mesh）** 是一种专门处理服务间通信的基础设施层，通过透明代理（Sidecar 或 eBPF）为微服务提供统一的流量管理... |
| **Service** | Service | Service 是 Kubernetes 中用于将运行在一组 Pod 上的网络应用暴露给集群内外的核心抽象对象 |
| **电信云与 5G 多接入边缘计算（MEC）** | Telco Cloud And 5G Mec | **电信云（Telco Cloud）** 和 **5G 多接入边缘计算（MEC, Multi-access Edge Computing）** 是通信行业... |
| **Topology Aware Routing** | Topology Aware Routing | 拓扑感知路由（Topology Aware Routing，旧称 Topology Aware Hints）是一种帮助将网络流量保留在其发起可用区（zon... |

---

### 多集群网络互联（Cluster Mesh）

随着企业 Kubernetes 集群数量从单个增长到数十甚至上百个，**多集群网络互联（Cluster Mesh）** 成为构建统一服务网格和跨集群负载均衡的关键技术。Cluster Mesh 允许不同地域、不同云厂商的 Kubernetes 集群中的 Pod 像在同一个网络中一样相互通信，实现真正的**全局服务发现**和**跨集群流量管理**。2026 年的主流实现包括 **Cilium Cluster Mesh** 和 **Istio Multi-Cluster**。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/cluster-mesh.md`）*

---

### 集群网络（Cluster Networking）

网络是 Kubernetes 的核心组成部分，理解其预期工作方式对于集群管理员至关重要。Kubernetes 需要解决四种不同的网络通信问题：容器到容器通信、Pod 到 Pod 通信、Pod 到 Service 通信、外部到 Service 通信。本文档重点讨论 Pod 到 Pod 的通信以及集群网络的实现方式。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/cluster-networking.md`）*

---

### DNS for Services and Pods

Kubernetes 通过集群 DNS（通常由 CoreDNS 实现）为 Service 和 Pod 创建 DNS 记录，使工作负载能够通过一致的域名而非易变的 IP 地址进行相互发现。kubelet 会为每个 Pod 配置 DNS 解析设置（`/etc/resolv.conf`），默认搜索域包括 Pod 所在命名空间和集群域。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/dns-for-services-and-pods.md`）*

---

### eBPF 与 Cilium 网络

**eBPF（Extended Berkeley Packet Filter）** 是一项革命性的 Linux 内核技术，允许在不修改内核源码或加载内核模块的情况下，在内核中安全地运行沙箱程序。**Cilium** 是基于 eBPF 的 Kubernetes 网络、安全和可观测性解决方案，正在逐步取代传统的 iptables 和 OVS 方案，成为 2026 年云原生网络的事实标准。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/ebpf-and-cilium-networking.md`）*

---

### EndpointSlices

EndpointSlice 是 Kubernetes 自 v1.21 起稳定的 API，用于跟踪 Service 的后端网络端点（通常是 Pod 的 IP 地址）。它是旧版 Endpoints API 的演进，能够支撑大规模 Service（数千个后端 Pod），并高效地更新后端列表，是 kube-proxy 进行内部流量路由的权威数据来源。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/endpointslices.md`）*

---

### Gateway API

Gateway API 是 Kubernetes 中用于暴露网络服务的一组扩展 API（以 CustomResourceDefinition 实现），旨在提供比 Ingress 更动态、更灵活、更面向角色的流量路由能力。它是 Ingress 的继任者，支持基础设施自动配置和高级路由策略，已被 Kubernetes 项目推荐为新项目的首选方案。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/gateway-api.md`）*

---

### Ingress Controllers

Ingress 资源本身只是声明式的路由配置，**必须有 Ingress Controller 在集群中运行**才能将其转化为实际的流量转发规则。Ingress Controller 通常以负载均衡器或反向代理的形式实现，负责监听 Ingress 和 EndpointSlice 的变化，并动态配置底层数据面（如 NGINX、Envoy、云厂商 LB 等）。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/ingress-controllers.md`）*

---

### Ingress

Ingress 是 Kubernetes 中用于管理集群外部 HTTP/HTTPS 访问到内部 Service 的 API 对象。它支持基于主机名（Host）和路径（Path）的路由规则，可提供负载均衡、SSL/TLS 终止以及基于名称的虚拟主机等能力。需要注意的是，**Ingress API 已被冻结**，Kubernetes 官方不再对其新增功能，推荐使用 **Gateway API** 作为继任方案。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/ingress.md`）*

---

### IPv4/IPv6 dual-stack

Kubernetes 支持为 Pod 和 Service 同时分配 IPv4 与 IPv6 地址，实现双栈（Dual-Stack）网络。自 v1.21 起，IPv4/IPv6 双栈默认启用，允许集群中的工作负载通过两种协议族同时进行通信，包括集群内部 Service 访问和 Pod 的集群外出网流量。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/ipv4-ipv6-dual-stack.md`）*

---

### Network Policies

NetworkPolicy 是 Kubernetes 中用于在 OSI 第 3/4 层（IP 地址和端口级别）控制流量的资源对象。它允许你精确指定 Pod 能够与哪些网络“实体”通信，包括其他 Pod、特定命名空间或特定 IP 网段。要实现 NetworkPolicy，集群必须部署支持该功能的 CNI 网络插件。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/network-policies.md`）*

---

### Networking on Windows

Kubernetes 支持在 Windows 节点上运行工作负载，并允许与 Linux 节点混合部署在同一个集群中。Windows 容器网络通过 CNI 插件暴露，其网络模型与 Linux 有显著差异：每个容器拥有一个虚拟网卡（vNIC），连接到 Hyper-V 虚拟交换机（vSwitch），由 Host Networking Service（HNS）和 Host Compute Service（HCS）协同管理。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/networking-on-windows.md`）*

---

### Service ClusterIP allocation

在 Kubernetes 中，`ClusterIP` 类型的 Service 会被分配一个集群范围内的虚拟 IP 地址，客户端通过该 IP 访问 Service，再由 Kubernetes 将流量负载均衡到后端 Pod。整个集群中，每个 Service 的 ClusterIP 必须唯一。Kubernetes 采用了一种分带（banding）分配策略，以降低用户手动指定静态 IP 与系统自动动态分配发生冲突的风险。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/service-clusterip-allocation.md`）*

---

### Service Internal Traffic Policy

Service Internal Traffic Policy（Service 内部流量策略）用于控制集群内部发起的流量如何被路由到后端端点。将该策略设置为 `Local` 时，kube-proxy 会仅将流量转发到与请求源位于**同一节点**上的端点，避免跨节点网络跳转，从而降低延迟、减少网络带宽成本，并有助于保留客户端源 IP。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/service-internal-traffic-policy.md`）*

---

### 服务网格（Service Mesh）

**服务网格（Service Mesh）** 是一种专门处理服务间通信的基础设施层，通过透明代理（Sidecar 或 eBPF）为微服务提供统一的流量管理、安全通信（mTLS）和可观测性能力。2026 年的服务网格技术已形成 **Sidecar 模式（Istio、Linkerd）** 与 **Sidecar-less 模式（Cilium Service Mesh、Istio Ambient Mesh）** 并存的格局。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/service-mesh.md`）*

---

### Service

Service 是 Kubernetes 中用于将运行在一组 Pod 上的网络应用暴露给集群内外的核心抽象对象。由于 Pod 是临时的、会被动态创建和销毁的，其 IP 地址也随之变化，Service 通过稳定的虚拟 IP（ClusterIP）和 DNS 名称，解耦了前端客户端与后端 Pod 的耦合，使现有应用无需改造即可在 Kubernetes 中运行。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/service.md`）*

---

### 电信云与 5G 多接入边缘计算（MEC）

**电信云（Telco Cloud）** 和 **5G 多接入边缘计算（MEC, Multi-access Edge Computing）** 是通信行业数字化转型的核心技术。Kubernetes 正在成为电信网络功能（CNF, Cloud-Native Network Functions）的主流承载平台，替代传统的专用硬件（如 EPC、IMS、RAN）。2026 年，全球主要运营商（如 Verizon、中国移动、德国电信）已将 5G 核心网和边缘节点全面云原生化，Kubernetes 在其中扮演着编排容器化网络功能、管理边缘计算资源的关键角色。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/telco-cloud-and-5g-mec.md`）*

---

### Topology Aware Routing

拓扑感知路由（Topology Aware Routing，旧称 Topology Aware Hints）是一种帮助将网络流量保留在其发起可用区（zone）内的机制。通过在 EndpointSlice 中为端点设置 zone 提示，kube-proxy 可优先将流量路由到同一拓扑区域的端点，从而降低网络延迟、提升可靠性并可能减少跨区流量成本。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/networking/topology-aware-routing.md`）*

---

## 相关页面

- [[entities/k8s-networking-domain-guide.md|k8s-networking-domain-guide]]
- [[entities/k8s-networking-ecosystem.md|k8s-networking-ecosystem]]

## 来源文件

- `domain-17-system-foundation/topic-dictionary/networking/cluster-mesh.md`
- `domain-17-system-foundation/topic-dictionary/networking/cluster-networking.md`
- `domain-17-system-foundation/topic-dictionary/networking/dns-for-services-and-pods.md`
- `domain-17-system-foundation/topic-dictionary/networking/ebpf-and-cilium-networking.md`
- `domain-17-system-foundation/topic-dictionary/networking/endpointslices.md`
- `domain-17-system-foundation/topic-dictionary/networking/gateway-api.md`
- `domain-17-system-foundation/topic-dictionary/networking/ingress-controllers.md`
- `domain-17-system-foundation/topic-dictionary/networking/ingress.md`
- `domain-17-system-foundation/topic-dictionary/networking/ipv4-ipv6-dual-stack.md`
- `domain-17-system-foundation/topic-dictionary/networking/network-policies.md`
- `domain-17-system-foundation/topic-dictionary/networking/networking-on-windows.md`
- `domain-17-system-foundation/topic-dictionary/networking/service-clusterip-allocation.md`
- `domain-17-system-foundation/topic-dictionary/networking/service-internal-traffic-policy.md`
- `domain-17-system-foundation/topic-dictionary/networking/service-mesh.md`
- `domain-17-system-foundation/topic-dictionary/networking/service.md`
- `domain-17-system-foundation/topic-dictionary/networking/telco-cloud-and-5g-mec.md`
- `domain-17-system-foundation/topic-dictionary/networking/topology-aware-routing.md`

## Related

- [[coredns]] — CoreDNS
- [[linkerd]] — Linkerd
- [[cni]] — CNI (Container Network Interface)
- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
