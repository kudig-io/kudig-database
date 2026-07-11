---
title: Dapr (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- orchestration
- dapr
- istio
- redis
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
- Dapr 是什么
- 如何 Dapr
trigger_keywords:
- Dapr
prerequisites:
- kubectl-basics
- service-mesh-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Dapr

> **CNCF 状态**: Graduated | **类别**: Orchestration | **主要语言**: Go

## 概述

Dapr（Distributed Application Runtime）是一个 CNCF 毕业项目，由 Microsoft 主导开发。它是一个可移植的、事件驱动的分布式应用运行时，为云原生和边缘应用提供构建块（Building Block）抽象。开发者通过标准 API 调用即可获得服务调用、状态管理、发布订阅、密钥管理、可观测性等分布式系统能力，无需关注底层实现。Dapr 采用 Sidecar 模式，与业务代码解耦，支持 Go, Java, Python, .NET, Node.js 等多语言。项目于 2021 年加入 CNCF，2024 年正式毕业。

## Key Features（核心能力）

- **Building Blocks API**：提供服务调用、State、Pub/Sub、Bindings、Secrets、Actors、Workflow 等标准 API
- **Sidecar 模式**：以 Sidecar 容器运行，与业务代码通过 HTTP/gRPC 通信
- **多语言支持**：通过标准 API 支持 Go, Java, Python, .NET, JS, Rust 等
- **Component 模型**：通过 Component CRD 插入不同的后端（Redis、Kafka、AWS、Azure）
- **可插拔架构**：状态存储、消息总线、密钥存储等组件均可替换
- **Actor 模型**：内置虚拟 Actor 模式实现有状态的并发单元

## 架构与工作原理

Dapr 采用 Sidecar 架构：Dapr Sidecar（daprd）与业务容器运行在同一个 Pod 中，通过 localhost HTTP/gRPC 通信。Sidecar 从 Dapr Operator 获取配置，从 Component CRD 加载各 Building Block 的后端连接信息。Dapr Operator 管理 Sidecar 注入和配置分发；Dapr Sentry 提供 mTLS 证书管理；Dapr Placement Service 管理 Actor 分布。

## K8s 集成

Dapr 在 Kubernetes 中通过 Mutating Webhook 自动将 Sidecar（daprd）注入到标注了 dapr.io/enabled: "true" 的 Pod 中。Component CRD 定义状态存储、消息总线等后端配置。Configuration CRD 控制 Dapr 运行时行为（如 tracing、mTLS）。通过 K8s Service 暴露 Dapr API，应用通过 Dapr App Protocol 与 Sidecar 通信。

## 生产用例

- **微服务通信**：通过 Dapr Service Invocation API 实现服务间 mTLS 调用
- **事件驱动架构**：使用 Dapr Pub/Sub API 连接 Kafka/RabbitMQ 而不绑定具体中间件
- **有状态应用**：通过 State API 使用 Redis/CosmosDB 管理应用状态
- **多语言团队**：不同语言团队共享同一分布式系统能力抽象

## 安装与快速开始

```bash
helm repo add dapr https://dapr.github.io/helm-charts/
helm install dapr dapr/dapr -n dapr-system --create-namespace
# 启用应用 Pod
kubectl annotate deploy myapp dapr.io/enabled=true
```

## 对比替代方案

相比 Service Mesh（Istio/Linkerd，专注网络通信），Dapr 提供更丰富的应用层抽象（State、Actor、Workflow）。相比 Spring Cloud（Java 专用），Dapr 通过 Sidecar 实现语言无关。相比 Knative，Dapr 不替代 K8s 调度，而是在应用层提供能力。

## Related

- [[02-istio-advanced-traffic-management]] — [[Istio|Istio]]io 高级流量管理|Istio 高级流量管理]]
- [[vscode-kubernetes-tools]] — VS Code Kubernetes Tools
- [[litmus]] — LitmusChaos
- [[pixie]] — Pixie
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 05-dapr-enterprise-distributed-runtime
- dapr
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
