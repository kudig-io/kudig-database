---
title: Loki
description: Loki 是 Grafana Labs 开源的日志聚合系统，被称为「日志界的 Prometheus」。它采用标签索引（而非全文索引）存储日志，大幅降低存储成本，...
summary: Loki 是 Grafana Labs 开源的日志聚合系统，被称为「日志界的 Prometheus」。它采用标签索引（而非全文索引）存储日志，大幅降低存储成本，...
category: dictionary
tags:
- k8s
- glossary
- loki
- logging
- observability
- grafana
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Loki 是什么
- Loki 详解
trigger_keywords:
- Loki
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Loki

> **英文名**: Loki

## 概述

Loki 是 Grafana Labs 开源的日志聚合系统，被称为「日志界的 Prometheus」。它采用标签索引（而非全文索引）存储日志，大幅降低存储成本，是云原生日志方案的优选。

## 核心概念/原理

### 核心架构

| 组件 | 功能 |
|------|------|
| Distributor | 接收日志流，校验和分发 |
| Ingester | 暂存日志并压缩写入存储 |
| Querier | 执行 LogQL 查询 |
| Query Frontend | 查询缓存和分片 |
| Compactor | 合并和压缩索引块 |

### 与 ELK 对比

| 特性 | ELK | Loki |
|------|-----|------|
| 索引方式 | 全文索引 | 标签索引 |
| 存储成本 | 高 | 低（10-20x） |
| 查询语言 | Kibana Query | LogQL |
| 适用规模 | 大规模全文检索 | 标签驱动的日志查询 |

## 关键机制或特性

- **LogQL**：类 PromQL 的日志查询语言，支持标签过滤和日志解析。
- **多租户**：通过 `X-Scope-OrgID` Header 隔离租户。
- **对象存储**：日志块存储在 S3/GCS/MinIO。
- **Promtail/Alloy**：日志采集 Agent（类似 Fluentd）。
- 与 Grafana 深度集成，Dashboard 中联合查询 Metrics + Logs。

## 使用场景与最佳实践

- 云原生日志方案优先选择 Loki 替代 ELK。
- 使用 Kubernetes 标签（pod、namespace、container）作为 Loki 标签。
- 避免高基数标签（如 request_id），使用 LogQL 过滤。
- 配置日志保留策略（retention）控制存储成本。
- 配合 Promtail 或 Grafana Alloy 采集容器日志。

## 参考链接

- [Loki Official](https://grafana.com/oss/loki/)

## Related

- [[domain-17-system-foundation/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/topic-dictionary/observability/grafana.md|Grafana]]
- [[domain-17-system-foundation/topic-dictionary/observability/logging.md|Logging]]
- [[domain-17-system-foundation/topic-dictionary/observability/opentelemetry.md|OpenTelemetry]]
- [[domain-17-system-foundation/topic-dictionary/observability/jaeger.md|Jaeger]]


<!-- risk-assessed -->
