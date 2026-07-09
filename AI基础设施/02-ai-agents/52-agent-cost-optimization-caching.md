---
title: Agent 成本优化与缓存
description: 'LLM请求缓存、Token用量优化、模型路由与成本监控的完整实现方案'
summary: 'LLM请求缓存、Token用量优化、模型路由与成本监控的完整实现方案'
category: platform-engineering
tags:
- ai-agent
- cost-optimization
- llm-cache
- token-optimization
- model-routing
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 所有工程师
- 架构师
- SRE
estimated_read_time: 15min
intent_queries:
- Agent 成本优化 是什么
- 如何 降低 LLM 调用成本
trigger_keywords:
- Agent 成本
- LLM 缓存
- Token 优化
- 模型路由
- 成本监控
prerequisites:
- kubectl-basics
- microservice-basics
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Agent 成本优化与缓存

## 1. 概述

LLM 调用成本是 AI Agent 运营的主要支出。本文档覆盖四种核心成本优化策略：请求缓存、Token 用量优化、智能模型路由和成本监控告警，帮助将 LLM 调用成本降低 50-80%。

## 2. 成本优化全景

```
LLM 成本优化策略:

1. 缓存优化 (Cache)
   → 语义缓存: 相似问题复用回答
   → 精确缓存: 完全相同请求直接返回
   → 预期节省: 30-50%

2. Token 优化 (Token)
   → Prompt 压缩: 移除冗余上下文
   → 上下文裁剪: 智能选择历史消息
   → 预期节省: 20-40%

3. 模型路由 (Routing)
   → 简单问题 → 小模型 (GPT-3.5)
   → 复杂问题 → 大模型 (GPT-4)
   → 预期节省: 40-60%

4. 请求合并 (Batching)
   → 相似请求批量处理
   → 异步非关键请求
   → 预期节省: 10-20%

综合策略: 缓存 + Token + 路由 → 50-80% 成本降低
```

## 3. LLM 请求缓存

### 3.1 精确缓存 (Exact Cache)

```python
# 基于 Redis 的精确缓存
import hashlib
import json
import redis
from typing import Optional, Dict

class LLMExactCache:
    """精确匹配缓存：相同输入直接返回缓存结果"""

    def __init__(self, redis_url: str, ttl: int = 3600):
        self.redis = redis.from_url(redis_url)
        self.ttl = ttl

    def _build_cache_key(self, model: str, messages: list, **kwargs) -> str:
        """构建缓存键"""
        cache_input = {
            "model": model,
            "messages": messages,
            **{k: v for k, v in kwargs.items() if k in ["temperature", "max_tokens"]}
        }
        # 移除不稳定的参数
        cache_str = json.dumps(cache_input, sort_keys=True)
        return f"llm:exact:{hashlib.sha256(cache_str.encode()).hexdigest()}"

    def get(self, model: str, messages: list, **kwargs) -> Optional[Dict]:
        """查询缓存"""
        key = self._build_cache_key(model, messages, **kwargs)
        cached = self.redis.get(key)
        if cached:
            return json.loads(cached)
        return None

    def set(self, model: str, messages: list, response: Dict, **kwargs):
        """写入缓存"""
        key = self._build_cache_key(model, messages, **kwargs)
        self.redis.setex(key, self.ttl, json.dumps(response))

    def get_or_call(self, model: str, messages: list, llm_call_fn, **kwargs) -> Dict:
        """缓存优先：命中缓存直接返回，否则调用 LLM"""
        cached = self.get(model, messages, **kwargs)
        if cached:
            cached["cache_hit"] = True
            return cached

        response = llm_call_fn(model, messages, **kwargs)
        self.set(model, messages, response, **kwargs)
        response["cache_hit"] = False
        return response

# 使用示例
cache = LLMExactCache("redis://localhost:6379")
response = cache.get_or_call(
    model="gpt-4",
    messages=[{"role": "user", "content": "什么是Kubernetes?"}],
    llm_call_fn=openai_chat_completion
)
```

