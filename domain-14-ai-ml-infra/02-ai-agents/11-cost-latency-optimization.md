---
title: 成本与延迟优化策略 (domain-14-ai-ml-infra)
description: 'title: 成本与延迟优化策略'
category: general
tags:
- ai
- ai-agent
- cost-optimization
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 成本与延迟优化策略 是什么
- 如何 成本与延迟优化策略
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- 成本与延迟优化策略
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- cilium-basics
- cni-basics
- etcd-basics
- redis-basics
- tracing-basics
created: "2026-05-23"
---

title: 成本与延迟优化策略
description: '# 成本与延迟优化策略'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- scheduler
- controller-manager
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- 成本与延迟优化策略 是什么
- 如何 成本与延迟优化策略
trigger_keywords:
- 成本与延迟优化策略
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

# 成本与延迟优化策略

> **文档类型**: 工程优化专题 | **最后更新**: 2026-03 | **关键词**: Token 优化, 语义缓存, 模型路由, 成本控制, 延迟优化, KV Cache, 批处理, LLM 成本, vLLM 优化

---

<!-- chunk: 概述 -->## 概述

LLM API 调用成本和响应延迟是 Agent 系统商业化落地的核心挑战。在实际生产环境中，无优化的 Agent 每次对话成本可高达 $0.5-2，而经过系统优化后可降低至 $0.02-0.1，即 **10-100x 的成本压缩空间**。本文覆盖从 Token 预算、语义缓存、模型路由到批处理策略的全套优化技术。

---

<!-- chunk: 1. 成本结构分析 -->## 1. 成本结构分析

## 1.1 Agent 成本分解

```
典型 K8s 诊断 Agent 单次任务成本分解（无优化）:

  总成本: ~$0.30
  │
  ├── LLM 调用 (85%)
  │   ├── 系统提示 Token  ~2000 tokens × $2.5/1M = $0.005 × N轮
  │   ├── 工具定义 Token  ~3000 tokens × $2.5/1M = $0.0075 × N轮
  │   ├── 对话历史 Token  ~5000 tokens × $2.5/1M = $0.0125 × N轮
  │   └── 输出 Token      ~1000 tokens × $10/1M = $0.01 × N轮
  │   (假设 10 轮交互)
  │
  ├── Embedding 调用 (5%)
  │   └── 每次 RAG 检索 ~500 tokens × $0.13/1M × 5次
  │
  └── 基础设施 (10%)
      └── Qdrant、Redis、计算资源

经过优化后（目标）:
  总成本: ~$0.03 (-90%)
```

## 1.2 成本监控仪表板

```python
from dataclasses import dataclass, field
from collections import defaultdict
import time

@dataclass
class LLMCostTracker:
    """实时 LLM 成本追踪器"""
    
    # 模型定价（每百万 Token，单位 USD）
    MODEL_PRICING = {
        "gpt-4o": {"input": 2.5, "output": 10.0},
        "gpt-4o-mini": {"input": 0.15, "output": 0.6},
        "claude-3-5-sonnet": {"input": 3.0, "output": 15.0},
        "claude-3-5-haiku": {"input": 0.8, "output": 4.0},
        "deepseek-chat": {"input": 0.27, "output": 1.1},
        "text-embedding-3-small": {"input": 0.02, "output": 0.0},
        "text-embedding-3-large": {"input": 0.13, "output": 0.0},
    }
    
    daily_costs: dict = field(default_factory=lambda: defaultdict(float))
    session_costs: dict = field(default_factory=dict)
    total_tokens: dict = field(default_factory=lambda: defaultdict(int))
    
    def record_call(
        self,
        model: str,
        input_tokens: int,
        output_tokens: int,
        session_id: str,
    ):
        """记录一次 LLM 调用的成本"""
        pricing = self.MODEL_PRICING.get(model, {"input": 0, "output": 0})
        
        cost = (
            input_tokens * pricing["input"] / 1_000_000 +
            output_tokens * pricing["output"] / 1_000_000
        )
        
        date_key = time.strftime("%Y-%m-%d")
        self.daily_costs[f"{date_key}:{model}"] += cost
        self.session_costs[session_id] = self.session_costs.get(session_id, 0) + cost
        self.total_tokens[model] += input_tokens + output_tokens
        
        return cost
    
    def get_daily_report(self) -> dict:
        """生成日成本报告"""
        date_key = time.strftime("%Y-%m-%d")
        today_costs = {
            k.split(":", 1)[1]: v 
            for k, v in self.daily_costs.items() 
            if k.startswith(date_key)
        }
        
        return {
            "date": date_key,
            "by_model": today_costs,
            "total_usd": sum(today_costs.values()),
            "projection_monthly_usd": sum(today_costs.values()) * 30,
        }

# 全局成本追踪器（通过依赖注入）
cost_tracker = LLMCostTracker()
```

