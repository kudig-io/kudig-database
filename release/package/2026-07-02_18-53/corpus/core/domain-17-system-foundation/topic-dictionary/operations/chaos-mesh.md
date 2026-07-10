---
title: Chaos Mesh 混沌工程平台
description: Chaos Mesh 是 PingCAP 开源并捐赠给 CNCF 的混沌工程平台，提供 Web UI 和声明式 API，支持对 Kubernetes、物理机和云...
summary: Chaos Mesh 是 PingCAP 开源并捐赠给 CNCF 的混沌工程平台，提供 Web UI 和声明式 API，支持对 Kubernetes、物理机和云...
category: dictionary
tags:
- k8s
- glossary
- operations
- chaos-engineering
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
- Chaos Mesh 混沌工程平台 是什么
- Chaos Mesh 详解
trigger_keywords:
- Chaos Mesh 混沌工程平台
- Chaos Mesh
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Chaos Mesh 混沌工程平台（Chaos Mesh）

## 概述

Chaos Mesh 是 PingCAP 开源并捐赠给 CNCF 的混沌工程平台，提供 Web UI 和声明式 API，支持对 Kubernetes、物理机和云环境注入各类故障。

## 核心概念/原理

- **声明式故障注入**：通过 YAML CRD 定义故障类型、目标和持续时间
- **Web Dashboard**：可视化创建和管理混沌实验
- **多平台支持**：Kubernetes、物理机（Chaosd）、AWS/GCP 等
- **CNCF 孵化项目**：PingCAP 主导开发

## 关键机制或特性

- 丰富的故障类型：PodChaos、NetworkChaos、IOChaos、TimeChaos、StressChaos、JVMChaos、HTTPChaos
- 精确的目标选择（Label/Annotation/Namespace 筛选）
- 故障自动恢复和超时保护
- 实验调度（定时/周期性故障注入）
- PhysicalMachineChaos 支持裸金属故障注入
- 与 Prometheus 集成导出实验指标

## 使用场景与最佳实践

- 分布式系统的弹性验证
- 数据库（TiDB 等）的故障注入测试
- 网络分区和延迟模拟
- 定时故障演练（Cron 调度）
- 微服务依赖链的级联故障验证

## 参考链接

- https://chaos-mesh.org/
- https://github.com/chaos-mesh/chaos-mesh

## Related

- [[domain-17-system-foundation/知识字典/operations/litmus.md|LitmusChaos]]
- [[domain-17-system-foundation/知识字典/operations/chaos-engineering.md|混沌工程]]
- [[domain-17-system-foundation/知识字典/observability/prometheus.md|Prometheus]]


<!-- risk-assessed -->
