---
title: 框架集成与生态系统
description: '**文档类型**: 集成指南 | **最后更新**: 2026-03 | **关键词**: OpenRouter, Frameworks, OpenAI SDK, LangChain, Vercel AI, LlamaIndex,
  Mastra, PydanticAI, Aider, Cline'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
- llm
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 开发工程师
- AI 工程师
estimated_read_time: 5min
intent_queries:
- 框架集成与生态系统 是什么
- 如何 框架集成与生态系统
trigger_keywords:
- 框架集成与生态系统
- ai
- coding
---

# 框架集成与生态系统

> **文档类型**: 集成指南 | **最后更新**: 2026-03 | **关键词**: OpenRouter, Frameworks, OpenAI SDK, LangChain, Vercel AI, LlamaIndex, Mastra, PydanticAI, Aider, Cline

---

## 概述

OpenRouter 兼容 OpenAI API 规范，因此任何支持 OpenAI 的框架和工具都可以直接接入。本文覆盖 OpenAI SDK、Vercel AI SDK、LangChain、LlamaIndex、PydanticAI 等主流框架的集成示例，以及 Aider、Cline、OpenCode 等 AI 编程工具的配置方法和 Langfuse 可观测性集成。

---

## 1. 集成生态总览

OpenRouter 兼容 OpenAI API 规范，因此**任何支持 OpenAI 的框架和工具**都可以直接使用 OpenRouter。

### 1.1 官方集成

| 框架 | 类型 | 语言 |
|------|------|------|
| **OpenRouter SDK** | 原生 SDK | TypeScript |
| **OpenAI SDK** | 直接兼容 | Python / TypeScript |
| **Vercel AI SDK** | AI 框架 | TypeScript (Next.js) |
| **LangChain** | Agent 框架 | Python / JavaScript |
| **LlamaIndex** | RAG 框架 | Python / TypeScript |
| **Mastra** | AI 框架 | TypeScript |
| **PydanticAI** | Python AI 框架 | Python |
| **TanStack AI** | UI 框架 | React / Solid / Preact |
| **Effect AI SDK** | 函数式框架 | TypeScript (Effect) |

### 1.2 工具集成

| 工具 | 类型 | 说明 |
|------|------|------|
| **Aider** | AI 编程助手 | 终端编码工具 |
| **Cline** | VS Code 插件 | 编码智能体 |
| **Roo Code** | 编码助手 | 多模型编码 |
| **Kilo Code** | 编码助手 | VS Code |
| **Deep Agents CLI** | 终端 Agent | 编码智能体 |
| **Junie CLI** | JetBrains 工具 | JetBrains 编码 |
| **VSCode Copilot** | IDE 集成 | VS Code |
| **Xcode** | IDE 集成 | Apple Xcode |
| **Langfuse** | 可观测性 | 追踪与监控 |

---

## 2. OpenAI SDK 集成

### 2.1 TypeScript

```typescript
import OpenAI from 'openai';

const client = new OpenAI({
  baseURL: 'https://openrouter.ai/api/v1',
  apiKey: process.env.OPENROUTER_API_KEY,
  defaultHeaders: {
    'HTTP-Referer': 'https://your-app.com',
    'X-OpenRouter-Title': 'Your App',
  },
});

// 同步
const response = await client.chat.completions.create({
  model: 'anthropic/claude-sonnet-4.6',
  messages: [{ role: 'user', content: 'Hello' }],
});

// 流式
const stream = await client.chat.completions.create({
  model: 'anthropic/claude-sonnet-4.6',
  messages: [{ role: 'user', content: 'Hello' }],
  stream: true,
});

for await (const chunk of stream) {
  process.stdout.write(chunk.choices[0]?.delta?.content || '');
}
```

### 2.2 Python

```python
from openai import OpenAI

client = OpenAI(
    base_url="https://openrouter.ai/api/v1",
    api_key="sk-or-v1-xxxx",
    default_headers={
        "HTTP-Referer": "https://your-app.com",
        "X-OpenRouter-Title": "Your App",
    },
)

# 同步
response = client.chat.completions.create(
    model="anthropic/claude-sonnet-4.6",
    messages=[{"role": "user", "content": "Hello"}],
)

# 流式
stream = client.chat.completions.create(
    model="openai/gpt-5.2",
    messages=[{"role": "user", "content": "Tell me a story"}],
    stream=True,
)

for chunk in stream:
    content = chunk.choices[0].delta.content
    if content:
        print(content, end="", flush=True)
```

---

## 3. Vercel AI SDK 集成