---

<!-- chunk: 2. Token 预算优化 -->## 2. Token 预算优化

## 2.1 系统提示压缩

```python
# 对比：未优化 vs 优化后的系统提示

UNOPTIMIZED_SYSTEM_PROMPT = """
你是一个非常专业的 Kubernetes 运维专家助手。你在 Kubernetes 领域有超过十年的丰富经验，
熟悉所有版本的 Kubernetes，包括 1.18、1.19、1.20、1.21、1.22、1.23、1.24、1.25、1.26、
1.27、1.28、1.29、1.30 等版本。你深入了解 Kubernetes 的各种组件，包括 kube-apiserver、
kube-controller-manager、kube-scheduler、kubelet、kube-proxy、etcd 等。你精通各种 CNI 插件，
包括 Calico、Flannel、Cilium、Weave 等。你会使用各种监控工具，包括 Prometheus、Grafana、
Alertmanager、Jaeger 等。你非常擅长故障排查，能够处理各种复杂的生产环境问题...
[约 500 tokens]
"""

OPTIMIZED_SYSTEM_PROMPT = """
你是 K8s 运维专家 Agent。职责：诊断问题 + 提供可操作的修复步骤。
规则：基于工具获取的实际数据回答；不确定时说明需要更多信息；给出风险提示。
[约 35 tokens - 节省 93%]
"""

# 动态系统提示（根据任务类型按需注入知识）
def build_contextual_system_prompt(task_type: str) -> str:
    BASE = "你是 K8s 运维专家 Agent。基于工具数据给出准确诊断和修复步骤。"
    
    TASK_ADDONS = {
        "network": "\n专注: CNI、Service、NetworkPolicy、DNS 问题",
        "storage": "\n专注: PVC、StorageClass、CSI 驱动问题",
        "security": "\n专注: RBAC、证书、Pod Security 策略问题",
        "scheduling": "\n专注: 资源不足、亲和性、Taint/Toleration 问题",
    }
    
    return BASE + TASK_ADDONS.get(task_type, "")
```

## 2.2 工具描述精简

```python
# 工具描述优化（减少每次调用携带的 Token 数）

# 未优化（~150 tokens/工具）
VERBOSE_TOOL = {
    "function": {
        "description": """这个工具用于获取 Kubernetes Pod 的详细状态信息。
        你可以使用这个工具来查看 Pod 的运行状态、容器状态、重启次数、
        IP 地址、所在节点、以及相关的事件信息。当你需要诊断 Pod 的问题时，
        比如 Pod 处于 Pending 状态、CrashLoopBackOff、OOMKilled 等情况时，
        这个工具会非常有用。它会返回 kubectl describe pod 命令的输出结果。""",
    }
}

# 优化后（~30 tokens/工具）
CONCISE_TOOL = {
    "function": {
        "description": "kubectl describe pod: 获取 Pod 状态/事件/容器信息。诊断 Pending/CrashLoop/OOM 使用。",
    }
}

# 对 20 个工具每次调用节省: (150-30) × 20 = 2400 tokens ≈ $0.006
```

## 2.3 对话历史压缩

