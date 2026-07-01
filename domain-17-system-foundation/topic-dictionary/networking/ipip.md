---
title: IPIP
description: 'IPIP（IP-in-IP）是一种网络隧道协议，将一个 IP 数据包封装在另一个 IP 数据包中传输。在 Kubernetes 网络中，IPIP 常用于跨节点 ...'
category: dictionary
tags:
- k8s
- glossary
- ipip
- tunnel
- networking
- cni
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- IPIP 是什么
- IPIP 详解
trigger_keywords:
- IPIP
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# IPIP

> **英文名**: IPIP

## 概述

IPIP（IP-in-IP）是一种网络隧道协议，将一个 IP 数据包封装在另一个 IP 数据包中传输。在 Kubernetes 网络中，IPIP 常用于跨节点 Pod 通信，是 Calico 等 CNI 插件支持的封装模式之一。

## 核心概念/原理

### IPIP 封装原理

```
原始包: [IP Header | Payload (Pod→Pod)]
封装后: [Outer IP Header (Node→Node) | IP Header | Payload]
```

### 与其他隧道协议对比

| 协议 | 封装层 | 开销 | MTU 影响 | 典型使用 |
|------|--------|------|----------|----------|
| IPIP | IP-in-IP | 20 bytes | -20 | Calico IPIP 模式 |
| VXLAN | Ethernet-in-UDP | 50+ bytes | -50 | Calico/Cilium VXLAN |
| Geneve | 类似 VXLAN | 可变 | 可变 | OVN-Kubernetes |

## 关键机制或特性

- IPIP 模式的 MTU 比 VXLAN 小 20 字节（外层 IP 头开销）。
- IPIP 不支持跨子网（不同 L2 域）通信，仅限同子网节点。
- Calico 支持 IPIP Always（所有跨节点流量）和 CrossSubnet（仅跨子网）两种模式。
- IPIP 流量在节点上是 `tunl0` 接口。

## 使用场景与最佳实践

- 同子网集群优先使用 IPIP 模式，开销最小。
- 跨子网或需要 L2 隔离时使用 VXLAN 模式。
- 排查 IPIP 问题时检查 `tunl0` 接口状态和路由表。
- 注意 IPIP 与 IPsec 的兼容性。

## 参考链接

- [Calico IPIP Mode - Project Calico](https://docs.tigera.io/calico/latest/networking/configure-ip-addresses/ipip)

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/vxlan.md|VXLAN]]
- [[domain-17-system-foundation/topic-dictionary/networking/cni.md|CNI]]
- [[domain-17-system-foundation/topic-dictionary/networking/networkpolicy.md|NetworkPolicy]]
- [[domain-17-system-foundation/topic-dictionary/networking/clusterip.md|ClusterIP]]
- [[domain-17-system-foundation/topic-dictionary/networking/nodeport.md|NodePort]]
