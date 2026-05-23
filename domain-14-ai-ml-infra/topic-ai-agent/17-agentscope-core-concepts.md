---
title: AgentScope 核心概念与基础操作 (domain-14-ai-ml-infra)
description: 'description: ''**文档类型**: 核心概念专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, 核心概念, State,'
category: general
tags:
- ai
- ai-agent
- redis
- mysql
- postgresql
- hpa
- llm
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- AgentScope 核心概念与基础操作 是什么
- 如何 AgentScope 核心概念与基础操作
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- AgentScope
- 核心概念与基础操作
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- redis-basics
- mysql-basics
created: "2026-05-23"
---

title: AgentScope 核心概念与基础操作
description: '**文档类型**: 核心概念专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, 核心概念, State,
  Message, Agent, Model, Formatter, Memory, ReActAgent, AgentBase, 自定义智能体'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- redis
- mysql
- postgresql
- hpa
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- AgentScope 核心概念与基础操作 是什么
- 如何 AgentScope 核心概念与基础操作
trigger_keywords:
- AgentScope
- 核心概念与基础操作
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

# AgentScope 核心概念与基础操作

> **文档类型**: 核心概念专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, 核心概念, State, Message, Agent, Model, Formatter, Memory, ReActAgent, AgentBase, 自定义智能体

---

<!-- chunk: 概述 -->## 概述

AgentScope 将构建 Agent 应用所需的组件抽象为**四大核心模块**：消息（Message）、模型（Model）、记忆（Memory）和工具（Tool），并通过统一的状态管理机制将它们串联。本文深入解析每个核心概念的设计原理与使用方法，为后续的工具系统、记忆管理和多 Agent 编排打下基础。

---

<!-- chunk: 1. 六大核心抽象 -->## 1. 六大核心抽象

```
AgentScope 核心抽象
│
├── State（状态）      → 所有对象的运行时快照，支持导出/恢复
├── Message（消息）    → 智能体间通信的统一数据结构
├── Model（模型）      → LLM API 的统一接口封装
├── Formatter（格式化）→ 消息到 LLM API 格式的转换层
├── Memory（记忆）     → 对话历史与知识的存储管理
└── Tool（工具）       → 智能体可调用的 Python 可调用对象
```

这六个概念的关系：

```
                    ┌─────────────┐
                    │    Agent    │
                    │  (智能体)   │
                    └──────┬──────┘
                           │ 组合使用
          ┌────────┬───────┼───────┬────────┐
          ▼        ▼       ▼       ▼        ▼
     ┌────────┐┌───────┐┌──────┐┌───────┐┌──────┐
     │ Model  ││Memory ││ Tool ││Format ││ State│
     │ 模型   ││ 记忆  ││ 工具 ││ 格式化││ 状态 │
     └────────┘└───────┘└──────┘└───────┘└──────┘
          │        │       │       │        │
          └────────┴───────┴───────┴────────┘
                    通过 Message 交互
```

---

<!-- chunk: 2. State — 状态管理 -->## 2. State — 状态管理

#<!-- chunk: 2.1 设计理念 -->## 2.1 设计理念

AgentScope 将对象的**初始化**与**状态管理**分离。所有有状态的模块都继承自 `StateModule` 基类，通过 `state_dict` 和 `load_state_dict` 方法可以：

- 导出当前状态快照
- 恢复到任意保存的状态
- 实现跨会话的状态持久化

#<!-- chunk: 2.2 StateModule 基类 -->## 2.2 StateModule 基类

`StateModule` 是 AgentScope 状态管理的基础，提供三个核心方法：

| 方法 | 参数 | 说明 |
|------|------|------|
| `register_state` | attr_name, custom_to_json, custom_from_json | 将属性注册为状态，支持自定义序列化 |
| `state_dict` | — | 获取当前对象的状态字典（同步方法） |
| `load_state_dict` | state_dict, strict | 将状态字典加载到当前对象 |

在 `StateModule` 对象中，以下属性自动成为状态的一部分：
- **继承自 StateModule 的属性**（自动注册）
- **通过 `register_state` 手动注册的属性**

