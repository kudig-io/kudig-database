---
title: CNI 网络模型与插件对比
description: '# CNI 网络模型与插件对比'
summary: 'kubeadm init 完成后，集群节点间网络不通，必须安装 CNI（Container Network Interface）插件。[[kubelet|kubelet]] 通过 CRI 调用 CNI 插件的 ADD/DEL/CHECK 命令来管理 Pod 网络命名空间，实现 Pod IP 分配、跨节点通信和网络策略等能力。'
category: concepts
tags:
- k8s
- cni
- networking
- calico
- cilium
- flannel
- pod-network
- dns
- apiserver
- kubelet
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNI 网络模型与插件对比 是什么
- 如何 CNI 网络模型与插件对比
trigger_keywords:
- CNI
- 网络模型与插件对比
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CNI 网络模型与插件对比

## 概述

kubeadm init 完成后，集群节点间网络不通，必须安装 CNI（Container Network Interface）插件。[[kubelet|kubelet]] 通过 CRI 调用 CNI 插件的 ADD/DEL/CHECK 命令来管理 Pod 网络命名空间，实现 Pod IP 分配、跨节点通信和网络策略等能力。

## CNI 安装时序

```
kubeadm init 完成
    ↓
CoreDNS Pod 处于 Pending（因为节点无网络）
    ↓
安装 CNI 插件（Calico/Cilium/Flannel）
    ↓
CNI DaemonSet 在每个节点运行
    ↓
写入 /etc/cni/net.d/ 配置文件
    ↓
kubelet 检测到 CNI 配置
    ↓
新创建的 Pod 获得 IP 和网络连通
    ↓
CoreDNS Pod 变为 Running
    ↓
集群可正常工作
```

## 主流 CNI 插件对比

| 插件 | 模式 | 数据面 | 性能 | 网络模型 | BGP 支持 | [[NetworkPolicy|NetworkPolicy]] |
|------|------|--------|------|---------|---------|--------------|
| Calico | Overlay + BGP | eBPF/Linux Routing | 高 | IP-in-IP/VXLAN | 是 | 是 |
| Cilium | Overlay + BGP | eBPF | 最高 | VXLAN/Geneve | 是 | 是（eBPF 级别） |
| Flannel | Overlay | VXLAN | 中 | VXLAN | 否 | 否（需配合其他插件） |
| Weave | Overlay | sleeve/fastdp | 中 | VXLAN | 否 | 是 |
| kube-router | 路由 | eBPF | 高 | BGP | 是 | 是 |

## Pod 网络模型

```
节点 A                              节点 B
┌──────────────────┐               ┌──────────────────┐
│  Pod A           │               │  Pod B           │
│  10.244.0.10     │               │  10.244.1.10     │
│       ↓ eth0      │               │       ↓ eth0      │
│  ┌─────┴─────┐    │               │  ┌─────┴─────┐    │
│  │  veth0    │    │               │  │  veth0    │    │
│  └─────┬─────┘    │               │  └─────┬─────┘    │
│        ↓          │               │        ↓          │
│  ┌─────┴─────┐    │               │  ┌─────┴─────┐    │
│  │  bridge   │    │               │  │  bridge   │    │
│  │  cni0     │    │               │  │  cni0     │    │
│  └─────┬─────┘    │               │  └─────┬─────┘    │
│        ↓          │               │        ↓          │
│  ┌─────┴──────────┴─────────────────────────┴─────┐    │
│  │              VXLAN Tunnel (overlay)           │    │
│  └────────────────────┬──────────────────────────┘    │
└──────────────────────┼────────────────────────────────┘
                       ↓
                 物理网卡（eth0）
```

## kubelet CNI 调用链

1. `kubelet.syncPod` 创建 Pod
2. 创建 Sandbox 容器
3. CRI 调用 `RunPodSandbox`
4. `cniNetworkPlugin.addToNetwork` 执行 CNI ADD
5. CNI 插件创建 veth pair、分配 Pod IP、配置路由规则
6. 返回 `PodNetworkStatus`

## CNI 配置

### Calico

```yaml
apiVersion: operator.tigera.io/v1
kind: Installation
metadata:
  name: default
spec:
  calicoNetwork:
    ipPools:
    - blockSize: 26
      cidr: 10.244.0.0/16
      encapsulation: VXLANCrossSubnet
      natOutgoing: Enabled
      nodeSelector: all()
```

### Cilium

```yaml
apiVersion: helm.cilium.io/v1alpha1
kind: Cilium
metadata:
  name: cilium
spec:
  kubeProxyReplacement: strict
  hubble:
    enabled: true
  ipam:
    mode: "kubernetes"
  tunnelProtocol: "vxlan"
```

## DNS 解析流程

```
Pod 内应用发起 DNS 查询:
1. 应用调用 gethostbyname("nginx.default.svc.cluster.local")
    ↓
2. 查询发送到 /etc/resolv.conf 中的 nameserver（10.96.0.10）
    ↓
3. CoreDNS 接收 UDP 53 端口请求
    ↓
4. CoreDNS 查询 Service Endpoints
    ↓
5. 返回 ClusterIP（10.96.0.x）
    ↓
6. kube-proxy iptables/ipvs 将 ClusterIP DNAT 到具体 PodIP
```

### CoreDNS Corefile

```
.:53 {
    errors
    health { lameduck 5s }
    ready
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
       ttl 30
    }
    prometheus :9153
    forward . /etc/resolv.conf { max_concurrent 1000 }
    cache 30
    loop
    reload
    loadbalance
}
```

## kube-proxy 模式

| 模式 | 说明 | 性能 | 状态 |
|------|------|------|------|
| iptables | 使用 iptables 规则转发 | 中（规则多时性能下降） | 默认 |
| ipvs | 使用 ipvs 内核模块转发 | 高（O(1) 查找） | 推荐 |
| nftables | 使用 nftables 规则转发 | 高 | 实验性 |

## 常见错误

| 错误 | 现象 | 原因 | 解决方案 |
|------|------|------|---------|
| CoreDNS 一直 Pending | `0/1 nodes are available` | CNI 未安装 | 安装 Calico/Cilium/Flannel |
| Pod 无法跨节点通信 | `ping: bad address` | 防火墙阻止 VXLAN（4789/8472） | 开放 VXLAN 端口 |
| Service 无法访问 | Connection refused | kube-proxy iptables 规则错误 | 检查 `iptables -t nat -L KUBE-SERVICES` |
| CNI 配置冲突 | Pod 网络异常 | 多个 CNI 配置文件 | 只保留一个 CNI 配置 |
| Pod CIDR 不匹配 | CNI 无法分配 IP | `--pod-network-cidr` 与 CNI 配置不一致 | 统一 Pod CIDR |

## 相关概念

- networking.md|Service 网络]]
- [[实体/cni-plugins.md|CNI 插件]]
- [[技能/kubeadm-cluster-lifecycle.md|[[kubeadm 集群创建生命周期|kubeadm 集群创建生命周期]]]]
- [[实体/kube-apiserver.md|kube-apiserver]]

## Related

- [[coredns]] — CoreDNS
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)


<!-- risk-assessed -->
