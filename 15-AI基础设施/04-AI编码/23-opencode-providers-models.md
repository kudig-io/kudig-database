---
title: Provider 与模型管理
description: '# Provider 与模型管理'
summary: 'OpenCode 通过 **AI SDK** 和 **Models.dev** 支持 75+ LLM Provider。本文详细覆盖 Provider 接入配置、模型选型策略、OpenCode Zen/Go 托管服务、企业级 Provider（AWS Bedrock/Azure OpenAI）配置以及 Per-Agent 模型分配。'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
- gateway
- vllm
- llm
- agent
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
- Provider 与模型管理 是什么
- 如何 Provider 与模型管理
trigger_keywords:
- Provider
- 与模型管理
- ai
- coding
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Provider 与模型管理

> **文档类型**: 配置指南 | **最后更新**: 2026-03 | **关键词**: OpenCode, Provider, LLM Model, OpenCode Zen, AWS Bedrock, Azure OpenAI, GitHub Copilot, Groq, VertexAI

---

## 概述

OpenCode 通过 **AI SDK** 和 **Models.dev** 支持 75+ LLM Provider。本文详细覆盖 Provider 接入配置、模型选型策略、OpenCode Zen/Go 托管服务、企业级 Provider（AWS Bedrock/Azure OpenAI）配置以及 Per-Agent 模型分配。

---

## 1. Provider 体系架构

### 1.1 认证流程

```
/connect 命令
    ↓
选择 Provider → 认证方式（OAuth / API Key / 手动输入）
    ↓
凭证安全存储 → ~/.local/share/opencode/auth.json
```

### 1.2 Provider 配置结构

```json
{
  "$schema": "https://opencode.ai/config.json",
  "provider": {
    "anthropic": {
      "options": {
        "baseURL": "https://api.anthropic.com/v1",
        "timeout": 600000,
        "chunkTimeout": 30000,
        "setCacheKey": true
      }
    }
  },
  "model": "anthropic/claude-sonnet-4-20250514",
  "small_model": "anthropic/claude-haiku-4-20250514"
}
```

**配置要点**：

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `model` | 主模型，格式 `provider/model-id` | — |
| `small_model` | 轻量模型，用于标题生成等低成本任务 | 自动选择 |
| `timeout` | 请求超时（毫秒） | 300000 |
| `chunkTimeout` | 流式响应块间超时（毫秒） | — |
| `baseURL` | 自定义 API 端点（代理/私有部署） | — |
| `setCacheKey` | 确保始终设置缓存键 | false |

---

## 2. 主流 Provider 接入

### 2.1 OpenCode Zen（官方托管，推荐新手）

OpenCode 团队精选并验证的模型列表，提供统一账单管理：

```bash
# TUI 内操作
/connect → 选择 opencode → opencode.ai/auth → 登录 → 复制 API Key
/models  # 查看推荐模型
```

### 2.2 OpenCode Go（低成本订阅）

提供热门开源编码模型的可靠访问，适合预算敏感场景。配置方式与 Zen 一致。

### 2.3 Anthropic (Claude)

```bash
/connect → 选择 Anthropic
# 方式 1: Claude Pro/Max OAuth（浏览器认证）
# 方式 2: Create an API Key（浏览器生成 + 粘贴验证码）
# 方式 3: Manually enter API Key
```

支持模型：Claude 4 Sonnet、Claude 4 Opus、Claude 3.7 Sonnet、Claude 3.5 Sonnet、Claude 3.5 Haiku、Claude 3 Haiku、Claude 3 Opus。

> 注意：Claude Pro/Max 订阅在 OpenCode 中使用**非 Anthropic 官方支持**的方式。

### 2.4 OpenAI

```bash
export OPENAI_API_KEY="sk-..."
/connect → 选择 OpenAI → 输入 API Key
```

支持模型：GPT-4.1、GPT-4.1-mini、GPT-4.1-nano、GPT-4.5 Preview、GPT-4o 系列、O1/O3/O4 系列。

