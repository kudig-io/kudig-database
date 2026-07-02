---
title: Agent Harness Loop 与执行引擎深度设计 (domain-14-ai-ml-infra)
description: 'title: Agent Harness Loop 与执行引擎深度设计'
summary: 'title: Agent Harness Loop 与执行引擎深度设计'
category: general
tags:
- ai
- ai-agent
- prometheus
- llm
- rag
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- Agent Harness Loop 与执行引擎深度设计 是什么
- 如何 Agent Harness Loop 与执行引擎深度设计
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- Agent
- Harness
- Loop
- 与执行引擎深度设计
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- prometheus-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: Agent Harness Loop 与执行引擎深度设计
description: '# Agent Harness Loop 与执行引擎深度设计'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[Prometheus|prometheus]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- Agent Harness Loop 与执行引擎深度设计 是什么
- 如何 Agent Harness Loop 与执行引擎深度设计
trigger_keywords:
- Agent
- Harness
- Loop
- 与执行引擎深度设计
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

# Agent Harness Loop 与执行引擎深度设计

> **文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Agent Loop, 执行引擎, 状态机, ReAct Loop, 反漂移, 超时保护, 异步执行, 有限状态机, Trajectory, 执行策略

---

<!-- chunk: 概述 -->## 概述

Loop（循环层）是 Agent Harness 六层架构的第一层，也是整个 Harness 的**执行心脏**。它决定了 Agent 如何观察、思考、行动，以及何时终止。一个设计良好的 Loop 层不仅驱动 Agent 完成任务，还负责异常处理、漂移检测、资源管控和执行轨迹记录。

本文从执行引擎的底层设计出发，深入探讨 Loop 层的状态机模型、执行策略、反漂移算法、并发控制、故障恢复机制，以及在 K8S 运维场景中的完整实现。

---

<!-- chunk: 1. Loop 层核心模型 -->## 1. Loop 层核心模型

## 1.1 有限状态机（FSM）模型

Agent Loop 的本质是一个有限状态机。每个循环迭代在以下状态间转移：

```
Agent Loop 有限状态机:

INIT ──→ OBSERVE ──→ THINK ──→ DECIDE
                                  │
                        ┌─────────┴─────────┐
                        ▼                   ▼
                      ACT                FINALIZE
                        │                   │
                        ▼                   ▼
                   OBSERVE_RESULT        OUTPUT
                        │
                        ▼
                    EVALUATE
                        │
              ┌─────────┴─────────┐
              ▼                   ▼
          CONTINUE              TERMINATE
              │                   │
              └──→ OBSERVE        ▼
                               OUTPUT

终止条件:
  - 任务完成（Agent 判断 is_final_answer）
  - 超时（wall-clock timeout）
  - 迭代上限（max_iterations）
  - 漂移检测（drift detected）
  - 约束违反（constraint violation）
  - 异常中断（unrecoverable error）
```

## 1.2 状态定义与转移规则

```python
from enum import Enum, auto
from dataclasses import dataclass, field
from typing import Optional, Any
import time

class LoopState(Enum):
    """Agent Loop 状态枚举"""
    INIT = auto()
    OBSERVE = auto()
    THINK = auto()
    DECIDE = auto()
    ACT = auto()
    OBSERVE_RESULT = auto()
    EVALUATE = auto()
    FINALIZE = auto()
    TERMINATED = auto()

@dataclass
class LoopStep:
    """单步执行记录"""
    iteration: int
    state: LoopState
    timestamp: float
    thought: Optional[str] = None
    action: Optional[dict] = None
    observation: Optional[str] = None
    tool_call: Optional[dict] = None
    tool_result: Optional[dict] = None
    tokens_used: int = 0
    latency_ms: float = 0.0
    metadata: dict = field(default_factory=dict)

class TerminationReason(Enum):
    """终止原因枚举"""
    TASK_COMPLETE = "task_complete"
    TIMEOUT = "timeout"
    MAX_ITERATIONS = "max_iterations"
    DRIFT_DETECTED = "drift_detected"
    CONSTRAINT_VIOLATION = "constraint_violation"
    UNRECOVERABLE_ERROR = "unrecoverable_error"
    HUMAN_INTERRUPT = "human_interrupt"
    COST_BUDGET_EXCEEDED = "cost_budget_exceeded"
```

---

<!-- chunk: 2. 执行引擎架构 -->## 2. 执行引擎架构

## 2.1 核心执行引擎实现

