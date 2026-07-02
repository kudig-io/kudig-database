---
title: Agent限流与成本控制
description: 'Agent系统的Token Bucket限流、预算控制、模型路由、缓存策略、降级与成本告警'
summary: 'Agent系统的Token Bucket限流、预算控制、模型路由、缓存策略、降级与成本告警'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- rate-limiting
- cost-control
- caching
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
- Agent限流与成本控制 是什么
- 如何控制LLM API成本
- Token Bucket限流实现
- Agent成本优化策略
trigger_keywords:
- rate limiting
- cost control
- token budget
- model routing
- semantic cache
- fallback
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

# Agent限流与成本控制

## 概述

Agent系统的成本主要来自LLM API调用。一次Agent推理可能包含多轮LLM调用（思考→工具选择→结果处理→最终回答），单次请求的Token消耗远高于传统API。限流与成本控制是Agent平台生产化的必要条件。

本文覆盖限流算法、预算管理、模型路由、缓存策略、降级方案和实时告警。

## 1. 限流算法

### 1.1 Token Bucket（令牌桶）

Token Bucket是最适合LLM API的限流算法，因为它允许突发流量同时控制平均速率。

```python
import time
import threading

class TokenBucketRateLimiter:
    """针对LLM API调用的令牌桶限流器"""

    def __init__(self, rate: float, capacity: int):
        """
        rate: 每秒生成的令牌数
        capacity: 桶容量（最大突发量）
        """
        self.rate = rate
        self.capacity = capacity
        self.tokens = capacity
        self.last_refill = time.monotonic()
        self.lock = threading.Lock()

    def acquire(self, tokens: int = 1) -> bool:
        """尝试获取令牌，非阻塞"""
        with self.lock:
            now = time.monotonic()
            elapsed = now - self.last_refill
            self.tokens = min(
                self.capacity,
                self.tokens + elapsed * self.rate
            )
            self.last_refill = now

            if self.tokens >= tokens:
                self.tokens -= tokens
                return True
            return False

    def wait_and_acquire(self, tokens: int = 1, timeout: float = 30.0) -> bool:
        """等待获取令牌，带超时"""
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.acquire(tokens):
                return True
            time.sleep(0.1)
        return False


class MultiDimensionRateLimiter:
    """多维度限流：按用户/Agent/全局"""

    def __init__(self):
        # 全局限流：1000 RPM
        self.global_limiter = TokenBucketRateLimiter(
            rate=1000/60, capacity=100
        )
        # 按用户限流：60 RPM/用户
        self.user_limiters: dict[str, TokenBucketRateLimiter] = {}
        # 按Agent限流：200 RPM/Agent
        self.agent_limiters: dict[str, TokenBucketRateLimiter] = {}

    def check(self, user_id: str, agent_id: str) -> tuple[bool, str]:
        """检查是否允许请求"""
        # 全局检查
        if not self.global_limiter.acquire():
            return False, "global_rate_limit"

        # 用户级检查
        user_limiter = self._get_user_limiter(user_id)
        if not user_limiter.acquire():
            return False, f"user_rate_limit:{user_id}"

        # Agent级检查
        agent_limiter = self._get_agent_limiter(agent_id)
        if not agent_limiter.acquire():
            return False, f"agent_rate_limit:{agent_id}"

        return True, "ok"

    def _get_user_limiter(self, user_id: str) -> TokenBucketRateLimiter:
        if user_id not in self.user_limiters:
            self.user_limiters[user_id] = TokenBucketRateLimiter(
                rate=60/60, capacity=10
            )
        return self.user_limiters[user_id]

    def _get_agent_limiter(self, agent_id: str) -> TokenBucketRateLimiter:
        if agent_id not in self.agent_limiters:
            self.agent_limiters[agent_id] = TokenBucketRateLimiter(
                rate=200/60, capacity=30
            )
        return self.agent_limiters[agent_id]
```

### 1.2 Sliding Window（滑动窗口）

