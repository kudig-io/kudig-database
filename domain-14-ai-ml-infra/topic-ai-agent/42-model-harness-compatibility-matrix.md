---
title: 模型 × Harness 兼容性矩阵（2025-2026） [02-ai-agents]
description: 'description: ''**文档类型**: 实践参考指南 | **最后更新**: 2026-04 | **关键词**: Model
  Compatibility,'
summary: 'description: ''**文档类型**: 实践参考指南 | **最后更新**: 2026-04 | **关键词**: Model Compatibility,'
category: general
tags:
- ai
- ai-agent
- gpu
- vllm
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
estimated_read_time: 15min
intent_queries:
- 模型 × Harness 兼容性矩阵（2025-2026） 是什么
- 如何 模型 × Harness 兼容性矩阵（2025-2026）
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- 模型
- Harness
- 兼容性矩阵
- 2025-2026
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 模型 × Harness 兼容性矩阵（2025-2026）
description: '**文档类型**: 实践参考指南 | **最后更新**: 2026-04 | **关键词**: Model Compatibility,
  Harness Support, Function Calling, Tool Use, Structured Output, Agent Ready, GPT,
  Claude, Gemini, Qwen, DeepSeek, Llama'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- gpu
- vllm
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 10min
intent_queries:
- 模型 × Harness 兼容性矩阵（2025-2026） 是什么
- 如何 模型 × Harness 兼容性矩阵（2025-2026）
trigger_keywords:
- 模型
- Harness
- 兼容性矩阵
- 2025-2026
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

# 模型 × Harness 兼容性矩阵（2025-2026）

> **文档类型**: 实践参考指南 | **最后更新**: 2026-04 | **关键词**: Model Compatibility, Harness Support, Function Calling, Tool Use, Structured Output, Agent Ready, GPT, Claude, Gemini, Qwen, DeepSeek, Llama

---

## 概述

Agent Harness 对底层模型有明确的能力要求：**不是所有模型都能驱动完整的六层 Harness**。模型必须支持工具调用（Function Calling）、结构化输出、系统提示词、流式输出等关键能力，才能作为 Harness 的"引擎"可靠运行。

本文提供 2025-2026 年主流模型的 Harness 兼容性完整清单，帮助工程师快速判断哪些模型可以直接用于 Harness 架构、哪些需要额外适配、哪些不适合。

---

## 1. Harness 对模型的六项核心能力要求

```
模型必须支持的 Harness 关键能力:

1. Function Calling / Tool Use（工具调用）        ← Layer 2: Tools
   模型原生支持函数调用，能生成结构化的工具调用请求
   重要性: ★★★★★（必须）

2. Structured Output（结构化输出）                ← Layer 5: Verification
   模型能输出 JSON 等结构化格式，便于验证层解析
   重要性: ★★★★★（必须）

3. System Prompt（系统提示词）                    ← Layer 6: Constraints
   支持 system role 消息，用于注入约束和行为规则
   重要性: ★★★★★（必须）

4. Streaming（流式输出）                          ← Layer 1: Loop
   支持 SSE 流式响应，用于实时 UI 和长运行任务
   重要性: ★★★★☆（强烈推荐）

5. Large Context Window（大上下文窗口 ≥128K）     ← Layer 3: Context
   足够的上下文空间容纳工具输出、历史轨迹、RAG 结果
   重要性: ★★★★☆（强烈推荐）

6. Parallel Tool Calls（并行工具调用）             ← Layer 2: Tools
   单次推理中同时调用多个工具，提升执行效率
   重要性: ★★★☆☆（推荐）
```

---

## 2. 闭源商业模型 Harness 兼容性清单

### 2.1 OpenAI 系列