```typescript
// app/api/chat/route.ts (Next.js App Router)
import { createOpenAI } from '@ai-sdk/openai';
import { streamText } from 'ai';

const openrouter = createOpenAI({
  baseURL: 'https://openrouter.ai/api/v1',
  apiKey: process.env.OPENROUTER_API_KEY,
  headers: {
    'HTTP-Referer': 'https://your-app.com',
    'X-OpenRouter-Title': 'Your App',
  },
});

export async function POST(req: Request) {
  const { messages } = await req.json();

  const result = streamText({
    model: openrouter('openai/gpt-5.2'),
    messages,
  });

  return result.toDataStreamResponse();
}
```

---

## 4. LangChain 集成

### 4.1 Python

```python
from langchain_openai import ChatOpenAI

llm = ChatOpenAI(
    model="anthropic/claude-sonnet-4.6",
    openai_api_key="sk-or-v1-xxxx",
    openai_api_base="https://openrouter.ai/api/v1",
    default_headers={
        "HTTP-Referer": "https://your-app.com",
        "X-OpenRouter-Title": "Your App",
    },
)

response = llm.invoke("What is quantum computing?")
print(response.content)
```

### 4.2 JavaScript

```typescript
import { ChatOpenAI } from '@langchain/openai';

const model = new ChatOpenAI({
  modelName: 'openai/gpt-5.2',
  openAIApiKey: process.env.OPENROUTER_API_KEY,
  configuration: {
    baseURL: 'https://openrouter.ai/api/v1',
    defaultHeaders: {
      'HTTP-Referer': 'https://your-app.com',
    },
  },
});

const response = await model.invoke('Hello');
```

---

## 5. LlamaIndex 集成

### 5.1 Python

```python
from llama_index.llms.openai import OpenAI

llm = OpenAI(
    model="openai/gpt-5.2",
    api_key="sk-or-v1-xxxx",
    api_base="https://openrouter.ai/api/v1",
    additional_kwargs={
        "headers": {
            "HTTP-Referer": "https://your-app.com",
        }
    },
)

response = llm.complete("What is RAG?")
```

---

## 6. PydanticAI 集成

```python
from pydantic_ai import Agent
from pydantic_ai.models.openai import OpenAIModel

model = OpenAIModel(
    'anthropic/claude-sonnet-4.6',
    base_url='https://openrouter.ai/api/v1',
    api_key='sk-or-v1-xxxx',
)

agent = Agent(model)
result = agent.run_sync('What is the capital of France?')
print(result.data)
```

---

## 7. AI 编码工具集成

### 7.1 Aider

```bash
# 环境变量配置
export OPENROUTER_API_KEY=sk-or-v1-xxxx

# 使用 OpenRouter 启动 Aider
aider --model openrouter/anthropic/claude-sonnet-4.6
```

### 7.2 Cline (VS Code)

在 VS Code Cline 插件设置中：

1. **API Provider** → 选择 "OpenRouter"
2. **API Key** → 输入 OpenRouter API Key
3. **Model** → 选择所需模型

### 7.3 OpenCode

在 `opencode.json` 中配置：

```json
{
  "provider": {
    "openrouter": {
      "apiKey": "sk-or-v1-xxxx"
    }
  },
  "agents": {
    "build": {
      "model": "openrouter/anthropic/claude-sonnet-4.6"
    }
  }
}
```

---

## 8. Langfuse 可观测性集成

```python
from langfuse.openai import OpenAI

client = OpenAI(
    base_url="https://openrouter.ai/api/v1",
    api_key="sk-or-v1-xxxx",
)

# Langfuse 自动追踪所有 OpenRouter 请求
response = client.chat.completions.create(
    model="openai/gpt-5.2",
    messages=[{"role": "user", "content": "Hello"}],
)
```

---

## 9. 框架选型指南

| 场景 | 推荐框架 | 原因 |
|------|---------|------|
| **快速原型** | OpenAI SDK | 零配置，最简单 |
| **Next.js Web 应用** | Vercel AI SDK | 原生 Streaming UI |
| **复杂 Agent** | LangChain | Chain/Tool/Memory 生态 |
| **RAG 应用** | LlamaIndex | 向量索引 + 检索优化 |
| **类型安全 Python** | PydanticAI | Pydantic 验证集成 |
| **终端编码** | Aider / Cline | 编程助手 |
| **原生 SDK** | OpenRouter SDK | 类型安全 + 完整能力 |

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [02 - 快速接入](./02-openrouter-quickstart-setup.md) | SDK 安装与首次请求 |
| [06 - Structured Outputs 与 Tool Calling](./06-openrouter-structured-outputs-tools.md) | 框架中的 Tool Calling 集成 |
| [10 - 流式传输](./10-openrouter-streaming-multimedia.md) | 框架的流式集成 |
| [topic-coding](../topic-coding/) | OpenCode Agent 中使用 OpenRouter |

---

*本文档基于 OpenRouter 官方文档（openrouter.ai/docs/frameworks）整理。*
