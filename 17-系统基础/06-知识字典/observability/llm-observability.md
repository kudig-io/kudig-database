---
title: LLM 可观测性
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- gpu
- vllm
- llm
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- LLM 可观测性 是什么
- 如何 LLM 可观测性
trigger_keywords:
- LLM
- 可观测性
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# LLM 可观测性

## 概述

大语言模型（LLM）的可观测性远不止传统的 CPU、内存和延迟监控。2026 年的 AI 生产系统需要追踪**提示词（Prompt）、响应（Response）、Token 消耗、模型输出质量、幻觉率（Hallucination Rate）以及成本**等专属指标。与常规微服务不同，LLM 推理的"正确性"往往是主观的，因此可观测性必须结合**自动化评估、人类反馈回路和 A/B 测试对比**来实现持续优化。

## 核心概念/原理

### 1. LLM 可观测性的四大支柱

| 支柱 | 核心问题 | 关键指标 |
|------|----------|----------|
| **性能（Performance）** | 模型响应有多快？ | Time to First Token (TTFT)、Time Per Output Token (TPOT)、总延迟 |
| **可靠性（Reliability）** | 服务是否稳定？ | 错误率、超时率、模型不可用频率 |
| **质量（Quality）** | 输出是否有用、准确？ | 幻觉率、相关性评分、人类满意度 |
| **成本（Cost）** | 每次调用花多少钱？ | Input/Output Token 数、每千 Token 成本、GPU 利用率 |

### 2. Prompt-Level 日志与追踪

每个 LLM 调用都应记录：
- **完整的 Prompt 和上下文长度**
- **模型返回的原始 Response**
- **使用的模型版本和参数**（temperature、top_p、max_tokens）
- **Token 消耗**（input_tokens、output_tokens、total_tokens）
- **请求来源**（用户 ID、会话 ID、应用端点）
- **延迟分解**（排队时间、Prefill 时间、Decode 时间）

这些数据通常通过 **[[OpenTelemetry|OpenTelemetry]]** 的自定义 Span 和 Event 进行采集，并存储在专门的 LLM 可观测性平台中。

### 3. 自动化质量评估

由于 LLM 输出的主观性，需要多种自动评估方法：
- **基于规则的检查**：检测输出是否包含拒绝回答的模板、格式是否正确
- **参考答案对比（Reference-based）**：对于有标准答案的任务，计算 BLEU、ROUGE、F1 分数
- **LLM-as-a-Judge**：使用更强的模型（如 GPT-4）作为评委，对两个模型的输出进行打分和对比
- **Embedding 相似度**：将输出和标准答案转换为向量，计算余弦相似度
- **事实性核查（Fact-checking）**：使用 RAG 知识库对输出中的事实声明进行溯源验证

### 4. 幻觉检测（Hallucination Detection）

幻觉是 LLM 生产化最大的挑战之一。检测方法包括：
- **Self-Consistency**：对同一问题多次采样，检查答案是否一致
- **RAG 溯源评分**：对于基于检索的回答，检查输出内容是否确实来自检索到的文档
- **置信度校准**：分析模型输出 token 的 log-probability，低置信度区域更容易出现幻觉
- **实体链接与知识图谱**：将输出中的实体与结构化知识库进行匹配验证

## 关键机制或特性

### LLM 可观测性工具栈

2026 年的主流 LLM 可观测性工具包括：
| 工具 | 定位 | 核心能力 |
|------|------|----------|
| **LangSmith** | LangChain 官方 | Prompt 追踪、评估数据集、A/B 测试 |
| **Langfuse** | 开源 | LLM 追踪、成本分析、Prompt 管理 |
| **Weights & Biases (W&B)** | MLOps 平台 | 模型实验追踪、评估可视化 |
| **Phoenix (Arize)** | 开源 | LLM 可解释性、漂移检测、RAG 评估 |
| **WhyLabs** | 数据/AI 可观测 | 数据漂移、模型行为异常检测 |
| **Gantry** | 商业 | 生产 LLM 评估、反馈闭环、自动优化 |