### 3.2 语义缓存 (Semantic Cache)

```python
# 基于向量相似度的语义缓存
import numpy as np
from typing import Optional, Tuple
import faiss
from sentence_transformers import SentenceTransformer

class SemanticCache:
    """语义缓存：相似问题复用回答"""

    def __init__(self, model_name: str = "all-MiniLM-L6-v2", threshold: float = 0.92):
        self.encoder = SentenceTransformer(model_name)
        self.threshold = threshold
        self.index = None
        self.responses = []
        self.questions = []

    def _encode(self, text: str) -> np.ndarray:
        """编码文本为向量"""
        return self.encoder.encode([text])[0]

    def add(self, question: str, response: dict):
        """添加缓存条目"""
        embedding = self._encode(question)

        if self.index is None:
            dimension = embedding.shape[0]
            self.index = faiss.IndexFlatIP(dimension)  # 内积相似度

        # 归一化向量
        faiss.normalize_L2(embedding.reshape(1, -1))
        self.index.add(embedding.reshape(1, -1))
        self.questions.append(question)
        self.responses.append(response)

    def search(self, question: str) -> Tuple[Optional[dict], float]:
        """搜索相似问题"""
        if self.index is None or self.index.ntotal == 0:
            return None, 0.0

        query_embedding = self._encode(question)
        faiss.normalize_L2(query_embedding.reshape(1, -1))

        scores, indices = self.index.search(query_embedding.reshape(1, -1), k=1)
        similarity = scores[0][0]

        if similarity >= self.threshold:
            return self.responses[indices[0][0]], float(similarity)
        return None, float(similarity)

# 使用示例
semantic_cache = SemanticCache(threshold=0.90)
response, similarity = semantic_cache.search("K8s是什么？")
if response:
    print(f"语义缓存命中，相似度: {similarity:.2f}")
else:
    response = call_llm(...)
    semantic_cache.add("K8s是什么？", response)
```

### 3.3 混合缓存策略

```python
# 混合缓存：精确 + 语义
class HybridLLMCache:
    """混合缓存策略"""

    def __init__(self, redis_url: str):
        self.exact_cache = LLMExactCache(redis_url)
        self.semantic_cache = SemanticCache(threshold=0.92)

    def get_or_call(self, model: str, messages: list, llm_call_fn, **kwargs) -> Dict:
        """三级缓存策略"""
        # Level 1: 精确缓存
        cached = self.exact_cache.get(model, messages, **kwargs)
        if cached:
            cached["cache_type"] = "exact"
            cached["cache_hit"] = True
            return cached

        # Level 2: 语义缓存
        user_message = self._extract_user_message(messages)
        semantic_result, similarity = self.semantic_cache.search(user_message)
        if semantic_result:
            semantic_result["cache_type"] = "semantic"
            semantic_result["cache_hit"] = True
            semantic_result["similarity"] = similarity
            return semantic_result

        # Level 3: 调用 LLM
        response = llm_call_fn(model, messages, **kwargs)
        response["cache_hit"] = False

        # 写入缓存
        self.exact_cache.set(model, messages, response, **kwargs)
        self.semantic_cache.add(user_message, response)

        return response

    def _extract_user_message(self, messages: list) -> str:
        for msg in reversed(messages):
            if msg["role"] == "user":
                return msg["content"]
        return ""
```

## 4. Token 用量优化

### 4.1 Prompt 压缩

