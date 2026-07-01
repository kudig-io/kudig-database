---
title: Kuberhealthy 合成监控
description: 'Kuberhealthy 是 CNCF Sandbox 项目，在 Kubernetes 上运行合成监控检查（Synthetic Checks），以 Pod 方式...'
category: dictionary
tags:
- k8s
- glossary
- operations
- monitoring
- synthetic
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuberhealthy 合成监控 是什么
- Kuberhealthy 详解
trigger_keywords:
- Kuberhealthy 合成监控
- Kuberhealthy
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Kuberhealthy 合成监控（Kuberhealthy）

## 概述

Kuberhealthy 是 CNCF Sandbox 项目，在 Kubernetes 上运行合成监控检查（Synthetic Checks），以 Pod 方式定期验证集群组件（DNS/API/存储/网络等）的健康状态。

## 核心概念/原理

- **合成监控**：主动探测集群组件健康状态
- **Pod 化检查**：每个检查以 Pod 方式运行
- **CNCF Sandbox**：社区驱动的 K8s 监控工具
- **Prometheus 集成**：标准 metrics 输出

## 关键机制或特性

- KuberhealthyCheck CRD 定义检查任务
- 内置检查（DNS/API Server/Deployment/Pod 状态）
- 自定义检查（任意容器化检查脚本）
- Prometheus metrics 导出
- Grafana Dashboard 集成
- 超时和重试配置
- 告警集成（Alertmanager）

## 使用场景与最佳实践

- K8s 集群的主动健康检查
- DNS/网络/存储的连通性验证
- 升级前后的功能回归测试
- 多集群的统一健康监控
- SLO 验证的自动化检查

## 参考链接

- https://kuberhealthy.github.io/kuberhealthy/
- https://github.com/kuberhealthy/kuberhealthy

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/operations/kube-burner.md|kube-burner]]
- [[domain-17-system-foundation/topic-dictionary/observability/kepler.md|Kepler]]
