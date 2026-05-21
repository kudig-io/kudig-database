---
title: AgentScope 概述与安装入门
description: 'title: AgentScope 概述与安装入门'
category: general
tags:
- ai
- ai-agent
- deep-dive
- configuration
- docker
- redis
- postgresql
- serverless
- llm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- AgentScope 概述与安装入门 是什么
- 如何 AgentScope 概述与安装入门
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- AgentScope
- 概述与安装入门
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- redis-basics
- observability-basics
---

title: AgentScope 概述与安装入门
description: '# AgentScope 概述与安装入门'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- docker
- redis
- postgresql
- serverless
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 10min
intent_queries:
- AgentScope 概述与安装入门 是什么
- 如何 AgentScope 概述与安装入门
trigger_keywords:
- AgentScope
- 概述与安装入门
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

# AgentScope 概述与安装入门

> **文档类型**: 框架入门专题 | **最后更新**: 2026-03 | **关键词**: AgentScope, 安装, 入门, ReAct Agent, 多 Agent 框架, 阿里巴巴, ModelScope, DashScope, 异步架构

---

<!-- chunk: 概述 -->## 概述

AgentScope 是阿里巴巴推出的**生产级、开发者友好**的多 Agent 框架，核心设计哲学是**面向日益增强的模型能力**——利用模型自身的推理和工具调用能力，而非用严格的提示词和固定编排来约束模型。

本文是 AgentScope 系列的第一篇，系统介绍框架定位、核心特性、安装配置，以及第一个 Hello World 示例，帮助读者在 5 分钟内完成环境搭建并运行首个 Agent。

> **官方资源**：
> - GitHub: https://github.com/agentscope-ai/agentscope
> - 文档: https://doc.agentscope.io/
> - Runtime: https://github.com/agentscope-ai/agentscope-runtime
> - Studio: https://github.com/agentscope-ai/agentscope-studio

---

<!-- chunk: 1. AgentScope 是什么 -->## 1. AgentScope 是什么

#<!-- chunk: 1.1 核心定位 -->## 1.1 核心定位

```
AgentScope 定位
│
├── 面向开发者的 Agent 框架（Developer-Centric）
│   不是低代码平台，强调代码控制力和灵活性
│
├── 生产就绪（Production-Ready）
│   内置 OTel 追踪、Runtime 部署、状态管理、沙箱执行
│
├── 面向日益增强的模型能力设计
│   利用模型推理 + 工具调用能力，而非固定编排约束
│
└── 内置微调支持（Agentic RL）
    支持通过强化学习直接微调 Agent 行为
```

#<!-- chunk: 1.2 设计哲学 -->## 1.2 设计哲学

AgentScope 1.0 的设计哲学与 LangChain 等框架有本质区别：

| 设计维度 | 传统框架（LangChain 等） | AgentScope |
|---------|------------------------|------------|
| **编排理念** | 严格的 Chain/Graph 编排，开发者定义每个步骤 | 信赖模型推理能力，ReAct 范式让模型自主决策 |
| **异步支持** | 部分支持，需显式处理 | 全面异步架构（async/await 原生） |
| **状态管理** | 各组件独立管理 | 统一状态接口（state_dict/load_state_dict） |
| **工具定义** | 需要特定装饰器/Schema | 任何 Python 可调用对象都是工具 |
| **生产部署** | 需额外框架（如 FastAPI） | 内置 Runtime（AgentApp + FastAPI 继承） |
| **微调能力** | 无内置支持 | 内置 Agentic RL 微调 |

#<!-- chunk: 1.3 在 Agent 框架生态中的位置 -->## 1.3 在 Agent 框架生态中的位置

