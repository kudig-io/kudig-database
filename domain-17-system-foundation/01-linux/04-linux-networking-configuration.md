---
title: 04 - Linux 网络配置与性能优化：生产环境网络运维专家指南
description: '# 04 - Linux 网络配置与性能优化：生产环境网络运维专家指南'
category: linux
tags:
- linux
- system
- kernel
- kubelet
- scheduler
- prometheus
- cilium
- flannel
- calico
- coredns
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 运维工程师
- SRE
- 系统管理员
estimated_read_time: 5min
intent_queries:
- Linux 网络配置与性能优化：生产环境网络运维专家指南 是什么
- 如何 Linux 网络配置与性能优化：生产环境网络运维专家指南
- Kubernetes 14 linux 最佳实践
trigger_keywords:
- Linux
- 网络配置与性能优化：生产环境网络运维专家指南
- linux
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
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
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/linux.md
  label: '速查卡: linux'
created: "2026-05-23"
---

# 04 - Linux 网络配置与性能优化：生产环境网络运维专家指南

> **适用版本**: Linux Kernel 5.x/6.x | **最后更新**: 2026-02 | **作者**: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: 概述 -->## 概述

网络是 [[Kubernetes|Kubernetes]] 集群基础设施中最关键也是最复杂的层次之一。从 Pod 内部的网络通信到 [[Service|Service]] 的负载均衡，从 [[Ingress|Ingress]] 的流量入口到 CNI 插件的底层实现，每一个环节都依赖 Linux 内核网络子系统。本文档深入解析 Linux 网络架构的核心概念，包括网络命名空间（Network Namespace）、虚拟以太网设备（veth pair）、网桥（bridge）、iptables/nftables 包过滤、IPVS 负载均衡、以及隧道技术（VXLAN/IP-in-IP）。这些技术是 Kubernetes 网络模型（CNI、kube-proxy、NetworkPolicy）的底层基础，掌握它们对于理解和排查 Kubernetes 网络问题至关重要。

---

<!-- chunk: 核心概念详解 -->## 核心概念详解

## Linux 网络架构总览

```
┌─────────────────────────────────────────────────────────────────┐
│                        用户空间                                   │
│   应用程序 (nginx, curl, kube-proxy)                             │
│   Socket API: socket(), bind(), listen(), accept(), send()      │
└───────────────────────────┬─────────────────────────────────────┘
                            │ 系统调用
┌───────────────────────────┴─────────────────────────────────────┐
│                        内核空间                                   │
│                                                                  │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │              TCP/IP 协议栈                                │  │
│   │  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐           │  │
│   │  │ TCP    │ │ UDP    │ │ ICMP   │ │ ARP    │           │  │
│   │  └────┬───┘ └────┬───┘ └────┬───┘ └────┬───┘           │  │
│   │       └──────────┴─────┬────┴──────────┘                │  │
│   │                        │                                  │  │
│   │                    IP 层 (路由/转发)                       │  │
│   │                        │                                  │  │
│   │       ┌────────────────┼────────────────┐                │  │
│   │       │                │                │                │  │
│   │  ┌────┴────┐    ┌─────┴─────┐   ┌─────┴─────┐          │  │
│   │  │netfilter │    │ conntrack │   │  NFQUEUE  │          │  │
│   │  │iptables  │    │ 连接跟踪  │   │  用户队列  │          │  │
│   │  │nftables  │    │           │   │           │          │  │
│   │  └─────────┘    └───────────┘   └───────────┘          │  │
│   └──────────────────────────────────────────────────────────┘  │
│                            │                                     │
│   ┌────────────────────────┴────────────────────────────────┐  │
│   │              网络设备接口                                 │  │
│   │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐       │  │
│   │  │ eth0    │ │ veth0   │ │ br0     │ │ lo      │       │  │
│   │  │ 物理网卡│ │ 虚拟网卡│ │ 网桥    │ │ 回环    │       │  │
│   │  └─────────┘ └─────────┘ └─────────┘ └─────────┘       │  │
│   └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## 网络命名空间 (Network Namespace)

网络命名空间是 Linux 内核提供的网络隔离机制，每个命名空间拥有独立的网络栈：网络接口、路由表、iptables 规则、端口号空间等。这是容器网络隔离的基础。

```bash
# 创建网络命名空间
ip netns add ns1
ip netns add ns2

