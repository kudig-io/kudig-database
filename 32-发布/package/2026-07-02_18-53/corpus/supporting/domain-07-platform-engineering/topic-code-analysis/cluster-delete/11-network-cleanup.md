---
title: 网络清理详解 — CNI / iptables / ipvs / 路由 (topic-code-analysis)
description: 'title: 网络清理详解 — CNI / iptables / ipvs / 路由'
summary: 'title: 网络清理详解 — CNI / iptables / ipvs / 路由'
category: general
tags:
- reference
- networking
- kubelet
- cilium
- flannel
- calico
- networkpolicy
- ebpf
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 网络清理详解 — CNI / iptables / ipvs / 路由 是什么
- 如何 网络清理详解 — CNI / iptables / ipvs / 路由
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 网络清理详解
- CNI
- iptables
- ipvs
- 路由
- platform
- engineering
- code
prerequisites:
- kubectl-basics
- platform-engineering-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 网络清理详解 — CNI / iptables / ipvs / 路由
category: cluster-delete
tags:
- network
- cni
- iptables
- ipvs
- flannel
- calico
- cilium
- weave
- cleanup
- routing
last_updated: 2026-05-18
description: 深入分析 Kubernetes 集群删除后的网络清理机制，涵盖 CNI 配置清理（Flannel/Calico/Cilium/Weave）、iptables
  KUBE 链清理、ipvs 规则清理、虚拟网络接口清理（cni0/flannel.1/tunl0/kube-ipvs0）以及路由清理等完整网络残留清除方案。
difficulty: advanced
intent_queries:
- kubernetes network cleanup after cluster deletion
- cni plugin cleanup flannel calico cilium
- iptables KUBE chain cleanup kubernetes
- ipvs cleanup kubernetes cluster delete
- kubelet network namespace cleanup
trigger_keywords:
- network cleanup
- cni cleanup
- iptables cleanup
- ipvs cleanup
- flannel cleanup
- calico cleanup
- cilium cleanup
- weave cleanup
- cni0
- flannel.1
- tunl0
- kube-ipvs0
- veth cleanup
reading_level: advanced
audience:
- platform-engineer
- network-engineer
- kubernetes-administrator
estimated_read_time: 5min
related_domains:
- domain-01-cluster-fundamentals
- domain-03-networking-traffic
related_topics:
- cluster-delete
- cleanup
- security-delete
- force-delete
domain_link: '[Networking](../domain-03-networking-traffic/README.md)'
topic_link: '[Cluster Delete Overview](./01-overview.md)'
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

# 网络清理详解 — CNI / iptables / ipvs / 路由

## 概述

`kubeadm reset` **不会**自动清理网络配置。这是设计决策——不同 CNI 插件有不同的清理需求，且 iptables/ipvs 规则可能包含非 Kubernetes 规则，盲目清理会影响主机网络。本文档详细分析各类网络配置的残留位置和清理方法。

---

## 源码中的设计决策

```go
var manualCleanupInstructions = dedent.Dedent(`
    The reset process does not perform cleanup of CNI plugin configuration,
    network filtering rules and kubeconfig files.
`)

```

**原因**:
1. CNI 插件种类繁多（Calico/Cilium/Flannel/Weave/Terway...），清理逻辑各不相同
2. iptables 规则可能包含非 Kubernetes 规则，盲目 `iptables -F` 会破坏主机网络
3. 路由和虚拟接口可能与宿主网络共享命名空间

---

## 1. CNI 配置清理

### 1.1 CNI 配置目录

```bash
ls /etc/cni/net.d/
```

不同 CNI 插件的配置文件：

| CNI 插件 | 配置文件 | 说明 |
|----------|---------|------|
| Flannel | `10-flannel.conflist` | Flannel CNI 配置 |
| Calico | `10-calico.conflist` / `calico-kubeconfig` | Calico CNI + kubeconfig |
| Cilium | `05-cilium.conflist` | Cilium CNI 配置 |
| Weave | `10-weave.conflist` | Weave CNI 配置 |
| Terway (ACK) | `10-terway.conflist` | 阿里云 Terway |
| Amazon VPC | `10-aws.conflist` | AWS VPC CNI |