```python
# Prompt 压缩策略
class PromptCompressor:
    """压缩 Prompt 减少 Token 用量"""

    def __init__(self, max_tokens: int = 4000):
        self.max_tokens = max_tokens

    def compress(self, messages: list) -> list:
        """压缩消息列表"""
        compressed = []

        # 1. 保留系统消息
        system_msgs = [m for m in messages if m["role"] == "system"]
        compressed.extend(system_msgs)

        # 2. 压缩历史消息
        history_msgs = [m for m in messages if m["role"] in ["user", "assistant"]]
        if len(history_msgs) > 10:
            # 保留最近 5 轮对话
            recent = history_msgs[-10:]
            # 早期对话只保留摘要
            early = history_msgs[:-10]
            summary = self._summarize_history(early)
            compressed.append({"role": "system", "content": f"历史对话摘要: {summary}"})
            compressed.extend(recent)
        else:
            compressed.extend(history_msgs)

        return compressed

    def _summarize_history(self, messages: list) -> str:
        """总结历史对话"""
        topics = []
        for msg in messages:
            if msg["role"] == "user":
                # 提取关键信息
                content = msg["content"][:100]
                topics.append(content)
        return "; ".join(topics[:5])

    def estimate_tokens(self, messages: list) -> int:
        """估算 Token 数量"""
        total_chars = sum(len(m["content"]) for m in messages)
        # 中文约 1.5 字/token，英文约 4 字符/token
        return int(total_chars / 2.5)
```

### 4.2 上下文裁剪

```python
# 智能上下文裁剪
class ContextTrimmer:
    """智能裁剪上下文，保留最相关信息"""

    def __init__(self, max_context_tokens: int = 3000):
        self.max_tokens = max_context_tokens

    def trim(self, messages: list, current_query: str) -> list:
        """裁剪上下文到指定大小"""
        if self._estimate_tokens(messages) <= self.max_tokens:
            return messages

        # 策略：保留系统消息 + 最近对话 + 相关历史
        system_msgs = [m for m in messages if m["role"] == "system"]
        other_msgs = [m for m in messages if m["role"] != "system"]

        # 计算可用 Token
        system_tokens = self._estimate_tokens(system_msgs)
        available_tokens = self.max_tokens - system_tokens

        # 最近对话优先
        recent_msgs = []
        recent_tokens = 0
        for msg in reversed(other_msgs):
            msg_tokens = self._estimate_tokens([msg])
            if recent_tokens + msg_tokens > available_tokens * 0.7:
                break
            recent_msgs.insert(0, msg)
            recent_tokens += msg_tokens

        # 剩余空间给相关历史
        remaining_tokens = available_tokens - recent_tokens
        relevant_msgs = self._find_relevant(
            other_msgs[:-len(recent_msgs)],
            current_query,
            remaining_tokens
        )

        return system_msgs + relevant_msgs + recent_msgs

    def _find_relevant(self, messages: list, query: str, max_tokens: int) -> list:
        """找到与当前查询相关的历史消息"""
        # 简化实现：关键词匹配
        query_words = set(query.lower().split())
        scored_msgs = []

        for msg in messages:
            content_words = set(msg["content"].lower().split())
            overlap = len(query_words & content_words)
            scored_msgs.append((overlap, msg))

        # 按相关度排序
        scored_msgs.sort(key=lambda x: x[0], reverse=True)

        relevant = []
        tokens = 0
        for score, msg in scored_msgs:
            if score == 0:
                continue
            msg_tokens = self._estimate_tokens([msg])
            if tokens + msg_tokens > max_tokens:
                break
            relevant.append(msg)
            tokens += msg_tokens

        return relevant

    def _estimate_tokens(self, messages: list) -> int:
        return sum(len(m["content"]) // 2.5 for m in messages)
```

### 4.3 Token 使用监控

