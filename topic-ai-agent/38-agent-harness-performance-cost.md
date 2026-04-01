# Agent Harness 性能与成本优化

> **文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Performance, Cost Optimization, Token 优化, 延迟优化, 缓存, 模型路由, 推理预算, 批处理, 流式输出, FinOps

---

## 概述

Agent Harness 的性能和成本优化是生产化的核心挑战。一个未优化的 Agent 可能每个任务消耗 $2-5 的 Token 费用、30 秒以上的端到端延迟。通过系统性的 Harness 优化——推理预算分配、上下文压缩、模型路由、缓存策略——可以在不牺牲质量的前提下将成本降低 60%+、延迟降低 50%+。

本文系统阐述 Harness 性能优化的关键策略、Token 经济学、模型路由决策、缓存体系、批处理优化，以及 Agent FinOps 实践。

---

## 1. Agent 成本结构分析

### 1.1 成本组成

```
Agent 任务成本分解:

典型诊断任务（10 步、使用 GPT-4o）:
  ├── LLM 推理成本: $0.85 (70%)
  │   ├── 输入 Token: ~40K tokens × $2.50/1M = $0.10
  │   ├── 输出 Token: ~8K tokens × $10.00/1M = $0.08
  │   └── 多轮累积输入: ~300K tokens × $2.50/1M = $0.75
  │       （每轮都需要发送完整上下文）
  │
  ├── 工具调用成本: $0.05 (4%)
  │   └── kubectl/prometheus API 调用
  │
  ├── 验证成本: $0.15 (12%)
  │   ├── LLM-as-Judge: ~10K tokens
  │   └── 自检循环: 1-2 轮额外推理
  │
  ├── RAG 检索成本: $0.05 (4%)
  │   └── Embedding + 向量检索
  │
  └── 基础设施成本: $0.12 (10%)
      └── 计算、网络、存储

关键洞察:
  多轮累积输入是最大成本项。每一轮 Loop 都需要发送完整上下文，
  10 轮迭代意味着上下文被发送了 10 次。
  优化上下文长度的收益是乘数级的。
```

### 1.2 成本-质量权衡矩阵

| 优化策略 | 成本节省 | 质量影响 | 风险 | 优先级 |
|---------|---------|---------|------|--------|
| **上下文压缩** | 30-50% | 低 | 关键信息丢失 | P0 |
| **模型路由** | 40-70% | 低-中 | 简单任务用弱模型 | P0 |
| **缓存复用** | 20-40% | 无 | 缓存过期 | P0 |
| **推理预算分配** | 10-20% | 无 | - | P1 |
| **并行工具调用** | 0（减延迟） | 无 | - | P1 |
| **流式输出** | 0（减感知延迟） | 无 | - | P2 |
| **批量处理** | 30-50% | 低 | 延迟增加 | P2 |

---

## 2. Token 优化策略

### 2.1 上下文压缩

```python
class ContextCompressor:
    """上下文压缩器：减少每轮发送的 Token 数"""

    def __init__(self, llm_compressor=None, max_ratio: float = 0.5):
        self.compressor = llm_compressor
        self.max_ratio = max_ratio  # 最大压缩到原始的 50%

    def compress(self, context: str, budget_tokens: int) -> str:
        """多策略压缩上下文"""
        current_tokens = self._count_tokens(context)

        if current_tokens <= budget_tokens:
            return context

        # 策略 1: 移除冗余空白和格式
        context = self._strip_formatting(context)
        if self._count_tokens(context) <= budget_tokens:
            return context

        # 策略 2: 历史步骤摘要化
        context = self._summarize_history(context)
        if self._count_tokens(context) <= budget_tokens:
            return context

        # 策略 3: 工具输出截断
        context = self._truncate_tool_outputs(context, budget_tokens)
        if self._count_tokens(context) <= budget_tokens:
            return context

        # 策略 4: LLM 辅助压缩（最后手段）
        if self.compressor:
            context = self._llm_compress(context, budget_tokens)

        return context

    def _strip_formatting(self, text: str) -> str:
        """移除冗余格式"""
        import re
        # 多个空行 → 单个空行
        text = re.sub(r'\n{3,}', '\n\n', text)
        # 多个空格 → 单个空格
        text = re.sub(r' {2,}', ' ', text)
        # 移除注释行
        text = re.sub(r'^\s*#.*$', '', text, flags=re.MULTILINE)
        return text

    def _summarize_history(self, text: str) -> str:
        """将详细的历史步骤替换为摘要"""
        # 保留最近 3 步的详细信息，其余摘要化
        import re
        steps = re.split(r'### Step \d+', text)
        if len(steps) <= 4:
            return text

        # 前面的步骤摘要化
        summary_parts = [steps[0]]  # 保留开头
        for step in steps[1:-3]:
            # 提取关键信息
            thought = re.search(r'思考: (.+?)(?:\n|$)', step)
            result = re.search(r'结果: (.+?)(?:\n|$)', step)
            summary = ""
            if thought:
                summary += f"[{thought.group(1)[:50]}]"
            if result:
                summary += f" → {result.group(1)[:30]}"
            summary_parts.append(summary)

        # 最近 3 步保留完整
        summary_parts.extend(steps[-3:])
        return "\n".join(summary_parts)

    def _truncate_tool_outputs(self, text: str, budget: int) -> str:
        """截断过长的工具输出"""
        import re
        # 找到工具输出块并截断
        def truncate_match(match):
            output = match.group(1)
            if len(output) > 500:
                return f"结果: {output[:300]}...[截断 {len(output)-300} 字符]"
            return match.group(0)

        return re.sub(r'结果: (.+?)(?=\n###|\n---|$)',
                      truncate_match, text, flags=re.DOTALL)

    def _llm_compress(self, text: str, budget: int) -> str:
        """使用 LLM 智能压缩"""
        prompt = f"""
将以下文本压缩到约 {budget} 个 token，保留所有关键信息：
- 保留: 错误信息、关键发现、数值数据
- 省略: 重复描述、格式细节、冗余内容

{text[:10000]}
"""
        return self.compressor.invoke(prompt)

    def _count_tokens(self, text: str) -> int:
        return len(text.split()) * 1.3  # 粗略估算
```