| 模型 | 版本/发布时间 | Function Calling | Structured Output | System Prompt | Streaming | 上下文窗口 | 并行工具调用 | Harness 就绪度 |
|------|-------------|-----------------|-------------------|---------------|-----------|-----------|------------|--------------|
| **GPT-4.1** | 2025-04 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 1M | ✅ | ★★★★★ |
| **GPT-4.1 mini** | 2025-04 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 1M | ✅ | ★★★★★ |
| **GPT-4.1 nano** | 2025-04 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 1M | ✅ | ★★★★☆ |
| **GPT-4o** | 2024-05 (11月更新) | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 128K | ✅ | ★★★★★ |
| **GPT-4o mini** | 2024-07 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 128K | ✅ | ★★★★★ |
| **o3** | 2025-04 | ✅ 原生 | ✅ | ✅ | ✅ | 200K | ✅ | ★★★★★ |
| **o4-mini** | 2025-04 | ✅ 原生 | ✅ | ✅ | ✅ | 200K | ✅ | ★★★★★ |
| **o3-mini** | 2025-01 | ✅ 原生 | ✅ | ✅ | ✅ | 200K | ✅ | ★★★★☆ |
| **o1** | 2024-09 | ⚠️ 有限 | ✅ | ⚠️ developer | ✅ | 200K | ❌ | ★★★☆☆ |
| **GPT-5** | 2025-05~ | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 400K | ✅ | ★★★★★ |
| **GPT-5.2** | 2025末~ | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 400K | ✅ | ★★★★★ |

**OpenAI 系列 Harness 特点**：
- GPT-4.1 系列是 **Harness 最佳选择之一**：1M 上下文 + 原生 Function Calling + Structured Output
- o3/o4-mini 的 **reasoning 模型也已完整支持工具调用**，适合需要深度推理的 Harness
- o1 的工具调用能力有限（不支持并行），不推荐作为 Harness 主引擎
- GPT-5 系列统一了推理和工具调用能力，是 2025 下半年的 Harness 旗舰

### 2.2 Anthropic 系列

| 模型 | 版本/发布时间 | Function Calling | Structured Output | System Prompt | Streaming | 上下文窗口 | 并行工具调用 | Harness 就绪度 |
|------|-------------|-----------------|-------------------|---------------|-----------|-----------|------------|--------------|
| **Claude Sonnet 4** | 2025-05 | ✅ 原生 | ✅ Tool Use | ✅ | ✅ | 200K | ✅ | ★★★★★ |
| **Claude Opus 4** | 2025-05 | ✅ 原生 | ✅ Tool Use | ✅ | ✅ | 200K | ✅ | ★★★★★ |
| **Claude 3.7 Sonnet** | 2025-02 | ✅ 原生 | ✅ Tool Use | ✅ | ✅ | 200K | ✅ | ★★★★★ |
| **Claude 3.5 Sonnet** | 2024-06 (10月更新) | ✅ 原生 | ✅ Tool Use | ✅ | ✅ | 200K | ✅ | ★★★★★ |
| **Claude 3.5 Haiku** | 2024-10 | ✅ 原生 | ✅ Tool Use | ✅ | ✅ | 200K | ✅ | ★★★★★ |
| **Claude Sonnet 4.5** | 2025末 | ✅ 原生 | ✅ | ✅ | ✅ | 200K | ✅ | ★★★★★ |
| **Claude Sonnet 4.6** | 2026 | ✅ 原生 | ✅ | ✅ | ✅ | 200K | ✅ | ★★★★★ |
| **Claude Opus 4.6** | 2026 | ✅ 原生 | ✅ | ✅ | ✅ | 200K | ✅ | ★★★★★ |

**Anthropic 系列 Harness 特点**：
- Claude 全系列从 3.5 开始 **全面支持 Harness 所需能力**
- Claude 的 **Extended Thinking 模式** 天然适合 Harness 的 Verification 层（自检推理）
- Claude Code（命令行 Agent）本身就是 **Harness 架构的标杆实现**
- SWE-bench 得分（80.8%）在 Harness 敏感基准上最高，证明 Harness × Claude 组合效果最佳

### 2.3 Google 系列