滑动窗口提供更精确的限流控制：

```python
import time
from collections import deque

class SlidingWindowRateLimiter:
    """滑动窗口限流器，精确控制时间窗口内的请求数"""

    def __init__(self, max_requests: int, window_seconds: int):
        self.max_requests = max_requests
        self.window_seconds = window_seconds
        self.requests: deque[float] = deque()
        self.lock = threading.Lock()

    def is_allowed(self) -> bool:
        with self.lock:
            now = time.monotonic()
            window_start = now - self.window_seconds

            # 移除过期请求
            while self.requests and self.requests[0] < window_start:
                self.requests.popleft()

            if len(self.requests) < self.max_requests:
                self.requests.append(now)
                return True
            return False

    def retry_after(self) -> float:
        """返回需要等待的秒数"""
        with self.lock:
            if not self.requests:
                return 0
            oldest = self.requests[0]
            return max(0, self.window_seconds - (time.monotonic() - oldest))


class TokenBasedSlidingWindow:
    """基于Token数量的滑动窗口限流"""

    def __init__(self, max_tokens: int, window_seconds: int):
        self.max_tokens = max_tokens
        self.window_seconds = window_seconds
        self.usage: deque[tuple[float, int]] = deque()  # (timestamp, tokens)
        self.lock = threading.Lock()

    def check_and_record(self, estimated_tokens: int) -> bool:
        """检查Token预算并记录使用"""
        with self.lock:
            now = time.monotonic()
            window_start = now - self.window_seconds

            # 清理过期记录
            while self.usage and self.usage[0][0] < window_start:
                self.usage.popleft()

            # 计算窗口内总Token
            total = sum(t for _, t in self.usage)

            if total + estimated_tokens <= self.max_tokens:
                self.usage.append((now, estimated_tokens))
                return True
            return False
```

### 1.3 分布式限流

在K8s多副本部署中，需要分布式限流：

```python
import redis
import time

class RedisRateLimiter:
    """基于Redis的分布式限流器"""

    def __init__(self, redis_client: redis.Redis):
        self.redis = redis_client

    def token_bucket_acquire(
        self,
        key: str,
        rate: float,
        capacity: int,
        tokens: int = 1
    ) -> bool:
        """Lua脚本实现原子性令牌桶"""
        lua_script = """
        local key = KEYS[1]
        local rate = tonumber(ARGV[1])
        local capacity = tonumber(ARGV[2])
        local tokens = tonumber(ARGV[3])
        local now = tonumber(ARGV[4])

        local bucket = redis.call('HMGET', key, 'tokens', 'last_refill')
        local current_tokens = tonumber(bucket[1]) or capacity
        local last_refill = tonumber(bucket[2]) or now

        local elapsed = now - last_refill
        current_tokens = math.min(capacity, current_tokens + elapsed * rate)

        local allowed = 0
        if current_tokens >= tokens then
            current_tokens = current_tokens - tokens
            allowed = 1
        end

        redis.call('HMSET', key, 'tokens', current_tokens, 'last_refill', now)
        redis.call('EXPIRE', key, math.ceil(capacity / rate) + 1)

        return allowed
        """

        result = self.redis.eval(
            lua_script,
            1,
            key,
            str(rate),
            str(capacity),
            str(tokens),
            str(time.time())
        )
        return bool(result)
```

## 2. 预算控制

### 2.1 三层预算体系

