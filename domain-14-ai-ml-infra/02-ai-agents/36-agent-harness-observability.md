---
title: Agent Harness 可观测性体系 (domain-14-ai-ml-infra)
description: 'description: ''**文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Observability,'
category: general
tags:
- ai
- ai-agent
- observability
- prometheus
- grafana
- llm
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- Agent Harness 可观测性体系 是什么
- 如何 Agent Harness 可观测性体系
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- Agent
- Harness
- 可观测性体系
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- observability-basics
created: "2026-05-23"
---

title: Agent Harness 可观测性体系
description: '**文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Observability,
  [[OpenTelemetry|OpenTelemetry]], Langfuse, Traces, Metrics, Logging, [[Prometheus|Prometheus]], Grafana, 告警, Agent
  调试, Span, 执行追踪'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- prometheus
- grafana
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- Agent Harness 可观测性体系 是什么
- 如何 Agent Harness 可观测性体系
trigger_keywords:
- Agent
- Harness
- 可观测性体系
- ai
- agent
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Agent Harness 可观测性体系

> **文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Observability, OpenTelemetry, Langfuse, Traces, Metrics, Logging, Prometheus, Grafana, 告警, Agent 调试, Span, 执行追踪

---

<!-- chunk: 概述 -->## 概述

可观测性是 Agent Harness 从"PoC"走向"生产"的必要条件。传统软件系统的可观测性（Metrics/Traces/Logs）需要针对 Agent 的特殊性进行扩展——Agent 的非确定性行为、多轮推理过程、工具调用链路、LLM 延迟特性，都需要专门的追踪和度量方案。

本文系统阐述 Agent Harness 的可观测性架构，包括 OpenTelemetry 集成、Langfuse 追踪、Prometheus 指标体系、告警规则设计、调试工具链，以及生产级 Dashboard 方案。

---

<!-- chunk: 1. Agent 可观测性特殊性 -->## 1. Agent 可观测性特殊性

#<!-- chunk: 1.1 与传统可观测性的差异 -->## 1.1 与传统可观测性的差异

```
传统软件可观测性 vs Agent 可观测性:

传统软件:
  - 确定性行为: 相同输入 → 相同输出
  - 同步调用链: A → B → C
  - 固定延迟分布: P99 可预测
  - 明确的成功/失败: HTTP 状态码

Agent 系统:
  - 非确定性行为: 相同输入 → 不同输出（每次推理不同）
  - 循环调用链: Loop → Think → Act → Observe → Loop
  - 高变异延迟: LLM 响应 1-60s，取决于输出长度
  - 模糊的成功: 完成了但答案可能有幻觉
  - 多维质量: 准确性、完整性、安全性同时评估
  - Token 经济: 每次调用有成本

Agent 需要额外追踪的维度:
  ├── 推理质量（每步 Thought 的质量）
  ├── 工具选择准确率
  ├── 幻觉率
  ├── 漂移检测次数
  ├── Token 消耗和成本
  ├── 验证通过率
  └── 人工干预频率
```

#<!-- chunk: 1.2 三支柱扩展模型 -->## 1.2 三支柱扩展模型

```
Agent 可观测性三支柱 + 扩展:

1. Traces（追踪）
   │  传统: 请求在微服务间的传播路径
   │  Agent: 任务在 Loop 迭代间的执行路径
   │  扩展: 每个 Span 包含 Thought、Action、Observation
   │
2. Metrics（指标）
   │  传统: QPS、延迟、错误率
   │  Agent: 任务完成率、平均步骤数、Token 消耗、成本
   │  扩展: 验证通过率、幻觉率、漂移频率
   │
3. Logs（日志）
   │  传统: 应用日志、错误日志
   │  Agent: 推理日志、工具调用日志、约束违反日志
   │  扩展: 完整执行轨迹（Trajectory）
   │
4. Evaluations（评估）← Agent 新增
   │  LLM-as-Judge 质量评估
   │  RAGAS 分数趋势
   │  验证器详细报告
```

