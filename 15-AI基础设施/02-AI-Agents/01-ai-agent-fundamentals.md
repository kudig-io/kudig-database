---
title: AI Agent 基础与核心架构 (AI基础设施)
description: 'title: AI Agent 基础与核心架构'
summary: 'title: AI Agent 基础与核心架构'
category: general
tags:
- ai
- ai-agent
- coredns
- hpa
- statefulset
- daemonset
- rbac
- networkpolicy
- operator
- llm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- AI Agent 基础与核心架构 是什么
- 如何 AI Agent 基础与核心架构
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- AI
- Agent
- 基础与核心架构
- ai
- ml
- infra
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: AI Agent 基础与核心架构
description: '# AI Agent 基础与核心架构'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[CoreDNS|coredns]]
- hpa
- [[StatefulSet|statefulset]]
- [[DaemonSet|daemonset]]
- rbac
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- AI Agent 基础与核心架构 是什么
- 如何 AI Agent 基础与核心架构
trigger_keywords:
- AI
- Agent
- 基础与核心架构
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

# AI Agent 基础与核心架构

> **文档类型**: 基础概念专题 | **最后更新**: 2026-03 | **关键词**: AI Agent, ReAct, CoT, ToT, Agent Loop, 自主决策, 工具调用, 多步推理, Agentic AI

---

<!-- chunk: 概述 -->## 概述

AI Agent 是一类能够**自主感知环境、多步推理决策、调用外部工具并持续执行直到目标达成**的 AI 系统，是大语言模型（LLM）从"对话助手"升级为"自主执行者"的关键范式转变。

本文系统梳理 Agent 的核心定义、分类体系、主流推理框架（ReAct/CoT/ToT/Plan-and-Execute）、Agent Loop 的完整解析，以及工程落地中的关键设计决策，为构建生产级 Agent 系统奠定理论基础。

---

<!-- chunk: 1. 什么是 AI Agent -->## 1. 什么是 AI Agent

## 1.1 核心定义

Agent 与普通 LLM 应用的本质区别：

```
普通 LLM 应用:
  用户输入 → LLM 生成 → 输出结果
  特点: 单轮/多轮对话，被动响应，无工具，无状态持久化

AI Agent:
  目标设定 → 自主规划 → 调用工具 → 观察结果 → 重新规划 → ... → 目标达成
  特点: 主动行动，多步执行，工具集成，状态追踪，自我纠错
```

**四大核心能力**：

| 能力 | 说明 | 技术实现 |
|------|------|---------|
| **感知（Perceive）** | 接收来自环境的输入（文本、代码、API 响应、文件内容） | 多模态输入、工具输出解析 |
| **规划（Plan）** | 将复杂目标分解为可执行的子任务序列 | CoT、ToT、Plan-and-Execute |
| **行动（Act）** | 调用工具或 API 执行具体操作 | Function Calling、代码执行 |
| **学习（Learn）** | 从执行结果中获取反馈，调整后续决策 | Reflexion、RLHF、经验记忆 |

## 1.2 Agent 分类体系

```
AI Agent 分类
│
├── 按自主程度
│   ├── 辅助型 Agent    - 生成建议，人工确认后执行
│   ├── 半自动型 Agent  - 只读操作自动执行，写操作需审批
│   └── 全自动型 Agent  - 在安全边界内完全自主执行
│
├── 按架构模式
│   ├── 单 Agent        - 单一 LLM 驱动，顺序执行工具链
│   ├── 多 Agent        - 多个专业 Agent 协同，分工合作
│   └── 分层 Agent      - Orchestrator + Worker 的层级结构
│
├── 按应用场景
│   ├── 任务型 Agent    - 完成特定任务（代码生成、数据分析）
│   ├── 对话型 Agent    - 长期陪伴、客服、教学
│   ├── 自动化 Agent    - 运维自动化、RPA、DevOps
│   └── 研究型 Agent    - 信息搜集、文献分析、报告生成
│
└── 按工具能力
    ├── 无工具 Agent    - 纯语言推理
    ├── 检索增强 Agent  - RAG + 知识库
    ├── 代码执行 Agent  - Python/Bash 执行环境
    └── 完整工具链 Agent - API、DB、文件系统、浏览器等
```

---

<!-- chunk: 2. Agent Loop：执行引擎解析 -->## 2. Agent Loop：执行引擎解析

