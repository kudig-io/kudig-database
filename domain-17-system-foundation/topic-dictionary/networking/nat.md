---
title: 网络地址转换
description: 'NAT（Network Address Translation，网络地址转换）是将一个 IP 地址和端口映射到另一个的过程。在 Kubernetes 中，NAT...'
category: dictionary
tags:
- k8s
- glossary
- nat
- networking
- kube-proxy
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 网络地址转换 是什么
- NAT (Network Address Translation) 详解
trigger_keywords:
- 网络地址转换
- NAT (Network Address Translation)
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# 网络地址转换

> **英文名**: NAT (Network Address Translation)

## 概述

NAT（Network Address Translation，网络地址转换）是将一个 IP 地址和端口映射到另一个的过程。在 Kubernetes 中，NAT 是 Service 实现流量转发的核心机制，由 kube-proxy 通过 iptables 或 IPVS 规则执行 SNAT 和 DNAT。

## 核心概念/原理

### Kubernetes 中的 NAT 类型

| 类型 | 方向 | 用途 |
|------|------|------|
| DNAT | 入站 | Service ClusterIP → Pod IP |
| SNAT (Masquerade) | 出站 | Pod 访问外部时隐藏源 IP |

### 工作原理

```
Client → Service ClusterIP:Port
       → [kube-proxy DNAT] → Pod IP:Port
Pod → External
       → [Masquerade SNAT] → Node IP → External
```

## 关键机制或特性

- kube-proxy 的 iptables 模式通过 `KUBE-SERVICES` 和 `KUBE-SVC-*` 链实现 DNAT。
- IPVS 模式使用内核 IPVS 模块，性能优于 iptables。
- `externalTrafficPolicy: Local` 保留客户端源 IP（不做 SNAT）。
- `masquerade-all` 配置强制对所有出站流量做 SNAT。

## 使用场景与最佳实践

- 需要保留客户端源 IP 时使用 `externalTrafficPolicy: Local`。
- 大规模集群优先使用 IPVS 模式替代 iptables。
- 排查 NAT 问题时使用 `iptables -t nat -L -n` 检查规则。
- 注意 SNAT 对网络策略和日志的影响（源 IP 变为节点 IP）。

## 参考链接

- [NAT - Wikipedia](https://en.wikipedia.org/wiki/Network_address_translation)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
- [[domain-17-system-foundation/topic-dictionary/networking/clusterip.md|ClusterIP]]
- [[domain-17-system-foundation/topic-dictionary/networking/nodeport.md|NodePort]]
- [[domain-17-system-foundation/topic-dictionary/networking/loadbalancer.md|LoadBalancer]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/kube-proxy.md|Kube-proxy]]
