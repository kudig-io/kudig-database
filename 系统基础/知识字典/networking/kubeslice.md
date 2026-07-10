---
title: KubeSlice 多集群网络
description: KubeSlice 是 Avesha 开源的 CNCF Sandbox 项目，通过创建跨集群的网络切片（Slice）实现多集群安全隔离的网络互通，无需修改底层
  ...
summary: KubeSlice 是 Avesha 开源的 CNCF Sandbox 项目，通过创建跨集群的网络切片（Slice）实现多集群安全隔离的网络互通，无需修改底层
  ...
category: dictionary
tags:
- k8s
- glossary
- networking
- multi-cluster
- slice
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeSlice 多集群网络 是什么
- KubeSlice 详解
trigger_keywords:
- KubeSlice 多集群网络
- KubeSlice
- dictionary
prerequisites:
- kubernetes
---



# KubeSlice 多集群网络（KubeSlice）

## 概述

KubeSlice 是 Avesha 开源的 CNCF Sandbox 项目，通过创建跨集群的网络切片（Slice）实现多集群安全隔离的网络互通，无需修改底层 CNI 即可打通多个 K8s 集群。

## 核心概念/原理

- **网络切片**：创建跨集群的隔离网络通道（Slice）
- **CNI 无关**：兼容任何底层 CNI 实现
- **安全隔离**：mTLS 加密的跨集群通信
- **CNCF Sandbox**：Avesha 主导

## 关键机制或特性

- Slice CRD 定义跨集群网络切片
- SliceGateway 建立集群间安全隧道
- SliceConfig 定义访问策略
- 支持跨集群 Service 发现
- DNS 集成（跨集群 DNS 解析）
- 带宽限制和流量管理

## 使用场景与最佳实践

- 多集群应用的安全网络互通
- 混合云/多云的网络连接
- 微服务的跨集群部署
- 替代 Submariner 的多集群方案
- 网络隔离要求严格的多租户环境

## 参考链接

- https://kubeslice.io/
- https://github.com/kubeslice/kubeslice-controller

## Related

- [[系统基础/topic-dictionary/networking/submariner.md|Submariner]]
- [[系统基础/topic-dictionary/networking/clusternet.md|Clusternet]]
- [[系统基础/topic-dictionary/networking/k8gb.md|K8GB]]
