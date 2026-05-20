---
title: Flannel 知识图谱索引
description: Flannel CNI 网络插件知识图谱，聚合 Flannel 架构、VXLAN/host-gw 模式、故障排查等所有相关内容
category: index
tags:
- k8s
- index
- catalog
- flannel
- cni
- network
- vxlan
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Flannel 知识图谱 是什么
- Flannel 网络插件相关内容
trigger_keywords:
- Flannel
- 知识图谱
- CNI
- 网络
- VXLAN
- host-gw
---

# Flannel 知识图谱索引

> 知识图谱：按关键字 **Flannel** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

- [Flannel Complete Guide](./domain-5-networking/04-flannel-complete-guide.md)
- [Flannel 网络故障排查](./topic-structural-trouble-shooting/03-networking/08-flannel-troubleshooting.md)

### 进阶专题

- [Flannel WireGuard 加密后端配置](./domain-5-networking/04a-flannel-wireguard-backend.md)
- [Flannel IPv6 Dual Stack 支持](./domain-5-networking/04b-flannel-ipv6-dual-stack.md)
- [Flannel Windows 节点支持](./domain-5-networking/04c-flannel-windows-support.md)
- [Flannel 多集群场景与子网冲突处理](./domain-5-networking/04d-flannel-multi-cluster.md)
- [flanneld 启动参数详解](./domain-5-networking/04e-flannel-command-reference.md)

## 关联文档 (K8s 集成)

### 架构基础

- [CNI 容器网络接口深度解析](./domain-3-control-plane/23-container-network-deep-dive.md)
- [Kubernetes 网络模型深度解析](./domain-5-networking/01-kubernetes-network-model-deep-dive.md)

### 故障排查

- [CNI 网络插件故障排查指南](./topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md)
- [Service 与 Ingress 故障排查](./topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md)
- [DNS 故障排查](./topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md)

### FTA 故障树

- [Flannel FTA 故障树](./topic-fta/list/flannel-fta.md)

## 扩展参考

### CNI 生态

- [CNI (Container Network Interface)](./domain-34-cncf-landscape/incubating/cni/cni.md)
- [Calico 网络](./domain-34-cncf-landscape/sandbox/calico/calico.md)
- [Cilium eBPF 网络与安全实践指南](./domain-15-network-fundamentals/99-cilium-ebpf-network-guide.md)

### 培训学习

- [Day 25: Flannel CNI](./topic-learn/inner-training/week-4-network-storage/day-25-flannel-cni.md)
- [Flannel Hands-on](./topic-learn/public-training/week-4-network-storage/day-25-flannel/01-flannel-hands-on.md)