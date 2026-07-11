---
title: Fluentd (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- fluentd
- containerd
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
- Fluentd 是什么
- 如何 Fluentd
trigger_keywords:
- Fluentd
prerequisites:
- kubectl-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Fluentd

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Ruby, C

## 概述

Fluentd 是一个 CNCF 毕业项目，由 Treasure Data（现 Arm Treasure Data）创建，是云原生领域最广泛使用的开源日志采集和处理工具。它提供统一的日志采集层（Unified Logging Layer），能够从多种数据源（文件、HTTP、Syslog、K8s 日志等）采集日志，经过过滤、解析、缓冲后输出到多种目标（Elasticsearch、S3、Kafka、Datadog 等）。Fluentd 拥有超过 800 个插件，覆盖了几乎所有主流数据源和目标系统。

## Key Features（核心能力）

- **统一日志层**：支持 800+ 插件，覆盖几乎所有数据源和输出目标
- **JSON 统一格式**：将所有日志统一为 JSON 格式，便于后续处理
- **高可靠性**：支持基于文件和内存的缓冲机制，防止日志丢失
- **灵活的路由**：通过 Tag 和 Match 实现日志的灵活路由和分发
- **Fluent Bit 集成**：与 Fluent Bit 配合，轻量采集 + 重度处理分层部署
- **高性能**：C 扩展的核心引擎支持高吞吐日志处理

## 架构与工作原理

Fluentd 的事件处理流水线由 Input、Parser、Filter、Buffer、Output 五个阶段组成。Input Plugin 从数据源读取原始日志；Parser 将非结构化日志解析为 JSON；Filter 对日志进行过滤、富化（如添加 K8s 元数据）；Buffer 提供可靠的缓冲机制（文件/内存）；Output Plugin 将处理后的日志发送到目标系统。通过 Event Loop（基于 Cool.io）高效处理并发 I/O。

## K8s 集成

在 Kubernetes 中，Fluentd 通常以 DaemonSet 部署到每个节点，自动采集节点上所有容器的 stdout/stderr 日志（/var/log/containers/）。通过 in_tail 插件读取容器日志文件，通过 kubernetes_metadata_filter 插件富化 Pod/Namespace/Label 元数据。输出通常发送到 Elasticsearch、Loki 或 Kafka。Fluentd ConfigMap 定义采集规则和路由策略。

## 生产用例

- **K8s 集群日志聚合**：统一采集所有容器日志发送到 Elasticsearch/Loki
- **多源日志统一**：将应用日志、Nginx 访问日志、系统日志统一到同一平台
- **日志流处理**：实时过滤敏感信息、解析结构化日志、添加元数据
- **合规日志归档**：将审计日志长期存储到 S3/对象存储满足合规要求

## 安装与快速开始

```bash
helm repo add fluent https://fluent.github.io/helm-charts
helm install fluentd fluent/fluentd -n logging --create-namespace
# 或使用 Fluent Bit 轻量版
helm install fluent-bit fluent/fluent-bit -n logging --create-namespace
```

## 对比替代方案

相比 Logstash（Elastic），Fluentd 更轻量、插件更丰富且支持非 Elastic 后端。相比 Vector（Rust），Fluentd 生态更成熟但性能稍逊。Fluent Bit 是其轻量级版本，适合边缘和资源受限场景。

## Related

- [[06-containerd-observability]] — [[containerd|containerd]]rd 可观测性|containerd 可观测性]]
- [[stacker]] — Stacker
- [[opentelemetry]] — OpenTelemetry
- [[kusionstack]] — KusionStack
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 02-fluentd-enterprise-log-processing
- fluentd
- [[实体/k8s-observability-ecosystem.md|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[概念/bp-observability.md|最佳实践：Observability]] — Cross-reference
- [[技能/k8s-logging-management-guide.md|Kubernetes 日志管理最佳实践]] — Cross-reference
- [[技能/deployment-workload-selection.md|工作负载控制器选型]] — Cross-reference
- [[实体/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
