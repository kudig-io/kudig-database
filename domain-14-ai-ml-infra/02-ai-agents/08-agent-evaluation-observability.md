---
title: Agent 评测体系与可观测性 (domain-14-ai-ml-infra)
description: 'description: ''**文档类型**: 工程质量专题 | **最后更新**: 2026-03 | **关键词**: Agent 评测, LLM-as-Judge,'
category: general
tags:
- ai
- ai-agent
- observability
- prometheus
- grafana
- helm
- postgresql
- job
- ingress
- llm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- Agent 评测体系与可观测性 是什么
- 如何 Agent 评测体系与可观测性
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- Agent
- 评测体系与可观测性
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- observability-basics
created: "2026-05-23"
---

title: Agent 评测体系与可观测性
description: '**文档类型**: 工程质量专题 | **最后更新**: 2026-03 | **关键词**: Agent 评测, LLM-as-Judge,
  RAGAS, Langfuse, LangSmith, Phoenix, 轨迹评估, [[OpenTelemetry|OpenTelemetry]], 可观测性, Agent 指标'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[Prometheus|prometheus]]
- grafana
- [[Helm|helm]]
- postgresql
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- Agent 评测体系与可观测性 是什么
- 如何 Agent 评测体系与可观测性
trigger_keywords:
- Agent
- 评测体系与可观测性
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

# Agent 评测体系与可观测性

> **文档类型**: 工程质量专题 | **最后更新**: 2026-03 | **关键词**: Agent 评测, LLM-as-Judge, RAGAS, Langfuse, LangSmith, Phoenix, 轨迹评估, OpenTelemetry, 可观测性, Agent 指标

---

<!-- chunk: 概述 -->## 概述

没有评测的 Agent 是黑盒。评测体系解决"Agent 质量是否达标"的问题，可观测性解决"Agent 为什么这么做"的问题。本文覆盖从单轮问答到多步轨迹的全面评测框架、RAGAS/LLM-as-Judge 实施方法、LangSmith/Langfuse/Phoenix 的配置与使用，以及生产 Agent 的关键监控指标体系。

---

<!-- chunk: 1. Agent 评测体系全景 -->## 1. Agent 评测体系全景

#<!-- chunk: 1.1 评测维度 -->## 1.1 评测维度

```
Agent 评测四维度
│
├── 1. 准确性（Correctness）
│      答案是否正确、事实是否准确
│      指标: 准确率、召回率、F1
│
├── 2. 效率（Efficiency）
│      工具调用数量、Token 消耗、完成时间
│      指标: 平均步骤数、Token/任务、延迟 P50/P95
│
├── 3. 可靠性（Reliability）
│      成功率、错误率、幻觉率
│      指标: 任务完成率、工具调用成功率、重试率
│
└── 4. 安全性（Safety）
       有害输出率、提示注入抵抗、合规遵守
       指标: 安全拦截率、PII 泄露率
```

#<!-- chunk: 1.2 评测粒度层次 -->## 1.2 评测粒度层次

| 层次 | 评测对象 | 方法 | 工具 |
|------|---------|------|------|
| **单轮问答** | 单次 LLM 调用的质量 | 人工/自动评分 | RAGAS |
| **工具调用** | 单次工具选择和参数的准确性 | 对比预期工具调用 | 自定义测试集 |
| **轨迹评估** | 整个 Agent 执行路径 | 轨迹 vs 最优路径 | LangSmith |
| **端到端** | 用户目标是否最终达成 | 任务完成率 | 人工标注 + 自动化 |

---

<!-- chunk: 2. RAGAS 评估框架 -->## 2. RAGAS 评估框架

#<!-- chunk: 2.1 核心指标详解 -->## 2.1 核心指标详解