# 列出所有命名空间
ip netns list

# 在命名空间中执行命令
ip netns exec ns1 ip addr
ip netns exec ns1 ping 10.0.0.2

# 查看进程的网络命名空间
ls -la /proc/<pid>/ns/net
readlink /proc/<pid>/ns/net

# 比较两个进程是否在同一命名空间
readlink /proc/1/ns/net
readlink /proc/<pid>/ns/net
```

## veth pair (虚拟以太网设备对)

veth pair 是一对虚拟网络设备，数据从一端进入会从另一端出来，类似一根虚拟网线。Kubernetes 中每个 Pod 都通过 veth pair 连接到宿主机的网桥。

```bash
# 创建 veth pair
ip link add veth-host type veth peer name veth-container

# 将一端移到容器命名空间
ip link set veth-container netns ns1

# 配置宿主机端
ip addr add 10.0.0.1/24 dev veth-host
ip link set veth-host up

# 配置容器端
ip netns exec ns1 ip addr add 10.0.0.2/24 dev veth-container
ip netns exec ns1 ip link set veth-container up
ip netns exec ns1 ip link set lo up
ip netns exec ns1 ip route add default via 10.0.0.1

# 测试连通性
ip netns exec ns1 ping 10.0.0.1
```

## 网桥 (Bridge)

网桥是 Linux 内核提供的数据链路层设备，工作在 MAC 层，将多个网络接口连接在一起，类似于物理交换机。Kubernetes 的多种 CNI 插件（如 bridge 模式、Flannel、Calico）都使用网桥来连接 Pod。

```bash
# 创建网桥
ip link add br0 type bridge
ip link set br0 up

# 将接口加入网桥
ip link set veth-host master br0
ip link set eth1 master br0

# 查看网桥
bridge link show
bridge fdb show           # MAC 转发表
brctl show                # 传统命令

# 配置网桥 IP
ip addr add 172.17.0.1/16 dev br0
```

```
┌─────────────────────────────────────────────────────────────┐
│                      宿主机网络                              │
│                                                              │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐               │
│  │  Pod 1   │   │  Pod 2   │   │  Pod 3   │               │
│  │ ns:pod1  │   │ ns:pod2  │   │ ns:pod3  │               │
│  │10.244.1.2│   │10.244.1.3│   │10.244.1.4│               │
│  └────┬─────┘   └────┬─────┘   └────┬─────┘               │
│       │              │              │                        │
│  veth-pod1      veth-pod2     veth-pod3                    │
│       │              │              │                        │
│  ┌────┴──────────────┴──────────────┴────┐                 │
│  │            cbr0 / br0 网桥             │                 │
│  │            172.17.0.1/16              │                 │
│  └──────────────────┬────────────────────┘                 │
│                     │                                        │
│                ┌────┴────┐                                   │
│                │  eth0   │ ← 物理网卡                        │
│                │ NAT/路由 │                                   │
│                └─────────┘                                   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## iptables / nftables 包过滤

iptables 是 Linux 内核 netfilter 框架的用户空间工具，用于配置网络包过滤、NAT 和 mangle 规则。Kubernetes 的 kube-proxy 在 iptables 模式下会生成大量 iptables 规则来实现 Service 负载均衡。

## iptables 架构

