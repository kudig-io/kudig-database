---
title: 流式传输与多模态输入
description: '**文档类型**: 功能详解 | **最后更新**: 2026-03 | **关键词**: OpenRouter, Streaming, SSE, Stream Cancellation, Multimodal, Image,
  PDF, Audio, Vision, Base64'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 开发工程师
- AI 工程师
estimated_read_time: 5min
intent_queries:
- 流式传输与多模态输入 是什么
- 如何 流式传输与多模态输入
trigger_keywords:
- 流式传输与多模态输入
- ai
- coding
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# 流式传输与多模态输入

> **文档类型**: 功能详解 | **最后更新**: 2026-03 | **关键词**: OpenRouter, Streaming, SSE, Stream Cancellation, Multimodal, Image, PDF, Audio, Vision, Base64

---

## 概述

OpenRouter 支持 SSE 流式传输和多模态输入两大核心能力。本文覆盖 SSE Streaming 基础与协议详解、Stream Cancellation（AbortController）、流中错误处理、图像输入（URL / Base64）、PDF 解析、多图混合、图像生成以及流式 + 多模态最佳实践。

---

## 1. SSE 流式传输

### 1.1 基本使用

```typescript
import { OpenRouter } from '@openrouter/sdk';

const openRouter = new OpenRouter({ apiKey: process.env.OPENROUTER_API_KEY });

const stream = await openRouter.chat.send({
  model: 'openai/gpt-5.2',
  messages: [{ role: 'user', content: 'Tell me a long story' }],
  stream: true,
});

for await (const chunk of stream) {
  const content = chunk.choices?.[0]?.delta?.content;
  if (content) {
    process.stdout.write(content);
  }

  // 最后一个 chunk 包含 usage 信息
  if (chunk.usage) {
    console.log('\nUsage:', chunk.usage);
  }
}
```

### 1.2 SSE 协议细节

OpenRouter 使用 **Server-Sent Events (SSE)** 协议进行流式传输：

| 事件类型 | 说明 |
|---------|------|
| `data: {...}` | 正常内容 chunk |
| `data: [DONE]` | 流结束标志 |
| `: OPENROUTER PROCESSING` | 心跳注释（防止连接超时） |

> 心跳注释（以 `:` 开头）是 SSE 规范的合法注释，应被安全忽略。

### 1.3 推荐 SSE 客户端

| 客户端 | 说明 |
|--------|------|
| `eventsource-parser` | 轻量 SSE 解析库 |
| **OpenAI SDK** | 内置 SSE 处理 |
| **Vercel AI SDK** | 内置流式 UI 支持 |

---

## 2. Stream Cancellation

### 2.1 取消机制

流式请求可通过中止连接来取消，支持的 Provider 会立即停止处理和计费：

```typescript
const controller = new AbortController();

try {
  const stream = await openRouter.chat.send({
    model: 'openai/gpt-5.2',
    messages: [{ role: 'user', content: 'Write a very long story' }],
    stream: true,
  }, {
    signal: controller.signal,
  });

  for await (const chunk of stream) {
    const content = chunk.choices?.[0]?.delta?.content;
    if (content) {
      process.stdout.write(content);
    }
    // 满足条件时取消
    if (someCondition) {
      controller.abort();
    }
  }
} catch (error) {
  if (error.name === 'AbortError') {
    console.log('Stream cancelled');
  } else {
    throw error;
  }
}
```

### 2.2 Provider 支持情况

**支持取消（停止处理 + 停止计费）**：

OpenAI、Azure、Anthropic、Fireworks、Mancer、Recursal、AnyScale、Lepton、OctoAI、Novita、DeepInfra、Together、Cohere、Hyperbolic、xAI、Cloudflare、DeepSeek 等

**不支持取消（继续处理 + 继续计费）**：

AWS Bedrock、Groq、Modal、Google、Google AI Studio、Mistral、AI21、Perplexity、HuggingFace、Replicate、SambaNova、Nebius 等

> 对于不支持取消的 Provider，模型会继续处理并按完整响应计费。

---

## 3. 流式错误处理

### 3.1 流开始前的错误

返回标准 JSON 错误响应 + 对应 HTTP 状态码：

```json
{
  "error": { "code": 400, "message": "Invalid model" }
}
```

### 3.2 流传输中的错误（Mid-Stream）

HTTP 状态码已为 200，错误通过 SSE 事件传递：

```json
{
  "id": "cmpl-abc123",
  "object": "chat.completion.chunk",
  "error": {
    "code": "server_error",
    "message": "Provider disconnected"
  },
  "choices": [{
    "index": 0,
    "delta": { "content": "" },
    "finish_reason": "error"
  }]
}
```

