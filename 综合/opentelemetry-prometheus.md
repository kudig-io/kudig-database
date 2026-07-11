---
title: OpenTelemetry × Prometheus
summary: OpenTelemetry 与 Prometheus 的交叉：OTLP 采集管道如何与 Prometheus 指标生态对接并走向统一遥测。
category: synthesis
tags:
- opentelemetry
- prometheus
- otlp
- metrics
- observability
tier: supporting
sources:
- 实体/opentelemetry.md
- 实体/prometheus.md
- 概念/observability-pillars.md
- 概念/k8s-observability-stack.md
- 概念/observability-stack-evolution.md
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.76
lifecycle: draft
lifecycle_changed: '2026-07-11'
---

# OpenTelemetry × Prometheus

## The Connection

OpenTelemetry（OTel）是 CNCF 推动的统一遥测标准（OTLP 协议 + SDK + Collector），目标是统一 metrics、logs、traces 三支柱的采集与传输。Prometheus 是云原生 metrics 的事实标准，拥有成熟的查询语言（PromQL）与告警生态。二者关系正在收敛：Prometheus 已原生支持 OTLP 摄取（v2.47+/v3），OTel Collector 也可将 OTLP 转换为 Prometheus exposition format。理解这条管道，才能避免"两套指标体系并存"的混乱。从历史演进看，二者曾各自独立发展——Prometheus 以 pull scrape 模式主导 K8s 指标采集，OTel 以 push OTLP 模式主导 traces/logs 采集。随着 OTel 成为 CNCF 的旗舰遥测项目，业界开始寻求统一：Prometheus v3 引入 OTLP/gRPC 原生接收端点，使得应用可以直接用 OTel SDK push 指标到 Prometheus，而无需暴露 `/metrics` 端点等待 scrape。这一融合的意义在于——应用开发者只需集成一套 OTel SDK（同时产出 metrics + traces + logs），不再需要同时维护 Prometheus client library 和 OpenTelemetry SDK 两套埋点代码。从协议层看，二者的语义模型仍需映射：OTel 用 `Meter` + `Attributes` 表达指标，Prometheus 用 metric name + labels；转换时需处理命名规约（OTel 的 dot notation → Prometheus 的 underscore）、单调性（OTel Sum 的 monotonic vs Prometheus Counter）以及 exemplars 的差异。^[inferred]

## Where They Co-occur

- **OTel SDK 采集 → Collector → Prometheus**：应用用 OTel SDK 产出 OTLP metrics，Collector 转换为 Prometheus 格式被 scrape，或直接通过 remote write 推送。
- **Prometheus 原生 OTLP 摄取**：Prometheus v3 支持 OTLP/gRPC 直接接收，省去 Collector 转换层。
- **指标语义对齐**：OTel 用 `Meter` + Attributes，Prometheus 用 metric name + labels；转换时需处理命名规约、单调性、exemplars 的差异。
- **Exemplars 关联 trace**：Prometheus exemplars 可携带 traceID，OTel trace 与之打通，实现"指标异常 → 跳转 trace"的排障闭环。
- **统一 Collector 数据面**：OTel Collector 作为统一网关，分发 metrics 给 Prometheus、traces 给 Jaeger、logs 给 Loki。
- **Thanos 长期存储**：Prometheus + Thanos 做长期指标存储，OTel 负责前端采集，二者各司其职。
- **Exemplar 携带 trace context**：Prometheus exemplars 可携带 `trace_id`/`span_id`，OTel trace 与之打通后，Grafana 支持从指标图表直接跳转到 Jaeger/Tempo trace 视图，实现"指标异常 → 根因 trace"的闭环排障。
- **Metric type 映射**：OTel `Counter` → Prometheus `counter`（monotonic），OTel `UpDownCounter` → Prometheus `gauge`，OTel `Histogram` → Prometheus `histogram`——但 bucket boundaries 默认不同，需在 OTel SDK 或 Collector 中自定义对齐。

## Cross-cutting Insight

OTel 解决"怎么采"，Prometheus 解决"怎么存与查"。将 OTel 作为统一采集前端、Prometheus 作为指标存储与查询后端，团队只需维护一套 SDK 与传输协议，却能复用整个 Prometheus 生态（Grafana、Alertmanager、PromQL）。这种分层让"指标"不再与某一后端绑定，是可观测性走向可移植的关键。更深层地看，二者的融合代表了可观测性标准化的一条路径：OTel 定义"数据格式与传输协议"（类似 SQL 之于关系数据库），Prometheus/Thanos/Mimir/Datadog 作为"存储与查询引擎"各自实现这套标准。如果这条路径走通，可观测性栈将实现"前端 SDK 标准化 + 后端引擎可替换"——团队可以在不影响应用代码的前提下切换指标后端（如从自建 Prometheus 迁移到 Mimir 或商业 SaaS），这正是可观测性可移植性的终极目标。但在实践中，OTel-Prometheus 的转换层引入了微妙的数据保真风险：OTel 的 Histogram 类型使用 explicit bucket boundaries，而 Prometheus 的 native histogram 使用 exponential bucket——两者对同一份延迟数据的 bucket 定义不同，可能导致 p99 计算结果出现偏差。这种语义层的不对齐是融合过程中需要持续关注的工程挑战。^[inferred]

## Tensions and Trade-offs

| 维度 | OpenTelemetry 侧重 | Prometheus 侧重 | 结合注意事项 |
|---|---|---|---|
| 范围 | 三支柱统一标准 | 仅 metrics（极成熟） | traces/logs 仍靠 OTel |
| 协议 | OTLP（gRPC/HTTP） | Pull scrape / remote write | 需 Collector 做协议转换 |
| 语义模型 | Instrument + Attributes | metric name + labels | 命名与类型映射需规约 |
| 查询 | 无自有查询语言 | PromQL 极成熟 | 查询仍依赖 Prom |
| 迁移成本 | 改 SDK 与 Collector | 改存储后端较少 | 渐进式：先 Collector 后端 |
| 数据保真 | OTel 类型丰富（Sum/Gauge/Histogram） | Prometheus 类型有限（4 种） | Histogram bucket 定义不一致 |

## Open Questions

- 当 Prometheus 原生 OTLP 摄取成熟后，OTel Collector 是否还应承担 metrics 转换角色？还是 Collector 退化为 traces/logs 管道？
- OTel 的 metric type（Gauge/Sum/Histogram）与 Prometheus 类型在 Histogram bucket 与 exemplars 上的不对齐如何收敛？native histogram 是否是解决方案？
- 在大规模集群下，OTLP push 模式与 Prometheus pull 模式在可靠性与负载上的真实差异？push 模式是否在服务发现上更轻量？
- OTel Collector 作为统一数据面后，其自身的可观测性（self-observability）如何保证？Collector 宕机是否会成为可观测性盲区？

## Related

- [[实体/opentelemetry.md|OpenTelemetry]]
- [[实体/prometheus.md|Prometheus]]
- [[实体/thanos.md|Thanos]]
- [[实体/jaeger.md|Jaeger]]
- [[概念/observability-pillars.md|可观测性支柱]]
- [[概念/k8s-observability-stack.md|K8s 可观测性栈]]
- [[概念/observability-stack-evolution.md|可观测性栈演进]]
- [[综合/kubernetes-prometheus.md|Kubernetes × Prometheus]]
- [[综合/slo-observability.md|SLO × 可观测性]]


<!-- risk-assessed -->