```python
import asyncio
import logging
from typing import Callable

logger = logging.getLogger("agent.loop")

class ExecutionEngine:
    """Agent 核心执行引擎

    职责：
    1. 驱动 Agent Loop 的状态转移
    2. 管理执行生命周期（启动、暂停、恢复、终止）
    3. 记录完整执行轨迹
    4. 强制执行终止条件
    """

    def __init__(
        self,
        llm,
        tools,
        context_manager,
        constraint_enforcer,
        max_iterations: int = 20,
        timeout_seconds: int = 300,
        think_budget_ratio: float = 0.6,
    ):
        self.llm = llm
        self.tools = tools
        self.context_mgr = context_manager
        self.constraints = constraint_enforcer
        self.max_iterations = max_iterations
        self.timeout_seconds = timeout_seconds
        self.think_budget_ratio = think_budget_ratio

        # 执行状态
        self._state = LoopState.INIT
        self._trajectory: list[LoopStep] = []
        self._start_time: float = 0
        self._total_tokens: int = 0
        self._is_paused: bool = False

        # 回调钩子
        self._on_step_complete: list[Callable] = []
        self._on_terminate: list[Callable] = []

    def run(self, task: str, initial_context: dict = None) -> dict:
        """同步执行 Agent Loop"""
        self._state = LoopState.INIT
        self._start_time = time.time()
        self._trajectory = []
        iteration = 0

        # 初始上下文构建
        context = self.context_mgr.build_context(task, initial_context)

        while iteration < self.max_iterations:
            # 终止条件检查
            termination = self._check_termination_conditions(iteration)
            if termination:
                return self._build_result(termination, iteration)

            # 暂停检查（支持人工干预）
            if self._is_paused:
                self._wait_for_resume()

            step_start = time.time()

            # OBSERVE: 收集当前状态
            self._state = LoopState.OBSERVE
            observation = self._observe(task, context, iteration)

            # THINK: LLM 推理
            self._state = LoopState.THINK
            thought = self._think(observation, iteration)

            # DECIDE: 判断是否需要行动
            self._state = LoopState.DECIDE
            if thought.is_final_answer:
                self._state = LoopState.FINALIZE
                return self._build_result(
                    TerminationReason.TASK_COMPLETE,
                    iteration + 1,
                    answer=thought.answer,
                )

            # ACT: 执行工具调用
            self._state = LoopState.ACT
            action_result = self._act(thought.action, iteration)

            # OBSERVE_RESULT: 观察工具结果
            self._state = LoopState.OBSERVE_RESULT
            context = self._update_context(context, thought, action_result)

            # EVALUATE: 评估是否继续
            self._state = LoopState.EVALUATE
            step = LoopStep(
                iteration=iteration,
                state=self._state,
                timestamp=time.time(),
                thought=thought.reasoning,
                action=thought.action,
                tool_result=action_result,
                tokens_used=thought.tokens_used,
                latency_ms=(time.time() - step_start) * 1000,
            )
            self._trajectory.append(step)

            # 触发步骤完成回调
            for callback in self._on_step_complete:
                callback(step)

            iteration += 1

        return self._build_result(TerminationReason.MAX_ITERATIONS, iteration)

    def _check_termination_conditions(self, iteration: int) -> Optional[TerminationReason]:
        """检查所有终止条件"""
        # 超时检查
        elapsed = time.time() - self._start_time
        if elapsed > self.timeout_seconds:
            logger.warning(f"Timeout after {elapsed:.1f}s (limit: {self.timeout_seconds}s)")
            return TerminationReason.TIMEOUT

        # 成本预算检查
        allowed, reason = self.constraints.check_budget(self._total_tokens)
        if not allowed:
            logger.warning(f"Budget exceeded: {reason}")
            return TerminationReason.COST_BUDGET_EXCEEDED

        # 漂移检测
        if self._detect_drift():
            logger.warning("Drift detected in agent loop")
            return TerminationReason.DRIFT_DETECTED

        return None

    def _build_result(
        self,
        reason: TerminationReason,
        iterations: int,
        answer: str = None,
    ) -> dict:
        """构建执行结果"""
        elapsed = time.time() - self._start_time
        result = {
            "status": reason.value,
            "answer": answer,
            "iterations": iterations,
            "total_tokens": self._total_tokens,
            "elapsed_seconds": elapsed,
            "trajectory": self._trajectory,
            "termination_reason": reason.value,
        }

        # 触发终止回调
        for callback in self._on_terminate:
            callback(result)

        return result
```

## 2.2 异步执行引擎

生产环境中，Agent 通常需要并发处理多个任务或并行调用多个工具：

