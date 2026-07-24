---
title: Flannel IPv6 Dual Stack 支持
description: Flannel IPv6 Dual Stack 配置指南，涵盖双栈网络架构、配置步骤、验证方法和已知限制
summary: Flannel IPv6 Dual Stack 配置指南，涵盖双栈网络架构、配置步骤、验证方法和已知限制
category: networking
tags:
- k8s
- networking
- flannel
- ipv6
- dual-stack
- cni
- apiserver
- cilium
- calico
- networkpolicy
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
estimated_read_time: 5min
intent_queries:
- Flannel IPv6 配置
- Flannel Dual Stack
- Kubernetes 双栈网络
trigger_keywords:
- Flannel
- IPv6
- Dual Stack
- 双栈
prerequisites:
- kubectl-basics
- networking-basics
- ebpf-basics
- cilium-basics
- cni-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/FTA故障树/list/flannel-fta.md
  label: '故障树: flannel'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flannel IPv6 Dual Stack 支持

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25+ | Flannel v0.20+ | **最后更新**: 2026-05

---

<!-- chunk: 1. 概述 -->
## 1. 概述

Flannel v0.20+ 支持 IPv6 Dual Stack，允许集群同时使用 IPv4 和 IPv6 地址进行 Pod 间通信。

### 1.1 支持矩阵

| 功能 | 支持情况 |
|:-----|:-------:|
| 单 IPv6 集群 | ✓ 支持 |
| [[系统基础/知识字典/networking/ipv4-ipv6-dual-stack.md|IPv4/IPv6 Dual Stack]] | ✓ v0.20+ |
| IPv6 only 后端 (VXLAN) | ✓ v0.21+ |
| Windows 节点 IPv6 | ✗ 暂不支持 |

---

<!-- chunk: 2. 架构原理 -->
## 2. 架构原理

### 2.1 双栈网络拓扑

```
┌─────────────────────────────────────────────────────────────────┐
│                    Dual Stack Cluster                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  Node 1 (192.168.1.10 / 2001:db8::10)                          │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  Pod A: 10.244.1.10 , 2001:db8:1::10                    │    │
│  │       │                                                  │    │
│  │       ▼                                                  │    │
│  │  flannel.1 (VXLAN VTEP - 双栈)                          │    │
│  │  └─ IPv4: 10.244.1.1                                    │    │
│  │  └─ IPv6: 2001:db8:1::1                                 │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 流量路径

**IPv4 流量**: 与传统 VXLAN 相同，通过 flannel.1 封装
**IPv6 流量**: 通过 flannel.1 (IPv6) 封装，或直接路由 (DirectRouting)

---

<!-- chunk: 3. 前置要求 -->
## 3. 前置要求

### 3.1 Kubernetes 版本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Kubernetes 1.16+ 支持双栈 Service
kubectl version --short

# 需要启用 --feature-gates=IPv6DualStack=true (k8s 1.21-1.23)
# Kubernetes 1.24+ 默认启用
```
### 3.2 节点网络要求

```bash
# 确认节点有 IPv6 地址
ip -6 addr show eth0

# 确认 IPv6 转发已启用
sysctl net.ipv6.conf.all.forwarding
# 应为 1
```

---

<!-- chunk: 4. 配置步骤 -->
## 4. 配置步骤

### 4.1 kube-apiserver 配置（k8s < 1.24）

```bash
# 如果使用 kubeadm，在 ClusterConfiguration 中设置
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
networking:
  podCIDR: 10.244.0.0/16
  serviceCIDR: 10.96.0.0/12
  dnsDomain: cluster.local
  ipv6:
    podCIDR: 2001:db8::/64
    serviceCIDR: 2001:db8:1::/112
```

### 4.2 Flannel ConfigMap 配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-flannel-cfg
  namespace: kube-flannel
data:
  net-conf.json: |
    {
      "Network": "10.244.0.0/16,2001:db8::/64",
      "Backend": {
        "Type": "vxlan",
        "VNI": 1,
        "Port": 4789,
        "IPv6Network": "2001:db8::/64"
      }
    }
```

### 4.3 直接路由模式配置

```yaml
{
  "Network": "10.244.0.0/16,2001:db8::/64",
  "Backend": {
    "Type": "vxlan",
    "DirectRouting": true,
    "IPv6Network": "2001:db8::/64"
  }
}
```

### 4.4 host-gw 模式配置

```yaml
{
  "Network": "10.244.0.0/16,2001:db8::/64",
  "Backend": {
    "Type": "host-gw",
    "IPv6Network": "2001:db8::/64"
  }
}
```

---

<!-- chunk: 5. 验证配置 -->
## 5. 验证配置

### 5.1 检查节点 CIDR 分配

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点分配的 Pod CIDR（双栈）
kubectl get nodes -o jsonpath='{range .items[*]} {
  name: {.metadata.name}
  podCIDR: {.spec.podCIDR}
  podCIDRs: {.spec.podCIDRs}
}'

# 预期输出包含 IPv4 和 IPv6 CIDR
```
### 5.2 检查 Flannel 子网环境

