---
title: AgentScope 记忆管理与上下文工程 (domain-14-ai-ml-infra)
description: 'title: AgentScope 记忆管理与上下文工程'
summary: 'title: AgentScope 记忆管理与上下文工程'
category: general
tags:
- ai
- ai-agent
- etcd
- redis
- mysql
- postgresql
- gateway
- llm
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- AgentScope 记忆管理与上下文工程 是什么
- 如何 AgentScope 记忆管理与上下文工程
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- AgentScope
- 记忆管理与上下文工程
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- etcd-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: AgentScope 记忆管理与上下文工程
description: '# AgentScope 记忆管理与上下文工程'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[etcd|etcd]]
- redis
- mysql
- postgresql
- gateway
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- AgentScope 记忆管理与上下文工程 是什么
- 如何 AgentScope 记忆管理与上下文工程
trigger_keywords:
- AgentScope
- 记忆管理与上下文工程
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

# AgentScope 记忆管理与上下文工程

> **文档类型**: 记忆管理专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, Memory, 记忆管理, InMemoryMemory, AsyncSQLAlchemyMemory, RedisMemory, 长期记忆, Mem0, ReMe, Session, JSONSession, 状态持久化, Token 管理, 上下文窗口, 记忆压缩, CompressionConfig, marks

---

<!-- chunk: 概述 -->## 概述

记忆是 Agent 实现**多轮对话连贯性**和**跨会话知识积累**的基础。AgentScope 提供了灵活的记忆管理体系：三种内置记忆后端（InMemoryMemory、AsyncSQLAlchemyMemory、RedisMemory）用于当前会话的对话历史，长期记忆（Mem0、ReMe）用于跨会话的知识积累，JSONSession 用于生产环境的状态持久化。

本文系统讲解 AgentScope 记忆管理的完整方案，从基础的 InMemoryMemory 到生产级的持久化和长期记忆。

---

<!-- chunk: 1. 记忆架构全景 -->## 1. 记忆架构全景

```
AgentScope 记忆架构
│
├── 短期记忆 (Short-term Memory)
│   ├── InMemoryMemory       → 纯内存，进程退出后丢失，开发调试用
│   ├── AsyncSQLAlchemyMemory→ SQL 持久化（SQLite/PostgreSQL/MySQL）
│   └── RedisMemory          → Redis 持久化，分布式场景
│
├── 长期记忆 (Long-term Memory)
│   ├── Mem0LongTermMemory
│   │   └── 基于 Mem0 的向量检索长期记忆
│   └── ReMePersonalLongTermMemory
│       └── 基于 ReMe 的个人化长期记忆
│
│   模式:
│   ├── agent_control  → 智能体通过工具自主管理
│   ├── static_control → 框架在 reply 前后自动读写
│   └── both           → 同时激活以上两种
│
├── 消息标记 (Marks)
│   └── 字符串标签系统，用于消息分类/过滤/删除
│
├── 记忆压缩 (CompressionConfig)
│   └── 内置于 ReActAgent，自动 LLM 摘要压缩
│
├── Session 管理
│   └── JSONSession → 文件持久化
│
└── 状态管理 (State)
    ├── state_dict()        → 导出状态快照（同步）
    └── load_state_dict()   → 恢复状态（同步）
```

---

<!-- chunk: 2. 三种记忆后端 -->## 2. 三种记忆后端

AgentScope 提供三种内置记忆实现，均实现相同的 `Memory` 接口：

| 记忆类型 | 存储后端 | 持久化 | 适用场景 |
|---------|---------|--------|--------|
| `InMemoryMemory` | Python 内存 | 否（进程退出后丢失） | 开发调试、短对话 |
| `AsyncSQLAlchemyMemory` | SQLite/PostgreSQL/MySQL | 是 | 生产环境单机/单数据库 |
| `RedisMemory` | Redis | 是 | 生产环境分布式、高性能 |

## 2.1 InMemoryMemory——基础使用

```python
from agentscope.memory import InMemoryMemory
from agentscope.message import Msg

# 创建记忆实例
memory = InMemoryMemory()

# 添加消息
await memory.add(Msg("user", "Pod 处于 CrashLoopBackOff", "user"))
await memory.add(Msg("assistant", "我来检查日志...", "assistant"))
await memory.add(Msg("user", "容器启动失败是什么原因？", "user"))

# 获取全部记忆
messages = await memory.get_memory()
# 返回: [Msg("user", ...), Msg("assistant", ...), Msg("user", ...)]

# 获取记忆数量
count = len(messages)
```