```python
class AsyncExecutionEngine:
    """异步执行引擎：支持并发工具调用和非阻塞执行"""

    def __init__(self, llm, tools, max_concurrent_tools: int = 3, **kwargs):
        self.llm = llm
        self.tools = tools
        self.max_concurrent_tools = max_concurrent_tools
        self._semaphore = asyncio.Semaphore(max_concurrent_tools)

    async def run(self, task: str) -> dict:
        """异步执行 Agent Loop"""
        trajectory = []
        iteration = 0

        while iteration < self.max_iterations:
            # 异步 LLM 推理
            thought = await self._async_think(task, trajectory)

            if thought.is_final_answer:
                return {"status": "success", "answer": thought.answer,
                        "trajectory": trajectory}

            # 并行工具调用（如果 Agent 请求多个工具）
            if thought.parallel_actions:
                results = await self._execute_parallel(thought.parallel_actions)
            else:
                results = [await self._execute_single(thought.action)]

            trajectory.append({
                "iteration": iteration,
                "thought": thought.reasoning,
                "actions": thought.parallel_actions or [thought.action],
                "results": results,
            })
            iteration += 1

        return {"status": "max_iterations", "trajectory": trajectory}

    async def _execute_parallel(self, actions: list) -> list:
        """并行执行多个工具调用"""
        async def _execute_with_semaphore(action):
            async with self._semaphore:
                return await self._execute_single(action)

        tasks = [_execute_with_semaphore(a) for a in actions]
        return await asyncio.gather(*tasks, return_exceptions=True)

    async def _execute_single(self, action: dict) -> dict:
        """执行单个工具调用（带超时保护）"""
        tool_name = action.get("tool")
        tool_args = action.get("args", {})

        try:
            result = await asyncio.wait_for(
                self.tools.async_execute(tool_name, tool_args),
                timeout=30.0,  # 单工具调用 30s 超时
            )
            return {"success": True, "result": result}
        except asyncio.TimeoutError:
            return {"success": False, "error": f"Tool {tool_name} timed out (30s)"}
        except Exception as e:
            return {"success": False, "error": str(e)}
```

---

<!-- chunk: 3. 反漂移检测算法 -->## 3. 反漂移检测算法

## 3.1 漂移类型分类

Agent 在执行过程中可能陷入多种漂移模式：

```
# 🟢 低风险：只读/信息收集，通常无副作用
Agent 漂移类型分类:

1. 动作重复漂移（Action Repetition Drift）
   Agent 反复执行完全相同的动作
   示例: 连续 5 次执行 kubectl get pods
   检测: 连续 N 次动作相同

2. 内容循环漂移（Content Loop Drift）
   Agent 反复编辑同一文件/资源
   示例: 修改 YAML → 报错 → 回退 → 修改 → 报错 → 回退
   检测: 动作目标的循环模式

3. 语义停滞漂移（Semantic Stagnation Drift）
   Agent 的推理没有实质性进展
   示例: 每轮思考的内容高度相似但不前进
   检测: 思考内容的语义相似度 > 阈值

4. 错误循环漂移（Error Loop Drift）
   Agent 反复遇到同一错误但无法解决
   示例: 连续遇到权限错误但不切换策略
   检测: 连续 N 次相同错误类型

5. 目标偏离漂移（Goal Deviation Drift）
   Agent 的行动越来越偏离原始目标
   示例: 诊断 Pod 问题时开始优化节点网络
   检测: 动作与目标的语义距离增加
```
## 3.2 多维度漂移检测器

