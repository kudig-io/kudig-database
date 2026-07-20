---
title: "可观测性 × AI/LLM 监控"
summary: "传统可观测性三支柱（Metrics/Logs/Traces）向 AI/LLM 领域延伸：Token 用量追踪、模型质量指标、GPU 利用率关联、AI 特有的告警设计"
category: synthesis
tags:
- observability
- ai-monitoring
- llm
- gpu
- token-usage
- model-quality
- opentelemetry
tier: supporting
sources:
- 概念/ai-ml-observability.md
- 概念/k8s-observability-stack.md
- 概念/observability-pillars.md
- 实体/opentelemetry.md
- 实体/prometheus.md
- 概念/gpu-scheduling-ai-workloads.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# 可观测性 × AI/LLM 监控

## The Connection（为什么这两个领域交叉）

传统可观测性（Observability）关注系统的"健康状态"——服务是否可用、延迟是否正常、错误率是否超标。AI/LLM 系统引入了全新的可观测性维度：模型输出质量（幻觉率、相关性）、Token 消耗（成本归因）、推理延迟分布（首 Token 延迟 vs 总延迟）、GPU 利用率与显存压力、向量检索命中率。这些维度无法用传统的 RED（Rate/Errors/Duration）或 USE（Utilization/Saturation/Errors）方法完全覆盖。

交叉的核心在于：AI 系统的"故障"不仅是"服务不可用"，更常见的是"服务可用但输出质量退化"——模型返回了看似合理但错误的答案（幻觉）、响应延迟从 2s 退化到 10s（GPU 争抢）、Token 用量突增 3 倍（Prompt 注入或循环调用）。传统告警（错误率 > 1%）无法捕获这些"静默退化"。

OpenTelemetry 正在扩展 AI/LLM 语义约定（Semantic Conventions），定义 `gen_ai.*` 属性族（模型名称、Token 数、温度参数等），使 LLM 调用可以像 HTTP 请求一样被追踪和分析。Prometheus 生态通过 DCGM Exporter 暴露 GPU 指标，与业务指标关联分析。两者结合构建 AI 系统的完整可观测性栈。

## Where They Co-occur（生产中的交叉场景）

### 场景一：LLM 调用链路追踪

用户提问 → API Gateway → RAG 检索（向量数据库）→ Prompt 组装 → LLM 推理 → 后处理 → 返回。每个环节都需要追踪：检索耗时、检索结果相关性、LLM 首 Token 延迟、总生成时间、Token 消耗。OpenTelemetry Trace 将整个链路串联，Span 属性记录 `gen_ai.usage.input_tokens`、`gen_ai.usage.output_tokens`、`gen_ai.request.model`。

### 场景二：Token 用量与成本归因

LLM API 按 Token 计费（如 GPT-4: $30/M input tokens, $60/M output tokens）。需要按团队/产品/功能归因 Token 消耗，识别成本异常（某功能 Token 用量突增 5 倍）。Prometheus 指标 `llm_tokens_total{team, model, direction}` 配合 Grafana 面板实现成本可视化。

### 场景三：模型质量持续监控

模型输出质量不能只靠用户反馈（滞后且稀疏）。自动化质量指标：幻觉检测（输出与检索文档的一致性）、相关性评分（输出与问题的匹配度）、安全过滤（有害内容检测率）。这些指标作为 Prometheus 时序数据存储，设置告警阈值（如幻觉率 > 5% 持续 10 分钟）。

### 场景四：GPU 利用率与推理性能关联

GPU 利用率 95% 但推理延迟正常 → 健康（高负载高效率）。GPU 利用率 30% 但推理延迟高 → 异常（可能是显存碎片、PCIe 带宽瓶颈、或调度问题）。DCGM Exporter 暴露 `DCGM_FI_DEV_GPU_UTIL`、`DCGM_FI_DEV_FB_USED`（显存使用），与推理延迟指标关联分析。

### 场景五：RAG 系统可观测性

RAG（检索增强生成）系统有独特的可观测性需求：检索命中率（检索到的文档是否相关）、检索延迟（向量数据库查询时间）、上下文窗口利用率（检索内容占 Prompt 的比例）、检索-生成一致性（生成内容是否基于检索结果）。每个环节都需要独立指标和追踪。

