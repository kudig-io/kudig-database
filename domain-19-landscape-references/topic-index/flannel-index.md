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
- cilium
- calico
- ingress
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
prerequisites:
- kubectl-basics
- cncf-ecosystem
- ebpf-basics
- cilium-basics
- cni-basics
---

# Flannel 知识图谱索引

> 知识图谱：按关键字 **Flannel** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

- [[domain-03-networking-traffic/04-flannel-complete-guide|Flannel Complete Guide]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/08-flannel-troubleshooting|Flannel 网络故障排查]]

### 进阶专题

- [[domain-03-networking-traffic/04a-flannel-wireguard-backend|Flannel WireGuard 加密后端配置]]
- [[domain-03-networking-traffic/04b-flannel-ipv6-dual-stack|Flannel IPv6 Dual Stack 支持]]
- [[domain-03-networking-traffic/04c-flannel-windows-support|Flannel Windows 节点支持]]
- [[domain-03-networking-traffic/04d-flannel-multi-cluster|Flannel 多集群场景与子网冲突处理]]
- [[domain-03-networking-traffic/04e-flannel-command-reference|flanneld 启动参数详解]]

## 关联文档 (K8s 集成)

### 架构基础

- [[domain-01-cluster-fundamentals/23-container-network-deep-dive|CNI 容器网络接口深度解析]]
- [[domain-03-networking-traffic/01-kubernetes-network-model-deep-dive|Kubernetes 网络模型深度解析]]

### 故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting|CNI 网络插件故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|Service 与 Ingress 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting|DNS 故障排查]]

### FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/flannel-fta|Flannel FTA 故障树]]

## 扩展参考

### CNI 生态

- [[domain-19-landscape-references/incubating/cni/cni|CNI (Container Network Interface)]]
- [[domain-19-landscape-references/sandbox/calico/calico|Calico 网络]]
- [[domain-03-networking-traffic/99-cilium-ebpf-network-guide|Cilium eBPF 网络与安全实践指南]]

### 培训学习

- [[domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-25-flannel-cni|Day 25: Flannel CNI]]
- [[domain-11-production-operations/topic-learn/public-training/week-4-network-storage/day-25-flannel/01-flannel-hands-on|Flannel Hands-on]]