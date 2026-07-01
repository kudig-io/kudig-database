---
title: K8S 可观测性栈
summary: 'K8S 可观测性栈：apiVersion: pyrra.dev/v1alpha1 kind: ServiceLevelObjective metadata:
  name: api-availability namespace: production spec: target: "99.9" window: 30d indicator:
  ratio: errors: metric: http_r...'
category: concepts
tags:
- observability
- opentelemetry
- prometheus
- grafana
- ebpf
- k8s
tier: core
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---



# K8S 可观测性栈

## OpenTelemetry CNCF Graduated

### 三大信号 GA

- **Traces（链路追踪）**：分布式追踪数据收集与导出，OTLP 协议标准化
- **Metrics（指标）**：支持多种指标类型（Counter、Gauge、Histogram、Summary）
- **Logs（日志）**：日志收集与导出，与 Traces/Metrics 关联

### Profiling — 第四信号

- 2024 年 OpenTelemetry Profiling 信号正式纳入规范
- 支持 CPU、内存、锁竞争等性能分析数据
- 与 Traces 关联，实现"从 trace 到 profile"的一键下钻

### GenAI 可观测性

- 新增 GenAI Semantic Conventions
- 追踪 LLM 调用：input/output tokens、latency、model 版本
- 支持 OpenAI、Anthropic、Gemini 等主流 SDK 的自动注入

### 核心组件

| 组件 | 用途 |
|------|------|
| OTel SDK | 应用埋点（auto-instrumentation） |
| OTel Collector | 数据收集、处理、导出 |
| OTLP | 统一传输协议（gRPC/HTTP） |

## Prometheus 3.0

### 核心改进

- **新 UI**：基于 React 重写，默认启用，支持更好的图表交互
- **Remote Write 2.0**：改进的远程写入协议，支持元数据传输、减少带宽
- **UTF-8 支持**：label name/value 支持 UTF-8 字符集
- **原生 OTLP 接入**：`/api/v1/otlp/v1/metrics` 端点，直接接收 OTLP metrics
- **原生直方图（Native Histograms）**：无需预定义 bucket，自动自适应精度

### PromQL 增强

- 改进的查询性能
- 更好的子查询支持
- 增强的聚合函数

### 生态整合

- 与 OpenTelemetry Collector 深度集成
- 支持 OTLP → Prometheus 转换
- 兼容现有告警规则（Alertmanager）

## Grafana LGTM 全栈

### 组件矩阵

| 组件 | 用途 | 替代 |
|------|------|------|
| **Loki** | 日志聚合 | Elasticsearch（轻量替代） |
| **Tempo** | 分布式追踪 | Jaeger、Zipkin |
| **Mimir** | 长期指标存储 | Thanos、VictoriaMetrics |
| **Pyroscope** | 持续性能分析 | Parca |
| **Alloy** | 统一数据收集 | Grafana Agent |
| **Beyla** | eBPF 自动注入 | - |

### Alloy — 替代 Grafana Agent

- 基于 OpenTelemetry Collector 构建
- 统一收集 metrics、logs、traces
- 支持 eBPF 自动注入（配合 Beyla）
- 原生 Kubernetes 发现与配置

### Beyla — eBPF 自动可观测性

- 零代码自动注入：HTTP、gRPC、SQL、Redis 等协议
- 基于 eBPF，无需修改应用代码或重新编译
- 自动生成 RED 指标（Rate、Errors、Duration）
- 导出至 Prometheus / OTel Collector

## 分布式追踪

### Jaeger v2 — OTel 架构

- **完全基于 OpenTelemetry Collector** 重写
- 统一的 OTLP 数据入口
- 支持 ClickHouse、Elasticsearch、Cassandra 等多种存储后端
- 性能提升：批量处理、异步写入

### OTLP 标准

- OpenTelemetry Protocol（OTLP）成为追踪数据传输的事实标准
- gRPC（默认 4317）和 HTTP（默认 4318）双协议支持
- 二进制 Protobuf 编码，高效传输

