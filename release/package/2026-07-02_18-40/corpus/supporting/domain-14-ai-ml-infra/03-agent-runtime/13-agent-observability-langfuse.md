---
title: Agent可观测性
description: 'Langfuse/LangSmith/Phoenix/Weave可观测性平台：Trace追踪、成本监控、Prompt管理与OpenTelemetry集成'
summary: 'Langfuse/LangSmith/Phoenix/Weave可观测性平台：Trace追踪、成本监控、Prompt管理与OpenTelemetry集成'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- observability
- langfuse
- langsmith
- tracing
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
- 架构师
estimated_read_time: 20min
intent_queries:
- Agent可观测性 是什么
- 如何监控Agent执行
- Langfuse集成详解
trigger_keywords:
- langfuse
- langsmith
- observability
- tracing
- cost-monitoring
prerequisites:
- llm-basics
- kubernetes-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# Agent可观测性

## 概述

AI Agent的可观测性是生产运维的核心能力。与传统应用不同，Agent的执行路径由LLM动态决定，具有高度不确定性。一个Agent任务可能涉及多次LLM调用、多个工具执行、复杂的分支逻辑，传统的日志和指标无法满足调试和优化需求。

本文档介绍主流的Agent可观测性平台：Langfuse（开源LLM可观测性）、LangSmith（LangChain官方平台）、Phoenix/Arize（OpenInference追踪）、W&B Weave，以及如何通过OpenTelemetry实现统一的可观测性架构。

```
可观测性三大支柱:

Traces（追踪）:
  - Agent执行的完整调用链
  - LLM调用、工具执行、分支逻辑
  - 延迟分析和瓶颈定位

Metrics（指标）:
  - Token用量和成本
  - 延迟百分位数
  - 错误率和成功率

Logs（日志）:
  - 详细的输入输出记录
  - Prompt和Completion文本
  - 调试和审计用途
```

## Langfuse集成

### 核心概念

Langfuse提供Trace、Generation、Span三级追踪模型：

```
Trace（追踪）:
  - 一次完整的Agent执行
  - 包含所有子操作
  - 关联用户、会话、元数据

Generation（生成）:
  - 一次LLM调用
  - 记录Prompt、Completion、Token用量
  - 关联模型参数

Span（跨度）:
  - 通用的操作记录
  - 工具调用、检索操作、后处理
  - 支持嵌套结构
```

### LangChain集成

```python
from langfuse import Langfuse
from langfuse.callback import CallbackHandler
from langchain.agents import AgentExecutor, create_openai_tools_agent
from langchain_openai import ChatOpenAI

# 初始化Langfuse
langfuse = Langfuse(
    public_key="pk-...",
    secret_key="sk-...",
    host="https://cloud.langfuse.com",
)

# 创建回调处理器
langfuse_handler = CallbackHandler(
    trace_name="research-agent",
    user_id="user-123",
    session_id="session-456",
    metadata={
        "environment": "production",
        "version": "1.0.0",
    },
)

# 创建Agent
llm = ChatOpenAI(model="gpt-4o", temperature=0.1)
agent = create_openai_tools_agent(llm, tools, prompt)
executor = AgentExecutor(agent=agent, tools=tools, verbose=True)

# 执行Agent（自动追踪）
result = await executor.ainvoke(
    {"input": "研究Kubernetes网络策略"},
    config={"callbacks": [langfuse_handler]},
)

# 手动记录自定义Span
trace = langfuse_handler.get_trace()
with langfuse.span(
    name="post-processing",
    trace_id=trace.id,
) as span:
    processed = post_process(result["output"])
    span.output = processed
```

### LangGraph集成

```python
from langfuse import Langfuse
from langfuse.callback import CallbackHandler
from langgraph.graph import StateGraph, END

# 创建Langfuse回调
langfuse_handler = CallbackHandler(trace_name="langgraph-agent")

# 定义LangGraph
workflow = StateGraph(AgentState)

workflow.add_node("agent", agent_node)
workflow.add_node("tools", tool_node)
workflow.add_node("human_review", human_review_node)

workflow.set_entry_point("agent")
workflow.add_conditional_edges(
    "agent",
    should_continue,
    {
        "continue": "tools",
        "human_review": "human_review",
        "end": END,
    },
)
workflow.add_edge("tools", "agent")
workflow.add_edge("human_review", "agent")

graph = workflow.compile()

# 执行（自动追踪）
result = await graph.ainvoke(
    {"messages": [HumanMessage(content="分析系统日志")]},
    config={"callbacks": [langfuse_handler]},
)
```