```
Agent 框架生态（2026）
│
├── 通用编排框架
│   ├── LangChain / LangGraph    - 生态最丰富，抽象层多
│   ├── LlamaIndex               - RAG 能力最强
│   └── Semantic Kernel           - 微软 .NET/Python 企业级
│
├── 多 Agent 协作框架
│   ├── AutoGen (微软)            - Group Chat 对话编排
│   ├── CrewAI                    - 角色扮演式，上手简单
│   └── AgentScope (阿里巴巴)     - 异步原生，生产级，内置微调 ◄─ 本系列
│
├── 低代码平台
│   ├── Dify                      - 可视化工作流
│   └── Coze / 扣子               - 面向非技术用户
│
└── 垂直领域
    └── MetaGPT                   - 软件开发模拟
```

---

<!-- chunk: 2. 核心特性全景 -->## 2. 核心特性全景

#<!-- chunk: 2.1 特性矩阵 -->## 2.1 特性矩阵

```
AgentScope 核心特性
│
├── 基础能力（5 分钟上手）
│   ├── ReAct Agent           - 开箱即用的推理+行动智能体
│   ├── 工具系统              - 任意 Python 可调用对象作为工具 + 7 个内置工具
│   ├── Human-in-the-Loop     - 实时介入、中断、恢复
│   ├── 记忆管理              - 短期（InMemory/AsyncSQLAlchemy/Redis）+ 长期记忆
│   ├── 计划模块              - 子任务分解与管理
│   ├── 实时语音              - 语音输入/输出 + TTS
│   ├── 评测框架              - ACEBench + OpenJudge
│   └── 模型微调              - Agentic RL 强化学习
│
├── 可扩展性
│   ├── 生态集成              - 大量工具、记忆、可观测性集成
│   ├── MCP 支持              - Model Context Protocol 原生集成
│   ├── A2A 支持              - Agent-to-Agent 协议
│   ├── MsgHub               - 灵活的多 Agent 消息编排
│   └── Pipeline             - 顺序/并行/路由/交接工作流
│
└── 生产就绪
    ├── 本地/云端/K8s 部署     - 多种部署模式
    ├── Serverless 弹性伸缩   - 按需扩展
    ├── OTel 可观测性          - OpenTelemetry 原生追踪
    ├── 沙箱执行              - 安全隔离的工具执行环境
    └── AgentScope Studio     - 可视化开发与追踪工具
```

#<!-- chunk: 2.2 核心模块四层架构 -->## 2.2 核心模块四层架构

AgentScope 1.0 将 Agent 应用所需的组件抽象为四大模块：

```
┌──────────────────────────────────────────────────────────┐
│                    Agent 应用层                           │
│   ReActAgent / 自定义 Agent / Voice Agent / A2A Agent    │
├──────────────────────────────────────────────────────────┤
│              Agent 基础设施层                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐    │
│  │ Message  │ │  Model   │ │  Memory  │ │   Tool   │    │
│  │ 消息系统  │ │ 模型接口  │ │ 记忆管理  │ │ 工具系统  │    │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘    │
├──────────────────────────────────────────────────────────┤
│              工作流与编排层                                │
│  MsgHub / Pipeline / Routing / Handoffs / Plan           │
├──────────────────────────────────────────────────────────┤
│              工程支撑层                                    │
│  Runtime / Studio / Tracing / Evaluation / Sandbox       │
└──────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 3. 安装与环境准备 -->## 3. 安装与环境准备

#<!-- chunk: 3.1 系统要求 -->## 3.1 系统要求

| 要求 | 说明 |
|------|------|
| **Python** | 3.10 或更高版本（推荐 3.11+，已验证 3.13） |
| **操作系统** | macOS / Linux / Windows |
| **包管理器** | pip 或 uv |
| **Node.js** | 20.0.0+（仅 AgentScope Studio 可视化工具需要） |
| **可选** | Docker / Podman（用于沙箱执行和生产部署） |

#<!-- chunk: 3.2 安装方式 -->## 3.2 安装方式

**方式一：从 PyPI 安装（推荐）**

```bash
# 基础安装
pip install agentscope

# 或使用 uv（更快）
uv pip install agentscope
```

**方式二：安装完整依赖**

```bash
# 包含所有模型 API 和工具函数的额外依赖
# macOS / Linux
pip install agentscope\[full\]

