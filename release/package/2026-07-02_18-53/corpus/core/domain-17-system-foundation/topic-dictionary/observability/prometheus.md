---
title: Prometheus
description: Prometheus 是 CNCF 毕业项目，是 Kubernetes 生态中最主流的监控系统。它采用 Pull 模型采集指标数据，支持强大的
  PromQL 查...
summary: Prometheus 是 CNCF 毕业项目，是 Kubernetes 生态中最主流的监控系统。它采用 Pull 模型采集指标数据，支持强大的 PromQL
  查...
category: dictionary
tags:
- k8s
- glossary
- observability
- prometheus
- monitoring
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Prometheus 是什么
- Prometheus 详解
trigger_keywords:
- Prometheus
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Prometheus

> **英文名**: Prometheus

## 概述

Prometheus 是 CNCF 毕业项目，是 Kubernetes 生态中最主流的监控系统。它采用 Pull 模型采集指标数据，支持强大的 PromQL 查询语言和告警机制。

## 核心概念/原理

### 核心架构

- **Prometheus Server**：采集和存储时间序列指标数据。
- **ServiceMonitor/PodMonitor**：定义采集目标（通过 Prometheus Operator CRD）。
- **Alertmanager**：处理告警规则，发送通知（邮件/Slack/PagerDuty 等）。
- **Grafana**：可视化指标数据的仪表盘工具。
- **PromQL**：Prometheus 查询语言，支持复杂的指标计算。

### 在 Kubernetes 中的集成

- **kube-prometheus-stack**：一键部署 Prometheus + Grafana + Alertmanager 的 Helm Chart。
- **Prometheus Operator**：通过 CRD 声明式管理 Prometheus 实例和采集规则。

## 关键机制或特性

- Prometheus 使用 Pull 模型通过 HTTP 抓取 `/metrics` 端点。
- 指标数据以时间序列存储，每个序列由 metric name + labels 标识。
- 支持 Recording Rules 预计算常用查询。
- Federation 支持多 Prometheus 实例的指标聚合。

## 使用场景与最佳实践

- 生产环境使用 Prometheus Operator 管理 Prometheus 实例。
- 配置合理的 scrape_interval（默认 15s，关键指标可调为 5s）。
- 使用 Recording Rules 优化频繁使用的 PromQL 查询。
- 实施告警分级，避免告警疲劳。

## 参考链接

- [Prometheus - Official Documentation](https://prometheus.io/docs/)

## Related

[[entities/prometheus.md|Prometheus]]


<!-- risk-assessed -->
