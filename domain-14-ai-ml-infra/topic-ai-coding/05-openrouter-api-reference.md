---
title: API 参考与请求/响应规范
description: '## 概述'
summary: '## 概述'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
- gateway
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 开发工程师
- AI 工程师
estimated_read_time: 5min
intent_queries:
- API 参考与请求/响应规范 是什么
- 如何 API 参考与请求/响应规范
trigger_keywords:
- API
- 参考与请求
- 响应规范
- ai
- coding
prerequisites:
- kubectl-basics
---



# API 参考与请求/响应规范

> **文档类型**: API 参考 | **最后更新**: 2026-03 | **关键词**: OpenRouter, API Reference, Chat Completions, Request Schema, Response, Parameters, Error Handling, OpenAPI

---

## 概述

本文提供 OpenRouter API 的完整参考：包括全部端点概览、Chat Completions 请求 Schema、消息类型、采样参数、响应格式（流式与非流式）、Tool Calling 类型、Assistant Prefill 技术、HTTP 状态码与错误处理以及 OpenAPI Spec 访问方式。

---

## 1. API 端点总览

| 端点 | 方法 | 说明 |
|------|------|------|
| `/api/v1/chat/completions` | POST | Chat Completions（核心端点） |
| `/api/v1/models` | GET | 获取模型列表 |
| `/api/v1/models/count` | GET | 获取模型数量 |
| `/api/v1/key` | GET | 查询 API Key 状态 |
| `/api/v1/generation` | GET | 查询 Generation 详情 |
| `/openapi.yaml` | GET | OpenAPI 3.1 Spec (YAML) |
| `/openapi.json` | GET | OpenAPI 3.1 Spec (JSON) |

---

## 2. 请求 Schema

### 2.1 完整请求类型

```typescript
type Request = {
  // 消息或 prompt（二选一）
  messages?: Message[];
  prompt?: string;

  // 模型选择
  model?: string;              // 如 "openai/gpt-5.2"
  models?: string[];           // Model Fallback
  route?: 'fallback';          // 启用 Fallback 路由

  // 输出格式控制
  response_format?: ResponseFormat;
  stop?: string | string[];
  stream?: boolean;            // 启用 SSE 流式

  // 插件
  plugins?: Plugin[];

  // 采样参数
  max_tokens?: number;         // [1, context_length)
  temperature?: number;        // [0, 2]，默认 1.0
  top_p?: number;              // (0, 1]，默认 1.0
  top_k?: number;              // [1, ∞)
  frequency_penalty?: number;  // [-2, 2]，默认 0
  presence_penalty?: number;   // [-2, 2]，默认 0
  repetition_penalty?: number; // (0, 2]，默认 1.0
  min_p?: number;              // [0, 1]
  top_a?: number;              // [0, 1]
  seed?: number;               // 整数
  logit_bias?: Record<number, number>;
  logprobs?: boolean;
  top_logprobs?: number;       // [0, 20]

  // Tool Calling
  tools?: Tool[];
  tool_choice?: ToolChoice;
  parallel_tool_calls?: boolean; // 默认 true

  // Verbosity 控制（OpenAI/Anthropic）
  verbosity?: 'low' | 'medium' | 'high' | 'max';

  // 预测输出（延迟优化）
  prediction?: { type: 'content'; content: string };

  // OpenRouter 专属
  provider?: ProviderPreferences;
  user?: string;               // 终端用户标识（滥用检测）

  // 调试选项（仅 Streaming）
  debug?: {
    echo_upstream_body?: boolean; // 返回发送给 Provider 的请求体
  };
};
```

### 2.2 Message 类型

```typescript
type Message =
  | {
      role: 'user' | 'assistant' | 'system';
      content: string | ContentPart[];
      name?: string;
    }
  | {
      role: 'tool';
      content: string;
      tool_call_id: string;
      name?: string;
    };

type ContentPart = TextContent | ImageContentPart;

type TextContent = {
  type: 'text';
  text: string;
};

type ImageContentPart = {
  type: 'image_url';
  image_url: {
    url: string;    // URL 或 Base64 编码
    detail?: string; // 默认 "auto"
  };
};
```

---

## 3. 采样参数详解

| 参数 | 范围 | 默认值 | 说明 |
|------|------|--------|------|
| `temperature` | 0~2 | 1.0 | 控制多样性。0=确定性，2=最大随机 |
| `top_p` | 0~1 | 1.0 | 核采样。只考虑概率和达到 P 的 token |
| `top_k` | 0~∞ | 0 (禁用) | 每步只考虑 top-K 个 token |
| `frequency_penalty` | -2~2 | 0 | 按频率抑制重复，负值鼓励重复 |
| `presence_penalty` | -2~2 | 0 | 按是否出现抑制重复，不考虑频率 |
| `repetition_penalty` | 0~2 | 1.0 | 基于原始概率的重复惩罚 |
| `min_p` | 0~1 | 0 | 最低相对概率阈值 |
| `top_a` | 0~1 | 0 | 动态 Top-P |
| `seed` | integer | - | 确定性输出（不保证所有模型） |
| `max_tokens` | 1~ctx | - | 最大输出 token 数 |
| `verbosity` | enum | medium | 响应详略程度 (low/medium/high/max) |

