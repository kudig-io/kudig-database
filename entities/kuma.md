---
title: Kuma (entities)
description: '## 概述'
summary: 'Kuma 是一个通用服务网格控制平面，设计简单易用且功能强大。它基于 Envoy 代理构建，支持 Kubernetes 和虚拟机环境，可通过单一控制平面管理多个服务网格部署。Kuma 提供开箱即用的策略，帮助团队快速实现零信任安全、可观测性和流量管理。'
category: entities
tags:
- k8s
- cncf
- service-mesh
- kuma
- prometheus
- grafana
- jaeger
- envoy
- gateway
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuma 是什么
- 如何 Kuma
trigger_keywords:
- Kuma
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- tracing-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kuma

> **CNCF 状态**: Sandbox | **类别**: [[Service|Service]]Service Mesh）|Service Mesh]] | **主要语言**: Go

## 概述

Kuma 是一个通用服务网格控制平面，设计简单易用且功能强大。它基于 Envoy 代理构建，支持 Kubernetes 和虚拟机环境，可通过单一控制平面管理多个服务网格部署。Kuma 提供开箱即用的策略，帮助团队快速实现零信任安全、可观测性和流量管理。

## 核心能力

- **多平台支持**: 同时支持 Kubernetes 和 VM/裸金属环境
- **多区域部署**: 单一控制平面管理多个集群/区域
- **零信任安全**: 自动 mTLS、访问策略、流量权限
- **可观测性**: 集成 Prometheus、Jaeger、Datadog
- **流量管理**: 负载均衡、熔断、重试、超时
- **Gateway 集成**: 内置 Kong Gateway 支持

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **渐进式启用**: 先在非生产环境测试策略
- **mTLS 优先**: 生产环境始终启用 mTLS
- **多区域规划**: 合理划分区域，减少跨区流量
- **监控覆盖**: 配置完整的可观测性堆栈
- **策略精细化**: 从宽松策略开始，逐步收紧
- **版本管理**: 使用 GitOps 管理策略版本

## 架构定位

在 CNCF 生态中，kuma 属于 **Service Mesh** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]]
- [[concepts/gitops-principles.md|gitops-principles]]
- [[concepts/observability-pillars.md|observability-pillars]]

## Related

- [[kubefleet]] — KubeFleet
- [[envoy]] — Envoy
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[jaeger]] — Jaeger

- kuma
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
