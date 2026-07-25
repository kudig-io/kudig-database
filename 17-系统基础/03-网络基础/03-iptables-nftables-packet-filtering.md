---
title: iptables/nftables 包过滤与 NAT
description: iptables 五表五链、NAT 原理、kube-proxy iptables/IPVS 模式、nftables 新特性、连接跟踪、K8s 网络规则
summary: iptables/nftables 完整知识，覆盖五表五链、NAT、kube-proxy 实现、nftables 迁移、连接跟踪调优
category: knowledge
tags:
- networking
- iptables
- nftables
- nat
- kube-proxy
- firewall
domain: 系统基础
difficulty: advanced
audience:
- SRE
- 平台工程师
- 网络工程师
---

# iptables/nftables 包过滤与 NAT

> iptables/nftables 是 Linux 内核的包过滤框架，是 Kubernetes Service 实现（kube-proxy）的核心机制。深入理解其工作原理是排查 K8s 网络问题的必备技能。

## Netfilter 框架

### 内核网络包处理流程

```
网络包到达
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│                    Netfilter Hooks                            │
│                                                              │
│  PREROUTING → [路由决策] → FORWARD → POSTROUTING → 发出     │
│      │                        ↑                              │
│      ▼                        │                              │
│  [本地进程?] ──→ INPUT → 本地进程 → OUTPUT ──→ POSTROUTING  │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 五个 Hook 点

| Hook | 位置 | 用途 |
|------|------|------|
| PREROUTING | 路由决策前 | DNAT、连接跟踪 |
| INPUT | 送往本地进程前 | 入站过滤 |
| FORWARD | 转发到其他主机 | 转发包过滤 |
| OUTPUT | 本地进程发出 | 出站过滤 |
| POSTROUTING | 路由决策后 | SNAT/MASQUERADE |

## iptables 详解

### 五表五链关系

```
表\链        PREROUTING  INPUT  FORWARD  OUTPUT  POSTROUTING
─────────────────────────────────────────────────────────────
raw            ✓          -       -        ✓         -
mangle         ✓          ✓       ✓        ✓         ✓
nat            ✓          ✓       -        ✓         ✓
filter         -          ✓       ✓        ✓         -
security       -          ✓       ✓        ✓         -
```

### 规则匹配流程

```
包到达 → 匹配表中的链 → 逐条匹配规则 → 执行目标动作
                                    │
                                    ├── ACCEPT: 放行
                                    ├── DROP: 丢弃
                                    ├── REJECT: 拒绝(返回错误)
                                    ├── DNAT: 修改目的地址
                                    ├── SNAT: 修改源地址
                                    ├── MASQUERADE: 动态 SNAT
                                    ├── REDIRECT: 重定向到本机
                                    ├── LOG: 记录日志
                                    ├── MARK: 设置标记
                                    └── RETURN: 返回上级链
```

### iptables 命令语法

```bash
iptables [-t 表] 命令 链 [匹配条件] -j 目标

# 示例
iptables -t nat -A PREROUTING -p tcp --dport 80 -j DNAT --to-destination 10.244.1.5:8080
iptables -t filter -A INPUT -p tcp --dport 22 -j ACCEPT
iptables -t filter -A FORWARD -s 10.244.0.0/16 -j ACCEPT
```

### 常用 iptables 命令

```bash
# 🟢 查看所有规则（带行号和计数）
iptables -L -n -v --line-numbers
iptables -t nat -L -n -v --line-numbers
iptables -t mangle -L -n -v

# 🟢 查看特定链
iptables -t nat -L KUBE-SERVICES -n --line-numbers
iptables -t filter -L INPUT -n -v

# 🟢 查看规则计数（排查流量）
iptables -t nat -L -n -v -x  # -x 显示精确数字

# 🟡 添加规则
iptables -t filter -I INPUT 1 -p tcp --dport 8080 -j ACCEPT

# 🟡 删除规则（按行号）
iptables -t filter -D INPUT 3

# 🔴 清空所有规则（危险！）
iptables -F
iptables -t nat -F
iptables -t mangle -F

# 🟢 保存/恢复规则
iptables-save > /etc/iptables/rules.v4
iptables-restore < /etc/iptables/rules.v4