### 1.2 清理方法

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
# 通用清理
rm -rf /etc/cni/net.d/*  # ⚠️ 删除系统/数据文件

# CNI 二进制（可选，如果不再需要）
rm -rf /opt/cni/bin/*  # ⚠️ 删除系统/数据文件
```

### 1.3 各 CNI 插件专用清理

#### Flannel

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
rm -rf /etc/cni/net.d/10-flannel.conflist  # ⚠️ 删除系统/数据文件
rm -rf /var/lib/cni/flannel/*  # ⚠️ 删除系统/数据文件
ip link delete flannel.1 2>/dev/null || true
```

#### Calico

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
rm -rf /etc/cni/net.d/10-calico.conflist  # ⚠️ 删除系统/数据文件
rm -rf /etc/cni/net.d/calico-kubeconfig  # ⚠️ 删除系统/数据文件
rm -rf /var/lib/cni/calico/*  # ⚠️ 删除系统/数据文件
ip link delete tunl0 2>/dev/null || true
ip link delete vxlan.calico 2>/dev/null || true
ip link delete cali.* 2>/dev/null || true   # calico 接口
```

#### Cilium

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
rm -rf /etc/cni/net.d/05-cilium.conflist  # ⚠️ 删除系统/数据文件
rm -rf /run/cilium/*  # ⚠️ 删除系统/数据文件
cilium uninstall 2>/dev/null || true        # 如果 cilium agent 还在运行
ip link delete cilium_host 2>/dev/null || true
ip link delete cilium_net 2>/dev/null || true
```

#### Weave

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
rm -rf /etc/cni/net.d/10-weave.conflist  # ⚠️ 删除系统/数据文件
rm -rf /var/lib/weave/*  # ⚠️ 删除系统/数据文件
ip link delete weave 2>/dev/null || true
ip link delete datapath 2>/dev/null || true
```

---

## 2. iptables 规则清理

### 2.1 Kubernetes 创建的 iptables 链

kube-proxy（iptables 模式）创建以下自定义链：

| 链名 | 表 | 用途 |
|------|-----|------|
| `KUBE-SERVICES` | nat | Service ClusterIP DNAT |
| `KUBE-EXTERNAL-SERVICES` | nat | ExternalIP / LoadBalancer |
| `KUBE-FIREWALL` | filter | 防火墙规则 |
| `KUBE-FORWARD` | filter | 转发规则 |
| `KUBE-NODEPORTS` | nat | NodePort DNAT |
| `KUBE-POSTROUTING` | nat | MASQUERADE |
| `KUBE-MARK-MASQ` | nat | 标记需要 MASQ 的包 |
| `KUBE-MARK-DROP` | nat | 标记需要丢弃的包 |

### 2.2 完整 iptables 清理

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `iptables -F/-P DROP`：清空/改防火墙规则，可能立即断网(含SSH)

```bash
# 清理 filter 表
iptables -F KUBE-FIREWALL 2>/dev/null || true
iptables -F KUBE-FORWARD 2>/dev/null || true
iptables -X KUBE-FIREWALL 2>/dev/null || true
iptables -X KUBE-FORWARD 2>/dev/null || true

# 清理 nat 表
iptables -t nat -F KUBE-SERVICES 2>/dev/null || true
iptables -t nat -F KUBE-EXTERNAL-SERVICES 2>/dev/null || true
iptables -t nat -F KUBE-NODEPORTS 2>/dev/null || true
iptables -t nat -F KUBE-POSTROUTING 2>/dev/null || true
iptables -t nat -F KUBE-MARK-MASQ 2>/dev/null || true
iptables -t nat -F KUBE-MARK-DROP 2>/dev/null || true
iptables -t nat -X KUBE-SERVICES 2>/dev/null || true
iptables -t nat -X KUBE-EXTERNAL-SERVICES 2>/dev/null || true
iptables -t nat -X KUBE-NODEPORTS 2>/dev/null || true
iptables -t nat -X KUBE-POSTROUTING 2>/dev/null || true
iptables -t nat -X KUBE-MARK-MASQ 2>/dev/null || true
iptables -t nat -X KUBE-MARK-DROP 2>/dev/null || true

# 清理 mangle 表
iptables -t mangle -F KUBE-SERVICES 2>/dev/null || true
iptables -t mangle -X KUBE-SERVICES 2>/dev/null || true

# 移除对 KUBE 链的引用
iptables -D INPUT -j KUBE-FIREWALL 2>/dev/null || true
iptables -D FORWARD -j KUBE-FORWARD 2>/dev/null || true
iptables -t nat -D PREROUTING -j KUBE-SERVICES 2>/dev/null || true
iptables -t nat -D OUTPUT -j KUBE-SERVICES 2>/dev/null || true
iptables -t nat -D POSTROUTING -j KUBE-POSTROUTING 2>/dev/null || true
```

### 2.3 一键清理脚本（仅 K8s 规则）

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `iptables -F/-P DROP`：清空/改防火墙规则，可能立即断网(含SSH)

```bash
#!/bin/bash
# 仅清理 Kubernetes 相关的 iptables 规则

echo ">>> 清理 iptables KUBE 链"
for chain in $(iptables -t nat -L | grep "Chain KUBE" | awk '{print $2}'); do
    iptables -t nat -F "$chain" 2>/dev/null
    iptables -t nat -X "$chain" 2>/dev/null
done

for chain in $(iptables -t filter -L | grep "Chain KUBE" | awk '{print $2}'); do
    iptables -F "$chain" 2>/dev/null
    iptables -X "$chain" 2>/dev/null
done

for chain in $(iptables -t mangle -L | grep "Chain KUBE" | awk '{print $2}'); do
    iptables -t mangle -F "$chain" 2>/dev/null
    iptables -t mangle -X "$chain" 2>/dev/null
done

echo ">>> iptables KUBE 链已清理"
```

### 2.4 ⚠️ 危险操作：全表清空

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `iptables -F/-P DROP`：清空/改防火墙规则，可能立即断网(含SSH)

```bash
# ⚠️ 会清除所有 iptables 规则（包括非 K8s 的）
# 仅在专用节点上使用
iptables -F
iptables -t nat -F
iptables -t mangle -F
iptables -X
iptables -t nat -X
iptables -t mangle -X
```

---

## 3. ipvs 规则清理

### 3.1 kube-proxy ipvs 模式

kube-proxy 使用 ipvs 模式时创建虚拟服务和真实服务器映射：

```bash
# 查看 ipvs 规则
ipvsadm -Ln
```

### 3.2 清理方法

```bash
# 清理所有 ipvs 规则
ipvsadm -C

# 查看清理结果
ipvsadm -Ln
# TCP/UDP  无任何条目
```

### 3.3 ipvs 相关的虚拟接口

kube-proxy ipvs 模式使用 `kube-ipvs0` 接口绑定 ClusterIP：

```bash
# 查看 kube-ipvs0 接口
ip addr show kube-ipvs0

# 删除
ip link delete kube-ipvs0 2>/dev/null || true
```

---

## 4. 路由清理

### 4.1 Kubernetes 创建的路由

```bash
# 查看路由表
ip route show | grep -E "10\.(244|96|32)\."

# 典型的 K8s 路由
# 10.244.0.0/24 via 192.168.1.10 dev eth0   (Calico BGP)
# 10.244.1.0/24 dev cni0 scope link          (Flannel)
# 10.244.2.0/24 via 192.168.1.12 dev eth0    (Calico BGP)
```

### 4.2 清理方法

```bash
# 查找并删除 K8s 相关路由（根据 Pod CIDR）
ip route show | grep "10.244" | while read route; do
    ip route del $route 2>/dev/null || true
done

# 或使用路由表号清理（如果 CNI 使用了独立路由表）
ip route flush table <table-id>
```

### 4.3 策略路由

部分 CNI（如 Calico、Cilium）使用策略路由：

```bash
# 查看策略路由规则
ip rule show

# 清理 Calico 策略路由
ip rule del from all lookup <calico-table-id> 2>/dev/null || true
ip rule del to all lookup <calico-table-id> 2>/dev/null || true

# 清理 Cilium 策略路由
ip rule del from all lookup <cilium-table-id> 2>/dev/null || true
```

---

## 5. 虚拟网络接口清理

### 5.1 常见的 K8s 虚拟接口

| 接口名 | 创建者 | 用途 |
|--------|--------|------|
| `cni0` | bridge CNI/Flannel | Pod 网络桥接 |
| `flannel.1` | Flannel | VXLAN 隧道 |
| `tunl0` | Calico (IPIP) | IPIP 隧道 |
| `vxlan.calico` | Calico (VXLAN) | VXLAN 隧道 |
| `kube-ipvs0` | kube-proxy (ipvs) | ClusterIP 绑定 |
| `cilium_host` | Cilium | eBPF 网络 |
| `cilium_net` | Cilium | eBPF 网络 |
| `weave` | Weave | Sleeve/fastdp |
| `datapath` | Weave | OVS datapath |
| `veth*` | 各 CNI | Pod 虚拟网卡对 |
| `cali*` | Calico | Calico veth 对 |

### 5.2 清理方法

```bash
# 删除 cni0 网桥
ip link delete cni0 2>/dev/null || true

# 删除 flannel 接口
ip link delete flannel.1 2>/dev/null || true

# 删除 calico 接口
ip link delete tunl0 2>/dev/null || true
ip link delete vxlan.calico 2>/dev/null || true

# 删除 cilium 接口
ip link delete cilium_host 2>/dev/null || true
ip link delete cilium_net 2>/dev/null || true

# 删除 weave 接口
ip link delete weave 2>/dev/null || true
ip link delete datapath 2>/dev/null || true

# 删除 ipvs 接口
ip link delete kube-ipvs0 2>/dev/null || true

# 清理所有 veth 对（Pod 残留）
ip link show | grep "veth" | awk -F: '{print $2}' | tr -d ' ' | while read iface; do
    ip link delete "$iface" 2>/dev/null || true
done

# 清理所有 cali 接口
ip -br link show | grep "^cali" | awk '{print $1}' | while read iface; do
    ip link delete "$iface" 2>/dev/null || true
done
```

---

## 6. 网络命名空间清理

### 6.1 查找残留网络命名空间

```bash
# 列出所有网络命名空间
ip netns list

# 查找 CNI 创建的命名空间
ls /var/run/netns/ | grep cni
```

### 6.2 清理方法

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
# 删除所有 CNI 网络命名空间
for ns in $(ip netns list | awk '{print $1}'); do
    ip netns delete "$ns" 2>/dev/null || true
done

# 清理残留的 netns 文件
rm -rf /var/run/netns/*  # ⚠️ 删除系统/数据文件
```

---

## 7. 完整网络清理脚本

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
#!/bin/bash
echo "=== 开始网络清理 ==="

# 1. CNI 配置
rm -rf /etc/cni/net.d/*  # ⚠️ 删除系统/数据文件

# 2. iptables（仅 KUBE 链）
for table in nat filter mangle; do
    for chain in $(iptables -t $table -L 2>/dev/null | grep "Chain KUBE" | awk '{print $2}'); do
        iptables -t $table -F "$chain" 2>/dev/null
        iptables -t $table -X "$chain" 2>/dev/null
    done
done

# 3. ipvs
ipvsadm -C 2>/dev/null || true

# 4. 虚拟接口
ip link delete cni0 2>/dev/null || true
ip link delete flannel.1 2>/dev/null || true
ip link delete tunl0 2>/dev/null || true
ip link delete kube-ipvs0 2>/dev/null || true
ip link delete cilium_host 2>/dev/null || true
ip link delete weave 2>/dev/null || true

# 5. veth 对
ip -br link show | grep "^veth" | awk '{print $1}' | while read iface; do
    ip link delete "$iface" 2>/dev/null || true
done
ip -br link show | grep "^cali" | awk '{print $1}' | while read iface; do
    ip link delete "$iface" 2>/dev/null || true
done

# 6. 网络命名空间
for ns in $(ip netns list 2>/dev/null | awk '{print $1}'); do
    ip netns delete "$ns" 2>/dev/null || true
done

# 7. CNI 数据
rm -rf /var/lib/cni/*  # ⚠️ 删除系统/数据文件
rm -rf /opt/cni/bin/* 2>/dev/null || true  # ⚠️ 删除系统/数据文件

echo "=== 网络清理完成 ==="

```

---

## 参考

- [kubeadm reset 手动清理](https://kubernetes.io/docs/reference/setup-tools/kubeadm/kubeadm-reset/#cleaning-up-your-control-plane-node)
- [kube-proxy iptables 详解](https://kubernetes.io/docs/concepts/services-networking/service/#proxy-mode-iptables)
- [CNI 规范](https://github.com/containernetworking/cni/blob/master/SPEC.md)

## Related

- [[reference|#reference Hub]] — tag hub

- 22-networkpolicy-reference
- [[README|README]]
- [[scripts/man/INSTALL.md|INSTALL]]
- [[domain-17-system-foundation/速查卡/go.md|go]]
- [[domain-17-system-foundation/速查卡/networking.md|networking]]

```

<!-- risk-assessed -->
