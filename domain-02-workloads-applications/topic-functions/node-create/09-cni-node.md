---
title: 节点网络：CNI 配置详解
description: 'title: 节点网络 CNI 配置详解'
category: general
tags:
- reference
- etcd
- kubelet
- cilium
- flannel
- calico
- coredns
- containerd
- docker
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点网络：CNI 配置详解 是什么
- 如何 节点网络：CNI 配置详解
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点网络：CNI
- 配置详解
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
created: "2026-05-23"
---

title: 节点网络 CNI 配置详解
description: '# 节点网络：CNI 配置详解'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- kubelet
- cilium
- flannel
- calico
- coredns
- containerd
last_updated: 2026-05-18
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 网络工程师
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes CNI plugin working mechanism
- pod network namespace veth pair
- Calico Cilium Flannel comparison
- CNI IPAM pod IP allocation
- Pod DNS configuration resolv.conf
trigger_keywords:
- CNI
- veth pair
- pod network
- network namespace
- Calico
- Cilium
- Flannel
- IPAM
- bridge
- DNS
- resolv.conf
- network policy
- cni0
- flannel.1
related_domains:
- domain-03-networking-traffic
- domain-10-troubleshooting-diagnostics
related_topics:
- node-create/02-registration
- node-create/08-troubleshooting
- cluster-create/06-join
- domain-03-networking-traffic/01-overview
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 节点网络：CNI 配置详解

## 概述

节点网络是 Kubernetes 集群中最复杂的子系统之一。每个 Pod 需要独立的网络命名空间、唯一的 IP 地址、正确的路由规则，以及与集群内外通信的能力。这些网络配置由 CNI（Container Network Interface）插件在 Pod 创建和销毁时自动完成。

CNI 是 Cloud Native Computing Foundation（CNCF）维护的容器网络标准接口。它定义了一组简单的规范，用于配置 Linux 容器的网络接口。Kubelet 通过 CRI（Container Runtime Interface）调用 CNI 插件来为 Pod 配置网络。

理解节点网络的工作原理对于以下场景至关重要：

- **网络故障排查**：Pod 无法通信时，需要理解 veth pair、网桥、路由的工作机制
- **CNI 插件选型**：不同 CNI 插件（Calico、Cilium、Flannel）有不同的网络模型和性能特征
- **网络策略配置**：NetworkPolicy 的实现依赖于 CNI 插件的能力
- **性能调优**：理解网络路径可以优化 Pod 间通信延迟

本文档详细分析节点网络的架构、CNI 插件的工作流程、Pod 网络命名空间的结构、以及常见网络问题的排查方法。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubelet 网络管理 | `pkg/kubelet/network/` | CNI 调用入口 |
| CNI 插件加载 | `pkg/kubelet/dockershim/network/cni/` | CNI 插件管理 |
| CRI 网络配置 | `pkg/kubelet/cri/` | CRI 层网络调用 |
| CNI 规范 | `containernetworking/cni/` | CNI 接口定义 |
| Calico | `projectcalico/calico/` | Calico CNI 插件 |
| Cilium | `cilium/cilium/` | Cilium CNI 插件 |
| Flannel | `flannel-io/flannel/` | Flannel CNI 插件 |

---

## 一、CNI 架构

### 1.1 CNI 工作流程

```
Pod 网络创建完整流程:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. kubelet 接收到 Pod 创建请求 (syncPod)                   │
  └─────────────────────────────────────────────────────────────┘
                            │
                            ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  2. 通过 CRI 调用容器运行时创建容器                          │
  │     - 创建 network namespace (netns)                        │
  │     - 容器进程加入 netns                                     │
  └─────────────────────────────────────────────────────────────┘
                            │
                            ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  3. 调用 CNI 插件 (cmdAdd)                                  │
  │     a. 读取 CNI 配置文件 (/etc/cni/net.d/)                  │
  │     b. 创建 veth pair (一端在宿主 netns，一端在 Pod netns)   │
  │     c. 调用 IPAM 插件分配 IP 地址                            │
  │     d. 配置 Pod netns 中的网络接口 (eth0)                    │
  │     e. 配置宿主端的路由/网桥                                  │
  │     f. 配置 DNS (/etc/resolv.conf)                           │
  └─────────────────────────────────────────────────────────────┘
                            │
                            ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  4. Pod 网络就绪                                             │
  │     - Pod 内可通过 eth0 通信                                  │
  │     - 同节点 Pod 通过网桥通信                                │
  │     - 跨节点 Pod 通过路由/隧道通信                            │
  └─────────────────────────────────────────────────────────────┘
```