## 2.2 AsyncSQLAlchemyMemory——SQL 持久化

```python
from agentscope.memory import AsyncSQLAlchemyMemory

# SQLite 后端（单机，零配置）
memory = AsyncSQLAlchemyMemory(
    url="sqlite+aiosqlite:///./agent_memory.db",
)

# PostgreSQL 后端（生产推荐）
memory = AsyncSQLAlchemyMemory(
    url="postgresql+asyncpg://user:pass@db-host:5432/agent_db",
    # 连接池配置（生产环境必须）
    pool_size=10,
    max_overflow=20,
)

# 使用方式与 InMemoryMemory 完全相同
await memory.add(Msg("user", "Pod Pending 怎么办？", "user"))
messages = await memory.get_memory()
```

> **优势**：进程重启后记忆不丢失；支持连接池；适合 FastAPI 等 Web 服务。

## 2.3 RedisMemory——分布式

```python
from agentscope.memory import RedisMemory

# 适合 K8s 多副本场景，多个 Agent 实例共享状态
memory = RedisMemory(
    url="redis://redis-host:6379/0",
)

# 使用方式与 InMemoryMemory 完全相同
await memory.add(Msg("user", "etcd leader 频繁切换", "user"))
```

## 2.4 在 Agent 中使用

```python
from agentscope.agent import ReActAgent
from agentscope.memory import InMemoryMemory

agent = ReActAgent(
    name="Expert",
    memory=InMemoryMemory(),  # 注入短期记忆
    ...
)

# Agent 自动管理记忆:
# 1. 收到消息 → memory.add(input_msg)
# 2. 生成响应 → memory.add(response_msg)
# 3. 下次推理时 → memory.get_memory() 获取历史作为上下文
```

## 2.5 消息标记系统（Marks）

AgentScope 的记忆支持 **marks**（字符串标签），用于消息的分类、过滤和批量删除：

```python
from agentscope.memory import InMemoryMemory
from agentscope.message import Msg

memory = InMemoryMemory()

# 添加带标记的消息
await memory.add(Msg("user", "集群状态如何？", "user"), marks="diagnosis")
await memory.add(Msg("system", "提示：检查日志", "system"), marks="hint")
await memory.add(Msg("assistant", "已完成检查", "assistant"), marks="diagnosis")

# 按标记检索消息
diag_msgs = await memory.get_memory(marks="diagnosis")
# 返回: [用户问题, 助手回复]

# 按标记删除消息
await memory.delete(marks="hint")
# 所有带 "hint" 标记的消息被删除
```

```
Marks 常见用法
│
├── "hint"         → 临时提示信息，用完即删
├── "diagnosis"    → 诊断过程消息，可按任务检索
├── "tool_result"  → 工具执行结果，压缩时可优先删除
└── "summary"      → 压缩生成的摘要消息
```

## 2.6 状态管理

```python
memory = InMemoryMemory()
await memory.add(Msg("user", "hello", "user"))

# 导出状态（同步 API，可序列化为 JSON）
state = memory.state_dict()
# state 包含所有存储的消息

# 创建新实例并恢复状态
new_memory = InMemoryMemory()
new_memory.load_state_dict(state)

# new_memory 现在拥有相同的对话历史
messages = await new_memory.get_memory()
```

---

<!-- chunk: 3. 长期记忆 -->## 3. 长期记忆

## 3.1 设计理念

AgentScope 不严格区分短期和长期记忆的作用——一切以**需求驱动**。长期记忆提供两种实现和三种运行模式：

**实现方案**：

| 实现 | 说明 | 适用场景 |
|------|------|--------|
| `Mem0LongTermMemory` | 基于 Mem0 的向量检索长期记忆 | 通用知识积累、事实检索 |
| `ReMePersonalLongTermMemory` | 基于 ReMe 的个人化长期记忆 | 用户偶好、个性化服务 |

**运行模式**：

| 模式 | 管理者 | 适用场景 |
|------|--------|--------|
| `agent_control` | 智能体自主决定何时读写 | 复杂推理场景，Agent 按需检索 |
| `static_control` | 框架在 reply 前后自动读写 | 简单场景，自动化知识增强 |
| `both` | 两者同时激活 | 最大灵活性 |

## 3.2 Mem0LongTermMemory