# 🟢 查看连接跟踪
conntrack -L -p tcp --dport 80
conntrack -L -p udp --dport 53
conntrack -C  # 当前连接数
conntrack -S  # 统计信息
```

## NAT 详解

### NAT 类型

| 类型 | 方向 | 修改内容 | K8s 场景 |
|------|------|----------|----------|
| SNAT | 出站 | 源 IP | Pod 访问外部 |
| DNAT | 入站 | 目的 IP | Service → Pod |
| MASQUERADE | 出站 | 源 IP (动态) | 跨节点 Pod 通信 |
| REDIRECT | 入站 | 目的端口 | 透明代理 |

### kube-proxy iptables 模式

```
Pod 访问 Service (10.96.1.100:80)
    │
    ▼
OUTPUT 链 → KUBE-SERVICES 链
    │ 匹配 dst=10.96.1.100 dport=80
    ▼
KUBE-SVC-XXXX 链 (Service 链)
    │ 概率选择后端 (statistic mode random)
    ├── 1/3 → KUBE-SEP-AAAA (Pod1: 10.244.1.5:8080)
    ├── 1/3 → KUBE-SEP-BBBB (Pod2: 10.244.2.3:8080)
    └── 1/3 → KUBE-SEP-CCCC (Pod3: 10.244.3.7:8080)
    │
    ▼
KUBE-SEP-XXXX 链 (Endpoint 链)
    │ DNAT --to-destination 10.244.1.5:8080
    ▼
POSTROUTING → MASQUERADE (跨节点时)
    │
    ▼
包到达目标 Pod
```

### kube-proxy IPVS 模式

```
Pod 访问 Service (10.96.1.100:80)
    │
    ▼
INPUT 链 → KUBE-SERVICES (匹配后跳到 IPVS)
    │
    ▼
IPVS 虚拟服务器 (10.96.1.100:80)
    │ 负载均衡算法 (rr/wrr/lc/sh)
    ├── RealServer 1: 10.244.1.5:8080
    ├── RealServer 2: 10.244.2.3:8080
    └── RealServer 3: 10.244.3.7:8080
    │
    ▼
DNAT + 转发到目标 Pod
```

### iptables vs IPVS 对比

| 特性 | iptables | IPVS |
|------|----------|------|
| 规则数量 | O(n) Service | O(1) 哈希表 |
| 更新延迟 | 全量更新慢 | 增量更新快 |
| 负载均衡 | 概率(随机) | 6种算法 |
| 大规模集群 | >5000 Service 性能差 | 万级 Service 无压力 |
| 连接限速 | 不支持 | 支持 |
| 健康检查 | 依赖 kube-proxy | 内置 |
| 会话保持 | 不支持 | 支持 (sh/dh) |

```bash
# 查看 IPVS 规则
ipvsadm -Ln
ipvsadm -Ln --stats
ipvsadm -Ln --timeout

# 查看 IPVS 连接
ipvsadm -Lnc
```

## nftables

### nftables vs iptables

| 特性 | iptables | nftables |
|------|----------|----------|
| 用户空间工具 | iptables/ip6tables/arptables | nft (统一) |
| 内核接口 | 每规则一次系统调用 | 批量 netlink |
| 规则查找 | 线性遍历 O(n) | 集合/映射 O(1) |
| 规则更新 | 非原子 | 原子事务 |
| IPv4/IPv6 | 分开管理 | 统一 inet 族 |
| 数据匹配 | 有限 | 集合/位图/字典 |
| 性能(万级规则) | 显著下降 | 几乎无影响 |

### nftables 基本语法

```bash
# 查看规则集
nft list ruleset
nft list table inet filter
nft list chain inet filter input

# 创建表和链
nft add table inet filter
nft add chain inet filter input { type filter hook input priority 0 \; policy drop \; }

# 添加规则
nft add rule inet filter input tcp dport 22 accept
nft add rule inet filter input ip saddr 10.244.0.0/16 accept

# 使用集合（高效匹配）
nft add set inet filter allowed_ports { type inet_service \; }
nft add element inet filter allowed_ports { 80, 443, 8080 }
nft add rule inet filter input tcp dport @allowed_ports accept

