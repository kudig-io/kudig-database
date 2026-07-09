---
title: OpenCost 成本监控
description: OpenCost 是 CNCF Sandbox 项目，为 Kubernetes 提供开源的成本分配和监控能力，精确计算每个 Pod/Namespace/Clus...
summary: OpenCost 是 CNCF Sandbox 项目，为 Kubernetes 提供开源的成本分配和监控能力，精确计算每个 Pod/Namespace/Clus...
category: dictionary
tags:
- k8s
- glossary
- observability
- cost
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
- OpenCost 成本监控 是什么
- OpenCost 详解
trigger_keywords:
- OpenCost 成本监控
- OpenCost
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OpenCost 成本监控（OpenCost）

## 概述

OpenCost 是 CNCF Sandbox 项目，为 Kubernetes 提供开源的成本分配和监控能力，精确计算每个 Pod/Namespace/Cluster 的资源成本，帮助企业优化云支出。

## 核心概念/原理

- **成本分配**：将云厂商账单精确拆分到 K8s 资源维度
- **多厂商**：支持 AWS/GCP/Azure/Alibaba 等云厂商
- **CNCF Sandbox**：Kubecost 开源核心
- **Prometheus 集成**：基于 Prometheus 指标计算成本

## 关键机制或特性

- 实时成本分配（Pod/Namespace/Cluster/Label 维度）
- 云厂商价格 API 集成
- 自定义价格表（私有云/裸金属）
- 成本异常检测
- OpenCost UI 可视化看板
- Kubecost API 兼容
- Helm Chart 一键部署

## 使用场景与最佳实践

- Kubernetes 集群的成本可视化
- 多租户环境的成本分摊
- 资源利用率优化
- 云支出预算和告警
- FinOps 实践的底层数据源

## 参考链接

- https://www.opencost.io/
- https://github.com/opencost/opencost

## Related

- [[系统基础/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[系统基础/topic-dictionary/observability/kepler.md|Kepler]]
- [[系统基础/topic-dictionary/observability/grafana.md|Grafana]]


<!-- risk-assessed -->