```python
from agentscope.memory import Mem0LongTermMemory
from agentscope.agent import ReActAgent

# 创建 Mem0 长期记忆
long_term = Mem0LongTermMemory(
    user_id="ops-engineer-001",
    # Mem0 配置（向量存储、LLM 提取等）
    mem0_config={
        "llm": {
            "provider": "openai",
            "config": {"model": "gpt-4o-mini"},
        },
    },
)

agent = ReActAgent(
    name="K8s-Expert",
    long_term_memory=long_term,
    long_term_memory_mode="agent_control",
    ...
)
```

## 3.3 agent_control 模式

智能体通过工具函数自主管理长期记忆——决定何时保存重要信息、何时检索历史知识。

```python
from agentscope.agent import ReActAgent

agent = ReActAgent(
    name="K8s-Expert",
    long_term_memory=long_term_memory_instance,
    long_term_memory_mode="agent_control",
    # 智能体会自动获得记忆管理相关的工具:
    # - 保存信息到长期记忆
    # - 从长期记忆中检索信息
    ...
)
```

**工作流程**：

```
用户: "之前你帮我诊断过 etcd 的问题，当时是什么原因？"
                    │
Agent 推理: 需要检索长期记忆中关于 etcd 诊断的历史
                    │
Agent 调用: recall_from_long_term_memory("etcd 诊断")
                    │
长期记忆返回: "2026-03-10 诊断: etcd 集群 leader 频繁切换，
              根因是磁盘 IOPS 不足，建议使用 SSD"
                    │
Agent 回复: "上次 etcd 的问题是磁盘 IOPS 不足导致 leader 频繁切换..."
```

## 3.4 static_control 模式

框架在每次 `reply` 调用的开始/结束时自动处理长期记忆：

```
static_control 工作流程:
│
├── reply 开始前
│   └── 自动从长期记忆中检索与当前消息相关的历史信息
│       → 注入到 Agent 的上下文中
│
├── Agent 推理和行动
│
└── reply 结束后
    └── 自动将本次对话的关键信息保存到长期记忆
```

```python
agent = ReActAgent(
    name="K8s-Expert",
    long_term_memory=long_term_memory_instance,
    long_term_memory_mode="static_control",
    ...
)
```

---

<!-- chunk: 4. 记忆压缩 -->## 4. 记忆压缩

## 4.1 为什么需要压缩

随着对话增长，记忆内容膨胀会导致：

```
记忆膨胀问题
│
├── 1. Token 超限    → 超出模型上下文窗口
├── 2. 成本增加      → 每次 LLM 调用消耗更多 Token
├── 3. 推理质量下降  → 过多无关信息干扰推理
└── 4. 延迟增加      → 更长的 prompt 导致更慢的响应
```

## 4.2 AgentScope 内置 CompressionConfig

AgentScope 的 `ReActAgent` 内置了记忆压缩功能，通过 `CompressionConfig` 配置：

```python
from agentscope.agent import ReActAgent
from agentscope.memory import InMemoryMemory, CompressionConfig

agent = ReActAgent(
    name="K8s-Expert",
    memory=InMemoryMemory(),
    compression_config=CompressionConfig(
        trigger_threshold=50,    # 消息数超过 50 时触发压缩
        keep_recent=10,          # 保留最近 10 条原始消息
        # 可自定义摘要 Schema（可选）
        # summary_schema=MySummarySchema,
    ),
    ...
)
```

**压缩流程**：

```
CompressionConfig 工作流程
│
├── 1. 检测触发条件
│      当前消息数 > trigger_threshold (50)
│
├── 2. 分离消息
│      旧消息 = messages[:-keep_recent]  → 待压缩
│      新消息 = messages[-keep_recent:]   → 保留
│
├── 3. LLM 摘要
│      将旧消息通过 LLM 压缩为简洁摘要
│
└── 4. 替换记忆
       记忆 = [摘要消息] + 新消息
```

**压缩策略**：

```
记忆压缩策略
│
├── 窗口截断（最简单）
│   保留系统消息 + 最近 N 条消息
│   优点: 简单快速，零成本
│   缺点: 丢失早期上下文
│
├── LLM 摘要压缩（CompressionConfig）
│   将旧消息用 LLM 压缩为摘要
│   优点: 保留关键信息
│   缺点: 额外 LLM 调用成本
│
└── 混合策略（推荐）
    保留系统消息 + 旧消息摘要 + 最近 N 条原始消息
    优点: 平衡信息保留和 Token 消耗
```

> **注意**：`CompressionConfig` 是 AgentScope 内置的压缩方案，无需自定义压缩类。如果需要更精细的控制，可通过 `summary_schema` 参数自定义摘要格式。