# 删除规则（需要 handle）
nft list chain inet filter input --handle
nft delete rule inet filter input handle 3
```

### kube-proxy nftables 模式 (K8s 1.31+)

```yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: "nftables"
nftables:
  masqueradeAll: false
  syncPeriod: 30s
```

**优势：**
- 规则更新原子性（不会半更新导致流量丢失）
- 大规模 Service 性能更好
- 统一 IPv4/IPv6 管理

## 连接跟踪（conntrack）

### 工作原理

```
新包到达 → 查找 conntrack 表
    │
    ├── 已有条目 → 按状态处理 (ESTABLISHED/RELATED)
    │
    └── 无条目 → 创建新条目 (NEW)
              │
              ▼
         记录五元组: (src_ip, dst_ip, src_port, dst_port, protocol)
         记录状态: NEW → ESTABLISHED → 超时删除
```

### 连接状态

| 状态 | 含义 | 超时(默认) |
|------|------|-----------|
| NEW | 新连接第一个包 | 120s (TCP) |
| ESTABLISHED | 双向有包 | 432000s (5天) |
| RELATED | 关联连接(FTP数据) | 同主连接 |
| INVALID | 无法识别 | 立即丢弃 |
| TIME_WAIT | TCP 关闭中 | 120s |
| CLOSE_WAIT | 等待关闭 | 60s |

### conntrack 调优

```bash
# /etc/sysctl.d/99-conntrack.conf

# 最大连接跟踪数（默认 65536，生产建议 100万+）
net.netfilter.nf_conntrack_max = 1048576

# 哈希表大小（通常为 max/4）
echo 262144 > /sys/module/nf_conntrack/parameters/hashsize

# TCP 超时优化
net.netfilter.nf_conntrack_tcp_timeout_established = 3600    # 默认 5天→1小时
net.netfilter.nf_conntrack_tcp_timeout_time_wait = 30       # 默认 120s
net.netfilter.nf_conntrack_tcp_timeout_close_wait = 15      # 默认 60s
net.netfilter.nf_conntrack_tcp_timeout_fin_wait = 30        # 默认 120s
net.netfilter.nf_conntrack_tcp_timeout_syn_sent = 30        # 默认 120s

# UDP 超时（DNS 相关）
net.netfilter.nf_conntrack_udp_timeout = 30                 # 默认 30s
net.netfilter.nf_conntrack_udp_timeout_stream = 120         # 默认 120s
```

### conntrack 故障排查

```bash
# 🟢 查看当前连接数
cat /proc/sys/net/netfilter/nf_conntrack_count
conntrack -C

# 🟢 查看最大值
cat /proc/sys/net/netfilter/nf_conntrack_max

# 🟢 查看连接跟踪表
conntrack -L -p tcp --dport 80 | head -20
conntrack -L -p udp --dport 53 | head -20

# 🟢 查看统计
conntrack -S
cat /proc/net/stat/nf_conntrack

# 🟢 检查表满丢包
dmesg | grep "nf_conntrack: table full"
netstat -s | grep "conntrack"

# 🟡 删除特定连接
conntrack -D -p tcp --dport 80 --dst 10.244.1.5

# 🟢 监控连接数趋势
watch -n 1 'conntrack -C'
```

## K8s 网络规则排查

### Service 不通排查流程

```
1. 检查 Service 和 Endpoints
   kubectl get svc <name> -o yaml
   kubectl get endpoints <name>
       │
2. 检查 iptables 规则
   iptables -t nat -L KUBE-SERVICES -n | grep <cluster-ip>
   iptables -t nat -L KUBE-SVC-XXXX -n -v
       │
3. 检查后端 Pod
   kubectl get pods -l app=<label> -o wide
   curl <pod-ip>:<target-port>
       │
4. 检查 conntrack
   conntrack -L -p tcp --dport <port>
       │
5. 检查 NetworkPolicy
   kubectl get networkpolicy -n <ns>
