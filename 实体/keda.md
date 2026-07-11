---
title: KEDA (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- orchestration
- keda
- prometheus
- grafana
- jaeger
- helm
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KEDA 是什么
- 如何 KEDA
trigger_keywords:
- KEDA
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- tracing-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KEDA

> **CNCF 状态**: Graduated | **类别**: Orchestration | **主要语言**: Go

## 概述

KEDA（Kubernetes Event-Driven Autoscaling）是一个 CNCF 毕业项目，由 Microsoft 和 Red Hat 联合开发。它是 Kubernetes 的事件驱动自动伸缩组件，扩展了 K8s 原生 HPA 的能力，支持基于外部事件源（Kafka 队列长度、Redis 队列深度、Prometheus 指标、AWS CloudWatch 等）的自动伸缩。KEDA 让无状态应用能够根据实际负载（而非 CPU/内存）进行弹性扩缩，特别适合事件驱动和 Serverless 架构。项目于 2023 年正式从 CNCF 毕业。

## Key Features（核心能力）

- **60+ Scalers**：内置支持 Kafka、RabbitMQ、Redis、AWS SQS、Azure Service Bus、Prometheus 等 60+ 事件源
- **Scale-to-Zero**：支持将 Deployment 缩放到零，真正实现 Serverless
- **ScaledObject CRD**：声明式定义伸缩目标和触发器
- **多触发器组合**：支持多个 Scaler 的 AND/OR 组合条件
- **External Scaler**：通过 gRPC 接口实现自定义 Scaler
- **Identity 支持**：支持 Azure Pod Identity、AWS IRSA 等云认证

## 架构与工作原理

KEDA 由三个核心组件构成：Operator（Controller）管理 ScaledObject 和 ScaledJob CRD 的生命周期；Metrics Adapter 作为 Kubernetes API Aggregation Layer 的扩展，将外部指标暴露为 K8s Custom Metrics；External Scaler 通过 gRPC 接口提供自定义指标源。KEDA Controller 监听 ScaledObject CRD，创建对应的 HPA 和触发器配置，Metrics Adapter 将外部指标转化为 HPA 可用的 Custom Metrics。

## K8s 集成

KEDA 深度集成 Kubernetes HPA 机制。ScaledObject CRD 定义目标 Deployment 和触发条件（Scaler），KEDA Controller 自动创建和管理 HPA 资源。Metrics Adapter 注册到 K8s API Server 的 Aggregation Layer，通过 /apis/external.metrics.k8s.io/ 端点提供外部指标。当队列无消息时，KEDA 将 Deployment replicas 设为 0；有新消息时快速从 0 扩展到 1 再按负载扩展。

## 生产用例

- **消息队列消费者伸缩**：根据 Kafka/RabbitMQ 队列深度自动伸缩消费者实例
- **Serverless 容器**：将普通 K8s Deployment 变为可缩放到零的 Serverless 服务
- **基于自定义指标伸缩**：使用 Prometheus 指标（如 QPS）而非 CPU/内存进行伸缩
- **数据库批处理**：根据待处理任务数动态调整 Worker 数量

## 安装与快速开始

```bash
helm repo add kedacore https://kedacore.github.io/charts
helm install keda kedacore/keda -n keda-system --create-namespace
```

## 对比替代方案

相比 K8s 原生 HPA（仅支持 CPU/内存/自定义指标），KEDA 提供了 60+ 预置事件源和 Scale-to-Zero 能力。相比 Knative Serving（专注 Serverless 平台），KEDA 更轻量且保持 Deployment 原生形态。

## Related

- [[score]] — Score
- [[jaeger]] — Jaeger
- [[helm]] — Helm
- [[cloudevents]] — CloudEvents
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- observability/99-keda-event-driven-autoscaling-guide.md|99-keda-event-driven-autoscaling-guide]]
- keda
- [[实体/cncf-orchestration.md|[[CNCF 编排与应用管理项目全景|CNCF 编排与应用管理项目全景]]]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