## 4.3 手动实现压缩策略

```python
from agentscope.message import Msg


class CompressedMemory:
    """带压缩功能的记忆管理器"""

    def __init__(
        self,
        model,
        max_messages: int = 50,
        keep_recent: int = 10,
    ):
        self.model = model
        self.max_messages = max_messages
        self.keep_recent = keep_recent
        self.messages: list[Msg] = []
        self.summary: str = ""

    async def add(self, msg: Msg) -> None:
        self.messages.append(msg)

        # 超过阈值时触发压缩
        if len(self.messages) > self.max_messages:
            await self._compress()

    async def _compress(self) -> None:
        """将旧消息压缩为摘要"""
        old_messages = self.messages[:-self.keep_recent]
        recent_messages = self.messages[-self.keep_recent:]

        # 用 LLM 生成摘要
        compress_prompt = f"""请将以下对话历史压缩为简洁的摘要，保留关键信息和决策:

{self._format_messages(old_messages)}

当前已有摘要: {self.summary}

请输出更新后的摘要（200字以内）:"""

        response = await self.model(compress_prompt)
        self.summary = response.content

        # 保留最近消息
        self.messages = recent_messages

    async def get_context(self) -> list[Msg]:
        """获取完整上下文（摘要 + 最近消息）"""
        context = []
        if self.summary:
            context.append(Msg(
                "system",
                f"[对话历史摘要]: {self.summary}",
                "system",
            ))
        context.extend(self.messages)
        return context

    def _format_messages(self, messages: list[Msg]) -> str:
        return "\n".join(
            f"{m.name}: {m.content}" for m in messages
        )
```

---

<!-- chunk: 5. Session 管理 -->## 5. Session 管理

## 5.1 为什么需要 Session

```
无 Session（开发阶段）:
  Agent 启动 → 对话 → 进程退出 → 记忆全部丢失

有 Session（生产环境）:
  Agent 启动 → 对话 → 状态自动保存
       ↓
  Agent 重启 → 从持久化存储恢复状态 → 继续对话
```

## 5.2 JSONSession（文件持久化）

AgentScope 提供 `JSONSession` 作为内置 Session 方案，基于文件系统持久化：

```python
from agentscope.session import JSONSession
from agentscope.agent import ReActAgent

# 创建文件持久化 Session
session = JSONSession(save_dir="./agent_sessions")

# 保存 Agent 状态
session.save_session_state(
    session_id="session-001",
    user_id="user-alice",
    agent=agent,
)

# 恢复 Agent 状态
session.load_session_state(
    session_id="session-001",
    user_id="user-alice",
    agent=agent,
)
```

## 5.3 生产环境 Session 选型

对于生产环境的分布式部署，推荐结合 `AsyncSQLAlchemyMemory` 作为记忆后端 + `JSONSession` 作为状态持久化：

| 组件 | 开发环境 | 生产环境（单机） | 生产环境（分布式） |
|------|---------|-------------|---------------|
| **记忆** | InMemoryMemory | AsyncSQLAlchemyMemory | RedisMemory |
| **Session** | 无需 | JSONSession | JSONSession + 共享存储 |
| **长期记忆** | 无需 | Mem0LongTermMemory | Mem0LongTermMemory |

---

<!-- chunk: 6. Token 管理与上下文窗口 -->## 6. Token 管理与上下文窗口

## 6.1 Token 计算

AgentScope 提供 Token 计算工具，用于监控和管理上下文窗口使用：

```python
from agentscope.token import count_tokens

# 计算消息的 Token 数
token_count = count_tokens(
    messages=[
        {"role": "system", "content": "你是 K8s 专家"},
        {"role": "user", "content": "Pod Pending 怎么办？"},
    ],
    model_name="qwen-max",
)
print(f"当前上下文: {token_count} tokens")
```

## 6.2 上下文窗口管理策略

```
上下文窗口管理
│
├── 主流模型上下文窗口
│   ├── qwen-max          128K tokens
│   ├── qwen-plus          32K tokens
│   ├── qwen-turbo        128K tokens
│   ├── gpt-4o            128K tokens
│   ├── gpt-4o-mini       128K tokens
│   └── claude-3.5-sonnet  200K tokens
│
├── 窗口分配建议
│   ├── 系统提示:     5-10%
│   ├── 历史摘要:     10-20%
│   ├── 最近对话:     30-40%
│   ├── 工具描述:     10-15%
│   └── 输出预留:     20-30%
│
└── 超限处理
    ├── 自动截断最旧消息
    ├── 触发记忆压缩
    └── 降级到更短的 prompt
```