# Windows
pip install agentscope[full]
```

**方式三：从源码安装（开发者模式）**

```bash
# 克隆仓库
git clone -b main https://github.com/agentscope-ai/agentscope.git
cd agentscope

# 可编辑模式安装
pip install -e .

# 或安装开发依赖
pip install -e .[dev]
```

**方式四：安装 AgentScope Runtime（生产部署）**

```bash
# Runtime 核心
pip install agentscope-runtime

# Runtime + 扩展
pip install "agentscope-runtime[ext]"
```

#<!-- chunk: 3.3 `agentscope[full]` 核心依赖清单 -->## 3.3 `agentscope[full]` 核心依赖清单

以下为 `agentscope[full]` v1.0.17 实际安装的核心依赖（基于 Python 3.13 / Linux x86_64 验证）：

| 分类 | 依赖包 | 版本 | 用途 |
|------|--------|------|------|
| **LLM 提供商** | `openai` | 2.28.0 | OpenAI API 客户端 |
| | `anthropic` | 0.85.0 | Anthropic Claude API 客户端 |
| | `dashscope` | 1.25.14 | 阿里云 DashScope（通义千问）API |
| **MCP 协议** | `mcp` | 1.26.0 | Model Context Protocol 支持 |
| **可观测性** | `opentelemetry-api` | 1.40.0 | OTel API |
| | `opentelemetry-sdk` | 1.40.0 | OTel SDK |
| | `opentelemetry-exporter-otlp` | 1.40.0 | OTel OTLP 导出器 |
| **数据处理** | `numpy` | 2.4.3 | 数值计算 |
| | `tiktoken` | 0.12.0 | Token 计数（OpenAI tokenizer） |
| | `sqlalchemy` | 2.0.48 | 数据库 ORM（记忆持久化） |
| **异步/IO** | `aiofiles` | 25.1.0 | 异步文件操作 |
| | `aioitertools` | 0.13.0 | 异步迭代工具 |
| | `python-socketio` | 5.16.1 | WebSocket 通信 |
| **工具** | `json5` / `json_repair` | 0.13.0 / 0.58.6 | 宽松 JSON 解析 + 自动修复 |
| | `docstring_parser` | 0.17.0 | 工具函数签名自动提取 |
| | `shortuuid` | 1.0.13 | 短 UUID 生成 |
| **音频** | `sounddevice` | 0.5.5 | 语音 Agent 音频输入输出 |

> 完整依赖树包含约 **242 个包**，以上仅列出核心直接依赖。

#<!-- chunk: 3.4 验证安装 -->## 3.4 验证安装

```python
import agentscope
print(agentscope.__version__)
# 输出: 1.0.17（或更高版本）
```

#<!-- chunk: 3.5 API Key 配置 -->## 3.5 API Key 配置

AgentScope 支持多种 LLM 提供商，需要配置相应的 API Key：

**DashScope（阿里云百炼/通义千问）**：

```bash
# 方式一: 环境变量
export DASHSCOPE_API_KEY="sk-your-dashscope-api-key"

# 方式二: 在代码中直接传递
# DashScopeChatModel(model_name="qwen-max", api_key="sk-xxx")
```

> 获取 DashScope API Key: https://dashscope.console.aliyun.com/

**OpenAI**：

```bash
export OPENAI_API_KEY="sk-your-openai-api-key"
```

**本地模型（Ollama）**：

```bash
# 无需 API Key，确保 Ollama 服务运行
ollama serve
ollama pull qwen2.5:7b
```

#<!-- chunk: 3.6 AgentScope Studio 安装（可视化工具） -->## 3.6 AgentScope Studio 安装（可视化工具）

AgentScope Studio 是**独立的可视化开发工具**，基于 Node.js，需要单独安装。它提供 Trace 可视化、Agent 实时交互、评测分析等功能。

> 官方仓库: https://github.com/agentscope-ai/agentscope-studio

**前置条件**：

```bash
# 确认 Node.js 版本（需 >= 20.0.0）
node --version   # 应显示 v20.x.x 或更高
npm --version    # 应显示 10.x.x 或更高
```

**RHEL / CentOS / Alibaba Cloud Linux 安装 Node.js**：

```bash
# 方式一: NodeSource 官方源（推荐）
curl -fsSL https://rpm.nodesource.com/setup_20.x | bash -
yum install -y nodejs