### Zipkin 衰落

- 社区活跃度持续下降
- 逐渐被 Jaeger + OTel 方案替代
- 现有系统建议迁移至 OTLP 标准

## eBPF 可观测性

### Tetragon v1.0+

- Cilium 生态的安全可观测性工具
- 实时监控系统调用、网络事件、文件访问
- 支持内核级策略执行（阻断恶意行为）
- 低开销（< 2% CPU），适合生产环境

### Hubble

- Cilium 内置的网络可观测性工具
- 实时可视化网络流（DNS、HTTP、Kafka 等）
- 基于 eBPF，零侵入
- 提供 CLI 和 UI 两种界面

### Pixie — 减速

- CNCF 项目，但社区发展放缓
- eBPF 自动采集 metrics/logs/traces
- 边缘计算架构（数据不离开集群）
- 与 New Relic 整合后独立性降低

### Beyla — 崛起

- Grafana 主推的 eBPF 自动可观测性方案
- 轻量级，资源消耗低
- 与 LGTM 栈深度集成
- 逐步替代 Pixie 在 eBPF 可观测性的地位

## SLO 监控

### Pyrra — K8S Operator

- Kubernetes 原生的 SLO 管理
- 自动生成 PrometheusRecordingRules
- 支持 SLI（Service Level Indicator）定义
- 自动生成多窗口、多 Burn Rate 告警规则

```yaml
# Pyrra SLO 定义示例
apiVersion: pyrra.dev/v1alpha1
kind: ServiceLevelObjective
metadata:
  name: api-availability
  namespace: production
spec:
  target: "99.9"
  window: 30d
  indicator:
    ratio:
      errors:
        metric: http_requests_total{status=~"5.."}
      total:
        metric: http_requests_total
```

### Sloth — CLI 工具

- 命令行 SLO 配置生成器
- 基于 YAML 定义 SLO，生成 Prometheus 规则
- 支持多窗口 Burn Rate 告警
- 与 GitOps 工作流集成

```yaml
# Sloth SLO 定义示例
sloth/v1/promeetheusservicelevel:
  service: api
  labels:
    team: backend
  slos:
    - name: availability
      objective: 99.9
      description: "API 可用性 SLO"
      sli:
        events:
          error_query: sum(rate(http_requests_total{status=~"5.."}[{{.window}}]))
          total_query: sum(rate(http_requests_total[{{.window}}]))
      alerting:
        page_alert:
          labels:
            severity: critical
        ticket_alert:
          labels:
            severity: warning
```

## 可观测性架构最佳实践

### 数据流

```
应用 → OTel SDK → OTel Collector → 后端存储
                          ↓
                    Prometheus（metrics）
                    Loki（logs）
                    Tempo（traces）
                    Pyroscope（profiles）
```

### 关键原则

1. **统一协议**：全链路使用 OTLP，避免多协议转换
2. **分层收集**：Agent（节点级）→ Gateway（集群级）→ 后端存储
3. **关联分析**：Trace ID 贯穿 logs/metrics/traces
4. **成本控制**：采样策略（head-based / tail-based）、数据保留期
5. **零侵入**：优先使用 eBPF 自动注入（Beyla、Tetragon）

## 参考资料

- [[opentelemetry]] - OpenTelemetry 框架
- [[prometheus]] - Prometheus 监控系统
- Grafana 可视化平台
- [[jaeger]] - Jaeger 分布式追踪
- eBPF 技术
- [[k8s-networking-evolution]] - K8S 网络演进

## Related

- [[concepts/slo-error-budget-framework.md|slo error budget framework]] — SLO 与 Error Budget 框架
- [[concepts/k8s-networking-evolution.md|k8s networking evolution]] — K8S 网络技术演进
- [[concepts/k8s-ai-ml-infrastructure.md|k8s ai ml infrastructure]] — K8S AI/ML 基础设施