## 6.3 实践：上下文窗口管理器

```python
class ContextWindowManager:
    """上下文窗口管理器"""

    def __init__(
        self,
        model_name: str = "qwen-max",
        max_tokens: int = 128000,
        output_reserve_ratio: float = 0.25,
    ):
        self.model_name = model_name
        self.max_tokens = max_tokens
        self.output_reserve = int(max_tokens * output_reserve_ratio)
        self.available_tokens = max_tokens - self.output_reserve

    def should_compress(self, messages: list[dict]) -> bool:
        """判断是否需要压缩"""
        current = count_tokens(messages, self.model_name)
        return current > self.available_tokens * 0.8  # 80% 阈值

    def trim_messages(self, messages: list[dict]) -> list[dict]:
        """截断超限消息"""
        # 保留系统消息
        system_msgs = [m for m in messages if m.get("role") == "system"]
        other_msgs = [m for m in messages if m.get("role") != "system"]

        system_tokens = count_tokens(system_msgs, self.model_name)
        budget = self.available_tokens - system_tokens

        # 从最近的消息开始保留
        kept = []
        used = 0
        for msg in reversed(other_msgs):
            msg_tokens = count_tokens([msg], self.model_name)
            if used + msg_tokens <= budget:
                kept.insert(0, msg)
                used += msg_tokens
            else:
                break

        return system_msgs + kept
```

---

<!-- chunk: 7. 状态持久化深度解析 -->## 7. 状态持久化深度解析

## 7.1 AgentScope 的嵌套式状态管理

```
Agent.state_dict()
│
├── agent 自身状态
│   ├── name
│   └── 其他自定义字段
│
├── memory.state_dict()
│   └── 所有存储的消息
│
├── toolkit.state_dict()
│   └── 已注册工具列表及其状态
│
└── long_term_memory.state_dict()
    └── 长期记忆内容和索引
```

## 7.2 完整的状态管理流程

```python
import json
from agentscope.agent import ReActAgent


async def save_agent_state(agent: ReActAgent, filepath: str) -> None:
    """保存 Agent 完整状态到文件"""
    state = agent.state_dict()  # 同步 API
    with open(filepath, "w") as f:
        json.dump(state, f, ensure_ascii=False, indent=2)
    print(f"Agent 状态已保存到 {filepath}")


async def load_agent_state(agent: ReActAgent, filepath: str) -> None:
    """从文件恢复 Agent 状态"""
    with open(filepath, "r") as f:
        state = json.load(f)
    agent.load_state_dict(state)  # 同步 API
    print(f"Agent 状态已从 {filepath} 恢复")


# 使用示例
agent = ReActAgent(name="K8s-Expert", ...)

# ... 对话若干轮 ...

# 保存状态
await save_agent_state(agent, "/tmp/agent_state.json")

# 创建新 Agent 并恢复状态
new_agent = ReActAgent(name="K8s-Expert", ...)
await load_agent_state(new_agent, "/tmp/agent_state.json")

# new_agent 现在拥有之前的对话记忆和工具状态
```

> **注意**：`state_dict()` 和 `load_state_dict()` 是**同步** API，不需要 `await`。这与 Memory 的 `add()`、`get_memory()` 等异步方法不同。

---

<!-- chunk: 8. 生产环境记忆架构设计 -->## 8. 生产环境记忆架构设计

## 8.1 推荐架构

```
生产环境记忆架构
│
├── 请求进入
│   └── API Gateway → AgentApp
│
├── 状态恢复
│   └── JSONSession.load_session_state(session_id, user_id, agent)
│
├── Agent 处理
│   ├── 短期记忆 (AsyncSQLAlchemyMemory) — 当前会话
│   └── 长期记忆 (Mem0LongTermMemory) — 跨会话知识
│
├── 状态保存
│   └── JSONSession.save_session_state(session_id, user_id, agent)
│
└── 存储层
    ├── PostgreSQL/Redis（记忆持久化）
    │   ├── AsyncSQLAlchemyMemory 连接池
    │   └── 支持多副本共享
    └── 向量数据库（长期记忆检索）
        ├── 语义相似度检索
        └── 知识图谱索引
```

## 8.2 FastAPI + AsyncSQLAlchemyMemory 生产示例

