---
title: 集群网络（Cluster Networking）
description: '# 集群网络（Cluster Networking）'
summary: '# 集群网络（Cluster Networking）'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- kubelet
- controller-manager
- cilium
- flannel
- calico
- daemonset
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 集群网络（Cluster Networking） 是什么
- 如何 集群网络（Cluster Networking）
trigger_keywords:
- 集群网络
- Cluster
- Networking
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 集群网络（Cluster Networking）

## 概述

网络是 [[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]] 的核心组成部分，理解其预期工作方式对于集群管理员至关重要。Kubernetes 需要解决四种不同的网络通信问题：容器到容器通信、Pod 到 Pod 通信、Pod 到 [[Service|Service]] 通信、外部到 Service 通信。本文档重点讨论 Pod 到 Pod 的通信以及集群网络的实现方式。

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

## 生产 YAML 示例

### CIDR 规划参考

```yaml
# 集群网络 CIDR 规划（确保不重叠）
# kube-apiserver 配置
--service-cluster-ip-range=10.96.0.0/16      # Service CIDR（65534 个 IP）

# kube-controller-manager 配置
--cluster-cidr=10.244.0.0/16                 # Pod CIDR
--node-cidr-mask-size=24                      # 每节点 /24（254 个 Pod IP）

# 节点 IP 范围（由基础设施决定）
# Node Network: 192.168.0.0/16

# 验证不重叠：
# Pod:     10.244.0.0/16 ✓
# Service: 10.96.0.0/16  ✓
# Node:    192.168.0.0/16 ✓
```

### Multus 多网卡配置

```yaml
# 为 Pod 添加第二张网卡（如 SR-IOV 高性能网络）
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: fast-network
  namespace: production
spec:
  config: '{
    "cniVersion": "0.3.1",
    "type": "macvlan",
    "master": "eth1",
    "mode": "bridge",
    "ipam": {
      "type": "host-local",
      "subnet": "10.10.0.0/16"
    }
  }'
---
apiVersion: v1
kind: Pod
metadata:
  name: multi-nic-pod
  annotations:
    k8s.v1.cni.cncf.io/networks: fast-network
spec:
  containers:
  - name: app
    image: registry.example.com/apps/high-perf:v1.0
```

## CNI 插件选型对比

| CNI | 网络策略 | eBPF | 双栈 | 加密 | 多网卡 | 适用场景 |
|-----|---------|------|------|------|--------|----------|
| Calico | 完整 | 可选 | 支持 | WireGuard | Multus | 通用生产 |
| Cilium | 完整+L7 | 原生 | 支持 | WireGuard/IPSec | 支持 | 高性能/安全 |
| Flannel | 不支持 | 否 | 有限 | 否 | 否 | 简单/学习 |
| Weave Net | 支持 | 否 | 支持 | IPSec | 否 | 小规模 |
| Antrea | 支持 | 否 | 支持 | IPSec | 否 | VMware 生态 |
| OVN-K8s | 支持 | 否 | 支持 | 否 | 支持 | OpenStack 集成 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 间无法通信 | CNI 未正确安装或 Pod CIDR 配置错误 | `kubectl get pods -n kube-system`；`ip route` 检查路由 |
| 节点 NotReady | CNI DaemonSet Pod 未启动 | `kubectl get ds -n kube-system`；查看 CNI Pod 日志 |
| Pod IP 耗尽 | 节点 CIDR mask 过小 | 调整 `--node-cidr-mask-size`；扩大 `--cluster-cidr` |
| 跨节点通信丢包 | overlay 封装 MTU 不匹配 | 检查 CNI 的 MTU 配置（通常需要 -50 for VXLAN） |

## 生产检查清单

- [ ] Pod CIDR、Service CIDR、Node IP 范围不重叠
- [ ] CNI 插件满足功能需求（网络策略、双栈、性能等）
- [ ] 节点 CIDR mask 大小为预期最大 Pod 数的 2 倍以上
- [ ] MTU 配置考虑 overlay 封装开销
- [ ] 大规模集群评估 CNI 的 IPAM 性能和可扩展性

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看节点分配的 Pod CIDR
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'

# 查看集群 CIDR 配置
kubectl cluster-info dump | grep -E "cluster-cidr|service-cluster-ip-range"

# 查看 CNI DaemonSet 状态
kubectl get ds -n kube-system

# 检查 Pod 网络
kubectl exec <pod> -- ip addr
kubectl exec <pod> -- ip route
```
## 交叉引用

- [eBPF 与 Cilium](ebpf-and-cilium-networking.md) — 基于 eBPF 的 CNI 方案
- [Network Policies](network-policies.md) — 依赖 CNI 实现的流量控制
- [IPv4/IPv6 Dual Stack](ipv4-ipv6-dual-stack.md) — 双栈 CIDR 规划
- [Service ClusterIP Allocation](service-clusterip-allocation.md) — Service CIDR 的分带策略

## 参考链接

- [Cluster Networking - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/networking/)

## Related

- [[17-系统基础/06-知识字典/networking/aeraki-mesh.md|Aeraki Mesh 七层网格]]
- [[17-系统基础/06-知识字典/networking/akri.md|Akri 边缘设备发现]]
- [[17-系统基础/06-知识字典/networking/antrea.md|Antrea 网络方案]]


<!-- risk-assessed -->
