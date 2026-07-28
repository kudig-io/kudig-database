---
title: TCP/UDP Protocol Stack
description: TCP/UDP Protocol Stack — Kubernetes 生产运维知识库
summary: TCP/UDP Protocol Stack — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- networking
- tcp
- udp
- dns
- load-balancing
- etcd
- cilium
- coredns
- ingress
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- TCP/UDP Protocol Stack 是什么
- 如何 TCP/UDP Protocol Stack
trigger_keywords:
- TCP
- UDP
- Protocol
- Stack
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- etcd-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# TCP/UDP Protocol Stack

## TCP vs UDP Comparison

| Property | TCP | UDP |
|----------|-----|-----|
| Connection | Connection-oriented (3-way handshake) | Connectionless |
| Reliability | Acknowledgment, retransmission, checksum | No guarantee |
| Ordering | Sequence numbers ensure order | No ordering |
| Header Size | 20+ bytes | 8 bytes |
| Flow Control | Sliding window | None |
| Congestion Control | Reno/CUBIC/BBR | None |
| K8s Usage | API Server, etcd, most Services | DNS, some metrics |

## TCP Connection Lifecycle

**Three-Way Handshake**: SYN -> SYN+ACK -> ACK establishes a reliable connection. The TIME_WAIT state (2*MSL duration) ensures stray packets are flushed before port reuse.

**Four-Way Termination**: FIN -> ACK -> FIN -> ACK gracefully closes connections. Excessive TIME_WAIT connections indicate connection churn; CLOSE_WAIT accumulation indicates application-level connection leaks.

Debug TCP states: `ss -s`, `ss -t state established`, `ss -tnp`

## TCP Congestion Control

| Algorithm | Characteristics | Best For |
|-----------|----------------|----------|
| CUBIC | Linux default, cubic growth function | General purpose |
| BBR | Google model-based, estimates bandwidth | High latency/lossy networks |
| Reno | Classic AIMD algorithm | Low latency networks |
| Vegas | Delay-based congestion detection | Low-loss environments |

Check: `sysctl net.ipv4.tcp_congestion_control`

## DNS Resolution Flow

DNS (typically UDP port 53) resolves names to IP addresses through recursive and iterative queries. In K8s, [[coredns|CoreDNS]] handles in-cluster DNS resolution, translating [[service|Service]] names to ClusterIPs and Pod names to Pod IPs. DNS failures are a common source of Service connectivity issues.

## Load Balancing Layers

| Layer | Type | Technology | K8s Equivalent |
|-------|------|-----------|----------------|
| L4 (Transport) | Port-based routing | IPVS, LVS | kube-proxy IPVS mode |
| L7 (Application) | Content-based routing | Nginx, HAProxy | [[ingress\|Ingress]] Controller |

Kube-proxy implements Service load balancing through iptables NAT rules (default) or IPVS (high-performance alternative). The conntrack table tracks connection state; a full conntrack table causes Service connectivity failures.

## K8s Critical Network Parameters

- `net.ipv4.ip_forward = 1` (required for Pod cross-node communication)
- `net.bridge.bridge-nf-call-iptables = 1` (required for kube-proxy)
- `net.netfilter.nf_conntrack_max = 1048576` (conntrack table size for large clusters)
- `net.core.somaxconn = 32768` (socket listen queue for high-concurrency Services)

## 源码实现分析

### kube-proxy IPVS 模式实现

```go
// kubernetes/pkg/proxy/ipvs/proxier.go
func (proxier *Proxier) syncProxyRules() {
    // 1. 遍历所有 Service 和 Endpoints
    for svcName, svc := range proxier.serviceMap {
        // 2. 创建 IPVS 虚拟服务器
        // ipvsadm -A -t <ClusterIP>:<port> -s rr
        proxier.ipvs.AddVirtualServer(&ipvs.VirtualServer{
            Address:  svc.ClusterIP,
            Port:     svc.Port,
            Scheduler: "rr",  // 轮询调度
        })
        // 3. 添加真实服务器（Endpoints）
        for _, ep := range endpoints {
            proxier.ipvs.AddRealServer(&ipvs.RealServer{
                Address: ep.IP,
                Port:    ep.Port,
                Weight:  1,
            })
        }
    }
    // 4. 同步 iptables 规则（SNAT + NodePort）
}
```