### 场景六：AI 告警设计

传统告警："错误率 > 1% 持续 5 分钟"。AI 告警需要更复杂的条件："P99 首 Token 延迟 > 5s 持续 3 分钟 AND GPU 显存使用 > 90%"（多条件组合）；"Token 用量环比增长 > 200%"（异常检测）；"模型输出平均长度突降 50%"（质量退化信号）。

## Production Patterns（生产模式与架构）

### 模式一：AI 可观测性全栈架构

```
┌─────────────────────────────────────────────────────────┐
│  AI/LLM Observability Stack                              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Application Layer (AI 应用)                            │
│  ├── OpenTelemetry SDK (gen_ai 语义约定)               │
│  ├── 自定义指标 (Token 用量、质量评分)                  │
│  └── 结构化日志 (Prompt/Response 摘要)                 │
│                                                         │
│  Collection Layer                                       │
│  ├── OTel Collector (Traces + Metrics)                 │
│  ├── DCGM Exporter (GPU 指标)                          │
│  ├── Prometheus (指标存储)                              │
│  └── Loki/Tempo (日志/追踪存储)                        │
│                                                         │
│  Analysis Layer                                         │
│  ├── Grafana (可视化 + 告警)                           │
│  ├── 质量评估服务 (幻觉检测、相关性评分)               │
│  ├── 异常检测 (Token 用量、延迟分布)                   │
│  └── 成本归因引擎 (按团队/功能)                        │
│                                                         │
│  Action Layer                                           │
│  ├── 告警 → PagerDuty/Slack                            │
│  ├── 自动扩缩容 (GPU 负载驱动)                        │
│  └── 模型降级 (质量退化时切换备用模型)                 │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 模式二：OpenTelemetry AI 语义约定

```python
# Python SDK 中的 LLM 调用追踪
from opentelemetry import trace
from opentelemetry.semconv.ai import GenAIAttributes

tracer = trace.get_tracer("llm-service")

with tracer.start_as_current_span("llm.chat") as span:
    span.set_attribute(GenAIAttributes.REQUEST_MODEL, "gpt-4o")
    span.set_attribute(GenAIAttributes.REQUEST_TEMPERATURE, 0.7)
    span.set_attribute(GenAIAttributes.REQUEST_MAX_TOKENS, 2048)

    response = llm_client.chat(messages=messages)

    span.set_attribute(GenAIAttributes.RESPONSE_MODEL, "gpt-4o-2024")
    span.set_attribute(GenAIAttributes.USAGE_INPUT_TOKENS, response.usage.input)
    span.set_attribute(GenAIAttributes.USAGE_OUTPUT_TOKENS, response.usage.output)
    span.set_attribute("gen_ai.response.finish_reason", response.finish_reason)

    # 自定义业务属性
    span.set_attribute("app.team", "customer-support")
    span.set_attribute("app.feature", "chatbot")
    span.set_attribute("app.session_id", session_id)
```

### 模式三：GPU 监控与关联

```yaml
# DCGM Exporter 配置 (关键指标)
# 部署为 DaemonSet，每 GPU 节点一个

# Prometheus 指标:
DCGM_FI_DEV_GPU_UTIL          # GPU 计算利用率 (%)
DCGM_FI_DEV_MEM_COPY_UTIL     # 显存带宽利用率 (%)
DCGM_FI_DEV_FB_USED           # 显存使用量 (MB)
DCGM_FI_DEV_FB_FREE           # 显存剩余 (MB)
DCGM_FI_DEV_GPU_TEMP          # GPU 温度 (°C)
DCGM_FI_DEV_POWER_USAGE       # 功耗 (W)
DCGM_FI_PROF_PIPE_TENSOR_ACTIVE  # Tensor Core 利用率

# 关联查询 (PromQL):
# GPU 利用率高但推理延迟也高 → 需要扩容
avg(DCGM_FI_DEV_GPU_UTIL{job="dcgm"}) by (node) > 90
and
histogram_quantile(0.99, rate(llm_inference_duration_seconds_bucket[5m])) > 5