```

### 常见 iptables 问题

| 问题 | 原因 | 解决 |
|------|------|------|
| Service 无法访问 | Endpoints 为空 | 检查 Pod label/selector |
| 规则未更新 | kube-proxy 异常 | 重启 kube-proxy |
| 规则数过多导致延迟 | 大量 Service | 切换 IPVS/eBPF |
| conntrack 表满 | 连接数超限 | 增大 nf_conntrack_max |
| NAT 不生效 | 包未经过正确链 | 检查路由/策略路由 |
| 旧连接未清除 | conntrack 缓存 | 删除旧 conntrack 条目 |

### 生产案例

#### 案例1：iptables 规则数导致网络延迟

**症状：** 集群 5000+ Service，Pod 网络延迟 P99 > 10ms

**根因：** iptables 线性遍历 15000+ 规则

**解决：** 迁移到 IPVS 模式或 Cilium eBPF

#### 案例2：conntrack 表满导致新连接失败

**症状：** 高峰期部分 Pod 无法建立新连接

**根因：** 默认 65536 连接跟踪数不够

**解决：**
```bash
sysctl -w net.netfilter.nf_conntrack_max=1048576
echo 262144 > /sys/module/nf_conntrack/parameters/hashsize
```

#### 案例3：Service 更新后旧连接不断

**症状：** 滚动更新后，部分流量仍到旧 Pod

**根因：** conntrack 缓存了旧的 DNAT 映射

**解决：**
```bash
# 清除特定 Service 的 conntrack 条目
conntrack -D -p tcp --dport <old-pod-port> --dst <old-pod-ip>
```

## 版本兼容矩阵

| 组件 | 版本 | 变化 |
|------|------|------|
| Linux Kernel | 3.13+ | nftables 可用 |
| Linux Kernel | 5.0+ | nftables 性能优化 |
| Kubernetes | 1.11+ | IPVS kube-proxy GA |
| Kubernetes | 1.31+ | nftables kube-proxy GA |
| Cilium | 1.12+ | eBPF 完全替代 iptables |

## 常见问题 FAQ

**Q1: kube-proxy iptables 和 IPVS 模式如何选择？**
A: Service 数量 < 1000 用 iptables 即可；> 1000 建议 IPVS；> 5000 建议 eBPF (Cilium)。IPVS 支持更多负载均衡算法和会话保持。

**Q2: 为什么 K8s 需要 MASQUERADE？**
A: 当 Pod 访问集群外部时，外部设备不知道如何回包到 Pod IP。MASQUERADE 将源 IP 改为节点 IP，回包经过节点时再 DNAT 回 Pod。

**Q3: iptables 规则更新是原子的吗？**
A: 不是。iptables 每次添加/删除一条规则，中间状态可能导致流量丢失。nftables 支持原子事务更新。

**Q4: conntrack 对 UDP DNS 有什么影响？**
A: UDP 是无连接的，conntrack 为每个 UDP “连接”维护状态。并发 DNS 查询可能导致 conntrack 插入竞态，产生丢包。解决方案：NodeLocal DNSCache。

**Q5: 如何查看某个 Service 的 iptables 规则？**
A: `iptables -t nat -L KUBE-SERVICES -n | grep <cluster-ip>`，然后跟踪到对应的 KUBE-SVC-XXXX 链和 KUBE-SEP-YYYY 链。

**Q6: eBPF 会完全替代 iptables 吗？**
A: 趋势是肯定的。Cilium 已完全用 eBPF 替代 iptables，性能提升 10x+。但过渡期仍需理解 iptables，因为很多集群仍在使用。

## 检查清单

- [ ] 理解 Netfilter 五个 Hook 点
- [ ] 掌握 iptables 五表五链关系
- [ ] 理解 DNAT/SNAT/MASQUERADE 区别
- [ ] 能排查 kube-proxy iptables 规则
- [ ] 掌握 conntrack 调优参数
- [ ] 理解 iptables vs IPVS vs nftables
- [ ] 能排查 Service 不通问题
- [ ] 了解 eBPF 替代 iptables 趋势
- [ ] 掌握 IPVS 负载均衡算法选择
- [ ] 能处理 conntrack 表满紧急故障

## 参考链接

- [[17-系统基础/03-网络基础/index.md|网络基础总索引]]
- [[17-系统基础/03-网络基础/01-tcp-ip-protocol-stack.md|TCP/IP 协议栈]]
- [[17-系统基础/06-知识字典/networking/index.md|网络知识字典]]
- [[17-系统基础/05-速查卡/networking.md|网络速查卡]]
