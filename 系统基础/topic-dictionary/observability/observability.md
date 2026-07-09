---
title: 可观测性（Observability）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
- jaeger
tier: core
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 可观测性（Observability） 是什么
- 如何 可观测性（Observability）
trigger_keywords:
- 可观测性
- Observability
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 可观测性（Observability）

## 概述

在 [[Kubernetes|Kubernetes]] 中，可观测性是通过收集和分析**指标（Metrics）**、**日志（Logs）**和**链路追踪（Traces）**——即可观测性的三大支柱——来更好地理解集群的内部状态、性能和健康情况的过程。控制平面组件和许多插件都会生成这些信号，通过聚合和关联它们，可以获得跨集群的统一视图。

## 核心概念/原理

- **指标（Metrics）**：量化的时序数据，用于监控性能、容量和健康状况。
- **日志（Logs）**：按时间顺序记录的事件，用于调试、审计和故障排查。
- **链路追踪（Traces）**：捕获请求在组件和应用之间的流动路径、延迟和依赖关系。
- **统一视图**：将三种信号聚合到统一的存储和分析平台，帮助运维人员和自动化系统做出决策。

## 关键机制或特性

### 指标（Metrics）

Kubernetes 组件以 [[Prometheus|Prometheus]] 格式从 `/metrics` 端点暴露指标，包括：

- kube-controller-manager
- kube-proxy
- kube-apiserver
- kube-scheduler
- [[kubelet|kubelet]]

kubelet 还暴露了 `/metrics/cadvisor`、`/metrics/resource` 和 `/metrics/probes` 端点。插件如 **kube-state-metrics** 可丰富 Kubernetes 对象状态的指标。

典型的指标流水线：

```
集群组件 → Prometheus 抓取器 → 时序数据库存储 → 告警/仪表板/自动化操作
```

常用工具：Prometheus、[[Thanos|Thanos]]、[[Cortex|Cortex]]、Grafana Mimir。

### 日志（Logs）

- **容器日志**：容器运行时通过 CRI 日志格式捕获容器的 `stdout` 和 `stderr`，kubelet 通过 `kubectl logs` 提供访问。
- **系统组件日志**：
  - 运行在容器中的组件（如 kube-scheduler、kube-proxy）写入 `/var/log` 下的 `.log` 文件。
  - 不运行在容器中的组件（如 kubelet、容器运行时）在 systemd 系统上写入 `journald`，否则写入 `/var/log`。
- **集群级日志架构**：通常在每个节点上运行日志代理（如 Fluent Bit、[[fluentd|Fluentd]]），将日志转发到集中式日志存储（如 Elasticsearch、Grafana Loki、OpenSearch）。

### 链路追踪（Traces）

Kubernetes 1.35 支持通过 **OpenTelemetry Protocol (OTLP)** 导出链路追踪数据，可直接通过内置 gRPC exporter 发送，也可通过 OpenTelemetry Collector 转发。

典型的追踪流水线：

```
控制平面/应用 Span → OTLP Exporter → OpenTelemetry Collector → 追踪后端 → 可视化分析
```

常用工具：Jaeger、Grafana Tempo、Zipkin、OpenTelemetry Collector。

## 使用场景

- **性能监控与容量规划**：通过指标了解资源使用趋势，提前规划扩容。
- **故障排查与根因分析**：通过日志和追踪定位异常请求路径和错误来源。
- **安全审计**：收集审计日志，分析 API 访问行为和潜在威胁。
- **自动化运维**：基于指标和日志构建告警和自动修复机制。
- **多集群/多云可视化**：通过分布式时序数据库和集中日志/追踪平台实现跨集群统一视图。

## 最佳实践/注意事项

- 指标、日志和追踪应分别设计存储保留策略，平衡查询需求与存储成本。
- 使用 `metrics-server` 获取资源使用指标，用于 HPA/VPA 等自动扩缩容场景。
- 节点级日志代理建议以 DaemonSet 运行，确保所有节点日志都被收集。
- 系统组件日志需要配置日志轮转，防止磁盘空间耗尽。
- 启用追踪时注意采样率和性能开销，生产环境中通常使用较低的采样率。
- 选择经过社区验证的第三方可观测性工具，并关注其安全更新。

## 参考链接

- [Observability - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/observability/)

## Related

- [[生态参考/topic-index/observability-index.md|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
