---
title: ReAct Agent 与 Harness 识别指南
description: 'title: ReAct Agent 与 Harness 识别指南'
category: general
tags:
- ai
- ai-agent
- guide
- prometheus
- postgresql
- llm
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- ReAct Agent 与 Harness 识别指南 是什么
- 如何 ReAct Agent 与 Harness 识别指南
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- ReAct
- Agent
- Harness
- 识别指南
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- prometheus-basics
---

title: ReAct Agent 与 Harness 识别指南
description: '# ReAct Agent 与 Harness 识别指南'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- prometheus
- postgresql
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- ReAct Agent 与 Harness 识别指南 是什么
- 如何 ReAct Agent 与 Harness 识别指南
trigger_keywords:
- ReAct
- Agent
- Harness
- 识别指南
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

# ReAct Agent 与 Harness 识别指南

> **文档类型**: 实践参考指南 | **最后更新**: 2026-04 | **关键词**: ReAct, Agent Harness, 识别判断, 推理框架, 成熟度模型, 分类标准, Agent Loop, Verification, Constraints

---

<!-- chunk: 概述 -->## 概述

在构建和评估 Agent 系统时，两个最基础的问题是：**"这个 Agent 是不是 ReAct？"** 和 **"这个 Agent 有没有 Harness？"**。前者决定了 Agent 的推理模式和能力边界，后者决定了 Agent 是否具备生产级的可靠性和可控性。

本文提供系统化的判断标准、清单和代码级的识别方法，帮助工程师快速评估 Agent 的推理模式和 Harness 完整度。

---

<!-- chunk: 1. 如何判断一个 Agent 是否是 ReAct -->## 1. 如何判断一个 Agent 是否是 ReAct

#<!-- chunk: 1.1 ReAct 核心定义 -->## 1.1 ReAct 核心定义

**ReAct = Reasoning + Acting**，是最广泛使用的 Agent 推理模式。其核心特征是**交替进行思考（Thought）和行动（Action）**：

```
ReAct 循环:
  Thought: 分析当前情况，决定下一步行动
  Action: 调用工具/执行操作
  Observation: 获取工具返回结果
  ... (循环直到任务完成)
  Final Answer: 给出最终答案
```

#<!-- chunk: 1.2 三要素判断法 -->## 1.2 三要素判断法

ReAct 的三要素必须**同时具备**：

| 特征 | 说明 | 判断方法 |
|------|------|---------|
| **Thought（思考）** | 每步都有显式的推理/分析过程 | 看输出是否包含 `Thought:` 或推理步骤 |
| **Action（行动）** | 基于推理调用工具/执行操作 | 看是否有 `Action:` + 工具调用 |
| **Observation（观察）** | 获取工具执行结果并反馈到下一轮 | 看是否有 `Observation:` + 结果解析 |

#<!-- chunk: 1.3 完整判断清单 -->## 1.3 完整判断清单

```
✅ 是 ReAct Agent 的标志:
  1. 有 Agent Loop（循环执行，非一次性请求-响应）
  2. 交替进行 Thought → Action → Observation 循环
  3. 有工具调用能力（Function Calling / Tool Use）
  4. LLM 自主决定何时调用工具、调用哪个工具
  5. LLM 自主决定何时结束（输出 Final Answer）
  6. 每步推理可追踪（有 trajectory）

❌ 不是 ReAct 的情况:
  - 纯 CoT: 只有思维链推理，没有工具调用
  - 纯 Plan-and-Execute: 先一次性生成完整计划再执行，而非交替推理行动
  - 纯对话: 没有 Agent Loop，只是多轮对话
  - 预定义流程: 执行路径固定，LLM 不做动态决策
```

#<!-- chunk: 1.4 代码层面判断 -->## 1.4 代码层面判断

##<!-- chunk: AgentScope 框架 -->## AgentScope 框架

直接看是否使用了 `ReActAgent` 或继承自 `ReActAgentBase`：

```python
# ✅ 这就是 ReAct Agent
from agentscope.agent import ReActAgent
agent = ReActAgent(name="Friday", model=model, toolkit=toolkit, ...)

# AgentScope 智能体继承体系:
# AgentBase          → 通用基类，不一定是 ReAct
# ReActAgentBase     → 有 _reasoning() + _acting()，是 ReAct
# ReActAgent         → 开箱即用的 ReAct 实现，是 ReAct
# UserAgent          → 用户代理，不是 ReAct
```

