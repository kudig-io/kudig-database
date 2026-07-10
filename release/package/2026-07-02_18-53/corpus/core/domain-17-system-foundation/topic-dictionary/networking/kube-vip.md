---
title: kube-vip 虚拟 IP
description: kube-vip 为 Kubernetes 集群提供虚拟 IP（VIP）和负载均衡能力，用于控制面高可用（API Server VIP）和
  Service 的 ...
summary: kube-vip 为 Kubernetes 集群提供虚拟 IP（VIP）和负载均衡能力，用于控制面高可用（API Server VIP）和 Service
  的 ...
category: dictionary
tags:
- k8s
- glossary
- networking
- ha
- vip
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-vip 虚拟 IP 是什么
- kube-vip 详解
trigger_keywords:
- kube-vip 虚拟 IP
- kube-vip
- dictionary
prerequisites:
- kubernetes
---



# kube-vip 虚拟 IP（kube-vip）

## 概述

kube-vip 为 Kubernetes 集群提供虚拟 IP（VIP）和负载均衡能力，用于控制面高可用（API Server VIP）和 Service 的 LoadBalancer 类型实现，无需外部负载均衡器。

## 核心概念/原理

- **虚拟 IP**：通过 ARP/NDP 或 BGP 广播 VIP
- **控制面 HA**：为 kubeadm 集群提供 API Server 高可用 VIP
- **Service LB**：实现 Service Type LoadBalancer（裸金属/本地环境）
- **轻量部署**：静态 Pod 或 DaemonSet 方式运行

## 关键机制或特性

- ARP 模式（L2 局域网 VIP 漂移）
- BGP 模式（L3 路由宣告，适合大规模）
- Leader Election 确保单活 VIP
- Service 自动检测（监控 LoadBalancer 类型 Service）
- 等价路由（ECMP）负载均衡
- 支持 IPVS 内核级负载均衡

## 使用场景与最佳实践

- kubeadm 集群的控制面高可用
- 裸金属/边缘环境的 LoadBalancer 实现
- 替代 MetalLB 的轻量方案
- 多集群的入口流量管理
- 无外部 LB 的内部服务暴露

## 参考链接

- https://kube-vip.io/
- https://github.com/kube-vip/kube-vip

## Related

- [[domain-17-system-foundation/知识字典/networking/metallb.md|MetalLB]]
- [[domain-17-system-foundation/知识字典/networking/consul.md|Consul]]
- [[domain-17-system-foundation/知识字典/networking/k8gb.md|K8GB]]
