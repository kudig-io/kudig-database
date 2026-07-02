---
title: 快速接入与环境配置
description: '# 快速接入与环境配置'
summary: '1. 登录 [openrouter.ai](https://openrouter.ai)'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 开发工程师
- AI 工程师
estimated_read_time: 5min
intent_queries:
- 快速接入与环境配置 是什么
- 如何 快速接入与环境配置
trigger_keywords:
- 快速接入与环境配置
- ai
- coding
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 快速接入与环境配置

> **文档类型**: 部署指南 | **最后更新**: 2026-03 | **关键词**: OpenRouter, QuickStart, SDK, API Key, OpenAI Compatible, Installation, Configuration

---

## 概述

本文覆盖 OpenRouter 的完整接入流程：从 API Key 创建、SDK 安装（OpenRouter SDK / OpenAI SDK / 直接 HTTP）、环境变量配置到首次请求验证。无论你使用 Python、TypeScript 还是 Shell，都可以在 5 分钟内开始使用 OpenRouter 的 400+ 模型。

---

## 1. 前置条件

| 条件 | 说明 |
|------|------|
| **OpenRouter 账号** | 访问 [openrouter.ai](https://openrouter.ai) 注册 |
| **API Key** | 在 [Keys 页面](https://openrouter.ai/keys) 创建 |
| **Credits** | 在 [Credits 页面](https://openrouter.ai/credits) 充值（可先使用 `:free` 模型免费试用） |
| **网络环境** | 能访问 `https://openrouter.ai/api/v1/*` |

---

## 2. API Key 管理

### 2.1 创建 API Key

1. 登录 [openrouter.ai](https://openrouter.ai)
2. 导航至 **Settings → Keys**
3. 点击 **Create Key**
4. 设置可选参数：

| 参数 | 说明 | 默认值 |
|------|------|--------|
| **Label** | Key 描述标签 | - |
| **Credit Limit** | 额度上限（美元） | 无限制 |
| **Limit Reset** | 额度重置周期 | 不重置 |
| **Include BYOK** | 是否将 BYOK 用量计入额度 | false |

### 2.2 Key 信息查询

```bash
# 查询 API Key 状态、剩余额度、用量统计
curl https://openrouter.ai/api/v1/key \
  -H "Authorization: Bearer $OPENROUTER_API_KEY"
```

响应示例：

```json
{
  "data": {
    "label": "my-app-key",
    "limit": 100,
    "limit_remaining": 87.5,
    "usage": 12.5,
    "usage_daily": 3.2,
    "usage_monthly": 12.5,
    "is_free_tier": false
  }
}
```

---

## 3. 三种接入方式

### 3.1 方式一：OpenRouter SDK（推荐）

OpenRouter 官方 TypeScript SDK，提供类型安全的原生接口：

**安装：**

```bash
# npm
npm install @openrouter/sdk

# yarn
yarn add @openrouter/sdk

# pnpm
pnpm add @openrouter/sdk
```

**使用：**

```typescript
import { OpenRouter } from '@openrouter/sdk';

const openRouter = new OpenRouter({
  apiKey: process.env.OPENROUTER_API_KEY,
  defaultHeaders: {
    'HTTP-Referer': 'https://your-app.com',      // 可选：App Attribution
    'X-OpenRouter-Title': 'Your App Name',         // 可选：排行榜显示名
  },
});

const completion = await openRouter.chat.send({
  model: 'openai/gpt-5.2',
  messages: [
    { role: 'user', content: 'What is the meaning of life?' },
  ],
  stream: false,
});

console.log(completion.choices[0].message.content);
```

### 3.2 方式二：OpenAI SDK（零迁移）

只需替换 `baseURL`，现有 OpenAI 代码无需任何修改：

**TypeScript：**

```typescript
import OpenAI from 'openai';

const openai = new OpenAI({
  baseURL: 'https://openrouter.ai/api/v1',
  apiKey: process.env.OPENROUTER_API_KEY,
  defaultHeaders: {
    'HTTP-Referer': 'https://your-app.com',
    'X-OpenRouter-Title': 'Your App Name',
  },
});

const completion = await openai.chat.completions.create({
  model: 'openai/gpt-5.2',
  messages: [
    { role: 'user', content: 'Hello, world!' },
  ],
});

console.log(completion.choices[0].message);
```

**Python：**

```python
from openai import OpenAI

client = OpenAI(
    base_url="https://openrouter.ai/api/v1",
    api_key="your-openrouter-key",
    default_headers={
        "HTTP-Referer": "https://your-app.com",
        "X-OpenRouter-Title": "Your App Name",
    },
)

completion = client.chat.completions.create(
    model="openai/gpt-5.2",
    messages=[
        {"role": "user", "content": "Hello, world!"},
    ],
)

print(completion.choices[0].message.content)
```

### 3.3 方式三：直接 HTTP API

无需任何 SDK，直接使用 HTTP 请求：

```bash
curl https://openrouter.ai/api/v1/chat/completions \
  -H "Authorization: Bearer $OPENROUTER_API_KEY" \
  -H "Content-Type: application/json" \
  -H "HTTP-Referer: https://your-app.com" \
  -H "X-OpenRouter-Title: Your App Name" \
  -d '{
    "model": "openai/gpt-5.2",
    "messages": [
      {"role": "user", "content": "Hello!"}
    ]
  }'
```

---

## 4. 环境变量配置

推荐使用环境变量管理 API Key：

```bash
# .env 文件
OPENROUTER_API_KEY=sk-or-v1-xxxxxxxxxxxx

# 可选：应用标识
OPENROUTER_REFERER=https://your-app.com
OPENROUTER_TITLE=Your App Name
```

### 各语言环境变量读取

| 语言 | 方式 |
|------|------|
| **Node.js** | `process.env.OPENROUTER_API_KEY` |
| **Python** | `os.environ["OPENROUTER_API_KEY"]` |
| **Shell** | `$OPENROUTER_API_KEY` |
| **Go** | `os.Getenv("OPENROUTER_API_KEY")` |

---

## 5. 首次请求验证

### 5.1 快速验证清单

```bash
# 1. 验证 API Key 有效
curl -s https://openrouter.ai/api/v1/key \
  -H "Authorization: Bearer $OPENROUTER_API_KEY" | jq .

# 2. 查看可用模型列表
curl -s https://openrouter.ai/api/v1/models | jq '.data | length'

# 3. 发送测试请求（使用免费模型）
curl -s https://openrouter.ai/api/v1/chat/completions \
  -H "Authorization: Bearer $OPENROUTER_API_KEY" \
  -H "Content-Type: application/json" \
  -d '{
    "model": "openai/gpt-oss-20b:free",
    "messages": [{"role": "user", "content": "Say hello in 5 languages"}]
  }' | jq '.choices[0].message.content'
```

### 5.2 免费试用模型

无需充值即可使用 `:free` 变体模型：

| 模型 | 模型 ID | 说明 |
|------|---------|------|
| GPT OSS 20B | `openai/gpt-oss-20b:free` | OpenAI 开源小模型 |
| Llama 3.3 70B | `meta-llama/llama-3.3-70b-instruct:free` | Meta 开源旗舰 |
| Gemma 3 | `google/gemma-3-27b-it:free` | Google 开源模型 |
| DeepSeek V3 | `deepseek/deepseek-chat:free` | DeepSeek 对话模型 |

> **免费模型限制**：购买 < 10 Credits 时 50 次/天；≥ 10 Credits 时 1000 次/天；所有用户 20 次/分钟。

---

## 6. 请求构建器

OpenRouter 提供在线 **Request Builder** 工具，可交互式构建 API 请求：

1. 访问 [openrouter.ai/playground](https://openrouter.ai/playground)
2. 选择模型
3. 配置参数（temperature、max_tokens 等）
4. 生成 Python / TypeScript / Shell 代码

---

## 7. HTTP Headers 说明

| Header | 必需 | 说明 |
|--------|------|------|
| `Authorization` | 是 | `Bearer <API_KEY>` 格式 |
| `Content-Type` | 是 | `application/json` |
| `HTTP-Referer` | 否 | 应用 URL，用于社区排行榜 |
| `X-OpenRouter-Title` | 否 | 应用名称，显示在排行榜 |
| `X-OpenRouter-Categories` | 否 | 应用分类标签 |

---

## 8. 快速切换模型

OpenRouter 的核心优势是通过更改 `model` 参数即可切换到任意模型，无需修改其他代码：

```typescript
// 切换到 Claude
const claude = await openRouter.chat.send({
  model: 'anthropic/claude-sonnet-4.6',
  messages: [{ role: 'user', content: 'Hello' }],
  stream: false,
});

// 切换到 Gemini
const gemini = await openRouter.chat.send({
  model: 'google/gemini-3-pro-preview',
  messages: [{ role: 'user', content: 'Hello' }],
  stream: false,
});

// 使用自动路由（Auto Router 选择最优模型）
const auto = await openRouter.chat.send({
  model: 'openrouter/auto',
  messages: [{ role: 'user', content: 'Hello' }],
  stream: false,
});

// 使用高吞吐变体
const fast = await openRouter.chat.send({
  model: 'meta-llama/llama-3.3-70b-instruct:nitro',
  messages: [{ role: 'user', content: 'Hello' }],
  stream: false,
});
```

---

## 9. 常见问题

| 问题 | 解决方案 |
|------|---------|
| 401 Unauthorized | 检查 API Key 是否正确，确认 Bearer Token 格式 |
| 402 Payment Required | 余额不足，需充值 Credits |
| 429 Too Many Requests | 达到 Rate Limit，减少请求频率或升级 |
| 模型不可用 | 检查模型 ID 是否正确（需含 Provider 前缀如 `openai/`） |
| 响应格式不同 | OpenRouter 标准化为 OpenAI 格式，`choices` 始终为数组 |

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [01 - 概述与核心架构](./01-openrouter-overview-architecture.md) | 理解 OpenRouter 全貌 |
| [03 - 模型与 Provider 生态](./03-openrouter-models-providers.md) | 深入模型选型与定价 |
| [05 - API 参考](./05-openrouter-api-reference.md) | 完整请求/响应 Schema |
| [09 - 框架集成](./09-openrouter-frameworks-integrations.md) | 主流框架接入指南 |

---

*本文档基于 OpenRouter 官方文档（openrouter.ai/docs/quickstart）整理。*


<!-- risk-assessed -->