### 1.2 CNI 配置文件

CNI 配置文件位于 `/etc/cni/net.d/` 目录下，文件名以数字前缀排序，数字最小的优先使用：

```json
{
  "cniVersion": "1.0.0",
  "name": "k8s-pod-network",
  "type": "calico",
  "plugins": [
    {
      "type": "calico",
      "datastore_type": "kubernetes",
      "mtu": 1440,
      "ipam": {
        "type": "calico-ipam",
        "assign_ipv4": "true"
      },
      "policy": {
        "type": "k8s"
      },
      "kubernetes": {
        "kubeconfig": "/etc/cni/net.d/calico-kubeconfig"
      }
    },
    {
      "type": "portmap",
      "capabilities": {"portMappings": true}
    },
    {
      "type": "bandwidth",
      "capabilities": {"bandwidth": true}
    }
  ]
}
```

### 1.3 CNI 插件类型

| 插件类型 | 功能 | 来源 |
|---------|------|------|
| `bridge` | 创建 Linux 网桥 | containernetworking/plugins |
| `ptp` | 创建点对点连接 | containernetworking/plugins |
| `host-local` | 本地 IP 地址管理 | containernetworking/plugins |
| `calico` | Calico BGP/VXLAN 网络 | projectcalico |
| `cilium` | Cilium eBPF 网络 | cilium |
| `flannel` | Flannel VXLAN 网络 | flannel |
| `portmap` | 端口映射 (hostPort) | containernetworking/plugins |
| `bandwidth` | 带宽限制 (TBF) | containernetworking/plugins |
| `tuning` | 网络参数调优 | containernetworking/plugins |

---

## 二、Pod 网络命名空间

### 2.1 网络命名空间结构

每个 Pod 拥有独立的网络命名空间，其中包含：

```
Pod 网络命名空间:
  ┌─────────────────────────────────────────────────────────────┐
  │  Network Namespace (netns)                                  │
  │  ├── lo (loopback: 127.0.0.1)                              │
  │  ├── eth0 (Pod IP: 10.244.1.10/24)                         │
  │  │     ├── MAC: xx:xx:xx:xx:xx:xx                          │
  │  │     ├── Gateway: 10.244.1.1                             │
  │  │     └── Routes:                                         │
  │  │           default via 10.244.1.1                         │
  │  │           10.244.1.0/24 dev eth0                         │
  │  ├── /etc/resolv.conf (DNS 配置)                           │
  │  │     nameserver 10.96.0.10                               │
  │  │     search default.svc.cluster.local svc.cluster.local  │
  │  └── iptables rules (NetworkPolicy)                        │
  └─────────────────────────────────────────────────────────────┘
```

### 2.2 查看和调试 Pod 网络

```bash
# 查看 Pod 的网络命名空间 ID
kubectl get pod <pod> -o jsonpath='{.status.containerStatuses[0].containerID}'
# 输出: containerd://abc123...

# 获取容器 PID
crictl inspect <container-id> | grep pid
# 或
pid=$(docker inspect <container-id> -f '{{.State.Pid}}')

# 进入 Pod 网络命名空间
nsenter -t <pid> -n ip addr
nsenter -t <pid> -n ip route
nsenter -t <pid> -n cat /etc/resolv.conf

# 使用 kubectl debug 调试
kubectl debug node/<node> -it --image=nicolaka/netshoot

# 使用 crictl 查看容器网络
crictl exec -i <container-id> ip addr
crictl exec -i <container-id> ip route
```

---

## 三、veth pair 与网桥

### 3.1 节点网络拓扑