| 模型 | 版本/发布时间 | Function Calling | Structured Output | System Prompt | Streaming | 上下文窗口 | 并行工具调用 | Harness 就绪度 |
|------|-------------|-----------------|-------------------|---------------|-----------|-----------|------------|--------------|
| **Gemini 2.5 Pro** | 2025-03 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 1M | ✅ | ★★★★★ |
| **Gemini 2.5 Flash** | 2025-04 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 1M | ✅ | ★★★★★ |
| **Gemini 2.0 Flash** | 2025-01 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 1M | ✅ | ★★★★☆ |
| **Gemini 1.5 Pro** | 2024 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 2M | ✅ | ★★★★☆ |
| **Gemini 3.0 / 3.1 Pro** | 2026 | ✅ 原生 | ✅ | ✅ | ✅ | 2M | ✅ | ★★★★★ |

**Google 系列 Harness 特点**：
- **上下文窗口最大**（1M-2M），天然适合 Harness Context 层需要大量信息的场景
- Gemini 2.5 Pro 的 **Thinking 模式** 类似 Claude Extended Thinking，增强自检能力
- Gemini 2.5 Flash 是 **性价比最高的 Harness 引擎**（$0.15/$0.60 per 1M tokens）
- GPQA Diamond 94.3%（Gemini 3.1 Pro），科学推理能力最强

### 2.4 xAI 系列

| 模型 | 版本/发布时间 | Function Calling | Structured Output | System Prompt | Streaming | 上下文窗口 | 并行工具调用 | Harness 就绪度 |
|------|-------------|-----------------|-------------------|---------------|-----------|-----------|------------|--------------|
| **Grok 3** | 2025-02 | ✅ 原生 | ✅ | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **Grok 4** | 2026 | ✅ 原生 | ✅ | ✅ | ✅ | 256K+ | ✅ | ★★★★★ |

---

## 3. 开源 / 半开源模型 Harness 兼容性清单

### 3.1 DeepSeek 系列

| 模型 | 版本/发布时间 | Function Calling | Structured Output | System Prompt | Streaming | 上下文窗口 | 并行工具调用 | Harness 就绪度 |
|------|-------------|-----------------|-------------------|---------------|-----------|-----------|------------|--------------|
| **DeepSeek-V3** | 2024-12 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **DeepSeek-R1** | 2025-01 | ⚠️ 有限 | ⚠️ 不稳定 | ✅ | ✅ | 128K | ❌ | ★★☆☆☆ |
| **DeepSeek-R1-Distill 系列** | 2025-01 | ⚠️ 有限 | ⚠️ 不稳定 | ✅ | ✅ | 128K | ❌ | ★★☆☆☆ |
| **DeepSeek V3.2** | 2026 | ✅ 增强 | ✅ | ✅ | ✅ | 128K | ✅ | ★★★★☆ |

**DeepSeek 系列 Harness 特点**：
- DeepSeek-V3 **性价比极高**（$0.27/$1.1 per 1M），适合成本敏感的 Harness 部署
- DeepSeek-R1 **不推荐作为 Harness 引擎**：工具调用不稳定，结构化输出差，适合纯推理任务
- DeepSeek V3.2 修复了工具调用问题，SWE-bench 67.8%，可用于生产 Harness
- 中文能力 ★★★★★，**国内场景 Harness 首选开源模型**

### 3.2 阿里 Qwen 系列

| 模型 | 版本/发布时间 | Function Calling | Structured Output | System Prompt | Streaming | 上下文窗口 | 并行工具调用 | Harness 就绪度 |
|------|-------------|-----------------|-------------------|---------------|-----------|-----------|------------|--------------|
| **Qwen2.5-72B-Instruct** | 2024-09 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **Qwen2.5-32B-Instruct** | 2024-09 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **Qwen2.5-14B-Instruct** | 2024-09 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 128K | ✅ | ★★★☆☆ |
| **Qwen2.5-7B-Instruct** | 2024-09 | ✅ 原生 | ⚠️ 不够稳定 | ✅ | ✅ | 128K | ⚠️ 有限 | ★★★☆☆ |
| **Qwen2.5-Coder-32B** | 2024-11 | ✅ 原生 | ✅ | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **Qwen3-235B (MoE)** | 2025-04 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 128K | ✅ | ★★★★★ |
| **Qwen3-32B** | 2025-04 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **Qwen3.5-397B** | 2026-03 | ✅ 原生 | ✅ | ✅ | ✅ | 128K | ✅ | ★★★★★ |