```python
class AdaptiveContextCompressor:
    """自适应上下文压缩器"""
    
    def __init__(self, llm, max_tokens: int = 4000):
        self.llm = llm
        self.max_tokens = max_tokens
        self.encoder = tiktoken.encoding_for_model("gpt-4o")
    
    def compress(self, messages: list[dict]) -> list[dict]:
        """智能压缩对话历史"""
        total_tokens = sum(
            len(self.encoder.encode(str(m.get("content", ""))))
            for m in messages
        )
        
        if total_tokens <= self.max_tokens:
            return messages
        
        system_msgs = [m for m in messages if m["role"] == "system"]
        conv_msgs = [m for m in messages if m["role"] != "system"]
        
        # 保留最近 4 条消息（2 轮对话）
        recent = conv_msgs[-4:]
        to_compress = conv_msgs[:-4]
        
        if not to_compress:
            return messages
        
        # 压缩旧消息
        summary = self.llm.invoke(
            f"一句话总结以下对话的关键信息（最多 80 字）：\n{to_compress}"
        ).content
        
        summary_msg = {
            "role": "system",
            "content": f"[历史摘要: {summary}]"
        }
        
        return system_msgs + [summary_msg] + recent
```

---

<!-- chunk: 3. 语义缓存（Semantic Cache） -->## 3. 语义缓存（Semantic Cache）

语义缓存是成本优化中投入产出比最高的手段：对于相似（而非完全相同）的问题，直接返回缓存结果。

## 3.1 基于向量相似度的缓存

```python
import hashlib
import numpy as np
from typing import Optional

class SemanticCache:
    """基于向量相似度的语义缓存"""
    
    def __init__(
        self,
        embedding_model,
        vector_store,
        similarity_threshold: float = 0.95,  # 相似度阈值
        ttl_seconds: int = 3600,             # 缓存有效期
        max_cache_size: int = 10000,
    ):
        self.embedding_model = embedding_model
        self.vector_store = vector_store
        self.similarity_threshold = similarity_threshold
        self.ttl = ttl_seconds
        self.redis = redis_client  # 存储缓存内容和 TTL
    
    def get(self, query: str) -> Optional[dict]:
        """检索语义相似的缓存结果"""
        
        query_embedding = self.embedding_model.embed_query(query)
        
        # 向量相似度检索
        results = self.vector_store.similarity_search_by_vector(
            query_embedding,
            k=1,
        )
        
        if not results:
            return None
        
        top_result = results[0]
        
        # 计算余弦相似度
        cached_embedding = top_result.metadata.get("embedding")
        if cached_embedding is None:
            return None
        
        similarity = self._cosine_similarity(query_embedding, cached_embedding)
        
        if similarity >= self.similarity_threshold:
            cache_key = top_result.metadata["cache_key"]
            cached_response = self.redis.get(f"semantic_cache:{cache_key}")
            
            if cached_response:
                return {
                    "hit": True,
                    "response": json.loads(cached_response),
                    "similarity": similarity,
                    "original_query": top_result.page_content,
                }
        
        return None
    
    def set(
        self,
        query: str,
        response: dict,
        query_embedding: list = None,
    ):
        """存储查询和响应到缓存"""
        
        if query_embedding is None:
            query_embedding = self.embedding_model.embed_query(query)
        
        cache_key = hashlib.md5(query.encode()).hexdigest()
        
        # 存储向量（用于相似度检索）
        self.vector_store.add_texts(
            texts=[query],
            metadatas=[{
                "cache_key": cache_key,
                "embedding": query_embedding,
                "timestamp": time.time(),
            }]
        )
        
        # 存储响应内容（带 TTL）
        self.redis.setex(
            f"semantic_cache:{cache_key}",
            self.ttl,
            json.dumps(response),
        )
    
    @staticmethod
    def _cosine_similarity(a: list, b: list) -> float:
        a_arr = np.array(a)
        b_arr = np.array(b)
        return float(np.dot(a_arr, b_arr) / (np.linalg.norm(a_arr) * np.linalg.norm(b_arr)))

# 集成到 Agent 服务
class CachedAgentService:
    def __init__(self, agent_executor, semantic_cache: SemanticCache):
        self.agent = agent_executor
        self.cache = semantic_cache
    
    def run(self, query: str, session_id: str = None) -> dict:
        # 对非实时数据查询尝试缓存
        if self._is_cacheable(query):
            cached = self.cache.get(query)
            if cached:
                return {
                    **cached["response"],
                    "cache_hit": True,
                    "similarity": cached["similarity"],
                    "cost_saved": True,
                }
        
        # Cache Miss：执行 Agent
        result = self.agent.invoke({"input": query})
        
        # 存入缓存（非实时操作）
        if self._is_cacheable(query):
            self.cache.set(query, result)
        
        return {**result, "cache_hit": False}
    
    def _is_cacheable(self, query: str) -> bool:
        """判断查询是否可以缓存（实时操作不缓存）"""
        # 实时数据查询不缓存
        realtime_keywords = ["当前", "现在", "最新", "实时", "live"]
        if any(kw in query for kw in realtime_keywords):
            return False
        
        # 读取操作可缓存，修改操作不缓存
        modification_keywords = ["修改", "更新", "删除", "扩容", "重启"]
        if any(kw in query for kw in modification_keywords):
            return False
        
        return True
```

