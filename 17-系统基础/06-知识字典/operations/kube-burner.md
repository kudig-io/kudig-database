---
title: kube-burner 性能测试
description: kube-burner 是 Cloud-Bulldozer 开源的 Kubernetes 性能测试和压力测试工具，通过声明式配置定义测试场景，用于评估
  K8s ...
summary: kube-burner 是 Cloud-Bulldozer 开源的 Kubernetes 性能测试和压力测试工具，通过声明式配置定义测试场景，用于评估
  K8s ...
category: dictionary
tags:
- k8s
- glossary
- operations
- performance
- testing
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-burner 性能测试 是什么
- kube-burner 详解
trigger_keywords:
- kube-burner 性能测试
- kube-burner
- dictionary
prerequisites:
- kubernetes
---



# kube-burner 性能测试（kube-burner）

## 概述

kube-burner 是 Cloud-Bulldozer 开源的 Kubernetes 性能测试和压力测试工具，通过声明式配置定义测试场景，用于评估 K8s 集群的规模性能和调度器行为。

## 核心概念/原理

- **声明式测试**：YAML 定义测试场景（创建/删除/修补资源）
- **大规模模拟**：支持创建数千个 Pod/Deployment 等
- **指标收集**：自动采集 Prometheus 指标和 K8s 事件
- **Cloud-Bulldozer**：Red Hat 性能测试工具集

## 关键机制或特性

- Job 定义测试步骤（Create/Measure/Delete/Patch）
- 模板化资源定义（Go template）
- 内置指标采集（Prometheus/Grafana 集成）
- 并发和速率控制
- OpenShift/Kubernetes 兼容
- 结果导出到 Elasticsearch/本地文件

## 使用场景与最佳实践

- Kubernetes 集群的基准性能测试
- 调度器性能评估和优化
- 大规模集群的容量规划
- 升级前后的性能对比
- CI/CD 中的性能回归测试

## 参考链接

- https://kube-burner.github.io/kube-burner/
- https://github.com/kube-burner/kube-burner

## Related

- [[17-系统基础/06-知识字典/scheduling/scheduler.md|Scheduler]]
- [[17-系统基础/06-知识字典/observability/prometheus.md|Prometheus]]
- [[17-系统基础/06-知识字典/operations/chaos-engineering.md|混沌工程]]
