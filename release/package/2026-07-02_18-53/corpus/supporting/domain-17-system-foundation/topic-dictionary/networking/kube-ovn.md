---
title: Kube-OVN CNI
description: Kube-OVN 是阿里云灵骏开源的 CNCF Sandbox 项目，基于 OVN/OVS 的 Kubernetes CNI 实现，提供企业级的网络功能（静态
  ...
summary: Kube-OVN 是阿里云灵骏开源的 CNCF Sandbox 项目，基于 OVN/OVS 的 Kubernetes CNI 实现，提供企业级的网络功能（静态
  ...
category: dictionary
tags:
- k8s
- glossary
- networking
- cni
- ovn
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kube-OVN CNI 是什么
- Kube-OVN 详解
trigger_keywords:
- Kube-OVN CNI
- Kube-OVN
- dictionary
prerequisites:
- kubernetes
---



# Kube-OVN CNI（Kube-OVN）

## 概述

Kube-OVN 是阿里云灵骏开源的 CNCF Sandbox 项目，基于 OVN/OVS 的 Kubernetes CNI 实现，提供企业级的网络功能（静态 IP/VPC/多子网/安全组等）。

## 核心概念/原理

- **OVN/OVS 数据面**：高性能的虚拟网络
- **企业网络**：VPC/子网/安全组/静态 IP
- **CNCF Sandbox**：阿里云主导
- **多租户网络**：完整的网络隔离能力

## 关键机制或特性

- Subnet CRD（VPC/子网管理）
- 固定 IP（Pod Annotation）
- 安全组（Security Group）
- QoS 带宽限制
- 网络 ACL
- 多网卡支持（Multus）
- DPDK 加速

## 使用场景与最佳实践

- 企业级 K8s 网络方案
- 需要 VPC/固定 IP 的场景
- 多租户网络隔离
- 安全组和 ACL 的精细控制
- 电信/金融行业的网络合规

## 参考链接

- https://kubeovn.github.io/
- https://github.com/kubeovn/kube-ovn

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/ovn-kubernetes.md|OVN-Kubernetes]]
- [[domain-17-system-foundation/topic-dictionary/networking/cilium.md|Cilium]]
- [[domain-17-system-foundation/topic-dictionary/networking/antrea.md|Antrea]]