```python
from collections import Counter
import hashlib

class DriftDetector:
    """多维度漂移检测器"""

    def __init__(
        self,
        action_window: int = 3,
        error_window: int = 4,
        similarity_threshold: float = 0.92,
        max_same_target_edits: int = 3,
    ):
        self.action_window = action_window
        self.error_window = error_window
        self.similarity_threshold = similarity_threshold
        self.max_same_target_edits = max_same_target_edits

    def detect(self, trajectory: list) -> Optional[dict]:
        """多维度漂移检测"""
        if len(trajectory) < self.action_window:
            return None

        # 检测 1: 动作重复漂移
        action_drift = self._detect_action_repetition(trajectory)
        if action_drift:
            return action_drift

        # 检测 2: 内容循环漂移
        content_drift = self._detect_content_loop(trajectory)
        if content_drift:
            return content_drift

        # 检测 3: 错误循环漂移
        error_drift = self._detect_error_loop(trajectory)
        if error_drift:
            return error_drift

        # 检测 4: 语义停滞漂移
        semantic_drift = self._detect_semantic_stagnation(trajectory)
        if semantic_drift:
            return semantic_drift

        return None

    def _detect_action_repetition(self, trajectory: list) -> Optional[dict]:
        """检测连续相同动作"""
        recent = trajectory[-self.action_window:]
        action_hashes = [
            hashlib.md5(str(step.get("action", "")).encode()).hexdigest()
            for step in recent
        ]
        if len(set(action_hashes)) == 1:
            return {
                "type": "action_repetition",
                "severity": "high",
                "message": f"连续 {self.action_window} 次执行相同动作",
                "repeated_action": recent[-1].get("action"),
                "recommendation": "切换策略或请求人工介入",
            }
        return None

    def _detect_content_loop(self, trajectory: list) -> Optional[dict]:
        """检测编辑循环（A→B→A→B 模式）"""
        if len(trajectory) < 4:
            return None

        recent = trajectory[-6:]
        targets = [step.get("action", {}).get("target", "") for step in recent]
        target_counts = Counter(targets)
        for target, count in target_counts.items():
            if target and count >= self.max_same_target_edits:
                return {
                    "type": "content_loop",
                    "severity": "medium",
                    "message": f"对同一目标 '{target}' 反复操作 {count} 次",
                    "target": target,
                    "recommendation": "检查操作是否产生预期效果",
                }
        return None

    def _detect_error_loop(self, trajectory: list) -> Optional[dict]:
        """检测连续相同错误"""
        recent = trajectory[-self.error_window:]
        errors = [
            step.get("tool_result", {}).get("error", "")
            for step in recent
            if step.get("tool_result", {}).get("error")
        ]
        if len(errors) >= self.error_window:
            error_hashes = [hashlib.md5(e.encode()).hexdigest() for e in errors]
            if len(set(error_hashes)) <= 2:  # 最多 2 种错误类型
                return {
                    "type": "error_loop",
                    "severity": "high",
                    "message": f"连续 {len(errors)} 次遇到相同/相似错误",
                    "errors": errors[-2:],
                    "recommendation": "需要切换诊断策略或升级处理",
                }
        return None

    def _detect_semantic_stagnation(self, trajectory: list) -> Optional[dict]:
        """检测语义停滞（推理内容高度相似但无进展）"""
        if len(trajectory) < 4:
            return None

        recent_thoughts = [
            step.get("thought", "")
            for step in trajectory[-4:]
            if step.get("thought")
        ]
        if len(recent_thoughts) < 3:
            return None

        # 使用简单的 Jaccard 相似度（生产环境建议用 embedding 相似度）
        similarities = []
        for i in range(len(recent_thoughts) - 1):
            sim = self._jaccard_similarity(recent_thoughts[i], recent_thoughts[i + 1])
            similarities.append(sim)

        avg_sim = sum(similarities) / len(similarities)
        if avg_sim > self.similarity_threshold:
            return {
                "type": "semantic_stagnation",
                "severity": "medium",
                "message": f"推理内容相似度 {avg_sim:.2f} 超过阈值",
                "recommendation": "注入新信息或重新构建问题",
            }
        return None

    @staticmethod
    def _jaccard_similarity(text1: str, text2: str) -> float:
        """Jaccard 文本相似度"""
        set1 = set(text1.split())
        set2 = set(text2.split())
        if not set1 or not set2:
            return 0.0
        intersection = set1 & set2
        union = set1 | set2
        return len(intersection) / len(union)
```

## 3.3 漂移恢复策略

