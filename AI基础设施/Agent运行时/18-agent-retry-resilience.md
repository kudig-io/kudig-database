---
title: Agent弹性设计
description: 'Agent系统重试策略、熔断器、超时控制、幂等性、死信队列与Chaos Testing'
summary: 'Agent系统重试策略、熔断器、超时控制、幂等性、死信队列与Chaos Testing'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- resilience
- retry
- circuit-breaker
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
- SRE
estimated_read_time: 20min
intent_queries:
- Agent弹性设计 是什么
- 如何实现Agent重试策略
- LLM API熔断器
- Agent幂等性保证
trigger_keywords:
- resilience
- retry
- circuit breaker
- timeout
- idempotency
- dead letter queue
- chaos testing
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

# Agent弹性设计

## 概述

Agent系统的可靠性挑战不同于传统微服务。LLM API具有独特的故障模式：429限流、高延迟波动、非确定性输出、工具调用超时。一次Agent执行链路可能包含5-10次LLM调用和多次工具调用，任一环节失败都影响整体成功率。

本文覆盖重试策略、熔断器、超时控制、幂等性、死信队列和混沌测试，构建端到端的Agent弹性体系。

## 1. 重试策略

### 1.1 指数退避+Jitter

LLM API的429错误需要退避重试，但简单指数退避会导致"惊群效应"（thundering herd）。加入Jitter抖动分散重试时间：

```python
import asyncio
import random
import time
from typing import Callable, Any, Optional
from dataclasses import dataclass

@dataclass
class RetryConfig:
    max_retries: int = 3
    base_delay: float = 1.0        # 基础延迟（秒）
    max_delay: float = 60.0        # 最大延迟
    exponential_base: float = 2.0  # 指数基数
    jitter_range: float = 0.5      # Jitter范围（0-1）

class LLMRetryError(Exception):
    """重试耗尽后的最终错误"""
    def __init__(self, last_error: Exception, attempts: int):
        self.last_error = last_error
        self.attempts = attempts
        super().__init__(f"Failed after {attempts} attempts: {last_error}")


async def retry_with_backoff(
    func: Callable,
    config: RetryConfig = RetryConfig(),
    retryable_errors: tuple = (Exception,),
    on_retry: Optional[Callable] = None
) -> Any:
    """指数退避+Jitter重试"""
    last_error = None

    for attempt in range(config.max_retries + 1):
        try:
            return await func()
        except retryable_errors as e:
            last_error = e

            if attempt == config.max_retries:
                break

            # 指数退避
            delay = min(
                config.base_delay * (config.exponential_base ** attempt),
                config.max_delay
            )

            # Jitter抖动：全范围随机
            jitter = delay * config.jitter_range * random.random()
            actual_delay = delay + jitter

            # 特殊处理429：使用Retry-After头
            if hasattr(e, 'response') and e.response and e.response.status == 429:
                retry_after = e.response.headers.get('Retry-After')
                if retry_after:
                    actual_delay = max(actual_delay, float(retry_after))

            if on_retry:
                await on_retry(attempt + 1, actual_delay, e)

            await asyncio.sleep(actual_delay)

    raise LLMRetryError(last_error, config.max_retries + 1)
```

### 1.2 针对不同错误的重试策略

