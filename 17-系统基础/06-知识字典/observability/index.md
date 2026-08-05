---
title: 可观测性知识词典
description: 涵盖 Kubernetes 可观测性三大支柱（指标、日志、链路追踪）及 SLO、告警、成本监控的完整术语体系与技术参考
summary: 可观测性领域词典，覆盖 Prometheus、OpenTelemetry、Grafana、Loki、Jaeger、SLO 等核心概念
category: dictionary
tags:
- dictionary
- observability
- monitoring
- logging
- tracing
- slo
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- SRE
- 平台工程师
- 开发工程师
---

# 可观测性知识词典（Observability）

> 本词典覆盖 Kubernetes 可观测性领域的核心术语、技术组件及工程实践，是 SRE 和平台工程师构建全栈可观测体系的权威参考。

## 领域概述

可观测性（Observability）是从系统外部输出推断系统内部状态的能力，云原生可观测性建立在三大支柱之上：

- **指标 (Metrics)**：数值型时间序列数据，用于趋势分析和告警
- **日志 (Logs)**：离散事件记录，用于详细排查和审计
- **链路追踪 (Traces)**：请求在分布式系统中的完整调用路径

补充支柱：
- **事件 (Events)**：K8s 对象状态变更通知
- **Profile**：CPU/内存/锁的运行时剖析
- **成本 (Cost)**：资源使用与费用关联

## 核心术语定义

### 指标与监控

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Prometheus | CNCF 毕业项目，拉取式指标采集与存储引擎 | Prometheus |
| PromQL | Prometheus 查询语言，支持速率、聚合、向量匹配 | PromQL |
| Metrics Server | K8s 资源指标聚合器，支撑 HPA/VPA | metrics-server |
| Thanos | Prometheus 高可用 + 长期存储 + 全局视图 | Thanos |
| Mimir | Grafana Labs 的水平扩展指标后端 | Mimir |
| OpenCost | K8s 成本监控与分配工具 | OpenCost |
| Kepler | 基于 eBPF 的能耗监控（碳排放） | Kepler |
| Perses | CNCF 可视化仪表盘框架（Grafana 替代） | Perses |

### 日志

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Loki | Grafana Labs 日志聚合系统，标签索引、低成本 | Loki |
| Fluentd | 统一日志收集器，插件化架构 | Fluentd |
| Logging Operator | Banzai Cloud 的日志流水线 Operator | Logging Operator |
| 日志架构 | 采集 → 传输 → 存储 → 查询 的分层设计 | EFK/Loki Stack |
| System Logs | K8s 组件日志（kubelet/apiserver/scheduler） | journald/文件 |

### 链路追踪

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| OpenTelemetry (OTel) | CNCF 可观测性统一标准（指标+日志+追踪） | OTel SDK/Collector |
| Jaeger | 分布式链路追踪后端，支持多种存储 | Jaeger |
| Tempo | Grafana Labs 追踪后端，对象存储低成本 | Tempo |
| Pixie | eBPF 无侵入式 K8s 可观测性 | Pixie |
| Drasi | 事件驱动的持续查询引擎 | Drasi |

### 告警与 SLO

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Alertmanager | Prometheus 告警路由、分组、静默、通知 | Alertmanager |
| SLO | 服务水平目标，定义可接受的错误率/延迟 | SLO Framework |
| SLI | 服务水平指标，量化 SLO 的具体度量 | Prometheus/OTel |
| Error Budget | 错误预算 = 1 - SLO，允许的最大不可用时间 | SLO 计算 |
| 告警降噪 | 通过分组、抑制、静默减少告警疑劳 | Alertmanager |

### K8s 原生可观测性

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| K8s Events | 集群对象状态变更事件（Warning/Normal） | kubectl get events |
| kube-state-metrics | K8s 对象状态指标导出器 | kube-state-metrics |
| node_exporter | 节点硬件/OS 指标导出器 | node_exporter |
| cAdvisor | 容器资源使用指标（内置 kubelet） | cAdvisor |
| Metrics API | K8s 标准资源指标 API (metrics.k8s.io) | metrics-server |

## 技术组件索引

### 指标监控类