```python
class DriftRecoveryStrategy:
    """漂移恢复策略"""

    def __init__(self, llm, max_recovery_attempts: int = 2):
        self.llm = llm
        self.max_recovery_attempts = max_recovery_attempts

    def recover(self, drift_info: dict, trajectory: list, task: str) -> dict:
        """根据漂移类型选择恢复策略"""
        drift_type = drift_info["type"]

        strategies = {
            "action_repetition": self._strategy_reframe,
            "content_loop": self._strategy_backtrack,
            "error_loop": self._strategy_alternative_tools,
            "semantic_stagnation": self._strategy_inject_context,
        }

        strategy = strategies.get(drift_type, self._strategy_reframe)
        return strategy(drift_info, trajectory, task)

    def _strategy_reframe(self, drift_info, trajectory, task) -> dict:
        """重构策略：让 Agent 重新理解任务"""
        recovery_prompt = f"""
        你在执行任务时陷入了重复循环。请停下来重新分析：
        
        原始任务: {task}
        已执行步骤: {len(trajectory)}
        问题: {drift_info['message']}
        
        请用完全不同的方法重新思考这个任务。不要重复之前的动作。
        """
        return {"strategy": "reframe", "prompt": recovery_prompt}

    def _strategy_backtrack(self, drift_info, trajectory, task) -> dict:
        """回退策略：回到最后一个成功状态"""
        last_success = None
        for step in reversed(trajectory):
            if step.get("tool_result", {}).get("success"):
                last_success = step
                break
        return {
            "strategy": "backtrack",
            "restore_point": last_success,
            "prompt": "从上一个成功状态重新开始，尝试不同的路径",
        }

    def _strategy_alternative_tools(self, drift_info, trajectory, task) -> dict:
        """替代工具策略：排除已失败的工具"""
        failed_tools = set()
        for step in trajectory[-4:]:
            if not step.get("tool_result", {}).get("success"):
                failed_tools.add(step.get("action", {}).get("tool"))
        return {
            "strategy": "alternative_tools",
            "excluded_tools": list(failed_tools),
            "prompt": f"以下工具暂时不可用: {failed_tools}，请使用其他工具完成任务",
        }

    def _strategy_inject_context(self, drift_info, trajectory, task) -> dict:
        """注入上下文策略：补充新信息打破停滞"""
        return {
            "strategy": "inject_context",
            "prompt": "请从不同角度审视任务，考虑之前忽略的信息和方法",
            "additional_context": "补充环境信息或相关文档",
        }
```

---

<!-- chunk: 4. 执行策略模式 -->## 4. 执行策略模式

## 4.1 策略模式分类

```
Agent 执行策略分类:

1. 线性执行策略（Sequential）
   步骤按严格顺序执行
   适用: SOP 驱动的标准流程
   示例: Pod 诊断 SOP

2. 自适应执行策略（Adaptive）
   根据每步结果动态调整下一步
   适用: 探索性任务
   示例: 未知问题根因分析

3. 分支执行策略（Branching）
   在关键决策点分叉，并行探索多条路径
   适用: 多种可能原因的诊断
   示例: 同时检查网络、存储、调度

4. 分阶段执行策略（Phased）
   分为信息收集、分析、行动三个阶段
   适用: 复杂运维任务
   示例: 大规模问题处置

5. 递归执行策略（Recursive）
   将大任务分解为子任务，递归执行
   适用: 多集群批量操作
   示例: 跨集群升级
```

## 4.2 分阶段执行引擎

```python
class PhasedExecutionEngine:
    """分阶段执行引擎：将任务分为收集→分析→行动三阶段"""

    def __init__(self, llm, tools, phase_configs: dict = None):
        self.llm = llm
        self.tools = tools
        self.phase_configs = phase_configs or {
            "gather": {"max_steps": 5, "tools": ["kubectl_get", "kubectl_describe",
                                                   "kubectl_logs", "kubectl_events"]},
            "analyze": {"max_steps": 3, "tools": ["prometheus_query", "loki_search"]},
            "act": {"max_steps": 5, "tools": ["kubectl_apply", "kubectl_patch"],
                    "require_approval": True},
        }

    def run(self, task: str) -> dict:
        """三阶段执行"""
        results = {}

        # Phase 1: 信息收集
        gather_result = self._execute_phase(
            "gather", task,
            system_prompt="你正在信息收集阶段。只收集信息，不做修改。"
        )
        results["gather"] = gather_result

        # Phase 2: 分析推理
        analysis_context = self._build_analysis_context(gather_result)
        analyze_result = self._execute_phase(
            "analyze", task,
            context=analysis_context,
            system_prompt="根据收集到的信息进行分析，确定根因和修复方案。"
        )
        results["analyze"] = analyze_result

        # Phase 3: 执行修复（需要审批）
        if analyze_result.get("action_plan"):
            act_result = self._execute_phase(
                "act", task,
                context=analyze_result,
                system_prompt="按照分析阶段的方案执行修复操作。"
            )
            results["act"] = act_result

        return results

    def _execute_phase(self, phase: str, task: str,
                       context: dict = None, system_prompt: str = None) -> dict:
        """执行单个阶段"""
        config = self.phase_configs[phase]
        available_tools = [
            t for t in self.tools if t.name in config.get("tools", [])
        ]

        engine = ExecutionEngine(
            llm=self.llm,
            tools=available_tools,
            max_iterations=config["max_steps"],
        )

        return engine.run(task, initial_context=context)
```

---

<!-- chunk: 5. 执行轨迹管理 -->## 5. 执行轨迹管理

