---
title: 可观测性栈演进
description: '| Loki | 29 个版本 | 日志聚合 |'
category: concepts
tags:
- k8s
- release-notes
- prometheus
- grafana
- loki
- thanos
- opentelemetry
- observability
- jaeger
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 可观测性栈演进 是什么
- 如何 可观测性栈演进
trigger_keywords:
- 可观测性栈演进
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- tracing-basics
- observability-basics
---

# 可观测性栈演进

> 本文档综合了 `domain-19-landscape-references/topic-release-notes/observability/` 目录下 5 个可观测性组件的 374 个版本发布说明 ^[inferred]

## 组件概览

| 组件 | 版本范围 | 定位 |
|---|---|---|
| Prometheus | 87 个版本 | 指标采集与告警 |
| Grafana | 71 个版本 | 数据可视化与仪表盘 |
| Loki | 29 个版本 | 日志聚合 |
| Thanos | 41 个版本 | Prometheus 高可用和长期存储 |
| OpenTelemetry Collector | 146 个版本 | 统一遥测数据采集 |

## Prometheus 版本演进

Prometheus 是 CNCF 毕业项目，Kubernetes 事实上的指标采集标准。

### v2.0 - 重大重构

这是 Prometheus 历史上最重要的版本：
- **完全重写的存储层**，带 WAL（Write-Ahead Log）
- 与 1.x 存储不向后兼容
- 新的陈旧性（staleness）行为
- 规则文件改用 YAML 语法
- 移除 `count_scalar`、`drop_common_labels` 等 PromQL 函数
- 重写 Protobuf exposition 格式解析器，性能大幅提升
- 规则分组支持
- 实验性 GRPC API
- PromQL `timestamp()` 函数

### v2.x 系列

- 持续性能优化
- 远程读写改进
- 服务发现增强
- 更好的 TSDB 压缩
- 增强的 PromQL 功能 ^[inferred]

## Grafana 版本演进

Grafana 是领先的开源可视化和监控仪表盘平台。

### 关键演进

- 新增数据源支持（Prometheus、Loki、Tempo 等）
- 改进的告警引擎
- 面板插件生态
- 探索（Explore）功能
- 企业级特性（RBAC、审计日志）^ [inferred]

## Loki 版本演进

Loki 是 Grafana Labs 开发的水平可扩展、高可用、多租户日志聚合系统。

### 核心特点

- 不索引日志内容，仅索引标签
- 与 Prometheus 标签体系一致
- LogQL 查询语言
- 与 Grafana 深度集成
- 成本效益高的日志存储 ^[inferred]

## Thanos 版本演进

Thanos 为 Prometheus 提供高可用、全球视图和长期存储。

### 核心组件

| 组件 | 功能 |
|---|---|
| Sidecar | 与 Prometheus 配对，上传数据到对象存储 |
| Store Gateway | 从对象存储提供历史数据查询 |
| Querier | 跨多个 Prometheus 实例的全球查询 |
| Compactor | 压缩和去重长期存储的数据 |
| Ruler | 全局告警规则评估 |

### 关键演进

- 对象存储支持（S3、GCS、Azure）
- 查询优化
- 去重机制改进
- 更好的压缩和降采样 ^[inferred]

## OpenTelemetry Collector 版本演进

OpenTelemetry Collector 是统一的遥测数据采集和转发组件。

### 版本模式

OpenTelemetry Collector 有 146 个发布版本，反映了其活跃的迭代节奏：
- Core 和 Contrib 分发
- 丰富的接收器（Receiver）、处理器（Processor）、导出器（Exporter）
- 支持 Metrics、Logs、Traces 三大数据类型
- Kubernetes 集群内的 DaemonSet 和 Deployment 部署模式 ^[inferred]

## 可观测性栈整合

### 经典栈：Prometheus + Grafana

```
Prometheus (采集+存储+告警) -> Grafana (可视化)
```

### 完整栈：Prometheus + Loki + Tempo + Grafana

```
Prometheus (Metrics) -> Grafana
Loki (Logs)          -> Grafana
Tempo (Traces)       -> Grafana
```

### 大规模栈：Prometheus + Thanos + Grafana

```
Prometheus (本地采集) -> Thanos Sidecar -> 对象存储
                                    -> Thanos Querier (全局查询) -> Grafana
```

### 现代化栈：OpenTelemetry + 后端

```
OTel Collector (统一采集) -> Prometheus/Loki/Jaeger/其他后端
```

## 版本选择建议

| 场景 | 推荐组合 |
|---|---|
| 小型集群 | Prometheus + Grafana |
| 需要日志 | Prometheus + Loki + Grafana |
| 大规模/多集群 | Prometheus + Thanos + Grafana |
| 统一遥测 | OpenTelemetry Collector + 后端 |

## 来源文档

- domain-19-landscape-references/topic-release-notes/observability/prometheus/（87 个文件）
- domain-19-landscape-references/topic-release-notes/observability/grafana/（71 个文件）
- domain-19-landscape-references/topic-release-notes/observability/loki/（29 个文件）
- domain-19-landscape-references/topic-release-notes/observability/thanos/（41 个文件）
- domain-19-landscape-references/topic-release-notes/observability/opentelemetry-collector/（146 个文件）

## Related

- [[opentelemetry]] — OpenTelemetry
- [[thanos]] — Thanos
- [[jaeger]] — Jaeger
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