```
                           数据包进入
                              │
                              ▼
                    ┌─────────────────┐
                    │   PREROUTING    │ ← raw, mangle, nat(DNAT)
                    └────────┬────────┘
                             │
                    ┌────────┴────────┐
                    │                 │
               目标是本机          需要转发
                    │                 │
                    ▼                 ▼
            ┌──────────────┐  ┌──────────────┐
            │    INPUT     │  │   FORWARD    │
            │ filter, mangle│  │ filter, mangle│
            └──────┬───────┘  └──────┬───────┘
                   │                 │
                   ▼                 │
            ┌──────────────┐         │
            │   本地进程    │         │
            └──────┬───────┘         │
                   │                 │
                   ▼                 │
            ┌──────────────┐         │
            │   OUTPUT     │         │
            │ raw, mangle  │         │
            │ nat, filter  │         │
            └──────┬───────┘         │
                   │                 │
                   └────────┬────────┘
                            │
                            ▼
                   ┌──────────────┐
                   │ POSTROUTING  │ ← mangle, nat(SNAT/MASQUERADE)
                   └──────────────┘
                            │
                            ▼
                        数据包发出
```

## iptables 在 Kubernetes 中的应用

```bash
# kube-proxy 生成的 iptables 规则链
# KUBE-SERVICES    - Service 入口
# KUBE-SVC-XXX     - Service 规则
# KUBE-SEP-XXX     - Endpoint 规则
# KUBE-NODEPORTS   - NodePort 规则
# KUBE-MARK-MASQ   - 标记需要 MASQUERADE 的包
# KUBE-POSTROUTING - MASQUERADE 规则

# 查看 kube-proxy 生成的规则
iptables -t nat -L KUBE-SERVICES -n
iptables -t nat -L KUBE-SVC-XXXX -n
iptables -t nat -L KUBE-SEP-XXXX -n

# 查看 Service 规则数量（衡量 iptables 模式的负担）
iptables -t nat -L KUBE-SERVICES -n | wc -l

# 统计所有 iptables 规则
iptables-save | wc -l

# 查看规则匹配计数
iptables -t nat -L KUBE-SERVICES -n -v
```

## nftables (iptables 的继任者)

```bash
# nftables 优势:
# - 更好的性能（减少规则匹配开销）
# - 更简洁的语法
# - 原子性规则更新
# - 统一的语法（不再区分表/链类型）

# 查看 nftables 规则
nft list ruleset

# 创建表
nft add table inet myfilter

# 创建链
nft add chain inet myfilter input { type filter hook input priority 0 \; policy drop \; }

# 添加规则
nft add rule inet myfilter input tcp dport 22 accept
nft add rule inet myfilter input tcp dport { 80, 443 } accept
nft add rule inet myfilter input ct state established,related accept

# Kubernetes 在较新版本中支持 nftables 模式的 kube-proxy
# kube-proxy --proxy-mode=nftables
```

---

## IPVS 负载均衡

IPVS (IP Virtual Server) 是 Linux 内核内置的四层负载均衡器，性能远优于 iptables 模式。kube-proxy 的 IPVS 模式是大规模 Kubernetes 集群的首选。

## IPVS vs iptables 模式对比

| 特性 | iptables 模式 | IPVS 模式 |
|:---|:---|:---|
| **规则匹配** | 线性遍历 O(n) | 哈希查找 O(1) |
| **性能** | 随 Service 数量下降 | 稳定高性能 |
| **调度算法** | 随机 | rr, wrr, lc, wlc, sh, dh 等 |
| **适用规模** | < 1000 Service | 5000+ Service |
| **依赖模块** | iptables, conntrack | ip_vs, ip_vs_rr 等 |

## IPVS 调度算法

| 算法 | 名称 | 说明 | 适用场景 |
|:---|:---|:---|:---|
| **rr** | Round-Robin | 轮询 | 通用 |
| **wrr** | Weighted Round-Robin | 加权轮询 | 后端性能不均 |
| **lc** | Least-Connection | 最少连接 | 长连接 |
| **wlc** | Weighted Least-Connection | 加权最少连接 | 通用推荐 |
| **sh** | Source Hashing | 源地址哈希 | 会话保持 |
| **dh** | Destination Hashing | 目标地址哈希 | 缓存服务器 |

## IPVS 配置