```
节点网络结构:
  ┌─────────────────────────────────────────────────────────────────────┐
  │  Host Network Namespace                                            │
  │                                                                    │
  │  eth0 (物理网卡: 192.168.1.10)                                    │
  │      │                                                             │
  │      ├── cni0 (网桥: 10.244.1.1/24)                               │
  │      │     ├── veth1a ──── veth1b (Pod 1 netns: 10.244.1.10)     │
  │      │     ├── veth2a ──── veth2b (Pod 2 netns: 10.244.1.11)     │
  │      │     └── veth3a ──── veth3b (Pod 3 netns: 10.244.1.12)     │
  │      │                                                             │
  │      └── flannel.1 / tunl0 / cilium_host (隧道接口)               │
  │            └── 跨节点通信                                          │
  └─────────────────────────────────────────────────────────────────────┘

  说明:
  - veth pair: 虚拟以太网设备对，一端在宿主 netns，一端在 Pod netns
  - cni0: Linux 网桥，连接同节点所有 Pod
  - 隧道接口: 用于跨节点 Pod 通信 (VXLAN/IP-in-IP/BGP/eBPF)
```

### 3.2 查看网络设备

```bash
# 查看所有网络接口
ip link show

# 查看网桥
ip link show type bridge
bridge fdb show

# 查看 veth pair
ip link show type veth

# 查看路由
ip route
# default via 192.168.1.1 dev eth0
# 10.244.1.0/24 dev cni0 proto kernel scope link src 10.244.1.1
# 10.244.2.0/24 via 192.168.1.11 dev flannel.1  # 跨节点路由

# 查看 ARP 表
arp -a
```

---

## 四、CNI 插件详解

### 4.1 Calico

```bash
# Calico 架构:
# - BGP 模式: 通过 BGP 协议交换路由，Pod IP 可达
# - VXLAN 模式: 通过 VXLAN 隧道封装跨节点流量
# - eBPF 模式: 使用 eBPF 替代 iptables，性能更高

# Calico 组件:
# - calico-node (DaemonSet): 每个节点运行，包含 Felix (规则) 和 BIRD (BGP)
# - calico-typha (Deployment): 减少与 API Server 的连接数
# - calico-kube-controllers: 策略控制器

# 查看 Calico 节点状态
calicoctl node status

# 查看 Calico 路由
calicoctl node status
ip route | grep bird

# 查看 Calico 策略
calicoctl get networkpolicy -A
```

### 4.2 Cilium

```bash
# Cilium 架构:
# - 基于 eBPF 的高性能网络插件
# - 支持 L3/L4/L7 网络策略
# - 支持透明加密 (WireGuard/IPsec)
# - 支持 Cluster Mesh (多集群网络)

# Cilium 组件:
# - cilium-agent (DaemonSet): 每个节点运行
# - cilium-operator (Deployment): IPAM/CRD 管理
# - Hubble: 可观测性组件

# 查看 Cilium 状态
cilium status
cilium endpoint list
cilium bpf tunnel list

# 查看 Hubble 可观测性
hubble observe --since 1m
```

### 4.3 Flannel

```bash
# Flannel 架构:
# - 简单的 VXLAN/VXLAN 网络
# - 不支持 NetworkPolicy (需搭配 Calico)
# - 适合小型集群

# Flannel 配置文件
cat /etc/cni/net.d/10-flannel.conflist

# 查看 Flannel 网络
ip link show flannel.1
ip route | grep flannel

# Flannel etcd 配置
etcdctl get /coreos.com/network/config
```

---

## 五、IPAM（IP 地址管理）

### 5.1 IPAM 工作原理

```bash
# CNI IPAM 插件负责为 Pod 分配 IP 地址
# 常见 IPAM 类型:

# host-local: 本地文件存储 IP 分配
# 分配记录: /var/lib/cni/networks/<network-name>/
cat /var/lib/cni/networks/k8s-pod-network/10.244.1.10
# 输出: <container-id>

# calico-ipam: Calico IPPool 管理
# 从 IPPool 中分配 Block，再从 Block 中分配 IP

# cilium: Cilium IPAM
# 支持 kubernetes host-scope / cluster-pool / multi-pool
```

### 5.2 IP 地址查看