- [[17-系统基础/06-知识字典/observability/prometheus.md|Prometheus（指标采集与存储）]]
- [[17-系统基础/06-知识字典/observability/promql.md|PromQL（查询语言）]]
- [[17-系统基础/06-知识字典/observability/metrics-server.md|Metrics Server（资源指标）]]
- [[17-系统基础/06-知识字典/observability/mimir.md|Mimir（水平扩展指标后端）]]
- [[17-系统基础/06-知识字典/observability/thanos.md|Thanos（HA + 长期存储）]]
- [[17-系统基础/06-知识字典/observability/metrics-for-kubernetes-object-states.md|K8s 对象状态指标]]
- [[17-系统基础/06-知识字典/observability/metrics-for-kubernetes-system-components.md|K8s 系统组件指标]]
- [[17-系统基础/06-知识字典/observability/opencost.md|OpenCost（成本监控）]]
- [[17-系统基础/06-知识字典/observability/kepler.md|Kepler（能耗监控）]]
- [[17-系统基础/06-知识字典/observability/perses.md|Perses（可视化仪表盘）]]

### 日志类

- [[17-系统基础/06-知识字典/observability/loki.md|Loki（日志聚合）]]
- [[17-系统基础/06-知识字典/observability/log-aggregation-with-loki.md|Loki 日志聚合实践]]
- [[17-系统基础/06-知识字典/observability/logging-architecture.md|日志架构设计]]
- [[17-系统基础/06-知识字典/observability/logging-operator.md|Logging Operator]]
- [[17-系统基础/06-知识字典/observability/logging.md|Logging（日志基础）]]
- [[17-系统基础/06-知识字典/observability/fluentd.md|Fluentd（日志收集）]]
- [[17-系统基础/06-知识字典/observability/system-logs.md|System Logs（系统日志）]]

### 链路追踪类

- [[17-系统基础/06-知识字典/observability/opentelemetry.md|OpenTelemetry（统一标准）]]
- [[17-系统基础/06-知识字典/observability/opentelemetry-and-distributed-tracing.md|OTel 与分布式追踪]]
- [[17-系统基础/06-知识字典/observability/jaeger.md|Jaeger（追踪后端）]]
- [[17-系统基础/06-知识字典/observability/tempo.md|Tempo（追踪存储）]]
- [[17-系统基础/06-知识字典/observability/traces-for-kubernetes-system-components.md|K8s 组件追踪]]
- [[17-系统基础/06-知识字典/observability/pixie.md|Pixie（eBPF 可观测）]]

### 告警与 SLO 类

- [[17-系统基础/06-知识字典/observability/alertmanager.md|Alertmanager（告警管理）]]
- [[17-系统基础/06-知识字典/observability/alerting-and-slo-monitoring.md|告警与 SLO 监控]]

### 其他

- [[17-系统基础/06-知识字典/observability/observability.md|Observability（可观测性总论）]]
- [[17-系统基础/06-知识字典/observability/grafana.md|Grafana（可视化平台）]]
- [[17-系统基础/06-知识字典/observability/datadog.md|Datadog（商业 APM）]]
- [[17-系统基础/06-知识字典/observability/kubernetes-events.md|Kubernetes Events]]
- [[17-系统基础/06-知识字典/observability/llm-observability.md|LLM Observability]]
- [[17-系统基础/06-知识字典/observability/opengemini.md|OpenGemini（时序数据库）]]
- [[17-系统基础/06-知识字典/observability/drasi.md|Drasi（持续查询）]]

## 可观测性架构模式

### 典型全栈架构

```
数据源层:
  App (OTel SDK) → Metrics/Traces/Logs
  K8s Components → kube-state-metrics / node_exporter
  Infra → cAdvisor / eBPF (Pixie/Kepler)

采集层:
  OTel Collector (统一接收、处理、导出)
  ├── Prometheus (scrape metrics)
  ├── Fluent Bit (collect logs)
  └── OTLP (receive traces)

存储层:
  ├── Metrics: Thanos / Mimir (long-term)
  ├── Logs: Loki / OpenSearch
  └── Traces: Tempo / Jaeger

展示层:
  Grafana (统一仪表盘)
  ├── Metrics Dashboard
  ├── Log Explorer
  ├── Trace Viewer
  └── SLO Overview

告警层:
  Alertmanager → PagerDuty / Slack / Webhook
```

### 方案对比

