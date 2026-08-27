---
title: Network Fundamentals
description: 网络基础知识域 — 协议栈、TCP/UDP、DNS、负载均衡、网络安全、SDN、eBPF/Cilium
category: subdomain
tags:
- networking
- tcp-udp
- dns
- load-balancing
- sdn
- ebpf
tier: core
created: '2026-07-02'
last_updated: '2026-08-25'
---
# 网络基础 Network Fundamentals

> Kubernetes 网络底层原理与基础设施。

## 网络模型层次

| 层次 | 内容 | K8s 关联 |
|------|------|----------|
| L2/L3 | 以太网/IP/路由 | Pod 网络/Node 网络 |
| L4 | TCP/UDP/SCTP | Service/kube-proxy |
| L7 | HTTP/gRPC/DNS | Ingress/Service Mesh |
| 安全 | TLS/防火墙/策略 | NetworkPolicy/mTLS |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[05-网络/02-网络基础/01-network-protocols-stack.md\|协议栈]] | OSI/TCP-IP 模型 | beginner |
| [[05-网络/02-网络基础/02-tcp-udp-deep-dive.md\|TCP/UDP 深度]] | 连接管理/拥塞控制 | intermediate |
| [[05-网络/02-网络基础/03-dns-principles-configuration.md\|DNS 原理]] | DNS 解析与 CoreDNS | intermediate |
| [[05-网络/02-网络基础/04-load-balancing-technologies.md\|负载均衡]] | L4/L7 LB 技术 | intermediate |
| [[05-网络/02-网络基础/05-network-security-fundamentals.md\|网络安全]] | TLS/防火墙/微分段 | intermediate |
| [[05-网络/02-网络基础/06-sdn-network-virtualization.md\|SDN 虚拟化]] | Overlay/VXLAN/eBPF | advanced |
| [[05-网络/02-网络基础/09-cilium-ebpf-network-guide.md\|Cilium eBPF]] | eBPF 网络实践指南 | advanced |

## Related

- [[05-网络/05-eBPF/index.md|eBPF 网络]]
- [[05-网络/03-服务网格/index.md|Service Mesh]]
- [[08-安全/02-网络安全/index.md|网络安全]]