```bash
# 查看已分配的 Pod IP
kubectl get pods -o wide --all-namespaces

# 查看节点上的 IP 分配
ip addr show

# 查看 CNI IPAM 分配记录
ls /var/lib/cni/networks/

# Calico IP 池
calicoctl get ippool -o yaml

# Cilium IP 池
cilium ip list
```

---

## 六、DNS 解析

### 6.1 Pod DNS 配置

```bash
# Pod 内的 DNS 配置
cat /etc/resolv.conf
# nameserver 10.96.0.10           # CoreDNS Service ClusterIP
# search default.svc.cluster.local svc.cluster.local cluster.local
# options ndots:5

# DNS 解析流程:
# Pod 查询 my-svc.default.svc.cluster.local
#     ↓
# /etc/resolv.conf nameserver → CoreDNS Service (10.96.0.10)
#     ↓
# CoreDNS 解析:
# - Service 名称 → ClusterIP
# - Pod 名称 → Pod IP (Headless Service)
# - 外部域名 → 上游 DNS
```

### 6.2 DNS 调试

```bash
# 测试 DNS 解析
kubectl run -it --rm debug --image=busybox -- nslookup kubernetes.default
kubectl run -it --rm debug --image=busybox -- nslookup my-svc.default.svc.cluster.local

# 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns

# 查看 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# 检查 CoreDNS 端点
kubectl get endpoints kube-dns -n kube-system
```

---

## 七、常见错误与排查

| 错误 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| Pod 无法跨节点通信 | CNI 配置错误或隧道未建立 | `ip route; ip link show` | 检查 CNI 配置，重启 CNI DaemonSet |
| IP 冲突 | IPAM 分配冲突 | `arping -D -I eth0 <ip>` | 重启 CNI/IPAM，清理分配记录 |
| DNS 不通 | CoreDNS 异常 | `kubectl get pods -n kube-system -l k8s-app=kube-dns` | 重启 CoreDNS，检查 Service/Endpoints |
| Pod 无法获取 IP | CNI 插件未安装 | `ls /etc/cni/net.d/; ls /opt/cni/bin/` | 安装 CNI 插件 |
| 网络策略不生效 | CNI 不支持 NetworkPolicy | `kubectl get networkpolicy` | 使用支持 NetworkPolicy 的 CNI（Calico/Cilium） |
| veth pair 丢失 | 内核模块未加载 | `ip link show type veth` | 加载 `veth` 内核模块 |
| 网桥转发失败 | iptables FORWARD 规则 | `iptables -L FORWARD` | 设置 `iptables -P FORWARD ACCEPT` |
| CNI 配置文件缺失 | kubeadm init 未安装 CNI | `ls /etc/cni/net.d/` | 手动安装 CNI 插件 |

### 网络调试命令速查

```bash
# 节点级网络检查
ip link show                  # 网络接口
ip addr show                  # IP 地址
ip route                      # 路由表
bridge fdb show               # 网桥转发表
iptables -L -n -v             # iptables 规则
ipvsadm -Ln                   # ipvs 规则

# Pod 网络检查
nsenter -t <pid> -n ip addr   # Pod IP
nsenter -t <pid> -n ip route  # Pod 路由
nsenter -t <pid> -n ss -tlnp  # Pod 监听端口
tcpdump -i any -n port 80     # 抓包

# CNI 检查
ls /etc/cni/net.d/            # CNI 配置
ls /opt/cni/bin/              # CNI 二进制
cat /var/lib/cni/*            # IPAM 记录
```

---

## 相关函数

| 函数/组件 | 源码位置 | 说明 |
|----------|---------|------|
| `cniNetworkPlugin` | `pkg/kubelet/network/cni/` | CNI 插件管理 |
| `cmdAdd` | CNI 插件实现 | Pod 网络创建 |
| `cmdDel` | CNI 插件实现 | Pod 网络删除 |
| `setupPod` | `pkg/kubelet/network/` | kubelet 网络配置入口 |
| `teardownPod` | `pkg/kubelet/network/` | kubelet 网络清理入口 |
| `IPAM` | CNI IPAM 插件 | IP 地址分配 |

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/networking.md|networking]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[skills/ts-networking.md|ts-networking]]