---

<!-- chunk: 2. OpenTelemetry 集成 -->## 2. OpenTelemetry 集成

#<!-- chunk: 2.1 Agent Span 模型 -->## 2.1 Agent Span 模型

```python
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter
from opentelemetry.sdk.resources import Resource
import time

# 初始化 OTel
resource = Resource.create({
    "service.name": "agent-harness",
    "service.version": "1.0.0",
    "deployment.environment": "production",
})

provider = TracerProvider(resource=resource)
provider.add_span_processor(
    BatchSpanProcessor(
        OTLPSpanExporter(endpoint="http://otel-collector:4317")
    )
)
trace.set_tracer_provider(provider)
tracer = trace.get_tracer("agent-harness", "1.0.0")


class OTelInstrumentedHarness:
    """OpenTelemetry 全链路追踪的 Harness"""

    def __init__(self, harness, version: str = "v1"):
        self.harness = harness
        self.version = version

    def run(self, task: str, context: dict = None) -> dict:
        """带完整追踪的任务执行"""
        with tracer.start_as_current_span("agent.task") as task_span:
            # 任务级属性
            task_span.set_attribute("agent.task", task[:500])
            task_span.set_attribute("agent.harness.version", self.version)
            task_span.set_attribute("agent.task.type",
                                    self._classify_task(task))

            start_time = time.time()

            try:
                # 上下文构建追踪
                with tracer.start_as_current_span("agent.context.build") as ctx_span:
                    built_context = self.harness.build_context(task, context)
                    ctx_span.set_attribute("context.tokens",
                                           self._count_tokens(built_context))
                    ctx_span.set_attribute("context.sources",
                                           len(context.get("sources", [])) if context else 0)

                # Loop 执行追踪
                result = self._traced_loop(task, built_context, task_span)

                # 验证追踪
                with tracer.start_as_current_span("agent.verification") as v_span:
                    verification = self.harness.verify(task, result, context)
                    v_span.set_attribute("verification.passed",
                                         verification.get("passed", False))
                    v_span.set_attribute("verification.score",
                                         verification.get("score", 0))

                # 任务结果属性
                task_span.set_attribute("agent.result.status",
                                        result.get("status", "unknown"))
                task_span.set_attribute("agent.result.iterations",
                                        result.get("iterations", 0))
                task_span.set_attribute("agent.result.total_tokens",
                                        result.get("total_tokens", 0))
                task_span.set_attribute("agent.result.duration_s",
                                        time.time() - start_time)

                return result

            except Exception as e:
                task_span.set_attribute("agent.result.status", "error")
                task_span.set_attribute("agent.result.error", str(e)[:500])
                task_span.record_exception(e)
                raise

    def _traced_loop(self, task: str, context: str, parent_span) -> dict:
        """Loop 级追踪"""
        trajectory = []
        iteration = 0

        while iteration < self.harness.max_iterations:
            with tracer.start_as_current_span(
                f"agent.loop.iteration.{iteration}"
            ) as iter_span:
                iter_span.set_attribute("iteration.number", iteration)

                # Think 追踪
                with tracer.start_as_current_span("agent.think") as think_span:
                    thought = self.harness.think(task, context, trajectory)
                    think_span.set_attribute("thought.tokens_input",
                                             thought.get("tokens_input", 0))
                    think_span.set_attribute("thought.tokens_output",
                                             thought.get("tokens_output", 0))
                    think_span.set_attribute("thought.is_final",
                                             thought.get("is_final", False))

                if thought.get("is_final"):
                    iter_span.set_attribute("iteration.final", True)
                    return {
                        "status": "success",
                        "answer": thought.get("answer"),
                        "iterations": iteration + 1,
                        "trajectory": trajectory,
                    }

                # Act 追踪
                with tracer.start_as_current_span(
                    f"agent.tool.{thought.get('tool', 'unknown')}"
                ) as tool_span:
                    action = thought.get("action", {})
                    tool_span.set_attribute("tool.name",
                                            action.get("tool", ""))
                    tool_span.set_attribute("tool.args",
                                            str(action.get("args", {}))[:500])

                    tool_result = self.harness.execute_tool(action)

                    tool_span.set_attribute("tool.success",
                                            tool_result.get("success", False))
                    tool_span.set_attribute("tool.latency_ms",
                                            tool_result.get("latency_ms", 0))
                    if not tool_result.get("success"):
                        tool_span.set_attribute("tool.error",
                                                tool_result.get("error", "")[:200])

                trajectory.append({
                    "iteration": iteration,
                    "thought": thought,
                    "tool_result": tool_result,
                })
                iteration += 1

        return {"status": "max_iterations", "iterations": iteration,
                "trajectory": trajectory}

    def _classify_task(self, task: str) -> str:
        task_lower = task.lower()
        if "pod" in task_lower:
            return "pod_diagnosis"
        elif "node" in task_lower:
            return "node_diagnosis"
        elif "network" in task_lower or "网络" in task_lower:
            return "network_diagnosis"
        return "general"

    def _count_tokens(self, text: str) -> int:
        return len(text.split()) * 1.3  # 粗略估算
```