Agent 的核心运行机制是一个**感知-规划-行动-观察**的闭环：

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────┐
│                        Agent Loop                            │
│                                                              │
│   ┌──────────┐                                               │
│   │  目标/任务 │ ◄── 用户输入 / 触发事件                        │
│   └────┬─────┘                                               │
│        │                                                     │
│        ▼                                                     │
│   ┌──────────┐    ┌──────────────────────────────────────┐   │
│   │  规划/推理 │───►│  工具选择 & 参数生成                   │   │
│   │  (LLM)   │    │  (Function Calling / Tool Use)       │   │
│   └────┬─────┘    └─────────────┬────────────────────────┘   │
│        │                        │                            │
│        │                        ▼                            │
│        │          ┌──────────────────────────────────────┐   │
│        │          │  工具执行层                            │   │
│        │          │  kubectl / API / SQL / 搜索 / 代码    │   │
│        │          └─────────────┬────────────────────────┘   │
│        │                        │                            │
│        │                        ▼                            │
│   ┌────┴─────┐    ┌──────────────────────────────────────┐   │
│   │ 是否完成? │◄───│  观察结果 / 错误信息 / 新状态           │   │
│   └────┬─────┘    └──────────────────────────────────────┘   │
│     是 │                                                     │
│        ▼                                                     │
│   ┌──────────┐                                               │
│   │  输出结果  │                                               │
│   └──────────┘                                               │
└─────────────────────────────────────────────────────────────┘
```
## Agent Loop 的关键工程细节

**1. 终止条件设计**（避免无限循环）：
```python
class AgentLoop:
    def __init__(self, max_iterations=10, timeout_seconds=120):
        self.max_iterations = max_iterations
        self.timeout = timeout_seconds
    
    def should_stop(self, state: AgentState) -> tuple[bool, str]:
        """多重终止条件检查"""
        # 条件1: 任务完成
        if state.is_task_complete:
            return True, "task_complete"
        
        # 条件2: 达到最大迭代次数
        if state.iteration_count >= self.max_iterations:
            return True, "max_iterations_reached"
        
        # 条件3: 超时
        if state.elapsed_seconds >= self.timeout:
            return True, "timeout"
        
        # 条件4: 连续失败次数过多
        if state.consecutive_failures >= 3:
            return True, "repeated_failures"
        
        return False, ""
```

**2. 状态追踪**：
```python
from dataclasses import dataclass, field
from typing import Any

@dataclass
class AgentState:
    goal: str
    iteration_count: int = 0
    messages: list[dict] = field(default_factory=list)  # 对话历史
    tool_calls: list[dict] = field(default_factory=list)  # 工具调用记录
    observations: list[str] = field(default_factory=list)  # 观察结果
    working_memory: dict[str, Any] = field(default_factory=dict)  # 临时工作记忆
    is_task_complete: bool = False
    final_answer: str = ""
    consecutive_failures: int = 0
    elapsed_seconds: float = 0.0
```

---

<!-- chunk: 3. 主流推理框架 -->## 3. 主流推理框架

## 3.1 ReAct（Reasoning + Acting）

**最广泛使用的 Agent 推理模式**，交替进行思考（Thought）和行动（Action）：

```
ReAct 循环:
  Thought: 分析当前情况，决定下一步行动
  Action: 调用工具/执行操作
  Observation: 获取工具返回结果
  ... (循环直到任务完成)
  Final Answer: 给出最终答案
```

**示例 — K8s 故障诊断 Agent**：

```
Goal: 诊断 namespace production 中 Pod nginx-deploy-xxx 的 Pending 问题

Thought: 需要先查看 Pod 的详细状态和事件
Action: kubectl_describe(resource="pod", name="nginx-deploy-xxx", namespace="production")
Observation: 
  Events:
    Warning  FailedScheduling  0/3 nodes available: 
    1 Insufficient memory, 2 node(s) had untolerated taint {node-role.kubernetes.io/spot: "true"}

Thought: 问题在于内存不足 + Node Taint 不匹配。需要检查 Pod 的资源请求和容忍度配置
Action: kubectl_get(resource="pod", name="nginx-deploy-xxx", output="yaml", namespace="production")
Observation: 
  resources:
    requests:
      memory: "4Gi"
      cpu: "2"
  tolerations: []  # 没有配置 Spot 节点容忍度

