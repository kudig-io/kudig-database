---
title: Tremor [entities]
description: '## 概述'
summary: 'Tremor 是一个高性能的事件处理引擎，专为处理大规模数据流（日志、指标、追踪数据）而设计。它由 Wayfair 开源，用 Rust 实现，通过自定义的查询语言（Troy/Trickle）定义数据管道，支持背压处理、有保证的交付和复杂事件处理。'
category: entities
tags:
- k8s
- cncf
- streaming
- tremor
- argocd
- elasticsearch
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tremor 是什么
- 如何 Tremor
trigger_keywords:
- Tremor
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Tremor

> **CNCF 状态**: Sandbox | **类别**: Streaming | **主要语言**: Rust

## 概述

Tremor 是一个 CNCF 沙箱项目，由 Wayfair 创建，是一个用 Rust 编写的高性能事件处理和路由引擎。它旨在替代 Logstash/Fluentd 等传统事件处理工具，提供更高的吞吐量和更低的资源消耗。Tremor 支持事件源（Source）、处理流水线（Pipeline）和输出目标（Sink）的声明式定义，特别适合日志处理、指标富化、事件路由和流式 ETL 场景。项目完全用 Rust 实现，具有内存安全和零成本抽象优势。

## Key Features（核心能力）

- **Rust 高性能**：基于 Rust 实现，吞吐量是 Logstash 的 10 倍以上
- **声明式流水线**：通过 Tremor Script 和 Trickle SQL 定义事件处理逻辑
- **多协议支持**：支持 Kafka、HTTP、gRPC、Syslog、File、NATS 等
- **Tremor Script**：专用脚本语言，支持事件过滤、变换和路由
- **Trickle SQL**：基于 SQL 的流式查询语言，支持窗口聚合和 JOIN
- **QoS 控制**：内置背压、断路器、重试等质量保障机制

## 架构与工作原理

Tremor 架构采用 Source-Pipeline-Sink 模型：Source 从数据源（Kafka、HTTP、Syslog）接收事件；Pipeline 通过 Tremor Script 或 Trickle SQL 对事件进行过滤、变换、聚合和路由；Sink 将处理结果发送到目标系统（Elasticsearch、S3、Kafka）。所有组件通过声明式 YAML 配置连接。Tremor 运行时使用异步事件循环和零拷贝设计实现高吞吐低延迟。

## K8s 集成

Tremor 可作为 DaemonSet 或 Deployment 部署到 Kubernetes。在日志处理场景中，以 DaemonSet 形式运行在每个节点，接收 Fluent Bit 转发的日志，进行富化后发送到 Elasticsearch。通过 ConfigMap 管理处理流水线配置。与 K8s 的集成包括：消费 K8s Events 做告警处理、聚合 Pod 指标做实时分析。

## 生产用例

- **高性能日志处理**：替代 Logstash/Fluentd 做日志聚合和富化
- **实时指标管道**：从 Prometheus 指标流中提取异常并告警
- **事件路由**：根据事件内容将数据路由到不同的下游系统
- **流式 ETL**：实时数据清洗和格式转换

## 安装与快速开始

```bash
# 使用 Docker 部署
docker run -d -v $(pwd)/tremor:/etc/tremor:ro tremorproject/tremor

# Helm 部署到 K8s
helm repo add tremor https://tremor-project.github.io/tremor-helm/
helm install tremor tremor/tremor -n logging --create-namespace
```

## 对比替代方案

相比 Fluentd/Logstash（Ruby/JRuby），Tremor 的 Rust 实现提供数量级的性能提升和更低的资源消耗。相比 Vector（同样 Rust 实现），Tremor 更专注于复杂事件处理而非通用日志管道。

## Related

- [[kaito]] — KAITO
- [[youki]] — youki
- [[easegress]] — Easegress
- [[perses]] — Perses
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tremor
- [[实体/drasi.md|[[Drasi|Drasi]]]]
- observability|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
