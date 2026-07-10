---
title: Network Service Mesh
description: Network Service Mesh（NSM）是 CNCF Sandbox 项目，使用服务网格的概念来管理网络服务（L2/L3 VPN、防火墙、负载均衡等）...
summary: Network Service Mesh（NSM）是 CNCF Sandbox 项目，使用服务网格的概念来管理网络服务（L2/L3 VPN、防火墙、负载均衡等）...
category: dictionary
tags:
- k8s
- glossary
- networking
- multi-cluster
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Network Service Mesh 是什么
- NSM 详解
trigger_keywords:
- Network Service Mesh
- NSM
- dictionary
prerequisites:
- kubernetes
---



# Network Service Mesh（NSM）

## 概述

Network Service Mesh（NSM）是 CNCF Sandbox 项目，使用服务网格的概念来管理网络服务（L2/L3 VPN、防火墙、负载均衡等），将网络功能从硬件解耦到软件定义。

## 核心概念/原理

- **网络服务网格**：将网络功能软件化，按需编排
- **L2/L3 VPN**：跨集群的 L2/L3 网络连接
- **CNCF Sandbox**：活跃的 NFV/SDN 社区
- **与 K8s 集成**：基于 K8s CRD 管理网络服务

## 关键机制或特性

- NetworkService / NetworkServiceEndpoint CRD
- NSMGR（Network Service Mesh Registry）
- Forwarder 数据面（VPP/memif/Kernel）
- 多集群 L2/L3 VPN
- 与 Multus CNI 集成
- 支持 Intel VPP 高性能转发

## 使用场景与最佳实践

- 5G/Telco 的网络功能虚拟化
- 跨集群 L2/L3 VPN 连接
- 传统网络设备的软件化替代
- 多租户网络隔离
- 云原生 NFV 基础设施

## 参考链接

- https://networkservicemesh.io/
- https://github.com/networkservicemesh/networkservicemesh

## Related

- [[domain-17-system-foundation/知识字典/networking/submariner.md|Submariner]]
- [[domain-17-system-foundation/知识字典/networking/cni.md|CNI]]
- [[domain-17-system-foundation/知识字典/networking/loxilb.md|LoxiLB]]