Thought: 确认两个问题：(1) 内存请求过大 4Gi，(2) 缺少 Spot 节点容忍度。
         建议降低内存请求至 2Gi，或添加对 Spot 节点的容忍度（如业务允许）

Final Answer: 
  根因: Pod 因内存请求(4Gi)超出可用节点容量，且无法调度到 Spot 节点(缺少 toleration)
  修复方案1(推荐): 降低 memory request 至 2Gi
  修复方案2: 添加 toleration: {key: "node-role.kubernetes.io/spot", operator: "Exists"}
  修复方案3: 扩容节点池
```

**ReAct 实现**：

```python
from langchain.agents import create_react_agent
from langchain_openai import ChatOpenAI
from langchain.tools import Tool

llm = ChatOpenAI(model="gpt-4o", temperature=0)

tools = [
    Tool(name="kubectl_describe", func=kubectl_describe, 
         description="获取 K8s 资源的详细信息和事件。输入: resource_type/name/namespace"),
    Tool(name="kubectl_get", func=kubectl_get,
         description="获取 K8s 资源的 YAML/JSON 配置。输入: resource_type/name/output_format/namespace"),
    Tool(name="kubectl_logs", func=kubectl_logs,
         description="获取 Pod 容器日志。输入: pod_name/namespace/container/lines"),
]

agent = create_react_agent(llm=llm, tools=tools, prompt=react_prompt)
```

## 3.2 Chain-of-Thought（CoT）

适用于**复杂推理任务**，通过逐步分解思维链来提升准确性：

```python
# Zero-shot CoT
system_prompt = """你是一个 Kubernetes 专家。
解决问题时，请按以下步骤思考：
1. 理解问题的本质和范围
2. 列出可能的原因（从最常见到最罕见）
3. 确定诊断所需的信息
4. 提出诊断步骤
5. 给出解决方案（按优先级排序）

在每一步明确说明你的推理过程。"""

# Few-shot CoT（通过示例引导）
example = """
问题: 为什么 HPA 不能扩容 Pod？
思考步骤:
  1. HPA 扩容需要三个条件: metrics-server 运行 + 目标 CPU/Memory 利用率超阈值 + maxReplicas 未达上限
  2. 先检查 metrics-server 是否正常: kubectl top nodes
  3. 检查 HPA 状态: kubectl describe hpa <name>
  4. 查看 HPA 事件是否有错误消息
  常见原因: metrics-server 未安装/不健康、Deployment 已达 maxReplicas、
            资源利用率未超阈值、HPA 处于 stabilization window 冷却期
"""
```

## 3.3 Tree-of-Thought（ToT）

适用于**存在多个解决路径的复杂问题**，并行探索多个思维分支：

```
                    [问题: 集群网络不通]
                          │
         ┌────────────────┼────────────────┐
         ▼                ▼                ▼
    [方向A: CNI]    [方向B: NetworkPolicy]  [方向C: DNS]
    检查 CNI 插件    检查 NP 规则             检查 CoreDNS
         │                │                │
    [评分: 0.7]      [评分: 0.9]        [评分: 0.3]
         │                │
    [剪枝放弃]      [继续探索: 深度]
                          │
                   ┌──────┴──────┐
                   ▼             ▼
           [NP 规则冲突]    [NP 方向错误]
           [评分: 0.95]    [评分: 0.4]
                   │
              [最终答案]
```

```python
# ToT 实现思路（简化版）
class TreeOfThought:
    def __init__(self, llm, num_branches=3, max_depth=4, beam_width=2):
        self.llm = llm
        self.num_branches = num_branches
        self.max_depth = max_depth
        self.beam_width = beam_width  # 每层保留最优的 N 个分支
    
    def solve(self, problem: str) -> str:
        # 1. 生成初始思维分支
        branches = self._generate_branches(problem, num=self.num_branches)
        
        for depth in range(self.max_depth):
            # 2. 评估每个分支的有效性
            scored = [(b, self._evaluate(problem, b)) for b in branches]
            
            # 3. Beam Search: 保留最优分支
            top_branches = sorted(scored, key=lambda x: x[1], reverse=True)[:self.beam_width]
            
            # 4. 检查是否有终态
            for branch, score in top_branches:
                if self._is_final(branch):
                    return branch.conclusion
            
            # 5. 扩展最优分支
            branches = []
            for branch, _ in top_branches:
                branches.extend(self._expand(branch))
        
        return top_branches[0][0].conclusion