```bash
# 加载 IPVS 模块
modprobe ip_vs
modprobe ip_vs_rr
modprobe ip_vs_wrr
modprobe ip_vs_sh
modprobe nf_conntrack

# 永久加载
cat > /etc/modules-load.d/ipvs.conf << 'EOF'
ip_vs
ip_vs_rr
ip_vs_wrr
ip_vs_sh
nf_conntrack
EOF

# 查看 IPVS 规则
ipvsadm -Ln                    # 列出所有虚拟服务
ipvsadm -Ln --stats            # 显示统计信息
ipvsadm -Ln -t <vip>:<port>    # 查看特定服务

# 查看 IPVS 连接
ipvsadm -Lnc                   # 当前连接
ipvsadm -Lnc --sort            # 排序

# Kubernetes 中 IPVS 模式验证
# 在 kube-proxy 配置中:
# mode: "ipvs"
# ipvs:
#   scheduler: "wlc"

# 验证 kube-proxy 模式
curl http://localhost:10249/proxyMode
```

---

## 隧道技术

隧道技术用于在不同主机上的 Pod 之间建立网络通信，是 Kubernetes 跨节点网络的基础。

## VXLAN (Virtual eXtensible LAN)

```
┌────────────────────────────────────────────────────────────────┐
│                    节点 A (10.0.0.1)                            │
│                                                                 │
│  ┌──────────┐    ┌──────────────┐    ┌──────────┐             │
│  │  Pod 1   │────│   br0 网桥    │────│ flannel.1│             │
│  │10.244.1.2│    │  10.244.1.0/24│    │ (VXLAN   │             │
│  └──────────┘    └──────────────┘    │  VNI=1)  │             │
│                                       └────┬─────┘             │
│                                            │                    │
│                                       UDP 封装                 │
│                                       (端口 4789)              │
└────────────────────────────────────────────┬────────────────────┘
                                             │
                                    物理网络 (Overlay)
                                             │
┌────────────────────────────────────────────┬────────────────────┐
│                    节点 B (10.0.0.2)       │                    │
│                                       UDP 解封                 │
│                                       ┌────┴─────┐             │
│  ┌──────────┐    ┌──────────────┐    │ flannel.1│             │
│  │  Pod 2   │────│   br0 网桥    │────│ (VXLAN   │             │
│  │10.244.2.2│    │  10.244.2.0/24│    │  VNI=1)  │             │
│  └──────────┘    └──────────────┘    └──────────┘             │
└────────────────────────────────────────────────────────────────┘

原始包: [Ethernet][IP: 10.244.1.2 → 10.244.2.2][Payload]
VXLAN包: [Ethernet][IP: 10.0.0.1 → 10.0.0.2][UDP:4789][VXLAN Header][原始包]
```

```bash
# 创建 VXLAN 接口
ip link add vxlan0 type vxlan id 1 dstport 4789 remote 10.0.0.2 local 10.0.0.1 dev eth0

# 配置 VXLAN
ip addr add 10.244.1.1/24 dev vxlan0
ip link set vxlan0 up

# 查看 VXLAN 转发表
bridge fdb show dev vxlan0
```

---

<!-- chunk: 常用命令参考 -->## 常用命令参考

## 网络配置命令

```bash
# ip 命令 (现代推荐)
ip addr show                    # 查看所有地址
ip addr add 192.168.1.100/24 dev eth0    # 添加地址
ip addr del 192.168.1.100/24 dev eth0    # 删除地址
ip link show                    # 查看接口
ip link set eth0 up             # 启用接口
ip link set eth0 down           # 禁用接口
ip -s link show eth0            # 查看统计
ip -stats link show eth0        # 详细统计

# 路由管理
ip route show                   # 查看路由表
ip route add 10.0.0.0/8 via 192.168.1.1  # 添加路由
ip route add default via 192.168.1.1      # 默认路由
ip route del 10.0.0.0/8        # 删除路由
ip route get 8.8.8.8           # 查看路由决策

# 邻居表 (ARP)
ip neigh show                   # 查看 ARP 表
ip neigh add 192.168.1.1 lladdr aa:bb:cc:dd:ee:ff dev eth0
ip neigh del 192.168.1.1 dev eth0

# 网络命名空间
ip netns list                   # 列出
ip netns add ns1                # 创建
ip netns del ns1                # 删除
ip netns exec ns1 <command>     # 执行命令
ip netns identify <pid>         # 查看进程所属命名空间
```