---

<!-- chunk: 4. 模型路由优化 -->## 4. 模型路由优化

## 4.1 智能路由策略

```python
from dataclasses import dataclass
from typing import Callable

@dataclass
class ModelProfile:
    name: str
    cost_per_1m_input: float  # USD
    cost_per_1m_output: float
    max_context: int
    avg_latency_ms: int
    tool_calling_quality: float  # 0-1
    reasoning_quality: float     # 0-1

MODELS = {
    "fast-cheap": ModelProfile("gpt-4o-mini", 0.15, 0.6, 128000, 300, 0.92, 0.85),
    "balanced": ModelProfile("gpt-4o", 2.5, 10.0, 128000, 500, 0.99, 0.97),
    "best-reasoning": ModelProfile("claude-3-5-sonnet", 3.0, 15.0, 200000, 500, 0.99, 0.98),
    "chinese-budget": ModelProfile("deepseek-chat", 0.27, 1.1, 128000, 1000, 0.92, 0.95),
    "long-context": ModelProfile("gemini-1.5-pro", 1.25, 5.0, 2000000, 1000, 0.90, 0.93),
}

class IntelligentModelRouter:
    """智能模型路由器"""
    
    def route(
        self,
        task: str,
        context_length: int,
        available_tools: int,
        language: str,
        latency_sensitive: bool,
        cost_sensitive: bool,
    ) -> str:
        """根据任务特征选择最优模型"""
        
        # 超长上下文（>100K tokens）
        if context_length > 100_000:
            return "long-context"
        
        # 延迟敏感 + 成本敏感
        if latency_sensitive and cost_sensitive:
            return "fast-cheap"
        
        # 中文场景 + 成本敏感
        if language == "zh" and cost_sensitive:
            return "chinese-budget"
        
        # 复杂推理（多步骤分析）
        complexity = self._assess_complexity(task, available_tools)
        if complexity == "high":
            return "best-reasoning"
        
        # 中等复杂度
        if complexity == "medium":
            if cost_sensitive:
                return "fast-cheap"
            return "balanced"
        
        # 简单任务
        return "fast-cheap"
    
    def _assess_complexity(self, task: str, tool_count: int) -> str:
        """评估任务复杂度"""
        high_complexity_keywords = ["分析", "规划", "设计", "评估", "compare", "compare"]
        medium_complexity_keywords = ["诊断", "排查", "检查", "diagnose", "investigate"]
        
        if any(kw in task for kw in high_complexity_keywords) or tool_count > 10:
            return "high"
        elif any(kw in task for kw in medium_complexity_keywords) or tool_count > 5:
            return "medium"
        return "low"

# 实际成本节省计算
def calculate_routing_savings(daily_requests: int = 1000) -> dict:
    """计算模型路由带来的成本节省"""
    
    # 假设任务分布
    task_distribution = {
        "fast-cheap": 0.60,     # 60% 简单任务
        "balanced": 0.25,       # 25% 中等任务
        "best-reasoning": 0.10, # 10% 复杂任务
        "chinese-budget": 0.05, # 5% 中文任务
    }
    
    avg_tokens_per_task = 5000  # 输入 + 输出 Token
    
    # 全用 GPT-4o 的成本
    all_gpt4o_cost = daily_requests * avg_tokens_per_task * 6.25 / 1_000_000
    
    # 路由后的成本
    routed_cost = 0
    for model, ratio in task_distribution.items():
        model_profile = MODELS[model]
        avg_cost = (model_profile.cost_per_1m_input * 0.7 + 
                    model_profile.cost_per_1m_output * 0.3) / 1_000_000
        routed_cost += daily_requests * ratio * avg_tokens_per_task * avg_cost
    
    return {
        "all_gpt4o_daily": round(all_gpt4o_cost, 2),
        "routed_daily": round(routed_cost, 2),
        "savings_ratio": round(1 - routed_cost / all_gpt4o_cost, 2),
        "monthly_savings_usd": round((all_gpt4o_cost - routed_cost) * 30, 2),
    }
```

