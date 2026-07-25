---
title: Agent 系统深度指南
description: '# Agent 系统深度指南'
summary: 'OpenCode 的 Agent 系统是其核心差异化能力之一。与其他 Agent CLI 的固定模式不同，OpenCode 提供了完整的 Agent 抽象：内置 Primary Agent（Build/Plan）和 Subagent（General/Explore），支持通过 JSON 或 Markdown 自定义 Agent，'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
- helm
- agent
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
- Agent 系统深度指南 是什么
- 如何 Agent 系统深度指南
trigger_keywords:
- Agent
- 系统深度指南
- ai
- coding
prerequisites:
- kubectl-basics
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Agent 系统深度指南

> **文档类型**: 核心能力专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, Agent, Build, Plan, Subagent, Custom Agent, Temperature, Max Steps, Markdown Agent

---

## 概述

OpenCode 的 Agent 系统是其核心差异化能力之一。与其他 Agent CLI 的固定模式不同，OpenCode 提供了完整的 Agent 抽象：内置 Primary Agent（Build/Plan）和 Subagent（General/Explore），支持通过 JSON 或 Markdown 自定义 Agent，每个 Agent 可独立配置模型、提示词、工具权限和温度参数。

---

## 1. Agent 类型体系

### 1.1 Primary Agent vs Subagent

| 类型 | 说明 | 交互方式 | 典型用例 |
|------|------|---------|---------|
| **Primary Agent** | 主对话 Agent，直接与用户交互 | `Tab` 键切换 | Build（全功能）、Plan（只读） |
| **Subagent** | 专用子 Agent，由 Primary Agent 调用或 `@` 引用 | `@agent-name` 或自动调用 | General（多步任务）、Explore（只读探索） |

```
┌─────────────────────────────────────────┐
│             Primary Agents              │
│  ┌──────────┐    ┌──────────┐           │
│  │  Build    │◄─►│   Plan   │  Tab 切换  │
│  │ (全功能)   │    │ (只读分析) │           │
│  └────┬─────┘    └──────────┘           │
│       │                                 │
│       ▼ 自动/手动调用                     │
│  ┌──────────┐    ┌──────────┐           │
│  │ General  │    │ Explore  │           │
│  │ (多步任务) │    │ (只读探索) │           │
│  └──────────┘    └──────────┘           │
│             Subagents                   │
└─────────────────────────────────────────┘
```

### 1.2 内置 Agent 详解

**Build（默认 Primary Agent）**：

- 所有工具启用（bash、edit、write、read、grep 等）
- 完整文件读写和 Shell 执行权限
- 适用于日常开发、代码生成、重构、Bug 修复

**Plan（Primary Agent）**：

- 文件编辑（edit）、写入（write）、bash 命令默认设为 `ask`（需确认）
- 适用于代码分析、方案设计、架构规划
- 不会意外修改代码库

**General（Subagent）**：

- 通用多步任务 Agent，拥有全部工具访问权限（除 todo）
- 适用于并行执行多个独立工作单元
- Primary Agent 可自动分派复杂子任务

**Explore（Subagent）**：

- 快速只读代码探索，**不能修改文件**
- 适用于按模式查找文件、搜索代码关键词、回答代码库相关问题

**隐藏系统 Agent**：

| Agent | 功能 | 触发方式 |
|-------|------|---------|
| **Compaction** | 长上下文自动压缩摘要 | token 达到 95% 时自动触发 |
| **Title** | 生成短会话标题 | 新会话首次交互后 |
| **Summary** | 创建会话摘要 | 手动或自动触发 |

---

## 2. Agent 使用方式

### 2.1 切换 Primary Agent

```
Tab          # 在 Build ↔ Plan 之间循环切换
Shift+Tab    # 反向切换
```

或使用 Agent 列表：`<Leader>+A`（默认 `Ctrl+X` → `A`）。

### 2.2 调用 Subagent

**手动 @ 引用**：

```
@general help me search for this function across all packages
@explore find all files that import the auth module
```

**自动调用**：Primary Agent 根据 Subagent 的 `description` 自动判断是否分派子任务。

### 2.3 子会话导航

Subagent 创建的子会话可通过快捷键导航：

| 操作 | 默认快捷键 |
|------|-----------|
| 进入第一个子会话 | `<Leader>+Down` |
| 下一个子会话 | `Right` |
| 上一个子会话 | `Left` |
| 返回父会话 | `Up` |

---

## 3. 自定义 Agent

### 3.1 JSON 方式

在 `opencode.json` 的 `agent` 字段定义：

```json
{
  "$schema": "https://opencode.ai/config.json",
  "agent": {
    "code-reviewer": {
      "description": "Reviews code for best practices and potential issues",
      "mode": "subagent",
      "model": "anthropic/claude-sonnet-4-20250514",
      "temperature": 0.1,
      "prompt": "You are a code reviewer. Focus on security, performance, and maintainability.",
      "permission": {
        "edit": "deny",
        "write": "deny",
        "bash": "deny"
      }
    },
    "k8s-ops": {
      "description": "[[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]] operations specialist for cluster management",
      "mode": "subagent",
      "model": "anthropic/claude-sonnet-4-20250514",
      "prompt": "{file:./prompts/k8s-ops.txt}",
      "steps": 20,
      "permission": {
        "bash": {
          "*": "ask",
          "kubectl *": "allow",
          "helm *": "allow",
          "rm *": "deny"
        }
      }
    },
    "fast-planner": {
      "description": "Quick planning with a fast model",
      "mode": "primary",
      "model": "anthropic/claude-haiku-4-20250514",
      "temperature": 0.1,
      "permission": {
        "edit": "deny",
        "write": "deny",
        "bash": "deny"
      }
    }
  }
}
```