### OpenAI SDK集成

```python
from langfuse import Langfuse
from langfuse.openai import openai  # 使用Langfuse包装的OpenAI客户端
from openai import AsyncOpenAI

# 自动追踪所有OpenAI调用
client = AsyncOpenAI()

# 使用装饰器追踪
from langfuse.decorators import observe

@observe(as_type="generation")
async def llm_call(messages: list, model: str = "gpt-4o"):
    """被Langfuse自动追踪的LLM调用"""
    response = await client.chat.completions.create(
        model=model,
        messages=messages,
    )
    return response.choices[0].message.content


@observe(name="agent-execution")
async def execute_agent(query: str):
    """被Langfuse追踪的Agent执行"""
    messages = [
        {"role": "system", "content": "你是一个研究助手。"},
        {"role": "user", "content": query},
    ]

    # 这个调用会被自动追踪
    response = await llm_call(messages)

    # 记录额外的元数据
    langfuse = Langfuse()
    span = langfuse.span(name="tool-execution")
    tool_result = await execute_search_tool(response)
    span.end(output=tool_result)

    return response
```

### Prompt管理

```python
from langfuse import Langfuse
from langfuse.prompt import PromptClient

langfuse = Langfuse()
prompt_client = PromptClient(langfuse)

# 从Langfuse获取Prompt（支持版本管理）
prompt = prompt_client.get_prompt(
    name="research-agent-system-prompt",
    version=3,
)

# 使用Prompt
system_prompt = prompt.compile(
    domain="kubernetes",
    expertise_level="expert",
)

# 执行Agent
response = await llm_call([
    {"role": "system", "content": system_prompt},
    {"role": "user", "content": query},
])

# 将Prompt版本关联到Trace
langfuse.generation(
    name="research-agent",
    model="gpt-4o",
    prompt=prompt,
    metadata={"prompt_version": prompt.version},
)
```

### 评估与测试

```python
from langfuse import Langfuse
from langfuse.evaluate import evaluate

langfuse = Langfuse()

# 定义评估函数
def correctness_evaluation(output: str, expected: str) -> float:
    """评估回答正确性"""
    # 可以使用LLM-as-Judge
    score = llm_judge(
        prompt=f"""
        评估以下回答的正确性:
        期望: {expected}
        实际: {output}
        
        返回0-1的分数。
        """,
    )
    return score

# 运行评估
evaluation = evaluate(
    dataset_id="research-qa-dataset",
    experiment_name="v1.2-evaluation",
    target_fn=lambda input: execute_agent(input["query"]),
    scores=[correctness_evaluation],
    metadata={"model": "gpt-4o", "temperature": 0.1},
)
```

## LangSmith

### Run Tree模型

```python
from langsmith import Client, traceable
from langsmith.run_helpers import trace

client = Client()

# 使用装饰器追踪
@traceable(
    name="agent-execution",
    run_type="chain",
    metadata={"version": "1.0"},
)
async def execute_agent(query: str) -> str:
    """LangSmith追踪的Agent执行"""
    # LLM调用会被自动追踪
    response = await llm_call(query)
    return response

# 手动创建Run Tree
from langsmith.run_trees import RunTree

run_tree = RunTree(
    name="agent-execution",
    run_type="chain",
    inputs={"query": "研究Kubernetes网络策略"},
)

# 添加子Run
child_run = run_tree.create_child(
    name="llm-call",
    run_type="llm",
    inputs={
        "messages": [...],
        "model": "gpt-4o",
    },
)

# 记录LLM输出
child_run.end(outputs={
    "content": response.content,
    "token_usage": response.usage,
})

# 提交Run
run_tree.post()
```

### 评估与Dataset