```python
from ragas import evaluate
from ragas.metrics import (
    faithfulness,           # 忠实度
    answer_relevancy,       # 答案相关性
    context_precision,      # 上下文精确率
    context_recall,         # 上下文召回率
    answer_correctness,     # 答案正确性（需 ground_truth）
    answer_similarity,      # 答案语义相似度
)
from ragas.metrics.critique import harmfulness  # 有害性检测

# 各指标含义：
METRIC_EXPLANATIONS = {
    "faithfulness": """
        答案中的每个声明是否都能在检索到的上下文中找到支撑。
        计算方式：能在上下文中验证的声明数 / 总声明数
        目标值：> 0.90（K8s 运维场景，不能有幻觉）
    """,
    "answer_relevancy": """
        答案是否真正回答了问题（而非偏题）。
        计算方式：从答案反向生成问题，与原问题的语义相似度
        目标值：> 0.80
    """,
    "context_precision": """
        检索到的上下文中，有多少比例是真正有用的。
        衡量检索的"噪声"程度
        目标值：> 0.75
    """,
    "context_recall": """
        回答问题所需的信息是否都被检索到了。
        目标值：> 0.70
    """,
}
```

#<!-- chunk: 2.2 完整 RAGAS 评估 Pipeline -->## 2.2 完整 RAGAS 评估 Pipeline

```python
from datasets import Dataset
from ragas import evaluate
from ragas.llms import LangchainLLMWrapper
from ragas.embeddings import LangchainEmbeddingsWrapper
from langchain_openai import ChatOpenAI, OpenAIEmbeddings
import pandas as pd

class RAGASEvaluator:
    def __init__(self, eval_llm_model: str = "gpt-4o"):
        # 评估用 LLM（建议用强模型）
        self.eval_llm = LangchainLLMWrapper(
            ChatOpenAI(model=eval_llm_model, temperature=0)
        )
        self.eval_embeddings = LangchainEmbeddingsWrapper(
            OpenAIEmbeddings(model="text-embedding-3-small")
        )
    
    def evaluate_rag_pipeline(
        self,
        test_cases: list[dict],
        rag_pipeline,
    ) -> pd.DataFrame:
        """
        test_cases 格式:
        [
          {
            "question": "Pod Pending 最常见的原因？",
            "ground_truth": "常见原因：1. 资源不足 2. 节点亲和性...",
          },
          ...
        ]
        """
        # 运行 RAG Pipeline 生成答案
        results = []
        for case in test_cases:
            rag_result = rag_pipeline.query(case["question"])
            results.append({
                "question": case["question"],
                "answer": rag_result["answer"],
                "contexts": [rag_result["sources"]],
                "ground_truth": case.get("ground_truth", ""),
            })
        
        # 构建评估数据集
        dataset = Dataset.from_list(results)
        
        # 选择适用的指标
        metrics = [faithfulness, answer_relevancy, context_precision]
        if any(r.get("ground_truth") for r in results):
            metrics.extend([context_recall, answer_correctness])
        
        # 执行评估
        eval_result = evaluate(
            dataset=dataset,
            metrics=metrics,
            llm=self.eval_llm,
            embeddings=self.eval_embeddings,
        )
        
        # 生成报告
        df = eval_result.to_pandas()
        
        print("\n=== RAG 评估报告 ===")
        print(f"Faithfulness:       {eval_result['faithfulness']:.3f}")
        print(f"Answer Relevancy:   {eval_result['answer_relevancy']:.3f}")
        print(f"Context Precision:  {eval_result['context_precision']:.3f}")
        
        # 找出表现差的用例（分数低于 0.7）
        poor_cases = df[df["faithfulness"] < 0.7]
        if len(poor_cases) > 0:
            print(f"\n警告：{len(poor_cases)} 个用例 faithfulness < 0.7，需要重点检查：")
            print(poor_cases"question", "faithfulness".to_string())
        
        return df
```

---

<!-- chunk: 3. LLM-as-Judge：自动化评测 -->## 3. LLM-as-Judge：自动化评测

#<!-- chunk: 3.1 基本原理 -->## 3.1 基本原理

使用 LLM 作为评估者（Judge），对 Agent 的输出质量进行打分：