# 方式二: 二进制包（离线/加速）
cd /usr/local
curl -O https://nodejs.org/dist/v20.18.3/node-v20.18.3-linux-x64.tar.xz
tar xf node-v20.18.3-linux-x64.tar.xz
ln -sf /usr/local/node-v20.18.3-linux-x64/bin/{node,npm,npx} /usr/local/bin/
```

> **注意**: `yum install node` 会报 "No match"，正确包名是 `nodejs`；但默认仓库版本通常过旧，建议使用 NodeSource 源。

**安装 C++ 编译工具链**（原生模块 `better-sqlite3` 需要）：

```bash
# 必须安装，否则 npm install 会报 "g++: Command not found"
yum install -y gcc-c++ make
```

**安装 Studio**：

```bash
# 国内环境建议使用淘宝镜像
npm config set registry https://registry.npmmirror.com

# 全局安装
npm install -g @agentscope/studio
```

**启动 Studio**：

```bash
# 前台启动（默认监听 http://localhost:3000）
as_studio

# 后台运行
nohup as_studio > /tmp/as_studio.log 2>&1 &
```

**连接 AgentScope 应用**：

在 Python 代码中配置 `studio_url`，Agent 运行数据将实时上报到 Studio：

```python
import agentscope

agentscope.init(
    # ...其他配置...
    studio_url="http://localhost:3000"
)
```

**Docker 方式部署 Studio**（替代方案）：

```bash
# 国内环境需要配置镜像加速，Docker Hub 直连可能超时
# Podman 用户编辑 /etc/containers/registries.conf 添加 mirror
docker run -p 3000:3000 agentscope/studio:latest
```

**云服务器（ECS）远程访问排查**：

如果在阿里云 ECS 上部署后浏览器无法访问，需检查三个层面：

| 排查层 | 检查命令 / 操作 | 说明 |
|--------|----------------|------|
| **① 服务绑定地址** | `ss -tlnp \| grep 3000` | 如果显示 `127.0.0.1:3000`，需改为 `as_studio --host 0.0.0.0` 或 `HOST=0.0.0.0 as_studio` |
| **② 阿里云安全组** | ECS 控制台 → 安全组 → 入方向添加 TCP/3000 | 这是云平台级别防火墙，**必须在控制台配置** |
| **③ OS 防火墙** | `firewall-cmd --add-port=3000/tcp --permanent && firewall-cmd --reload` | 操作系统级别防火墙 |

---

<!-- chunk: 4. Hello World：第一个 Agent -->## 4. Hello World：第一个 Agent

#<!-- chunk: 4.1 最简示例 — ReAct Agent 对话 -->## 4.1 最简示例 — ReAct Agent 对话

```python
from agentscope.agent import ReActAgent, UserAgent
from agentscope.model import DashScopeChatModel
from agentscope.formatter import DashScopeChatFormatter
from agentscope.memory import InMemoryMemory
from agentscope.tool import Toolkit, execute_python_code, execute_shell_command
import os
import asyncio


async def main():
    # 1. 准备工具
    toolkit = Toolkit()
    toolkit.register_tool_function(execute_python_code)
    toolkit.register_tool_function(execute_shell_command)

    # 2. 创建 ReAct Agent
    agent = ReActAgent(
        name="Friday",
        sys_prompt="You're a helpful assistant named Friday.",
        model=DashScopeChatModel(
            model_name="qwen-max",
            api_key=os.environ["DASHSCOPE_API_KEY"],
            stream=True,
        ),
        memory=InMemoryMemory(),
        formatter=DashScopeChatFormatter(),
        toolkit=toolkit,
    )

    # 3. 创建用户 Agent（接收终端输入）
    user = UserAgent(name="user")

    # 4. 对话循环
    msg = None
    while True:
        msg = await agent(msg)
        msg = await user(msg)
        if msg.get_text_content() == "exit":
            break


