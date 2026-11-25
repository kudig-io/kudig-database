---
title: 可观测性栈演进
description: '| Loki | 29 个版本 | 日志聚合 |'
summary: '| Loki | 29 个版本 | 日志聚合 |'
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
tier: core
created: '2026-05-23'
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
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 可观测性栈演进

> 本文档综合了 `生态参考/_archived-release-notes/observability/` 目录下 5 个可观测性组件的 374 个版本发布说明 ^[inferred]

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
- 实验性 [[grpc|GRPC]] API
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
- Kubernetes 集群内的 [[daemonset|DaemonSet]] 和 Deployment 部署模式 ^[inferred]

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

## 源码实现分析

### Prometheus 采集与存储引擎

```go
// github.com/prometheus/prometheus/scrape/scrape.go
// Prometheus 采集循环
func (s *scrapeLoop) run(interval time.Duration) {
    ticker := time.NewTicker(interval)
    for range ticker.C {
        // 1. HTTP GET /metrics 拉取指标
        b, err := s.scraper.scrape(ctx)
        
        // 2. 解析文本格式指标
        samples := s.parser.Parse(b)
        
        // 3. 写入 TSDB（本地磁盘）
        s.appender.Append(samples)
        // TSDB: 2h block → compact → 持久化
    }
}

// github.com/prometheus/prometheus/tsdb/db.go
// TSDB 存储架构
// Head (2h, 内存+WAL) → Block (2h/36h/...) → Compaction
// 查询: 倒排索引 (label→series) + 时间序列数据
```

```
┌─────────────────────────────────────────────────────────┐
│     可观测性栈演进                                  │
├─────────────────────────────────────────────────────────┤
│  Gen 1: Prometheus + Grafana (2015-2019)               │
│    └─ 单体、本地存储、无高可用                      │
│                                                         │
│  Gen 2: Thanos / Cortex / Mimir (2019-2022)            │
│    └─ 长期存储、多集群、水平扩展                  │
│                                                         │
│  Gen 3: OpenTelemetry + eBPF (2022+)                   │
│    └─ 统一采集、自动埋点、内核态采集              │
│                                                         │
│  趋势: OTel 统一三支柱 + eBPF 无侵入采集          │
│  Metrics + Traces + Logs → 统一查询 (Grafana)       │
└─────────────────────────────────────────────────────────┘
```

### 生产运维：可观测性栈诊断

```bash
# 🟢 检查 Prometheus 状态
kubectl get pods -n monitoring -l app=prometheus
kubectl port-forward -n monitoring svc/prometheus 9090:9090
# 访问 /-/healthy 和 /api/v1/status/tsdb

# 🟢 检查采集目标状态
# Prometheus UI → Status → Targets
# 关注 State=DOWN 的目标

# 🟢 检查 OTel Collector
kubectl get pods -n observability -l app=otel-collector
kubectl logs -n observability -l app=otel-collector --tail=30

# 🟡 检查 Thanos/Mimir 组件
kubectl get pods -n thanos
thanos tools bucket inspect --objstore.config=bucket.yml
```

## 面试要点

1. **Prometheus 的架构和限制？**
   - Pull 模型：主动拉取 /metrics 端点
   - 本地 TSDB：2h block + WAL，单机存储
   - 限制：无原生高可用、无长期存储、无原生告警路由
   - 解决：Thanos/Mimir 提供长期存储 + 去重 + 水平扩展

2. **OpenTelemetry 的核心价值？**
   - 统一 SDK：一套 API 采集 Metrics/Traces/Logs
   - 厂商无关：数据可发送到任意后端
   - Collector：独立进程，支持路由/过滤/转换
   - 自动埋点：Java/Python/Node.js 自动注入

3. **Thanos 和 Mimir 的对比？**
   - Thanos：Sidecar 模式，对象存储后端，社区驱动
   - Mimir：单体架构，内置多租户，Grafana Labs 维护
   - 两者都解决 Prometheus 的长期存储和多集群问题
   - 选择：已有 Prometheus 用 Thanos，新建用 Mimir

4. **eBPF 在可观测性中的应用？**
   - 无侵入采集：无需修改应用代码或添加 SDK
   - 内核态采集 HTTP/DNS/TCP 指标（如 Cilium Hubble）
   - 自动生成分布式追踪（如 Grafana Beyla）
   - 限制：需要较新内核（≥ 5.8），L7 解析能力有限

## 来源文档

- 生态参考/_archived-release-notes/observability/prometheus/（87 个文件）
- 生态参考/_archived-release-notes/observability/grafana/（71 个文件）
- 生态参考/_archived-release-notes/observability/loki/（29 个文件）
- 生态参考/_archived-release-notes/observability/thanos/（41 个文件）
- 生态参考/_archived-release-notes/observability/opentelemetry-collector/（146 个文件）

## Related

- [[opentelemetry]] — OpenTelemetry
- [[thanos]] — Thanos
- [[jaeger]] — Jaeger
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