```python
from agentscope.module import StateModule
import json


class K8sConfig(StateModule):
    """K8s 集群配置（有状态对象）"""

    def __init__(self, cluster_name: str) -> None:
        super().__init__()
        self.cluster_name = cluster_name
        self.register_state("cluster_name")  # 手动注册为状态


class DiagnosisContext(StateModule):
    """诊断上下文（嵌套状态管理）"""

    def __init__(self) -> None:
        super().__init__()
        # config 继承自 StateModule → 自动成为状态的一部分
        self.config = K8sConfig("production-cluster")
        self.findings = "尚无发现"
        self.register_state("findings")  # 手动注册


# 嵌套式状态导出
ctx = DiagnosisContext()
state = ctx.state_dict()
print(json.dumps(state, indent=2, ensure_ascii=False))
# {
#   "config": { "cluster_name": "production-cluster" },
#   "findings": "尚无发现"
# }
```

> **关键**：`AgentBase`、`MemoryBase`、`LongTermMemoryBase` 和 `Toolkit` 都继承自 `StateModule`，因此支持自动嵌套状态管理。

#<!-- chunk: 2.3 Agent 状态管理实践 -->## 2.3 Agent 状态管理实践

```python
agent = ReActAgent(name="Friday", ...)

# 导出状态（state_dict 是同步方法）
state = agent.state_dict()
# state 包含: name, _sys_prompt, memory 内容, toolkit 状态

# 对话后状态发生变化
await agent(Msg("user", "你好", "user"))
new_state = agent.state_dict()
# memory.content 现在包含对话消息

# 恢复到初始状态
agent.load_state_dict(state)
# agent 的记忆被清空，恢复到初始状态
```

#<!-- chunk: 2.4 有状态对象一览 -->## 2.4 有状态对象一览

AgentScope 中以下对象都是有状态的（继承 `StateModule`）：

| 对象 | 状态内容 | 应用场景 |
|------|---------|----------|
| Agent（智能体） | name, sys_prompt, memory, toolkit | 会话恢复、Agent 迁移 |
| Memory（记忆） | 对话历史消息、压缩摘要 | 持久化对话 |
| Long-term Memory | 检索索引、跨会话知识 | 跨会话知识积累 |
| Toolkit（工具模块） | active_groups、工具状态 | 动态工具管理 |
| PlanNotebook | 计划和子任务状态 | 任务计划持久化 |

---

<!-- chunk: 3. Message — 消息系统 -->## 3. Message — 消息系统

#<!-- chunk: 3.1 Msg 类 -->## 3.1 Msg 类

`Msg` 是 AgentScope 中最核心的数据结构，承担四大职责：

```
Message 的四大职责
│
├── 1. 智能体间信息交换    → agent_a(msg) → agent_b(response)
├── 2. 用户界面信息展示    → agent.print(msg) → 终端/Web UI
├── 3. 记忆存储            → memory.add(msg)
└── 4. LLM API 统一媒介    → formatter.format([msg1, msg2, ...]) → API 请求
```

#<!-- chunk: 3.2 创建消息 -->## 3.2 创建消息

```python
from agentscope.message import Msg

# 基础文本消息
user_msg = Msg(
    name="user",
    content="请分析 Pod CrashLoopBackOff 的原因",
    role="user",
)

# 系统消息
system_msg = Msg(
    name="system",
    content="你是一个 Kubernetes 运维专家",
    role="system",
)

# 助手消息
assistant_msg = Msg(
    name="Friday",
    content="我来帮你分析 Pod 的问题...",
    role="assistant",
)
```

#<!-- chunk: 3.3 消息的核心字段 -->## 3.3 消息的核心字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | str | 消息发送者名称 |
| `content` | str / list | 消息内容（支持多模态） |
| `role` | str | 角色标识：`"user"`, `"assistant"`, `"system"` |
| `metadata` | dict | 元数据（可选） |

#<!-- chunk: 3.4 多模态消息 -->## 3.4 多模态消息

```python
# 包含图片的消息
multimodal_msg = Msg(
    name="user",
    content=[
        {"type": "text", "text": "这个架构图有什么问题？"},
        {"type": "image_url", "image_url": {"url": "https://example.com/arch.png"}},
    ],
    role="user",
)

# 获取纯文本内容
text = multimodal_msg.get_text_content()
```

---

<!-- chunk: 4. Model — 模型接口 -->## 4. Model — 模型接口

#<!-- chunk: 4.1 支持的模型提供商 -->## 4.1 支持的模型提供商