### 2.2 推理预算分配

```python
class ReasoningBudgetAllocator:
    """推理预算分配器：不同阶段分配不同 Token 预算

    LangChain 实证：规划多分配、执行少分配，整体效率提升 20%。
    """

    def __init__(self, total_budget: int = 50_000):
        self.total = total_budget
        self.phase_ratios = {
            "planning": 0.30,       # 30% 给规划
            "information_gathering": 0.25,  # 25% 给信息收集
            "analysis": 0.25,       # 25% 给分析
            "execution": 0.10,      # 10% 给执行
            "verification": 0.10,   # 10% 给验证
        }

    def allocate(self, phase: str) -> int:
        """分配阶段预算"""
        ratio = self.phase_ratios.get(phase, 0.2)
        return int(self.total * ratio)

    def adaptive_allocate(self, phase: str, task_complexity: str,
                          used_so_far: int) -> int:
        """自适应预算分配"""
        remaining = self.total - used_so_far
        if remaining <= 0:
            return 0

        base = self.allocate(phase)

        # 复杂任务给分析阶段更多预算
        if task_complexity == "complex" and phase == "analysis":
            base = int(base * 1.5)

        # 简单任务给执行阶段更少预算
        if task_complexity == "simple" and phase == "execution":
            base = int(base * 0.5)

        return min(base, remaining)
```

---

## 3. 模型路由

### 3.1 智能模型路由器

```python
class ModelRouter:
    """智能模型路由器：根据任务类型和预算选择最优模型

    核心策略: 简单任务用便宜模型，复杂/高风险任务用强模型。
    """

    MODEL_TIERS = {
        "tier1_premium": {
            "models": ["gpt-4o", "claude-sonnet-4"],
            "cost_per_1k_tokens": 0.01,
            "quality_score": 0.95,
            "latency_p50_ms": 2000,
        },
        "tier2_standard": {
            "models": ["gpt-4o-mini", "claude-haiku-3.5"],
            "cost_per_1k_tokens": 0.001,
            "quality_score": 0.85,
            "latency_p50_ms": 800,
        },
        "tier3_fast": {
            "models": ["gpt-4o-mini"],
            "cost_per_1k_tokens": 0.0005,
            "quality_score": 0.75,
            "latency_p50_ms": 400,
        },
    }

    TASK_MODEL_MAPPING = {
        # 推理任务（复杂分析、根因推断）→ Premium
        "root_cause_analysis": "tier1_premium",
        "complex_diagnosis": "tier1_premium",
        "multi_step_planning": "tier1_premium",

        # 信息提取（从日志提取关键信息）→ Standard
        "information_extraction": "tier2_standard",
        "log_analysis": "tier2_standard",
        "status_summary": "tier2_standard",

        # 格式化任务（生成 YAML、格式化输出）→ Fast
        "yaml_generation": "tier3_fast",
        "output_formatting": "tier3_fast",
        "simple_classification": "tier3_fast",
    }

    def route(self, task_type: str, risk_level: str = "medium",
              remaining_budget_usd: float = None) -> dict:
        """路由到最优模型"""
        # 高风险任务强制使用 Premium
        if risk_level in ("high", "critical"):
            tier = "tier1_premium"
        else:
            tier = self.TASK_MODEL_MAPPING.get(task_type, "tier2_standard")

        # 预算不足时降级
        if remaining_budget_usd is not None:
            tier_config = self.MODEL_TIERS[tier]
            if remaining_budget_usd < 0.1 and tier == "tier1_premium":
                tier = "tier2_standard"
            elif remaining_budget_usd < 0.01:
                tier = "tier3_fast"

        tier_config = self.MODEL_TIERS[tier]
        return {
            "tier": tier,
            "model": tier_config["models"][0],
            "expected_cost": tier_config["cost_per_1k_tokens"],
            "expected_latency_ms": tier_config["latency_p50_ms"],
        }

    def route_for_loop_phase(self, phase: str, iteration: int) -> dict:
        """Loop 不同阶段使用不同模型"""
        if phase == "planning":
            return self.route("multi_step_planning", "medium")
        elif phase == "information_gathering":
            return self.route("information_extraction", "low")
        elif phase == "analysis":
            return self.route("root_cause_analysis", "high")
        elif phase == "execution":
            return self.route("yaml_generation", "medium")
        elif phase == "verification":
            return self.route("complex_diagnosis", "high")
        return self.route("status_summary", "low")
```