| 方案 | 适用规模 | 成本 | 复杂度 | 代表组合 |
|------|----------|------|--------|----------|
| 轻量级 | <50 节点 | 低 | 低 | Prometheus + Loki + Grafana |
| 中等 | 50-500 节点 | 中 | 中 | Thanos + Loki + Tempo + Grafana |
| 大规模 | >500 节点 | 高 | 高 | Mimir + Loki + Tempo + OTel Collector |
| 商业 | 任意 | 高 | 低 | Datadog / New Relic / Dynatrace |

## 生产最佳实践

### 指标监控

1. **四大黄金指标**：延迟、流量、错误率、饱和度（每个服务必须监控）
2. **标签规范**：避免高基数标签（user_id、request_id），使用聚合而非原始值
3. **采集间隔**：默认 30s，关键服务 15s，避免过短导致存储压力
4. **保留策略**：原始数据 15d，降采样 5m 保留 90d，1h 保留 1y

### 日志管理

1. **结构化日志**：JSON 格式，必含 timestamp/level/service/trace_id
2. **日志级别**：生产默认 INFO，DEBUG 仅临时开启
3. **采样策略**：高吐吐量服务对 DEBUG/INFO 采样，ERROR 全量保留
4. **关联追踪**：日志中注入 trace_id，实现日志-追踪关联

### 告警治理

1. **告警分级**：P1(立即响应) / P2(1h内) / P3(下个工作日)
2. **基于 SLO 告警**：错误预算消耗速率 > 阈值时告警，而非单一指标
3. **告警收敛**：相同根因的告警合并，避免告警风暴
4. **Runbook 关联**：每条告警必须关联排查手册

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| Prometheus 采集失败 | target 不可达/认证失败 | 检查 Prometheus targets 页面、网络策略 |
| 指标缺失 | ServiceMonitor 未匹配/Pod 未暴露 metrics | 检查 ServiceMonitor selector、Pod annotations |
| Loki 查询超时 | 日志量过大/标签索引膨胀 | 检查 LogQL、调整保留策略、添加标签过滤 |
| 追踪数据丢失 | 采样率过低/Collector 过载 | 检查 OTel Collector 队列、调整采样率 |
| Alertmanager 未通知 | 路由规则不匹配/静默规则生效 | 检查 alertmanager config、silences |
| Grafana 无数据 | 数据源配置错误/时间范围不对 | 检查 datasource、调整时间范围 |

## 学习路径

```
基础: Prometheus + Grafana 部署 → PromQL 基础
进阶: OTel 集成 → Loki 日志 → Tempo 追踪
高级: Thanos/Mimir 高可用 → SLO 体系 → 告警治理
专家: eBPF 可观测 → AI 驱动异常检测 → 全链路关联
```

## 参考链接

- https://prometheus.io/
- https://opentelemetry.io/
- https://grafana.com/oss/loki/
- https://www.jaegertracing.io/
- https://thanos.io/
- https://grafana.com/oss/tempo/
- https://grafana.com/oss/mimir/
- https://www.opencost.io/

## Related

- [[09-可观测性/05-告警/index|告警运维]]
- [[09-可观测性/06-SLO-SLI/01-slo-engineering-practice|SLO 工程]]
- [[17-系统基础/06-知识字典/networking/cilium.md|Cilium Hubble 网络可观测]]
- [[09-可观测性/03-日志/15-logging-auditing|安全审计日志]]

## 深度技术解析

### OpenTelemetry Collector 架构

OTel Collector 是可观测性数据管道的核心组件：

```yaml
# OTel Collector 配置示例
apiVersion: v1
kind: ConfigMap
metadata:
  name: otel-collector-config
data:
  config.yaml: |
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: 0.0.0.0:4317
          http:
            endpoint: 0.0.0.0:4318
      prometheus:
        config:
          scrape_configs:
          - job_name: 'k8s-pods'
            kubernetes_sd_configs:
            - role: pod
    processors:
      batch:
        timeout: 5s
        send_batch_size: 1024
      memory_limiter:
        check_interval: 1s
        limit_mib: 1024
        spike_limit_mib: 256
      attributes:
        actions:
        - key: environment
          value: production
          action: upsert
    exporters:
      prometheusremotewrite:
        endpoint: http://mimir:9009/api/v1/push
      loki:
        endpoint: http://loki:3100/loki/api/v1/push
      otlp/tempo:
        endpoint: tempo:4317
        tls:
          insecure: true
    service:
      pipelines:
        metrics:
          receivers: [otlp, prometheus]
          processors: [memory_limiter, batch]
          exporters: [prometheusremotewrite]
        logs:
          receivers: [otlp]
          processors: [memory_limiter, batch]
          exporters: [loki]
        traces:
          receivers: [otlp]
          processors: [memory_limiter, batch]
          exporters: [otlp/tempo]
```