```python
class SmartRetryStrategy:
    """根据错误类型采用不同重试策略"""

    # 错误分类与策略
    ERROR_STRATEGIES = {
        # 限流错误：长退避
        "rate_limit": {
            "retryable": True,
            "base_delay": 5.0,
            "max_retries": 5,
            "max_delay": 120.0,
        },
        # 服务端错误：标准退避
        "server_error": {
            "retryable": True,
            "base_delay": 1.0,
            "max_retries": 3,
            "max_delay": 30.0,
        },
        # 超时：中等退避
        "timeout": {
            "retryable": True,
            "base_delay": 2.0,
            "max_retries": 2,
            "max_delay": 30.0,
        },
        # 上下文过长：不重试
        "context_length": {
            "retryable": False,
        },
        # 认证失败：不重试
        "auth_error": {
            "retryable": False,
        },
        # 内容过滤：不重试
        "content_filter": {
            "retryable": False,
        },
    }

    @classmethod
    def classify_error(cls, error: Exception) -> str:
        """分类错误类型"""
        error_msg = str(error).lower()

        if hasattr(error, 'status_code'):
            status = error.status_code
            if status == 429:
                return "rate_limit"
            elif status >= 500:
                return "server_error"
            elif status == 401 or status == 403:
                return "auth_error"

        if "timeout" in error_msg or "timed out" in error_msg:
            return "timeout"
        elif "context_length" in error_msg or "too long" in error_msg:
            return "context_length"
        elif "content_filter" in error_msg or "safety" in error_msg:
            return "content_filter"

        return "server_error"  # 默认分类

    @classmethod
    def get_retry_config(cls, error: Exception) -> RetryConfig:
        """根据错误类型获取重试配置"""
        error_type = cls.classify_error(error)
        strategy = cls.ERROR_STRATEGIES[error_type]

        if not strategy["retryable"]:
            raise error  # 不可重试，直接抛出

        return RetryConfig(
            max_retries=strategy["max_retries"],
            base_delay=strategy["base_delay"],
            max_delay=strategy["max_delay"],
        )
```

### 1.3 Agent级别重试

Agent推理失败时的重试需要特殊处理：

```python
class AgentRetryHandler:
    """Agent推理级别的重试"""

    def __init__(self, llm_client, max_round_retries: int = 2):
        self.llm = llm_client
        self.max_round_retries = max_round_retries

    async def execute_with_retry(self, agent_config: dict, user_input: str) -> str:
        """带重试的Agent执行"""
        messages = [{"role": "user", "content": user_input}]

        for round_attempt in range(self.max_round_retries + 1):
            try:
                result = await self._run_agent_loop(agent_config, messages)
                return result
            except AgentLoopError as e:
                if round_attempt == self.max_round_retries:
                    raise

                # 重试时注入错误上下文
                messages.append({
                    "role": "system",
                    "content": f"上一轮推理失败: {e.reason}。请调整策略重试。"
                })
                continue

    async def _run_agent_loop(self, config: dict, messages: list) -> str:
        """执行Agent推理循环"""
        for step in range(config.get("max_steps", 10)):
            response = await retry_with_backoff(
                lambda: self.llm.chat(messages, tools=config.get("tools")),
                config=RetryConfig(max_retries=2)
            )

            if response.tool_calls:
                # 执行工具调用
                tool_results = await self._execute_tools(response.tool_calls)
                messages.append(response)
                messages.extend(tool_results)
            else:
                return response.content

        raise AgentLoopError("Max steps exceeded")
```

## 2. 熔断器

### 2.1 熔断器状态机

```
┌──────────┐  连续失败≥阈值   ┌──────────┐  超时后    ┌──────────┐
│  CLOSED  │ ───────────────→ │   OPEN   │ ────────→ │HALF-OPEN │
│ (正常)    │                  │ (熔断)    │           │ (探测)    │
└──────────┘                  └──────────┘           └──────────┘
     ↑                             │                      │
     │                             │ 探测失败             │ 探测成功
     │                             ▼                      │
     │                        ┌──────────┐                │
     └────────────────────────│   OPEN   │←───────────────┘
                              └──────────┘
```