##<!-- chunk: LangChain 框架 -->## LangChain 框架

```python
# ✅ ReAct
from langchain.agents import create_react_agent
agent = create_react_agent(llm=llm, tools=tools, prompt=react_prompt)

# ❌ 不是 ReAct（是 Plan-and-Execute）
from langgraph.prebuilt import create_plan_and_execute_agent
```

##<!-- chunk: 自定义 Agent -->## 自定义 Agent

检查是否具备以下结构：

```python
# ReAct 的最小实现骨架
class ReActAgent:
    def run(self, task: str) -> str:
        while not done:
            # 1. Thought: LLM 推理
            thought = self.llm.reason(context)

            # 2. 判断是否结束
            if thought.is_final_answer:
                return thought.answer

            # 3. Action: 调用工具
            result = self.execute_tool(thought.action)

            # 4. Observation: 将结果加入上下文
            context.add_observation(result)
```

#<!-- chunk: 1.5 ReAct 与其他推理框架对比 -->## 1.5 ReAct 与其他推理框架对比

| 框架 | 有 Loop? | 有工具调用? | 推理方式 | 是否 ReAct |
|------|----------|-----------|---------|-----------|
| **ReAct** | ✅ | ✅ | 交替 Thought/Action | ✅ |
| CoT | ❌ | ❌ | 线性思维链 | ❌ |
| ToT | ❌ | ❌ | 树状分支搜索 | ❌ |
| Plan-and-Execute | ✅ | ✅ | 先计划再执行 | ❌ |
| Reflexion | ✅ | 可选 | 自我反思迭代 | ❌（但可组合） |

#<!-- chunk: 1.6 边界案例 -->## 1.6 边界案例

```
灰色地带 — 需要进一步判断:

1. Plan-then-ReAct（计划后 ReAct 执行）
   计划阶段: Plan-and-Execute
   执行阶段: ReAct
   判定: 执行层是 ReAct，整体是混合架构

2. ReAct + Reflexion（带反思的 ReAct）
   ReAct Loop 内执行任务
   失败后触发 Reflexion 反思
   判定: 核心是 ReAct，Reflexion 是增强

3. Multi-Agent 中的 ReAct Worker
   Orchestrator 用 Plan-and-Execute
   Worker Agent 各自用 ReAct
   判定: Worker 是 ReAct，系统整体是混合架构
```

---

<!-- chunk: 2. 如何判断 Agent 是否有 Harness -->## 2. 如何判断 Agent 是否有 Harness

#<!-- chunk: 2.1 Harness 核心定义 -->## 2.1 Harness 核心定义

**Harness = 包裹在 AI 模型外部的完整运行系统**，将模型的原始认知能力转化为可靠的生产输出。

```
普通 LLM 使用（无 Harness）:
  用户输入 → LLM 生成 → 输出结果
  问题: 答案不稳定、幻觉、丢失上下文、无法执行、无安全边界

Agent + Harness（完整系统）:
  目标 → 循环执行 → 工具调用 → 上下文管理 → 状态持久化 → 自检验证 → 约束控制 → 可靠输出
  效果: 稳定、可靠、可审计、可控、可度量

关键比喻:
  模型是马，Harness 是马具（缰绳、鞍座、胸带、眼罩）
  马具不让马更强壮，而是让马的力量可靠地转化为有用的工作
```

#<!-- chunk: 2.2 六层架构检查法 -->## 2.2 六层架构检查法

Harness 包含六层架构。**不需要全部具备才叫 Harness**，但层级越完整，Harness 越成熟：

```
Agent Harness 六层架构（逐层检查）:

Layer 1: Loop（循环层）
  □ 有 Agent Loop（持续运行直到目标达成）
  □ 有超时保护
  □ 有最大迭代限制
  □ 有反漂移检测（防止死循环）
  □ 有轨迹记录（trajectory）

Layer 2: Tools（工具层）
  □ 有工具注册/发现机制
  □ 有参数校验
  □ 有工具权限控制
  □ 有工具错误处理和重试

Layer 3: Context（上下文层）
  □ 有上下文窗口管理
  □ 有信息优先级排序
  □ 有 RAG 检索集成
  □ 有上下文压缩/摘要

Layer 4: Persistence（持久化层）
  □ 有跨会话记忆
  □ 有执行记录持久存储
  □ 有状态恢复能力

Layer 5: Verification（验证层）      ← 关键分水岭
  □ 有输出自检循环
  □ 有事实验证
  □ 有格式校验
  □ 有幻觉检测

Layer 6: Constraints（约束层）
  □ 有安全边界（只读/命令黑名单）
  □ 有成本控制（Token 预算）
  □ 有审计日志
  □ 有人工审批机制
```