```python
from dataclasses import dataclass
from datetime import datetime, date

@dataclass
class BudgetConfig:
    daily_limit_usd: float
    monthly_limit_usd: float
    per_request_limit_usd: float
    alert_threshold_pct: float  # 告警阈值百分比

class BudgetManager:
    """三层预算管理：用户/Agent/全局"""

    def __init__(self, redis_client: redis.Redis):
        self.redis = redis_client

    def check_budget(
        self,
        user_id: str,
        agent_id: str,
        estimated_cost_usd: float
    ) -> tuple[bool, str]:
        """检查预算是否充足"""

        # 1. 单次请求预算
        per_request_limit = 0.50  # $0.50/次
        if estimated_cost_usd > per_request_limit:
            return False, f"per_request_exceeded:{estimated_cost_usd:.4f}>{per_request_limit}"

        # 2. 用户日预算
        user_daily = self._get_usage(f"user:{user_id}:daily")
        user_daily_limit = 10.0  # $10/天/用户
        if user_daily + estimated_cost_usd > user_daily_limit:
            return False, f"user_daily_exceeded:{user_daily:.4f}+{estimated_cost_usd:.4f}>{user_daily_limit}"

        # 3. Agent日预算
        agent_daily = self._get_usage(f"agent:{agent_id}:daily")
        agent_daily_limit = 100.0  # $100/天/Agent
        if agent_daily + estimated_cost_usd > agent_daily_limit:
            return False, f"agent_daily_exceeded:{agent_daily:.4f}+{estimated_cost_usd:.4f}>{agent_daily_limit}"

        # 4. 全局月预算
        global_monthly = self._get_usage("global:monthly")
        global_monthly_limit = 10000.0  # $10,000/月
        if global_monthly + estimated_cost_usd > global_monthly_limit:
            return False, f"global_monthly_exceeded:{global_monthly:.4f}+{estimated_cost_usd:.4f}>{global_monthly_limit}"

        return True, "ok"

    def record_usage(
        self,
        user_id: str,
        agent_id: str,
        actual_cost_usd: float
    ):
        """记录实际消耗"""
        pipe = self.redis.pipeline()
        today = date.today().isoformat()
        month = today[:7]

        pipe.incrbyfloat(f"user:{user_id}:daily:{today}", actual_cost_usd)
        pipe.expire(f"user:{user_id}:daily:{today}", 86400 * 2)

        pipe.incrbyfloat(f"agent:{agent_id}:daily:{today}", actual_cost_usd)
        pipe.expire(f"agent:{agent_id}:daily:{today}", 86400 * 2)

        pipe.incrbyfloat(f"global:monthly:{month}", actual_cost_usd)
        pipe.expire(f"global:monthly:{month}", 86400 * 35)

        pipe.execute()

    def _get_usage(self, key_pattern: str) -> float:
        today = date.today().isoformat()
        month = today[:7]

        if "daily" in key_pattern:
            key = f"{key_pattern}:{today}"
        elif "monthly" in key_pattern:
            key = f"{key_pattern}:{month}"
        else:
            key = key_pattern

        value = self.redis.get(key)
        return float(value) if value else 0.0

    def get_usage_report(self, user_id: str) -> dict:
        """获取用户用量报告"""
        today = date.today().isoformat()
        month = today[:7]

        return {
            "user_daily": self._get_usage(f"user:{user_id}:daily:{today}"),
            "user_monthly": self._get_usage(f"user:{user_id}:monthly:{month}"),
            "global_monthly": self._get_usage(f"global:monthly:{month}"),
        }
```

### 2.2 Token预算估算

```python
class TokenCostEstimator:
    """Token成本估算器"""

    # 2026 Q2 定价 (USD per 1M tokens)
    MODEL_PRICING = {
        "gpt-4o": {"input": 2.50, "output": 10.00},
        "gpt-4o-mini": {"input": 0.15, "output": 0.60},
        "claude-sonnet": {"input": 3.00, "output": 15.00},
        "claude-haiku": {"input": 0.25, "output": 1.25},
        "gemini-1.5-pro": {"input": 1.25, "output": 5.00},
        "gemini-1.5-flash": {"input": 0.075, "output": 0.30},
        "qwen-max": {"input": 0.12, "output": 0.12},  # ¥0.12/千token
        "deepseek-v3": {"input": 0.14, "output": 0.28},
    }

    @classmethod
    def estimate_cost(
        cls,
        model: str,
        input_tokens: int,
        output_tokens: int
    ) -> float:
        """估算单次调用成本（USD）"""
        pricing = cls.MODEL_PRICING.get(model)
        if not pricing:
            raise ValueError(f"Unknown model: {model}")

        input_cost = (input_tokens / 1_000_000) * pricing["input"]
        output_cost = (output_tokens / 1_000_000) * pricing["output"]
        return input_cost + output_cost

    @classmethod
    def estimate_agent_cost(
        cls,
        model: str,
        num_tool_calls: int = 2,
        avg_input_per_step: int = 2000,
        avg_output_per_step: int = 500
    ) -> float:
        """估算Agent单次请求的总成本"""
        # Agent推理步骤：思考 + 工具调用 + 结果处理 + 最终回答
        total_steps = 1 + num_tool_calls * 2 + 1  # 思考 + (调用+处理)*N + 回答
        total_input = total_steps * avg_input_per_step
        total_output = total_steps * avg_output_per_step
        return cls.estimate_cost(model, total_input, total_output)
```