```python
import time
from enum import Enum
from dataclasses import dataclass, field

class CircuitState(Enum):
    CLOSED = "closed"         # 正常状态，允许请求
    OPEN = "open"             # 熔断状态，拒绝请求
    HALF_OPEN = "half_open"   # 探测状态，允许少量请求

@dataclass
class CircuitBreakerConfig:
    failure_threshold: int = 5       # 连续失败阈值
    recovery_timeout: float = 30.0   # 熔断恢复超时（秒）
    half_open_max_calls: int = 3     # 半开状态最大探测次数
    success_threshold: int = 2       # 半开状态连续成功阈值

class CircuitBreaker:
    """熔断器：保护不可用的Tool/Model"""

    def __init__(self, name: str, config: CircuitBreakerConfig = CircuitBreakerConfig()):
        self.name = name
        self.config = config
        self.state = CircuitState.CLOSED
        self.failure_count = 0
        self.success_count = 0
        self.last_failure_time = 0
        self.half_open_calls = 0

    async def call(self, func, *args, **kwargs):
        """通过熔断器执行调用"""
        if self.state == CircuitState.OPEN:
            if time.time() - self.last_failure_time > self.config.recovery_timeout:
                self.state = CircuitState.HALF_OPEN
                self.half_open_calls = 0
                self.success_count = 0
            else:
                raise CircuitOpenError(
                    f"Circuit {self.name} is OPEN. "
                    f"Retry after {self.config.recovery_timeout}s"
                )

        if self.state == CircuitState.HALF_OPEN:
            if self.half_open_calls >= self.config.half_open_max_calls:
                raise CircuitOpenError(
                    f"Circuit {self.name} half-open limit reached"
                )
            self.half_open_calls += 1

        try:
            result = await func(*args, **kwargs)
            self._on_success()
            return result
        except Exception as e:
            self._on_failure()
            raise

    def _on_success(self):
        """成功回调"""
        if self.state == CircuitState.HALF_OPEN:
            self.success_count += 1
            if self.success_count >= self.config.success_threshold:
                self.state = CircuitState.CLOSED
                self.failure_count = 0
        else:
            self.failure_count = 0

    def _on_failure(self):
        """失败回调"""
        self.failure_count += 1
        self.last_failure_time = time.time()

        if self.state == CircuitState.HALF_OPEN:
            self.state = CircuitState.OPEN
        elif self.failure_count >= self.config.failure_threshold:
            self.state = CircuitState.OPEN

    def get_state(self) -> dict:
        return {
            "name": self.name,
            "state": self.state.value,
            "failure_count": self.failure_count,
            "last_failure": self.last_failure_time,
        }


class CircuitOpenError(Exception):
    """熔断器打开异常"""
    pass
```

### 2.2 多目标熔断管理

```python
class CircuitBreakerManager:
    """管理多个熔断器（每个Tool/Model一个）"""

    def __init__(self):
        self.breakers: dict[str, CircuitBreaker] = {}

    def get_breaker(self, target: str) -> CircuitBreaker:
        """获取目标的熔断器"""
        if target not in self.breakers:
            self.breakers[target] = CircuitBreaker(target)
        return self.breakers[target]

    async def call(self, target: str, func, *args, **kwargs):
        """通过熔断器调用目标"""
        breaker = self.get_breaker(target)
        return await breaker.call(func, *args, **kwargs)

    def get_all_states(self) -> list[dict]:
        """获取所有熔断器状态"""
        return [b.get_state() for b in self.breakers.values()]

    def reset(self, target: str):
        """重置指定熔断器"""
        if target in self.breakers:
            self.breakers[target] = CircuitBreaker(target)


# 使用示例
manager = CircuitBreakerManager()

# Tool调用通过熔断器
async def call_tool(tool_name: str, params: dict):
    return await manager.call(
        f"tool:{tool_name}",
        lambda: tool_registry.execute(tool_name, params)
    )

# Model调用通过熔断器
async def call_model(model_name: str, messages: list):
    return await manager.call(
        f"model:{model_name}",
        lambda: llm_client.chat(model=model_name, messages=messages)
    )
```

## 3. 超时控制

### 3.1 三级超时体系