**Qwen 系列 Harness 特点**：
- Qwen3 系列 **原生支持 MCP 协议**，与 Harness Tools 层的工具注册发现机制天然匹配
- Qwen2.5-72B 和 Qwen3-235B 是 **私有化部署 Harness 的最优选择**（中文 ★★★★★ + 工具调用可靠）
- 7B 以下模型工具调用不够稳定，不推荐用于生产级 Harness
- Qwen3.5-397B SWE-bench 76.4%，**开源模型 Harness 能力天花板**
- 通过阿里云百炼 API 调用时与 AgentScope 集成最深

### 3.3 Meta Llama 系列

| 模型 | 版本/发布时间 | Function Calling | Structured Output | System Prompt | Streaming | 上下文窗口 | 并行工具调用 | Harness 就绪度 |
|------|-------------|-----------------|-------------------|---------------|-----------|-----------|------------|--------------|
| **Llama 3.3 70B** | 2024-12 | ✅ 原生 | ✅ | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **Llama 3.1 405B** | 2024-07 | ✅ 原生 | ✅ | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **Llama 3.1 70B** | 2024-07 | ✅ 原生 | ✅ | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **Llama 3.1 8B** | 2024-07 | ⚠️ 有限 | ⚠️ 不稳定 | ✅ | ✅ | 128K | ❌ | ★★☆☆☆ |
| **Llama 4 Scout (17B MoE)** | 2025-04 | ✅ 原生 | ✅ | ✅ | ✅ | 10M | ✅ | ★★★★☆ |
| **Llama 4 Maverick (17B MoE)** | 2025-04 | ✅ 原生 | ✅ | ✅ | ✅ | 1M | ✅ | ★★★★☆ |

**Llama 系列 Harness 特点**：
- Llama 4 Scout 拥有 **10M 上下文窗口**，是 Harness Context 层的极限选择
- Llama 3.1 8B 工具调用不够可靠，不推荐用于 Harness 主引擎
- Llama 系列在 **英文场景下表现优异**，中文能力弱于 Qwen/DeepSeek

### 3.4 Mistral 系列

| 模型 | 版本/发布时间 | Function Calling | Structured Output | System Prompt | Streaming | 上下文窗口 | 并行工具调用 | Harness 就绪度 |
|------|-------------|-----------------|-------------------|---------------|-----------|-----------|------------|--------------|
| **Mistral Large 2** | 2024-07 | ✅ 原生 | ✅ JSON Mode | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **Mistral Nemo (12B)** | 2024-07 | ✅ 原生 | ✅ | ✅ | ✅ | 128K | ✅ | ★★★☆☆ |
| **Mistral Small 3.1 (24B)** | 2025 | ✅ 原生 | ✅ | ✅ | ✅ | 128K | ✅ | ★★★★☆ |
| **Codestral (22B)** | 2024 | ✅ 原生 | ✅ | ✅ | ✅ | 32K | ✅ | ★★★☆☆ |

**Mistral 系列 Harness 特点**：
- Mistral 的 Function Calling 实现质量高，**开箱即用无需特殊 Prompt**
- Mistral Small 3.1 是 **轻量级 Harness 的好选择**（24B 参数，单卡可跑）

### 3.5 其他值得关注的模型