```python
# Token 使用量追踪
from dataclasses import dataclass
from datetime import datetime
from typing import Dict
import redis

@dataclass
class TokenUsage:
    input_tokens: int
    output_tokens: int
    model: str
    timestamp: datetime
    cost: float

class TokenTracker:
    """Token 使用量追踪器"""

    MODEL_PRICING = {
        "gpt-4": {"input": 0.03, "output": 0.06},           # per 1K tokens
        "gpt-4-turbo": {"input": 0.01, "output": 0.03},
        "gpt-3.5-turbo": {"input": 0.0005, "output": 0.0015},
        "claude-3-opus": {"input": 0.015, "output": 0.075},
        "claude-3-sonnet": {"input": 0.003, "output": 0.015},
    }

    def __init__(self, redis_url: str):
        self.redis = redis.from_url(redis_url)

    def track(self, usage: TokenUsage):
        """记录 Token 使用"""
        # 按小时聚合
        hour_key = usage.timestamp.strftime("%Y%m%d%H")
        self.redis.hincrby(f"tokens:{hour_key}", "input", usage.input_tokens)
        self.redis.hincrby(f"tokens:{hour_key}", "output", usage.output_tokens)
        self.redis.hincrbyfloat(f"tokens:{hour_key}", "cost", usage.cost)

        # 按模型聚合
        model_key = f"tokens:model:{usage.model}:{hour_key}"
        self.redis.hincrby(model_key, "input", usage.input_tokens)
        self.redis.hincrby(model_key, "output", usage.output_tokens)

    def calculate_cost(self, model: str, input_tokens: int, output_tokens: int) -> float:
        """计算成本"""
        pricing = self.MODEL_PRICING.get(model, {"input": 0.01, "output": 0.03})
        input_cost = (input_tokens / 1000) * pricing["input"]
        output_cost = (output_tokens / 1000) * pricing["output"]
        return input_cost + output_cost

    def get_daily_summary(self, date: str) -> Dict:
        """获取每日汇总"""
        total_input = 0
        total_output = 0
        total_cost = 0.0

        for hour in range(24):
            key = f"tokens:{date}{hour:02d}"
            data = self.redis.hgetall(key)
            if data:
                total_input += int(data.get(b"input", 0))
                total_output += int(data.get(b"output", 0))
                total_cost += float(data.get(b"cost", 0))

        return {
            "date": date,
            "total_input_tokens": total_input,
            "total_output_tokens": total_output,
            "total_cost_usd": round(total_cost, 2)
        }
```

## 5. 模型路由

### 5.1 智能路由策略

```python
# 智能模型路由
from typing import Dict, List
from enum import Enum

class QueryComplexity(Enum):
    SIMPLE = "simple"        # 简单查询
    MODERATE = "moderate"    # 中等复杂
    COMPLEX = "complex"      # 复杂推理

class ModelRouter:
    """根据查询复杂度路由到不同模型"""

    MODEL_MAP = {
        QueryComplexity.SIMPLE: "gpt-3.5-turbo",
        QueryComplexity.MODERATE: "gpt-4-turbo",
        QueryComplexity.COMPLEX: "gpt-4",
    }

    def __init__(self):
        self.complexity_classifier = ComplexityClassifier()

    def route(self, query: str, context: List[Dict] = None) -> str:
        """路由查询到合适的模型"""
        complexity = self.complexity_classifier.classify(query, context)
        model = self.MODEL_MAP[complexity]
        return model

    def route_with_cost_estimate(self, query: str, context: List[Dict] = None) -> Dict:
        """路由并返回成本估算"""
        complexity = self.complexity_classifier.classify(query, context)
        model = self.MODEL_MAP[complexity]

        # 估算 Token
        estimated_tokens = len(query) // 2.5
        if context:
            estimated_tokens += sum(len(m["content"]) // 2.5 for m in context)

        return {
            "model": model,
            "complexity": complexity.value,
            "estimated_tokens": int(estimated_tokens),
            "estimated_cost": self._estimate_cost(model, int(estimated_tokens))
        }

    def _estimate_cost(self, model: str, tokens: int) -> float:
        pricing = TokenTracker.MODEL_PRICING.get(model, {"input": 0.01, "output": 0.03})
        return (tokens / 1000) * pricing["input"] * 1.5  # 假设输出是输入的 1.5 倍


class ComplexityClassifier:
    """查询复杂度分类器"""

    COMPLEX_INDICATORS = [
        "分析", "比较", "评估", "设计", "优化", "解释原因",
        "analyze", "compare", "evaluate", "design", "optimize"
    ]

    SIMPLE_INDICATORS = [
        "什么是", "定义", "列表", "查询", "状态",
        "what is", "define", "list", "status", "check"
    ]

    def classify(self, query: str, context: List[Dict] = None) -> QueryComplexity:
        """分类查询复杂度"""
        query_lower = query.lower()

        # 检查复杂指标
        complex_score = sum(1 for ind in self.COMPLEX_INDICATORS if ind in query_lower)
        simple_score = sum(1 for ind in self.SIMPLE_INDICATORS if ind in query_lower)

        # 查询长度也是指标
        if len(query) > 500:
            complex_score += 2
        elif len(query) < 50:
            simple_score += 1

        # 上下文长度
        if context and len(context) > 10:
            complex_score += 1

        if complex_score > simple_score + 1:
            return QueryComplexity.COMPLEX
        elif simple_score > complex_score:
            return QueryComplexity.SIMPLE
        return QueryComplexity.MODERATE
```