---

## 4. 缓存策略

### 4.1 多级缓存架构

```python
import hashlib
import json
import time
from typing import Optional

class MultiLevelCache:
    """多级缓存：减少重复 LLM 调用"""

    def __init__(self, memory_cache_size: int = 1000,
                 redis_client=None, ttl_seconds: int = 3600):
        self._l1_cache: dict = {}  # L1: 内存缓存
        self._l1_max = memory_cache_size
        self._redis = redis_client  # L2: Redis 缓存
        self._ttl = ttl_seconds
        self._stats = {"hits": 0, "misses": 0}

    def get(self, key: str) -> Optional[dict]:
        """查询缓存"""
        # L1: 内存
        if key in self._l1_cache:
            entry = self._l1_cache[key]
            if time.time() - entry["timestamp"] < self._ttl:
                self._stats["hits"] += 1
                return entry["value"]
            else:
                del self._l1_cache[key]

        # L2: Redis
        if self._redis:
            cached = self._redis.get(f"agent_cache:{key}")
            if cached:
                value = json.loads(cached)
                # 回填 L1
                self._l1_cache[key] = {
                    "value": value, "timestamp": time.time(),
                }
                self._stats["hits"] += 1
                return value

        self._stats["misses"] += 1
        return None

    def set(self, key: str, value: dict):
        """写入缓存"""
        # L1
        if len(self._l1_cache) >= self._l1_max:
            # LRU 淘汰
            oldest = min(self._l1_cache, key=lambda k: self._l1_cache[k]["timestamp"])
            del self._l1_cache[oldest]

        self._l1_cache[key] = {"value": value, "timestamp": time.time()}

        # L2
        if self._redis:
            self._redis.setex(
                f"agent_cache:{key}",
                self._ttl,
                json.dumps(value),
            )

    @staticmethod
    def make_key(prompt: str, model: str, tools: list = None) -> str:
        """生成缓存 key"""
        key_parts = f"{model}:{prompt}"
        if tools:
            key_parts += f":{','.join(sorted(str(t) for t in tools))}"
        return hashlib.sha256(key_parts.encode()).hexdigest()

    def get_stats(self) -> dict:
        total = self._stats["hits"] + self._stats["misses"]
        return {
            "hits": self._stats["hits"],
            "misses": self._stats["misses"],
            "hit_rate": self._stats["hits"] / total if total > 0 else 0,
        }


class SemanticCache:
    """语义缓存：相似问题复用答案"""

    def __init__(self, vector_store, similarity_threshold: float = 0.95):
        self.store = vector_store
        self.threshold = similarity_threshold

    def get(self, query: str) -> Optional[dict]:
        """语义匹配查询"""
        results = self.store.search(query, top_k=1)
        if results and results[0].score >= self.threshold:
            return results[0].metadata.get("cached_response")
        return None

    def set(self, query: str, response: dict):
        """存入语义缓存"""
        self.store.upsert(
            text=query,
            metadata={"cached_response": response,
                       "timestamp": time.time()},
        )
```

### 4.2 工具结果缓存