| 模型 | 厂商 | 发布时间 | Function Calling | 上下文窗口 | Harness 就绪度 | 备注 |
|------|------|---------|-----------------|-----------|--------------|------|
| **GLM-5** | 智谱 | 2026 | ✅ | 128K | ★★★★☆ | SWE-bench 77.8%，国产新锐 |
| **Kimi K2.5** | Moonshot | 2026 | ✅ | 128K | ★★★★☆ | SWE-bench 76.8%，工具调用能力强 |
| **MiniMax M2.5** | MiniMax | 2026 | ✅ | 128K | ★★★★☆ | SWE-bench 80.2%，开源 Harness 潜力股 |
| **Step-3.5-Flash** | 阶跃星辰 | 2026 | ✅ | 128K | ★★★☆☆ | SWE-bench 74.4%，推理能力强 |
| **Gemma 3 (27B)** | Google | 2025 | ⚠️ 有限 | 128K | ★★☆☆☆ | 轻量级，工具调用能力有限 |
| **Phi-4 (14B)** | Microsoft | 2025 | ⚠️ 有限 | 16K | ★★☆☆☆ | 端侧模型，上下文窗口小 |

---

## 4. Harness 场景选型推荐

### 4.1 按 Harness 层级需求选模型

| Harness 层级需求 | 推荐模型（API） | 推荐模型（自部署） | 原因 |
|-----------------|----------------|------------------|------|
| **Loop 层（高频调用）** | GPT-4o-mini / Gemini 2.5 Flash | Qwen3-32B / Mistral Small 3.1 | 低延迟 + 低成本 + 工具调用可靠 |
| **Tools 层（复杂工具编排）** | Claude Sonnet 4 / GPT-4.1 | Qwen3-235B / DeepSeek V3.2 | 工具选择准确率最高 |
| **Context 层（超长上下文）** | Gemini 2.5 Pro (1M) / GPT-4.1 (1M) | Llama 4 Scout (10M) | 大上下文窗口 |
| **Verification 层（自检推理）** | Claude Opus 4 / o3 | DeepSeek-R1（纯推理验证） | 深度推理 + 自我纠错能力强 |
| **Constraints 层（指令遵循）** | GPT-4o / Claude Sonnet 4 | Qwen2.5-72B | 指令遵循能力 ★★★★★ |

### 4.2 按部署场景选模型

| 场景 | 推荐模型 | 备选 | 成本参考 |
|------|---------|------|---------|
| **云端 API（快速启动）** | Claude Sonnet 4 | GPT-4.1, Gemini 2.5 Pro | $3-15/1M tokens |
| **云端 API（成本敏感）** | Gemini 2.5 Flash | GPT-4o-mini, DeepSeek-V3 | $0.1-0.6/1M tokens |
| **私有化部署（国内合规）** | Qwen3-235B / Qwen2.5-72B | DeepSeek V3.2 | GPU 成本（4-8×A100） |
| **私有化部署（国际）** | Llama 4 Maverick | Mistral Large 2 | GPU 成本 |
| **轻量级 / 边缘部署** | Qwen3-32B | Mistral Small 3.1, Llama 3.3 70B | 单卡 / 双卡 |
| **K8S 运维 Agent（中文）** | Qwen3-235B (API) | DeepSeek V3.2 | 自部署或百炼 API |
| **K8S 运维 Agent（英文）** | Claude Sonnet 4 | GPT-4.1 | API 调用 |

### 4.3 多模型路由策略（Harness 最佳实践）

```
Harness 多模型路由架构:

┌─────────────────────────────────────────────────────┐
│                   Model Router                       │
│                                                      │
│  任务输入 → 复杂度评估 → 模型选择 → 执行 → 结果     │
│                                                      │
│  路由规则:                                            │
│                                                      │
│  ├── 简单查询 / 高频调用                              │
│  │   └── Gemini 2.5 Flash / GPT-4o-mini              │
│  │       成本: ~$0.15/1M tokens, 延迟: <0.5s         │
│  │                                                    │
│  ├── 中等复杂度 / 工具编排                             │
│  │   └── Claude Sonnet 4 / GPT-4.1                   │
│  │       成本: ~$3-5/1M tokens, 延迟: ~1s            │
│  │                                                    │
│  ├── 深度推理 / 复杂分析（延迟不敏感）                 │
│  │   └── Claude Opus 4 / o3 / DeepSeek-R1            │
│  │       成本: ~$10-15/1M tokens, 延迟: 5-60s        │
│  │                                                    │
│  └── 超长上下文 / 全量日志分析                         │
│      └── Gemini 2.5 Pro (1M) / Llama 4 Scout (10M)  │
│          适合需要处理大量上下文的诊断任务              │
│                                                      │
│  Fallback 链:                                         │
│  主模型 → 备选模型 → 降级模型                         │
│  Claude Sonnet 4 → GPT-4.1 → Gemini 2.5 Flash       │
└─────────────────────────────────────────────────────┘
```