### 2.5 Google Gemini

```bash
export GEMINI_API_KEY="..."
/connect → 选择 Google → 输入 API Key
```

支持模型：Gemini 2.5、Gemini 2.5 Flash、Gemini 2.0 Flash、Gemini 2.0 Flash Lite。

### 2.6 GitHub Copilot

```bash
export GITHUB_TOKEN="ghp_..."
```

通过 Copilot 访问多个底层模型：GPT-4.1、Claude 3.7 Sonnet、Claude Sonnet 4、O1、O3 Mini、O4 Mini、Gemini 2.0 Flash、Gemini 2.5 Pro。

### 2.7 AWS Bedrock

```json
{
  "provider": {
    "amazon-bedrock": {
      "options": {
        "region": "us-east-1",
        "profile": "my-aws-profile",
        "endpoint": "https://bedrock-runtime.us-east-1.vpce-xxxxx.amazonaws.com"
      }
    }
  }
}
```

**认证方式优先级**：

1. Bearer Token（`AWS_BEARER_TOKEN_BEDROCK`）
2. AWS Credential Chain（Profile → Access Keys → Shared Credentials → IAM Roles → Web Identity Tokens → Instance Metadata）

支持 EKS IRSA（`AWS_WEB_IDENTITY_TOKEN_FILE` / `AWS_ROLE_ARN`）和 VPC Endpoint。

### 2.8 Azure OpenAI

```bash
export AZURE_RESOURCE_NAME=XXX  # Resource name
/connect → 搜索 Azure → 输入 API Key
```

需要先在 Azure AI Foundry 中部署模型。支持 Entra ID 认证（无需 API Key）。

### 2.9 Azure Cognitive Services

```bash
export AZURE_COGNITIVE_SERVICES_RESOURCE_NAME=XXX
/connect → 搜索 Azure Cognitive Services → 输入 API Key
```

### 2.10 Google Cloud VertexAI

```json
{
  "provider": {
    "google-vertex": {
      "options": {
        "project": "my-project-id",
        "location": "us-central1"
      }
    }
  }
}
```

支持模型：Gemini 2.5、Gemini 2.5 Flash。

### 2.11 Groq

```bash
/connect → 搜索 Groq → 输入 API Key
```

支持模型：Llama 4 Maverick (17b-128e)、Llama 4 Scout (17b-16e)、QWEN QWQ-32b、DeepSeek R1 distill Llama 70b、Llama 3.3 70b。

### 2.12 其他 Provider

OpenCode 还支持：302.AI、Baseten、Cerebras、Cloudflare AI Gateway、OpenRouter 等 75+ Provider。每个 Provider 通过 `/connect` 搜索并配置。

---

## 3. 完整支持模型矩阵

| Provider | 代表模型 |
|----------|---------|
| **OpenAI** | GPT-4.1, GPT-4.1-mini, GPT-4.1-nano, GPT-4.5 Preview, O1, O1-pro, O1-mini, O3, O3-mini, O4 Mini |
| **Anthropic** | Claude 4 Sonnet, Claude 4 Opus, Claude 3.7 Sonnet, Claude 3.5 Sonnet, Claude 3.5 Haiku |
| **Google** | Gemini 2.5, Gemini 2.5 Flash, Gemini 2.0 Flash, Gemini 2.0 Flash Lite |
| **GitHub Copilot** | GPT-4.1, Claude Sonnet 4, Claude 3.7 Sonnet (Thinking), O1, O3 Mini, O4 Mini, Gemini 2.5 Pro |
| **AWS Bedrock** | Claude 3.7 Sonnet |
| **Azure OpenAI** | GPT-4.1 系列, GPT-4.5 Preview, O1/O3 系列, O4 Mini |
| **VertexAI** | Gemini 2.5, Gemini 2.5 Flash |
| **Groq** | Llama 4 Maverick, Llama 4 Scout, QWQ-32b, DeepSeek R1 distill 70b, Llama 3.3 70b |