AgentScope 通过模型包装器（Model Wrapper）提供统一的 LLM 接口：

| 模型类 | 提供商 | 典型模型 |
|--------|--------|---------|
| `DashScopeChatModel` | 阿里云百炼 | qwen-max, qwen-plus, qwen-turbo |
| `OpenAIChatModel` | OpenAI | gpt-4o, gpt-4o-mini |
| `OllamaChatModel` | Ollama（本地） | qwen2.5, llama3, mistral |
| `AnthropicChatModel` | Anthropic | claude-3.5-sonnet |
| `GeminiChatModel` | Google | gemini-1.5-pro |

#<!-- chunk: 4.2 模型配置详解 -->## 4.2 模型配置详解

**DashScope（推荐，与 AgentScope 集成最深）**：

```python
from agentscope.model import DashScopeChatModel

model = DashScopeChatModel(
    model_name="qwen-max",           # 模型名称
    api_key=os.environ["DASHSCOPE_API_KEY"],  # API Key
    stream=True,                      # 流式输出（生产推荐）
    enable_thinking=False,            # 是否启用思考模式（Qwen3 支持）
    temperature=0.7,                  # 温度参数
    max_tokens=4096,                  # 最大输出 Token 数
)
```

**OpenAI**：

```python
from agentscope.model import OpenAIChatModel

model = OpenAIChatModel(
    model_name="gpt-4o",
    api_key=os.environ["OPENAI_API_KEY"],
    stream=True,
    temperature=0,
    # 可选：自定义 API 端点（适用于 Azure OpenAI 或代理）
    # base_url="https://your-proxy.com/v1",
)
```

**本地模型（Ollama）**：

```python
from agentscope.model import OllamaChatModel

model = OllamaChatModel(
    model_name="qwen2.5:7b",
    base_url="http://localhost:11434",
    stream=True,
)
```

#<!-- chunk: 4.3 模型调用流程 -->## 4.3 模型调用流程

```
应用代码                AgentScope 内部
  │                         │
  │  agent(msg)             │
  │──────────────────►      │
  │                    formatter.format(messages)
  │                         │ → 转换为 API 格式
  │                    model(prompt)
  │                         │ → 调用 LLM API
  │                    解析响应 → Msg
  │  ◄──────────────────    │
  │  response               │
```

---

<!-- chunk: 5. Formatter — 提示词格式化 -->## 5. Formatter — 提示词格式化

#<!-- chunk: 5.1 为什么需要 Formatter -->## 5.1 为什么需要 Formatter

不同 LLM API 对消息格式的要求不同。Formatter 负责将 AgentScope 的 `Msg` 对象转换为具体 API 所需的格式，同时处理提示工程、截断和消息验证。

#<!-- chunk: 5.2 内置 Formatter -->## 5.2 内置 Formatter

| Formatter | 适用模型 | 特点 |
|-----------|---------|------|
| `DashScopeChatFormatter` | DashScope 系列 | 支持 Qwen 特有的工具调用格式 |
| `OpenAIChatFormatter` | OpenAI / Azure | 标准 OpenAI Chat Completions 格式 |
| `OllamaChatFormatter` | Ollama 本地模型 | 适配 Ollama API 格式 |
| `AnthropicChatFormatter` | Claude 系列 | Anthropic Messages API 格式 |
| `MultiAgentFormatter` | 多智能体场景 | 处理消息中包含多个身份实体的场景 |

#<!-- chunk: 5.3 使用规则 -->## 5.3 使用规则

**关键原则：Formatter 必须与 Model 匹配**。

```python
# 正确 ✅ - DashScope 模型 + DashScope 格式化器
agent = ReActAgent(
    model=DashScopeChatModel(model_name="qwen-max", ...),
    formatter=DashScopeChatFormatter(),
    ...
)

# 正确 ✅ - OpenAI 模型 + OpenAI 格式化器
agent = ReActAgent(
    model=OpenAIChatModel(model_name="gpt-4o", ...),
    formatter=OpenAIChatFormatter(),
    ...
)

# 错误 ❌ - 混用会导致格式错误
agent = ReActAgent(
    model=DashScopeChatModel(model_name="qwen-max", ...),
    formatter=OpenAIChatFormatter(),  # 格式不兼容!
    ...
)
```

#<!-- chunk: 5.4 多智能体格式化 -->## 5.4 多智能体格式化

