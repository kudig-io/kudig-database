---
title: 工具体系与权限模型
description: '**文档类型**: 核心能力专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, Tools, Permissions,
  Custom Tools, bash, edit, read, grep, Security, TypeScript'
summary: '**文档类型**: 核心能力专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, Tools, Permissions,
  Custom Tools, bash, edit, read, grep, Security, TypeScript'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
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
- 工具体系与权限模型 是什么
- 如何 工具体系与权限模型
trigger_keywords:
- 工具体系与权限模型
- ai
- coding
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工具体系与权限模型

> **文档类型**: 核心能力专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, Tools, Permissions, Custom Tools, bash, edit, read, grep, [[安全/README.md|security]], TypeScript

---

## 概述

OpenCode 的工具体系是 Agent 从「纯 Chat」进化为「执行者」的关键。本文覆盖 14 个内置工具的功能与配置、Custom Tool 开发方法（TypeScript）、以及精细化权限控制模型——从全局通配到 Per-Agent 粒度的完整安全方案。

---

## 1. 内置工具全景

| 工具 | 功能 | 权限键 | 匹配粒度 |
|------|------|--------|---------|
| **bash** | 执行 Shell 命令 | `bash` | 匹配解析后的命令，如 `git status --porcelain` |
| **edit** | 精确字符串替换修改文件 | `edit` | 匹配文件路径 |
| **write** | 创建或覆盖文件 | `edit` | 匹配文件路径（与 edit 共享权限键） |
| **patch** | 应用 diff/patch 文件 | `edit` | 匹配文件路径（与 edit 共享权限键） |
| **read** | 读取文件内容（支持行范围） | `read` | 匹配文件路径 |
| **grep** | 正则搜索文件内容 | `grep` | 匹配正则模式 |
| **glob** | 模式匹配查找文件路径 | `glob` | 匹配 glob 模式 |
| **list** | 列出目录内容 | `list` | 匹配目录路径 |
| **lsp** | LSP 代码智能查询（实验性） | `lsp` | 非粒度控制 |
| **webfetch** | 获取网页内容 | `webfetch` | 匹配 URL |
| **websearch** | Web 搜索（通过 Exa AI） | `websearch` | 匹配查询词 |
| **todowrite** | 创建/更新任务列表 | `todowrite` | — |
| **todoread** | 读取任务列表 | `todoread` | — |
| **question** | 向用户提问（多选/自由输入） | `question` | — |
| **[[SKILL|skill]]** | 按需加载 SKILL.md | `skill` | 匹配 Skill 名称 |

> **底层实现**：grep、glob、list 底层使用 **ripgrep** 引擎，自动遵守 `.gitignore` 规则。websearch 通过 Exa AI 托管 MCP 服务实现，无需 API Key。

---

## 2. 权限模型

### 2.1 权限动作

| 动作 | 说明 | 用户体验 |
|------|------|---------|
| `"allow"` | 自动执行，无需确认 | 透明执行 |
| `"ask"` | 提示用户确认 | 可选 once/always/reject |
| `"deny"` | 阻止执行 | 静默拒绝 |

### 2.2 全局权限配置

```json
{
  "$schema": "https://opencode.ai/config.json",
  "permission": {
    "*": "ask",
    "read": "allow",
    "grep": "allow",
    "glob": "allow",
    "list": "allow",
    "bash": {
      "*": "ask",
      "git *": "allow",
      "npm *": "allow",
      "yarn *": "allow",
      "pnpm *": "allow",
      "grep *": "allow",
      "rm *": "deny",
      "sudo *": "deny"
    },
    "edit": {
      "*": "ask",
      "src/**": "allow",
      "*.env": "deny",
      "*.env.*": "deny"
    }
  }
}
```

规则匹配顺序：**最后匹配的规则优先**。通常将 `"*"` 通配放在第一行，更具体的规则放后面。

### 2.3 通配符语法

| 通配符 | 说明 | 示例 |
|--------|------|------|
| `*` | 匹配零个或多个任意字符 | `git *` 匹配 `git status`、`git commit -m "..."` |
| `?` | 匹配恰好一个字符 | `test?.js` 匹配 `test1.js` |
| `~` / `$HOME` | Home 目录展开 | `~/projects/*` → `/Users/name/projects/*` |

### 2.4 默认权限

| 权限 | 默认值 | 说明 |
|------|--------|------|
| 大多数工具 | `"allow"` | 默认允许，开发效率优先 |
| `doom_loop` | `"ask"` | 相同输入的工具调用重复 3 次时触发 |
| `external_directory` | `"ask"` | 访问工作目录外路径时触发 |
| `read` (`.env`) | `"deny"` | 保护敏感环境变量文件 |

```json
{
  "permission": {
    "read": {
      "*": "allow",
      "*.env": "deny",
      "*.env.*": "deny",
      "*.env.example": "allow"
    }
  }
}
```

### 2.5 外部目录访问

```json
{
  "permission": {
    "external_directory": {
      "~/projects/shared-lib/**": "allow"
    },
    "edit": {
      "~/projects/shared-lib/**": "deny"
    }
  }
}
```

