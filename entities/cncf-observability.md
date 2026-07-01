---
title: CNCF 可观测性项目全景
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- monitoring
- logging
- tracing
- cost
- prometheus
- grafana
- jaeger
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNCF 可观测性项目全景 是什么
- 如何 CNCF 可观测性项目全景
trigger_keywords:
- CNCF
- 可观测性项目全景
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- kafka-basics
- logging-basics
- tracing-basics
- observability-basics
---



# CNCF 可观测性项目全景

> 聚合页面 | 涵盖 16 个 CNCF 可观测性项目

## 概述

可观测性是云原生架构的核心支柱之一。CNCF 可观测性生态围绕 **指标（Metrics）**、**日志（Logging）**、**追踪（Tracing）** 三大信号构建，辅以成本管理和 AI 驱动的智能运维。

---

## 指标监控（Metrics）

### [[prometheus]] — 毕业项目

Prometheus 是 CNCF 第二个毕业项目，事实上的云原生监控标准。

- **数据模型**: 多维时间序列，基于标签（label）的键值对
- **查询语言**: PromQL 支持丰富的聚合与计算
- **高可用**: [[03-prometheus-ha-deployment|联邦集群与 Thanos/Cortex 方案]]
- **生态集成**: 与 Grafana、Alertmanager 深度集成
- **核心指标**: 拉取（pull）模式采集，服务发现自动感知

### [[thanos]] — 孵化项目

Thanos 为 Prometheus 提供全局查询、长期存储和高可用能力。

- 全局视图跨多个 Prometheus 实例
- 对象存储（S3/GCS/MinIO）作为长期后端
- 降采样查询优化历史数据性能

### [[cortex]] — 孵化项目

Cortex 提供多租户、水平可扩展的 Prometheus 兼容后端。

- 多租户隔离，适合 SaaS 场景
- 块存储 + 对象存储分层架构
- 与 Prometheus 远程写入协议兼容

### [[perses]] — 沙箱项目

Perses 是下一代 Prometheus 仪表板工具，提供声明式仪表板定义。

### [[trickster]] — 沙箱项目

Trickster 是 Prometheus 查询结果的缓存加速代理，降低后端压力。

---

## 日志与事件流（Logging）

### [[fluentd]] — 毕业项目

Fluentd 是统一日志收集层，CNCF 日志采集的事实标准。

- 统一日志格式（JSON），插件生态丰富（500+）
- 支持缓冲、重试、多路输出
- 与 Elasticsearch、S3、Kafka 等后端集成

### [[logging-operator]] — 沙箱项目

Logging Operator 通过 CRD 在 Kubernetes 中声明式管理日志管道。

### [[tremor]] — 沙箱项目

Tremor 是事件流处理引擎，用于日志预处理、路由和聚合。

---

## 分布式追踪（Tracing）

### [[jaeger]] — 毕业项目

Jaeger 是分布式追踪系统，支持 OpenTracing 和 OpenTelemetry。

- 端到端分布式追踪
- 根因分析与依赖拓扑
- 自适应采样策略

### [[opentelemetry]] — 孵化项目

OpenTelemetry（OTel）是统一的可观测性数据采集框架。

- **三大信号**: Metrics、Traces、Logs 统一采集
- **厂商中立**: 导出到 Prometheus/Jaeger/Zipkin 等任意后端
- **自动插桩**: 支持 Java/Go/Python/Node.js 等语言
- 是可观测性领域的未来标准，逐步替代 OpenTracing 和 OpenCensus

---

## 智能运维与成本

### [[opencost]] — 孵化项目

OpenCost 提供 Kubernetes 资源成本监控和分配。

- 按命名空间/标签/工作负载分配成本
- 与 Prometheus 集成，Grafana 可视化
- CNCF 推荐的成本管理标准

### [[kepler]] — 沙箱项目

Kepler（Kubernetes-based Efficient Power Level Exporter）监控 K8s 工作负载能耗。

### [[holmesgpt]] — 沙箱项目

HolmesGPT 利用 AI/LLM 辅助 Kubernetes 故障诊断。

### [[pixie]] — 沙箱项目

Pixie 提供无存储的全栈可观测性，利用 eBPF 实现零侵入数据采集。

---

## 架构选型建议

| 场景 | 推荐方案 |
|---|---|
| 中小规模监控 | Prometheus + Grafana |
| 大规模多集群监控 | Prometheus + Thanos 或 Cortex |
| 统一可观测性 | OpenTelemetry + Prometheus + Jaeger |
| 成本优化 | OpenCost + Prometheus |
| 零侵入采集 | Pixie（eBPF）或 OpenTelemetry 自动插桩 |

---

## 相关页面

- [[entities/cncf-security.md|cncf-security]] — 安全与合规
- [[entities/cncf-storage.md|cncf-storage]] — 存储与数据库
- [[entities/cncf-networking.md|cncf-networking]] — 网络与服务网格
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — K8s 架构

## Related

- [[thanos]] — Thanos
- [[deployment]] — Deployment
- [[cortex]] — Cortex
- [[logging-operator]] — Logging Operator
- [[kubernetes]] — Kubernetes (CNCF Graduated)