```python
from enum import Enum

class JudgeScore(Enum):
    EXCELLENT = 5  # 完美回答，无任何问题
    GOOD = 4       # 良好，有轻微不足
    ACCEPTABLE = 3 # 可接受，有明显改进空间
    POOR = 2       # 差，有重大问题
    FAILING = 1    # 不合格，需要完全重写

class LLMJudge:
    """LLM-as-Judge 评测器"""
    
    JUDGE_PROMPT_TEMPLATE = """
    你是 Kubernetes 运维领域的专家评委。请评估以下 AI Agent 回答的质量。
    
    【问题】
    {question}
    
    【参考答案（Ground Truth）】
    {ground_truth}
    
    【Agent 回答】
    {agent_answer}
    
    请从以下维度评分（1-5分，5分最高）：
    
    1. **技术准确性**（0-5）：技术内容是否正确，有无事实错误
    2. **完整性**（0-5）：是否覆盖了问题的核心方面
    3. **可操作性**（0-5）：给出的命令/步骤是否可以实际执行
    4. **安全性**（0-5）：是否有潜在危险的建议（如误删数据）
    
    输出格式：
    {{
        "technical_accuracy": <1-5>,
        "completeness": <1-5>,
        "actionability": <1-5>,
        "safety": <1-5>,
        "overall_score": <1-5>,
        "reasoning": "<评分理由，100字以内>",
        "critical_issues": ["<严重问题1>", "<严重问题2>"],
        "improvement_suggestions": ["<建议1>", "<建议2>"]
    }}
    """
    
    def __init__(self, judge_llm):
        self.judge = judge_llm
    
    def evaluate(
        self,
        question: str,
        agent_answer: str,
        ground_truth: str = "",
    ) -> dict:
        """评估单个回答"""
        prompt = self.JUDGE_PROMPT_TEMPLATE.format(
            question=question,
            ground_truth=ground_truth or "（无参考答案）",
            agent_answer=agent_answer,
        )
        
        response = self.judge.invoke(prompt)
        
        try:
            scores = json.loads(response.content)
        except json.JSONDecodeError:
            # 解析失败时的降级处理
            scores = self._parse_scores_fallback(response.content)
        
        return scores
    
    def batch_evaluate(
        self,
        test_cases: list[dict],
        batch_size: int = 5,
    ) -> pd.DataFrame:
        """批量评估"""
        results = []
        
        for i in range(0, len(test_cases), batch_size):
            batch = test_cases[i:i+batch_size]
            
            for case in batch:
                score = self.evaluate(
                    question=case["question"],
                    agent_answer=case["agent_answer"],
                    ground_truth=case.get("ground_truth", ""),
                )
                results.append({
                    "question": case["question"],
                    **score
                })
        
        df = pd.DataFrame(results)
        
        print(f"\n=== LLM-as-Judge 评估结果 ===")
        print(f"平均分：{df['overall_score'].mean():.2f} / 5.0")
        print(f"达标率（>=3分）：{(df['overall_score'] >= 3).mean():.1%}")
        
        return df
```

#<!-- chunk: 3.2 轨迹评估（Trajectory Evaluation） -->## 3.2 轨迹评估（Trajectory Evaluation）

评估 Agent 的**执行路径**，而非仅最终答案：

