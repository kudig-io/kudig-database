---
title: Spiderpool IP 池管理
description: 'Spiderpool 是 DaoCloud 开源的 CNCF Sandbox 项目，为 Kubernetes 提供 Underlay 网络的 IP 地址管理（I...'
category: dictionary
tags:
- k8s
- glossary
- networking
- ipam
- cni
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Spiderpool IP 池管理 是什么
- Spiderpool 详解
trigger_keywords:
- Spiderpool IP 池管理
- Spiderpool
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Spiderpool IP 池管理（Spiderpool）

## 概述

Spiderpool 是 DaoCloud 开源的 CNCF Sandbox 项目，为 Kubernetes 提供 Underlay 网络的 IP 地址管理（IPAM），解决容器使用固定 IP 和 Underlay 网络的挑战。

## 核心概念/原理

- **Underlay IPAM**：为 Pod 分配 Underlay 网络的固定 IP
- **多 CNI 兼容**：支持 Macvlan、IPVLAN、SR-IOV、IB SR-IOV
- **CNCF Sandbox**：DaoCloud 主导
- **固定 IP**：支持 Pod 固定 IP 和 IP 池管理

## 关键机制或特性

- SpiderIPPool / SpiderSubnet / SpiderEndpoint CRD
- 固定 IP（Pod Annotation 指定 IP）
- IP 池管理和自动回收
- 多网卡 IPAM（Multus 集成）
- IP 冲突检测和自动修复
- Webhook 验证 IP 合法性
- IPv4/IPv6 双栈支持

## 使用场景与最佳实践

- 需要 Pod 固定 IP 的场景（金融/电信）
- Underlay 网络的 K8s 部署
- 多网卡 Pod 的 IP 管理
- SR-IOV 高性能网络的 IP 分配
- 传统网络环境的 K8s 集成

## 参考链接

- https://spiderpool.dev/
- https://github.com/spidernet-io/spiderpool

## Related

- [[domain-17-system-foundation/topic-dictionary/networking/cni.md|CNI]]
- [[domain-17-system-foundation/topic-dictionary/networking/metallb.md|MetalLB]]
- [[domain-17-system-foundation/topic-dictionary/networking/antrea.md|Antrea]]