## 5.1 Trajectory 数据模型

```python
from dataclasses import dataclass, field
from typing import Optional, Any
from datetime import datetime
import json

@dataclass
class TrajectoryEntry:
    """轨迹条目：记录 Agent 的每一步"""
    step_id: str
    iteration: int
    timestamp: str
    phase: str                       # gather / analyze / act
    thought: str                     # Agent 的推理过程
    action: Optional[dict]           # 执行的动作
    observation: Optional[str]       # 观察到的结果
    tool_name: Optional[str]         # 使用的工具
    tool_args: Optional[dict]        # 工具参数
    tool_result: Optional[Any]       # 工具返回结果
    tool_success: bool = True
    tokens_input: int = 0
    tokens_output: int = 0
    latency_ms: float = 0.0
    is_key_step: bool = False        # 标记关键步骤
    error: Optional[str] = None

@dataclass
class ExecutionTrajectory:
    """完整执行轨迹"""
    task_id: str
    task: str
    start_time: str
    end_time: Optional[str] = None
    status: str = "running"
    entries: list[TrajectoryEntry] = field(default_factory=list)
    total_tokens: int = 0
    total_cost_usd: float = 0.0
    termination_reason: Optional[str] = None

    def add_entry(self, entry: TrajectoryEntry):
        """添加轨迹条目"""
        self.entries.append(entry)
        self.total_tokens += entry.tokens_input + entry.tokens_output

    def get_key_steps(self) -> list:
        """获取关键步骤（用于历史压缩）"""
        return [e for e in self.entries if e.is_key_step]

    def get_error_steps(self) -> list:
        """获取错误步骤（用于失败分析）"""
        return [e for e in self.entries if e.error]

    def to_summary(self) -> str:
        """生成轨迹摘要（用于日志/审计）"""
        lines = [f"Task: {self.task}", f"Status: {self.status}",
                 f"Steps: {len(self.entries)}", f"Tokens: {self.total_tokens}"]
        for entry in self.entries:
            tool_info = f" [{entry.tool_name}]" if entry.tool_name else ""
            status = "✓" if entry.tool_success else "✗"
            lines.append(f"  {status} Step {entry.iteration}{tool_info}: "
                        f"{entry.thought[:80]}...")
        return "\n".join(lines)

    def export_json(self) -> str:
        """导出为 JSON（用于存储/分析）"""
        return json.dumps({
            "task_id": self.task_id,
            "task": self.task,
            "start_time": self.start_time,
            "end_time": self.end_time,
            "status": self.status,
            "total_tokens": self.total_tokens,
            "total_cost_usd": self.total_cost_usd,
            "termination_reason": self.termination_reason,
            "entries": [vars(e) for e in self.entries],
        }, indent=2, ensure_ascii=False)
```

## 5.2 轨迹分析与优化

```python
class TrajectoryAnalyzer:
    """轨迹分析器：从历史执行中提取优化洞察"""

    def analyze(self, trajectories: list[ExecutionTrajectory]) -> dict:
        """分析多条执行轨迹"""
        return {
            "efficiency": self._analyze_efficiency(trajectories),
            "failure_patterns": self._analyze_failures(trajectories),
            "tool_usage": self._analyze_tool_usage(trajectories),
            "bottlenecks": self._identify_bottlenecks(trajectories),
        }

    def _analyze_efficiency(self, trajectories) -> dict:
        """效率分析"""
        steps_list = [len(t.entries) for t in trajectories]
        token_list = [t.total_tokens for t in trajectories]
        success_count = sum(1 for t in trajectories if t.status == "success")
        return {
            "avg_steps": sum(steps_list) / len(steps_list) if steps_list else 0,
            "avg_tokens": sum(token_list) / len(token_list) if token_list else 0,
            "success_rate": success_count / len(trajectories) if trajectories else 0,
            "p95_steps": sorted(steps_list)[int(len(steps_list) * 0.95)]
            if steps_list else 0,
        }

    def _analyze_failures(self, trajectories) -> list:
        """失败模式分析"""
        failure_patterns = {}
        for t in trajectories:
            if t.status != "success":
                reason = t.termination_reason or "unknown"
                failure_patterns[reason] = failure_patterns.get(reason, 0) + 1
        return sorted(
            [{"reason": k, "count": v} for k, v in failure_patterns.items()],
            key=lambda x: x["count"], reverse=True,
        )

    def _analyze_tool_usage(self, trajectories) -> dict:
        """工具使用分析"""
        tool_stats = {}
        for t in trajectories:
            for entry in t.entries:
                if entry.tool_name:
                    if entry.tool_name not in tool_stats:
                        tool_stats[entry.tool_name] = {
                            "total_calls": 0, "success": 0, "failures": 0,
                            "avg_latency_ms": 0, "latencies": [],
                        }
                    stats = tool_stats[entry.tool_name]
                    stats["total_calls"] += 1
                    if entry.tool_success:
                        stats["success"] += 1
                    else:
                        stats["failures"] += 1
                    stats["latencies"].append(entry.latency_ms)

        for name, stats in tool_stats.items():
            stats["avg_latency_ms"] = (
                sum(stats["latencies"]) / len(stats["latencies"])
                if stats["latencies"] else 0
            )
            stats["success_rate"] = (
                stats["success"] / stats["total_calls"]
                if stats["total_calls"] else 0
            )
            del stats["latencies"]

        return tool_stats

    def _identify_bottlenecks(self, trajectories) -> list:
        """瓶颈识别"""
        bottlenecks = []

        # 识别慢工具
        for t in trajectories:
            for entry in t.entries:
                if entry.latency_ms > 5000:  # > 5s
                    bottlenecks.append({
                        "type": "slow_tool",
                        "tool": entry.tool_name,
                        "latency_ms": entry.latency_ms,
                        "task_id": t.task_id,
                    })

        # 识别高 token 消耗步骤
        for t in trajectories:
            for entry in t.entries:
                total = entry.tokens_input + entry.tokens_output
                if total > 10000:
                    bottlenecks.append({
                        "type": "high_token_step",
                        "step": entry.iteration,
                        "tokens": total,
                        "task_id": t.task_id,
                    })

        return bottlenecks
```

