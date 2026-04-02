# 集群网络（Cluster Networking）

## 概述

网络是 Kubernetes 的核心组成部分，理解其预期工作方式对于集群管理员至关重要。Kubernetes 需要解决四种不同的网络通信问题：容器到容器通信、Pod 到 Pod 通信、Pod 到 Service 通信、外部到 Service 通信。本文档重点讨论 Pod 到 Pod 的通信以及集群网络的实现方式。

## 核心概念/原理

- **共享机器上的应用隔离**：Kubernetes 允许多个应用共享节点，为了避免端口冲突，Kubernetes 采用动态 IP 分配而非静态端口协调。
- **Kubernetes 网络模型**：每个 Pod 拥有独立的 IP 地址，Pod 之间可以直接通信，无需 NAT。
- **IP 地址范围分配**：集群需要为 Pod、Service 和 Node 分配互不重叠的 IP 地址：
  - **网络插件（CNI）**：为 Pod 分配 IP。
  - **kube-apiserver**：为 Service 分配 IP。
  - **kubelet 或 cloud-controller-manager**：为 Node 分配 IP。

## 关键机制或特性

### 集群网络类型

根据配置的 IP 协议族，Kubernetes 集群可分为：

- **仅 IPv4**：所有组件仅分配 IPv4 地址。
- **仅 IPv6**：所有组件仅分配 IPv6 地址。
- **双栈（IPv4/IPv6 或 IPv6/IPv4）**：所有组件分配 IPv4 和 IPv6 地址，且必须就主 IP 族达成一致。

注意：集群类型取决于 Pod、Service 和 Node 对象中存在的 IP 地址，而不是底层服务器接口上的实际 IP。

### CNI 插件

容器运行时通过 **Container Network Interface (CNI)** 插件来管理网络和安全功能。常见的 CNI 插件包括：

- **Calico**：提供网络和网络策略，支持多种网络模式。
- **Cilium**：基于 eBPF 的网络、可观测性和安全解决方案。
- **Flannel**：简单的 overlay 网络提供者。
- **Weave Net**：提供网络和网络策略，支持网络分区后继续工作。
- **Antrea、OVN-Kubernetes、Multus** 等。

不同 CNI 插件提供的功能范围不同，有的只是基本的接口增删，有的则提供高级 IPAM、多插件集成、网络策略等能力。

## 使用场景

- **基础 Pod 网络**：为集群中所有 Pod 提供互通的网络环境。
- **网络策略隔离**：通过支持 NetworkPolicy 的 CNI 插件实现 Pod 间的安全隔离。
- **双栈支持**：需要同时支持 IPv4 和 IPv6 的业务场景。
- **高性能网络**：对网络延迟和吞吐量要求高的场景，可选择基于 eBPF 或直接路由的 CNI 方案。
- **多云/混合云部署**：选择支持跨集群网络或 overlay 网络的 CNI 插件。

## 最佳实践/注意事项

- 确保 Pod CIDR、Service CIDR 和 Node IP 范围不重叠。
- 选择 CNI 插件时，综合考虑功能需求（网络策略、双栈、多网卡、eBPF 等）和运维复杂度。
- 在双栈集群中，确保所有相关组件（CNI、kube-apiserver、kubelet/cloud-controller-manager）对主 IP 族的配置一致。
- 对于大规模集群，注意 CNI 插件的 IPAM 性能和可扩展性。
- 定期评估 CNI 插件的社区活跃度和安全更新情况。

## 参考链接

- [Cluster Networking - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/networking/)