---

<!-- chunk: 5. KV Cache 与 Prompt Caching -->## 5. KV Cache 与 Prompt Caching

## 5.1 OpenAI Prompt Caching（固定前缀复用）

```python
# Prompt Caching 利用技巧：将系统提示和工具定义放在最前面（最稳定部分）
# OpenAI 自动缓存超过 1024 token 的相同前缀，节省 50% 成本

def build_cache_optimized_messages(
    system_prompt: str,      # 稳定部分（会被缓存）
    tool_definitions: list,  # 稳定部分（会被缓存）
    conversation_history: list,  # 变动部分
    current_query: str,
) -> list[dict]:
    """
    构建利用 Prompt Caching 的消息列表
    缓存命中条件：前缀超过 1024 tokens 且完全相同
    """
    return [
        {"role": "system", "content": system_prompt},
        # 工具定义通过 tools 参数传递（也会被缓存）
        *conversation_history,  # 历史消息
        {"role": "user", "content": current_query},  # 最新消息
    ]
    # 建议：系统提示 + 工具定义 >= 1024 tokens 才能触发缓存
    # 缓存命中后：输入 Token 成本 -50%

# Anthropic Claude 的显式 Prompt Caching
import anthropic

client = anthropic.Anthropic()

response = client.messages.create(
    model="claude-3-5-sonnet-20241022",
    max_tokens=4096,
    system=[
        {
            "type": "text",
            "text": system_prompt,
            "cache_control": {"type": "ephemeral"}  # 标记为可缓存（5分钟）
        }
    ],
    tools=[
        {
            **tool_definition,
            "cache_control": {"type": "ephemeral"}  # 工具定义也可缓存
        }
        for tool_definition in tool_definitions
    ],
    messages=conversation_history + [
        {"role": "user", "content": current_query}
    ]
)

# 查看缓存命中情况
usage = response.usage
print(f"缓存读取 tokens: {usage.cache_read_input_tokens}")  # 0.1x 价格
print(f"缓存写入 tokens: {usage.cache_creation_input_tokens}")  # 1.25x 价格（首次）
print(f"普通输入 tokens: {usage.input_tokens}")
```

## 5.2 vLLM KV Cache 优化

```python
# vLLM 的 Prefix Caching（同一系统提示的多个请求复用 KV Cache）
# 在 vLLM 部署参数中启用:
# --enable-prefix-caching

# 利用此功能的关键：确保系统提示在所有请求中完全相同

# 测量缓存命中率
import requests

def get_vllm_cache_metrics(vllm_url: str) -> dict:
    response = requests.get(f"{vllm_url}/metrics")
    metrics_text = response.text
    
    # 解析 vllm_cache_usage_perc 和 vllm_num_preemptions_total
    return parse_prometheus_metrics(metrics_text)
```

---

<!-- chunk: 6. 批处理与并发优化 -->## 6. 批处理与并发优化

## 6.1 异步批处理