```python
from langsmith import Client
from langsmith.evaluation import evaluate

client = Client()

# 创建Dataset
dataset = client.create_dataset(
    dataset_name="agent-qa-evaluation",
    description="Agent问答质量评估数据集",
)

# 添加测试用例
client.create_example(
    inputs={"query": "什么是Kubernetes网络策略?"},
    outputs={"expected": "Kubernetes网络策略是..."},
    dataset_id=dataset.id,
)

# 定义评估函数
def evaluate_correctness(run, example):
    """评估回答正确性"""
    output = run.outputs.get("output", "")
    expected = example.outputs.get("expected", "")

    # LLM-as-Judge评估
    score = llm_judge(output, expected)
    return {"key": "correctness", "score": score}

def evaluate_completeness(run, example):
    """评估回答完整性"""
    output = run.outputs.get("output", "")
    # 检查关键点覆盖
    key_points = extract_key_points(example.outputs["expected"])
    coverage = calculate_coverage(output, key_points)
    return {"key": "completeness", "score": coverage}

# 运行评估
experiment_results = evaluate(
    target_fn=lambda inputs: execute_agent(inputs["query"]),
    data="agent-qa-evaluation",
    evaluators=[
        evaluate_correctness,
        evaluate_completeness,
    ],
    experiment_prefix="v1.2",
    metadata={
        "model": "gpt-4o",
        "temperature": 0.1,
    },
)
```

## Phoenix/Arize (OpenInference)

### OpenInference追踪

```python
import phoenix as px
from phoenix.otel import register
from openinference.instrumentation.langchain import LangChainInstrumentor
from openinference.instrumentation.openai import OpenAIInstrumentor

# 启动Phoenix
px.launch_app()

# 注册OpenTelemetry追踪
tracer_provider = register(
    project_name="agent-observability",
    endpoint="http://localhost:6006/v1/traces",
)

# 自动检测LangChain
LangChainInstrumentor().instrument(tracer_provider=tracer_provider)

# 自动检测OpenAI
OpenAIInstrumentor().instrument(tracer_provider=tracer_provider)

# Agent执行会被自动追踪
from langchain.agents import AgentExecutor

executor = AgentExecutor(agent=agent, tools=tools)
result = executor.invoke({"input": "分析系统性能"})
```

### 自定义Span

```python
from opentelemetry import trace

tracer = trace.get_tracer("agent-tracer")

@tracer.start_as_current_span("tool-execution")
async def execute_tool(tool_name: str, arguments: dict) -> dict:
    """自定义工具执行Span"""
    span = trace.get_current_span()

    span.set_attribute("tool.name", tool_name)
    span.set_attribute("tool.arguments", json.dumps(arguments))

    try:
        result = await tool_handlers[tool_name](**arguments)
        span.set_attribute("tool.status", "success")
        return result
    except Exception as e:
        span.set_attribute("tool.status", "error")
        span.set_attribute("tool.error", str(e))
        raise
```

## W&B Weave

### Weave集成

```python
import weave

# 初始化Weave
weave.init(project_name="agent-observability")

# 使用装饰器追踪
@weave.op()
async def llm_call(messages: list, model: str = "gpt-4o"):
    """Weave追踪的LLM调用"""
    client = AsyncOpenAI()
    response = await client.chat.completions.create(
        model=model,
        messages=messages,
    )
    return response.choices[0].message.content

@weave.op()
async def execute_tool(tool_name: str, arguments: dict):
    """Weave追踪的工具执行"""
    result = await tool_handlers[tool_name](**arguments)
    return result

@weave.op()
async def agent_execution(query: str):
    """Weave追踪的Agent执行"""
    messages = [
        {"role": "system", "content": "你是一个研究助手。"},
        {"role": "user", "content": query},
    ]

    response = await llm_call(messages)

    if should_use_tools(response):
        tool_results = []
        for tool_call in extract_tool_calls(response):
            result = await execute_tool(
                tool_call["name"],
                tool_call["arguments"],
            )
            tool_results.append(result)

        messages.append({"role": "assistant", "content": response})
        messages.append({"role": "tool", "content": json.dumps(tool_results)})
        response = await llm_call(messages)

    return response

# 执行（自动追踪）
result = await agent_execution("研究Kubernetes最佳实践")
```

### Weave评估

```python
import weave
from weave import Evaluation

# 定义评估数据集
dataset = [
    {
        "query": "什么是Kubernetes网络策略?",
        "expected": "Kubernetes网络策略是一种...",
    },
    {
        "query": "如何配置Pod安全策略?",
        "expected": "Pod安全策略配置步骤...",
    },
]

# 定义评估函数
@weave.op()
def evaluate_correctness(output: str, expected: str) -> dict:
    score = calculate_similarity(output, expected)
    return {"correctness": score}

@weave.op()
def evaluate_latency(output: str, expected: str, latency: float) -> dict:
    return {"latency_score": 1.0 if latency < 5.0 else 0.5}

# 创建评估
evaluation = Evaluation(
    dataset=dataset,
    scorers=[evaluate_correctness, evaluate_latency],
)

# 运行评估
scores = await evaluation.evaluate(agent_execution)
```

