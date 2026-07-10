---
title: Easegress (entities)
description: '## 概述'
summary: 'Easegress 是一个云原生的全生命周期 API 编排和流量网关，提供高可用、高性能的流量调度能力。它支持丰富的流量治理功能，包括 API 编排、金丝雀发布、限流熔断、服务发现、WebSocket、MQTT 代理等。Easegress 采用过滤器链（Filter Pipeline）架构，用户可以灵活组合过滤器实现复杂的流量处理逻辑。'
category: entities
tags:
- k8s
- cncf
- networking
- easegress
- prometheus
- grafana
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Easegress 是什么
- 如何 Easegress
trigger_keywords:
- Easegress
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Easegress

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Easegress 是一个云原生的全生命周期 API 编排和流量网关，提供高可用、高性能的流量调度能力。它支持丰富的流量治理功能，包括 API 编排、金丝雀发布、限流熔断、服务发现、WebSocket、MQTT 代理等。Easegress 采用过滤器链（Filter Pipeline）架构，用户可以灵活组合过滤器实现复杂的流量处理逻辑。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **过滤器组合**: 按职责拆分过滤器（限流→认证→路由→代理），保持管道清晰
- **集群部署**: 生产环境部署 3+ 节点的 Raft 集群确保高可用
- **Wasm 扩展**: 复杂的自定义逻辑使用 Wasm 过滤器实现，避免修改核心代码
- **健康检查**: 为上游服务器配置主动健康检查
- **监控**: 利用内置的 Prometheus metrics 监控流量和延迟

## 架构定位

在 CNCF 生态中，easegress 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[concepts/controller-pattern.md|controller-pattern]]

## Related

- [[kairos]] — Kairos
- [[kaito]] — KAITO
- [[youki]] — youki
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- easegress
- [[entities/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