### 3.3 完整错误处理示例

```typescript
async function streamWithErrorHandling(prompt: string) {
  try {
    const stream = await openRouter.chat.send({
      model: 'openai/gpt-5.2',
      messages: [{ role: 'user', content: prompt }],
      stream: true,
    });

    for await (const chunk of stream) {
      // 检查 mid-stream 错误
      if ('error' in chunk) {
        console.error(`Stream error: ${chunk.error.message}`);
        return;
      }

      const content = chunk.choices?.[0]?.delta?.content;
      if (content) {
        process.stdout.write(content);
      }
    }
  } catch (error) {
    // 处理 pre-stream 错误
    console.error(`Error: ${error.message}`);
  }
}
```

---

## 4. 调试选项

在流式请求中启用调试，查看发送给 Provider 的实际请求体：

```json
{
  "stream": true,
  "debug": {
    "echo_upstream_body": true
  }
}
```

---

## 5. 多模态输入

### 5.1 支持的输入格式

| 格式 | 支持方式 | 适用模型 |
|------|---------|---------|
| **图像 (URL)** | `image_url.url` | 支持视觉的模型 |
| **图像 (Base64)** | `image_url.url` (data URI) | 支持视觉的模型 |
| **PDF (URL)** | `image_url.url` + file-parser 插件 | 所有模型（通过插件） |
| **PDF (Base64)** | `image_url.url` (data URI) | 所有模型（通过插件） |
| **音频** | Provider 特定格式 | 支持音频的模型 |

### 5.2 图像输入

```json
{
  "model": "openai/gpt-5.2",
  "messages": [
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "What's in this image?"
        },
        {
          "type": "image_url",
          "image_url": {
            "url": "https://example.com/image.jpg",
            "detail": "high"
          }
        }
      ]
    }
  ]
}
```

**Base64 方式**：

```json
{
  "type": "image_url",
  "image_url": {
    "url": "data:image/jpeg;base64,/9j/4AAQSkZJRgABAQ..."
  }
}
```

| `detail` 值 | 说明 |
|-------------|------|
| `auto` | 模型自动选择（默认） |
| `low` | 低分辨率，快速 + 便宜 |
| `high` | 高分辨率，慢 + 详细 |

### 5.3 PDF 输入

```json
{
  "model": "openai/gpt-5.2",
  "plugins": [{ "id": "file-parser" }],
  "messages": [
    {
      "role": "user",
      "content": [
        { "type": "text", "text": "Summarize this PDF" },
        {
          "type": "image_url",
          "image_url": {
            "url": "https://example.com/document.pdf"
          }
        }
      ]
    }
  ]
}
```

> 启用 `file-parser` 插件后，**任何模型**都可以处理 PDF，不限于原生支持的模型。

### 5.4 多图像 + 文本混合

```json
{
  "messages": [
    {
      "role": "user",
      "content": [
        { "type": "text", "text": "Compare these two images:" },
        {
          "type": "image_url",
          "image_url": { "url": "https://example.com/img1.jpg" }
        },
        {
          "type": "image_url",
          "image_url": { "url": "https://example.com/img2.jpg" }
        },
        { "type": "text", "text": "Which one looks better?" }
      ]
    }
  ]
}
```

---

## 6. 图像生成

OpenRouter 也支持图像生成模型：

```bash
# 查询可用的图像生成模型
curl "https://openrouter.ai/api/v1/models?output_modalities=image"
```

---

## 7. 最佳实践

| 实践 | 说明 |
|------|------|
| **流式 UI** | 使用 Vercel AI SDK 的 `useChat` hook 获得最佳流式 UI 体验 |
| **超时处理** | 监听心跳注释，长时间无数据时主动超时 |
| **重试策略** | pre-stream 错误可重试；mid-stream 错误需重新发送 |
| **图像压缩** | 上传前压缩图像，减少 token 消耗 |
| **PDF 分块** | 大 PDF 分块处理，避免超出上下文限制 |
| **AbortController** | 在 UI 中提供"停止生成"按钮，使用 AbortController 取消流 |

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [05 - API 参考](./05-openrouter-api-reference.md) | 流式响应 Schema 详解 |
| [07 - 插件与 Web Search](./07-openrouter-plugins-web-search.md) | File Parser 插件与 PDF 解析 |
| [09 - 框架集成](./09-openrouter-frameworks-integrations.md) | Vercel AI SDK 流式 UI 集成 |
| [12 - 企业级实践](./12-openrouter-enterprise-advanced.md) | 故障排查与最佳实践 |

---

*本文档基于 OpenRouter 官方文档（openrouter.ai/docs/guides/features/streaming）整理。*