```python
import asyncio
from collections import deque

class BatchedLLMProcessor:
    """批处理 LLM 请求，提升吞吐量"""
    
    def __init__(
        self,
        llm,
        batch_size: int = 10,
        max_wait_ms: int = 100,  # 最多等待 100ms 凑批
    ):
        self.llm = llm
        self.batch_size = batch_size
        self.max_wait = max_wait_ms / 1000
        self.queue = asyncio.Queue()
    
    async def process_batch(self, requests: list) -> list:
        """并发处理一批请求"""
        tasks = [
            asyncio.create_task(self.llm.ainvoke(req["messages"]))
            for req in requests
        ]
        results = await asyncio.gather(*tasks, return_exceptions=True)
        return results
    
    async def batch_worker(self):
        """后台批处理工作器"""
        while True:
            batch = []
            deadline = asyncio.get_event_loop().time() + self.max_wait
            
            # 凑批
            while (len(batch) < self.batch_size and 
                   asyncio.get_event_loop().time() < deadline):
                try:
                    item = await asyncio.wait_for(
                        self.queue.get(),
                        timeout=deadline - asyncio.get_event_loop().time()
                    )
                    batch.append(item)
                except asyncio.TimeoutError:
                    break
            
            if batch:
                results = await self.process_batch([item["request"] for item in batch])
                for item, result in zip(batch, results):
                    item["future"].set_result(result)

# 离线评估场景的批量处理
async def batch_evaluate_agent(
    test_cases: list[dict],
    agent_executor,
    concurrency: int = 5,
) -> list[dict]:
    """并发执行批量评估任务"""
    semaphore = asyncio.Semaphore(concurrency)
    
    async def run_single(case: dict) -> dict:
        async with semaphore:
            try:
                result = await agent_executor.ainvoke(
                    {"input": case["question"]}
                )
                return {
                    "question": case["question"],
                    "answer": result["output"],
                    "success": True
                }
            except Exception as e:
                return {
                    "question": case["question"],
                    "error": str(e),
                    "success": False
                }
    
    tasks = [run_single(case) for case in test_cases]
    return await asyncio.gather(*tasks)
```

---

<!-- chunk: 7. 综合优化效果对比 -->## 7. 综合优化效果对比

## 7.1 各策略成本节省汇总

| 优化策略 | 适用场景 | 成本节省 | 实施复杂度 | 推荐优先级 |
|---------|---------|---------|-----------|----------|
| **模型路由** | 所有场景 | 50-70% | 中 | P0 最高 |
| **语义缓存** | 知识查询场景 | 30-60% | 中 | P0 最高 |
| **系统提示压缩** | 所有场景 | 10-30% | 低 | P1 高 |
| **Prompt Caching** | 固定前缀场景 | 10-50% | 低 | P1 高 |
| **上下文压缩** | 长对话场景 | 20-40% | 中 | P1 高 |
| **并行工具调用** | 多工具场景 | 0%（降延迟）| 低 | P1 高 |
| **KV Cache (vLLM)** | 自部署 LLM | 20-50% | 低 | P2 中 |
| **批处理** | 离线任务 | 10-20% | 高 | P3 低 |

## 7.2 生产环境成本优化路线图

```
第一阶段（立即执行，低风险）:
  1. 系统提示精简（节省 10-30%）
  2. 工具描述优化（节省 5-15%）
  3. 启用模型路由（节省 50-70%）
  预计总节省: 60-80%

第二阶段（2周内，中等复杂度）:
  4. 语义缓存集成（节省 30-60% of remaining）
  5. 对话历史压缩（节省 20-40% of remaining）
  预计总节省: 80-90%

第三阶段（1月内，架构优化）:
  6. Claude Prompt Caching（节省 20-50% on Claude）
  7. 自部署 vLLM（高频场景，节省 70-90%）
  预计总节省: 90-95%
```

---

<!-- chunk: 8. 延迟优化 -->## 8. 延迟优化

## 8.1 关键路径延迟分析