---

<!-- chunk: 3. Langfuse 集成 -->## 3. Langfuse 集成

#<!-- chunk: 3.1 Langfuse 追踪集成 -->## 3.1 Langfuse 追踪集成

```python
from langfuse import Langfuse
from langfuse.decorators import observe, langfuse_context

# 初始化 Langfuse
langfuse = Langfuse(
    public_key="pk-...",
    secret_key="sk-...",
    host="https://langfuse.your-domain.com",
)

class LangfuseTracedHarness:
    """Langfuse 集成的 Harness"""

    def __init__(self, harness):
        self.harness = harness

    @observe(name="agent-task")
    def run(self, task: str, context: dict = None) -> dict:
        """任务级追踪"""
        langfuse_context.update_current_trace(
            metadata={"harness_version": self.harness.version},
            tags=["agent", "harness"],
        )

        # 上下文构建
        built_context = self._build_context(task, context)

        # 执行 Loop
        result = self._execute_loop(task, built_context)

        # 验证
        verification = self._verify(task, result, context)

        # 记录评分
        langfuse_context.score_current_trace(
            name="task_completion",
            value=1.0 if result.get("status") == "success" else 0.0,
        )
        if verification.get("score"):
            langfuse_context.score_current_trace(
                name="verification_score",
                value=verification["score"],
            )

        return result

    @observe(name="context-build", as_type="span")
    def _build_context(self, task, context):
        return self.harness.build_context(task, context)

    @observe(name="loop-iteration", as_type="span")
    def _loop_step(self, task, context, trajectory, iteration):
        """单步 Loop 追踪"""
        # Think
        thought = self._think(task, context, trajectory)
        if thought.get("is_final"):
            return {"final": True, "answer": thought.get("answer")}

        # Act
        tool_result = self._act(thought.get("action"))
        return {
            "final": False,
            "thought": thought,
            "tool_result": tool_result,
        }

    @observe(name="llm-think", as_type="generation")
    def _think(self, task, context, trajectory):
        """LLM 推理追踪（generation 类型自动记录 token 和模型）"""
        return self.harness.think(task, context, trajectory)

    @observe(name="tool-call", as_type="span")
    def _act(self, action):
        """工具调用追踪"""
        langfuse_context.update_current_observation(
            metadata={"tool": action.get("tool"), "args": str(action.get("args"))},
        )
        return self.harness.execute_tool(action)

    @observe(name="verification", as_type="span")
    def _verify(self, task, result, context):
        return self.harness.verify(task, result, context)
```

---

<!-- chunk: 4. Prometheus 指标体系 -->## 4. Prometheus 指标体系

#<!-- chunk: 4.1 Agent Harness 指标定义 -->## 4.1 Agent Harness 指标定义