#<!-- chunk: 2.3 五级成熟度模型判断法 -->## 2.3 五级成熟度模型判断法

| 等级 | 名称 | 特征 | 是否有 Harness |
|------|------|------|--------------|
| **L1** | 裸 Agent（Ad-hoc） | 直接调用 LLM API，无循环、无工具、无验证 | ❌ 无 Harness |
| **L2** | 基础 Harness（Managed） | 有 Agent Loop + 工具调用 + 超时保护，但无验证、无约束 | ⚠️ 最低限度 Harness |
| **L3** | 生产就绪（Production-Ready） | 六层架构完整 + CI/CD 质量门禁 + 基本监控 | ✅ 有 Harness |
| **L4** | 企业级（Enterprise） | 多 Agent 编排 + 灰度发布 + A/B 测试 + 完整可观测性 | ✅ 成熟 Harness |
| **L5** | 自进化（Self-Evolving） | Meta-Agent 自动优化 Harness 参数 | ✅ 高级 Harness |

#<!-- chunk: 2.4 快速判断清单 -->## 2.4 快速判断清单

##<!-- chunk: 最小 Harness 判断（L2 — 至少满足以下全部） -->## 最小 Harness 判断（L2 — 至少满足以下全部）

```
□ 有 Agent Loop（循环执行，非一次性调用）
□ 有工具调用（>= 2 个工具）
□ 有超时保护
□ 有最大迭代限制
```

##<!-- chunk: 生产级 Harness 判断（L3 — 在 L2 基础上增加） -->## 生产级 Harness 判断（L3 — 在 L2 基础上增加）

```
□ 有验证层（>= 3 个验证器）           ← 关键分水岭
□ 有约束层（只读 + 命令黑名单）
□ 有上下文管理（分层构建）
□ 有持久化（执行记录持久存储）
□ 有 Prometheus 指标
□ 有 CI/CD 质量门禁
□ 有基线对比
□ 有基本告警规则
```

##<!-- chunk: 企业级 Harness 判断（L4 — 在 L3 基础上增加） -->## 企业级 Harness 判断（L4 — 在 L3 基础上增加）

```
□ 有多 Agent 编排
□ 有灰度发布流程
□ 有 A/B 测试
□ 有 OTel 全链路追踪
□ 有 Langfuse 集成
□ 有红队测试
□ 有 LLM 提供商容灾
□ 有 SLA 监控
□ 有配置热更新
□ 有 Prompt 版本管理
```

#<!-- chunk: 2.5 代码级 Harness 识别 -->## 2.5 代码级 Harness 识别

##<!-- chunk: 有 Harness 的 Agent（典型结构） -->## 有 Harness 的 Agent（典型结构）

```python
class HarnessedAgent:
    def __init__(self):
        # Layer 1: Loop
        self.max_iterations = 15
        self.timeout_seconds = 300

        # Layer 2: Tools
        self.tool_registry = ToolRegistry()

        # Layer 3: Context
        self.context_manager = ContextManager(max_tokens=100000)

        # Layer 4: Persistence
        self.memory = AsyncSQLAlchemyMemory(url="postgresql://...")

        # Layer 5: Verification  ← 分水岭
        self.verifiers = [
            FaithfulnessVerifier(),
            FormatVerifier(),
            SafetyVerifier(),
        ]

        # Layer 6: Constraints
        self.constraints = ConstraintEngine(
            read_only=True,
            blocked_commands=["kubectl delete", "rm -rf"],
            max_cost_usd=2.0,
        )

    async def run(self, task: str) -> dict:
        for i in range(self.max_iterations):
            # Loop 驱动
            thought = await self.llm.reason(context)
            if thought.is_final:
                # Verification 验证
                verified = await self.verify(thought.answer)
                if verified:
                    return {"status": "success", "answer": thought.answer}
                else:
                    continue  # 自检失败，重试

            # Constraints 约束检查
            if not self.constraints.allow(thought.action):
                return {"status": "blocked", "reason": "constraint_violation"}

            # Tools 执行
            result = await self.tool_registry.execute(thought.action)

            # Context 更新
            self.context_manager.add_observation(result)

            # Persistence 记录
            await self.memory.save_step(thought, result)
```