## 3. 模型路由

### 3.1 智能路由策略

根据请求复杂度选择合适的模型，简单问题用小模型降低成本：

```python
from enum import Enum
from typing import Optional

class ComplexityLevel(Enum):
    SIMPLE = "simple"       # 简单问答
    MODERATE = "moderate"   # 中等复杂
    COMPLEX = "complex"     # 复杂推理
    CRITICAL = "critical"   # 关键任务

class ModelRouter:
    """智能模型路由器"""

    # 路由策略配置
    ROUTING_TABLE = {
        ComplexityLevel.SIMPLE: {
            "primary": "gpt-4o-mini",
            "fallback": "claude-haiku",
            "cost_weight": 0.9,  # 优先考虑成本
        },
        ComplexityLevel.MODERATE: {
            "primary": "claude-sonnet",
            "fallback": "gpt-4o",
            "cost_weight": 0.5,
        },
        ComplexityLevel.COMPLEX: {
            "primary": "gpt-4o",
            "fallback": "claude-sonnet",
            "cost_weight": 0.2,
        },
        ComplexityLevel.CRITICAL: {
            "primary": "gpt-4o",
            "fallback": "claude-sonnet",
            "cost_weight": 0.0,  # 不考虑成本
        },
    }

    def route(
        self,
        query: str,
        context: Optional[dict] = None
    ) -> tuple[str, ComplexityLevel]:
        """路由请求到合适的模型"""
        complexity = self._classify_complexity(query, context)
        config = self.ROUTING_TABLE[complexity]

        # 检查主模型可用性
        if self._is_model_available(config["primary"]):
            return config["primary"], complexity

        # 使用fallback
        return config["fallback"], complexity

    def _classify_complexity(
        self,
        query: str,
        context: Optional[dict]
    ) -> ComplexityLevel:
        """基于规则+统计的复杂度分类"""
        # 规则匹配
        simple_patterns = [
            "你好", "谢谢", "是的", "好的",
            "what is", "how to", "define"
        ]
        complex_patterns = [
            "分析", "比较", "推理", "设计",
            "analyze", "compare", "reason", "design",
            "多步骤", "优化", "debug"
        ]

        query_lower = query.lower()

        # 简单模式匹配
        if len(query) < 20 and any(p in query_lower for p in simple_patterns):
            return ComplexityLevel.SIMPLE

        # 复杂模式匹配
        if any(p in query_lower for p in complex_patterns):
            return ComplexityLevel.COMPLEX

        # 基于上下文判断
        if context and context.get("requires_tools"):
            return ComplexityLevel.COMPLEX

        # 基于长度判断
        if len(query) > 500:
            return ComplexityLevel.MODERATE

        return ComplexityLevel.SIMPLE

    def _is_model_available(self, model: str) -> bool:
        """检查模型是否可用"""
        # 实际实现中检查模型健康状态
        return True
```

### 3.2 成本优化路由

