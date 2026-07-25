---
title: 发布说明索引 — 可观测性
description: '| Prometheus | 87 | v3.11 | v3.7 | 监控与告警引擎 |'
summary: '| Prometheus | 87 | v3.11 | v3.7 | 监控与告警引擎 |'
category: references
tags:
- k8s
- release-notes
- observability
- prometheus
- grafana
- loki
- thanos
- opentelemetry
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — 可观测性 是什么
- 如何 发布说明索引 — 可观测性
trigger_keywords:
- 发布说明索引
- 可观测性
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 发布说明索引 — 可观测性

> 本文档汇总可观测性领域 5 个核心项目的发布说明索引，共覆盖 **374 篇**发布说明。

---

## 项目总览

| 项目 | 文件数 | 最新版本 | 最近 Breaking Changes | 说明 |
|------|--------|----------|----------------------|------|
| Prometheus | 87 | v3.11 | v3.7 | 监控与告警引擎 |
| Grafana | 71 | v12.4 | — | 可视化仪表盘 |
| Loki | 29 | v3.7 | v3.2 | 日志聚合系统 |
| Thanos | 41 | v0.41 | v0.41 | Prometheus 高可用扩展 |
| OpenTelemetry Collector | 146 | v0.149 | v0.149 | 统一遥测数据收集 |

---

## 项目详情

### Prometheus

- **实体页面**: [[prometheus|Prometheus]]
- **最新版本**: v3.11
- **发布说明目录**: `生态参考/_archived-release-notes/observability/prometheus/`
- **版本覆盖**: v0.11 → v3.11（87 个版本）
- **Breaking Changes 提醒**:
  - v3.7: 追踪相关修复（OTLP HTTP 追踪启动失败）
- **升级要点**: v3.x 系列引入新的查询引擎优化和远程写入改进

### Grafana

- **实体页面**: Grafana
- **最新版本**: v12.4
- **发布说明目录**: `生态参考/_archived-release-notes/observability/grafana/`
- **版本覆盖**: v0.1 → v12.4（71 个版本）
- **升级要点**: v10+ 引入 Scenes 框架，v11+ 默认启用新仪表盘引擎

### Loki

- **实体页面**: Loki
- **最新版本**: v3.7
- **发布说明目录**: `生态参考/_archived-release-notes/observability/loki/`
- **版本覆盖**: v0.1 → v3.7（29 个版本）
- **Breaking Changes 提醒**:
  - v3.2: 存储格式变更，需迁移现有索引
- **升级要点**: v3.x 统一了 BoltDB 与 TSDB 索引

### Thanos

- **实体页面**: [[thanos|Thanos]]
- **最新版本**: v0.41
- **发布说明目录**: `生态参考/_archived-release-notes/observability/thanos/`
- **版本覆盖**: v0.1 → v0.41（41 个版本）
- **Breaking Changes 提醒**:
  - v0.41: 部分查询 API 行为变更
- **升级要点**: 持续改进 Sidecar 和 Store Gateway 性能

### OpenTelemetry Collector

- **实体页面**: [[opentelemetry|OpenTelemetry Collector]]
- **最新版本**: v0.149
- **发布说明目录**: `生态参考/_archived-release-notes/observability/opentelemetry-collector/`
- **版本覆盖**: v0.1 → v0.149（146 个版本）
- **Breaking Changes 提醒**:
  - v0.149: 配置格式和处理器管道变更
- **升级要点**: 高频发布，建议关注 stable 版本迁移

---

## 跨项目 Breaking Changes 汇总

| 版本 | 项目 | 变更摘要 |
|------|------|----------|
| v3.7 | Prometheus | OTLP HTTP 追踪启动修复 |
| v3.2 | Loki | 存储索引格式迁移 |
| v0.41 | Thanos | 查询 API 行为变更 |
| v0.149 | OpenTelemetry Collector | 配置格式和处理器管道变更 |

---

## 相关导航

- [[22-概念/12-研究/observability-stack-evolution.md|可观测性技术栈演进]]
- [[21-生态参考/98-merged-indexes/index.md|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## Related

- [[23-实体/15-参考与索引/k8s-observability-ecosystem.md|k8s-observability-ecosystem]] — 可观测性体系：指标、日志、链路追踪与混沌工程
- [[21-生态参考/98-merged-indexes/index.md|release-notes-networking]] — 发布说明索引 — 网络
- [[opentelemetry]] — OpenTelemetry
- [[thanos]] — Thanos
- [[prometheus]] — Prometheus


<!-- risk-assessed -->