```python
from prometheus_client import Counter, Histogram, Gauge, Summary, Info

# === 任务级指标 ===
harness_task_total = Counter(
    'harness_task_total',
    'Harness 处理的任务总数',
    ['harness_version', 'status', 'task_type']
)

harness_task_duration_seconds = Histogram(
    'harness_task_duration_seconds',
    '任务端到端执行时间',
    ['harness_version', 'task_type'],
    buckets=[0.5, 1, 2, 5, 10, 30, 60, 120, 300, 600]
)

harness_iterations_per_task = Histogram(
    'harness_iterations_per_task',
    '每任务迭代次数',
    ['harness_version', 'task_type'],
    buckets=[1, 2, 3, 5, 8, 10, 15, 20]
)

# === Token 和成本指标 ===
harness_tokens_total = Counter(
    'harness_tokens_total',
    'Token 消耗总量',
    ['harness_version', 'direction']  # direction: input/output
)

harness_cost_usd_total = Counter(
    'harness_cost_usd_total',
    '累计成本（美元）',
    ['harness_version', 'model']
)

# === 质量指标 ===
harness_verification_pass_rate = Gauge(
    'harness_verification_pass_rate',
    '验证层通过率（滑动窗口）',
    ['harness_version']
)

harness_faithfulness_score = Summary(
    'harness_faithfulness_score',
    '忠实度评分分布',
    ['harness_version']
)

harness_hallucination_detected = Counter(
    'harness_hallucination_detected_total',
    '幻觉检测次数',
    ['harness_version', 'severity']
)

# === 安全指标 ===
harness_constraint_violations = Counter(
    'harness_constraint_violations_total',
    '约束违反次数',
    ['constraint_type', 'harness_version']
)

harness_injection_attempts = Counter(
    'harness_injection_attempts_total',
    '提示注入攻击检测次数',
    ['injection_type', 'harness_version']
)

harness_approval_requests = Counter(
    'harness_approval_requests_total',
    '审批请求总数',
    ['harness_version', 'status']  # status: approved/rejected/timeout
)

# === 工具层指标 ===
harness_tool_calls_total = Counter(
    'harness_tool_calls_total',
    '工具调用总数',
    ['tool_name', 'success']
)

harness_tool_latency_seconds = Histogram(
    'harness_tool_latency_seconds',
    '工具调用延迟',
    ['tool_name'],
    buckets=[0.1, 0.25, 0.5, 1, 2, 5, 10, 30]
)

# === Loop 层指标 ===
harness_drift_detected = Counter(
    'harness_drift_detected_total',
    '漂移检测触发次数',
    ['drift_type', 'harness_version']
)

harness_loop_termination = Counter(
    'harness_loop_termination_total',
    'Loop 终止原因',
    ['reason', 'harness_version']
)
```

#<!-- chunk: 4.2 指标收集器 -->## 4.2 指标收集器

```python
import time
from functools import wraps

class HarnessMetricsCollector:
    """Harness 指标收集器"""

    def __init__(self, harness_version: str):
        self.version = harness_version

    def record_task(self, result: dict):
        """记录任务指标"""
        status = result.get("status", "unknown")
        task_type = result.get("task_type", "general")

        harness_task_total.labels(
            harness_version=self.version,
            status=status,
            task_type=task_type,
        ).inc()

        if "elapsed_seconds" in result:
            harness_task_duration_seconds.labels(
                harness_version=self.version,
                task_type=task_type,
            ).observe(result["elapsed_seconds"])

        if "iterations" in result:
            harness_iterations_per_task.labels(
                harness_version=self.version,
                task_type=task_type,
            ).observe(result["iterations"])

        if "total_tokens" in result:
            harness_tokens_total.labels(
                harness_version=self.version,
                direction="total",
            ).inc(result["total_tokens"])

        if "termination_reason" in result:
            harness_loop_termination.labels(
                reason=result["termination_reason"],
                harness_version=self.version,
            ).inc()

    def record_tool_call(self, tool_name: str, success: bool,
                         latency_seconds: float):
        """记录工具调用指标"""
        harness_tool_calls_total.labels(
            tool_name=tool_name,
            success=str(success),
        ).inc()
        harness_tool_latency_seconds.labels(
            tool_name=tool_name,
        ).observe(latency_seconds)

    def record_verification(self, report: dict):
        """记录验证指标"""
        score = report.get("total_score", 0)
        passed = report.get("overall_passed", False)

        harness_verification_pass_rate.labels(
            harness_version=self.version,
        ).set(1.0 if passed else 0.0)

        harness_faithfulness_score.labels(
            harness_version=self.version,
        ).observe(score)

    def record_violation(self, constraint_type: str):
        """记录约束违反"""
        harness_constraint_violations.labels(
            constraint_type=constraint_type,
            harness_version=self.version,
        ).inc()

    def record_drift(self, drift_type: str):
        """记录漂移检测"""
        harness_drift_detected.labels(
            drift_type=drift_type,
            harness_version=self.version,
        ).inc()
```