---

<!-- chunk: 6. K8S 运维场景 Loop 实战 -->## 6. K8S 运维场景 Loop 实战

## 6.1 Pod Pending 诊断 Loop

```python
class PodPendingDiagnosisLoop:
    """Pod Pending 场景的标准诊断 Loop"""

    DIAGNOSIS_SOP = [
        {"step": "describe_pod", "tool": "kubectl_describe",
         "args_template": "pod {pod_name} -n {namespace}",
         "extract": ["Events", "Conditions", "Status"]},
        {"step": "check_events", "tool": "kubectl_events",
         "args_template": "--field-selector involvedObject.name={pod_name} -n {namespace}",
         "extract": ["FailedScheduling", "InsufficientCPU", "InsufficientMemory"]},
        {"step": "check_nodes", "tool": "kubectl_get",
         "args_template": "nodes -o wide",
         "extract": ["Ready", "SchedulingDisabled", "allocatable"]},
        {"step": "check_node_resources", "tool": "kubectl_top",
         "args_template": "nodes",
         "extract": ["CPU%", "Memory%"]},
        {"step": "check_pvc", "tool": "kubectl_get",
         "args_template": "pvc -n {namespace}",
         "condition": "volume_mount_exists",
         "extract": ["Bound", "Pending"]},
    ]

    def run(self, pod_name: str, namespace: str) -> dict:
        """执行 Pod Pending 诊断"""
        context = {"pod_name": pod_name, "namespace": namespace}
        findings = []

        for sop_step in self.DIAGNOSIS_SOP:
            # 条件检查（某些步骤仅在特定条件下执行）
            if sop_step.get("condition"):
                if not self._check_condition(sop_step["condition"], findings):
                    continue

            # 执行工具调用
            args = sop_step["args_template"].format(**context)
            result = self.tools.execute(sop_step["tool"], args)

            # 提取关键信息
            extracted = self._extract_signals(result, sop_step["extract"])
            findings.append({
                "step": sop_step["step"],
                "result": result,
                "signals": extracted,
            })

            # 快速路径：如果已经找到明确根因，提前终止
            root_cause = self._check_root_cause(findings)
            if root_cause:
                return {
                    "status": "diagnosed",
                    "root_cause": root_cause,
                    "findings": findings,
                    "steps_taken": len(findings),
                }

        # 所有 SOP 步骤执行完毕，综合分析
        return self._synthesize_diagnosis(findings)
```

## 6.2 问题处置执行引擎