### PromQL 核心模式

```promql
# === 四大黄金指标 ===

# 1. 延迟 (P99)
histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le, service))

# 2. 流量 (QPS)
sum(rate(http_requests_total[5m])) by (service)

# 3. 错误率
sum(rate(http_requests_total{status=~"5.."}[5m])) by (service)
/
sum(rate(http_requests_total[5m])) by (service)

# 4. 饱和度 (CPU)
sum(rate(container_cpu_usage_seconds_total[5m])) by (pod)
/
sum(kube_pod_container_resource_limits{resource="cpu"}) by (pod)

# === SLO 计算 ===
# 可用性 SLO (30天窗口)
1 - (
  sum(rate(http_requests_total{status=~"5.."}[30d]))
  /
  sum(rate(http_requests_total[30d]))
)

# 错误预算消耗速率 (Multi-window Multi-burn-rate)
# 快速消耗: 1h 窗口，14.4x 燃烧率
sum(rate(http_requests_total{status=~"5.."}[1h]))
/
sum(rate(http_requests_total[1h]))
> 14.4 * (1 - 0.999)
```

### SLO 实施框架

```
SLO 定义流程:

1. 识别关键用户旅程 (CUJ)
   └─ 例: 用户登录、下单、支付

2. 定义 SLI (服务水平指标)
   └─ 例: 成功率 = 成功请求 / 总请求

3. 设定 SLO 目标
   └─ 例: 99.9% 成功率 (30天滚动窗口)

4. 计算错误预算
   └─ 错误预算 = 1 - 0.999 = 0.1% = 43.2min/30d

5. 配置告警 (Multi-burn-rate)
   ├─ 快速消耗: 1h/5m 窗口, 14.4x/14.4x
   └─ 慢速消耗: 6h/3d 窗口, 6x/3x

6. 建立响应流程
   └─ 错误预算耗尽 → 冻结发布 → 优先修复可靠性
```

## 生产案例研究

### 案例：电商平台可观测性体系建设

**背景：** 某电商平台 200+ 微服务，日均 10亿+ 请求，需要：
- 全链路追踪覆盖率 > 95%
- 告警准确率 > 90%（减少误报）
- MTTR < 15min

**架构方案：**
- OTel SDK 统一接入（Java/Go/Python）
- OTel Collector 集群（DaemonSet + Deployment 两级）
- Mimir (metrics) + Loki (logs) + Tempo (traces)
- Grafana 统一仪表盘 + SLO 概览
- Alertmanager + PagerDuty 告警路由

**关键成果：**
- MTTR 从 45min 降至 12min
- 告警疑劳减少 70%（基于 SLO 告警）
- 存储成本降低 60%（Loki vs ELK）

## 常用运维命令速查

```bash
# === Prometheus ===
# 查看采集目标状态
kubectl port-forward svc/prometheus 9090:9090 -n monitoring
# 查询当前活跃时间序列数
promtool tsdb analyze /prometheus/data
# 检查 PromQL 表达式
promtool query instant http://prometheus:9090 'up'

# === Loki ===
# 查看日志流数量
curl -s http://loki:3100/loki/api/v1/labels | jq
# 查询日志
curl -s "http://loki:3100/loki/api/v1/query_range?query={app=\"my-app\"}&limit=100"

# === OTel Collector ===
# 查看 Collector 状态
kubectl get pods -n otel-system -l app=otel-collector
# 查看 Collector 指标
curl -s http://otel-collector:8888/metrics | grep otelcol

# === K8s 原生 ===
# 查看集群事件
kubectl get events -A --sort-by='.lastTimestamp' | tail -20
# 查看节点资源使用
kubectl top nodes
# 查看 Pod 资源使用
kubectl top pods -A --sort-by=cpu | head -20
```

## 缩略语表