---

## 4. 响应 Schema

### 4.1 非流式响应

```typescript
type Response = {
  id: string;
  choices: NonStreamingChoice[];
  created: number;             // Unix 时间戳
  model: string;               // 实际使用的模型
  object: 'chat.completion';
  system_fingerprint?: string;
  usage?: ResponseUsage;
};

type NonStreamingChoice = {
  message: {
    role: 'assistant';
    content: string | null;
    tool_calls?: ToolCall[];
  };
  finish_reason: string;       // "stop" | "length" | "tool_calls" | "error"
  index: number;
};
```

### 4.2 Usage 对象

```typescript
type ResponseUsage = {
  prompt_tokens: number;
  completion_tokens: number;
  total_tokens: number;
  prompt_tokens_details?: {
    cached_tokens: number;      // 缓存命中 token 数
    cache_write_tokens: number; // 缓存写入 token 数
  };
};
```

### 4.3 流式响应

```typescript
type StreamChunk = {
  id: string;
  choices: StreamingChoice[];
  created: number;
  model: string;
  object: 'chat.completion.chunk';
  usage?: ResponseUsage;       // 仅最后一个 chunk 包含
};

type StreamingChoice = {
  delta: {
    role?: 'assistant';
    content?: string;
    tool_calls?: ToolCallDelta[];
  };
  finish_reason: string | null;
  index: number;
};
```

---

## 5. Tool Calling

### 5.1 Tool 定义

```typescript
type Tool = {
  type: 'function';
  function: {
    name: string;
    description?: string;
    parameters: object;  // JSON Schema
  };
};

type ToolChoice =
  | 'none'     // 不调用任何工具
  | 'auto'     // 模型自行决定
  | 'required' // 必须调用工具
  | { type: 'function'; function: { name: string } }; // 指定工具
```

### 5.2 Tool Calling 示例

```json
{
  "model": "openai/gpt-5.2",
  "messages": [
    { "role": "user", "content": "What's the weather in London?" }
  ],
  "tools": [
    {
      "type": "function",
      "function": {
        "name": "get_weather",
        "description": "Get current weather for a location",
        "parameters": {
          "type": "object",
          "properties": {
            "location": { "type": "string" }
          },
          "required": ["location"]
        }
      }
    }
  ],
  "tool_choice": "auto"
}
```

---

## 6. Assistant Prefill

引导模型从特定前缀开始生成：

```json
{
  "model": "openai/gpt-5.2",
  "messages": [
    { "role": "user", "content": "What is the meaning of life?" },
    { "role": "assistant", "content": "I'm not sure, but my best guess is" }
  ]
}
```

---

## 7. Error Handling

### 7.1 HTTP 状态码

| 状态码 | 说明 | 常见原因 |
|--------|------|---------|
| **400** | Bad Request | 参数无效、Schema 错误 |
| **401** | Unauthorized | API Key 无效或缺失 |
| **402** | Payment Required | Credits 不足 |
| **408** | Request Timeout | 请求超时 |
| **429** | Too Many Requests | Rate Limit 超限 |
| **502** | Bad Gateway | Provider 返回错误 |
| **503** | [[Service]] Unavailable | 无可用 Provider |

### 7.2 Error 响应格式

```json
{
  "error": {
    "code": 400,
    "message": "Invalid model specified"
  }
}
```

### 7.3 流式中间错误

流式传输中如果已发送部分 token 后出错，HTTP 状态码仍为 200，错误通过 SSE 事件传递：

```json
{
  "id": "cmpl-abc123",
  "object": "chat.completion.chunk",
  "error": {
    "code": "server_error",
    "message": "Provider disconnected unexpectedly"
  },
  "choices": [{
    "index": 0,
    "delta": { "content": "" },
    "finish_reason": "error"
  }]
}
```

---

## 8. OpenAPI Specification

完整 API 规范可通过以下地址获取：

```bash
# YAML 格式
curl https://openrouter.ai/openapi.yaml

# JSON 格式
curl https://openrouter.ai/openapi.json
```

可导入 Swagger UI、Postman 或任何 OpenAPI 兼容工具使用。

---

## 9. 非标准参数处理

| 场景 | 行为 |
|------|------|
| 模型不支持的参数（如 OpenAI 模型收到 `top_k`） | 参数被忽略，请求正常执行 |
| Provider 特有参数（如 Mistral 的 `safe_prompt`） | 直通转发给对应 Provider |
| `name` 字段（非 OpenAI 模型） | 自动转换为 `{name}: {content}` 格式 |

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [04 - 智能路由](./04-openrouter-provider-routing.md) | provider 请求参数详解 |
| [06 - Structured Outputs 与 Tool Calling](./06-openrouter-structured-outputs-tools.md) | JSON Schema 约束与工具调用 |
| [10 - 流式传输与多模态](./10-openrouter-streaming-multimedia.md) | SSE Streaming 详解 |
| [12 - 企业级实践](./12-openrouter-enterprise-advanced.md) | Rate Limits 与故障排查 |

---

*本文档基于 OpenRouter 官方文档（openrouter.ai/docs/api-reference）整理。*
