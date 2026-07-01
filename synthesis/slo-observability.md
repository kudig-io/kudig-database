---
title: SLO × 可观测性
category: synthesis
tags:
- slo
- sli
- observability
- prometheus
- metrics
sources:
- domain-06-observability/06-slo-sli/01-slo-engineering-practice.md
- domain-06-observability/06-slo-sli/02-error-budget-policy.md
- domain-06-observability/06-slo-sli/03-sli-implementation-guide.md
- domain-06-observability/02-metrics/README.md
created: "2026-06-26"
updated: "2026-06-26"
last_updated: 2026-06-26
summary: "SLO 工程与可观测性体系的交叉：如何从原始指标推导服务等级目标并驱动错误预算决策。"
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.78
lifecycle: draft
lifecycle_changed: "2026-06-26"
---

# SLO × 可观测性

## The Connection

SLO（服务等级目标）定义了服务可接受的表现边界，SLI（服务等级指标）是衡量 SLO 的具体指标。可观测性系统（Metrics、Logs、Traces）提供生成 SLI 所需的原始数据。没有可观测性，SLO 只是空谈；没有 SLO，可观测性数据缺乏业务语境。^[inferred]

## Where They Co-occur

- Prometheus 是 K8s 环境中最常用的 SLI 数据源
- burn rate alert 将 SLO 错误预算消耗速度与告警策略结合
- Grafana 仪表盘同时展示 SLI 趋势和错误预算剩余量
- 分布式追踪用于计算延迟类 SLI（如 p99 latency）
- 日志用于计算可用性/正确率类 SLI（如错误日志比例）

## Cross-cutting Insight

可观测性回答"系统现在怎么样"，SLO 回答"这样可以接受吗"。将二者结合，可观测性从"排查故障的工具"升级为"服务质量治理的仪表盘"——团队可以基于错误预算决定何时冻结发布、何时投入可靠性工程。^[inferred]

## Tensions and Trade-offs

| 维度 | 纯可观测性 | 纯 SLO | 结合 |
|---|---|---|---|
| 数据量 | 大量原始指标 | 少量目标值 | 需从海量指标中精选 SLI |
| 告警 | 基于阈值 | 基于预算消耗 | burn rate 减少告警疲劳 |
| 成本 | 存储全量数据贵 | 几乎无存储成本 | 可采样/降精度降低开销 |
| 文化 | 工程师驱动 | 业务驱动 | 需要跨团队共识 |

## Open Questions

- 如何为 AI/ML 推理服务定义合适的延迟和可用性 SLI？
- 在专有云多租户环境中，SLO 应该按集群、命名空间还是应用维度拆分？
- 当错误预算耗尽时，GitOps 发布流水线应如何自动拦截变更？

## Related

- [[domain-06-observability/06-slo-sli/01-slo-engineering-practice.md|01 slo engineering practice]]
- [[domain-06-observability/06-slo-sli/02-error-budget-policy.md|02 error budget policy]]
- [[domain-06-observability/06-slo-sli/03-sli-implementation-guide.md|03 sli implementation guide]]
- [[domain-19-landscape-references/topic-release-notes/README.md|README]]