# 显存即将耗尽 → 即将 OOM
DCGM_FI_DEV_FB_USED / (DCGM_FI_DEV_FB_USED + DCGM_FI_DEV_FB_FREE) > 0.95
```

### 模式四：Token 用量成本面板

```yaml
# Prometheus 指标设计
llm_tokens_total{model, team, feature, direction}  # direction: input/output
llm_requests_total{model, team, feature, status}
llm_cost_dollars_total{model, team, feature}

# Grafana 面板:
# 1. 按团队的 Token 消耗趋势 (堆叠面积图)
# 2. 按模型的请求分布 (饼图)
# 3. 成本 Top 10 功能 (表格)
# 4. Token 用量异常检测 (环比/同比)
# 5. 每请求平均 Token 数趋势 (识别 Prompt 膨胀)

# 告警规则:
# Token 用量突增
rate(llm_tokens_total[5m]) > 3 * rate(llm_tokens_total[1h] offset 1d)
# 单请求 Token 异常 (可能的 Prompt 注入)
llm_tokens_total{direction="input"} / llm_requests_total > 10000
```

### 模式五：模型质量监控

```
质量指标采集:
  1. 自动化评估 (每次请求):
     - 输出长度分布 (突降 = 可能退化)
     - 响应时间分布 (突增 = 性能退化)
     - 安全过滤触发率 (突增 = 输入异常)

  2. 采样评估 (每 N 次请求):
     - 幻觉检测 (LLM-as-Judge 或规则)
     - 相关性评分 (与检索文档对比)
     - 用户满意度 (thumbs up/down)

  3. 定期评估 (每日/每周):
     - 基准测试集回归 (Golden Set)
     - A/B 对比 (新旧模型)
     - 对抗样本测试

告警设计:
  - 幻觉率 > 5% 持续 10min → P2 告警
  - 平均输出长度下降 > 30% → P3 告警
  - 用户负反馈率 > 10% → P2 告警
  - 安全过滤触发率 > 20% → P1 告警 (可能的攻击)