```python
@dataclass
class AgentTrajectory:
    """Agent 执行轨迹"""
    task: str
    steps: list[dict]  # [{"thought": "...", "action": "...", "observation": "..."}]
    final_answer: str
    total_steps: int
    total_tokens: int
    success: bool

class TrajectoryEvaluator:
    """评估 Agent 执行轨迹的质量"""
    
    def evaluate_trajectory(
        self,
        trajectory: AgentTrajectory,
        optimal_step_count: int = None,
    ) -> dict:
        """多维度评估轨迹"""
        
        scores = {}
        
        # 1. 效率评分（步骤数）
        if optimal_step_count:
            efficiency = min(optimal_step_count / trajectory.total_steps, 1.0)
            scores["efficiency"] = efficiency
        
        # 2. 工具调用质量
        tool_calls = [s for s in trajectory.steps if s.get("action")]
        scores["tool_selection_accuracy"] = self._evaluate_tool_selection(tool_calls)
        
        # 3. 推理连贯性
        scores["reasoning_coherence"] = self._evaluate_reasoning_chain(trajectory.steps)
        
        # 4. 错误恢复能力
        errors = [s for s in trajectory.steps if "error" in str(s.get("observation", "")).lower()]
        scores["error_recovery"] = 1.0 if not errors else self._evaluate_recovery(errors, trajectory)
        
        # 5. 任务完成
        scores["task_completion"] = 1.0 if trajectory.success else 0.0
        
        # 综合得分
        weights = {
            "efficiency": 0.2,
            "tool_selection_accuracy": 0.3,
            "reasoning_coherence": 0.2,
            "error_recovery": 0.1,
            "task_completion": 0.2,
        }
        
        scores["overall"] = sum(
            scores.get(k, 0) * w for k, w in weights.items()
        )
        
        return scores
    
    def _evaluate_tool_selection(self, tool_calls: list) -> float:
        """评估工具选择是否合理"""
        if not tool_calls:
            return 1.0
        
        issues = 0
        for i, call in enumerate(tool_calls):
            action = call.get("action", "")
            observation = str(call.get("observation", ""))
            
            # 检查是否重复调用了相同工具（浪费）
            if i > 0 and action == tool_calls[i-1].get("action"):
                issues += 1
            
            # 检查工具调用是否返回了明显不相关的结果
            # （简化：实际需要 LLM 评估）
        
        return max(0.0, 1.0 - issues * 0.2)
```

---

<!-- chunk: 4. 可观测性平台 -->## 4. 可观测性平台

#<!-- chunk: 4.1 Langfuse（推荐：开源可自托管） -->## 4.1 Langfuse（推荐：开源可自托管）

```python
from langfuse import Langfuse
from langfuse.decorators import observe, langfuse_context

# 初始化
langfuse = Langfuse(
    public_key="pk-lf-...",
    secret_key="sk-lf-...",
    host="http://langfuse.your-domain.com",  # 自托管实例
)

# 方式1：使用装饰器（最简单）
@observe(name="k8s_diagnosis_agent")
def run_diagnosis_agent(problem: str) -> str:
    """整个函数的执行会被追踪"""
    
    # 更新 span 元数据
    langfuse_context.update_current_trace(
        name=f"诊断: {problem[:50]}",
        tags=["production", "k8s-ops"],
        user_id="ops-engineer-001",
    )
    
    # 执行 Agent
    result = agent_executor.invoke({"input": problem})
    
    # 记录评估分数
    langfuse_context.score_current_trace(
        name="task_completion",
        value=1 if result["success"] else 0,
    )
    
    return result["output"]

# 方式2：手动追踪（更细粒度控制）
def traced_tool_call(tool_name: str, args: dict, trace_id: str) -> str:
    span = langfuse.span(
        trace_id=trace_id,
        name=f"tool:{tool_name}",
        input=args,
    )
    
    try:
        result = execute_tool(tool_name, args)
        span.end(output=result, level="DEFAULT")
        return result
    except Exception as e:
        span.end(
            output=str(e),
            level="ERROR",
            status_message=f"工具调用失败: {type(e).__name__}"
        )
        raise
```

#<!-- chunk: 4.2 Langfuse K8s 自托管部署 -->## 4.2 Langfuse K8s 自托管部署

```yaml
# Langfuse Helm 部署
helm repo add langfuse https://langfuse.github.io/langfuse-k8s
helm install langfuse langfuse/langfuse \
  --namespace ai-observability \
  --create-namespace \
  --set nextauth.secret="your-random-secret" \
  --set langfuse.salt="your-random-salt" \
  --set postgresql.auth.password="your-db-password" \
  --set ingress.enabled=true \
  --set ingress.hosts[0].host="langfuse.your-domain.com" \
  -f langfuse-values.yaml
```