asyncio.run(main())
```

**运行效果**：

```
user: 用 Python 计算 1+1
Friday: {
  "type": "tool_use",
  "name": "execute_python_code",
  "input": {"code": "print(1+1)", "timeout": 300}
}
system: {
  "type": "tool_result",
  "name": "execute_python_code",
  "output": [{"type": "text", "text": "<returncode>0</returncode><stdout>2\n</stdout>"}]
}
Friday: 1+1 的结果是 2。
user: exit
```

#<!-- chunk: 4.2 使用 OpenAI 模型 -->## 4.2 使用 OpenAI 模型

```python
from agentscope.model import OpenAIChatModel
from agentscope.formatter import OpenAIChatFormatter

agent = ReActAgent(
    name="Friday",
    sys_prompt="You're a helpful assistant named Friday.",
    model=OpenAIChatModel(
        model_name="gpt-4o",
        api_key=os.environ["OPENAI_API_KEY"],
        stream=True,
    ),
    memory=InMemoryMemory(),
    formatter=OpenAIChatFormatter(),
    toolkit=toolkit,
)
```

#<!-- chunk: 4.3 使用本地模型（Ollama） -->## 4.3 使用本地模型（Ollama）

```python
from agentscope.model import OllamaChatModel
from agentscope.formatter import OllamaChatFormatter

agent = ReActAgent(
    name="Friday",
    sys_prompt="You're a helpful assistant named Friday.",
    model=OllamaChatModel(
        model_name="qwen2.5:7b",
        # Ollama 默认地址
        base_url="http://localhost:11434",
        stream=True,
    ),
    memory=InMemoryMemory(),
    formatter=OllamaChatFormatter(),
    toolkit=toolkit,
)
```

#<!-- chunk: 4.4 无工具的简单对话 Agent -->## 4.4 无工具的简单对话 Agent

```python
from agentscope.agent import ReActAgent
from agentscope.model import DashScopeChatModel
from agentscope.formatter import DashScopeChatFormatter
from agentscope.memory import InMemoryMemory
from agentscope.message import Msg
import asyncio
import os


async def simple_chat():
    agent = ReActAgent(
        name="小助手",
        sys_prompt="你是一个友善的中文助手，擅长回答各种问题。",
        model=DashScopeChatModel(
            model_name="qwen-max",
            api_key=os.environ["DASHSCOPE_API_KEY"],
            stream=True,
        ),
        memory=InMemoryMemory(),
        formatter=DashScopeChatFormatter(),
    )

    # 直接发送消息（非交互式）
    msg = Msg(
        name="user",
        content="请简要介绍 Kubernetes 的核心组件",
        role="user",
    )

    response = await agent(msg)
    print(f"Agent 回复: {response.get_text_content()}")


asyncio.run(simple_chat())
```

---

<!-- chunk: 5. 项目结构与生态 -->## 5. 项目结构与生态

#<!-- chunk: 5.1 AgentScope 项目矩阵 -->## 5.1 AgentScope 项目矩阵

```
AgentScope 生态
│
├── agentscope (核心框架)
│   ├── agentscope.agent      - 智能体模块（ReActAgent, UserAgent, AgentBase...）
│   ├── agentscope.model      - 模型接口（DashScope, OpenAI, Ollama...）
│   ├── agentscope.memory     - 记忆管理（InMemoryMemory, AsyncSQLAlchemyMemory, RedisMemory）
│   ├── agentscope.tool       - 工具系统（Toolkit, ToolResponse, 7 个内置工具）
│   ├── agentscope.message    - 消息系统（Msg）
│   ├── agentscope.formatter  - 提示词格式化器
│   ├── agentscope.pipeline   - 编排管道（sequential/fanout_pipeline, stream_printing_messages）
│   ├── agentscope.mcp        - MCP 协议客户端（Http/StdIO）
│   └── agentscope.session    - 会话管理（JSONSession）
│
├── agentscope-runtime (生产运行时)
│   ├── AgentApp              - FastAPI 继承的 Agent 服务
│   ├── Sandbox               - 安全沙箱执行环境
│   ├── DeployManager         - 部署管理器
│   └── Adapters              - 多框架适配器
│
├── agentscope-studio (可视化工具)
│   ├── 追踪可视化             - OpenTelemetry Trace 展示
│   ├── 项目管理              - 运行管理与配置
│   └── 评测界面              - Agent 评测可视化
│
└── agentscope-samples (示例项目)
    ├── ReAct Agent 示例
    ├── 狼人杀多 Agent 游戏
    ├── 深度研究 Agent
    ├── 浏览器自动化 Agent
    └── Agentic RL 微调示例