```python
class ToolResultCache:
    """工具结果缓存：避免重复调用相同的 kubectl/prometheus 命令"""

    def __init__(self, cache: MultiLevelCache, tool_ttls: dict = None):
        self.cache = cache
        # 不同工具不同 TTL
        self.tool_ttls = tool_ttls or {
            "kubectl_get": 30,        # 30 秒
            "kubectl_describe": 60,   # 1 分钟
            "kubectl_events": 15,     # 15 秒
            "kubectl_top": 15,        # 15 秒
            "prometheus_query": 30,   # 30 秒
            "loki_search": 60,        # 1 分钟
        }

    def get_or_execute(self, tool_name: str, args: dict,
                       executor) -> dict:
        """缓存命中则返回，否则执行并缓存"""
        key = self._make_key(tool_name, args)
        cached = self.cache.get(key)
        if cached:
            cached["from_cache"] = True
            return cached

        result = executor(tool_name, args)
        if result.get("success"):
            self.cache.set(key, result)

        result["from_cache"] = False
        return result

    def _make_key(self, tool_name: str, args: dict) -> str:
        args_str = json.dumps(args, sort_keys=True)
        return hashlib.md5(f"{tool_name}:{args_str}".encode()).hexdigest()
```

---

## 5. 延迟优化

### 5.1 并行化策略

```python
class LatencyOptimizer:
    """延迟优化器"""

    @staticmethod
    async def parallel_tool_calls(tools_to_call: list[dict],
                                   executor, max_concurrent: int = 3) -> list:
        """并行工具调用"""
        semaphore = asyncio.Semaphore(max_concurrent)

        async def call_with_limit(tool_call):
            async with semaphore:
                return await executor.async_execute(
                    tool_call["name"], tool_call["args"]
                )

        tasks = [call_with_limit(tc) for tc in tools_to_call]
        return await asyncio.gather(*tasks, return_exceptions=True)

    @staticmethod
    async def speculative_execution(primary_tool, backup_tool, args,
                                     timeout: float = 5.0) -> dict:
        """推测执行：主工具超时则使用备选"""
        try:
            result = await asyncio.wait_for(
                primary_tool.async_execute(**args),
                timeout=timeout,
            )
            return {"source": "primary", "result": result}
        except asyncio.TimeoutError:
            result = await backup_tool.async_execute(**args)
            return {"source": "backup", "result": result}

    @staticmethod
    async def streaming_think(llm, prompt: str, callback=None) -> str:
        """流式推理：边生成边处理"""
        full_response = ""
        async for chunk in llm.astream(prompt):
            full_response += chunk
            if callback:
                callback(chunk)  # 实时回调（如更新 UI）
        return full_response
```

### 5.2 Prompt Caching

```python
class PromptCacheOptimizer:
    """Prompt 缓存优化：利用 Anthropic/OpenAI 的 Prompt Caching"""

    def __init__(self):
        self._static_prefix: str = ""
        self._static_prefix_tokens: int = 0

    def optimize_for_loop(self, system_prompt: str, environment: str,
                          knowledge: str) -> dict:
        """优化 Loop 中的 Prompt 以利用 Provider 缓存

        策略: 将不变的部分放在前面（System + Environment + Knowledge），
              变化的部分放在后面（History + Current Question）。
              
        Anthropic Prompt Caching: 前缀匹配可享受 90% 折扣。
        OpenAI Prompt Caching: 自动检测重复前缀。
        """
        # 不变前缀（跨迭代保持不变）
        static_prefix = f"{system_prompt}\n\n{environment}\n\n{knowledge}"

        return {
            "static_prefix": static_prefix,
            "static_tokens": self._count_tokens(static_prefix),
            "cache_savings_estimate": "80-90% on prefix tokens",
            "tip": "保持前缀稳定，只在后缀追加历史和当前问题",
        }

    def _count_tokens(self, text: str) -> int:
        return len(text.split()) * 1.3
```

---

## 6. Agent FinOps

### 6.1 成本监控 Dashboard