```yaml
# langfuse-values.yaml
langfuse:
  nextPublicSignUpDisabled: "true"  # 生产环境禁止公开注册
  enableExperimentalFeatures: "true"
  
postgresql:
  enabled: true
  primary:
    persistence:
      size: 50Gi
      storageClass: fast-ssd

clickhouse:
  enabled: true  # 用于高性能分析查询
  
resources:
  requests:
    memory: "1Gi"
    cpu: "500m"
  limits:
    memory: "2Gi"
    cpu: "1"
```

#<!-- chunk: 4.3 LangSmith（OpenAI 生态最完整） -->## 4.3 LangSmith（OpenAI 生态最完整）

```python
from langchain.callbacks.tracers import LangChainTracer
from langsmith import Client

# 配置 LangSmith
import os
os.environ["LANGCHAIN_TRACING_V2"] = "true"
os.environ["LANGCHAIN_API_KEY"] = "ls__..."
os.environ["LANGCHAIN_PROJECT"] = "kudig-k8s-agent"

# LangChain 会自动追踪（无需额外代码）
result = agent_executor.invoke({"input": "诊断 Pod Pending 问题"})

# 手动提交评估
client = Client()

def submit_evaluation(run_id: str, score: float, comment: str):
    client.create_feedback(
        run_id=run_id,
        key="technical_accuracy",
        score=score,
        comment=comment,
        source_info={"evaluator": "human_expert"},
    )
```

#<!-- chunk: 4.4 Phoenix（Arize）：本地可观测性 -->## 4.4 Phoenix（Arize）：本地可观测性

```python
import phoenix as px
from phoenix.trace.langchain import LangChainInstrumentor

# 启动本地 Phoenix 服务
px.launch_app()

# 自动追踪 LangChain 调用
LangChainInstrumentor().instrument()

# 执行后在 http://localhost:6006 查看追踪
result = agent_executor.invoke({"input": "问题描述"})
```

---

<!-- chunk: 5. 生产监控指标体系 -->## 5. 生产监控指标体系

#<!-- chunk: 5.1 Prometheus 指标定义 -->## 5.1 Prometheus 指标定义

```python
from prometheus_client import Counter, Histogram, Gauge, Summary

# Agent 业务指标
agent_requests_total = Counter(
    'agent_requests_total',
    'Agent 处理的总请求数',
    ['agent_type', 'status', 'problem_type']
)

agent_task_duration_seconds = Histogram(
    'agent_task_duration_seconds',
    'Agent 任务执行时间（秒）',
    ['agent_type'],
    buckets=[0.5, 1, 2, 5, 10, 30, 60, 120]
)

agent_tool_calls_total = Counter(
    'agent_tool_calls_total',
    'Agent 工具调用总次数',
    ['tool_name', 'status']
)

agent_llm_tokens_total = Counter(
    'agent_llm_tokens_total',
    'LLM Token 总消耗',
    ['model', 'token_type']  # token_type: input/output
)

agent_iteration_count = Histogram(
    'agent_iteration_count',
    'Agent 单次任务的迭代次数',
    ['agent_type'],
    buckets=[1, 2, 3, 5, 8, 10, 15, 20]
)

agent_hallucination_rate = Gauge(
    'agent_hallucination_rate',
    'Agent 幻觉率（滑动窗口）',
    ['agent_type']
)

agent_task_success_rate = Gauge(
    'agent_task_success_rate',
    '任务成功率（最近 100 次）',
    ['agent_type']
)

# 在 Agent 执行中埋点
class InstrumentedAgent:
    def run(self, task: str, agent_type: str = "general") -> dict:
        start_time = time.time()
        
        try:
            result = self._execute(task)
            status = "success" if result.get("success") else "failed"
        except Exception:
            status = "error"
            raise
        finally:
            # 记录指标
            duration = time.time() - start_time
            problem_type = classify_problem(task)
            
            agent_requests_total.labels(
                agent_type=agent_type,
                status=status,
                problem_type=problem_type
            ).inc()
            
            agent_task_duration_seconds.labels(agent_type=agent_type).observe(duration)
        
        return result
```