## 连接与端口查看

```bash
# ss 命令 (推荐替代 netstat)
ss -tlnp                        # TCP 监听端口
ss -tuln                        # TCP+UDP 监听
ss -tnp                         # TCP 已建立连接
ss -t state established         # 按状态过滤
ss -t state time-wait           # TIME_WAIT 连接
ss -s                           # 连接统计摘要
ss -t dst :80                   # 目标端口 80
ss -t src :80                   # 源端口 80

# | 选项 | 说明 |
# |:---|:---|
# | `-t` | TCP 连接 |
# | `-u` | UDP 连接 |
# | `-l` | 仅监听 |
# | `-n` | 不解析名称 |
# | `-p` | 显示进程 |
# | `-a` | 所有连接 |
# | `-e` | 扩展信息 |
# | `-m` | 内存使用 |
# | `-i` | TCP 内部信息 |
```

## 网络诊断工具

```bash
# 连通性测试
ping -c 4 8.8.8.8               # ICMP 测试
ping -c 4 -M do -s 1400 8.8.8.8 # MTU 测试 (1400 + 28 = 1428)
traceroute 8.8.8.8              # 路由追踪
mtr 8.8.8.8                     # 持续路由追踪
nc -zv host 80                  # TCP 端口测试
nc -zuv host 53                 # UDP 端口测试

# DNS 排查
nslookup domain.com             # 简单查询
dig domain.com                  # 详细查询
dig @8.8.8.8 domain.com         # 指定 DNS 服务器
dig +trace domain.com           # 完整追踪
dig domain.com CNAME            # 查询特定记录类型
host domain.com                 # 简洁查询

# 抓包分析
tcpdump -i eth0                 # 抓取所有包
tcpdump -i eth0 port 80         # 按端口过滤
tcpdump -i eth0 host 192.168.1.1 # 按主机过滤
tcpdump -i eth0 -w capture.pcap # 保存到文件
tcpdump -i eth0 -nn -vvv        # 不解析名称，详细输出
tcpdump -i eth0 tcp[tcpflags] & (tcp-syn) != 0  # 仅 SYN 包
tcpdump -i eth0 'port 53'       # DNS 查询

# 接口统计
ifstat                          # 接口流量
sar -n DEV 1                    # 网络设备统计
cat /proc/net/dev               # 接口统计
ethtool -S eth0                 # 网卡详细统计
```

---

<!-- chunk: 性能调优 -->## 性能调优

## 内核网络参数优化