当消息中包含多个身份实体时（如多人聊天、游戏），标准的 `role` 字段（user/assistant/system）无法区分不同发言者。此时需要使用 `MultiAgentFormatter`（如 `DashScopeMultiAgentFormatter`）：

```python
from agentscope.formatter import DashScopeMultiAgentFormatter

# 适用于多人聊天、游戏、社交仿真等场景
formatter = DashScopeMultiAgentFormatter()
```

> **关键区分**：多智能体工作流 ≠ 格式化器中的多智能体。
>
> 例如，即使以下代码涉及多个智能体（tool_agent 和调用者），但输入被包装为 role="user" 的消息，标准 Formatter 即可区分：
>
> ```python
> async def tool_function(query: str) -> str:
>     """调用另一个智能体的工具函数"""
>     msg = Msg("user", query, role="user")
>     tool_agent = ReActAgent(name="Programmer", ...)
>     return await tool_agent(msg)
> ```
>
> 只有当单次 LLM 调用的输入消息中包含多个不同身份的发言者（如 Alice、Bob、Charlie 同时对话），才需要 `MultiAgentFormatter`。

---

<!-- chunk: 6. Agent — 智能体体系 -->## 6. Agent — 智能体体系

#<!-- chunk: 6.1 核心基类 -->## 6.1 核心基类

```
AgentScope 智能体继承体系
│
├── AgentBase（所有智能体基类）
│   ├── reply(msg)           → 处理消息并生成响应
│   ├── observe(msg)         → 接收消息但不返回响应
│   ├── print(msg)           → 输出消息到终端/Web
│   └── handle_interrupt()   → 处理用户中断
│
├── ReActAgentBase（ReAct 智能体基类）
│   ├── 继承 AgentBase 全部方法
│   ├── _reasoning()         → 推理阶段（LLM 生成工具调用）
│   └── _acting()            → 行动阶段（执行工具函数）
│
├── ReActAgent（开箱即用的 ReAct 智能体）
│   └── 继承 ReActAgentBase，提供完整实现
│
└── UserAgent（用户代理智能体）
    └── 从终端接收用户输入
```

#<!-- chunk: 6.2 AgentBase 三大核心函数 -->## 6.2 AgentBase 三大核心函数

```python
from agentscope.agent import AgentBase
from agentscope.message import Msg


class MyAgent(AgentBase):
    """自定义智能体示例"""

    async def reply(self, msg: Msg | list[Msg] | None) -> Msg:
        """
        核心函数：处理传入消息并生成响应。
        - 接收用户/其他智能体的消息
        - 执行推理和工具调用
        - 返回响应消息
        """
        pass

    async def observe(self, msg: Msg | list[Msg] | None) -> None:
        """
        观察函数：接收消息但不返回响应。
        - 用于旁听其他智能体的对话
        - 将消息存储在记忆中
        - 适合监控/日志类智能体
        """
        pass

    async def handle_interrupt(self) -> Msg:
        """
        中断处理：当用户中断智能体回复时调用。
        - 实时介入（Realtime Steering）的关键机制
        - 允许优雅地处理中断
        """
        pass
```

#<!-- chunk: 6.3 ReActAgent 完整参数 -->## 6.3 ReActAgent 完整参数

```python
from agentscope.agent import ReActAgent

agent = ReActAgent(
    # === 必需参数 ===
    name="K8s-Expert",                  # 智能体名称
    sys_prompt="你是一个 Kubernetes 运维专家...",  # 系统提示
    model=model,                         # LLM 模型实例
    formatter=formatter,                 # 提示词格式化器

    # === 可选参数 ===
    toolkit=toolkit,                     # 工具模块
    memory=InMemoryMemory(),             # 短期记忆
    long_term_memory=None,               # 长期记忆
    long_term_memory_mode="agent_control",  # 长期记忆管理模式
    # "agent_control": 智能体自主管理
    # "static_control": 开发者管理
    # "both": 两者同时激活

    enable_meta_tool=False,              # 是否允许智能体自主管理工具
    parallel_tool_calls=True,            # 是否允许并行工具调用
    max_iters=10,                        # 最大迭代次数
    plan_notebook=None,                  # 计划模块
    print_hint_msg=True,                 # 是否打印提示消息
)
```

#<!-- chunk: 6.4 从零自定义 Agent -->## 6.4 从零自定义 Agent