### 3.2 Markdown 方式

将 Markdown 文件放在以下位置：

- 全局：`~/.config/opencode/agents/`
- 项目级：`.opencode/agents/`

**示例**：`.opencode/agents/review.md`

```markdown
---
description: Reviews code for quality and best practices
mode: subagent
model: anthropic/claude-sonnet-4-20250514
temperature: 0.1
permission:
  edit: deny
  write: deny
  bash: deny
---

You are in code review mode. Focus on:

- Code quality and best practices
- Potential bugs and edge cases
- Performance implications
- Security considerations

Provide constructive feedback without making direct changes.
```

文件名即为 Agent 名称：`review.md` → 使用 `@review` 调用。

---

## 4. 配置选项详解

| 选项 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `description` | string | ✅ | Agent 描述，影响自动调用决策和 TUI 展示 |
| `mode` | string | ✅ | `"primary"` 或 `"subagent"` |
| `model` | string | ❌ | 覆盖模型，格式 `provider/model-id` |
| `temperature` | number | ❌ | 0.0-1.0，控制随机性和创造性 |
| `steps` | number | ❌ | 最大迭代步数，达到后强制文本回复 |
| `prompt` | string | ❌ | 自定义 System Prompt，支持 `{file:path}` 文件引用 |
| `permission` | object | ❌ | Per-Agent 权限覆盖，合并全局配置 |
| `disable` | boolean | ❌ | 设为 `true` 禁用此 Agent |

### 4.1 Temperature 调优指南

| 范围 | 特性 | 适用场景 | 示例 Agent |
|------|------|---------|-----------|
| 0.0-0.2 | 高度确定性、精确 | 代码分析、Plan、代码审查、重构 | code-reviewer, plan |
| 0.3-0.5 | 平衡创造性与确定性 | 日常开发、Build 模式 | build |
| 0.6-1.0 | 高创造性、多样化 | 头脑风暴、探索性编程、文档生成 | brainstorm |

> Qwen 模型默认 Temperature 为 0.55，其他模型默认为 0。

### 4.2 Max Steps 限制

控制 Agent 的最大迭代次数，用于成本控制：

```json
{
  "agent": {
    "quick-fix": {
      "description": "Fast bug fixes with limited iterations",
      "mode": "subagent",
      "steps": 5,
      "prompt": "You are a quick fixer. Solve problems with minimal steps."
    }
  }
}
```

达到步数限制时，Agent 收到特殊 System Prompt，要求：
1. 总结已完成的工作
2. 列出推荐的后续任务

### 4.3 Prompt 文件引用

使用 `{file:path}` 语法引用外部文件作为 System Prompt，路径相对于配置文件位置：

```json
{
  "agent": {
    "review": {
      "prompt": "{file:./prompts/code-review.txt}"
    }
  }
}
```

---

## 5. 默认 Agent 配置

```json
{
  "default_agent": "plan"
}
```

- `default_agent` 必须是 **Primary Agent**（不能是 Subagent）
- 此设置影响所有界面：TUI、CLI（`opencode run`）、Desktop App、GitHub Action
- 如果指定的 Agent 不存在或是 Subagent，会回退到 `"build"` 并显示警告

---

## 6. 覆盖内置 Agent

可以覆盖 Build/Plan 等内置 Agent 的配置：

```json
{
  "agent": {
    "build": {
      "model": "anthropic/claude-sonnet-4-20250514",
      "prompt": "{file:./prompts/build.txt}",
      "permission": {
        "bash": {
          "*": "ask",
          "git *": "allow",
          "npm *": "allow"
        }
      }
    },
    "plan": {
      "model": "anthropic/claude-haiku-4-20250514"
    }
  }
}
```

---

## 7. 实战 Agent 设计模式

### 7.1 安全审计 Agent

```json
{
  "agent": {
    "security-audit": {
      "description": "Security-focused code analysis",
      "mode": "subagent",
      "model": "openai/o3",
      "temperature": 0.0,
      "prompt": "You are a security auditor. Focus on OWASP Top 10, injection vulnerabilities, authentication flaws, and sensitive data exposure. Never modify code.",
      "permission": {
        "edit": "deny",
        "write": "deny",
        "bash": "deny"
      }
    }
  }
}
```

### 7.2 文档生成 Agent

```json
{
  "agent": {
    "doc-writer": {
      "description": "Generate and update documentation",
      "mode": "subagent",
      "model": "anthropic/claude-sonnet-4-20250514",
      "temperature": 0.5,
      "prompt": "You are a technical writer. Generate clear, comprehensive documentation. Follow JSDoc/TSDoc conventions.",
      "permission": {
        "edit": { "*.md": "allow", "*.mdx": "allow", "*": "deny" },
        "bash": "deny"
      }
    }
  }
}
```

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [05 - 工具与权限](25-opencode-tools-permissions.md) | Agent 的工具和权限配置 |
| [08 - [[SKILL|Skill]] 与命令](28-opencode-skills-commands.md) | Agent 可加载的 Skill 定义 |
| [03 - Provider 与模型](23-opencode-providers-models.md) | Per-Agent 模型选择 |
| [12 - 进阶话题](32-opencode-advanced-topics.md) | 团队级 Agent 协作模式 |

---

*本文档基于 OpenCode 官方文档（opencode.ai/docs/agents）整理。*


<!-- risk-assessed -->
