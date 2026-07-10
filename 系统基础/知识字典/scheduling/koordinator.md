---
title: Koordinator 增强调度
description: Koordinator 是阿里巴巴开源的 CNCF Sandbox 项目，提供 Kubernetes 增强调度和资源编排能力，专注于混部（Colocation）...
summary: Koordinator 是阿里巴巴开源的 CNCF Sandbox 项目，提供 Kubernetes 增强调度和资源编排能力，专注于混部（Colocation）...
category: dictionary
tags:
- k8s
- glossary
- scheduling
- qos
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
- Koordinator 增强调度 是什么
- Koordinator 详解
trigger_keywords:
- Koordinator 增强调度
- Koordinator
- dictionary
prerequisites:
- kubernetes
---



# Koordinator 增强调度（Koordinator）

## 概述

Koordinator 是阿里巴巴开源的 CNCF Sandbox 项目，提供 Kubernetes 增强调度和资源编排能力，专注于混部（Colocation）场景下的资源利用率提升和 QoS 保障。

## 核心概念/原理

- **混部调度**：在线服务和离线任务混合部署，提升资源利用率
- **QoS 保障**：精细化的资源隔离和干扰控制
- **设备调度**：GPU/RDMA/FPGA 等异构资源的统一调度
- **CNCF Sandbox**：阿里巴巴主导

## 关键机制或特性

- QoS 动态超卖（Dynamic Resource Overcommitment）
- CPU Burst 和 CFS Burst 弹性调度
- 设备插件（GPU Share / RDMA / FPGA）
- Gang Scheduling 和 Coscheduling
- 弹性配额（ElasticQuota）多级资源管理
- Node Resource Manager 精细资源管控

## 使用场景与最佳实践

- 在线/离线混部提升集群利用率
- GPU 共享和细粒度调度
- 需要严格 QoS 保障的多租户环境
- 大规模集群的资源弹性超卖
- AI 训练与在线服务的资源协同

## 参考链接

- https://koordinator.sh/
- https://github.com/koordinator-sh/koordinator

## Related

- [[系统基础/topic-dictionary/scheduling/volcano.md|Volcano]]
- [[系统基础/topic-dictionary/scheduling/scheduler.md|Scheduler]]
- [[系统基础/topic-dictionary/scheduling/qos.md|QoS]]