```
Agent 任务端到端延迟分解（典型 5 步任务）:

  总延迟: ~8500ms
  │
  ├── LLM 推理 (70%)  ~6000ms
  │   ├── TTFT (首 Token) ~500ms × 5 = 2500ms
  │   └── Token 生成    ~700ms/500 tokens × 5 轮 = 3500ms
  │
  ├── 工具执行 (20%)   ~1700ms
  │   └── kubectl 命令  ~200-500ms × 5 次
  │
  └── RAG 检索 (10%)   ~800ms
      └── Qdrant 向量搜索 ~100-200ms × 4 次

优化后目标: ~3500ms (节省 59%)
```

## 8.2 流式输出降低感知延迟

```python
# 流式输出将 TTFT 从 8s（等待完整响应）降至 0.5s（第一个 Token）
# 用户感知延迟降低 10-16x

async def stream_with_early_ux(query: str) -> AsyncGenerator:
    """先发送 Agent 的思考过程，降低用户感知等待"""
    
    # 立即发送"正在处理"状态
    yield {
        "type": "status",
        "content": "正在诊断中..."
    }
    
    async for event in agent.astream_events({"input": query}, version="v2"):
        if event["event"] == "on_tool_start":
            # 实时告知用户正在执行哪个工具
            yield {
                "type": "tool_start",
                "content": f"正在执行: {event['name']}"
            }
        
        elif event["event"] == "on_chat_model_stream":
            content = event["data"]["chunk"].content
            if content:
                yield {
                    "type": "token",
                    "content": content
                }
```

## 8.3 预取（Prefetch）策略

```python
class PrefetchAgent:
    """预取相关上下文，降低 RAG 延迟"""
    
    def __init__(self, retriever, llm):
        self.retriever = retriever
        self.llm = llm
        self.prefetch_cache = {}
    
    async def predict_and_prefetch(self, partial_input: str):
        """用户输入时就开始预取可能需要的知识"""
        
        # 预测用户可能的完整问题
        predicted_queries = await self._predict_queries(partial_input)
        
        # 并发预取所有预测查询
        prefetch_tasks = [
            self.retriever.aget_relevant_documents(q)
            for q in predicted_queries
        ]
        
        results = await asyncio.gather(*prefetch_tasks)
        
        # 缓存预取结果
        for query, docs in zip(predicted_queries, results):
            self.prefetch_cache[query] = {
                "docs": docs,
                "timestamp": time.time()
            }
```

---

<!-- chunk: 9. 最佳实践与反模式 -->## 9. 最佳实践与反模式

## 最佳实践

- **成本预算告警**：设置日成本和月成本上限告警，防止成本失控
- **按用户计费追踪**：精确到每个 session/user 的 Token 消耗，支持成本分摊
- **优先 Cache 而非优化提示**：提示优化影响质量，缓存几乎没有质量损失
- **生产监控成本**：将成本数据接入 Grafana，让工程师直观看到成本变化
- **定期审查热门查询**：发现可以预缓存或用规则替代的高频查询

## 反模式

- **所有任务用最贵的模型**：GPT-4o 处理简单问候或格式转换是极大浪费
- **不限制 Token 输出**：没有 max_tokens 限制，少数恶意请求可产生极高成本
- **缓存实时数据**：将"当前 Pod 状态"缓存，返回过时数据导致诊断错误
- **忽略缓存命中率**：语义缓存部署后不监控命中率，不知道是否在发挥作用

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [02 - LLM 模型选型](./02-llm-foundation-models.md) | 模型路由的定价基准 |
| [07 - 记忆管理](./07-memory-context-management.md) | 上下文压缩对成本的影响 |
| [08 - 评测与可观测性](./observability.md|08-agent-evaluation-observability]].md) | 成本 Prometheus 指标 |
| [09 - 生产部署](./09-production-deployment-guide.md) | vLLM 部署与 KV Cache |
| [domain-14-ai-ml-infra/26-cost-optimization-overview.md](../domain-14-ai-ml-infra/26-cost-optimization-overview.md) | AI 基础设施成本优化 |
| [domain-14-ai-ml-infra/23-llm-cost-monitoring.md](../domain-14-ai-ml-infra/23-llm-cost-monitoring.md) | LLM 成本监控体系 |

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
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 09-production-deployment-guide
- 10-security-guardrails
- 12-enterprise-case-studies
- 13-trusted-agent-system-fiscal-plan