```bash
# 在节点上查看
cat /run/flannel/subnet.env

# 预期输出：
# FLANNEL_NETWORK=10.244.0.0/16,2001:db8::/64
# FLANNEL_SUBNET=10.244.1.1/24
# FLANNEL_SUBNET_V6=2001:db8:1::1/64
# FLANNEL_MTU=1450
# FLANNEL_IPV6_MTU=1450
```

### 5.3 检查 flannel.1 接口

```bash
# 查看 IPv4
ip addr show flannel.1

# 查看 IPv6
ip -6 addr show flannel.1

# 查看 VXLAN 配置
ip -d link show flannel.1
```

### 5.4 测试双栈连通性

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 测试 IPv4 连通性
kubectl exec -it <pod-a> -- ping -c 3 <pod-b-ipv4>

# 测试 IPv6 连通性
kubectl exec -it <pod-a> -- ping -6 -c 3 <pod-b-ipv6>

# 测试 Service 连通性
kubectl exec -it <pod-a> -- curl -4 http://kubernetes.default.svc.cluster.local
kubectl exec -it <pod-a> -- curl -6 http://kubernetes.default.svc.cluster.local
```
---

<!-- chunk: 6. 路由表 -->
## 6. 路由表

### 6.1 IPv6 路由示例

```bash
# 在节点上查看 IPv6 路由
ip -6 route show | grep flannel

# 预期：
# 2001:db8:2::/64 via 2001:db8:2:: dev flannel.1 metric 1024
```

### 6.2 FDB 表（VXLAN）

```bash
# 查看 IPv6 FDB
bridge -6 fdb show dev flannel.1

# 预期：
# 2001:db8:2::1 dev flannel.1 dst 192.168.1.20 self permanent
```

---

<!-- chunk: 7. 故障排查 -->
## 7. 故障排查

### 7.1 常见问题

| 问题 | 原因 | 解决方案 |
|:-----|:-----|:--------|
| IPv6 Pod 无地址 | 未启用 Dual Stack | 检查 kube-apiserver 配置 |
| 跨节点 IPv6 不通 | 防火墙阻断 IPv6 | 开放 IPv6 UDP 4789 |
| 路由缺失 | DirectRouting 未生效 | 检查 flanneld 日志 |
| MTU 问题 | IPv6 封装开销更大 | 设置 MTU=1400 |

### 7.2 排查命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 IPv6 模块加载
lsmod | grep vxlan

# 2. 检查 flanneld 日志
kubectl logs -n kube-flannel -l app=flannel --tail=50 | grep -i ipv6

# 3. 检查 IPv6 转发
sysctl -a | grep ipv6.conf.all.forwarding

# 4. 抓包分析 IPv6 流量
tcpdump -i flannel.1 ip6 -nn

# 5. 检查 ND (Neighbor Discovery)
ip -6 neigh show dev flannel.1
```
---

<!-- chunk: 8. 已知限制 -->
## 8. 已知限制

| 限制 | 说明 |
|:-----|:----|
| Windows 不支持 | Flannel Windows 后端暂不支持 IPv6 |
| WireGuard IPv6 | WireGuard 后端需单独配置 IPv6 |
| 云厂商限制 | 部分云厂商对 IPv6 VXLAN 支持有限 |
| 混合集群 | IPv4-only 节点与 IPv6 节点混用需谨慎 |

---

<!-- chunk: 9. 与 [[Cilium|Cilium]]/Calico 对比 -->
## 9. 与 Cilium/Calico 对比

| 特性 | Flannel | Cilium | Calico |
|:-----|:-------:|:------:|:------:|
| IPv6 单栈 | ✓ | ✓ | ✓ |
| IPv6 Dual Stack | ✓ | ✓ | ✓ |
| eBPF-based | ✗ | ✓ | ✗ |
| [[NetworkPolicy|NetworkPolicy]] | 需 Canal | 原生 | 原生 |
| 生产推荐度 | 中 | 高 | 高 |

---

<!-- chunk: 10. 推荐配置场景 -->
## 10. 推荐配置场景

### 场景一：新建双栈集群

```yaml
net-conf.json: |
  {
    "Network": "10.244.0.0/16,2001:db8::/64",
    "Backend": {
      "Type": "vxlan",
      "DirectRouting": true,
      "IPv6Network": "2001:db8::/64"
    }
  }
```

### 场景二：现有 IPv4 集群迁移

1. 先在非生产环境验证
2. 逐步将节点切换到双栈模式
3. 确保控制平面先支持双栈

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 网络 KUDIG Database — Global MOC
- [[网络/README.md|Domain 03: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel Windows 节点支持
- Flannel 多集群场景与子网冲突处理

## See Also

- 04-flannel-complete-guide
- 04a-flannel-wireguard-backend
- 04c-flannel-windows-support
- 04d-flannel-multi-cluster

## Related

- [[生态参考/领域索引/flannel-index.md|Flannel 知识图谱索引]]


<!-- risk-assessed -->
