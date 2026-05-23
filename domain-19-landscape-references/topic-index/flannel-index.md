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
created: "2026-05-23"
---

# Flannel 知识图谱索引

> 知识图谱：按关键字 **Flannel** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

- Flannel Complete Guide
- networking/08-flannel-troubleshooting|Flannel 网络故障排查]]

### 进阶专题

- [[domain-03-networking-traffic/00-core-k8s-networking/04a-flannel-wireguard-backend|Flannel WireGuard 加密后端配置]]]]
- [[domain-03-networking-traffic/00-core-k8s-networking/04b-flannel-ipv6-dual-stack|Flannel IPv6 Dual Stack 支持]]]]
- Flannel Windows 节点支持
- Flannel 多集群场景与子网冲突处理
- flanneld 启动参数详解

## 关联文档 (K8s 集成)

### 架构基础

- CNI 容器网络接口深度解析
- Kubernetes 网络模型深度解析

### 故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting|CNI 网络插件故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting|Service 与 Ingress 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting|DNS 故障排查]]

### FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/flannel-fta|Flannel FTA 故障树]]

## 扩展参考

### CNI 生态

- [[domain-02-workloads-applications/00-core-workloads/15-container-runtime-interfaces]]
- Calico 网络
- Cilium eBPF 网络与安全实践指南

### 培训学习

- Day 25: Flannel CNI
- Flannel Hands-on