---

<!-- chunk: 5. 告警规则 -->## 5. 告警规则

#<!-- chunk: 5.1 生产告警规则集 -->## 5.1 生产告警规则集

```yaml
groups:
  - name: harness_critical
    rules:
    # 验证通过率低
    - alert: HarnessVerificationFailureRateHigh
      expr: |
        avg_over_time(harness_verification_pass_rate[10m]) < 0.85
      for: 10m
      labels:
        severity: critical
        team: agent-platform
      annotations:
        summary: "Harness 验证通过率低于 85%"
        description: >
          {{ $labels.harness_version }} 的验证通过率已降至
          {{ $value | humanizePercentage }}。
          可能原因: Prompt 漂移、工具返回异常、上下文质量下降。
        runbook: "https://wiki.example.com/agent-harness/verification-failure"

    # 约束违反
    - alert: HarnessConstraintViolation
      expr: |
        rate(harness_constraint_violations_total[5m]) > 0
      for: 1m
      labels:
        severity: critical
        team: security
      annotations:
        summary: "检测到 Agent 约束违反"
        description: >
          约束类型: {{ $labels.constraint_type }},
          Harness: {{ $labels.harness_version }}。
          需要立即检查 Agent 行为。

    # 提示注入攻击
    - alert: HarnessInjectionAttemptDetected
      expr: |
        rate(harness_injection_attempts_total[5m]) > 0.1
      for: 5m
      labels:
        severity: critical
        team: security
      annotations:
        summary: "频繁检测到提示注入攻击"
        description: "类型: {{ $labels.injection_type }}"

  - name: harness_warning
    rules:
    # 漂移频繁
    - alert: HarnessDriftRateHigh
      expr: |
        rate(harness_drift_detected_total[15m]) > 0.05
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Agent 漂移检测频繁触发"
        description: "漂移类型: {{ $labels.drift_type }}。可能需要优化 Prompt 或工具配置。"

    # 任务成功率下降
    - alert: HarnessTaskSuccessRateLow
      expr: |
        rate(harness_task_total{status="success"}[1h])
        / rate(harness_task_total[1h]) < 0.8
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "Harness 任务成功率低于 80%"

    # Token 消耗异常
    - alert: HarnessTokenConsumptionHigh
      expr: |
        rate(harness_tokens_total[1h]) > 500000
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Token 消耗速率过高"
        description: "当前速率: {{ $value }} tokens/小时"

    # 工具延迟异常
    - alert: HarnessToolLatencyHigh
      expr: |
        histogram_quantile(0.95, rate(harness_tool_latency_seconds_bucket[5m])) > 10
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "工具调用 P95 延迟超过 10 秒"
        description: "工具: {{ $labels.tool_name }}"

    # 成本告警
    - alert: HarnessDailyCostHigh
      expr: |
        increase(harness_cost_usd_total[24h]) > 100
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "Agent 日成本超过 $100"
```