```python
from contextlib import asynccontextmanager
from fastapi import FastAPI
from agentscope.agent import ReActAgent
from agentscope.memory import AsyncSQLAlchemyMemory, CompressionConfig
from agentscope.session import JSONSession
import os


@asynccontextmanager
async def lifespan(app: FastAPI):
    # 启动：初始化连接池
    app.state.session = JSONSession(save_dir="./sessions")
    print("Agent 服务启动")
    yield
    print("Agent 服务关闭")


app = FastAPI(lifespan=lifespan)


@app.post("/chat")
async def chat(session_id: str, user_id: str, message: str):
    # 使用 AsyncSQLAlchemyMemory 作为持久化记忆
    memory = AsyncSQLAlchemyMemory(
        url=os.getenv("DB_URL", "sqlite+aiosqlite:///./memory.db"),
        pool_size=10,
    )

    agent = ReActAgent(
        name="K8s-Expert",
        memory=memory,
        compression_config=CompressionConfig(
            trigger_threshold=50,
            keep_recent=10,
        ),
        ...
    )

    # 恢复会话状态
    app.state.session.load_session_state(
        session_id=session_id,
        user_id=user_id,
        agent=agent,
    )

    # 处理请求
    msg = Msg("user", message, "user")
    response = await agent(msg)

    # 保存会话状态
    app.state.session.save_session_state(
        session_id=session_id,
        user_id=user_id,
        agent=agent,
    )

    return {"response": response.get_text_content()}
```

---

<!-- chunk: 9. 最佳实践与反模式 -->## 9. 最佳实践与反模式

## 最佳实践

- **开发用 InMemoryMemory，生产用 AsyncSQLAlchemyMemory/RedisMemory**：开发调试时 InMemoryMemory 足够，上线前切换到持久化记忆
- **使用 CompressionConfig**：长对话场景（>30 轮）必须启用内置压缩，无需自定义压缩类
- **善用 marks 标记系统**：对消息分类标记，方便按任务检索和压缩时优先删除
- **Agent 状态定期持久化**：在每次 reply 完成后保存状态，防止异常丢失
- **长期记忆用 agent_control**：让智能体自主决定何时保存/检索，比 static_control 更灵活

## 反模式

- **InMemoryMemory 用于生产**：进程重启后所有对话丢失——生产应用 AsyncSQLAlchemyMemory 或 RedisMemory
- **不管理上下文窗口**：随对话增长 Token 超限，LLM 返回截断或错误——用 CompressionConfig
- **每条消息都存长期记忆**：过多噪音降低检索质量
- **忽略 state_dict 的序列化**：包含不可序列化对象导致保存失败
- **state_dict/load_state_dict 使用 await**：这两个方法是同步 API，不需要 await

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [17 - 核心概念](./17-agentscope-core-concepts.md) | Memory 在核心抽象中的位置 |
| [20 - 多 Agent 编排](./20-agentscope-multi-agent-orchestration.md) | 多 Agent 场景的共享记忆 |
| [22 - 生产部署](./deployment.md|22-agentscope-production-deployment]].md) | Session + Runtime 的生产部署 |
| [07 - 记忆管理与上下文窗口](./07-memory-context-management.md) | 通用记忆管理理论与策略 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent KUDIG Database — Global MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README.md|[[AI Agent 工程专题|AI Agent 工程专题]]]]
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

## Related

- 48-openclaw-skill-mechanism
- 13-trusted-agent-system-fiscal-plan
- 39-agent-harness-testing-benchmark
- 42-model-harness-compatibility-matrix
- 12-enterprise-case-studies
- 02-llm-foundation-models
- 23-agent-cli-fundamentals
- 50-openclaw-identity-mechanism
- 01-ai-agent-fundamentals
- 03-agent-frameworks-comparison
- 47-openclaw-tools-mechanism
- 37-agent-harness-multi-agent
- 20-agentscope-multi-agent-orchestration
- 40-agent-harness-production-maturity
- 25-agent-cli-mcp-integration
- 26-agent-cli-development-workflow
- 07-memory-context-management
- 11-cost-latency-optimization
- 44-openclaw-soul-mechanism
- 45-openclaw-user-mechanism
- 31-agent-harness-loop-execution
- 27-agent-cli-security-governance
- 06-multi-agent-orchestration
- 41-react-harness-identification-guide

## See Also

- 17-agentscope-core-concepts
- 18-agentscope-tool-system
- 20-agentscope-multi-agent-orchestration
- 21-agentscope-advanced-features


<!-- risk-assessed -->