#<!-- chunk: 5.2 关键告警规则 -->## 5.2 关键告警规则

```yaml
# Prometheus AlertManager 规则
groups:
  - name: agent_alerts
    rules:
    
    # 任务成功率过低
    - alert: AgentTaskSuccessRateLow
      expr: agent_task_success_rate < 0.7
      for: 10m
      labels:
        severity: critical
      annotations:
        summary: "Agent 任务成功率过低（{{ $value | humanizePercentage }}）"
        description: "{{ $labels.agent_type }} 的成功率已低于 70%，需要立即检查"
    
    # LLM 响应延迟高
    - alert: LLMHighLatency
      expr: |
        histogram_quantile(0.95, 
          rate(agent_task_duration_seconds_bucket[5m])
        ) > 30
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Agent 响应 P95 延迟超过 30 秒"
    
    # Token 消耗异常
    - alert: TokenConsumptionSpike
      expr: |
        rate(agent_llm_tokens_total[5m]) > 
        rate(agent_llm_tokens_total[1h] offset 1h) * 3
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Token 消耗异常增长（超过历史基线 3 倍）"
        description: "可能存在无限循环或异常大量请求"
    
    # 工具调用失败率高
    - alert: ToolCallFailureRateHigh
      expr: |
        rate(agent_tool_calls_total{status="error"}[5m]) /
        rate(agent_tool_calls_total[5m]) > 0.3
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "工具调用失败率超过 30%"
```

#<!-- chunk: 5.3 Grafana Dashboard 关键面板 -->## 5.3 Grafana Dashboard 关键面板

```
Agent 监控 Dashboard 推荐面板:

┌─────────────────────────────────────────┐
│  任务成功率  │  平均延迟  │  Token/小时  │
│   96.3%    │  4.2s     │  125K/h     │
├─────────────────────────────────────────┤
│      任务完成时间分布（P50/P95/P99）       │
│  P50: 2.1s  P95: 8.3s  P99: 24s       │
├─────────────────────────────────────────┤
│  工具调用统计      │  按问题类型分布        │
│  - kubectl: 45%   │  - 网络: 32%          │
│  - rag_query: 30% │  - 存储: 18%          │
│  - search: 25%    │  - 调度: 28%          │
├─────────────────────────────────────────┤
│         按时间的 Token 消耗趋势           │
│  [成本监控图表]                           │
├─────────────────────────────────────────┤
│     最近失败任务列表（点击查看 Trace）      │
└─────────────────────────────────────────┘
```

---

<!-- chunk: 6. 自动化评估 CI/CD 集成 -->## 6. 自动化评估 CI/CD 集成

```yaml
# GitHub Actions：Agent 质量门禁
name: Agent Quality Gate

on:
  pull_request:
    paths:
      - 'agent/**'
      - 'prompts/**'

jobs:
  evaluate:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4
    
    - name: Run Agent Evaluation
      env:
        OPENAI_API_KEY: ${{ secrets.OPENAI_API_KEY }}
        LANGFUSE_PUBLIC_KEY: ${{ secrets.LANGFUSE_PUBLIC_KEY }}
      run: |
        python scripts/run_agent_evaluation.py \
          --test-set tests/agent_test_cases.json \
          --min-faithfulness 0.85 \
          --min-success-rate 0.90 \
          --output evaluation_report.json
    
    - name: Check Quality Gate
      run: |
        python scripts/check_quality_gate.py \
          --report evaluation_report.json \
          --fail-on-regression
    
    - name: Upload Evaluation Report
      uses: actions/upload-artifact@v4
      with:
        name: evaluation-report
        path: evaluation_report.json
```