```

## 3.4 Plan-and-Execute

适用于**长流程、多步骤任务**，先制定完整计划再逐步执行：

```python
# Plan-and-Execute 架构
class PlanAndExecuteAgent:
    def __init__(self, planner_llm, executor_llm, replanner_llm):
        self.planner = planner_llm    # 强模型：负责制定计划（如 GPT-4o）
        self.executor = executor_llm  # 可较弱：负责执行单步（如 GPT-4o-mini）
        self.replanner = replanner_llm  # 负责根据执行结果重新规划
    
    def run(self, goal: str) -> str:
        # Step 1: 制定完整执行计划
        plan = self.planner.create_plan(goal)
        # plan = ["检查集群节点状态", "查看 Pending Pod 列表", ...]
        
        results = []
        for i, step in enumerate(plan):
            # Step 2: 执行单步任务
            result = self.executor.execute(step, context=results)
            results.append({"step": step, "result": result})
            
            # Step 3: 重新规划（根据执行结果调整后续步骤）
            if self._need_replan(result):
                remaining = self.replanner.replan(
                    goal=goal,
                    completed=results,
                    remaining_steps=plan[i+1:]
                )
                plan[i+1:] = remaining
        
        return self._synthesize(goal, results)
```

## 3.5 Reflexion（反思机制）

通过**自我反思**从失败中学习，迭代改进：

```python
class ReflexionAgent:
    def __init__(self, llm, max_attempts=3):
        self.llm = llm
        self.max_attempts = max_attempts
        self.reflections = []  # 长期记忆中存储的反思内容
    
    def run(self, task: str) -> str:
        for attempt in range(self.max_attempts):
            # 执行任务（附加历史反思作为上下文）
            result = self._execute(task, reflections=self.reflections)
            
            # 自我评估
            evaluation = self._evaluate(task, result)
            
            if evaluation.is_success:
                return result
            
            # 生成反思（分析失败原因）
            reflection = self._reflect(
                task=task,
                result=result,
                failure_reason=evaluation.failure_reason
            )
            self.reflections.append(reflection)
            # 下次尝试时，reflection 作为上下文避免重复错误
        
        return self._best_attempt()
```

---

<!-- chunk: 4. 推理框架选型指南 -->## 4. 推理框架选型指南

| 场景 | 推荐框架 | 原因 |
|------|---------|------|
| 需要调用工具完成任务 | **ReAct** | 工具调用与推理交织，是最通用的框架 |
| 数学/逻辑推理 | **CoT** | 逐步分解复杂推理，显著提升准确性 |
| 多路径探索（如规划）| **ToT** | 并行探索多方案，适合开放性问题 |
| 长流程任务（>5步） | **Plan-and-Execute** | 减少 LLM 在执行中的分心，提升效率 |
| 反复失败的任务 | **Reflexion** | 从错误中学习，避免重复犯错 |
| 复杂多 Agent 场景 | **组合使用** | Orchestrator 用 Plan-and-Execute + Worker 用 ReAct |

---

<!-- chunk: 5. Agent 的关键工程挑战 -->## 5. Agent 的关键工程挑战

## 5.1 幻觉（Hallucination）控制

```python
# 最佳实践: 强制 Agent 引用来源
system_prompt = """
你是一个 K8s 运维专家 Agent。规则：
1. 所有诊断结论必须基于你通过工具获取的实际数据
2. 如果工具返回的信息不足以得出结论，必须明确说明需要更多信息
3. 禁止凭记忆猜测集群当前状态——必须通过工具实时获取
4. 给出建议时，必须同时说明风险和回滚方法
"""
```

## 5.2 工具调用失败处理

```python
import time
from typing import Callable

def resilient_tool_call(
    tool_fn: Callable,
    args: dict,
    max_retries: int = 3,
    backoff_seconds: float = 2.0
) -> tuple[bool, str]:
    """带指数退避的工具调用重试"""
    for attempt in range(max_retries):
        try:
            result = tool_fn(**args)
            return True, result
        except TimeoutError as e:
            if attempt < max_retries - 1:
                wait = backoff_seconds * (2 ** attempt)
                time.sleep(wait)
        except PermissionError as e:
            # 权限错误不需要重试
            return False, f"权限不足: {e}"
        except Exception as e:
            if attempt == max_retries - 1:
                return False, f"工具调用失败（已重试 {max_retries} 次）: {e}"
            time.sleep(backoff_seconds)
    
    return False, "达到最大重试次数"
