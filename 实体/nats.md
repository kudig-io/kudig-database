---
title: NATS (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- streaming
- nats
- istio
- opa
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- NATS 是什么
- 如何 NATS
trigger_keywords:
- NATS
prerequisites:
- kubectl-basics
- service-mesh-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# NATS

> **CNCF 状态**: Incubating | **类别**: Streaming | **主要语言**: Go

## 概述

NATS 是一个 CNCF 毕业项目，由 Synadia 公司主导开发，是一个高性能、轻量级的云原生消息系统。它提供发布-订阅（Pub/Sub）、请求-响应（Request-Reply）和队列组（Queue Group）等消息通信模式。NATS 以极低的延迟（亚毫秒级）和极高的吞吐量（每秒数百万消息）著称，是微服务通信和事件驱动架构的理想选择。项目于 2010 年由 Derek Collison 创建，2018 年加入 CNCF，2023 年正式毕业。

## Key Features（核心能力）

- **高性能消息传输**：单节点支持每秒数百万消息，亚毫秒级延迟
- **多通信模式**：支持 Pub/Sub、Request/Reply、Queue Groups 和 JetStream 持久化
- **JetStream 持久化**：内置持久化引擎，支持 At-Least-Once 和 Exactly-Once 语义
- **多租户支持**：通过 Account 隔离实现多租户安全通信
- **Leaf Nodes**：支持边缘节点连接到核心集群，实现混合部署
- **多语言客户端**：提供 Go, Java, Python, JavaScript, Rust, C 等 20+ 语言客户端

## 架构与工作原理

NATS 架构简洁高效：NATS Server 是核心组件，采用类似 router 的设计，不持久化消息（除非启用 JetStream）。客户端通过 TCP 连接 Server，使用简单的文本协议通信。集群模式通过 gossip 协议实现服务发现和路由。JetStream 作为内嵌的持久化层，支持 Streams（消息流）和 Consumers（消费者），提供文件系统或内存存储后端。Leaf Node 允许远程节点以低延迟方式连接到主集群。

## K8s 集成

NATS 可通过 Helm Chart 部署到 Kubernetes，支持 StatefulSet 部署模式实现集群高可用。JetStream 使用 PVC 提供持久化存储。K8s Service 提供 ClusterIP 或 Headless Service 实现服务发现。NATS 与 K8s 的集成包括：通过 ConfigMap 配置集群参数，通过 Secret 管理 TLS 证书和认证凭据，通过 PodDisruptionBudget 保证可用性。

## 生产用例

- **微服务异步通信**：为微服务架构提供高性能的消息总线
- **事件驱动架构**：作为 Event Backbone 支撑事件溯源和 CQRS 模式
- **IoT 数据接入**：利用 Leaf Node 在边缘收集 IoT 设备数据
- **实时数据流**：金融交易、实时分析等低延迟数据流场景

## 安装与快速开始

```bash
helm repo add nats https://nats-io.github.io/k8s/helm/charts/
helm install nats nats/nats -n nats --create-namespace --set jetstream.enabled=true
```

## 对比替代方案

相比 Kafka，NATS 更轻量、延迟更低、运维更简单，但持久化能力不如 Kafka 成熟。相比 RabbitMQ，NATS 性能更高但 AMQP 协议兼容性较弱。JetStream 补齐了持久化短板。

## Related

- [[vineyard]] — Vineyard
- [[distribution]] — Distribution
- [[03-istio-security-hardening]] — [[Istio|Istio]]io 安全加固|Istio 安全加固]]
- [[copa]] — Copa (Copacetic)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- nats
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
