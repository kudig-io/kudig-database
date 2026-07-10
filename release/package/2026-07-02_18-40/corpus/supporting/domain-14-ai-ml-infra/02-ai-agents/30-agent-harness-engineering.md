---
title: Agent Harness 工程：从模型包装到生产级 Agent 系统设计 (domain-14-ai-ml-infra)
description: 'title: Agent Harness 工程：从模型包装到生产级 Agent 系统设计'
summary: 'title: Agent Harness 工程：从模型包装到生产级 Agent 系统设计'
category: general
tags:
- ai
- ai-agent
- kubelet
- scheduler
- prometheus
- helm
- containerd
- redis
- job
- llm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- Agent Harness 工程：从模型包装到生产级 Agent 系统设计 是什么
- 如何 Agent Harness 工程：从模型包装到生产级 Agent 系统设计
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- Agent
- Harness
- 工程：从模型包装到生产级
- Agent
- 系统设计
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- redis-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: Agent Harness 工程：从模型包装到生产级 Agent 系统设计
description: '# Agent Harness 工程：从模型包装到生产级 Agent 系统设计'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[kubelet|kubelet]]
- scheduler
- [[Prometheus|prometheus]]
- [[Helm|helm]]
- [[containerd|containerd]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 15min
intent_queries:
- Agent Harness 工程：从模型包装到生产级 Agent 系统设计 是什么
- 如何 Agent Harness 工程：从模型包装到生产级 Agent 系统设计
trigger_keywords:
- Agent
- Harness
- 工程：从模型包装到生产级
- Agent
- 系统设计
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

# Agent Harness 工程：从模型包装到生产级 Agent 系统设计

> **文档类型**: 前沿工程专题 | **最后更新**: 2026-04 | **关键词**: Agent Harness, Harness Engineering, 六层架构, Loop, Verification, Constraints, 基准测试, SWE-bench, GAIA, AgentBench, 质量门禁, 可观测性, Martin Fowler, Anthropic, Agentic AI

---

<!-- chunk: 概述 -->## 概述

**"2025 是 Agent 之年，2026 是 Agent Harness 之年。"**

模型之间的差距在收窄，但同一个模型在不同 Harness 下的表现差异堪比不同物种。Martin Fowler 网站于 2026 年 2 月由 Birgitta Böckeler 正式提出 "Harness Engineering" 概念；Anthropic 发布《Effective Harnesses for Long-Running Agents》；GitHub Copilot 工程师 Sean Goedecke 总结："最近的改进不是模型变好了，而是模型周围的系统变好了"。

本文系统性地阐述 Agent Harness 的核心定义、六层架构模型、设计模式、行业实证数据、基准测试全景、验证与质量门禁、可观测性体系，以及 K8S 运维场景中的 Harness 完整落地方案。所有内容基于 2025-2026 年最新行业实践，提供可直接工程化的架构方案和代码示例。

---

<!-- chunk: 1. 什么是 Agent Harness -->## 1. 什么是 Agent Harness

## 1.1 核心定义

一句话：**包裹在 AI 模型外部的完整系统，将模型的原始认知能力转化为可靠的生产输出。**

"Harness" 源自马具——缰绳、鞍座、胸带、眼罩。马本身很强壮，但没有马具可能跑偏、受惊、半路吃草。马具不让马更强壮，而是让马的力量**可靠地**转化为有用的工作。

```
普通 LLM 使用（无 Harness）:
  用户输入 → LLM 生成 → 输出结果
  问题: 答案不稳定、幻觉、丢失上下文、无法执行、无安全边界

Agent + Harness（完整系统）:
  目标 → 循环执行 → 工具调用 → 上下文管理 → 状态持久化 → 自检验证 → 约束控制 → 可靠输出
  效果: 稳定、可靠、可审计、可控、可度量
```

**模型是引擎，Harness 是整辆车。**

## 1.2 Harness vs 相关概念辨析

| 概念 | 范围 | 关注点 | 代表实践 |
|------|------|--------|---------|
| **Prompt Engineering** | 单次输入优化 | 写好提示词 | Few-shot、CoT Prompting |
| **Context Engineering** | 信息上下文构建 | 给模型看什么信息 | RAG、上下文窗口管理 |
| **Agent Engineering** | 自主执行体构建 | 让 AI 自主做事 | ReAct、Tool Use、Multi-Agent |
| **Harness Engineering** | 完整运行系统设计 | 让 Agent **可靠地**做事 | 六层架构、质量门禁、约束控制 |

```
范式演进路线:
2023: Prompt Engineering   → 优化输入文本           → "写好 Prompt"
2024: Context Engineering  → 优化信息上下文         → "给对信息"
2025: Agent Engineering    → 构建自主执行体         → "能做事"
2026: Harness Engineering  → 设计完整的运行系统     → "可靠地做事"
                             ↑ 当前前沿

核心转变:
  × "找到最好的模型"    → ✓ "为模型设计最好的运行系统"
  × "写出最好的 Prompt" → ✓ "构建完整的循环-工具-上下文-验证链"
  × "评估模型能力"      → ✓ "评估端到端系统表现"
```

## 1.3 为什么 2026 年是 Harness Engineering 之年

三个汇聚条件：

1. **模型能力跨越基线**：GPT-4.1、Claude Sonnet 4、Gemini 2.5 Pro 将基础能力提升到可用水平，模型不再是瓶颈
2. **模型间差距收窄**：顶级模型间的差距从"代差"缩小到"微差"，同一模型不同 Harness 的表现差异远大于不同模型间的差异
3. **Agent 规模化需求**：企业从"Agent PoC"走向"Agent 生产"，可靠性和可控性成为核心诉求

---

<!-- chunk: 2. Harness 六层架构模型 -->## 2. Harness 六层架构模型

```
Agent Harness 六层架构
│
├── Layer 1: Loop（循环层）
│   Agent 持续运行直到目标达成或触发终止条件
│
├── Layer 2: Tools（工具层）
│   让 AI 从"说"变为"做"，接入真实环境
│
├── Layer 3: Context（上下文层）
│   精确控制 AI 看到什么、看不到什么
│
├── Layer 4: Persistence（持久化层）
│   跨会话/执行保持记忆与状态
│
├── Layer 5: Verification（验证层）
│   执行后自检，确保输出质量
│
└── Layer 6: Constraints（约束层）
    明确的安全边界与行为限制
```

> 六层之间相互增强：Loop 驱动 Tools 执行，Tools 的输出经 Context 过滤后进入 Verification，Constraints 全程约束每一层的行为，Persistence 跨循环保持状态。

## 2.1 Layer 1: Loop（循环层）

Agent 的核心执行引擎。与传统"请求-响应"模式不同，Agent Loop 持续运行：观察 → 思考 → 行动 → 观察结果 → 继续或终止。

```python
class AgentLoop:
    """Agent 循环层：Harness 的执行引擎"""

    def __init__(self, llm, tools, max_iterations=20, timeout_seconds=300):
        self.llm = llm
        self.tools = tools
        self.max_iterations = max_iterations
        self.timeout_seconds = timeout_seconds

    def run(self, task: str) -> dict:
        start_time = time.time()
        trajectory = []
        iteration = 0

        while iteration < self.max_iterations:
            # 超时保护
            if time.time() - start_time > self.timeout_seconds:
                return {"status": "timeout", "trajectory": trajectory}

            # 1. 观察 + 思考
            observation = self._gather_context(task, trajectory)
            thought = self.llm.reason(observation)

            # 2. 决策：是否需要行动
            if thought.is_final_answer:
                return {
                    "status": "success",
                    "answer": thought.answer,
                    "trajectory": trajectory,
                    "iterations": iteration + 1,
                }

            # 3. 行动：调用工具
            action_result = self._execute_action(thought.action)

            # 4. 记录轨迹
            trajectory.append({
                "iteration": iteration,
                "thought": thought.reasoning,
                "action": thought.action,
                "observation": action_result,
            })

            # 5. 反漂移检测：连续相同操作 → 中断
            if self._detect_drift(trajectory):
                return {"status": "drift_detected", "trajectory": trajectory}

            iteration += 1

        return {"status": "max_iterations", "trajectory": trajectory}

    def _detect_drift(self, trajectory: list, window: int = 3) -> bool:
        """检测 Agent 是否陷入循环（反漂移）"""
        if len(trajectory) < window:
            return False
        recent = trajectory[-window:]
        actions = [step["action"] for step in recent]
        return len(set(str(a) for a in actions)) == 1  # 连续相同动作
```

**Harness 关键设计点**：
- **超时保护**：防止 Agent 无限运行，消耗资源和成本
- **最大迭代数**：硬性上限，即使 Agent 认为没完成也强制终止
- **反漂移检测**：识别 Agent 陷入"反复编辑同一文件"等死循环
- **轨迹记录**：每一步都记录，用于事后审计和评估

## 2.2 Layer 2: Tools（工具层）

工具让 Agent 从"只能说"变为"能做事"。但工具不是越多越好——**Vercel 将 15 个工具精简为 2 个，准确率从 80% 提升到 100%，速度提升 3.5x**。

```python
from typing import Protocol, Any

class Tool(Protocol):
    """工具标准接口"""
    name: str
    description: str

    def execute(self, **kwargs) -> Any: ...
    def validate_args(self, **kwargs) -> bool: ...

class ToolRegistry:
    """工具注册与管理"""

    def __init__(self):
        self._tools: dict[str, Tool] = {}
        self._usage_stats: dict[str, int] = {}

    def register(self, tool: Tool, allowed_contexts: list[str] = None):
        """注册工具，可指定允许的上下文"""
        self._tools[tool.name] = tool

    def get_tool_definitions(self, context: str = None) -> list[dict]:
        """根据上下文过滤可用工具（精简原则）"""
        tools = self._tools.values()
        return [
            {
                "name": t.name,
                "description": t.description,
                "parameters": t.get_schema(),
            }
            for t in tools
        ]

    def execute(self, tool_name: str, args: dict) -> dict:
        """安全执行工具调用"""
        tool = self._tools.get(tool_name)
        if not tool:
            return {"error": f"Unknown tool: {tool_name}"}

        if not tool.validate_args(**args):
            return {"error": f"Invalid arguments for {tool_name}"}

        try:
            result = tool.execute(**args)
            self._usage_stats[tool_name] = self._usage_stats.get(tool_name, 0) + 1
            return {"success": True, "result": result}
        except Exception as e:
            return {"error": str(e), "tool": tool_name}
```

**工具精简原则（Less is More）**：

| 原则 | 说明 | 案例 |
|------|------|------|
| **最小工具集** | 只提供完成任务必需的工具 | Vercel: 15→2 工具 |
| **无歧义描述** | 工具用途描述清晰，LLM 不会混淆 | `read_file` vs `search_codebase` 明确区分 |
| **参数验证** | 调用前校验参数合法性 | 禁止空路径、非法命令 |
| **错误恢复** | 工具失败时提供有意义的错误信息 | 返回具体原因而非 "Error" |

## 2.3 Layer 3: Context（上下文层）

上下文层决定 Agent 的"视野"——看到什么信息直接决定推理质量。

```python
class ContextManager:
    """上下文管理器：控制 Agent 的信息视野"""

    def __init__(self, max_tokens: int = 128000):
        self.max_tokens = max_tokens
        self.priority_sources = []
        self.rag_retriever = None

    def build_context(self, task: str, history: list = None) -> str:
        """分层构建上下文"""
        context_parts = []
        token_budget = self.max_tokens

        # 第 1 优先级：系统指令与角色定义（SOUL.md）
        system_prompt = self._load_system_prompt()
        context_parts.append(("system", system_prompt))
        token_budget -= count_tokens(system_prompt)

        # 第 2 优先级：当前任务上下文（环境扫描）
        env_context = self._scan_environment(task)
        context_parts.append(("environment", env_context))
        token_budget -= count_tokens(env_context)

        # 第 3 优先级：RAG 检索的相关知识
        if self.rag_retriever and token_budget > 2000:
            knowledge = self.rag_retriever.retrieve(task, top_k=5)
            context_parts.append(("knowledge", knowledge))
            token_budget -= count_tokens(knowledge)

        # 第 4 优先级：对话/执行历史（动态压缩）
        if history and token_budget > 1000:
            compressed = self._compress_history(history, token_budget)
            context_parts.append(("history", compressed))

        return self._assemble(context_parts)

    def _compress_history(self, history: list, budget: int) -> str:
        """智能历史压缩：保留关键步骤，省略冗余"""
        # 保留：错误步骤、关键发现、最终结论
        # 省略：成功的中间步骤、重复观察
        key_steps = [h for h in history if h.get("is_key_step")]
        recent = history[-3:]  # 最近 3 步始终保留
        return format_history(list(set(key_steps + recent)), budget)
```

**上下文工程的"信噪比"原则**：

```
信噪比优化策略:

高信号:
  ✓ 当前任务直接相关的文档/代码
  ✓ 环境状态（集群信息、配置、版本）
  ✓ 错误日志和关键事件
  ✓ 历史类似问题的解决方案

低信号（应过滤）:
  ✗ 无关的系统日志噪声
  ✗ 重复的成功操作记录
  ✗ 过时的历史信息
  ✗ 与任务无关的知识
```

## 2.4 Layer 4: Persistence（持久化层）

让 Agent 拥有"记忆"——跨会话保持状态和经验。

```python
from dataclasses import dataclass, field
from datetime import datetime

@dataclass
class AgentMemory:
    """Agent 持久化记忆"""
    short_term: list = field(default_factory=list)   # 当前会话
    long_term: dict = field(default_factory=dict)     # 跨会话知识
    episodic: list = field(default_factory=list)       # 历史事件记录

class PersistenceLayer:
    """持久化层：跨会话状态管理"""

    def __init__(self, storage_backend, vector_store=None):
        self.storage = storage_backend
        self.vector_store = vector_store

    def save_checkpoint(self, agent_id: str, state: dict):
        """保存执行检查点（可恢复）"""
        checkpoint = {
            "agent_id": agent_id,
            "timestamp": datetime.utcnow().isoformat(),
            "state": state,
            "trajectory": state.get("trajectory", []),
        }
        self.storage.save(f"checkpoint:{agent_id}", checkpoint)

    def learn_from_execution(self, task: str, result: dict):
        """从执行中学习，存入长期记忆"""
        if result.get("status") == "success":
            pattern = {
                "task_type": classify_task(task),
                "solution_steps": result["trajectory"],
                "tokens_used": result.get("total_tokens"),
                "success": True,
            }
            # 存入向量数据库，支持语义检索
            if self.vector_store:
                self.vector_store.upsert(
                    text=f"{task} -> {result['answer']}",
                    metadata=pattern,
                )

    def recall_similar(self, task: str, top_k: int = 3) -> list:
        """检索历史相似任务的解决经验"""
        if self.vector_store:
            return self.vector_store.search(task, top_k=top_k)
        return []
```

## 2.5 Layer 5: Verification（验证层）

Agent 执行完毕后的自检——这是 Harness 区别于"裸 Agent"的关键层。

```python
class VerificationLayer:
    """验证层：Agent 自检与质量保证"""

    def __init__(self, verifiers: list = None):
        self.verifiers = verifiers or []

    def verify(self, task: str, result: dict, context: dict) -> dict:
        """多维度验证 Agent 输出"""
        verification_report = {"passed": True, "checks": []}

        for verifier in self.verifiers:
            check = verifier.check(task, result, context)
            verification_report["checks"].append(check)
            if not check["passed"]:
                verification_report["passed"] = False

        return verification_report

class FactualConsistencyVerifier:
    """事实一致性检查：确保输出与上下文一致"""

    def check(self, task, result, context) -> dict:
        # 使用另一个 LLM 检查事实一致性
        prompt = f"""
        检查以下回答是否与给定上下文一致，是否存在幻觉。
        上下文: {context.get('sources', '')}
        回答: {result.get('answer', '')}
        输出 JSON: {{"consistent": true/false, "issues": [...]}}
        """
        check_result = self.judge_llm.invoke(prompt)
        return {"verifier": "factual_consistency", "passed": check_result["consistent"],
                "details": check_result.get("issues", [])}

class CommandSafetyVerifier:
    """命令安全检查：拦截危险操作"""

    DANGEROUS_PATTERNS = [
        r"kubectl\s+delete\s+(?:namespace|ns|node)",
        r"kubectl\s+drain\s+.*--force.*--delete-emptydir",
        r"rm\s+-rf\s+/",
        r"DROP\s+(?:TABLE|DATABASE)",
    ]

    def check(self, task, result, context) -> dict:
        commands = extract_commands(result.get("answer", ""))
        dangerous = []
        for cmd in commands:
            for pattern in self.DANGEROUS_PATTERNS:
                if re.search(pattern, cmd, re.IGNORECASE):
                    dangerous.append({"command": cmd, "pattern": pattern})
        return {"verifier": "command_safety", "passed": len(dangerous) == 0,
                "details": dangerous}

class OutputFormatVerifier:
    """输出格式检查：确保 YAML/JSON 语法正确"""

    def check(self, task, result, context) -> dict:
        answer = result.get("answer", "")
        issues = []
        # 检查 YAML 块
        yaml_blocks = re.findall(r"```yaml\n(.*?)```", answer, re.DOTALL)
        for i, block in enumerate(yaml_blocks):
            try:
                yaml.safe_load(block)
            except yaml.YAMLError as e:
                issues.append(f"YAML block {i}: {e}")
        return {"verifier": "output_format", "passed": len(issues) == 0,
                "details": issues}
```

**自检循环模式（LangChain 实证的核心改进）**：

```
Agent 完成任务
    │
    ▼
自检清单（Checklist）
    ├── 1. 回答是否直接回应了问题？
    ├── 2. 引用的事实是否有上下文支撑？
    ├── 3. 给出的命令是否可安全执行？
    ├── 4. YAML/JSON 是否语法正确？
    ├── 5. 是否遗漏了关键步骤？
    └── 6. 是否存在安全风险？
    │
    ▼
  通过? ──→ 输出结果
    │
   不通过
    │
    ▼
  自我纠正（最多 N 轮）
```

## 2.6 Layer 6: Constraints（约束层）

约束是 Harness 最容易被忽视但最关键的层——它定义了 Agent **不能**做什么。

```python
from dataclasses import dataclass

@dataclass
class AgentConstraints:
    """Agent 约束配置"""
    # 执行限制
    max_iterations: int = 20
    max_tokens_per_task: int = 50000
    timeout_seconds: int = 300

    # 安全边界
    read_only_mode: bool = False
    allowed_namespaces: list = None        # K8S: 只允许操作指定 namespace
    blocked_commands: list = None           # 禁止执行的命令
    require_approval_for: list = None       # 需要人工审批的操作类型

    # 成本控制
    max_cost_per_task_usd: float = 1.0
    daily_token_budget: int = 1_000_000

    # 输出约束
    max_output_length: int = 5000
    must_cite_sources: bool = True          # 必须引用来源
    no_pii_in_output: bool = True           # 输出不得包含 PII

class ConstraintEnforcer:
    """约束执行器：全程强制执行"""

    def __init__(self, constraints: AgentConstraints):
        self.constraints = constraints
        self.total_tokens = 0
        self.total_cost = 0.0

    def check_before_action(self, action: dict) -> tuple[bool, str]:
        """动作执行前的约束检查"""
        # Token 预算检查
        if self.total_tokens >= self.constraints.max_tokens_per_task:
            return False, "Token budget exceeded"

        # 成本预算检查
        if self.total_cost >= self.constraints.max_cost_per_task_usd:
            return False, "Cost budget exceeded"

        # 只读模式检查
        if self.constraints.read_only_mode:
            if action.get("type") in ["write", "delete", "update"]:
                return False, "Read-only mode: write operations blocked"

        # 命令黑名单检查
        cmd = action.get("command", "")
        if self.constraints.blocked_commands:
            for blocked in self.constraints.blocked_commands:
                if blocked in cmd:
                    return False, f"Blocked command: {blocked}"

        # 人工审批检查
        if self.constraints.require_approval_for:
            if action.get("type") in self.constraints.require_approval_for:
                approved = self._request_human_approval(action)
                if not approved:
                    return False, "Human approval denied"

        return True, "OK"
```

**约束层的核心原则：Less is More**

Vercel 的案例完美诠释了这一原则：将 15 个工具精简为 2 个后，准确率从 80% 跳升至 100%。更少的选择 = 更少的决策错误。

---

<!-- chunk: 3. Harness 设计模式 -->## 3. Harness 设计模式

## 3.1 五大核心设计模式

| 模式 | 说明 | 适用场景 | 实证来源 |
|------|------|---------|---------|
| **自检循环** | 完成后强制运行检查清单再输出 | 所有生产 Agent | LangChain: +13.7% 基准分 |
| **工具精简** | 精简到最少必要工具，减少决策空间 | 工具密集型 Agent | Vercel: 80%→100% 准确率 |
| **环境预扫描** | 执行前先扫描目录结构/集群状态/配置 | 代码 Agent、运维 Agent | LangChain 实证 |
| **推理预算分配** | 规划和验证分配更多 Token，执行分配较少 | 复杂多步任务 | LangChain 实证 |
| **反漂移检测** | 检测 Agent 反复编辑同一文件等死循环 | 长时运行 Agent | Anthropic 最佳实践 |

## 3.2 SOUL.md 与 SKILL.md 分层设计

Agent Harness 的配置通常分为两层：

```
# 🟢 低风险：只读/信息收集，通常无副作用
SOUL.md（角色定义 + 约束规则）:
  - 你是 K8S 运维诊断专家
  - 你只能使用 kubectl/prometheus/loki 三个工具
  - 生产环境中禁止执行 delete/drain 命令
  - 每个诊断必须引用具体的 Event 或日志证据
  - 不确定时必须标注"需人工确认"

SKILL.md（标准操作流程 SOP）:
  - Pod Pending 诊断流程: describe → events → scheduler → node
  - Node NotReady 诊断流程: kubelet → containerd → disk/memory → dmesg
  - OOM 诊断流程: describe → limits → top pods → oom-killer log
```
SOUL.md 定义"你是谁"和"你不能做什么"（Constraints），SKILL.md 定义"你怎么做"（Loop + Tools 编排）。两者结合形成完整的 Harness 配置。

---

<!-- chunk: 4. 行业实证案例 -->## 4. 行业实证案例

## 4.1 LangChain 编码 Agent（2026 年 2 月）

**实验设置**：在行业标准编码基准上测试，**不更换任何模型**，仅修改 Harness。

| Harness 改进 | 对应层 | 效果 |
|-------------|--------|------|
| 添加自检循环 | Verification | 拦截了 70%+ 的幻觉输出 |
| 注入环境上下文（目录结构扫描） | Context | 减少"文件不存在"类错误 |
| 反漂移检测 | Loop | 解决了 15% 任务中的死循环 |
| 推理预算重分配（规划多、执行少） | Constraints | Token 效率提升 20% |
| 失败模式分析（跨 run 学习） | Persistence | 避免重复犯相同错误 |

**结果**：基准分从 52.8% 提升至 66.5%，排名从 Top 30 外跃升至 Top 5。**没有换模型，没有微调，纯 Harness 改进。**

## 4.2 Vercel AI Agent

| 改进 | 具体操作 | 量化效果 |
|------|---------|---------|
| 工具精简 | 15 个工具 → 2 个工具 | 准确率 80% → 100% |
| Token 节约 | 工具描述和决策空间缩小 | Token 消耗 -37% |
| 响应加速 | 决策路径简化 | 速度 +3.5x |

**核心洞察**：约束不是限制，是赋能。更少的选择 = 更精准的决策。

## 4.3 OpenAI Codex

Codex 采用完整 Harness 设计，生成超过 100 万行生产代码，**零手动输入**。其 Harness 包含：
- 沙箱化的代码执行环境（每次执行在隔离容器中）
- 测试驱动验证循环（写代码 → 运行测试 → 修复 → 重新测试）
- 严格的文件权限约束（只能操作指定目录）
- 自动回滚机制（测试失败自动 revert）

## 4.4 GitHub Copilot Agent Mode

Sean Goedecke（GitHub Copilot 核心工程师）的总结：

> "很多最近的改进不是模型变好了。而是模型周围的系统改进了。"

Copilot Agent Mode 相比普通 Copilot 的 Harness 差异：

| 维度 | Copilot Chat | Copilot Agent Mode |
|------|-------------|-------------------|
| Loop | 单轮对话 | 多步执行循环 |
| Tools | 无（只生成文本） | 读文件、写文件、执行命令、搜索 |
| Context | 当前文件 | 整个项目结构 + 依赖关系 |
| Persistence | 无 | 跨步骤状态保持 |
| Verification | 无 | 运行测试、语法检查 |
| Constraints | 无（无限制文本输出） | 不删除关键文件、不泄露密钥 |

**同一个模型，6 层 Harness 的差异造就了完全不同的产品体验。**

---

<!-- chunk: 5. Agent 基准测试全景（2025-2026） -->## 5. Agent 基准测试全景（2025-2026）

## 5.1 基准测试分类

```
Agent 基准测试分类:
│
├── 软件工程
│   ├── SWE-bench        真实 GitHub Issue 修复
│   ├── HumanEval/MBPP   代码生成正确性
│   └── SWE-bench Verified  人工验证子集
│
├── 通用推理
│   ├── GAIA             多步推理 + 工具调用
│   └── GSM8K            数学推理
│
├── Web/浏览器
│   ├── WebArena          自托管网站多步交互
│   ├── Mind2Web          真实网站 2350 任务
│   └── WebChoreArena     长时 Web 任务（500+ 场景）
│
├── 工具/函数调用
│   ├── ToolBench         16000+ API 多工具链
│   ├── BFCL              多轮函数调用准确率
│   └── API-Bank          API 选择 + 参数生成
│
├── 多环境综合
│   ├── AgentBench        8 种环境综合评测
│   └── τ-bench (TAU)     航空/零售真实业务流程
│
└── 安全与对抗
    ├── AgentHarm          Agent 安全性基准
    └── InjectBench        提示注入抵抗测试
```

## 5.2 核心基准测试详解

| 基准 | 任务类型 | 规模 | 顶级得分 | 人类基线 | Harness 敏感度 |
|------|---------|------|---------|---------|--------------|
| **SWE-bench** | 真实 GitHub Issue 修复 | 2294 题 | ~49% (Devin) | N/A | 极高：Harness 决定成败 |
| **GAIA** | 多步推理 + 工具使用 | 466 题 | ~75% (L1) | 92% | 高：工具选择影响大 |
| **AgentBench** | 8 环境综合 | 多维度 | ~60% | 因任务而异 | 高：环境适应力 |
| **WebArena** | 网站交互 | 812 任务 | ~61.7% | 78% | 极高：上下文理解 |
| **ToolBench** | API 调用链 | 16k+ API | 变化中 | N/A | 中：工具数量影响 |
| **BFCL** | 函数调用准确率 | 多类别 | ~95% | N/A | 低：主要看模型能力 |
| **τ-bench** | 企业业务流程 | 零售/航空 | ~50% | ~95% | 高：SOP 编排能力 |
| **HumanEval** | 代码生成 | 164 题 | ~97% | 100% | 低：单次生成任务 |

> **Harness 敏感度**反映了同一模型在不同 Harness 下的成绩波动幅度。SWE-bench 和 WebArena 的 Harness 敏感度极高，意味着 Harness 设计的好坏对成绩影响巨大。

## 5.3 自定义基准测试设计

为特定场景（如 K8S 运维）设计基准测试：

```python
class K8sAgentBenchmark:
    """K8S 运维 Agent 自定义基准测试"""

    def __init__(self):
        self.test_cases = self._load_test_cases()

    def _load_test_cases(self) -> list:
        return [
            {
                "id": "pod-pending-001",
                "scenario": "Pod 处于 Pending 状态超过 5 分钟",
                "environment": {
                    "cluster_state": "node-pool-saturated",
                    "events": ["FailedScheduling: 0/3 nodes available"],
                },
                "expected_diagnosis": "节点资源不足或调度约束不满足",
                "expected_tools": ["kubectl_describe", "kubectl_get_events"],
                "expected_actions": ["检查节点资源", "检查调度约束"],
                "max_steps": 5,
                "difficulty": "L1",
            },
            {
                "id": "node-notready-001",
                "scenario": "Node 进入 NotReady 状态",
                "environment": {
                    "cluster_state": "kubelet-not-responding",
                    "node_conditions": ["MemoryPressure=True"],
                },
                "expected_diagnosis": "内存压力导致 kubelet 无法正常工作",
                "expected_tools": ["kubectl_describe_node", "kubectl_top"],
                "max_steps": 8,
                "difficulty": "L2",
            },
            # ... 更多测试用例
        ]

    def evaluate(self, agent, harness) -> dict:
        """运行基准评估"""
        results = []
        for case in self.test_cases:
            result = harness.run(agent, case["scenario"], case["environment"])
            score = self._score_result(case, result)
            results.append({"case_id": case["id"], **score})

        return {
            "total_cases": len(results),
            "pass_rate": sum(1 for r in results if r["passed"]) / len(results),
            "avg_steps": sum(r["steps"] for r in results) / len(results),
            "avg_score": sum(r["score"] for r in results) / len(results),
            "by_difficulty": self._group_by_difficulty(results),
        }

---

<!-- chunk: 6. Harness 验证与质量门禁 -->## 6. Harness 验证与质量门禁

## 6.1 评测指标体系

| 评测维度 | 指标 | 生产目标值 | 工具 |
|---------|------|-----------|------|
| **准确性** | Faithfulness（忠实度） | > 0.90 | RAGAS |
| **准确性** | Answer Relevancy（答案相关性） | > 0.80 | RAGAS |
| **效率** | 平均步骤数 / Token 消耗 / P95 延迟 | < 最优×1.5 / < 预算 / < 30s | Prometheus |
| **可靠性** | 任务完成率 / 幻觉率 / 工具成功率 | > 90% / < 5% / > 95% | LLM-as-Judge |
| **安全性** | 提示注入抵抗 / PII 泄露率 / 越权率 | > 99% / 0% / 0% | 红队测试 |
| **轨迹质量** | 工具选择准确率 / 推理连贯性 / 错误恢复 | > 0.85 / > 0.80 / > 0.70 | TrajectoryEvaluator |
| **可观测性** | Trace 覆盖率 / 告警响应时间 | 100% / < 5min | Langfuse + OTel |

## 6.2 CI/CD 质量门禁集成

Agent 的 Harness 变更（Prompt/Tools/Loop 逻辑）必须经过质量门禁验证：

```yaml
# GitHub Actions: Agent Harness Quality Gate
name: Harness Quality Gate

on:
  pull_request:
    paths:
      - 'agent/**'
      - 'prompts/**'
      - 'harness/**'
      - 'SOUL.md'
      - 'SKILL.md'

jobs:
  harness-eval:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4

    - name: Setup Evaluation Environment
      run: |
        pip install ragas langchain langfuse prometheus-client
        pip install -r requirements-eval.txt

    - name: Run Harness Benchmark
      env:
        OPENAI_API_KEY: ${{ secrets.OPENAI_API_KEY }}
        LANGFUSE_PUBLIC_KEY: ${{ secrets.LANGFUSE_PUBLIC_KEY }}
      run: |
        python scripts/run_harness_evaluation.py \
          --test-set tests/harness_test_cases.json \
          --harness-config harness/config.yaml \
          --output evaluation_report.json

    - name: Check Quality Gate
      run: |
        python scripts/check_harness_quality_gate.py \
          --report evaluation_report.json \
          --baseline reports/baseline.json \
          --fail-on-regression

    - name: Upload Report
      if: always()
      uses: actions/upload-artifact@v4
      with:
        name: harness-evaluation-report
        path: evaluation_report.json
```

```python
# scripts/check_harness_quality_gate.py
import json, sys

def check_quality_gate(report_path: str, baseline_path: str = None,
                       fail_on_regression: bool = True):
    with open(report_path) as f:
        report = json.load(f)

    THRESHOLDS = {
        "faithfulness": 0.85,
        "answer_relevancy": 0.80,
        "task_completion_rate": 0.90,
        "hallucination_rate": 0.05,     # 上界
        "avg_steps_ratio": 1.5,          # 相对最优路径的步骤比
        "command_safety_score": 1.0,     # 必须 100% 安全
    }

    failed = []
    for metric, threshold in THRESHOLDS.items():
        if metric not in report:
            continue
        actual = report[metric]
        is_upper_bound = metric in ["hallucination_rate", "avg_steps_ratio"]
        if is_upper_bound and actual > threshold:
            failed.append(f"  ✗ {metric}: {actual:.3f} > {threshold}")
        elif not is_upper_bound and actual < threshold:
            failed.append(f"  ✗ {metric}: {actual:.3f} < {threshold}")

    # 回归检测：与基线对比
    if baseline_path and fail_on_regression:
        with open(baseline_path) as f:
            baseline = json.load(f)
        for metric in ["faithfulness", "task_completion_rate"]:
            if metric in report and metric in baseline:
                if report[metric] < baseline[metric] - 0.02:  # 允许 2% 波动
                    failed.append(
                        f"  ✗ REGRESSION {metric}: "
                        f"{report[metric]:.3f} < baseline {baseline[metric]:.3f}"
                    )

    if failed:
        print("Harness Quality Gate FAILED:")
        for f in failed:
            print(f)
        sys.exit(1)
    else:
        print("Harness Quality Gate PASSED ✓")
```

## 6.3 A/B 测试与灰度评估

新 Harness 上线前的灰度策略：

```
Harness 灰度发布流程:

1. Shadow Mode（影子模式）
   新旧 Harness 并行运行，只对比结果不生效新版
   持续 24h，收集对比数据

2. Canary（金丝雀）
   5% 流量切到新 Harness
   监控: 成功率、延迟、Token 消耗、用户满意度
   持续 48h

3. Progressive Rollout（渐进发布）
   5% → 25% → 50% → 100%
   每阶段至少运行 24h
   任一指标回退 > 5% 自动回滚
```

---

<!-- chunk: 7. Harness 可观测性 -->## 7. Harness 可观测性

## 7.1 OpenTelemetry for Agents

```python
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter

# 初始化 OTel
provider = TracerProvider()
provider.add_span_processor(
    BatchSpanProcessor(OTLPSpanExporter(endpoint="http://otel-collector:4317"))
)
trace.set_tracer_provider(provider)
tracer = trace.get_tracer("agent-harness")

class TracedHarness:
    """带完整追踪的 Harness 包装器"""

    def run(self, task: str) -> dict:
        with tracer.start_as_current_span("agent_task") as span:
            span.set_attribute("task", task[:200])
            span.set_attribute("harness.version", self.version)

            # Loop 层追踪
            for i, step in enumerate(self._execute_loop(task)):
                with tracer.start_as_current_span(f"iteration_{i}") as step_span:
                    step_span.set_attribute("thought", step["thought"][:500])
                    step_span.set_attribute("action", str(step["action"]))

                    # Tool 层追踪
                    if step.get("tool_call"):
                        with tracer.start_as_current_span(
                            f"tool:{step['tool_call']['name']}"
                        ) as tool_span:
                            tool_span.set_attribute("tool.args", str(step["tool_call"]["args"]))
                            tool_span.set_attribute("tool.success", step["tool_result"]["success"])

            # Verification 层追踪
            with tracer.start_as_current_span("verification") as v_span:
                verification = self._verify(result)
                v_span.set_attribute("verification.passed", verification["passed"])

            span.set_attribute("result.status", result["status"])
            span.set_attribute("result.iterations", result["iterations"])
            return result
```

## 7.2 Prometheus 指标体系

```python
from prometheus_client import Counter, Histogram, Gauge

# Harness 级指标
harness_task_total = Counter(
    'harness_task_total', 'Harness 处理的任务总数',
    ['harness_version', 'status', 'task_type']
)
harness_task_duration = Histogram(
    'harness_task_duration_seconds', '任务端到端执行时间',
    ['harness_version'],
    buckets=[0.5, 1, 2, 5, 10, 30, 60, 120, 300]
)
harness_iterations = Histogram(
    'harness_iterations_per_task', '每任务迭代次数',
    ['harness_version'],
    buckets=[1, 2, 3, 5, 8, 10, 15, 20]
)
harness_verification_pass_rate = Gauge(
    'harness_verification_pass_rate', '验证层通过率（滑动窗口）',
    ['harness_version']
)
harness_drift_detected_total = Counter(
    'harness_drift_detected_total', '漂移检测触发次数',
    ['harness_version']
)
harness_constraint_violations = Counter(
    'harness_constraint_violations_total', '约束违反次数',
    ['constraint_type', 'harness_version']
)
```

## 7.3 关键告警规则

```yaml
groups:
  - name: harness_alerts
    rules:
    - alert: HarnessVerificationFailureRateHigh
      expr: harness_verification_pass_rate < 0.85
      for: 10m
      labels:
        severity: critical
      annotations:
        summary: "Harness 验证通过率低于 85%"
        description: "{{ $labels.harness_version }} 的验证通过率已降至 {{ $value | humanizePercentage }}，需检查 Prompt 或工具配置"

    - alert: HarnessDriftRateHigh
      expr: rate(harness_drift_detected_total[5m]) > 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Agent 漂移检测频繁触发"
        description: "可能存在 Prompt 不明确或工具返回不一致的问题"

    - alert: HarnessConstraintViolation
      expr: rate(harness_constraint_violations_total[5m]) > 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "检测到约束违反"
        description: "类型: {{ $labels.constraint_type }}，需立即检查"
```

---

<!-- chunk: 8. K8S 运维 Agent Harness 完整设计 -->## 8. K8S 运维 Agent Harness 完整设计

## 8.1 工单诊断智能体 Harness 架构

```
# 🟢 低风险：只读/信息收集，通常无副作用
K8S 工单诊断智能体 Harness 全景:

┌─────────────────────────────────────────────────────┐
│                  Constraints Layer                   │
│  只读模式 │ 禁止 delete/drain │ Token 预算 │ PII 脱敏  │
├─────────────────────────────────────────────────────┤
│                  Verification Layer                  │
│  RAGAS 忠实度>0.90 │ 命令安全检查 │ YAML 语法校验    │
├─────────────────────────────────────────────────────┤
│                  Persistence Layer                   │
│  历史工单库 │ 集群基线记忆 │ 故障模式知识图谱          │
├─────────────────────────────────────────────────────┤
│                   Context Layer                      │
│  kudig-database 950+ 文档 │ 集群实时状态 │ Event 日志  │
├─────────────────────────────────────────────────────┤
│                    Tools Layer                       │
│  kubectl │ prometheus_query │ loki_search │ helm_info  │
├─────────────────────────────────────────────────────┤
│                     Loop Layer                       │
│  工单接入 → 信息采集 → 根因分析 → 方案生成            │
│  → 安全评审 → [人工审批] → 执行 → 验证 → 闭环         │
└─────────────────────────────────────────────────────┘
```
## 8.2 K8S Harness 完整实现

```python
from dataclasses import dataclass

@dataclass
class K8sHarnessConfig:
    """K8S 运维 Harness 配置"""
    # Loop
    max_diagnosis_steps: int = 10
    timeout_seconds: int = 120

    # Tools
    allowed_tools: list = None

    # Context
    knowledge_base_path: str = "kudig-database"
    rag_top_k: int = 5

    # Persistence
    memory_backend: str = "redis"
    vector_store: str = "milvus"

    # Verification
    min_faithfulness: float = 0.90
    require_evidence: bool = True

    # Constraints
    read_only: bool = True
    blocked_commands: list = None
    require_approval_for_writes: bool = True
    max_tokens_per_task: int = 30000

    def __post_init__(self):
        if self.allowed_tools is None:
            self.allowed_tools = [
                "kubectl_get", "kubectl_describe", "kubectl_logs",
                "kubectl_top", "kubectl_events",
                "prometheus_query", "loki_search",
            ]
        if self.blocked_commands is None:
            self.blocked_commands = [
                "kubectl delete", "kubectl drain", "kubectl cordon",
                "helm uninstall", "kubectl edit",
            ]


class K8sDiagnosisHarness:
    """K8S 工单诊断 Harness 完整实现"""

    def __init__(self, config: K8sHarnessConfig, llm, tools, rag):
        self.config = config
        self.llm = llm
        self.loop = AgentLoop(llm, tools, config.max_diagnosis_steps, config.timeout_seconds)
        self.context_mgr = ContextManager(rag_retriever=rag)
        self.persistence = PersistenceLayer(storage_backend=config.memory_backend)
        self.verifier = VerificationLayer([
            FactualConsistencyVerifier(),
            CommandSafetyVerifier(),
            OutputFormatVerifier(),
        ])
        self.constraints = ConstraintEnforcer(
            AgentConstraints(
                read_only_mode=config.read_only,
                blocked_commands=config.blocked_commands,
                max_tokens_per_task=config.max_tokens_per_task,
                require_approval_for=["write", "delete"] if config.require_approval_for_writes else [],
            )
        )

    def diagnose(self, ticket: dict) -> dict:
        """执行完整诊断流程"""
        task = ticket["description"]

        # 1. Context: 构建上下文
        context = self.context_mgr.build_context(task)

        # 2. Persistence: 检索历史相似工单
        similar = self.persistence.recall_similar(task, top_k=3)
        if similar:
            context += f"\n\n历史相似工单参考:\n{format_similar(similar)}"

        # 3. Loop: 执行诊断循环
        result = self.loop.run(task)

        # 4. Verification: 验证输出
        verification = self.verifier.verify(task, result, {"sources": context})
        if not verification["passed"]:
            # 自我纠正：将验证失败信息反馈给 Agent 重做
            result = self._self_correct(task, result, verification)

        # 5. Persistence: 保存执行经验
        self.persistence.learn_from_execution(task, result)

        return {
            "diagnosis": result.get("answer"),
            "confidence": self._calculate_confidence(result, verification),
            "evidence": result.get("trajectory"),
            "verification": verification,
            "similar_tickets": similar,
        }
```

---

<!-- chunk: 9. 多 Agent Harness 编排 -->## 9. 多 Agent Harness 编排

## 9.1 Harness 组合模式

单个 Agent 的 Harness 就绪后，下一步是**多 Agent 的 Harness 组合**。

```
多 Agent Harness 编排:

┌──────────────────────────────────────────────┐
│              Orchestrator Harness             │
│  任务分配 │ 结果聚合 │ 冲突解决 │ 全局约束    │
├──────────────────────────────────────────────┤
│  ┌─────────┐  ┌──────────┐  ┌────────────┐  │
│  │诊断 Agent│  │修复 Agent │  │验证 Agent  │  │
│  │ Harness  │  │ Harness   │  │ Harness    │  │
│  │(只读)    │  │(写操作)   │  │(只读+对比) │  │
│  └─────────┘  └──────────┘  └────────────┘  │
│       │            │              │          │
│       ▼            ▼              ▼          │
│  根因分析 ──→ 修复方案 ──→ 验证恢复          │
└──────────────────────────────────────────────┘
```

**Harness 隔离原则**：
- 诊断 Agent 的 Harness 设为**只读**（Constraints: read_only=True）
- 修复 Agent 的 Harness 带**人工审批**（Constraints: require_approval=True）
- 验证 Agent 的 Harness **独立运行**，不信任修复 Agent 的自述结果

## 9.2 分层 Harness 设计

```python
class LayeredHarness:
    """分层 Harness：基础层 + 场景层 + 用户层"""

    def __init__(self):
        # 基础层：所有 Agent 共享的 Harness 配置
        self.base = BaseHarness(
            max_iterations=20,
            timeout=300,
            safety_checks=True,
            otel_tracing=True,
        )
        # 场景层：特定场景的 Harness 扩展
        self.scenario_harnesses = {
            "k8s_diagnosis": K8sDiagnosisHarness(read_only=True),
            "k8s_remediation": K8sRemediationHarness(require_approval=True),
            "code_review": CodeReviewHarness(max_files=50),
        }
        # 用户层：用户自定义的 SOUL.md / SKILL.md
        self.user_config = UserHarnessConfig.from_files(
            soul_path="SOUL.md",
            skill_path="SKILL.md",
        )

    def build(self, task_type: str) -> ComposedHarness:
        """组合三层 Harness"""
        scenario = self.scenario_harnesses.get(task_type, self.base)
        return ComposedHarness(
            base=self.base,
            scenario=scenario,
            user=self.user_config,
        )
```

---

<!-- chunk: 10. 最佳实践与反模式 -->## 10. 最佳实践与反模式

## 最佳实践

| 实践 | 说明 | 对应层 |
|------|------|-------|
| **工具精简** | 只提供完成任务必需的最少工具，减少 LLM 决策空间 | Tools |
| **环境预扫描** | 执行前先收集环境信息注入上下文 | Context |
| **自检清单** | 每次输出前强制运行验证检查清单 | Verification |
| **推理预算分配** | 规划和验证多分配 Token，执行阶段少分配 | Constraints |
| **反漂移保护** | 检测连续相同操作并中断 | Loop |
| **分层约束** | 基础安全约束全局生效，场景约束按任务叠加 | Constraints |
| **灰度上线** | 新 Harness 先 Shadow → Canary → Progressive | 全局 |
| **可观测性优先** | 从第一天就接入 OTel/Langfuse 追踪 | 全局 |
| **失败模式学习** | 跨运行分析失败模式，自动优化 Harness | Persistence |
| **基线对比** | 每次变更与历史基线对比，防止回退 | Verification |

## 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| **工具过载** | 给 Agent 所有可能的工具 → 决策混乱 | 精简到最小必要集 |
| **无超时保护** | Agent 无限运行 → 成本爆炸 | 设置硬性超时和迭代上限 |
| **跳过验证** | 信任 Agent 输出 → 幻觉上线 | 强制自检 + 事实一致性校验 |
| **无约束的写操作** | Agent 直接操作生产环境 → 灾难 | 只读默认，写操作需审批 |
| **用同一模型自评** | GPT-4o 生成 + GPT-4o 评估 → 同质偏见 | 用不同模型做 Judge |
| **只测 Happy Path** | 测试集全是简单场景 → 边缘崩溃 | 包含异常、边界、对抗用例 |
| **无基线记录** | 不记录历史评估分数 → 无法判断进退 | 每次评估保存基线 |
| **Prompt 代替 Harness** | 用提示词解决系统问题 → 脆弱 | 用系统工程解决系统问题 |

---

<!-- chunk: 11. Harness 成熟度模型 -->## 11. Harness 成熟度模型

```
Agent Harness 成熟度五级:

L1 - 裸 Agent
    直接调用 LLM API，无循环、无工具、无验证
    风险: 幻觉率高、不可控

L2 - 基础 Harness
    有 Agent Loop + 基本工具调用
    但无验证、无约束、无持久化
    典型: 简单的 ReAct Agent

L3 - 生产就绪 Harness
    六层架构完整：Loop + Tools + Context + Persistence + Verification + Constraints
    有 CI/CD 质量门禁，有基本监控
    典型: 生产级诊断 Agent

L4 - 企业级 Harness
    多 Agent 编排 + 分层 Harness + 灰度发布 + 完整可观测性
    有 A/B 测试，有回归检测，有失败模式自动学习
    典型: 企业 AIOps 平台

L5 - 自进化 Harness
    Harness 配置本身由 Meta-Agent 优化
    自动调整工具集、上下文策略、约束参数
    典型: 下一代自适应 Agent 平台（前沿研究）
```

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [01 - AI Agent 基础](./01-ai-agent-fundamentals.md) | Agent Loop、ReAct 推理模式——Harness Loop 层的理论基础 |
| [03 - Agent 框架对比](./03-agent-frameworks-comparison.md) | LangChain/LangGraph——Harness 的工程载体 |
| [05 - Tool Use & Function Calling](./05-tool-use-function-calling.md) | 工具调用规范——Harness Tools 层的实现基础 |
| [07 - 记忆管理](./07-memory-context-management.md) | 记忆系统——Harness Persistence 层的核心能力 |
| [08 - 评测与可观测性](./08-agent-evaluation-observability.md) | RAGAS/LLM-as-Judge——Harness Verification 层的评测工具 |
| [09 - 生产部署指南](./09-production-deployment-guide.md) | K8s 部署——Harness 的运行基础设施 |
| [10 - 安全护栏](./10-security-guardrails.md) | 安全框架——Harness Constraints 层的安全实现 |
| [43 - OpenClaw File-First 架构集成](./43-openclaw-framework-integration.md) | SOUL.md/SKILL.md 分层设计的完整实施方案、File-First 与 Harness 融合 |
| [openclaw-workspace/](./openclaw-workspace/) | K8S 运维 Agent 的完整 7 文件工作区配置实例 |
| [domain-10-troubleshooting-diagnostics](../domain-10-troubleshooting-diagnostics/) | K8S 故障排查——K8S Harness 的知识语料库 |
| [topic-fta](../domain-10-troubleshooting-diagnostics/FTA故障树/) | FTA 故障树——结构化诊断 Harness 的推理骨架 |

---

<!-- chunk: 参考来源 -->## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| Martin Fowler 网站 / Birgitta Böckeler | 《Harness Engineering》正式定义 | 2026-02 |
| Anthropic | 《Effective Harnesses for Long-Running Agents》 | 2026-02 |
| Sean Goedecke (GitHub Copilot) | Agent 系统改进 vs 模型改进的观察 | 2025 |
| Aakash Gupta | 《2025 Was Agents. 2026 Is Agent Harnesses》 | 2026-01 |
| LangChain 团队 | 编码 Agent Harness 优化实验 52.8%→66.5% | 2026-02 |
| Vercel 团队 | 工具精简实验 15→2 | 2025 |
| OpenAI | Codex Agent 系统设计 | 2025-2026 |

---

*本文档为 kudig-database 项目 02-ai-agents 专题原创内容，结合 2025-2026 年行业最新实践编写。*
```

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 02-ai-agents MOC
- [[domain-14-ai-ml-infra/AI-Agents/README.md|AI Agent 工程专题]]
- [[domain-14-ai-ml-infra/AI-Agents/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/AI-Agents/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/AI-Agents/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/AI-Agents/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/AI-Agents/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/AI-Agents/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/AI-Agents/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/AI-Agents/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/AI-Agents/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/AI-Agents/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 28-agent-cli-enterprise-automation
- 29-agentscope-studio-skill-demo
- 31-agent-harness-loop-execution
- 32-agent-harness-tool-engineering


<!-- risk-assessed -->
