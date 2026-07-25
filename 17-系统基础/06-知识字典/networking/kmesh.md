---
title: KMesh 内核级服务网格
description: KMesh 是华为开源的 CNCF Sandbox 项目，基于 eBPF 和可编程硬件在内核态实现服务网格数据面，将 L4 流量管理下沉到内核，显著降低
  Sid...
summary: KMesh 是华为开源的 CNCF Sandbox 项目，基于 eBPF 和可编程硬件在内核态实现服务网格数据面，将 L4 流量管理下沉到内核，显著降低
  Sid...
category: dictionary
tags:
- k8s
- glossary
- networking
- service-mesh
- ebpf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KMesh 内核级服务网格 是什么
- KMesh 详解
trigger_keywords:
- KMesh 内核级服务网格
- KMesh
- dictionary
prerequisites:
- kubernetes
---



# KMesh 内核级服务网格（KMesh）

## 概述

KMesh 是华为开源的 CNCF Sandbox 项目，基于 eBPF 和可编程硬件在内核态实现服务网格数据面，将 L4 流量管理下沉到内核，显著降低 Sidecar 的资源开销和延迟。

## 核心概念/原理

- **内核态数据面**：基于 eBPF 在内核层处理流量
- **无 Sidecar**：消除 Envoy/Istio Sidecar 的资源开销
- **CNCF Sandbox**：华为主导
- **Istio 兼容**：复用 Istio 控制面

## 关键机制或特性

- eBPF 程序在内核态处理 L4 流量
- Waypoint Proxy 模式（L7 用户态处理）
- 兼容 Istio 控制面（xDS API）
- 支持 HTTP/gRPC 流量管理
- 零信任 mTLS 在内核态实现
- 与 Istio Ambient Mesh 互补

## 使用场景与最佳实践

- 资源敏感的服务网格部署
- 需要超低延迟的微服务通信
- Sidecar 开销不可接受的场景
- Istio Ambient 的增强方案
- 大规模集群的服务网格

## 参考链接

- https://kmesh.io/
- https://github.com/kmesh-net/kmesh

## Related

- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
- [[17-系统基础/06-知识字典/networking/cilium.md|Cilium]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