##<!-- chunk: 无 Harness 的 Agent（裸 Agent） -->## 无 Harness 的 Agent（裸 Agent）

```python
# ❌ 裸 Agent：直接调用 LLM + 工具，无验证、无约束、无持久化
class NakedAgent:
    def run(self, task: str) -> str:
        response = self.llm.chat(task)
        if response.tool_calls:
            result = self.execute_tool(response.tool_calls[0])
            return self.llm.chat(f"工具返回: {result}")
        return response.content
```

#<!-- chunk: 2.6 关键分水岭：验证层 -->## 2.6 关键分水岭：验证层

行业实证数据表明，**验证层是 Harness 区别于"裸 Agent"的关键分水岭**：

```
验证层 ROI 实证:

LangChain 编码 Agent（2026-02 实验）:
  无验证:        基准分 52.8%
  添加自检循环:   基准分 66.5%  → +13.7% 绝对提升（最高单项改进）

Anthropic 长运行 Agent:
  无验证:        任务完成率 71%
  带验证:        任务完成率 89%  → +18% 绝对提升

验证拦截的问题类型:
  - 幻觉输出: 40%
  - 格式错误: 25%
  - 逻辑不一致: 20%
  - 安全风险: 15%
```

#<!-- chunk: 2.7 一句话总结 -->## 2.7 一句话总结

```
裸 Agent       = LLM + 工具 + Loop
有 Harness     = 裸 Agent + 验证（自检循环）+ 约束（安全边界）+ 上下文管理 + 持久化 + 可观测性

最简判断:
  ✅ 有自检验证 + 有约束控制 → 有 Harness
  ❌ 只有 LLM + 工具循环调用 → 裸 Agent
```

---

<!-- chunk: 3. 综合识别矩阵 -->## 3. 综合识别矩阵

| 系统特征 | 纯 LLM | 裸 Agent | 基础 Harness (L2) | 生产 Harness (L3+) |
|---------|--------|---------|------------------|-------------------|
| LLM 推理 | ✅ | ✅ | ✅ | ✅ |
| 工具调用 | ❌ | ✅ | ✅ | ✅ |
| Agent Loop | ❌ | ✅ | ✅ | ✅ |
| 超时/迭代限制 | ❌ | ❌/⚠️ | ✅ | ✅ |
| 反漂移检测 | ❌ | ❌ | ⚠️ 可选 | ✅ |
| 上下文管理 | ❌ | ❌ | ⚠️ 基础 | ✅ |
| 持久化 | ❌ | ❌ | ❌ | ✅ |
| **验证层** | ❌ | ❌ | ❌ | **✅ 分水岭** |
| 约束层 | ❌ | ❌ | ❌ | ✅ |
| 可观测性 | ❌ | ❌ | ❌ | ✅ |
| 轨迹记录 | ❌ | ⚠️ | ✅ | ✅ |
| 审计日志 | ❌ | ❌ | ❌ | ✅ |

---

<!-- chunk: 4. 实战应用示例 -->## 4. 实战应用示例

#<!-- chunk: 4.1 评估现有 Agent 系统 -->## 4.1 评估现有 Agent 系统

```python
class AgentSystemAssessment:
    """Agent 系统快速评估工具"""

    def assess(self, agent_system: dict) -> dict:
        """
        输入 agent_system 描述，输出评估结果

        agent_system 示例:
        {
            "has_agent_loop": True,
            "has_tools": True,
            "tool_count": 5,
            "has_timeout": True,
            "has_max_iterations": True,
            "has_drift_detection": False,
            "has_verification": False,
            "has_constraints": False,
            "has_persistence": False,
            "has_observability": False,
        }
        """
        # 1. 判断推理模式
        is_react = (
            agent_system.get("has_agent_loop", False)
            and agent_system.get("has_tools", False)
            and agent_system.get("has_thought_action_observation", True)
        )

        # 2. 判断 Harness 等级
        harness_level = self._assess_harness_level(agent_system)

        # 3. 差距分析
        gaps = self._gap_analysis(harness_level, agent_system)

        return {
            "is_react": is_react,
            "harness_level": harness_level,
            "gaps_to_next_level": gaps,
            "recommendation": self._recommend(harness_level),
        }

    def _assess_harness_level(self, s: dict) -> str:
        if not s.get("has_agent_loop"):
            return "L1"
        if not all([
            s.get("has_tools"),
            s.get("has_timeout"),
            s.get("has_max_iterations"),
        ]):
            return "L1"
        if not all([
            s.get("has_verification"),
            s.get("has_constraints"),
        ]):
            return "L2"
        if not all([
            s.get("has_observability"),
            s.get("has_persistence"),
        ]):
            return "L3"
        return "L4"

    def _recommend(self, level: str) -> str:
        recommendations = {
            "L1": "添加 Agent Loop + 工具 + 超时保护，升级到 L2",
            "L2": "添加验证层（最高 ROI 改进）+ 约束层，升级到 L3",
            "L3": "添加完整可观测性 + 灰度发布，升级到 L4",
            "L4": "探索 Meta-Agent 自优化，向 L5 演进",
        }
        return recommendations.get(level, "已达最高成熟度")
```