```

#<!-- chunk: 5.2 与现有专题的关系 -->## 5.2 与现有专题的关系

| 本系列文档 | 对应专题现有内容 | 关系说明 |
|-----------|----------------|---------|
| 16 - 概述与安装 | [03 - 框架对比](./03-agent-frameworks-comparison.md) | AgentScope 在框架对比中的深度展开 |
| 17 - 核心概念 | [01 - Agent 基础](./01-ai-agent-fundamentals.md) | AgentScope 对通用 Agent 概念的具体实现 |
| 18 - 工具系统 | [05 - Tool Use](./[[domain-14-ai-ml-infra/05-tool-use-function-calling.md|05-tool-use-function-calling]].md) | AgentScope 的工具调用实现与 MCP 集成 |
| 19 - 记忆管理 | [07 - 记忆管理](./07-memory-context-management.md) | AgentScope 的记忆/上下文具体方案 |
| 20 - 多 Agent | [06 - 多 Agent 编排](./06-multi-agent-orchestration.md) | AgentScope MsgHub/Pipeline 编排实践 |
| 21 - 高级特性 | [04 - RAG](./04-rag-knowledge-retrieval.md) | AgentScope RAG、评测、RL 微调等进阶 |
| 22 - 生产部署 | [09 - 生产部署](./[[domain-14-ai-ml-infra/09-production-deployment-guide.md|09-production-deployment-guide]].md) | AgentScope Runtime 的 K8s 部署实践 |

---

<!-- chunk: 6. 内置工具列表 -->## 6. 内置工具列表

AgentScope 内置了实用工具函数，通过 `toolkit.register_tool_function()` 即可注册：

| 工具函数 | 模块 | 功能 |
|---------|------|------|
| `execute_python_code` | `agentscope.tool` | 执行 Python 代码 |
| `execute_shell_command` | `agentscope.tool` | 执行 Shell 命令 |
| `view_text_file` | `agentscope.tool` | 查看文本文件内容 |
| `write_text_file` | `agentscope.tool` | 写入文本文件 |
| `insert_text_file` | `agentscope.tool` | 向文本文件指定位置插入内容 |
| `dashscope_text_to_image` | `agentscope.tool` | DashScope 文生图 |
| `openai_text_to_image` | `agentscope.tool` | OpenAI DALL·E 文生图 |

> 详见 [18 - 工具系统与 MCP 集成](./18-agentscope-tool-system.md)。

---

<!-- chunk: 7. 快速排错 -->## 7. 快速排错

#<!-- chunk: 7.1 常见安装问题 -->## 7.1 常见安装问题

**AgentScope 核心安装问题**：

| 问题 | 原因 | 解决方案 |
|------|------|--------|
| `ModuleNotFoundError: No module named 'agentscope'` | 未正确安装 | `pip install agentscope` |
| `Python version < 3.10` | 版本不满足要求 | 升级 Python 至 3.10+，推荐使用 pyenv 管理 |
| `ImportError: cannot import name 'ReActAgent'` | 版本过旧 | `pip install --upgrade agentscope` |
| DashScope API 认证失败 | API Key 未设置或无效 | 检查 `DASHSCOPE_API_KEY` 环境变量 |
| OpenAI API 连接超时 | 网络问题 | 配置代理或使用 `base_url` 指定中转地址 |
| `extras` 安装失败（macOS） | shell 转义问题 | 使用 `pip install agentscope\[full\]`（反斜杠转义） |

**AgentScope Studio 安装问题**：

| 问题 | 原因 | 解决方案 |
|------|------|--------|
| `yum install node` → No match | 包名错误 | 正确包名为 `nodejs`，但建议用 NodeSource 源安装 v20+ |
| `npm install` → `g++: Command not found` | 缺少 C++ 编译器 | `yum install -y gcc-c++ make`（原生模块 better-sqlite3 需要编译） |
| `docker pull` → `dial tcp ... i/o timeout` | Docker Hub 国内无法访问 | 配置镜像加速或改用 `npm install -g @agentscope/studio` |
| Studio 启动后外网无法访问 | 默认绑定 127.0.0.1 | `as_studio --host 0.0.0.0` + 安全组放行 + 防火墙放行 |
| ECS 公网 IP 无法访问 3000 端口 | 阿里云安全组未配置 | ECS 控制台 → 安全组 → 入方向 → 添加 TCP/3000 规则 |

#<!-- chunk: 7.2 推荐开发环境 -->## 7.2 推荐开发环境

```bash
# 推荐使用 pyenv + virtualenv
pyenv install 3.11.9
pyenv virtualenv 3.11.9 agentscope-env
pyenv activate agentscope-env

