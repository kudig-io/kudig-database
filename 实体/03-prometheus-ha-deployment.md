---
title: Prometheus 高可用部署 (entities)
description: '# Prometheus 高可用部署'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- 03-prometheus-ha-deployment
- prometheus
- grafana
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Prometheus 高可用部署 是什么
- 如何 Prometheus 高可用部署
trigger_keywords:
- Prometheus
- 高可用部署
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Prometheus 高可用部署

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

Prometheus 高可用部署是关于在生产环境中部署和运维高可用 Prometheus 监控系统的最佳实践。由于 Prometheus 本身不提供原生的集群和高可用能力，需要通过双实例冗余、Thanos/Cortex/Mimir 联邦、AlertManager 集群等组合方案实现监控系统的高可用性。该实践涵盖 Prometheus 实例冗余、长期存储、查询联邦、告警高可用等多个维度。

## Key Features（核心能力）

- **双实例冗余**：部署两个相同配置的 Prometheus 实例独立采集和告警
- **Thanos/Cortex 集成**：通过 Thanos Sidecar 或 Cortex 实现长期存储和全局查询
- **AlertManager 集群**：部署多节点 AlertManager 集群实现告警高可用
- **远程写入**：通过 Remote Write 将指标实时转发到中央存储
- **分区采集**：通过分片（Sharding）将采集目标分配到多个 Prometheus 实例
- **自动故障转移**：通过 Prometheus Operator 实现实例自动恢复

## 架构与工作原理

高可用架构有三种模式：1) 简单冗余模式——两个 Prometheus 独立运行，AlertManager 去重告警；2) Thanos 联邦模式——Thanos Sidecar 上传 Block 到对象存储，Thanos Query 提供全局查询；3) Cortex/Mimir 模式——多副本写入、分布式存储，提供真正的水平扩展。AlertManager 集群通过 Gossip 协议同步告警状态，避免重复通知。

## K8s 集成

通过 Prometheus Operator 部署和管理 Prometheus 实例，StatefulSet 保证实例标识。Thanos 通过 Sidecar 模式注入到 Prometheus Pod，Query Store Gateway 通过 Deployment 部署。使用 K8s PodDisruptionBudget 保证最小可用实例数。Cortex/Mimir 通过微服务模式部署，利用 Ingester、Querier、Compactor 等组件分散负载。

## 生产用例

- **大规模集群监控**：数千节点集群的指标采集和存储
- **长期指标存储**：超过 Prometheus 默认 15 天的长期数据保留需求
- **多集群监控联邦**：统一查询多个集群的监控指标
- **高可用告警**：确保告警系统不因单点故障中断

## 安装与快速开始

```bash
# Prometheus Operator + Thanos
helm install kube-prometheus prometheus-community/kube-prometheus-stack -n monitoring --create-namespace --set prometheus.thanos.create=true
```

## 对比替代方案

相比单实例 Prometheus，双实例+Thanos 方案提供高可用和长期存储但运维复杂度更高。相比 Cortex/Mimir，Thanos 更简单但扩展性稍弱。

## Related

- [[atlantis]] — Atlantis
- [[submariner]] — Submariner
- [[deployment]] — Deployment
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-prometheus-ha-deployment
- [[实体/cncf-observability.md|[[CNCF 可观测性项目全景|CNCF 可观测性项目全景]]]] — Cross-reference


<!-- risk-assessed -->