## OpenTelemetry for LLM Agent

### 统一追踪架构

```python
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import (
    OTLPSpanExporter,
)
from opentelemetry.sdk.resources import Resource

# 配置OpenTelemetry
resource = Resource.create({
    "service.name": "agent-service",
    "service.version": "1.0.0",
    "deployment.environment": "production",
})

provider = TracerProvider(resource=resource)
processor = BatchSpanProcessor(
    OTLPSpanExporter(endpoint="http://otel-collector:4317")
)
provider.add_span_processor(processor)
trace.set_tracer_provider(provider)

tracer = trace.get_tracer("agent-tracer")
```

### LLM语义约定

```python
from opentelemetry import trace

tracer = trace.get_tracer("agent-tracer")

@tracer.start_as_current_span("llm.completion")
async def traced_llm_call(
    messages: list,
    model: str,
    temperature: float = 0.1,
):
    """遵循OpenInference语义约定的LLM调用追踪"""
    span = trace.get_current_span()

    # 设置LLM特定属性
    span.set_attribute("llm.model_name", model)
    span.set_attribute("llm.temperature", temperature)
    span.set_attribute("llm.input_messages", json.dumps([
        {"role": m["role"], "content": m["content"][:100]}
        for m in messages
    ]))

    client = AsyncOpenAI()
    response = await client.chat.completions.create(
        model=model,
        messages=messages,
        temperature=temperature,
    )

    # 记录输出
    span.set_attribute("llm.output_message", response.choices[0].message.content[:200])
    span.set_attribute("llm.token_count.prompt", response.usage.prompt_tokens)
    span.set_attribute("llm.token_count.completion", response.usage.completion_tokens)
    span.set_attribute("llm.token_count.total", response.usage.total_tokens)
    span.set_attribute("llm.finish_reason", response.choices[0].finish_reason)

    return response
```

## 成本追踪与Token用量监控

### 成本计算器

```python
from dataclasses import dataclass
from datetime import datetime

@dataclass
class ModelPricing:
    """模型定价信息"""
    input_price_per_1k: float   # 每1K输入Token价格
    output_price_per_1k: float  # 每1K输出Token价格


# 模型定价表（示例）
MODEL_PRICING = {
    "gpt-4o": ModelPricing(input_price_per_1k=0.005, output_price_per_1k=0.015),
    "gpt-4o-mini": ModelPricing(input_price_per_1k=0.00015, output_price_per_1k=0.0006),
    "claude-3-5-sonnet": ModelPricing(input_price_per_1k=0.003, output_price_per_1k=0.015),
}


class CostTracker:
    """Agent成本追踪器"""

    def __init__(self):
        self.usage_records = []

    def record_usage(
        self,
        agent_id: str,
        model: str,
        input_tokens: int,
        output_tokens: int,
        metadata: dict = None,
    ):
        """记录Token用量"""
        pricing = MODEL_PRICING.get(model)
        if not pricing:
            raise ValueError(f"Unknown model: {model}")

        input_cost = (input_tokens / 1000) * pricing.input_price_per_1k
        output_cost = (output_tokens / 1000) * pricing.output_price_per_1k
        total_cost = input_cost + output_cost

        record = UsageRecord(
            agent_id=agent_id,
            model=model,
            input_tokens=input_tokens,
            output_tokens=output_tokens,
            input_cost=input_cost,
            output_cost=output_cost,
            total_cost=total_cost,
            timestamp=datetime.utcnow(),
            metadata=metadata or {},
        )

        self.usage_records.append(record)
        return record

    def get_total_cost(
        self,
        agent_id: str = None,
        start_time: datetime = None,
        end_time: datetime = None,
    ) -> float:
        """获取总成本"""
        filtered = self.usage_records

        if agent_id:
            filtered = [r for r in filtered if r.agent_id == agent_id]
        if start_time:
            filtered = [r for r in filtered if r.timestamp >= start_time]
        if end_time:
            filtered = [r for r in filtered if r.timestamp <= end_time]

        return sum(r.total_cost for r in filtered)

    def get_usage_summary(
        self,
        agent_id: str = None,
    ) -> dict:
        """获取用量摘要"""
        filtered = self.usage_records
        if agent_id:
            filtered = [r for r in filtered if r.agent_id == agent_id]

        return {
            "total_calls": len(filtered),
            "total_input_tokens": sum(r.input_tokens for r in filtered),
            "total_output_tokens": sum(r.output_tokens for r in filtered),
            "total_cost": sum(r.total_cost for r in filtered),
            "by_model": self._group_by_model(filtered),
        }

    def _group_by_model(self, records: list) -> dict:
        """按模型分组统计"""
        by_model = {}
        for record in records:
            if record.model not in by_model:
                by_model[record.model] = {
                    "calls": 0,
                    "input_tokens": 0,
                    "output_tokens": 0,
                    "cost": 0,
                }
            by_model[record.model]["calls"] += 1
            by_model[record.model]["input_tokens"] += record.input_tokens
            by_model[record.model]["output_tokens"] += record.output_tokens
            by_model[record.model]["cost"] += record.total_cost
        return by_model
```