### Prompt 版本管理

Prompt 的微小改动可能显著影响输出质量和成本。最佳实践要求：
- 将 Prompt 像代码一样版本化（Prompt-as-Code）
- 记录每个 Prompt 版本的性能和质量指标
- 通过 A/B 测试验证新 Prompt 版本的效果
- 使用 Prompt 缓存（如 vLLM Prefix Caching）减少重复计算

### Token 消耗与成本监控

```python
# 示例：记录 LLM 调用指标
from opentelemetry import metrics

meter = metrics.get_meter(__name__)
token_counter = meter.create_counter("llm.tokens.total")
cost_histogram = meter.create_histogram("llm.request.cost_usd")

# 在每次 LLM 调用后记录
token_counter.add(input_tokens + output_tokens, attributes={"model": model_name})
cost_histogram.record(cost, attributes={"model": model_name})
```

### RAG 可观测性

对于 RAG 系统，可观测性需要覆盖完整的检索-生成链路：
- **检索阶段**：查询延迟、召回文档数量、Top-K 相关性分数
- **重排序阶段**：Reranker 得分分布、过滤后文档数量
- **生成阶段**：Prompt 长度、生成长度、引用准确性（Citation Accuracy）
- **端到端**：用户问题 → 检索结果 → 最终答案的完整 Trace

## 使用场景

1. **客服机器人质量监控**：自动评估每日 10 万条对话的幻觉率和用户满意度，发现异常时触发告警
2. **成本异常检测**：某应用突然 Token 消耗激增 5 倍，通过可观测性追踪发现是 Prompt 中误传了超长上下文
3. **模型版本 A/B 测试**：将 10% 流量切换到新模型版本，对比 TTFT、幻觉率和用户评分后决定是否全量发布
4. **RAG 系统调优**：发现 30% 的问题是因为检索阶段未召回正确文档，进而优化 Embedding 模型和分块策略
5. **Prompt 攻击检测**：通过监控 Prompt 长度和模式，识别潜在的提示词注入（Prompt Injection）攻击

##  base 实践/注意事项

- **不要只监控延迟**：LLM 的核心价值在于输出质量，必须建立质量指标的自动评估体系
- **保护用户隐私**：Prompt 和 Response 日志可能包含敏感信息，必须脱敏、加密，并设置合理的保留期
- **避免过度依赖自动化评分**：LLM-as-a-Judge 虽然高效，但仍需定期通过人工抽样验证其准确性
- **关联业务指标**：将 LLM 可观测性指标与核心业务指标（转化率、留存率）关联，证明 AI 投入的商业价值
- **监控模型漂移**：当输入问题的分布或模型输出的风格发生显著变化时，及时触发再训练或 Prompt 调整
- **设置 Token 预算告警**：为每个应用或团队设置每日/每月 Token 预算，超支时自动通知并限制调用
- **Trace 中注入 RAG 上下文**：在 OpenTelemetry Trace 中不仅记录 Prompt/Response，还要记录检索到的文档 ID，便于问题溯源
- **冷启动与缓存监控**：监控 Prompt Cache 命中率，低命中率往往意味着更高的延迟和成本

## 参考链接

- [LangSmith Documentation](https://docs.smith.langchain.com/)
- [Langfuse Documentation](https://langfuse.com/docs)
- [Arize Phoenix - LLM Tracing and Evals](https://docs.arize.com/phoenix/)
- [OpenTelemetry for LLM Applications](https://opentelemetry.io/docs/demo/services/llm/)
- [Weights & Biases - LLM Evaluation](https://wandb.ai/site/guides/llm-evaluation)

## Related

- [[17-系统基础/06-知识字典/observability/alerting-and-slo-monitoring.md|告警与 SLO 监控工程]]
- [[17-系统基础/06-知识字典/observability/alertmanager.md|告警管理器]]
- [[17-系统基础/06-知识字典/observability/datadog.md|Datadog]]


<!-- risk-assessed -->
