---
title: OpenYurt 边缘计算
description: OpenYurt 是阿里巴巴开源的 CNCF Sandbox 项目，将 Kubernetes 能力扩展到边缘计算场景，解决云边网络不可靠、边缘自治和大规模边缘节...
summary: OpenYurt 是阿里巴巴开源的 CNCF Sandbox 项目，将 Kubernetes 能力扩展到边缘计算场景，解决云边网络不可靠、边缘自治和大规模边缘节...
category: dictionary
tags:
- k8s
- glossary
- specialized-workloads
- edge
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
- OpenYurt 边缘计算 是什么
- OpenYurt 详解
trigger_keywords:
- OpenYurt 边缘计算
- OpenYurt
- dictionary
prerequisites:
- kubernetes
---



# OpenYurt 边缘计算（OpenYurt）

## 概述

OpenYurt 是阿里巴巴开源的 CNCF Sandbox 项目，将 Kubernetes 能力扩展到边缘计算场景，解决云边网络不可靠、边缘自治和大规模边缘节点管理等挑战。

## 核心概念/原理

- **云边协同**：云端管控 + 边缘自治的混合架构
- **边缘自治**：云边断连时边缘节点独立运行
- **CNCF Sandbox**：阿里巴巴主导
- **无侵入**：对原生 K8s 零修改，渐进式扩展

## 关键机制或特性

- YurtHub：边缘节点代理（缓存 + 自治）
- YurtTunnel：云边安全通信通道
- NodePool：边缘节点池管理
- Raven：跨节点池网络打通
- YurtAppSet：边缘应用分发
- 与 KubeEdge 互补的边缘方案

## 使用场景与最佳实践

- CDN/IoT/零售等边缘场景
- 云边网络不可靠环境的 K8s 管理
- 大规模边缘节点（数千节点）管理
- 边缘应用的统一分发和更新
- 混合云/多云的边缘扩展

## 参考链接

- https://openyurt.io/
- https://github.com/openyurtio/openyurt

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/kubeedge.md|KubeEdge]]
- [[domain-17-system-foundation/topic-dictionary/tooling/k3s.md|K3s]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/karmada.md|Karmada]]