### 5.2 基于 K8s 的模型路由服务

```yaml
# 模型路由服务
apiVersion: apps/v1
kind: Deployment
metadata:
  name: model-router
  namespace: ai-agent
spec:
  replicas: 2
  selector:
    matchLabels:
      app: model-router
  template:
    metadata:
      labels:
        app: model-router
    spec:
      containers:
        - name: router
          image: registry.company.com/model-router:v1.0.0
          ports:
            - containerPort: 8080
          env:
            - name: OPENAI_API_KEY
              valueFrom:
                secretKeyRef:
                  name: openai-api
                  key: api-key
            - name: REDIS_URL
              value: "redis://redis:6379"
            - name: CACHE_TTL
              value: "3600"
            - name: SEMANTIC_CACHE_THRESHOLD
              value: "0.92"
          resources:
            requests:
              cpu: 250m
              memory: 512Mi
            limits:
              cpu: "1"
              memory: 2Gi
```

## 6. 成本监控与告警

### 6.1 成本监控 Dashboard

```yaml
# Grafana Dashboard
apiVersion: v1
kind: ConfigMap
metadata:
  name: llm-cost-dashboard
  namespace: monitoring
data:
  dashboard.json: |
    {
      "panels": [
        {
          "title": "每小时 LLM 成本",
          "targets": [{
            "expr": "sum(rate(llm_cost_usd_total[1h]))",
            "legendFormat": "总成本"
          }]
        },
        {
          "title": "缓存命中率",
          "targets": [{
            "expr": "rate(llm_cache_hits_total[5m]) / rate(llm_requests_total[5m]) * 100",
            "legendFormat": "命中率 %"
          }]
        },
        {
          "title": "模型使用分布",
          "targets": [{
            "expr": "sum by (model)(rate(llm_requests_total[5m]))",
            "legendFormat": "{{ model }}"
          }]
        },
        {
          "title": "Token 使用量",
          "targets": [{
            "expr": "sum(rate(llm_input_tokens_total[5m]))",
            "legendFormat": "输入 Token/s"
          }, {
            "expr": "sum(rate(llm_output_tokens_total[5m]))",
            "legendFormat": "输出 Token/s"
          }]
        },
        {
          "title": "每日成本趋势",
          "targets": [{
            "expr": "sum(increase(llm_cost_usd_total[24h]))",
            "legendFormat": "日成本"
          }]
        }
      ]
    }
```

### 6.2 成本告警规则

```yaml
# 成本告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: llm-cost-alerts
  namespace: ai-agent
spec:
  groups:
    - name: llm-cost
      rules:
        - alert: DailyCostExceeded
          expr: |
            sum(increase(llm_cost_usd_total[24h])) > 1000
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "每日 LLM 成本超过 $1000"
            description: "当前日成本: ${{ $value }}"

        - alert: HourlyCostSpike
          expr: |
            rate(llm_cost_usd_total[1h]) > rate(llm_cost_usd_total[1h] offset 24h) * 3
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "小时成本相比昨日同期增长 3 倍"

        - alert: CacheHitRateLow
          expr: |
            rate(llm_cache_hits_total[1h]) / rate(llm_requests_total[1h]) < 0.3
          for: 1h
          labels:
            severity: info
          annotations:
            summary: "缓存命中率低于 30%"
            description: "当前命中率: {{ $value | humanizePercentage }}"

        - alert: ExpensiveModelUsageHigh
          expr: |
            rate(llm_requests_total{model=~"gpt-4.*"}[1h]) / rate(llm_requests_total[1h]) > 0.5
          for: 2h
          labels:
            severity: info
          annotations:
            summary: "高级模型使用比例超过 50%"
            description: "考虑优化路由策略，将更多简单请求路由到小模型"
```