```bash
# /etc/sysctl.d/99-network-tuning.conf

# ===== TCP 缓冲区 =====
net.core.rmem_max = 16777216            # 最大接收缓冲区 (16MB)
net.core.wmem_max = 16777216            # 最大发送缓冲区 (16MB)
net.ipv4.tcp_rmem = 4096 87380 16777216 # TCP 接收缓冲区 (min/default/max)
net.ipv4.tcp_wmem = 4096 65536 16777216 # TCP 发送缓冲区 (min/default/max)
net.core.optmem_max = 65536             # 每个套接字的选项内存

# ===== 连接队列 =====
net.core.somaxconn = 65535              # listen() backlog 最大值
net.core.netdev_max_backlog = 65535     # 网络设备积压队列
net.ipv4.tcp_max_syn_backlog = 65535    # SYN 队列大小

# ===== TIME_WAIT 优化 =====
net.ipv4.tcp_fin_timeout = 15           # FIN-WAIT-2 超时
net.ipv4.tcp_tw_reuse = 1               # 允许重用 TIME_WAIT
net.ipv4.tcp_max_tw_buckets = 65535     # TIME_WAIT 最大数量

# ===== TCP 保活 =====
net.ipv4.tcp_keepalive_time = 600       # 保活探测间隔
net.ipv4.tcp_keepalive_probes = 3       # 保活探测次数
net.ipv4.tcp_keepalive_intvl = 15       # 保活探测间隔

# ===== TCP 性能 =====
net.ipv4.tcp_slow_start_after_idle = 0  # 禁用空闲后慢启动
net.ipv4.tcp_no_metrics_save = 1        # 不缓存 TCP 指标
net.ipv4.tcp_mtu_probing = 1            # 启用 MTU 探测
net.ipv4.tcp_window_scaling = 1         # 启用窗口缩放
net.ipv4.tcp_sack = 1                   # 启用选择性确认
net.ipv4.tcp_fack = 1                   # 启用转发确认
net.ipv4.tcp_timestamps = 1             # 启用时间戳

# ===== IP 转发 (Kubernetes 必须) =====
net.ipv4.ip_forward = 1
net.ipv4.conf.all.forwarding = 1
net.ipv6.conf.all.forwarding = 1

# ===== conntrack (连接跟踪) =====
net.netfilter.nf_conntrack_max = 1048576            # 最大跟踪连接数
net.netfilter.nf_conntrack_tcp_timeout_established = 86400  # 已建立连接超时
net.netfilter.nf_conntrack_tcp_timeout_close_wait = 60      # CLOSE_WAIT 超时
net.netfilter.nf_conntrack_tcp_timeout_time_wait = 30       # TIME_WAIT 超时

# 应用配置
sysctl --system
```

## 网卡优化

```bash
# 查看网卡信息
ethtool eth0                    # 网卡信息
ethtool -i eth0                 # 驱动信息
ethtool -k eth0                 # 卸载特性
ethtool -S eth0                 # 统计信息

# 设置 Ring Buffer（增加缓冲减少丢包）
ethtool -g eth0                 # 查看当前 Ring Buffer
ethtool -G eth0 rx 4096 tx 4096 # 设置 Ring Buffer 大小

# 启用硬件卸载
ethtool -K eth0 tso on          # TCP Segmentation Offload
ethtool -K eth0 gso on          # Generic Segmentation Offload
ethtool -K eth0 gro on          # Generic Receive Offload
ethtool -K eth0 lro on          # Large Receive Offload

# 中断亲和性 (多队列网卡)
ethtool -l eth0                 # 查看队列数
ethtool -L eth0 combined 8      # 设置队列数
# 设置 IRQ 亲和性
set_irq_affinity eth0

# 网卡通道绑定
ethtool -L eth0 combined 4      # 4 个组合通道
```

## 连接跟踪调优

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
# 查看当前连接跟踪数
cat /proc/sys/net/netfilter/nf_conntrack_count
cat /proc/sys/net/netfilter/nf_conntrack_max

# 连接跟踪使用率
echo "scale=2; $(cat /proc/sys/net/netfilter/nf_conntrack_count) * 100 / $(cat /proc/sys/net/netfilter/nf_conntrack_max)" | bc

# 如果使用率 > 80%，需要增加 conntrack_max
sysctl -w net.netfilter.nf_conntrack_max=2097152

# 查看连接跟踪表
conntrack -L | head
conntrack -C                   # 统计

# 清理连接跟踪表
conntrack -F
```

---

<!-- chunk: 安全加固 -->## 安全加固

## 防火墙安全配置

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `iptables -F/-P DROP`：清空/改防火墙规则，可能立即断网(含SSH)

```bash
# iptables 生产环境安全基线
# 1. 默认拒绝所有入站
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT ACCEPT

# 2. 允许已建立连接
iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT

# 3. 允许本地回环
iptables -A INPUT -i lo -j ACCEPT

# 4. 允许 SSH (建议限制来源 IP)
iptables -A INPUT -p tcp --dport 22 -s 10.0.0.0/8 -j ACCEPT