```

## 5.3 上下文窗口管理

```python
class ContextWindowManager:
    """动态管理 Agent 上下文，防止超出 Token 限制"""
    
    def __init__(self, max_tokens: int = 100_000, reserve_ratio: float = 0.3):
        self.max_tokens = max_tokens
        self.reserve_tokens = int(max_tokens * reserve_ratio)  # 为输出预留
    
    def trim_messages(self, messages: list[dict]) -> list[dict]:
        """保留系统提示 + 最近 N 条消息"""
        available = self.max_tokens - self.reserve_tokens
        
        # 始终保留系统消息
        system_msgs = [m for m in messages if m["role"] == "system"]
        other_msgs = [m for m in messages if m["role"] != "system"]
        
        system_tokens = self._count_tokens(system_msgs)
        available -= system_tokens
        
        # 从最近的消息开始保留
        kept = []
        token_count = 0
        for msg in reversed(other_msgs):
            tokens = self._count_tokens([msg])
            if token_count + tokens <= available:
                kept.insert(0, msg)
                token_count += tokens
            else:
                break
        
        return system_msgs + kept
    
    def summarize_old_messages(self, messages: list[dict], llm) -> str:
        """对过旧的消息进行摘要压缩"""
        prompt = f"请将以下对话历史压缩为 200 字以内的摘要，保留关键信息：\n{messages}"
        return llm.invoke(prompt)
```

---

<!-- chunk: 6. 生产级 Agent 设计原则 -->## 6. 生产级 Agent 设计原则

## 6.1 最小权限原则

```yaml
# Agent 的 K8s RBAC 配置示例
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: agent-readonly-role
rules:
  # 只读权限：诊断 Agent 不需要写权限
  - apiGroups: [""]
    resources: ["pods", "services", "endpoints", "events", "nodes"]
    verbs: ["get", "list", "watch"]
  - apiGroups: ["apps"]
    resources: ["deployments", "replicasets", "statefulsets", "daemonsets"]
    verbs: ["get", "list", "watch"]
  # 日志读取权限
  - apiGroups: [""]
    resources: ["pods/log"]
    verbs: ["get"]
  # 严格禁止: 无 create/update/delete/exec 权限
```

## 6.2 可审计性

每次 Agent 操作都必须完整记录：

```python
import structlog
from datetime import datetime

logger = structlog.get_logger()

class AuditedAgent:
    def execute_tool(self, tool_name: str, args: dict, user_id: str) -> str:
        trace_id = self._generate_trace_id()
        
        # 记录工具调用开始
        logger.info("agent_tool_call_start",
            trace_id=trace_id,
            tool=tool_name,
            args=args,
            user_id=user_id,
            timestamp=datetime.utcnow().isoformat()
        )
        
        start_time = time.time()
        try:
            result = self.tools[tool_name](**args)
            latency_ms = (time.time() - start_time) * 1000
            
            logger.info("agent_tool_call_success",
                trace_id=trace_id,
                tool=tool_name,
                latency_ms=round(latency_ms, 2),
                result_length=len(str(result))
            )
            return result
        except Exception as e:
            logger.error("agent_tool_call_error",
                trace_id=trace_id,
                tool=tool_name,
                error=str(e),
                error_type=type(e).__name__
            )
            raise
```

## 6.3 人工审批门禁（Human-in-the-Loop）

```python
from enum import Enum

class RiskLevel(Enum):
    LOW = "low"       # 只读操作，自动执行
    MEDIUM = "medium" # 影响范围有限，异步通知后执行
    HIGH = "high"     # 影响生产，需人工审批
    CRITICAL = "critical"  # 不可逆操作，需双人审批

TOOL_RISK_LEVELS = {
    "kubectl_get": RiskLevel.LOW,
    "kubectl_describe": RiskLevel.LOW,
    "kubectl_logs": RiskLevel.LOW,
    "kubectl_scale": RiskLevel.MEDIUM,
    "kubectl_rollout_restart": RiskLevel.HIGH,
    "kubectl_delete": RiskLevel.CRITICAL,
    "kubectl_apply": RiskLevel.HIGH,
}