```python
import asyncio
from dataclasses import dataclass

@dataclass
class TimeoutConfig:
    step_timeout: float = 30.0       # 单步超时（单次LLM/Tool调用）
    round_timeout: float = 120.0     # 单轮超时（一次Agent推理循环）
    global_timeout: float = 300.0    # 全局超时（整个Agent执行）
    stream_timeout: float = 60.0     # Streaming首Token超时

class TimeoutManager:
    """Agent超时管理器"""

    def __init__(self, config: TimeoutConfig = TimeoutConfig()):
        self.config = config

    async def execute_with_timeout(self, func, timeout_type: str = "step"):
        """带超时执行"""
        timeout_map = {
            "step": self.config.step_timeout,
            "round": self.config.round_timeout,
            "global": self.config.global_timeout,
            "stream": self.config.stream_timeout,
        }

        timeout = timeout_map.get(timeout_type, self.config.step_timeout)

        try:
            return await asyncio.wait_for(func(), timeout=timeout)
        except asyncio.TimeoutError:
            raise AgentTimeoutError(
                f"{timeout_type} timeout exceeded ({timeout}s)"
            )

    async def execute_agent(self, agent_func, user_input: str):
        """带完整超时控制的Agent执行"""
        async def _inner():
            return await agent_func(user_input)

        return await self.execute_with_timeout(_inner, "global")

    async def execute_step(self, step_func):
        """带超时的单步执行"""
        return await self.execute_with_timeout(step_func, "step")

    async def execute_streaming(self, stream_func):
        """带超时的Streaming执行"""
        async def _first_token():
            async for chunk in stream_func():
                yield chunk
                break  # 只检查首Token

        return await self.execute_with_timeout(_first_token, "stream")


class AgentTimeoutError(Exception):
    """Agent超时异常"""
    pass
```

### 3.2 Streaming超时

```python
class StreamingTimeoutHandler:
    """Streaming场景的超时处理"""

    def __init__(
        self,
        first_token_timeout: float = 10.0,
        inter_token_timeout: float = 5.0
    ):
        self.first_token_timeout = first_token_timeout
        self.inter_token_timeout = inter_token_timeout

    async def stream_with_timeout(self, stream_gen):
        """带超时的Streaming消费"""
        first_token = True
        last_token_time = time.monotonic()

        async for chunk in stream_gen:
            now = time.monotonic()

            if first_token:
                # 首Token超时检查
                elapsed = now - last_token_time
                if elapsed > self.first_token_timeout:
                    raise StreamingTimeoutError(
                        f"First token timeout: {elapsed:.1f}s > {self.first_token_timeout}s"
                    )
                first_token = False
            else:
                # Token间超时检查
                elapsed = now - last_token_time
                if elapsed > self.inter_token_timeout:
                    raise StreamingTimeoutError(
                        f"Inter-token timeout: {elapsed:.1f}s > {self.inter_token_timeout}s"
                    )

            last_token_time = now
            yield chunk


class StreamingTimeoutError(Exception):
    pass
```

## 4. 幂等性保证

### 4.1 Tool调用去重

Agent重试可能导致同一Tool被重复调用。通过幂等键确保同一调用只执行一次：

```python
import hashlib
import json
from typing import Optional

class IdempotencyManager:
    """Tool调用幂等性管理"""

    def __init__(self, redis_client, ttl: int = 3600):
        self.redis = redis_client
        self.ttl = ttl

    def generate_idempotency_key(
        self,
        agent_id: str,
        tool_name: str,
        parameters: dict
    ) -> str:
        """生成幂等键"""
        content = json.dumps({
            "agent": agent_id,
            "tool": tool_name,
            "params": parameters
        }, sort_keys=True)
        return f"idempotent:{hashlib.sha256(content.encode()).hexdigest()}"

    async def execute_once(
        self,
        agent_id: str,
        tool_name: str,
        parameters: dict,
        tool_func
    ) -> dict:
        """确保Tool只执行一次"""
        key = self.generate_idempotency_key(agent_id, tool_name, parameters)

        # 检查是否已执行
        cached = self.redis.get(key)
        if cached:
            return json.loads(cached)

        # 分布式锁防止并发重复执行
        lock_key = f"lock:{key}"
        lock = self.redis.lock(lock_key, timeout=30)

        if lock.acquire(blocking=True, blocking_timeout=5):
            try:
                # 双重检查
                cached = self.redis.get(key)
                if cached:
                    return json.loads(cached)

                # 执行Tool
                result = await tool_func(parameters)

                # 缓存结果
                self.redis.setex(key, self.ttl, json.dumps(result))
                return result
            finally:
                lock.release()
        else:
            raise IdempotencyLockError(f"Failed to acquire lock for {tool_name}")

    def invalidate(self, agent_id: str, tool_name: str, parameters: dict):
        """使缓存失效（用于需要重新执行的场景）"""
        key = self.generate_idempotency_key(agent_id, tool_name, parameters)
        self.redis.delete(key)


class IdempotencyLockError(Exception):
    pass
```