# 5. 允许 ICMP (ping)
iptables -A INPUT -p icmp --icmp-type echo-request -m limit --limit 1/s -j ACCEPT

# 6. 允许 Kubernetes 必要端口
iptables -A INPUT -p tcp --dport 6443 -j ACCEPT   # API Server
iptables -A INPUT -p tcp --dport 10250 -j ACCEPT   # Kubelet
iptables -A INPUT -p tcp --dport 30000:32767 -j ACCEPT  # NodePort

# 7. 记录被拒绝的包
iptables -A INPUT -j LOG --log-prefix "IPTables-Dropped: " --log-level 4

# 保存规则
iptables-save > /etc/iptables/rules.v4
```

## 网络安全内核参数

```bash
# /etc/sysctl.d/99-network-security.conf

# 禁用 ICMP 重定向
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0
net.ipv4.conf.default.accept_redirects = 0
net.ipv4.conf.default.send_redirects = 0
net.ipv6.conf.all.accept_redirects = 0

# 禁用源路由
net.ipv4.conf.all.accept_source_route = 0
net.ipv4.conf.default.accept_source_route = 0

# 启用反向路径过滤
net.ipv4.conf.all.rp_filter = 1
net.ipv4.conf.default.rp_filter = 1

# 记录 Martian 包
net.ipv4.conf.all.log_martians = 1
net.ipv4.conf.default.log_martians = 1

# SYN Flood 防护
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.tcp_synack_retries = 2

# 禁用 ICMP 广播响应
net.ipv4.icmp_echo_ignore_broadcasts = 1

# 忽略 ICMP 错误响应
net.ipv4.icmp_ignore_bogus_error_responses = 1

sysctl --system
```

---

<!-- chunk: 与 Kubernetes 的关系 -->## 与 Kubernetes 的关系

## kube-proxy 网络模式

Kubernetes 的 kube-proxy 组件负责实现 Service 的负载均衡，它支持三种模式：

| 模式 | 实现 | 性能 | 适用场景 |
|:---|:---|:---|:---|
| **userspace** | 用户空间代理 | 最差 | 已弃用 |
| **iptables** | iptables 规则 | 中等 | 小规模集群 |
| **IPVS** | 内核 IPVS | 最好 | 生产环境推荐 |
| **nftables** | nftables 规则 | 好 | Kubernetes 1.29+ |

## CNI 插件底层技术

| CNI 插件 | 底层技术 | 网络模式 | 适用场景 |
|:---|:---|:---|:---|
| **Flannel** | VXLAN/host-gw | Overlay/路由 | 简单部署 |
| **Calico** | BGP/VXLAN/eBPF | 路由/Overlay | 高性能、网络策略 |
| **Cilium** | eBPF | eBPF 直通 | 高性能、可观测性 |
| **Weave** | VXLAN/Sleeve | Overlay | 简单跨云 |
| **kube-router** | BPF/ipvs | 路由 | 轻量级 |

```bash
# 在节点上排查 CNI 网络
# 查看节点上的 veth 设备
ip link show type veth

# 查看网桥
bridge link show
bridge fdb show

# 查看 Pod 的网络命名空间
crictl inspect <container_id> | jq .info.pid
nsenter --target <pid> --net ip addr
nsenter --target <pid> --net ip route
nsenter --target <pid> --net iptables -t nat -L -n

# 查看节点路由（Calico/BGP 模式）
ip route show | grep -E "10.244|bird"
```

---

<!-- chunk: 最佳实践 -->## 最佳实践

1. **生产环境使用 IPVS 模式**: 集群规模超过 100 个 Service 时，IPVS 模式性能显著优于 iptables
2. **合理规划 CIDR**: 确保 Pod CIDR 和 Service CIDR 不与现有网络冲突
3. **启用 conntrack 调优**: 大规模集群中 conntrack 表容易耗尽
4. **监控网络指标**: 使用 Prometheus + Cilium/Hubble 进行网络可观测性
5. **限制 NodePort 范围**: 使用 `--service-node-port-range` 限制端口范围
6. **使用 NetworkPolicy**: 通过网络策略实现 Pod 间访问控制

---

<!-- chunk: 故障排查 -->## 故障排查

## 网络故障诊断流程

```bash
# 1. 检查物理层
ethtool eth0 | grep "Link detected"      # 链路状态
ethtool -S eth0 | grep -i error          # 网卡错误计数