```python
class AgentFinOps:
    """Agent FinOps: 成本监控与优化"""

    def __init__(self, cost_calculator, metrics_collector):
        self.calculator = cost_calculator
        self.metrics = metrics_collector

    def daily_report(self, date: str = None) -> dict:
        """每日成本报告"""
        return {
            "date": date or datetime.utcnow().strftime("%Y-%m-%d"),
            "total_cost_usd": self.metrics.get_daily_cost(),
            "total_tokens": self.metrics.get_daily_tokens(),
            "total_tasks": self.metrics.get_daily_tasks(),
            "cost_per_task_avg": (
                self.metrics.get_daily_cost()
                / max(self.metrics.get_daily_tasks(), 1)
            ),
            "breakdown_by_model": self.metrics.get_cost_by_model(),
            "breakdown_by_task_type": self.metrics.get_cost_by_task_type(),
            "optimization_opportunities": self._identify_optimizations(),
        }

    def _identify_optimizations(self) -> list:
        """识别优化机会"""
        opportunities = []

        # 检查是否有简单任务使用了高端模型
        model_usage = self.metrics.get_model_usage_by_task_type()
        for task_type, models in model_usage.items():
            if task_type in ("status_summary", "output_formatting"):
                if any(m in models for m in ("gpt-4o", "claude-sonnet-4")):
                    opportunities.append({
                        "type": "model_downgrade",
                        "task_type": task_type,
                        "current_model": models[0],
                        "suggested_model": "gpt-4o-mini",
                        "estimated_savings": "60-80%",
                    })

        # 检查缓存命中率
        cache_stats = self.metrics.get_cache_stats()
        if cache_stats.get("hit_rate", 0) < 0.3:
            opportunities.append({
                "type": "improve_caching",
                "current_hit_rate": cache_stats["hit_rate"],
                "target_hit_rate": 0.5,
                "estimated_savings": "20-30%",
            })

        return opportunities
```

### 6.2 成本预警与限流

```python
class CostThrottler:
    """成本限流器：防止成本失控"""

    def __init__(self, daily_budget: float = 50.0, alert_at: float = 0.8):
        self.daily_budget = daily_budget
        self.alert_threshold = alert_at  # 80% 时告警
        self.daily_spent = 0.0
        self.throttle_mode = False

    def check_and_throttle(self, estimated_cost: float) -> tuple[bool, str]:
        """检查是否需要限流"""
        projected = self.daily_spent + estimated_cost

        # 超预算：拒绝
        if projected > self.daily_budget:
            return False, f"日预算已耗尽: ${self.daily_spent:.2f}/{self.daily_budget:.2f}"

        # 接近预算：告警 + 降级
        if projected > self.daily_budget * self.alert_threshold:
            self.throttle_mode = True
            return True, "进入降级模式: 使用低成本模型"

        return True, "OK"

    def get_throttle_config(self) -> dict:
        """获取限流配置"""
        if self.throttle_mode:
            return {
                "model_tier": "tier3_fast",      # 降级到最便宜模型
                "max_iterations": 5,              # 减少迭代
                "context_budget": 30_000,          # 压缩上下文
                "skip_verification": False,        # 验证不能跳
                "cache_aggressive": True,          # 激进缓存
            }
        return {}
```

---

## 7. 最佳实践

### 7.1 性能优化核心原则

| 原则 | 说明 | 预期收益 |
|------|------|---------|
| **上下文压缩** | 减少每轮发送的 Token | 成本 -30-50% |
| **模型路由** | 简单任务用便宜模型 | 成本 -40-70% |
| **工具结果缓存** | 避免重复 kubectl 调用 | 延迟 -30% |
| **并行工具调用** | 独立工具同时执行 | 延迟 -40% |
| **推理预算分配** | 规划多给、执行少给 | 效率 +20% |
| **Prompt 缓存** | 利用 Provider 前缀缓存 | 成本 -80% 前缀 |
| **语义缓存** | 相似问题复用答案 | 成本 -20% |

### 7.2 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| **全程 GPT-4o** | 成本爆炸 | 模型路由：按任务类型分级 |
| **无上下文压缩** | Token 累积膨胀 | 历史步骤摘要化 |
| **无缓存** | 重复调用浪费 | 多级缓存 |
| **串行工具调用** | 延迟累加 | 识别可并行工具 |
| **无成本监控** | 成本失控 | Agent FinOps Dashboard |
| **无预算限制** | 单任务可能花 $50 | 任务级和日级预算上限 |

---

## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | 推理预算分配基础概念 |
| [33 - 上下文与记忆](./33-agent-harness-context-memory.md) | 上下文压缩的详细实现 |
| [35 - 安全与约束](./35-agent-harness-security-constraints.md) | 成本约束实现 |
| [11 - 成本延迟优化](./11-cost-latency-optimization.md) | Agent 成本优化基础理论 |

---

## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| LangChain | 推理预算重分配实验 +20% 效率 | 2026-02 |
| Anthropic | Prompt Caching 技术文档 | 2025 |
| OpenAI | Prompt Caching 自动优化 | 2025 |
| Vercel | 工具精简 Token -37% 实证 | 2025 |

---

*本文档为 kudig-database 项目 topic-ai-agent 系列原创内容，深入展开 Agent Harness 性能与成本优化。*