### 4.2 Agent会话幂等

```python
class AgentSessionIdempotency:
    """Agent会话级别的幂等性"""

    def __init__(self, redis_client):
        self.redis = redis_client

    def check_request_id(
        self,
        request_id: str,
        agent_id: str
    ) -> Optional[dict]:
        """检查请求是否已处理"""
        key = f"request:{agent_id}:{request_id}"
        result = self.redis.get(key)
        if result:
            return json.loads(result)
        return None

    def record_result(
        self,
        request_id: str,
        agent_id: str,
        result: dict,
        ttl: int = 86400
    ):
        """记录请求结果"""
        key = f"request:{agent_id}:{request_id}"
        self.redis.setex(key, ttl, json.dumps(result))

    async def execute_idempotent(
        self,
        request_id: str,
        agent_id: str,
        agent_func
    ) -> dict:
        """幂等执行Agent"""
        # 检查是否已处理
        cached = self.check_request_id(request_id, agent_id)
        if cached:
            return cached

        # 执行并记录
        result = await agent_func()
        self.record_result(request_id, agent_id, result)
        return result
```

## 5. 死信队列

### 5.1 失败任务处理

```python
import json
from datetime import datetime
from enum import Enum

class DLQStatus(Enum):
    PENDING = "pending"
    RETRYING = "retrying"
    RESOLVED = "resolved"
    ABANDONED = "abandoned"

class DeadLetterQueue:
    """Agent失败任务的死信队列"""

    def __init__(self, redis_client):
        self.redis = redis_client
        self.queue_key = "agent:dlq"

    def enqueue(
        self,
        agent_id: str,
        task_id: str,
        error: Exception,
        context: dict
    ):
        """将失败任务加入死信队列"""
        entry = {
            "agent_id": agent_id,
            "task_id": task_id,
            "error": str(error),
            "error_type": type(error).__name__,
            "context": context,
            "enqueued_at": datetime.utcnow().isoformat(),
            "retry_count": 0,
            "status": DLQStatus.PENDING.value,
        }

        self.redis.lpush(self.queue_key, json.dumps(entry))

    def dequeue(self) -> Optional[dict]:
        """取出一个待处理任务"""
        data = self.redis.rpop(self.queue_key)
        if data:
            return json.loads(data)
        return None

    def get_pending(self, limit: int = 100) -> list[dict]:
        """获取待处理任务列表"""
        items = self.redis.lrange(self.queue_key, 0, limit - 1)
        return [json.loads(item) for item in items]

    def retry_task(self, task_id: str, max_retries: int = 3) -> bool:
        """重试指定任务"""
        items = self.redis.lrange(self.queue_key, 0, -1)

        for i, item_data in enumerate(items):
            item = json.loads(item_data)
            if item["task_id"] == task_id:
                if item["retry_count"] >= max_retries:
                    item["status"] = DLQStatus.ABANDONED.value
                    self.redis.lset(self.queue_key, i, json.dumps(item))
                    return False

                item["retry_count"] += 1
                item["status"] = DLQStatus.RETRYING.value
                item["last_retry_at"] = datetime.utcnow().isoformat()
                self.redis.lset(self.queue_key, i, json.dumps(item))
                return True

        return False

    def resolve_task(self, task_id: str):
        """标记任务已解决"""
        items = self.redis.lrange(self.queue_key, 0, -1)

        for i, item_data in enumerate(items):
            item = json.loads(item_data)
            if item["task_id"] == task_id:
                item["status"] = DLQStatus.RESOLVED.value
                item["resolved_at"] = datetime.utcnow().isoformat()
                self.redis.lset(self.queue_key, i, json.dumps(item))
                return

    def get_stats(self) -> dict:
        """获取DLQ统计"""
        items = self.get_pending(limit=10000)
        stats = {"total": len(items), "by_status": {}, "by_agent": {}}

        for item in items:
            status = item.get("status", "unknown")
            agent = item.get("agent_id", "unknown")
            stats["by_status"][status] = stats["by_status"].get(status, 0) + 1
            stats["by_agent"][agent] = stats["by_agent"].get(agent, 0) + 1

        return stats
```