## 7. 成本优化报告

```python
# 成本优化报告生成
class CostOptimizationReport:
    """生成成本优化报告"""

    def __init__(self, tracker: TokenTracker, cache: HybridLLMCache):
        self.tracker = tracker
        self.cache = cache

    def generate(self, start_date: str, end_date: str) -> Dict:
        """生成指定时间段的成本报告"""
        summary = self.tracker.get_daily_summary(start_date)
        cache_stats = self._get_cache_stats()

        # 计算优化效果
        total_requests = cache_stats["total_requests"]
        cache_hits = cache_stats["cache_hits"]
        cache_savings = cache_stats["estimated_savings"]

        # 计算模型路由节省
        routing_savings = self._calculate_routing_savings(start_date)

        return {
            "period": {"start": start_date, "end": end_date},
            "total_cost_usd": summary["total_cost_usd"],
            "total_tokens": {
                "input": summary["total_input_tokens"],
                "output": summary["total_output_tokens"]
            },
            "optimization": {
                "cache": {
                    "hit_rate": f"{cache_hits / max(total_requests, 1) * 100:.1f}%",
                    "savings_usd": cache_savings,
                    "savings_percentage": f"{cache_savings / max(summary['total_cost_usd'], 1) * 100:.1f}%"
                },
                "model_routing": {
                    "savings_usd": routing_savings["savings"],
                    "simple_queries_routed": routing_savings["simple_count"]
                },
                "total_savings_usd": cache_savings + routing_savings["savings"],
                "total_savings_percentage": f"{(cache_savings + routing_savings['savings']) / max(summary['total_cost_usd'] + cache_savings + routing_savings['savings'], 1) * 100:.1f}%"
            },
            "recommendations": self._generate_recommendations(summary, cache_stats)
        }

    def _get_cache_stats(self) -> Dict:
        return {
            "total_requests": 10000,
            "cache_hits": 4500,
            "estimated_savings": 450.0
        }

    def _calculate_routing_savings(self, date: str) -> Dict:
        return {
            "savings": 300.0,
            "simple_count": 6000
        }

    def _generate_recommendations(self, summary: Dict, cache_stats: Dict) -> list:
        """生成优化建议"""
        recommendations = []

        hit_rate = cache_stats["cache_hits"] / max(cache_stats["total_requests"], 1)
        if hit_rate < 0.3:
            recommendations.append("缓存命中率较低，建议检查缓存策略和相似度阈值")

        if summary["total_cost_usd"] > 500:
            recommendations.append("日成本较高，建议增加小模型路由比例")

        return recommendations
```

## 8. 最佳实践

```
LLM 成本优化检查清单:

缓存策略:
  □ 精确缓存已启用
  □ 语义缓存阈值已调优
  □ 缓存 TTL 设置合理
  □ 缓存命中率监控

Token 优化:
  □ Prompt 压缩已启用
  □ 上下文裁剪策略已配置
  □ Token 使用量监控

模型路由:
  □ 复杂度分类器已训练
  □ 模型映射表已配置
  □ 路由决策日志完整

监控告警:
  □ 成本 Dashboard 已创建
  □ 成本超限告警已配置
  □ 缓存命中率告警
  □ 定期成本报告生成
```

## Related

- [[AI基础设施/02-ai-agents/51-agent-guardrails-content-safety|Agent 安全护栏]]
- AI基础设施/
- 可观测性/

## See Also

- OpenAI 定价文档
- LLM 缓存最佳实践
- 成本优化策略


<!-- risk-assessed -->
