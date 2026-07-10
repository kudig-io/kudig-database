---
title: Thanos
description: Thanos 是 CNCF 孵化项目，为 Prometheus 提供高可用、长期存储和多集群全局视图能力。它解决了单实例 Prometheus
  的存储和扩展瓶颈...
summary: Thanos 是 CNCF 孵化项目，为 Prometheus 提供高可用、长期存储和多集群全局视图能力。它解决了单实例 Prometheus 的存储和扩展瓶颈...
category: dictionary
tags:
- k8s
- glossary
- thanos
- prometheus
- observability
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Thanos 是什么
- Thanos 详解
trigger_keywords:
- Thanos
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Thanos

> **英文名**: Thanos

## 概述

Thanos 是 CNCF 孵化项目，为 Prometheus 提供高可用、长期存储和多集群全局视图能力。它解决了单实例 Prometheus 的存储和扩展瓶颈，是大规模 Kubernetes 监控的首选方案。

## 核心概念/原理

### 核心组件

| 组件 | 功能 |
|------|------|
| Sidecar | 与 Prometheus 同 Pod 部署，上传 TSDB 数据到对象存储 |
| Store Gateway | 从对象存储查询历史数据 |
| Query | 合并多个 Prometheus/Store Gateway 的查询结果 |
| Compactor | 压缩和降采样对象存储中的历史数据 |
| Ruler | 在 Thanos 级别执行告警规则 |
| Receive | 接收远程写入的数据（Push 模式） |

## 关键机制或特性

- **全局视图**：跨集群查询所有 Prometheus 实例的数据。
- **无限保留**：TSDB 数据上传到 S3/GCS/MinIO 实现长期存储。
- **降采样**：自动将历史数据降采样（5m/1h）减少查询开销。
- **去重**：相同指标的多个副本自动去重。
- 兼容 PromQL，无需修改现有查询。

## 使用场景与最佳实践

- 多集群环境使用 Thanos Query 提供统一查询入口。
- 对象存储优先选择 S3 兼容的存储（MinIO、AWS S3）。
- 配置 Compactor 的 retention 策略管理存储成本。
- 使用 Thanos Ruler 实现全局告警规则。
- 考虑 VictoriaMetrics 作为 Thanos 的替代方案。

## 参考链接

- [Thanos Official](https://thanos.io/)

## Related

- [[系统基础/topic-dictionary/observability/prometheus.md|Prometheus]]
- [[系统基础/topic-dictionary/observability/grafana.md|Grafana]]
- [[系统基础/topic-dictionary/observability/alertmanager.md|Alertmanager]]
- [[系统基础/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[系统基础/topic-dictionary/observability/opentelemetry.md|OpenTelemetry]]


<!-- risk-assessed -->