### conntrack 与 Service 连接跟踪

```
Client Pod → Service ClusterIP:Port
    │
    ▼
netfilter PREROUTING → conntrack 查找/新建连接跟踪条目
    │
    ▼
DNAT: ClusterIP:Port → Pod IP:TargetPort
    │  (记录在 conntrack 表中，后续包自动转发)
    ▼
目标 Pod 响应 → conntrack 反向 NAT → Client Pod

// conntrack 表满 → 新连接被丢弃 → Service 连接失败
// 检查: conntrack -C  (当前条目数)
// 调整: sysctl net.netfilter.nf_conntrack_max
```

## 使用场景

### 场景一：诊断 TCP 连接状态

```bash
# 🟢 低风险 - 查看 TCP 连接统计
ss -s                          # 总览
ss -tn state time-wait | wc -l # TIME_WAIT 数量
ss -tn state close-wait        # CLOSE_WAIT（应用泄漏）

# 🟢 低风险 - 查看 conntrack 表使用情况
sysctl net.netfilter.nf_conntrack_count
sysctl net.netfilter.nf_conntrack_max
conntrack -C                   # 当前条目数

# 🟢 低风险 - 查看 IPVS 规则
ipvsadm -Ln                    # 所有虚拟服务器
```

### 场景二：优化 TCP 参数（高并发场景）

```bash
# 🟡 中风险 - 调整内核参数
sysctl -w net.core.somaxconn=32768          # 监听队列
sysctl -w net.ipv4.tcp_max_syn_backlog=8096 # SYN 队列
sysctl -w net.ipv4.tcp_tw_reuse=1           # 复用 TIME_WAIT
sysctl -w net.netfilter.nf_conntrack_max=1048576

# 持久化到 /etc/sysctl.d/99-k8s.conf
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| UDP 不需要连接跟踪 | conntrack 也跟踪 UDP（用于 DNS 响应匹配） |
| TIME_WAIT 是问题 | 少量 TIME_WAIT 正常，大量才需优化（tcp_tw_reuse） |
| CLOSE_WAIT 是内核问题 | CLOSE_WAIT 积压是应用未正确关闭连接（代码 bug） |
| kube-proxy 转发所有流量 | kube-proxy 只维护规则，流量不经过它（除 NodePort） |
| Service ClusterIP 可 ping | ClusterIP 是虚拟 IP，只响应已定义的端口，不响应 ICMP |
| BBR 总是比 CUBIC 好 | BBR 在高延迟/丢包网络优势明显，低延迟网络差异不大 |

## 面试要点

1. **TCP 三次握手为什么不是两次？** — 防止历史连接初始化。若只有两次，客户端的过期 SYN 到达服务端后会直接建立连接，浪费资源。第三次 ACK 确认客户端仍然活跃。

2. **kube-proxy iptables vs IPVS 模式区别？** — iptables：规则线性匹配 O(n)，1000+ Service 时延迟显著；IPVS：hash 表查找 O(1)，支持多种调度算法（rr/lc/sh），大规模集群必选。eBPF（Cilium）更进一步，内核层处理。

3. **conntrack 表满的影响和解决？** — 新连接被丢弃，表现为 Service 间歇性连接失败。解决：增大 nf_conntrack_max；缩短 tcp_timeout；使用 Cilium eBPF 替代 conntrack；监控 conntrack 使用率告警。

4. **K8s 中 DNS 解析的完整链路？** — Pod /etc/resolv.conf → CoreDNS Service (ClusterIP) → CoreDNS Pod → 集群内域名直接解析 / 外部域名转发上游 DNS。UDP 53 为主，>512字节切换 TCP。NetworkPolicy 必须放行 DNS。

## Related

- [[etcd]] — etcd
- [[22-概念/15-运行时与系统/linux-sysctl-tuning.md|linux-sysctl-tuning]] — Linux Sysctl Tuning for Kubernetes
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[22-概念/03-网络/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[cilium]] — Cilium
- [[22-概念/15-运行时与系统/linux-sysctl-tuning.md|Linux Sysctl Tuning]]
- [[22-概念/03-网络/service-mesh-architecture.md|Service Mesh Architecture]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|Cilium eBPF Networking]]


<!-- risk-assessed -->