### Prometheus指标

```python
from prometheus_client import Counter, Histogram, Gauge

# 定义Agent指标
agent_llm_calls = Counter(
    "agent_llm_calls_total",
    "Total LLM API calls",
    ["agent_id", "model", "status"],
)

agent_llm_tokens = Counter(
    "agent_llm_tokens_total",
    "Total tokens used",
    ["agent_id", "model", "direction"],  # direction: input/output
)

agent_llm_latency = Histogram(
    "agent_llm_latency_seconds",
    "LLM call latency",
    ["agent_id", "model"],
    buckets=[0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0],
)

agent_cost = Counter(
    "agent_cost_dollars",
    "Total cost in dollars",
    ["agent_id", "model"],
)

agent_tool_calls = Counter(
    "agent_tool_calls_total",
    "Total tool calls",
    ["agent_id", "tool_name", "status"],
)

agent_active_tasks = Gauge(
    "agent_active_tasks",
    "Currently active agent tasks",
    ["agent_id"],
)


# 使用示例
async def monitored_llm_call(
    agent_id: str,
    messages: list,
    model: str = "gpt-4o",
):
    """带Prometheus指标的LLM调用"""
    agent_active_tasks.labels(agent_id=agent_id).inc()

    with agent_llm_latency.labels(
        agent_id=agent_id,
        model=model,
    ).time():
        try:
            response = await client.chat.completions.create(
                model=model,
                messages=messages,
            )

            agent_llm_calls.labels(
                agent_id=agent_id,
                model=model,
                status="success",
            ).inc()

            agent_llm_tokens.labels(
                agent_id=agent_id,
                model=model,
                direction="input",
            ).inc(response.usage.prompt_tokens)

            agent_llm_tokens.labels(
                agent_id=agent_id,
                model=model,
                direction="output",
            ).inc(response.usage.completion_tokens)

            # 记录成本
            cost = calculate_cost(
                model,
                response.usage.prompt_tokens,
                response.usage.completion_tokens,
            )
            agent_cost.labels(
                agent_id=agent_id,
                model=model,
            ).inc(cost)

            return response

        except Exception as e:
            agent_llm_calls.labels(
                agent_id=agent_id,
                model=model,
                status="error",
            ).inc()
            raise
        finally:
            agent_active_tasks.labels(agent_id=agent_id).dec()
```

### Grafana Dashboard

```yaml
# Grafana Dashboard配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: agent-dashboard
data:
  dashboard.json: |
    {
      "dashboard": {
        "title": "Agent Observability",
        "panels": [
          {
            "title": "LLM Calls per Second",
            "targets": [
              {
                "expr": "rate(agent_llm_calls_total[5m])"
              }
            ]
          },
          {
            "title": "Token Usage",
            "targets": [
              {
                "expr": "rate(agent_llm_tokens_total[5m])"
              }
            ]
          },
          {
            "title": "Cost per Hour",
            "targets": [
              {
                "expr": "increase(agent_cost_dollars[1h])"
              }
            ]
          },
          {
            "title": "P95 Latency",
            "targets": [
              {
                "expr": "histogram_quantile(0.95, rate(agent_llm_latency_seconds_bucket[5m]))"
              }
            ]
          }
        ]
      }
    }
```

---

*Agent可观测性是理解和优化Agent行为的关键，通过Langfuse/LangSmith等平台可以实现从Trace到成本的全链路监控。*
