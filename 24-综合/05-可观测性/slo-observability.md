---
title: SLO × 可观测性
summary: SLO 工程与可观测性体系的交叉：如何从原始指标推导服务等级目标并驱动错误预算决策。
category: synthesis
tags:
- slo
- sli
- observability
- prometheus
- metrics
tier: supporting
sources:
- 可观测性/06-slo-sli/01-slo-engineering-practice.md
- 可观测性/06-slo-sli/02-error-budget-policy.md
- 可观测性/06-slo-sli/03-sli-implementation-guide.md
- 可观测性/02-metrics/README.md
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-07-11
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.78
lifecycle: draft
lifecycle_changed: '2026-06-26'
---


# SLO × 可观测性

## The Connection

SLO（服务等级目标）定义了服务可接受的表现边界——例如"99.9% 的请求在 200ms 内完成"。SLI（服务等级指标）是衡量 SLO 的具体度量值，通常从可观测性系统的原始指标中计算得出（如 `successful_requests / total_requests`）。可观测性系统（Metrics、Logs、Traces）提供生成 SLI 所需的原始数据流。没有可观测性，SLO 只是纸面上的数字；没有 SLO，可观测性数据缺乏业务语境，沦为"仪表盘越多越焦虑"的信息过载。从数据流视角看，SLO 工程在可观测性之上构建了一层"语义抽象"：原始指标（如 `http_request_duration_seconds_bucket`）经过 SLI 定义（`histogram_quantile(0.99, ...)`）聚合为目标值，再经过 SLO 评估（目标值是否在 99.9% 窗口内达标）转化为服务质量判断，最终通过错误预算（error budget）驱动行动决策（发布冻结 vs 继续迭代）。这条"原始数据 → SLI → SLO → 错误预算 → 行动"的链路将可观测性从"工程工具"提升为"业务治理基础设施"。^[inferred]

## Where They Co-occur

- Prometheus 是 K8s 环境中最常用的 SLI 数据源，通过 PromQL 直接表达可用性（`sum(rate(http_requests_total{status!~"5.."}[5m])) / sum(rate(http_requests_total[5m]))`）和延迟（histogram_quantile）
- burn rate alert 将 SLO 错误预算消耗速度与告警策略结合——multi-window multi-burn-rate 算法（如 1h/5m 和 6h/30m 双窗口）在 2% 预算消耗时触发 page，在 10% 时触发 ticket
- Grafana 仪表盘同时展示 SLI 趋势、错误预算剩余量和事件标记（deployment annotations），形成"可观测性 → SLO 判断 → 行动"的闭环视图
- 分布式追踪（Jaeger/Tempo）用于计算延迟类 SLI（如 p99 latency）并定位尾延迟的根因 span
- 日志用于计算可用性/正确率类 SLI（如错误日志比例、业务异常码占比），适合 metrics 无法覆盖的语义级指标
- **Sloth/OpenSLO 声明式 SLO**：把 SLO 定义为 Kubernetes CRD，由控制器自动生成 Prometheus alerting rules，将 SLO 从手动 PromQL 变为声明式管理
- **错误预算策略自动化**：错误预算耗尽时通过 webhook 拦截 ArgoCD 同步（发布冻结），将 SLO 从"事后度量"升级为"主动治理"
- **SLO 级联**：用户可见的 ULTRA SLO（如"页面加载 < 1s"）可拆解为下游依赖服务的子 SLO，可观测性系统需提供分布式依赖图来追溯 SLO 违约的根因服务
- **多窗口 burn rate 告警**：Google SRE 推荐的 multi-window multi-burn-rate 策略（如 1h/5m 对应 14.4x burn rate 触发 page、6h/30m 对应 6x burn rate 触发 ticket）在 Prometheus 中通过多条 AlertingRule 实现，自动平衡告警灵敏度与噪声控制
- **SLI Dashboard 模板化**：Grafana provisioning + JSON 模板化 SLI 仪表盘，每个新服务自动生成包含 availability、latency p99/p999、error budget remaining 的标准视图，降低可观测性接入门槛
- **SLO 报告与 review**：定期（如每月）生成 SLO 合规报告，推动"是否需要调整 SLO 目标"、"是否需要投入可靠性工程"的数据驱动讨论，而非凭直觉拍板
- **Pyrra 自动化 SLO 管理**：Pyrra 将 SLO 定义为 CRD 并自动生成 Prometheus alerting rules + Grafana dashboard，减少手写 PromQL 的错误率
- **SLO 级联告警抑制**：当上游 SLO 违约导致下游 SLI 下降时，Alertmanager inhibition rule 应抑制下游告警避免级联告警风暴
- **SLI 采样与精度**：高 QPS 服务可采样计算延迟 SLI，但低 QPS 服务需全量采集（样本不足导致 p99 波动剧烈）——采样策略按服务级别分级
- **SLO as Code**：OpenSLO 规范允许用 YAML 声明 SLO 定义（indicator、target、window），配合 Sloth 自动编译为 Prometheus recording rules 和 alerting rules——实现 SLO 的 GitOps 管理和版本化