> `external_directory` 允许的目录会继承工作区默认权限。建议额外限制 edit、bash 等写操作权限。

### 2.6 Per-Agent 权限

Agent 权限与全局配置**合并**，Agent 规则优先级更高：

```json
{
  "permission": {
    "bash": { "*": "ask", "git *": "allow" }
  },
  "agent": {
    "build": {
      "permission": {
        "bash": { "*": "ask", "git commit *": "ask", "git push *": "deny" }
      }
    },
    "plan": {
      "permission": {
        "edit": "deny",
        "bash": "ask"
      }
    }
  }
}
```

### 2.7 "Ask" 交互体验

当权限为 `"ask"` 时，用户有三个选项：

| 选项 | 说明 |
|------|------|
| **once** | 仅批准此次请求 |
| **always** | 批准匹配该模式的所有未来请求（当前会话内） |
| **reject** | 拒绝请求 |

---

## 3. Custom Tools 开发

### 3.1 文件位置

- 项目级：`.opencode/tools/`
- 全局级：`~/.config/opencode/tools/`

### 3.2 使用 tool() Helper

```typescript
// .opencode/tools/database.ts
import { tool } from "@opencode-ai/plugin"

export default tool({
  description: "Query the project database",
  args: {
    query: tool.schema.string().describe("SQL query to execute"),
  },
  async execute(args) {
    // 数据库查询逻辑
    return `Executed query: ${args.query}`
  },
})
```

文件名即为工具名：`database.ts` → `database` 工具。

### 3.3 多工具导出

```typescript
// .opencode/tools/math.ts
import { tool } from "@opencode-ai/plugin"

export const add = tool({
  description: "Add two numbers",
  args: {
    a: tool.schema.number().describe("First number"),
    b: tool.schema.number().describe("Second number"),
  },
  async execute(args) {
    return args.a + args.b
  },
})

export const multiply = tool({
  description: "Multiply two numbers",
  args: {
    a: tool.schema.number().describe("First number"),
    b: tool.schema.number().describe("Second number"),
  },
  async execute(args) {
    return args.a * args.b
  },
})
```

生成工具名：`math_add` 和 `math_multiply`（文件名 + `_` + 导出名）。

### 3.4 使用 Context

```typescript
// .opencode/tools/project.ts
import { tool } from "@opencode-ai/plugin"

export default tool({
  description: "Get project information",
  args: {},
  async execute(args, context) {
    const { agent, sessionID, messageID, directory, worktree } = context
    return `Agent: ${agent}, Directory: ${directory}, Worktree: ${worktree}`
  },
})
```

| Context 属性 | 说明 |
|-------------|------|
| `agent` | 当前执行的 Agent 名称 |
| `sessionID` | 当前会话 ID |
| `messageID` | 当前消息 ID |
| `directory` | 会话工作目录 |
| `worktree` | Git worktree 根目录 |

### 3.5 调用其他语言

```typescript
// .opencode/tools/python-add.ts
import { tool } from "@opencode-ai/plugin"
import path from "path"

export default tool({
  description: "Add two numbers using Python",
  args: {
    a: tool.schema.number().describe("First number"),
    b: tool.schema.number().describe("Second number"),
  },
  async execute(args, context) {
    const script = path.join(context.worktree, ".opencode/tools/add.py")
    const result = await Bun.$`python3 ${script} ${args.a} ${args.b}`.text()
    return result.trim()
  },
})
```

### 3.6 覆盖内置工具

自定义工具与内置工具同名时，自定义工具优先：

```typescript
// .opencode/tools/bash.ts — 替换内置 bash 工具
import { tool } from "@opencode-ai/plugin"

export default tool({
  description: "Restricted bash wrapper",
  args: { command: tool.schema.string() },
  async execute(args) {
    if (args.command.includes("rm -rf")) return "blocked: dangerous command"
    // 安全命令可执行
    return `blocked: ${args.command}`
  },
})
```

---

## 4. 安全最佳实践

| 实践 | 配置示例 |
|------|---------|
| **全局 ask 默认** | `"*": "ask"` |
| **bash 粒度控制** | 允许 `git *`、`npm *`；禁止 `rm *`、`sudo *`、`chmod *` |
| **`.env` 保护** | `read: { "*.env": "deny", "*.env.*": "deny", "*.env.example": "allow" }` |
| **doom_loop 保持 ask** | 防止 Agent 陷入重复调用死循环 |
| **外部目录最小化** | 仅允许必要路径，对外部路径禁用 edit |
| **Plan Agent 只读** | `edit: "deny", write: "deny", bash: "deny"` |
| **生产环境 Server 认证** | `OPENCODE_SERVER_PASSWORD` 保护 HTTP API |

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [04 - Agent 系统](./04-opencode-agents-system.md) | Per-Agent 权限配置 |
| [06 - MCP 集成](./06-opencode-mcp-integration.md) | MCP 工具的权限管理 |
| [12 - 进阶话题](./12-opencode-advanced-topics.md) | 安全加固策略 |

---

*本文档基于 OpenCode 官方文档（opencode.ai/docs/tools、opencode.ai/docs/permissions、opencode.ai/docs/custom-tools）整理。*


<!-- risk-assessed -->