## 6. Chaos Testing for Agent

### 6.1 故障注入框架

```python
import asyncio
import random
from typing import Callable, Optional
from enum import Enum

class ChaosType(Enum):
    LATENCY = "latency"           # 延迟注入
    ERROR = "error"               # 错误注入
    TIMEOUT = "timeout"           # 超时注入
    RATE_LIMIT = "rate_limit"     # 限流注入
    PARTIAL_RESPONSE = "partial"  # 部分响应
    SLOW_STREAM = "slow_stream"   # 慢Streaming

class ChaosConfig:
    """混沌测试配置"""
    def __init__(
        self,
        chaos_type: ChaosType,
        probability: float = 0.1,  # 10%概率触发
        latency_ms: int = 5000,
        error_rate: float = 0.5
    ):
        self.chaos_type = chaos_type
        self.probability = probability
        self.latency_ms = latency_ms
        self.error_rate = error_rate

class ChaosAgent:
    """Agent混沌测试注入器"""

    def __init__(self, configs: list[ChaosConfig]):
        self.configs = configs
        self.active = True

    async def inject_chaos(self, original_func: Callable, *args, **kwargs):
        """在调用前注入混沌"""
        if not self.active:
            return await original_func(*args, **kwargs)

        for config in self.configs:
            if random.random() < config.probability:
                return await self._apply_chaos(config, original_func, *args, **kwargs)

        return await original_func(*args, **kwargs)

    async def _apply_chaos(
        self,
        config: ChaosConfig,
        original_func: Callable,
        *args,
        **kwargs
    ):
        """应用混沌故障"""
        if config.chaos_type == ChaosType.LATENCY:
            delay = random.uniform(0, config.latency_ms / 1000)
            await asyncio.sleep(delay)
            return await original_func(*args, **kwargs)

        elif config.chaos_type == ChaosType.ERROR:
            raise ChaosInjectedError("Chaos: Injected LLM error")

        elif config.chaos_type == ChaosType.TIMEOUT:
            await asyncio.sleep(999)  # 触发超时
            raise asyncio.TimeoutError("Chaos: Injected timeout")

        elif config.chaos_type == ChaosType.RATE_LIMIT:
            raise ChaosRateLimitError("Chaos: Rate limited (429)")

        elif config.chaos_type == ChaosType.PARTIAL_RESPONSE:
            result = await original_func(*args, **kwargs)
            # 截断响应
            if isinstance(result, str):
                return result[:len(result)//2]
            return result

    def enable(self):
        self.active = True

    def disable(self):
        self.active = False


class ChaosInjectedError(Exception):
    pass

class ChaosRateLimitError(Exception):
    pass
```

### 6.2 Agent Chaos Test Suite