#<!-- chunk: 4.2 团队 Code Review 中的 Agent 分类 -->## 4.2 团队 Code Review 中的 Agent 分类

在团队 Code Review 中，可以使用以下问题快速分类：

```
Agent 分类 Code Review Checklist:

1. 推理模式识别
   Q: 执行过程中是否交替进行 Thought/Action/Observation？
   → 是 → ReAct
   → 否 → 检查是否是 Plan-and-Execute 或 CoT

2. Harness 等级识别
   Q: 有没有输出验证（self-check）？
   → 无 → 最多 L2（基础 Harness 或更低）
   → 有 → 至少 L3

   Q: 有没有安全约束（命令黑名单/只读模式）？
   → 无 → 不适合生产环境
   → 有 → 继续检查其他维度

   Q: 有没有可观测性（metrics/traces/logs）？
   → 无 → 生产运维困难
   → 有 → 具备生产级可观测性
```

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [01 - AI Agent 基础与核心架构](./01-ai-agent-fundamentals.md) | ReAct/CoT/ToT/Reflexion 推理框架理论基础 |
| [17 - AgentScope 核心概念](./[[domain-14-ai-ml-infra/17-agentscope-core-concepts.md|17-agentscope-core-concepts]].md) | AgentScope 中 ReActAgent 继承体系 |
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | Harness 六层架构总览、设计模式、行业实证 |
| [31 - Harness Loop 与执行引擎](./31-agent-harness-loop-execution.md) | Loop 层状态机、反漂移检测 |
| [34 - Harness 验证与质量门禁](./[[domain-14-ai-ml-infra/34-agent-harness-verification-quality.md|34-agent-harness-verification-quality]].md) | 验证层（分水岭）设计详解 |
| [35 - Harness 安全与约束工程](./[[domain-14-ai-ml-infra/35-agent-harness-security-constraints.md|35-agent-harness-security-constraints]].md) | 约束层四层模型 |
| [40 - Harness 生产运维与成熟度模型](./40-agent-harness-production-maturity.md) | 五级成熟度评估清单 |

---

<!-- chunk: 参考来源 -->## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| Yao et al. | 《ReAct: Synergizing Reasoning and Acting in Language Models》 | 2022-10 |
| Birgitta Böckeler (Martin Fowler) | 《Harness Engineering》概念提出 | 2026-02 |
| Anthropic | 《Building Effective Agents》、《Effective Harnesses for Long-Running Agents》 | 2025-12 |
| LangChain | Agent Loop 反漂移检测实验、自检循环 ROI 数据 | 2026-02 |
| Sean Goedecke (GitHub) | Copilot Agent Mode 执行引擎设计 | 2025 |

---

*本文档为 kudig-database 项目 topic-ai-agent 系列原创内容，提供 ReAct Agent 与 Harness 的系统化识别方法。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-14-ai-ml-infra/topic-ai-agent/MOC.md|topic-ai-agent MOC]]
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

- [[domain-14-ai-ml-infra/39-agent-harness-testing-benchmark.md|39-agent-harness-testing-benchmark]]
- [[domain-14-ai-ml-infra/40-agent-harness-production-maturity.md|40-agent-harness-production-maturity]]
- [[domain-14-ai-ml-infra/42-model-harness-compatibility-matrix.md|42-model-harness-compatibility-matrix]]
- [[domain-14-ai-ml-infra/43-openclaw-framework-integration.md|43-openclaw-framework-integration]]