```python
from agentscope.agent import AgentBase
from agentscope.model import DashScopeChatModel
from agentscope.formatter import DashScopeChatFormatter
from agentscope.memory import InMemoryMemory
from agentscope.message import Msg
import os


class K8sDiagnosisAgent(AgentBase):
    """K8s 诊断智能体 — 从 AgentBase 自定义"""

    def __init__(self) -> None:
        super().__init__()
        self.name = "K8s-Doctor"
        self.sys_prompt = """你是一个 Kubernetes 生产运维诊断专家。
诊断原则:
1. 先收集信息再下结论
2. 给出根因分析 + 修复步骤 + 验证方法
3. 对破坏性操作给出风险提示
4. 所有结论必须基于工具获取的实际数据"""

        self.model = DashScopeChatModel(
            model_name="qwen-max",
            api_key=os.environ["DASHSCOPE_API_KEY"],
            stream=False,
        )
        self.formatter = DashScopeChatFormatter()
        self.memory = InMemoryMemory()

    async def reply(self, msg: Msg | list[Msg] | None) -> Msg:
        """处理诊断请求"""
        # 存储输入消息到记忆
        await self.memory.add(msg)

        # 构建提示词
        prompt = await self.formatter.format(
            [
                Msg("system", self.sys_prompt, "system"),
                *await self.memory.get_memory(),
            ],
        )

        # 调用模型
        response = await self.model(prompt)

        # 创建响应消息
        reply_msg = Msg(
            name=self.name,
            content=response.content,
            role="assistant",
        )

        # 存储响应到记忆
        await self.memory.add(reply_msg)

        # 打印消息
        await self.print(reply_msg)

        return reply_msg

    async def observe(self, msg: Msg | list[Msg] | None) -> None:
        """观察消息（旁听模式）"""
        await self.memory.add(msg)

    async def handle_interrupt(self) -> Msg:
        """处理中断"""
        return Msg(
            name=self.name,
            content="诊断已中断。如需继续，请重新描述问题。",
            role="assistant",
        )
```

---

<!-- chunk: 7. Memory — 记忆基础 -->## 7. Memory — 记忆基础

#<!-- chunk: 7.1 内置记忆类型 -->## 7.1 内置记忆类型

AgentScope 提供三种记忆存储实现：

| 类 | 存储方式 | 适用场景 |
|----|---------|----------|
| `InMemoryMemory` | 内存 | 开发调试、短会话 |
| `AsyncSQLAlchemyMemory` | 关系数据库（SQLite/PostgreSQL/MySQL） | 生产环境，支持连接池 |
| `RedisMemory` | Redis | 高性能分布式场景 |

#<!-- chunk: 7.2 InMemoryMemory 基础使用 -->## 7.2 InMemoryMemory 基础使用

```python
from agentscope.memory import InMemoryMemory
from agentscope.message import Msg

memory = InMemoryMemory()

# 添加消息
await memory.add(Msg("user", "Pod 处于 Pending 状态", "user"))
await memory.add(Msg("assistant", "我来帮你诊断...", "assistant"))

# 添加带标记(mark)的消息
await memory.add(
    Msg("system", "<system-hint>先检查资源配额</system-hint>", "system"),
    marks="hint",  # 标记为 hint 类型
)

# 按标记检索消息
hint_msgs = await memory.get_memory(mark="hint")

# 按标记删除消息
deleted = await memory.delete_by_mark("hint")

# 获取所有记忆
messages = await memory.get_memory()
```

#<!-- chunk: 7.3 Mark（标记）系统 -->## 7.3 Mark（标记）系统

**标记**是 AgentScope 记忆管理的重要特性，用于对消息进行分类、过滤和检索：

```
Mark 标记系统
│
├── 消息分类    → 区分提示消息、工具结果、用户对话
├── 选择性检索  → get_memory(mark="hint") 只获取特定类型
├── 批量管理    → delete_by_mark("hint") 清理一次性提示
└── 内部使用    → ReActAgent 用 "hint" 标记管理一次性提示
```

#<!-- chunk: 7.4 状态管理 -->## 7.4 状态管理

```python
memory = InMemoryMemory()
await memory.add(Msg("user", "hello", "user"))

# 导出状态（state_dict 是同步方法）
state = memory.state_dict()
# state 包含 _compressed_summary 和所有消息（含标记）

# 恢复状态
new_memory = InMemoryMemory()
new_memory.load_state_dict(state)
```