```

## Trade-offs & Decision Matrix（权衡与决策）

| 维度 | 传统 APM (Datadog/New Relic) | OTel + Prometheus 自建 | AI 专用平台 (LangSmith/Arize) | 混合方案 |
|------|---------------------------|----------------------|------------------------------|---------|
| LLM 追踪 | 有限（需自定义） | 灵活（gen_ai 约定） | 原生支持 | 最佳 |
| GPU 监控 | 需集成 DCGM | 原生支持 | 不支持 | 需补充 |
| 质量评估 | 不支持 | 需自建 | 原生支持 | 最佳 |
| 成本归因 | 有限 | 灵活 | 原生支持 | 最佳 |
| 部署成本 | SaaS 按量计费 | 开源但运维重 | SaaS 按量计费 | 中 |
| 数据主权 | 数据在 SaaS | 数据本地 | 数据在 SaaS | 可控 |
| 集成复杂度 | 低 | 高 | 低 | 中 |
| 适用规模 | 中大型 | 大型（有 SRE 团队） | 中小型 | 所有 |

### 决策矩阵

- **已有 Prometheus/Grafana 栈** → 扩展 OTel gen_ai 约定 + DCGM（增量最小）
- **快速上线 AI 监控** → LangSmith/Arize（开箱即用）
- **数据合规要求（不出境）** → OTel + Prometheus 自建
- **大规模 GPU 集群** → DCGM + Prometheus + 自建质量评估
- **混合（推荐）** → OTel 追踪 + Prometheus 指标 + AI 平台质量评估

## Anti-patterns & Pitfalls（反模式）

### 反模式一：只监控基础设施不监控模型

GPU 利用率、显存、温度全部正常，但模型输出质量严重退化（如微调后幻觉率从 2% 升到 15%）。基础设施监控无法发现"模型层面的故障"。**正确做法**：基础设施指标 + 模型质量指标 + 业务指标三层监控缺一不可。

### 反模式二：记录完整 Prompt/Response 到日志

将完整的用户输入和模型输出写入日志系统。问题：隐私风险（用户数据）、存储成本爆炸（长对话）、合规违规（GDPR）。**正确做法**：只记录元数据（Token 数、延迟、模型版本）；完整内容加密存储或采样记录；敏感信息脱敏。

### 反模式三：告警阈值静态不变

设置"P99 延迟 > 3s 告警"后永不调整。随着模型升级（更大模型延迟更高）或业务增长（负载增加），阈值不再合理，要么告警疲劳要么漏报。**正确做法**：基于 SLO 动态设置阈值；定期（每月）审查告警有效性；使用异常检测替代固定阈值。

### 反模式四：忽略 Token 用量的长尾分布

平均 Token 用量正常，但 1% 的请求消耗了 50% 的 Token（如超长文档摘要、循环对话）。平均值掩盖了成本异常。**正确做法**：监控 P95/P99 Token 用量；按请求维度追踪异常；设置单请求 Token 上限。

### 反模式五：GPU 指标与业务指标割裂

GPU 监控在基础设施团队，LLM 业务指标在 AI 团队，两个团队各看各的面板。GPU 显存不足导致推理延迟升高时，AI 团队看到的是"延迟告警"，基础设施团队看到的是"显存告警"，无人关联。**正确做法**：统一面板关联 GPU 指标与推理指标；告警规则包含跨层条件。

### 反模式六：追踪采样率过低

LLM 调用追踪采样率设为 1%（与 HTTP 服务相同）。但 LLM 调用频率远低于 HTTP（每秒数十次 vs 数千次），1% 采样意味着大部分调用无追踪数据。**正确做法**：LLM 调用 100% 追踪（频率低，成本可控）；或至少 10-20% 采样。

## Operational Checklist（运维检查清单）

### 基础设施监控

- [ ] 部署 DCGM Exporter（所有 GPU 节点）
- [ ] 配置 GPU 指标采集：利用率、显存、温度、功耗、Tensor Core
- [ ] 设置 GPU 告警：显存 > 90%、温度 > 85°C、利用率 < 10%（空闲浪费）
- [ ] 关联 GPU 指标与 Pod（通过 node + GPU ID）

### LLM 应用监控

- [ ] 集成 OpenTelemetry SDK（gen_ai 语义约定）
- [ ] 追踪关键属性：模型名、Token 数、延迟、finish_reason
- [ ] 自定义指标：Token 用量（按团队/功能）、请求计数、错误率
- [ ] 结构化日志：请求 ID、模型版本、Token 数（不含完整内容）
- [ ] 首 Token 延迟（TTFT）和总延迟分开追踪

### 质量与成本

- [ ] 部署质量评估服务（采样评估幻觉率、相关性）
- [ ] Token 成本归因面板（按团队/功能/模型）
- [ ] 设置 Token 用量异常告警（环比 > 200%）
- [ ] 定期基准测试（Golden Set 回归）
- [ ] 用户反馈收集（thumbs up/down）

### 告警设计

- [ ] P1：服务不可用（错误率 > 10% 持续 2min）
- [ ] P2：质量退化（幻觉率 > 5% 持续 10min）
- [ ] P2：性能退化（P99 延迟 > SLO 持续 5min）
- [ ] P3：成本异常（Token 用量环比 > 200%）
- [ ] P3：GPU 资源浪费（利用率 < 20% 持续 30min）

## Related

- [[概念/ai-ml-observability.md|AI/ML 可观测性]]
- [[概念/k8s-observability-stack.md|K8s 可观测性栈]]
- [[概念/observability-pillars.md|可观测性三支柱]]
- [[实体/opentelemetry.md|OpenTelemetry]]
- [[实体/prometheus.md|Prometheus]]
- [[概念/gpu-scheduling-ai-workloads.md|GPU 调度与 AI 工作负载]]
- [[综合/opentelemetry-prometheus.md|OpenTelemetry × Prometheus]]
- [[综合/slo-observability.md|SLO × 可观测性]]
- [[综合/ai-workload-cost-optimization-finops.md|AI 工作负载 × 成本优化 × FinOps]]