```python
class IncidentExecutionEngine:
    """问题处置专用执行引擎

    特点：
    1. 三阶段执行（诊断→决策→修复）
    2. 每个阶段有独立的超时和约束
    3. 修复阶段强制人工审批
    4. 全程轨迹记录和回滚支持
    """

    def __init__(self, llm, tools, approval_handler):
        self.llm = llm
        self.tools = tools
        self.approval = approval_handler

        self.phase_limits = {
            "diagnose": {"max_steps": 10, "timeout": 120, "read_only": True},
            "decide": {"max_steps": 3, "timeout": 60, "read_only": True},
            "remediate": {"max_steps": 5, "timeout": 180, "read_only": False},
        }

    async def handle_incident(self, incident: dict) -> dict:
        """处置问题"""
        trajectory = ExecutionTrajectory(
            task_id=incident["id"],
            task=incident["description"],
            start_time=datetime.utcnow().isoformat(),
        )

        # Phase 1: 诊断
        diagnosis = await self._phase_diagnose(incident, trajectory)
        if diagnosis["confidence"] < 0.7:
            return {"status": "escalate", "reason": "诊断置信度不足",
                    "diagnosis": diagnosis, "trajectory": trajectory}

        # Phase 2: 决策
        action_plan = await self._phase_decide(diagnosis, trajectory)

        # Phase 3: 修复（需要审批）
        approved = await self.approval.request(
            action_plan,
            context={"incident": incident, "diagnosis": diagnosis},
        )
        if not approved:
            return {"status": "pending_approval", "action_plan": action_plan,
                    "trajectory": trajectory}

        result = await self._phase_remediate(action_plan, trajectory)

        trajectory.end_time = datetime.utcnow().isoformat()
        trajectory.status = result.get("status", "unknown")
        return {"status": result["status"], "trajectory": trajectory}
```

---

<!-- chunk: 7. 最佳实践 -->## 7. 最佳实践

## 7.1 Loop 层设计核心原则

| 原则 | 说明 | 实践建议 |
|------|------|---------|
| **有限执行** | 所有循环必须有明确终止条件 | 设置 max_iterations + timeout 双重保护 |
| **可观测** | 每一步都必须被记录 | 使用 TrajectoryEntry 记录完整上下文 |
| **可恢复** | 异常中断后能从检查点恢复 | 定期保存 checkpoint |
| **反漂移** | 主动检测并打断死循环 | 部署多维度漂移检测器 |
| **分阶段** | 复杂任务分阶段执行 | 信息收集→分析→行动三阶段 |
| **快速路径** | 已知场景提前终止 | SOP 匹配时跳过探索阶段 |

## 7.2 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| **无限循环** | 没有终止条件，资源耗尽 | 超时 + 迭代上限 + 成本预算 |
| **无轨迹记录** | 出问题无法审计回溯 | 每步记录完整的 TrajectoryEntry |
| **忽略漂移** | Agent 陷入死循环消耗资源 | 部署漂移检测 + 恢复策略 |
| **同步阻塞** | 串行工具调用效率低 | 识别可并行工具，异步执行 |
| **硬编码流程** | 无法适应不同场景 | 使用策略模式，运行时选择执行策略 |

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | Harness 六层架构总览，Loop 层基本定义 |
| [32 - Harness 工具工程](./32-agent-harness-tool-engineering.md) | Loop 层驱动的工具调用设计 |
| [33 - 上下文与记忆工程](./33-agent-harness-context-memory.md) | Loop 中的上下文管理和持久化 |
| [34 - 验证与质量门禁](./34-agent-harness-verification-quality.md) | Loop 结束后的验证层 |
| [01 - AI Agent 基础](./01-ai-agent-fundamentals.md) | Agent Loop、ReAct 推理模式的理论基础 |

---

<!-- chunk: 参考来源 -->## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| Anthropic | 《Building Effective Agents》Loop 设计模式 | 2025-12 |
| LangChain | Agent Loop 反漂移检测实验 | 2026-02 |
| Sean Goedecke (GitHub) | Copilot Agent Mode 执行引擎设计 | 2025 |
| Microsoft Research | Agent 执行轨迹分析与优化 | 2026-01 |

---

*本文档为 kudig-database 项目 02-ai-agents 系列原创内容，深入展开 Agent Harness Loop 层设计。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 02-ai-agents KUDIG Database — Global MOC
- [[domain-14-ai-ml-infra/02-ai-agents/README.md|AI Agent 工程专题]]
- [[domain-14-ai-ml-infra/02-ai-agents/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/02-ai-agents/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/02-ai-agents/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/02-ai-agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/02-ai-agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/02-ai-agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/02-ai-agents/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/02-ai-agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/02-ai-agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/02-ai-agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 29-agentscope-studio-skill-demo
- 30-agent-harness-engineering
- 32-agent-harness-tool-engineering
- 33-agent-harness-context-memory


<!-- risk-assessed -->