> **注意**：InMemoryMemory 在进程退出后数据丢失。生产环境应使用 `AsyncSQLAlchemyMemory`（关系数据库）或 `RedisMemory`（Redis），详见 [19 - 记忆管理与上下文工程](./19-agentscope-memory-context.md)。

---

<!-- chunk: 8. 完整示例：K8s 问答 Agent -->## 8. 完整示例：K8s 问答 Agent

将上述概念组合，构建一个完整的 K8s 知识问答 Agent：

```python
import asyncio
import os

from agentscope.agent import ReActAgent
from agentscope.model import DashScopeChatModel
from agentscope.formatter import DashScopeChatFormatter
from agentscope.memory import InMemoryMemory
from agentscope.message import Msg


async def k8s_qa_agent():
    """K8s 知识问答 Agent"""

    agent = ReActAgent(
        name="K8s-Expert",
        sys_prompt="""你是一个资深 Kubernetes 运维专家，拥有以下专长:
- 集群架构设计与优化
- 故障诊断与排查
- 性能调优
- 安全最佳实践

回答问题时:
1. 给出清晰的结构化回答
2. 包含具体的命令和配置示例
3. 说明潜在风险和注意事项""",
        model=DashScopeChatModel(
            model_name="qwen-max",
            api_key=os.environ["DASHSCOPE_API_KEY"],
            stream=True,
        ),
        memory=InMemoryMemory(),
        formatter=DashScopeChatFormatter(),
        max_iters=5,
    )

    # 模拟多轮对话
    questions = [
        "Pod 一直处于 Pending 状态，可能是什么原因？",
        "如果是资源不足导致的，如何快速解决？",
        "如何配置 HPA 来自动处理这类问题？",
    ]

    for q in questions:
        print(f"\n{'='*60}")
        print(f"用户: {q}")
        print(f"{'='*60}")

        msg = Msg(name="user", content=q, role="user")
        response = await agent(msg)

        print(f"\nAgent: {response.get_text_content()}")


asyncio.run(k8s_qa_agent())
```

---

<!-- chunk: 9. 最佳实践 -->## 9. 最佳实践

#<!-- chunk: 设计原则 -->## 设计原则

- **系统提示要具体**：sys_prompt 中明确智能体的专长、行为边界和输出格式要求
- **Formatter 与 Model 严格匹配**：这是 AgentScope 中最常见的配置错误
- **利用 observe 实现旁听**：监控类智能体用 observe 而非 reply，避免不必要的响应
- **状态管理贯穿始终**：利用 state_dict/load_state_dict 实现会话恢复和智能体迁移
- **max_iters 设置合理上限**：防止推理循环，建议 5-15 次

#<!-- chunk: 命名规范 -->## 命名规范

```python
# 推荐: 清晰的角色命名
agent = ReActAgent(name="K8s-Diagnosis-Expert", ...)
agent = ReActAgent(name="Network-Analyzer", ...)
agent = ReActAgent(name="Cost-Advisor", ...)

# 避免: 模糊的命名
agent = ReActAgent(name="agent1", ...)
agent = ReActAgent(name="bot", ...)
```

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [16 - 概述与安装](./16-agentscope-overview-installation.md) | 安装配置与 Hello World |
| [18 - 工具系统与 MCP](./18-agentscope-tool-system.md) | Tool 详解与 MCP 集成 |
| [19 - 记忆管理](./19-agentscope-memory-context.md) | Memory 深度使用与生产方案 |
| [01 - Agent 基础](./01-ai-agent-fundamentals.md) | 通用 Agent 概念与推理框架 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent KUDIG Database — Global MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README.md|[[AI Agent 工程专题|AI Agent 工程专题]]]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals.md|[[AI Agent 基础与核心架构|AI Agent 基础与核心架构]]]]
- [[domain-14-ai-ml-infra/topic-ai-agent/02-llm-foundation-models.md|[[LLM 基座模型选型与评估|LLM 基座模型选型与评估]]]]
- [[domain-14-ai-ml-infra/topic-ai-agent/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 15-agent-corpus-gap-analysis
- 16-agentscope-overview-installation
- 18-agentscope-tool-system
- 19-agentscope-memory-context