---

## 4. 模型选型策略

| 场景 | 推荐模型 | 理由 |
|------|---------|------|
| **日常开发（综合）** | Claude Sonnet 4 / GPT-4.1 | 编码质量与速度平衡 |
| **复杂架构设计** | Claude 4 Opus / O3 | 深度推理能力强 |
| **快速迭代 / Plan** | Claude Haiku 4 / GPT-4.1-mini | 速度快、成本低 |
| **大上下文窗口** | Gemini 2.5 (100万+ tokens) | 处理超大代码库 |
| **预算敏感** | Groq (Llama 4) / OpenCode Go | 开源模型、低成本 |
| **高速推理** | Cerebras (Qwen 3 Coder 480B) | 推理芯片加速 |
| **标题/摘要生成** | small_model（自动选择） | 无需手动配置 |

---

## 5. Per-Agent 模型配置

不同 Agent 可使用不同模型，实现成本与能力的精细化匹配：

```json
{
  "agent": {
    "build": {
      "model": "anthropic/claude-sonnet-4-20250514"
    },
    "plan": {
      "model": "anthropic/claude-haiku-4-20250514"
    },
    "code-reviewer": {
      "model": "openai/o3"
    },
    "explore": {
      "model": "anthropic/claude-haiku-4-20250514"
    }
  }
}
```

模型 ID 格式：`provider/model-id`。例如 OpenCode Zen 使用 `opencode/gpt-5.1-codex`。

---

## 6. 自定义 Base URL（代理/私有部署）

```json
{
  "provider": {
    "openai": {
      "options": {
        "baseURL": "https://my-proxy.example.com/v1"
      }
    }
  }
}
```

适用场景：企业代理网关、自托管 LLM（如 vLLM + OpenAI 兼容 API）、Cloudflare AI Gateway。

## 故障排查表

| 问题现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|--------|
| Provider 连接失败 | API Key 无效或网络不通 | `opencode --debug` 查看日志 | 检查环境变量和网络代理 |
| 模型响应超时 | 上游服务过载 | 检查 Provider 状态页 | 配置 timeout 和 retry |
| 模型切换后行为异常 | 不同模型提示词兼容性差异 | 对比不同模型的输出 | 为每个模型调整 system prompt |
| 自定义 Base URL 不生效 | 配置文件格式错误 | `cat ~/.config/opencode/config.json` | 检查 JSON 语法和字段名 |
| Token 计费异常 | 模型定价变更 | 查看 Provider 定价页 | 设置用量告警阈值 |

## 模型配置示例

```json
{
  "providers": {
    "openrouter": {
      "apiKey": "${OPENROUTER_API_KEY}",
      "models": {
        "default": "anthropic/claude-sonnet-4",
        "fast": "openai/gpt-4o-mini",
        "reasoning": "anthropic/claude-sonnet-4"
      }
    }
  },
  "agent": {
    "model": "default",
    "temperature": 0.1,
    "maxTokens": 8192
  }
}
```

## 关联文档

| 文档 | 关系 |
|------|------|
| [02 - 安装部署](22-opencode-installation-quickstart.md) | Provider 快速配置 |
| [04 - Agent 系统](24-opencode-agents-system.md) | Per-Agent 模型配置 |
| [12 - 进阶话题](32-opencode-advanced-topics.md) | 成本控制策略 |

---

## 版本兼容性

| OpenCode 版本 | 支持 Provider | 主要变化 |
|--------------|-------------|--------|
| 0.5+ | OpenRouter/Anthropic/OpenAI | Per-Agent 模型配置 |
| 0.4+ | + 自定义 Base URL | 私有部署支持 |
| 0.3+ | + Ollama | 本地模型支持 |
| 0.2+ | OpenRouter only | 初始多模型支持 |

*本文档基于 OpenCode 官方文档（opencode.ai/docs/providers）整理。*


<!-- risk-assessed -->