---

## 5. 不支持 / 不推荐用于 Harness 的模型

| 模型 | 原因 | 替代建议 |
|------|------|---------|
| **DeepSeek-R1** | 工具调用不稳定，结构化输出差 | 用 DeepSeek-V3/V3.2 替代；R1 仅用于 Verification 层的纯推理验证 |
| **DeepSeek-R1-Distill 系列** | 蒸馏模型工具调用能力更弱 | 用 Qwen3-32B 或 Llama 3.3 70B 替代 |
| **o1 (初始版本)** | 不支持并行工具调用，system prompt 限制 | 升级到 o3 或 o4-mini |
| **Llama 3.1 8B** | 小参数模型工具调用不可靠 | 至少使用 70B 或 Llama 4 Scout |
| **Qwen2.5-7B** | 结构化输出不稳定 | 至少使用 14B 或 32B |
| **Phi-4 (14B)** | 上下文窗口仅 16K，工具调用有限 | 用 Mistral Nemo 12B 或 Qwen3-32B 替代 |
| **Gemma 3 (27B)** | 工具调用能力有限 | 用 Qwen3-32B 或 Mistral Small 3.1 替代 |
| **纯 Embedding 模型** | 无生成能力 | 仅用于 RAG 检索，不能驱动 Harness |

---

## 6. 版本演进时间线

```
2024-2026 模型 Harness 支持演进:

2024 Q2  GPT-4o                  ← 第一批成熟 Harness 引擎
         Claude 3.5 Sonnet

2024 Q3  Llama 3.1 (8B/70B/405B) ← 开源模型 Function Calling 成熟
         Qwen2.5 系列
         Mistral Large 2

2024 Q4  Claude 3.5 Sonnet V2   ← SWE-bench 最佳成绩
         GPT-4o-mini
         DeepSeek-V3
         Llama 3.3 70B

2025 Q1  DeepSeek-R1             ← 推理强但工具调用弱（不适合 Harness 主引擎）
         Gemini 2.0 Flash
         o3-mini
         Claude 3.7 Sonnet

2025 Q2  GPT-4.1 系列 (1M)      ← Harness 引擎大升级：1M 上下文
         Gemini 2.5 Pro/Flash
         Claude Sonnet 4 / Opus 4
         o3 / o4-mini
         Qwen3 系列              ← 原生 MCP 支持
         Llama 4 Scout/Maverick  ← 10M 上下文
         GPT-5

2025 下半年
         GPT-5.2
         Claude 4.5 Sonnet

2026 Q1  Gemini 3.0 / 3.1 Pro   ← GPQA 94.3%
         Qwen3.5-397B            ← 开源 Harness 能力天花板
         DeepSeek V3.2           ← 修复工具调用
         GLM-5, Kimi K2.5        ← 国产新锐崛起
         Claude 4.6 系列
         Grok 4
         MiniMax M2.5            ← 开源 SWE-bench 80.2%

趋势:
  ✓ 2024: Function Calling 从实验走向稳定
  ✓ 2025: 所有顶级模型全面支持 Harness 所需能力
  ✓ 2026: 模型间 Harness 能力差距收窄，Harness 设计比模型选择更重要
```

---

## 7. AgentScope 框架兼容性

在 AgentScope 框架中使用 Harness 时，模型与 Formatter 的匹配关系：