## Cross-cutting Insight

可观测性回答"系统现在怎么样"，SLO 回答"这样可以接受吗"。将二者结合，可观测性从"排查故障的工具"升级为"服务质量治理的仪表盘"——团队可以基于错误预算决定何时冻结发布、何时投入可靠性工程。更深层的价值在于：SLO 为可观测性数据赋予了经济意义——99.9% 和 99.99% 之间的差距不是 0.09% 的技术指标，而是 10 倍的工程投入决策。当错误预算成为发布门禁和容量规划的输入时，可观测性系统从"被动记录"变为"主动驱动业务决策"的核心基础设施。然而 SLO 落地最大的挑战不在技术而在组织文化：定义 SLO 需要产品方（用户体验预期）、开发方（功能迭代节奏）和 SRE（可靠性投入）三方达成共识，而错误预算耗尽时的"发布冻结"决策往往面临业务压力的挑战。没有高管支持的 SLO 治理框架，错误预算制度容易沦为"SRE 一厢情愿"的纸面规则。因此 SLO 的成功落地需要可观测性工具支撑（精确的 SLI 测量）、流程保障（发布门禁自动化）、组织文化（错误预算的权威性）三位一体。^[inferred]

## Tensions and Trade-offs

| 维度 | 纯可观测性 | 纯 SLO | 结合 |
|---|---|---|---|
| 数据量 | 大量原始指标（high-cardinality） | 少量目标值（SLI 聚合） | 需从海量指标中精选 SLI |
| 告警 | 基于固定阈值 | 基于预算消耗速率 | burn rate 减少告警疲劳 |
| 成本 | 存储全量数据贵 | 几乎无存储成本 | 可采样/降精度降低开销 |
| 文化 | 工程师驱动（技术视角） | 业务驱动（用户视角） | 需要跨团队共识与 SLO review |
| 复杂度 | 采集即可用 | 需定义 SLI 规约与窗口策略 | SLI 设计质量决定 SLO 有效性 |
| 工具链 | Prometheus/Grafana | Sloth/Pyrra/Nobl9 | 需打通采集 → SLO 引擎 → 告警 → 预算策略 |
| 组织文化 | 工程师自发驱动 | 需产品/业务/SRE 三方共识 | 错误预算权威性需高管背书 |

## Open Questions

- 如何为 AI/ML 推理服务定义合适的延迟和可用性 SLI？推理尾延迟分布与传统 HTTP 服务差异显著（长尾更重），p99 是否足够？
- 在专有云多租户环境中，SLO 应该按集群、命名空间还是应用维度拆分？全局 SLO 与租户 SLO 如何加权？
- 当错误预算耗尽时，GitOps 发布流水线应如何自动拦截变更？如何区分"计划内变更"和"需要冻结的变更"？
- SLO 的回顾窗口（30d vs 7d）如何与业务的季节性波动（如电商大促）协调，避免误判预算状态？
- 当可观测性系统自身出现数据缺失（如 Prometheus 重启导致指标断点）时，SLO 计算如何优雅降级而非误报预算违约？

## Related

- [[09-可观测性/06-SLO-SLI/01-slo-engineering-practice.md|01 slo engineering practice]]
- [[09-可观测性/06-SLO-SLI/02-error-budget-policy.md|02 error budget policy]]
- [[09-可观测性/06-SLO-SLI/03-sli-implementation-guide.md|03 sli implementation guide]]
- [[21-生态参考/03-领域索引/README.md|README]]