| 缩写 | 全称 | 说明 |
|------|------|------|
| OTel | OpenTelemetry | 可观测性统一标准 |
| SLI | Service Level Indicator | 服务水平指标 |
| SLO | Service Level Objective | 服务水平目标 |
| SLA | Service Level Agreement | 服务水平协议 |
| MTTR | Mean Time To Recovery | 平均恢复时间 |
| MTTD | Mean Time To Detection | 平均发现时间 |
| CUJ | Critical User Journey | 关键用户旅程 |
| APM | Application Performance Monitoring | 应用性能监控 |
| eBPF | Extended Berkeley Packet Filter | 扩展 BPF 技术 |
| OTLP | OpenTelemetry Protocol | OTel 传输协议 |
| QPS | Queries Per Second | 每秒查询数 |
| RED | Rate, Errors, Duration | 服务监控三指标 |

## 版本兼容性矩阵

| 组件 | K8s 1.28 | K8s 1.29 | K8s 1.30 | K8s 1.31 |
|------|-----------|-----------|-----------|----------|
| Prometheus | v2.48+ | v2.50+ | v2.52+ | v3.0+ |
| OTel Collector | v0.90+ | v0.95+ | v0.100+ | v0.105+ |
| Grafana | v10.2+ | v10.4+ | v11.0+ | v11.2+ |
| Loki | v2.9+ | v3.0+ | v3.1+ | v3.2+ |
| Tempo | v2.3+ | v2.4+ | v2.5+ | v2.6+ |
| Thanos | v0.33+ | v0.34+ | v0.35+ | v0.36+ |
| Mimir | v2.11+ | v2.12+ | v2.13+ | v2.14+ |
| Jaeger | v1.52+ | v1.55+ | v1.58+ | v1.60+ |

## 常见问题 FAQ

**Q1: Prometheus 和 OTel Metrics 应该选哪个？**

A: 两者不矛盾。OTel SDK 负责应用内指标采集，通过 OTLP 导出到 OTel Collector，再转换为 Prometheus 格式写入 Prometheus/Mimir。Prometheus 仍然是 K8s 生态的指标存储和查询标准。建议：应用用 OTel SDK，基础设施用 Prometheus scrape，统一汇入 Mimir/Thanos。

**Q2: Loki 和 Elasticsearch 怎么选？**

A: Loki 优势：成本低（只索引标签，不索引全文）、与 Grafana 深度集成、运维简单。ES 优势：全文检索能力强、生态成熟。建议：K8s 日志用 Loki（标签查询为主），应用日志需要复杂全文检索时用 ES。

**Q3: 采样率怎么设置？**

A: 分场景：
- 生产环境头部采样：1-10%（高吐吐量服务）
- 错误请求：100% 采样（通过 tail sampling）
- 开发/测试：100%
- 关键交易链路：100%
OTel Collector 的 tail sampling processor 可实现“错误全采、正常采样”。

**Q4: 指标基数爆炸怎么处理？**

A: 高基数是可观测性最大敌人。解决方案：
1. 避免 user_id/request_id 作为标签
2. 使用 recording rules 预聚合
3. Prometheus 的 metric_relabel_configs 丢弃无用指标
4. Mimir/Thanos 的 limit 配置限制每租户序列数

**Q5: 如何实现日志-追踪-指标三者关联？**

A: 核心是统一的 trace_id：
1. OTel SDK 自动注入 trace_id 到日志、指标、追踪
2. Grafana 中通过 trace_id 从日志跳转到追踪
3. 从追踪跳转到对应时间段的指标
4. Exemplar 机制将指标数据点关联到具体 trace

## 可观测性成熟度检查清单

| 级别 | 检查项 | 状态 |
|------|--------|------|
| L1 基础 | 所有服务暴露 /metrics 端点 | ☐ |
| L1 基础 | 节点/Pod 资源监控覆盖 | ☐ |
| L1 基础 | 集中式日志收集 | ☐ |
| L2 进阶 | 分布式追踪接入 > 80% 服务 | ☐ |
| L2 进阶 | 结构化日志 + trace_id 关联 | ☐ |
| L2 进阶 | 告警规则覆盖四大黄金指标 | ☐ |
| L3 高级 | SLO 定义并可视化 | ☐ |
| L3 高级 | 基于错误预算的告警 | ☐ |
| L3 高级 | 日志-追踪-指标三者关联 | ☐ |
| L4 专家 | eBPF 无侵入式可观测 | ☐ |
| L4 专家 | AI 驱动异常检测 | ☐ |
| L4 专家 | 全链路压测 + 可观测联动 | ☐ |