---

<!-- chunk: 6. Grafana Dashboard 设计 -->## 6. Grafana Dashboard 设计

#<!-- chunk: 6.1 Dashboard 面板布局 -->## 6.1 Dashboard 面板布局

```
Agent Harness Grafana Dashboard 布局:

Row 1: 总览指标
  ┌────────────┬────────────┬────────────┬────────────┐
  │ 任务完成率  │ 验证通过率  │ 日Token消耗 │ 日成本($)   │
  │ (Stat)     │ (Stat)     │ (Stat)     │ (Stat)     │
  └────────────┴────────────┴────────────┴────────────┘

Row 2: 任务趋势
  ┌──────────────────────────┬──────────────────────────┐
  │ 任务数量趋势（by 状态）    │ 任务延迟分布（P50/P95/P99）│
  │ (Time Series)            │ (Time Series)            │
  └──────────────────────────┴──────────────────────────┘

Row 3: Loop & 工具
  ┌──────────────────────────┬──────────────────────────┐
  │ 平均迭代次数趋势          │ 工具调用分布（by 工具名）  │
  │ (Time Series)            │ (Bar Chart)              │
  └──────────────────────────┴──────────────────────────┘

Row 4: 质量 & 安全
  ┌──────────────────────────┬──────────────────────────┐
  │ 验证分数趋势              │ 约束违反和漂移检测        │
  │ (Time Series)            │ (Time Series)            │
  └──────────────────────────┴──────────────────────────┘

Row 5: Token & 成本
  ┌──────────────────────────┬──────────────────────────┐
  │ Token 消耗趋势            │ 成本趋势（by 模型）      │
  │ (Time Series)            │ (Time Series)            │
  └──────────────────────────┴──────────────────────────┘
```

#<!-- chunk: 6.2 关键 PromQL 查询 -->## 6.2 关键 PromQL 查询

```yaml
# Dashboard 核心 PromQL 查询

# 任务完成率
task_success_rate: |
  sum(rate(harness_task_total{status="success"}[$__rate_interval]))
  / sum(rate(harness_task_total[$__rate_interval]))

# P95 任务延迟
task_p95_latency: |
  histogram_quantile(0.95,
    sum(rate(harness_task_duration_seconds_bucket[$__rate_interval])) by (le)
  )

# 平均迭代次数
avg_iterations: |
  sum(rate(harness_iterations_per_task_sum[$__rate_interval]))
  / sum(rate(harness_iterations_per_task_count[$__rate_interval]))

# Token 消耗速率
token_rate: |
  sum(rate(harness_tokens_total[$__rate_interval])) by (direction)

# 验证通过率趋势
verification_rate: |
  avg_over_time(harness_verification_pass_rate[$__rate_interval])

# 工具调用 Top-5 延迟
tool_latency_top5: |
  topk(5,
    histogram_quantile(0.95,
      sum(rate(harness_tool_latency_seconds_bucket[$__rate_interval])) by (le, tool_name)
    )
  )

# 日成本
daily_cost: |
  increase(harness_cost_usd_total[24h])
```

---

<!-- chunk: 7. 调试工具链 -->## 7. 调试工具链

#<!-- chunk: 7.1 执行轨迹回放器 -->## 7.1 执行轨迹回放器