```python
class CostOptimizedRouter(ModelRouter):
    """成本优化路由器"""

    def __init__(self, daily_budget_remaining: float):
        self.budget_remaining = daily_budget_remaining

    def route(self, query: str, context: Optional[dict] = None) -> tuple[str, ComplexityLevel]:
        """预算感知路由"""
        model, complexity = super().route(query, context)

        # 预算紧张时降级模型
        if self.budget_remaining < 1.0:  # 剩余<$1
            if complexity in (ComplexityLevel.SIMPLE, ComplexityLevel.MODERATE):
                return "gpt-4o-mini", complexity  # 强制使用小模型
            elif complexity == ComplexityLevel.COMPLEX:
                return "claude-sonnet", complexity  # 降级但保持质量

        # 预算充足时使用最优模型
        return model, complexity

    def update_budget(self, cost: float):
        self.budget_remaining -= cost
```

## 4. 缓存策略

### 4.1 精确缓存

```python
import hashlib
import json
from typing import Optional

class ExactCache:
    """精确缓存：完全相同的输入返回缓存结果"""

    def __init__(self, redis_client: redis.Redis, ttl: int = 3600):
        self.redis = redis_client
        self.ttl = ttl

    def get_cache_key(self, agent_id: str, model: str, messages: list) -> str:
        """生成缓存键"""
        content = json.dumps({
            "agent": agent_id,
            "model": model,
            "messages": messages
        }, sort_keys=True)
        return f"llm_cache:{hashlib.sha256(content.encode()).hexdigest()}"

    def get(
        self,
        agent_id: str,
        model: str,
        messages: list
    ) -> Optional[str]:
        """查询缓存"""
        key = self.get_cache_key(agent_id, model, messages)
        result = self.redis.get(key)
        return result.decode() if result else None

    def set(
        self,
        agent_id: str,
        model: str,
        messages: list,
        response: str
    ):
        """写入缓存"""
        key = self.get_cache_key(agent_id, model, messages)
        self.redis.setex(key, self.ttl, response)
```

### 4.2 语义缓存

```python
import numpy as np
from typing import Optional

class SemanticCache:
    """语义缓存：相似语义的输入返回缓存结果"""

    def __init__(
        self,
        redis_client: redis.Redis,
        embedding_func,
        similarity_threshold: float = 0.92,
        ttl: int = 3600
    ):
        self.redis = redis_client
        self.embedding_func = embedding_func
        self.threshold = similarity_threshold
        self.ttl = ttl

    def get(
        self,
        agent_id: str,
        query: str
    ) -> Optional[tuple[str, float]]:
        """语义相似度检索"""
        query_embedding = self.embedding_func(query)

        # 从Redis获取该Agent的所有缓存embedding
        pattern = f"semantic_cache:{agent_id}:*"
        keys = self.redis.keys(pattern)

        best_match = None
        best_similarity = 0.0

        for key in keys:
            cached = self.redis.hgetall(key)
            cached_embedding = np.frombuffer(cached[b"embedding"], dtype=np.float32)

            # 余弦相似度
            similarity = np.dot(query_embedding, cached_embedding) / (
                np.linalg.norm(query_embedding) * np.linalg.norm(cached_embedding)
            )

            if similarity > best_similarity and similarity >= self.threshold:
                best_similarity = similarity
                best_match = cached[b"response"].decode()

        if best_match:
            return best_match, best_similarity
        return None

    def set(
        self,
        agent_id: str,
        query: str,
        response: str
    ):
        """写入语义缓存"""
        embedding = self.embedding_func(query)
        cache_id = hashlib.sha256(query.encode()).hexdigest()[:16]
        key = f"semantic_cache:{agent_id}:{cache_id}"

        self.redis.hset(key, mapping={
            "query": query,
            "response": response,
            "embedding": embedding.astype(np.float32).tobytes()
        })
        self.redis.expire(key, self.ttl)
```

### 4.3 缓存策略选择