```python
class AgentChaosTestSuite:
    """Agent弹性测试套件"""

    def __init__(self, agent_factory: Callable):
        self.agent_factory = agent_factory
        self.results: list[dict] = []

    async def test_retry_resilience(self):
        """测试重试弹性"""
        chaos = ChaosAgent([
            ChaosConfig(ChaosType.ERROR, probability=0.5),
        ])

        agent = self.agent_factory()
        success_count = 0
        total = 10

        for i in range(total):
            try:
                result = await chaos.inject_chaos(
                    lambda: agent.execute("test query")
                )
                success_count += 1
            except Exception:
                pass

        self.results.append({
            "test": "retry_resilience",
            "success_rate": success_count / total,
            "passed": success_count / total > 0.7,
        })

    async def test_circuit_breaker(self):
        """测试熔断器"""
        chaos = ChaosAgent([
            ChaosConfig(ChaosType.ERROR, probability=1.0),
        ])

        agent = self.agent_factory()
        open_detected = False

        for i in range(20):
            try:
                await chaos.inject_chaos(lambda: agent.execute("test"))
            except CircuitOpenError:
                open_detected = True
                break
            except Exception:
                pass

        self.results.append({
            "test": "circuit_breaker",
            "circuit_opened": open_detected,
            "passed": open_detected,
        })

    async def test_timeout_handling(self):
        """测试超时处理"""
        chaos = ChaosAgent([
            ChaosConfig(ChaosType.LATENCY, probability=1.0, latency_ms=60000),
        ])

        agent = self.agent_factory()
        timeout_detected = False

        try:
            await asyncio.wait_for(
                chaos.inject_chaos(lambda: agent.execute("test")),
                timeout=5.0
            )
        except (asyncio.TimeoutError, AgentTimeoutError):
            timeout_detected = True

        self.results.append({
            "test": "timeout_handling",
            "timeout_detected": timeout_detected,
            "passed": timeout_detected,
        })

    async def test_graceful_degradation(self):
        """测试优雅降级"""
        chaos = ChaosAgent([
            ChaosConfig(ChaosType.RATE_LIMIT, probability=0.8),
        ])

        agent = self.agent_factory()
        degraded_success = 0
        total = 10

        for i in range(total):
            try:
                result = await chaos.inject_chaos(
                    lambda: agent.execute("test", allow_degraded=True)
                )
                if result:
                    degraded_success += 1
            except Exception:
                pass

        self.results.append({
            "test": "graceful_degradation",
            "degraded_success_rate": degraded_success / total,
            "passed": degraded_success / total > 0.5,
        })

    def get_report(self) -> dict:
        """生成测试报告"""
        total = len(self.results)
        passed = sum(1 for r in self.results if r.get("passed"))

        return {
            "total_tests": total,
            "passed": passed,
            "failed": total - passed,
            "pass_rate": passed / total if total > 0 else 0,
            "details": self.results,
        }
```

### 6.3 K8s Chaos实验

```yaml
# Litmus Chaos实验：LLM API故障注入
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: agent-chaos-engine
spec:
  appinfo:
    appns: agent-system
    applabel: app=agent-runtime
    appkind: deployment
  chaosServiceAccount: litmus-admin
  experiments:
    - name: pod-network-latency
      spec:
        components:
          env:
            - name: NETWORK_LATENCY
              value: "5000"   # 5秒延迟
            - name: DESTINATION_PORTS
              value: "443"    # HTTPS端口
        probe:
          - name: agent-success-rate-check
            type: httpProbe
            httpProbe/inputs:
              url: http://agent-service/health
              method:
                get:
                  criteria: "=="
                  responseCode: "200"
            mode: Continuous
            runProperties:
              probeTimeout: 5s
              interval: 10s

    - name: pod-delete
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: "30"
            - name: CHAOS_INTERVAL
              value: "10"
        probe:
          - name: agent-recovery-check
            type: httpProbe
            httpProbe/inputs:
              url: http://agent-service/ready
              method:
                get:
                  criteria: "=="
                  responseCode: "200"
            mode: Edge
```

## 相关主题

- [[AI基础设施/Agent运行时/17-agent-rate-limiting-cost-control|Agent限流与成本控制]]
- [[AI基础设施/Agent运行时/19-agent-ci-cd-pipeline|Agent CI/CD流水线]]
- [[AI基础设施/Agent运行时/21-agent-runtime-architecture-overview|Agent Runtime架构总览]]

## 参考资料

- Exponential Backoff and Jitter
- Circuit Breaker Pattern
- Chaos Engineering Principles
- Litmus Chaos Documentation
