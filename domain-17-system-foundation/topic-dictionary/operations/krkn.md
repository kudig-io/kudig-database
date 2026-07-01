---
title: Krkn 混沌工程
description: 'Krkn（原 Kraken）是 Red Hat 开源的混沌工程工具，专注于 Kubernetes/OpenShift 的故障注入，支持 Pod/Node/Net...'
category: dictionary
tags:
- k8s
- glossary
- operations
- chaos-engineering
- openshift
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Krkn 混沌工程 是什么
- Krkn 详解
trigger_keywords:
- Krkn 混沌工程
- Krkn
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Krkn 混沌工程（Krkn）

## 概述

Krkn（原 Kraken）是 Red Hat 开源的混沌工程工具，专注于 Kubernetes/OpenShift 的故障注入，支持 Pod/Node/Network/Cloud 等多种故障场景，是 OpenShift 生态的混沌工程首选。

## 核心概念/原理

- **K8s/OpenShift 专注**：深度集成 OpenShift 生态
- **多故障类型**：Pod/Node/Network/Cloud/Time/PVC 故障
- **场景驱动**：YAML 定义混沌场景
- **Red Hat 支持**：OpenShift 测试的核心工具

## 关键机制或特性

- Pod Disruption（删除/重启/网络隔离）
- Node Disruption（关机等）
- Network Chaos（延迟/丢包/DNS 故障）
- Time Skew（时钟偏移）
- Cloud 故障（AWS/Azure/GCP 实例停止）
- 与 Prometheus/Grafana 集成指标

## 使用场景与最佳实践

- OpenShift 集群的弹性验证
- 生产环境的故障演练
- 网络故障的模拟和验证
- 云资源故障的影响评估
- CI/CD 中的弹性测试

## 参考链接

- https://krkn-chaos.dev/
- https://github.com/krkn-chaos/krkn

## Related

- [[domain-17-system-foundation/topic-dictionary/operations/litmus.md|LitmusChaos]]
- [[domain-17-system-foundation/topic-dictionary/operations/chaos-mesh.md|Chaos Mesh]]
- [[domain-17-system-foundation/topic-dictionary/operations/chaos-engineering.md|混沌工程]]