| 模型提供商 | AgentScope Model 类 | Formatter 类 | Harness 就绪 |
|-----------|---------------------|-------------|-------------|
| 阿里云百炼 (Qwen) | `DashScopeChatModel` | `DashScopeChatFormatter` | ✅ 最佳集成 |
| OpenAI (GPT) | `OpenAIChatModel` | `OpenAIChatFormatter` | ✅ |
| Anthropic (Claude) | `AnthropicChatModel` | `AnthropicChatFormatter` | ✅ |
| 本地 Ollama | `OllamaChatModel` | `OllamaChatFormatter` | ✅ (取决于模型) |
| vLLM (OpenAI 兼容) | `OpenAIChatModel` | `OpenAIChatFormatter` | ✅ |

```python
# AgentScope 中切换不同模型驱动 Harness
from agentscope.agent import ReActAgent

# 方案 1: 阿里云百炼（国内推荐）
from agentscope.model import DashScopeChatModel
from agentscope.formatter import DashScopeChatFormatter

agent = ReActAgent(
    name="K8s-Expert",
    model=DashScopeChatModel(model_name="qwen-max", stream=True, ...),
    formatter=DashScopeChatFormatter(),
    ...
)

# 方案 2: OpenAI
from agentscope.model import OpenAIChatModel
from agentscope.formatter import OpenAIChatFormatter

agent = ReActAgent(
    name="K8s-Expert",
    model=OpenAIChatModel(model_name="gpt-4.1", stream=True, ...),
    formatter=OpenAIChatFormatter(),
    ...
)

# 方案 3: 自部署 vLLM（OpenAI 兼容协议）
agent = ReActAgent(
    name="K8s-Expert",
    model=OpenAIChatModel(
        model_name="qwen2.5-72b",
        base_url="http://vllm-service:8000/v1",
        stream=True,
        ...
    ),
    formatter=OpenAIChatFormatter(),
    ...
)
```

---

## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [02 - LLM 基座模型选型与评估](./02-llm-foundation-models.md) | 模型全维度性能对比、成本分析、部署指南 |
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | Harness 六层架构定义、对模型的能力要求 |
| [32 - Harness 工具工程](./32-agent-harness-tool-engineering.md) | 工具层 Schema 标准、Function Calling 最佳实践 |
| [38 - Harness 性能与成本优化](./38-agent-harness-performance-cost.md) | 多模型路由、成本控制策略 |
| [41 - ReAct Agent 与 Harness 识别指南](./41-react-harness-identification-guide.md) | Harness 判断标准和成熟度模型 |
| [17 - AgentScope 核心概念](./17-agentscope-core-concepts.md) | AgentScope 中模型与 Formatter 的匹配 |

---

## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| Iternal Technologies | 《The Definitive LLM Selection & Benchmarks Guide》2026 版 | 2026-03 |
| Creole Studios | 《Top LLMs to Use in 2026》 | 2025-12 |
| LMSYS Chatbot Arena | Arena Elo 排行榜 | 2026-03 |
| OpenAI | GPT-4.1 / o3 / GPT-5 系列发布文档 | 2025 |
| Anthropic | Claude Sonnet 4 / Opus 4 发布文档 | 2025-05 |
| Google | Gemini 2.5 系列发布文档 | 2025-03 |
| 阿里云 | Qwen3 / Qwen3.5 技术报告 | 2025-2026 |
| Birgitta Böckeler (Martin Fowler) | 《Harness Engineering》 | 2026-02 |

---

*本文档为 kudig-database 项目 topic-ai-agent 系列原创内容，基于 2025-2026 行业最新数据，提供模型 Harness 兼容性的全量参考清单。*

---

## Obsidian 相关文档

- topic-ai-agent KUDIG Database — Global MOC
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

- 40-agent-harness-production-maturity
- 41-react-harness-identification-guide
- 43-openclaw-framework-integration
- 44-openclaw-soul-mechanism


<!-- risk-assessed -->