# 2. 检查 IP 层
ip addr show                             # 地址配置
ip route show                            # 路由表
ping -c 4 <gateway>                      # 网关可达性

# 3. 检查 DNS
dig <service>.<namespace>.svc.cluster.local  # K8s Service DNS
dig @<coredns-ip> <domain>               # 指定 CoreDNS

# 4. 检查连接
ss -tlnp | grep <port>                   # 端口监听
ss -tnp | grep <ip>                      # 连接状态

# 5. 检查防火墙/iptables
iptables -t nat -L -n -v                 # NAT 规则
iptables -L -n -v                        # filter 规则
conntrack -L | wc -l                     # 连接跟踪数

# 6. 抓包分析
tcpdump -i any port <port> -nn -vvv      # 抓包
tcpdump -i any host <ip> -w /tmp/cap.pcap # 保存分析

# 7. Kubernetes 网络排查
kubectl get pods -n kube-system          # CNI Pod 状态
kubectl logs -n kube-system <cni-pod>    # CNI 日志
kubectl get endpoints <service>          # Endpoints
kubectl describe service <service>       # Service 详情
```

---

## 常见网络问题场景

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 场景 1: Pod 无法访问 Service
# 排查步骤:
kubectl get endpoints <service>      # 1. 检查 Endpoints 是否存在
kubectl exec -it <pod> -- curl <svc-ip>:<port>  # 2. 从 Pod 内测试
iptables -t nat -L KUBE-SVC-XXX -n -v  # 3. 检查 iptables 规则
ipvsadm -Ln -t <vip>:<port>          # 4. 检查 IPVS 规则（如使用 IPVS）

# 场景 2: 跨节点 Pod 无法通信
# 排查步骤:
ip route show | grep -E "10.244|host" # 1. 检查节点路由
bridge fdb show                       # 2. 检查 MAC 转发表
tcpdump -i any host <remote-pod-ip>   # 3. 抓包分析
# 4. 检查 CNI 插件状态和配置

# 场景 3: DNS 解析失败
# 排查步骤:
kubectl get pods -n kube-system -l k8s-app=kube-dns  # 1. CoreDNS 状态
kubectl logs -n kube-system <coredns-pod>             # 2. CoreDNS 日志
dig @<coredns-ip> <service>.<namespace>.svc.cluster.local  # 3. 直接查询
nsenter --target <pid> --net dig kubernetes.default.svc.cluster.local  # 4. 从 Pod 网络空间

# 场景 4: conntrack 表满
# 症状: 新连接无法建立，dmesg 出现 "nf_conntrack: table full, dropping packet"
cat /proc/sys/net/netfilter/nf_conntrack_count     # 当前连接数
cat /proc/sys/net/netfilter/nf_conntrack_max        # 最大值
# 临时修复:
sysctl -w net.netfilter.nf_conntrack_max=2097152
# 永久修复: 写入 /etc/sysctl.d/
```

---

<!-- chunk: 相关文档 -->## 相关文档

- [01-linux-system-architecture](./01-linux-system-architecture.md) - 系统架构
- [06-linux-performance-tuning](./06-linux-performance-tuning.md) - 性能调优
- [08-linux-container-fundamentals](./08-linux-container-fundamentals.md) - 容器基础

---

**维护者**: Allen Galler (allengaller@gmail.com) | **许可证**: MIT

## See Also

- 02-linux-process-management
- 03-linux-filesystem-deep-dive
- 05-linux-storage-management
- 06-linux-performance-tuning