```python
class TrajectoryReplayer:
    """执行轨迹回放器：用于调试和分析"""

    def __init__(self, storage_backend):
        self.storage = storage_backend

    def load_trajectory(self, task_id: str) -> dict:
        """加载执行轨迹"""
        return self.storage.get(f"trajectory:{task_id}")

    def replay(self, trajectory: dict, verbose: bool = True) -> None:
        """回放执行过程"""
        print(f"任务: {trajectory['task']}")
        print(f"状态: {trajectory['status']}")
        print(f"迭代: {trajectory['iterations']}")
        print(f"Token: {trajectory['total_tokens']}")
        print("-" * 60)

        for entry in trajectory.get("entries", []):
            if verbose:
                print(f"\n--- 迭代 {entry['iteration']} ---")
                print(f"思考: {entry.get('thought', 'N/A')[:200]}")
                if entry.get("tool_name"):
                    print(f"工具: {entry['tool_name']}")
                    print(f"参数: {entry.get('tool_args', {})}")
                    print(f"结果: {str(entry.get('tool_result', ''))[:200]}")
                    print(f"成功: {entry.get('tool_success', 'N/A')}")
                print(f"延迟: {entry.get('latency_ms', 0):.0f}ms")
                print(f"Token: {entry.get('tokens_input', 0) + entry.get('tokens_output', 0)}")

    def analyze_failure(self, trajectory: dict) -> dict:
        """分析失败原因"""
        analysis = {
            "termination_reason": trajectory.get("termination_reason"),
            "error_steps": [],
            "bottleneck_steps": [],
            "drift_indicators": [],
        }

        entries = trajectory.get("entries", [])
        for entry in entries:
            # 错误步骤
            if not entry.get("tool_success", True):
                analysis["error_steps"].append({
                    "iteration": entry["iteration"],
                    "tool": entry.get("tool_name"),
                    "error": entry.get("tool_result", {}).get("error"),
                })

            # 瓶颈步骤（延迟 > 5s）
            if entry.get("latency_ms", 0) > 5000:
                analysis["bottleneck_steps"].append({
                    "iteration": entry["iteration"],
                    "latency_ms": entry["latency_ms"],
                    "tool": entry.get("tool_name"),
                })

        return analysis
```

---

<!-- chunk: 8. 最佳实践 -->## 8. 最佳实践

#<!-- chunk: 8.1 可观测性核心原则 -->## 8.1 可观测性核心原则

| 原则 | 说明 | 实践建议 |
|------|------|---------|
| **第一天就接入** | 不要等到出问题才加可观测性 | 项目初始化即集成 OTel/Langfuse |
| **全链路追踪** | Task → Loop → Think → Tool → Verify | 每个阶段都有独立 Span |
| **业务指标优先** | 任务完成率比 QPS 更重要 | 优先建立业务质量指标 |
| **成本透明** | 每个任务都要记录 Token 和成本 | 实时 Dashboard 展示成本趋势 |
| **告警精准** | 告警要可执行，不是噪声 | 区分 critical/warning，关联 Runbook |
| **轨迹可回放** | 问题复现需要完整执行记录 | 持久化 Trajectory |

#<!-- chunk: 8.2 反模式 -->## 8.2 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| **无追踪** | 出问题无法定位 | OTel 全链路追踪 |
| **指标过多** | 信息过载，无人关注 | 聚焦 10-15 个核心指标 |
| **告警风暴** | 告警太多等于没有告警 | 分级告警 + 静默策略 |
| **只看指标不看轨迹** | 知道"差"但不知道"为什么差" | 指标 + 轨迹结合分析 |

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | 可观测性基础架构 |
| [34 - 验证与质量门禁](./34-agent-harness-verification-quality.md) | 验证指标的来源 |
| [08 - 评测与可观测性](./08-agent-evaluation-observability.md) | Agent 评测基础理论 |
| [domain-06-observability](../domain-06-observability/) | 企业级监控告警体系 |

---

<!-- chunk: 参考来源 -->## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| Langfuse | Agent 追踪与评估平台 | 2025-2026 |
| OpenTelemetry | Semantic Conventions for GenAI | 2025-2026 |
| Anthropic | Agent 可观测性最佳实践 | 2026-02 |
| Datadog | LLM Observability 产品设计 | 2025 |

---

*本文档为 kudig-database 项目 topic-ai-agent 系列原创内容，深入展开 Agent Harness 可观测性体系。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README.md|AI Agent 工程专题]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/topic-ai-agent/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 34-agent-harness-verification-quality
- 35-agent-harness-security-constraints
- 37-agent-harness-multi-agent
- 38-agent-harness-performance-cost
