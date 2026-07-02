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

DNS (typically UDP port 53) resolves names to IP addresses through recursive and iterative queries. In K8s, [[CoreDNS|CoreDNS]] handles in-cluster DNS resolution, translating [[Service|Service]] names to ClusterIPs and Pod names to Pod IPs. DNS failures are a common source of Service connectivity issues.

## Load Balancing Layers

| Layer | Type | Technology | K8s Equivalent |
|-------|------|-----------|----------------|
| L4 (Transport) | Port-based routing | IPVS, LVS | kube-proxy IPVS mode |
| L7 (Application) | Content-based routing | Nginx, HAProxy | [[Ingress|Ingress]] Controller |

Kube-proxy implements Service load balancing through iptables NAT rules (default) or IPVS (high-performance alternative). The conntrack table tracks connection state; a full conntrack table causes Service connectivity failures.

## K8s Critical Network Parameters

- `net.ipv4.ip_forward = 1` (required for Pod cross-node communication)
- `net.bridge.bridge-nf-call-iptables = 1` (required for kube-proxy)
- `net.netfilter.nf_conntrack_max = 1048576` (conntrack table size for large clusters)
- `net.core.somaxconn = 32768` (socket listen queue for high-concurrency Services)

## Related

- [[etcd]] — etcd
- [[concepts/linux-sysctl-tuning.md|linux-sysctl-tuning]] — Linux Sysctl Tuning for Kubernetes
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[cilium]] — Cilium
- [[concepts/linux-sysctl-tuning.md|Linux Sysctl Tuning]]
- [[concepts/service-mesh-architecture.md|Service Mesh Architecture]]
- [[concepts/cilium-ebpf-networking.md|Cilium eBPF Networking]]


<!-- risk-assessed -->