```
缓存策略决策:

精确缓存:
  适用: FAQ、模板回复、固定查询
  命中率: 低（5-15%）
  准确性: 100%
  实现复杂度: 低

语义缓存:
  适用: 客服、知识问答、开放域对话
  命中率: 中（20-40%）
  准确性: 92%+（取决于阈值）
  实现复杂度: 中

混合缓存（推荐）:
  策略: 先精确 → 后语义 → 最后LLM
  命中率: 高（30-50%）
  准确性: 高
  实现复杂度: 中

不建议缓存的场景:
  - 实时数据查询（股票、天气）
  - 个性化回答（需要唯一性）
  - 长上下文对话（上下文变化大）
  - 流式输出（缓存意义不大）
```

## 5. 降级策略

### 5.1 Fallback Model链

```python
class FallbackChain:
    """模型降级链"""

    def __init__(self):
        self.chain = [
            {"model": "gpt-4o", "timeout": 30, "retries": 2},
            {"model": "claude-sonnet", "timeout": 25, "retries": 2},
            {"model": "gpt-4o-mini", "timeout": 20, "retries": 3},
            {"model": "deepseek-v3", "timeout": 15, "retries": 3},
        ]

    async def execute(self, messages: list, **kwargs) -> str:
        """按降级链执行，任一成功即返回"""
        errors = []

        for config in self.chain:
            try:
                result = await self._call_model(
                    model=config["model"],
                    messages=messages,
                    timeout=config["timeout"],
                    retries=config["retries"],
                    **kwargs
                )
                return result
            except Exception as e:
                errors.append({
                    "model": config["model"],
                    "error": str(e)
                })
                continue

        # 所有模型都失败
        raise AllModelsFailedError(errors)

    async def _call_model(self, model: str, messages: list, timeout: int, retries: int, **kwargs):
        """调用单个模型"""
        for attempt in range(retries):
            try:
                # 实际API调用
                pass
            except Exception as e:
                if attempt < retries - 1:
                    await asyncio.sleep(2 ** attempt)
                else:
                    raise
```

### 5.2 降级触发条件

```yaml
降级触发条件:

模型不可用:
  - HTTP 429 (Rate Limited)
  - HTTP 500/502/503 (Server Error)
  - 超时 (30s)
  - 连续3次失败

成本超限:
  - 单次请求预估>$1
  - 日预算剩余<10%
  - 月预算剩余<5%

质量降级:
  - 模型输出被安全过滤拦截
  - 输出格式不符合预期
  - 置信度过低
```

## 6. 实时成本告警

### 6.1 告警规则

```python
from dataclasses import dataclass
from enum import Enum

class AlertLevel(Enum):
    INFO = "info"
    WARNING = "warning"
    CRITICAL = "critical"

@dataclass
class AlertRule:
    name: str
    level: AlertLevel
    condition: str  # "daily_cost > threshold"
    threshold: float
    window: str     # "1h", "1d", "1m"
    cooldown: int   # 告警冷却秒数

class CostAlertManager:
    """成本告警管理器"""

    DEFAULT_RULES = [
        AlertRule(
            name="daily_budget_80pct",
            level=AlertLevel.WARNING,
            condition="daily_usage_pct > 80",
            threshold=0.8,
            window="1d",
            cooldown=3600
        ),
        AlertRule(
            name="daily_budget_95pct",
            level=AlertLevel.CRITICAL,
            condition="daily_usage_pct > 95",
            threshold=0.95,
            window="1d",
            cooldown=1800
        ),
        AlertRule(
            name="single_request_high_cost",
            level=AlertLevel.WARNING,
            condition="request_cost > 0.5",
            threshold=0.5,
            window="per_request",
            cooldown=0
        ),
        AlertRule(
            name="hourly_spike",
            level=AlertLevel.WARNING,
            condition="hourly_cost > avg_hourly * 3",
            threshold=3.0,
            window="1h",
            cooldown=3600
        ),
    ]

    def __init__(self, budget_manager: BudgetManager):
        self.budget_manager = budget_manager
        self.rules = self.DEFAULT_RULES
        self.last_alert_time: dict[str, float] = {}

    def check_alerts(self, agent_id: str) -> list[dict]:
        """检查是否需要触发告警"""
        alerts = []
        usage = self.budget_manager.get_usage_report(agent_id)

        for rule in self.rules:
            if self._should_alert(rule, usage):
                alert = self._create_alert(rule, usage)
                alerts.append(alert)
                self.last_alert_time[rule.name] = time.time()

        return alerts

    def _should_alert(self, rule: AlertRule, usage: dict) -> bool:
        """检查是否应该触发告警"""
        # 冷却检查
        last_time = self.last_alert_time.get(rule.name, 0)
        if time.time() - last_time < rule.cooldown:
            return False

        # 条件检查
        if rule.name == "daily_budget_80pct":
            return usage.get("daily_usage_pct", 0) > rule.threshold
        elif rule.name == "single_request_high_cost":
            return usage.get("last_request_cost", 0) > rule.threshold

        return False

    def _create_alert(self, rule: AlertRule, usage: dict) -> dict:
        return {
            "rule": rule.name,
            "level": rule.level.value,
            "message": f"[{rule.level.value.upper()}] {rule.condition}",
            "usage": usage,
            "timestamp": time.time()
        }
```

