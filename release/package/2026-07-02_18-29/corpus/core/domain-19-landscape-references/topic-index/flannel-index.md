---
title: Flannel 知识图谱索引
description: Flannel CNI 网络插件知识图谱，聚合 Flannel 架构、VXLAN/host-gw 模式、故障排查等所有相关内容
summary: Flannel CNI 网络插件知识图谱，聚合 Flannel 架构、VXLAN/host-gw 模式、故障排查等所有相关内容
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flannel 知识图谱索引

> 知识图谱：按关键字 **Flannel** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

- Flannel Complete Guide
- Flannel 网络故障排查

### 进阶专题

- [[domain-03-networking-traffic/00-core-k8s-networking/04a-flannel-wireguard-backend.md|Flannel WireGuard 加密后端配置]]
- [[domain-03-networking-traffic/00-core-k8s-networking/04b-flannel-ipv6-dual-stack.md|Flannel IPv6 Dual Stack 支持]]
- Flannel Windows 节点支持
- Flannel 多集群场景与子网冲突处理
- flanneld 启动参数详解

## 关联文档 (K8s 集成)

### 架构基础

- CNI 容器网络接口深度解析
- Kubernetes 网络模型深度解析

### 故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md|CNI 网络插件故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/03-service-ingress-troubleshooting.md|Service 与 Ingress 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md|DNS 故障排查]]

### FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/flannel-fta.md|Flannel FTA 故障树]]

## 扩展参考

### CNI 生态

- [[domain-02-workloads-applications/00-core-workloads/15-container-runtime-interfaces.md|15 container runtime interfaces]]
- Calico 网络
- Cilium eBPF 网络与安全实践指南

### 培训学习

- Day 25: Flannel CNI
- Flannel Hands-on

<!-- risk-assessed -->
