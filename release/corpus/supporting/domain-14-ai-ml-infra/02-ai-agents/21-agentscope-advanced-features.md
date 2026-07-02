---
title: AgentScope 高级特性与扩展开发 (domain-14-ai-ml-infra)
description: 'description: ''**文档类型**: 高级特性专题 | **最后更新**: 2026-03 | **关键词**: AgentScope,
  Hooks, Middleware,'
summary: 'description: ''**文档类型**: 高级特性专题 | **最后更新**: 2026-03 | **关键词**: AgentScope,
  Hooks, Middleware,'
category: general
tags:
- ai
- ai-agent
- kubelet
- jaeger
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
estimated_read_time: 15min
intent_queries:
- AgentScope 高级特性与扩展开发 是什么
- 如何 AgentScope 高级特性与扩展开发
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- AgentScope
- 高级特性与扩展开发
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- tracing-basics
---



title: AgentScope 高级特性与扩展开发
description: '**文档类型**: 高级特性专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, Hooks, Middleware,
  RAG, A2A, Agent-to-Agent, 实时语音, Realtime Steering, 结构化输出, Agentic RL, 强化学习微调, 评测,
  ACEBench, Embedding'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[kubelet|kubelet]]
- [[Jaeger|jaeger]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- AgentScope 高级特性与扩展开发 是什么
- 如何 AgentScope 高级特性与扩展开发
trigger_keywords:
- AgentScope
- 高级特性与扩展开发
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

# AgentScope 高级特性与扩展开发

> **文档类型**: 高级特性专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, Hooks, Middleware, RAG, A2A, Agent-to-Agent, 实时语音, Realtime Steering, 结构化输出, Agentic RL, 强化学习微调, 评测, ACEBench, Embedding

---

<!-- chunk: 概述 -->## 概述

AgentScope 除了核心的 Agent/Tool/Memory/Pipeline 之外，还提供了丰富的高级特性：Agent Hooks 和 Middleware 实现行为增强，RAG 支持知识增强，A2A 协议实现跨框架 Agent 通信，实时语音支持语音交互，Agentic RL 支持强化学习微调，以及完整的评测体系。

本文系统讲解这些高级特性的设计原理、使用方法和生产实践。

---

<!-- chunk: 1. Agent Hooks — 钩子函数 -->## 1. Agent Hooks — 钩子函数

## 1.1 概念

Hooks 允许在 Agent 核心函数（reply、observe、print、_reasoning、_acting）的**前后**插入自定义逻辑，无需修改 Agent 源码。

```
Agent Hooks 执行流程
│
├── before_reply_hook(msg)      ← 消息预处理、日志记录
├── agent.reply(msg)            ← Agent 核心逻辑
│   ├── before_reasoning_hook() ← 推理前钩子（ReActAgent）
│   ├── agent._reasoning()      ← 推理
│   ├── after_reasoning_hook()  ← 推理后钩子
│   ├── before_acting_hook()    ← 行动前钩子
│   ├── agent._acting()         ← 行动（执行工具）
│   └── after_acting_hook()     ← 行动后钩子
├── after_reply_hook(response)  ← 响应后处理、指标采集
│
├── before_observe_hook(msg)
├── agent.observe(msg)
├── after_observe_hook()
│
├── before_print_hook(msg)
├── agent.print(msg)
└── after_print_hook()
```

## 1.2 Hook 注册 API

AgentScope 提供两种 Hook 注册方式：

| 注册方式 | 作用范围 | 用途 |
|---------|---------|------|
| `register_instance_hook(agent, hook_fn)` | 单个 Agent 实例 | 特定 Agent 的调试/监控 |
| `register_class_hook(AgentClass, hook_fn)` | 某类 Agent 的所有实例 | 全局级别的日志/审计 |

**统一的 Hook 签名**：

```python
from agentscope.agent import ReActAgent
from agentscope.hook import register_instance_hook, register_class_hook


# Hook 函数统一签名: async def hook(agent, *args)
async def log_before_reply(agent, msg):
    """reply 前的钩子：接收 agent 和输入消息"""
    print(f"[{agent.name}] 收到消息: {msg.get_text_content()[:80]}")


async def log_after_reply(agent, response):
    """响应后的钩子：接收 agent 和响应消息"""
    print(f"[{agent.name}] 响应长度: {len(response.get_text_content())}")


# 实例级别注册（仅影响单个 Agent）
agent = ReActAgent(name="K8s-Expert", ...)
register_instance_hook(agent, "before_reply", log_before_reply)
register_instance_hook(agent, "after_reply", log_after_reply)

# 类级别注册（影响 ReActAgent 的所有实例）
register_class_hook(ReActAgent, "before_reply", log_before_reply)
```

## 1.3 常用 Hook 场景

| Hook 位置 | 典型用途 |
|-----------|---------|
| `before_reply` | 输入验证、敏感信息脱敏、请求限流 |
| `after_reply` | 响应审计、延迟监控、Token 统计 |
| `before_reasoning` | 注入额外上下文（如当前时间、环境信息） |
| `after_reasoning` | 检查推理结果合理性 |
| `before_acting` | 工具调用权限检查、风险评估 |
| `after_acting` | 工具执行结果验证、错误处理增强 |
| `before_observe` | 消息过滤（屏蔽无关信息） |
| `before_print` | 输出格式化、多语言翻译 |

---

<!-- chunk: 3. Middleware — 中间件 -->## 3. Middleware — 中间件

> **重要**：AgentScope 的 Middleware 注册在 **Toolkit**（而非 Agent）上，采用洋葱模型。详细用法见 [18 - 工具系统第 9 节](./18-agentscope-tool-system.md)。

```
Hooks vs Middleware 职责划分
│
├── Hooks    → 作用于 Agent 级别
│   ├── register_instance_hook   → 单个 Agent
│   └── register_class_hook      → 某类 Agent 的所有实例
│   用途: 日志、审计、监控、上下文注入
│
└── Middleware → 作用于 Toolkit 级别
    └── toolkit.register_middleware(fn)
    用途: 权限控制、输出截断、结果转换
```

---

<!-- chunk: 2. RAG — 检索增强生成 -->## 2. RAG — 检索增强生成

## 2.1 AgentScope RAG 架构

AgentScope 的 RAG 模块采用 **Reader → Knowledge → Store** 三层架构：

```
AgentScope RAG 架构
│
├── Reader（文档读取器）
│   ├── TextReader       → 纯文本文件
│   ├── PDFReader        → PDF 文档
│   ├── ImageReader      → 图片文件（多模态）
│   └── 自定义 Reader    → 继承 ReaderBase 扩展
│
├── Knowledge（知识库管理）
│   └── SimpleKnowledge  → 包含 Reader + Store，统一管理
│       ├── load()       → 加载文档
│       ├── retrieve()   → 检索相关片段
│       └── delete()     → 删除文档
│
└── Store（向量存储）
    └── QdrantStore      → 基于 Qdrant 的向量存储
        ├── 云端: Qdrant Cloud
        └── 本地: Qdrant 容器
```

## 2.2 使用 AgentScope RAG

```python
from agentscope.rag import SimpleKnowledge, QdrantStore, TextReader
from agentscope.agent import ReActAgent


async def rag_agent_example():
    # 1. 创建知识库
    knowledge = SimpleKnowledge(
        name="k8s-troubleshooting",
        reader=TextReader(),           # 文本读取器
        store=QdrantStore(             # Qdrant 向量存储
            collection_name="k8s_docs",
            url="http://localhost:6333",  # 本地 Qdrant
        ),
        embedding_model=embedding_model,
    )

    # 2. 加载文档
    await knowledge.load(
        paths=["./domain-10-troubleshooting-diagnostics/"],
        file_types=[".md"],
    )

    # 3. 检索相关内容
    results = await knowledge.retrieve(
        query="Pod Pending 排查步骤",
        top_k=5,
    )
```

## 2.3 RAG 集成方式

AgentScope 支持两种 RAG 集成模式：

| 集成模式 | 说明 | 适用场景 |
|---------|------|--------|
| **Agentic 集成** | RAG 作为工具注册到 Toolkit，Agent 自主决定何时检索 | 复杂场景，Agent 需要判断是否需要检索 |
| **Generic 集成** | 在 Agent 的 sys_prompt 中自动注入检索结果 | 简单场景，每次都需要检索 |

```python
# Agentic 集成（推荐）：把检索函数作为工具注册
async def search_knowledge(query: str) -> str:
    """搜索 K8s 知识库。

    Args:
        query: 搜索关键词或问题描述

    Returns:
        相关知识片段
    """
    results = await knowledge.retrieve(query=query, top_k=5)
    return "\n\n".join(r.content for r in results)

toolkit = Toolkit()
toolkit.register_tool_function(search_knowledge)
```

## 2.4 RAG 最佳实践

| 环节 | 最佳实践 | 反模式 |
|------|---------|--------|
| 文档读取 | 使用对应 Reader（TextReader/PDFReader） | 统一用纯文本处理所有格式 |
| 分块 | 按语义段落切分，保持上下文完整 | 固定长度切分导致语义断裂 |
| 嵌入 | 使用多语言模型（BGE-M3） | 英文模型处理中文文档 |
| 检索 | Top-K=5 + Re-ranking | Top-K=1 导致信息不足 |
| 集成 | Agentic 集成（RAG 作为工具） | 每次都全量注入检索结果 |
| 注入 | 明确标注"来自知识库" | 直接拼接导致 LLM 混淆来源 |

---

<!-- chunk: 4. A2A 协议 — Agent-to-Agent -->## 4. A2A 协议 — Agent-to-Agent

## 4.1 什么是 A2A

A2A（Agent-to-Agent）是 Google 提出的开放协议，用于不同框架的 Agent 之间进行标准化通信。AgentScope 内置 A2A 支持。

```
A2A vs MCP 的区别
│
├── MCP（Model Context Protocol）
│   Agent ↔ 工具/数据
│   "Agent 如何调用外部工具"
│
└── A2A（Agent-to-Agent Protocol）
    Agent ↔ Agent
    "不同 Agent（甚至不同框架）如何协作"
```

## 4.2 A2A Agent 示例

```python
# AgentScope 的 A2A Agent 可以与其他框架的 Agent 通信
from agentscope.agent import ReActAgent

# 创建支持 A2A 协议的 Agent
a2a_agent = ReActAgent(
    name="K8s-Expert-A2A",
    sys_prompt="你是 K8s 运维专家，通过 A2A 协议接收和响应请求",
    ...
)

# 通过 AgentScope Runtime 部署后，
# 其他框架的 Agent 可以通过 A2A 协议与之通信
```

## 4.3 A2A 在生产中的价值

```
A2A 生产应用场景
│
├── 跨团队 Agent 协作
│   团队A 的 LangGraph Agent ←A2A→ 团队B 的 AgentScope Agent
│
├── 渐进式迁移
│   逐步将旧框架 Agent 替换为 AgentScope Agent
│   两者通过 A2A 无缝协作
│
└── 微服务化 Agent
    每个领域部署独立的 Agent 服务
    通过 A2A 协议组成 Agent 网格
```

---

<!-- chunk: 5. 实时语音 Agent -->## 5. 实时语音 Agent

## 5.1 语音 Agent 架构

```
语音 Agent 架构
│
├── 语音输入
│   ├── 麦克风采集
│   ├── ASR（语音识别）
│   └── 文本消息
│
├── Agent 处理
│   └── ReActAgent（标准推理和工具调用）
│
├── 语音输出
│   ├── TTS（文本转语音）
│   └── 音频播放
│
└── Web 界面
    └── 实时双向语音交互
```

## 5.2 创建语音 Agent

```python
from agentscope.agent import ReActAgent
from agentscope.tts import TTSModel

# 创建带 TTS 的 Agent
voice_agent = ReActAgent(
    name="Voice-Assistant",
    sys_prompt="你是一个语音助手，请用简洁自然的语言回答。",
    model=model,
    formatter=formatter,
    memory=memory,
    # TTS 配置
    tts_model=TTSModel(
        model_name="cosyvoice",  # 阿里 CosyVoice
        # 或其他 TTS 服务
    ),
)
```

---

<!-- chunk: 6. 实时介入（Realtime Steering） -->## 6. 实时介入（Realtime Steering）

## 6.1 概念

实时介入允许用户在 Agent 执行过程中**实时中断**并调整方向，AgentScope 通过 `handle_interrupt` 机制实现优雅中断：

```
实时介入流程
│
├── Agent 正在执行（推理 + 工具调用）
│
├── 用户发送中断信号
│
├── Agent 收到中断
│   ├── 暂停当前执行
│   ├── 保存已完成的状态
│   └── 调用 handle_interrupt()
│
└── 用户可以:
    ├── 修改指令，Agent 继续
    ├── 取消当前任务
    └── 切换到其他任务
```

## 6.2 自定义中断处理

```python
from agentscope.agent import AgentBase
from agentscope.message import Msg


class InterruptibleAgent(AgentBase):
    """支持优雅中断的 Agent"""

    async def handle_interrupt(self) -> Msg:
        """处理用户中断"""
        # 保存当前进度
        progress = await self._save_progress()

        return Msg(
            name=self.name,
            content=f"""执行已中断。

当前进度:
{progress}

您可以:
1. 输入新指令继续
2. 输入 "resume" 从断点恢复
3. 输入 "cancel" 取消任务""",
            role="assistant",
        )

    async def _save_progress(self) -> str:
        """保存中断时的进度"""
        state = await self.state_dict()
        # 持久化状态...
        return f"已完成 {len(state.get('tool_calls', []))} 次工具调用"
```

---

<!-- chunk: 7. 结构化输出 -->## 7. 结构化输出

## 7.1 让 Agent 输出结构化数据

```python
from agentscope.agent import ReActAgent

# 通过系统提示引导结构化输出
agent = ReActAgent(
    name="Structured-Expert",
    sys_prompt="""你是 K8s 诊断专家。

输出格式要求（严格 JSON）:
{
    "severity": "critical|high|medium|low",
    "root_cause": "问题根因描述",
    "affected_resources": ["受影响的资源列表"],
    "fix_steps": [
        {"step": 1, "action": "操作描述", "command": "具体命令", "risk": "风险等级"}
    ],
    "verification": "验证步骤",
    "rollback": "回滚方案"
}""",
    ...
)
```

---

<!-- chunk: 8. Tracing — 全链路追踪 -->## 8. Tracing — 全链路追踪

## 8.1 通过 agentscope.init 启用追踪

AgentScope 通过 `agentscope.init()` 统一初始化追踪：

```python
import agentscope

# 启用 AgentScope Studio 追踪
agentscope.init(
    studio_url="http://studio:3000",
    tracing_url="http://otel-collector:4317",
)
```

## 8.2 内置追踪装饰器

AgentScope 提供内置装饰器自动追踪关键操作：

| 装饰器 | 追踪内容 |
|---------|--------|
| `@trace_llm` | LLM 调用（Token、延迟、模型名） |
| `@trace_reply` | Agent reply 全过程 |
| `@trace_format` | Formatter 格式化过程 |
| `@trace` | 通用追踪装饰器 |

## 8.3 第三方 Tracing 集成

AgentScope 支持将追踪数据导出到多种后端：

| 后端 | 类型 | 说明 |
|------|------|------|
| AgentScope Studio | 内置 | 官方可视化工具，包含追踪 + 评测 |
| Alibaba Cloud CloudMonitor | 云服务 | 阿里云原生监控 |
| Arize-Phoenix | 开源 | 专注 LLM 可观测性 |
| Langfuse | 开源/云 | LLM 工程平台 |
| Jaeger / Zipkin | 开源 | 通用分布式追踪 |

---

<!-- chunk: 9. Agentic RL — 强化学习微调 -->## 9. Agentic RL — 强化学习微调

## 8.1 概念

AgentScope 内置 Agentic RL 支持，允许通过强化学习直接微调 Agent 的行为，而非仅优化 prompt：

```
Agentic RL 工作流
│
├── 1. Agent 执行任务
│      使用当前策略（LLM + prompt）
│
├── 2. 获取反馈
│      ├── 环境反馈（任务成功/失败）
│      ├── LLM-as-Judge 评估
│      └── 人工评分
│
├── 3. 策略更新
│      通过 RL 算法更新 LLM 权重
│
└── 4. 迭代优化
       重复 1-3，持续提升 Agent 能力
```

## 8.2 AgentScope Agentic RL 示例

AgentScope 提供了多个开箱即用的 Agentic RL 训练场景：

| 示例 | 描述 | 基础模型 | 训练效果 |
|------|------|---------|---------|
| Math Agent | 多步数学推理 | Qwen3-0.6B | 准确率 75% → 85% |
| Frozen Lake | 环境导航 | Qwen2.5-3B-Instruct | 成功率 15% → 86% |
| Learn to Ask | LLM-as-Judge 反馈 | Qwen2.5-7B-Instruct | 准确率 47% → 92% |
| Email Search | 工具使用优化 | Qwen3-4B-Instruct | 准确率 60% |
| Werewolf Game | 多 Agent 博弈 | Qwen2.5-7B-Instruct | 狼人胜率 50% → 80% |
| Data Augment | 合成数据增强 | Qwen3-0.6B | AIME-24 准确率 20% → 60% |

## 8.3 K8s 运维 Agent 微调思路

```
K8s 诊断 Agent 微调方案
│
├── 训练数据
│   ├── 历史故障诊断记录（问题→诊断步骤→根因）
│   ├── kudig-database 知识库（结构化 SOP）
│   └── 合成数据（问题模拟 + 诊断轨迹）
│
├── 奖励函数
│   ├── 诊断准确性（根因是否正确）
│   ├── 诊断效率（步骤数是否最优）
│   ├── 工具使用合理性（是否调用了正确的工具）
│   └── 安全性（是否避免了危险操作）
│
├── 基础模型
│   └── Qwen2.5-7B-Instruct（或更大模型）
│
└── 预期效果
    ├── 诊断准确率提升 20-30%
    ├── 平均诊断步骤减少 40%
    └── 误操作率降低到 <1%
```

---

<!-- chunk: 10. 评测体系 -->## 10. 评测体系

## 9.1 AgentScope 评测框架

AgentScope 提供了完整的 Agent 评测能力：

```
评测体系
│
├── ACEBench（内置基准测试）
│   ├── 工具使用准确性评估
│   ├── 多步推理能力评估
│   └── 标准化评分指标
│
├── OpenJudge（LLM-as-Judge）
│   ├── 使用 LLM 评估 Agent 输出质量
│   ├── 支持多维度评分
│   └── 可自定义评分标准
│
└── AgentScope Studio（可视化评测）
    ├── 评测结果可视化
    ├── Agent 轨迹回放
    └── 对比不同 Agent 版本
```

## 9.2 评测维度

| 维度 | 评估指标 | 方法 |
|------|---------|------|
| **准确性** | 诊断根因正确率 | Ground Truth 对比 |
| **效率** | 平均推理步骤数 | 轨迹分析 |
| **工具使用** | 工具选择准确率、调用成功率 | 工具调用日志分析 |
| **安全性** | 危险操作检测率 | 安全护栏命中率 |
| **一致性** | 同一问题多次回答一致性 | 多次采样对比 |
| **延迟** | 端到端响应时间 | P50/P95/P99 延迟 |
| **成本** | 每次诊断的 Token 消耗 | Token 计数 |

## 9.3 评测实践

```python
# 评测 K8s 诊断 Agent 的准确性
test_cases = [
    {
        "input": "Pod 处于 Pending 状态",
        "expected_root_cause": "资源不足或调度约束",
        "expected_tools": ["kubectl_get_pods", "kubectl_describe_resource"],
    },
    {
        "input": "Service 无法访问",
        "expected_root_cause": "Endpoint 为空或 Selector 不匹配",
        "expected_tools": ["kubectl_get_pods", "kubectl_describe_resource"],
    },
    {
        "input": "Node NotReady",
        "expected_root_cause": "kubelet 异常或资源压力",
        "expected_tools": ["kubectl_describe_resource", "kubectl_get_events"],
    },
]


async def evaluate_agent(agent, test_cases):
    """评测 Agent"""
    results = []
    for case in test_cases:
        msg = Msg("user", case["input"], "user")
        response = await agent(msg)

        # 使用 LLM-as-Judge 评分
        score = await llm_judge(
            question=case["input"],
            expected=case["expected_root_cause"],
            actual=response.get_text_content(),
        )
        results.append({
            "input": case["input"],
            "score": score,
            "response_length": len(response.get_text_content()),
        })

    avg_score = sum(r["score"] for r in results) / len(results)
    print(f"平均评分: {avg_score:.2f}")
    return results
```

---

<!-- chunk: 11. Embedding 模块 -->## 11. Embedding 模块

AgentScope 提供 Embedding 接口用于文本向量化，支持 RAG 和语义搜索：

```python
from agentscope.embedding import EmbeddingModel

# DashScope Embedding
embedding = EmbeddingModel(
    model_name="text-embedding-v3",
    api_key=os.environ["DASHSCOPE_API_KEY"],
)

# 生成向量
vector = await embedding.embed("Kubernetes Pod Pending 排查")
# vector: [0.023, -0.114, 0.089, ...]
```

---

<!-- chunk: 12. 最佳实践与反模式 -->## 12. 最佳实践与反模式

## 最佳实践

- **Hooks 用于 Agent 级别的横切关注点**：日志、监控、审计等通用逻辑用 Hooks，工具执行的横切逻辑用 Toolkit Middleware
- **`register_instance_hook` vs `register_class_hook`**：单个 Agent 调试用实例级别，全局日志用类级别
- **RAG 用 Agentic 集成**：RAG 作为工具之一注册到 Toolkit，让 Agent 自主决定何时检索
- **agentscope.init 启用追踪**：生产环境通过 `agentscope.init(studio_url=..., tracing_url=...)` 启用全链路追踪
- **A2A 实现松耦合**：不同团队的 Agent 通过 A2A 通信，避免框架耦合
- **评测驱动开发**：先定义评测指标，再优化 Agent
- **Agentic RL 从小模型开始**：先在 0.6B-3B 模型上验证方法，再扩展到大模型

## 反模式

- **过度使用 Hooks**：Hooks 链过长（>5 个）增加调试难度
- **混淆 Hooks 和 Middleware**：Agent 级逻辑用 Hooks，工具级逻辑用 Toolkit Middleware
- **RAG 全量注入**：检索结果不经筛选直接全部注入上下文
- **不启用 Tracing**：生产环境无追踪时，性能问题和错误几乎无法定位
- **忽视结构化输出验证**：Agent 输出的 JSON 不一定合法，需要验证和重试
- **Agentic RL 无安全约束**：微调后的模型可能产生更激进的操作策略
- **评测数据集过小**：少于 50 个测试用例的评测结果不具备统计意义

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [17 - 核心概念](./17-agentscope-core-concepts.md) | Agent 基类与扩展点 |
| [18 - 工具系统](./18-agentscope-tool-system.md) | MCP 集成与工具注册 |
| [20 - 多 Agent 编排](./20-agentscope-multi-agent-orchestration.md) | A2A 在多 Agent 场景的应用 |
| [22 - 生产部署](./deployment.md|22-agentscope-production-deployment]].md) | Runtime 部署与可观测性 |
| [04 - RAG 检索增强](./04-rag-knowledge-retrieval.md) | 通用 RAG 架构与策略 |
| [08 - 评测与可观测性](./08-agent-evaluation-observability.md) | 通用评测体系 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent MOC
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

- 19-agentscope-memory-context
- 20-agentscope-multi-agent-orchestration
- 22-agentscope-production-deployment
- 23-agent-cli-fundamentals