```python
# scripts/check_quality_gate.py
import json
import sys

def check_quality_gate(report_path: str, fail_on_regression: bool = True):
    with open(report_path) as f:
        report = json.load(f)
    
    THRESHOLDS = {
        "faithfulness": 0.85,
        "answer_relevancy": 0.80,
        "task_completion_rate": 0.90,
        "hallucination_rate": 0.05,
    }
    
    failed_metrics = []
    for metric, threshold in THRESHOLDS.items():
        if metric in report:
            actual = report[metric]
            if metric == "hallucination_rate":
                if actual > threshold:
                    failed_metrics.append(f"{metric}: {actual:.3f} > {threshold}")
            else:
                if actual < threshold:
                    failed_metrics.append(f"{metric}: {actual:.3f} < {threshold}")
    
    if failed_metrics:
        print("Quality Gate FAILED:")
        for failure in failed_metrics:
            print(f"  ✗ {failure}")
        if fail_on_regression:
            sys.exit(1)
    else:
        print("Quality Gate PASSED")
        for metric in THRESHOLDS:
            print(f"  ✓ {metric}: {report.get(metric, 'N/A'):.3f}")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", required=True)
    parser.add_argument("--fail-on-regression", action="store_true")
    args = parser.parse_args()
    check_quality_gate(args.report, args.fail_on_regression)
```

---

<!-- chunk: 7. 最佳实践与反模式 -->## 7. 最佳实践与反模式

#<!-- chunk: 最佳实践 -->## 最佳实践

- **评测集要真实**：从生产日志中采样真实问题，而非人工构造理想化用例
- **持续评估**：每次代码/提示词变更后自动运行评估，防止质量回退
- **分层监控**：同时监控系统级指标（延迟/成本）和业务级指标（准确性/完成率）
- **可观测性从第一天开始**：生产上线时就接入追踪，而非出问题后再补
- **用 LLM-as-Judge 节省人力**：人工评分 10% 作为标定集，其余用 LLM Judge

#<!-- chunk: 反模式 -->## 反模式

- **只评估 Happy Path**：测试集全是简单问题，上线后遇到边缘情况崩溃
- **Faithfulness 忽略**：只看最终准确率，不检查是否有幻觉——在 K8s 运维场景幻觉会造成真实问题
- **无基线对比**：没有记录历史评估分数，无法判断版本升级是进步还是退步
- **评估用同一个模型**：用 GPT-4o 生成答案又用 GPT-4o 评估，存在同质偏见

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [04 - RAG 检索](./04-rag-knowledge-retrieval.md) | RAGAS 评估 RAG 管道质量 |
| [09 - 生产部署](./09-production-deployment-guide.md) | Prometheus/Grafana 在 K8s 的配置 |
| [domain-20-enterprise-monitoring-alerting](../domain-06-observability/) | 企业级监控告警系统 |
| [domain-06-observability](../domain-06-observability/) | 可观测性基础设施 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent KUDIG Database — Global MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README.md|AI Agent 工程专题]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/topic-ai-agent/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails.md|安全护栏、提示注入防护与合规]]
- [[domain-14-ai-ml-infra/topic-ai-agent/11-cost-latency-optimization.md|成本与延迟优化策略]]

## Related

- 48-openclaw-skill-mechanism
- 13-trusted-agent-system-fiscal-plan
- 39-agent-harness-testing-benchmark
- 42-model-harness-compatibility-matrix
- 12-enterprise-case-studies
- 02-llm-foundation-models
- 23-agent-cli-fundamentals
- 50-openclaw-identity-mechanism
- 01-ai-agent-fundamentals
- 03-agent-frameworks-comparison
- 47-openclaw-tools-mechanism
- 37-agent-harness-multi-agent
- 20-agentscope-multi-agent-orchestration
- 40-agent-harness-production-maturity
- 25-agent-cli-mcp-integration
- 26-agent-cli-development-workflow
- 07-memory-context-management
- 11-cost-latency-optimization
- 44-openclaw-soul-mechanism
- 45-openclaw-user-mechanism
- 31-agent-harness-loop-execution
- 27-agent-cli-security-governance
- 06-multi-agent-orchestration
- 41-react-harness-identification-guide

## See Also

- 06-multi-agent-orchestration
- 07-memory-context-management
- 09-production-deployment-guide
- 10-security-guardrails