class HumanInLoopGate:
    def check_approval(self, tool_name: str, args: dict) -> bool:
        risk = TOOL_RISK_LEVELS.get(tool_name, RiskLevel.HIGH)
        
        if risk == RiskLevel.LOW:
            return True  # 自动通过
        
        if risk in [RiskLevel.HIGH, RiskLevel.CRITICAL]:
            # 发送审批请求（Slack、飞书、企业微信）
            approval_request = self._create_approval_request(tool_name, args, risk)
            return self._wait_for_approval(approval_request, timeout=300)
        
        return False
```

---

<!-- chunk: 7. Agent vs 传统自动化对比 -->## 7. Agent vs 传统自动化对比

| 维度 | 传统脚本/自动化 | AI Agent |
|------|---------------|---------|
| **适用场景** | 确定性、重复性任务 | 需要推理判断的复杂任务 |
| **异常处理** | 预定义错误处理 | 自适应应对未知情况 |
| **可维护性** | 逻辑变化需改代码 | 修改提示词即可调整行为 |
| **可解释性** | 执行步骤固定可预期 | 每次推理过程可追踪但有随机性 |
| **成本** | 低（无 LLM 调用） | 高（LLM Token 消耗） |
| **准确性** | 高（规则明确时） | 存在幻觉风险，需验证 |
| **扩展性** | 新场景需重新开发 | 描述新场景即可快速扩展 |

**决策原则**：能用脚本解决的问题，不要用 Agent；当任务具备**模糊性、多样性、需要自然语言理解**时，Agent 才能发挥优势。

---

<!-- chunk: 8. 最佳实践与反模式 -->## 8. 最佳实践与反模式

## 最佳实践

- **明确任务边界**：系统提示中清晰定义 Agent 的职责范围和禁止行为
- **工具描述精准**：工具的 description 直接影响 LLM 的选择质量，要具体说明输入输出和适用场景
- **渐进式授权**：先从只读工具开始，验证 Agent 行为可靠后再逐步开放写权限
- **失败快速**：设置合理的超时和重试上限，避免无效循环消耗资源
- **结果验证**：Agent 的每次操作结果应有验证步骤，而非盲目继续

## 反模式

- **工具过载**：给 Agent 超过 20 个工具会导致工具选择准确率显著下降
- **无终止条件**：没有 max_iterations 限制，Agent 可能陷入无限循环
- **权限过大**：Agent 拥有完整的集群管理员权限，一旦推理错误后果灾难性
- **忽略错误**：工具调用失败后不处理直接继续，导致基于错误结果的后续决策
- **上下文无限增长**：不管理上下文窗口，随着会话变长推理质量下降

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [02 - LLM 模型选型](./02-llm-foundation-models.md) | 为 Agent 选择合适的基座模型 |
| [03 - Agent 框架对比](./03-agent-frameworks-comparison.md) | LangChain/AutoGen 等框架实现 Agent Loop |
| [05 - Tool Use & Function Calling](./05-tool-use-function-calling.md) | Agent 工具调用的详细规范 |
| [06 - 多 Agent 编排](./06-multi-agent-orchestration.md) | 单 Agent 到多 Agent 架构升级 |
| [14 - Agent 赋能设计与落地路径](./14-agent-kudig-design-strategy.md) | K8s 运维 Agent 的顶层设计 |
| [AI基础设施/17-llm-inference-serving.md](../01-%E5%9F%BA%E7%A1%80%E8%AE%BE%E6%96%BD/17-llm-inference-serving.md) | LLM 推理服务部署 |

---

*本文档为 kudig-database 项目 02-ai-agents 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 02-ai-agents KUDIG Database — Global MOC
- [[15-AI基础设施/02-AI-Agents/README.md|AI Agent 工程专题]]
- [[15-AI基础设施/02-AI-Agents/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[15-AI基础设施/02-AI-Agents/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[15-AI基础设施/02-AI-Agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[15-AI基础设施/02-AI-Agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[15-AI基础设施/02-AI-Agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[15-AI基础设施/02-AI-Agents/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[15-AI基础设施/02-AI-Agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[15-AI基础设施/02-AI-Agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[15-AI基础设施/02-AI-Agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]
- [[15-AI基础设施/02-AI-Agents/11-cost-latency-optimization.md|成本与延迟优化策略]]

## See Also

- 49-openclaw-memory-mechanism
- 50-openclaw-identity-mechanism
- 02-llm-foundation-models
- 03-agent-frameworks-comparison


<!-- risk-assessed -->