# 安装 AgentScope
pip install agentscope\[full\]

# 验证
python -c "import agentscope; print(agentscope.__version__)"
```

---

<!-- chunk: 8. 最佳实践与反模式 -->## 8. 最佳实践与反模式

#<!-- chunk: 最佳实践 -->## 最佳实践

- **从 ReActAgent 开始**：AgentScope 的 ReAct Agent 是最核心的组件，先熟练使用再扩展
- **API Key 用环境变量管理**：避免在代码中硬编码密钥，使用 `os.environ` 或 `.env` 文件
- **优先使用 DashScope**：AgentScope 与阿里云 DashScope 集成最深，Qwen 系列模型兼容性最佳
- **使用 stream=True**：生产环境始终开启流式输出，提升用户体验和响应速度
- **安装 full 依赖**：开发阶段建议安装 `agentscope[full]`，避免缺少依赖导致的功能缺失

#<!-- chunk: 反模式 -->## 反模式

- **忽视异步设计**：AgentScope 原生异步，不要用 `sync` 包装器绕过 `async/await`
- **跳过环境验证**：安装后不验证版本，可能导致 API 不兼容
- **混用 Formatter 和 Model**：DashScopeChatModel 必须配合 DashScopeChatFormatter，混用会导致格式错误
- **在生产环境用 InMemoryMemory**：内存记忆不持久化，重启丢失，生产应使用 `AsyncSQLAlchemyMemory`（PostgreSQL/SQLite）或 `RedisMemory`

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [17 - 核心概念与基础操作](./17-agentscope-core-concepts.md) | Agent、Message、Model、Formatter 详解 |
| [18 - 工具系统与 MCP 集成](./18-agentscope-tool-system.md) | Toolkit、MCP、自定义工具 |
| [03 - 主流 Agent 框架对比](./03-agent-frameworks-comparison.md) | AgentScope 与 LangChain/AutoGen/CrewAI 对比 |
| [01 - AI Agent 基础与核心架构](./01-ai-agent-fundamentals.md) | Agent 通用概念和推理框架 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*

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

- [[domain-14-ai-ml-infra/14-agent-kudig-design-strategy.md|14-agent-kudig-design-strategy]]
- [[domain-14-ai-ml-infra/15-agent-corpus-gap-analysis.md|15-agent-corpus-gap-analysis]]
- [[domain-14-ai-ml-infra/17-agentscope-core-concepts.md|17-agentscope-core-concepts]]
- [[domain-14-ai-ml-infra/18-agentscope-tool-system.md|18-agentscope-tool-system]]