### 6.2 告警集成

```yaml
# Prometheus告警规则
groups:
  - name: agent_cost_alerts
    rules:
      - alert: AgentDailyBudgetHigh
        expr: agent_daily_cost_usd / agent_daily_budget_usd > 0.8
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Agent {{ $labels.agent_id }} 日预算使用超过80%"

      - alert: AgentDailyBudgetCritical
        expr: agent_daily_cost_usd / agent_daily_budget_usd > 0.95
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Agent {{ $labels.agent_id }} 日预算使用超过95%"

      - alert: AgentRequestCostHigh
        expr: agent_request_cost_usd > 0.5
        labels:
          severity: warning
        annotations:
          summary: "Agent {{ $labels.agent_id }} 单次请求成本过高"

# Grafana Dashboard 关键指标
metrics:
  - agent_daily_cost_usd          # 日成本
  - agent_monthly_cost_usd        # 月成本
  - agent_request_cost_usd        # 单次请求成本
  - agent_cache_hit_rate          # 缓存命中率
  - agent_fallback_rate           # 降级率
  - agent_rate_limit_hit_rate     # 限流命中率
```

## 7. K8s部署配置

```yaml
# Agent限流与成本控制组件部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: agent-cost-controller
spec:
  replicas: 2
  selector:
    matchLabels:
      app: cost-controller
  template:
    metadata:
      labels:
        app: cost-controller
    spec:
      containers:
      - name: controller
        image: agent-cost-controller:latest
        env:
        - name: REDIS_URL
          value: "redis://redis:6379"
        - name: DAILY_GLOBAL_BUDGET
          value: "10000"  # $10,000/天
        - name: ALERT_WEBHOOK_URL
          valueFrom:
            secretKeyRef:
              name: alert-config
              key: webhook-url
        resources:
          requests:
            cpu: "250m"
            memory: "256Mi"
          limits:
            cpu: "1"
            memory: "1Gi"
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: rate-limit-config
data:
  config.yaml: |
    global:
      requests_per_minute: 1000
      tokens_per_minute: 1000000
    per_user:
      requests_per_minute: 60
      tokens_per_minute: 100000
    per_agent:
      requests_per_minute: 200
      tokens_per_minute: 500000
```

## 相关主题

- [[domain-14-ai-ml-infra/03-agent-runtime/18-agent-retry-resilience|Agent弹性设计]]
- [[domain-14-ai-ml-infra/03-agent-runtime/15-cloud-agent-platforms|云Agent平台即服务]]
- [[domain-14-ai-ml-infra/03-agent-runtime/21-agent-runtime-architecture-overview|Agent Runtime架构总览]]

## 参考资料

- Token Bucket算法
- Redis分布式限流
- LLM API定价对比
