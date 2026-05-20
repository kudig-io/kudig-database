---
title: Agent Skill 与自定义命令
description: '**文档类型**: 扩展开发专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, Skill, SKILL.md, Custom Command, Template, Arguments,
  Shell Output, File Reference'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
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
- Agent Skill 与自定义命令 是什么
- 如何 Agent Skill 与自定义命令
trigger_keywords:
- Agent
- Skill
- 与自定义命令
- ai
- coding
---

# Agent Skill 与自定义命令

> **文档类型**: 扩展开发专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, Skill, SKILL.md, Custom Command, Template, Arguments, Shell Output, File Reference

---

## 概述

OpenCode 提供两套互补的扩展机制：**Agent Skill**（可复用行为定义，Agent 按需加载）和 **Custom Command**（模板化提示词命令，用户直接触发），使团队能够标准化工作流、沉淀最佳实践，并在项目级和全局级共享。

---

## 1. Agent Skill 系统

### 1.1 什么是 Skill

Skill 是通过 `SKILL.md` 文件定义的可复用指令集。Agent 通过内置 `skill` 工具**按需加载**——不预加载全部 Skill 内容到上下文中，而是在 Agent 判断需要时主动调用。

### 1.2 文件位置与搜索路径

| 位置 | 路径 |
|------|------|
| 项目级 | `.opencode/skills/<name>/SKILL.md` |
| 全局级 | `~/.config/opencode/skills/<name>/SKILL.md` |
| Claude 兼容 | `.claude/skills/<name>/SKILL.md` |
| Claude 全局兼容 | `~/.claude/skills/<name>/SKILL.md` |
| Agent 兼容 | `.agents/skills/<name>/SKILL.md` |
| Agent 全局兼容 | `~/.agents/skills/<name>/SKILL.md` |

项目级路径从当前工作目录向上遍历至 Git worktree 根目录，沿途加载匹配的 Skill。

### 1.3 SKILL.md 规范

```markdown
---
name: git-release
description: Create consistent releases and changelogs
license: MIT
compatibility: opencode
metadata:
  audience: maintainers
  workflow: github
---

## What I do
- Draft release notes from merged PRs
- Propose a version bump
- Provide a copy-pasteable `gh release create` command

## When to use me
Use this when you are preparing a tagged release.
Ask clarifying questions if the target versioning scheme is unclear.
```

**Frontmatter 字段**：

| 字段 | 必填 | 说明 |
|------|------|------|
| `name` | ✅ | Skill 名称，1-64 字符 |
| `description` | ✅ | 描述，1-1024 字符，足够具体以供 Agent 选择 |
| `license` | ❌ | 许可证 |
| `compatibility` | ❌ | 兼容性标识 |
| `metadata` | ❌ | 自定义字符串键值对 |

### 1.4 命名规则

- 1–64 字符
- 小写字母数字，单连字符分隔
- 不能以 `-` 开头或结尾
- 不能包含连续 `--`
- 必须与包含 `SKILL.md` 的目录名一致
- 正则：`^[a-z0-9]+(-[a-z0-9]+)*$`

### 1.5 发现与加载机制

OpenCode 在 `skill` 工具描述中列出所有可用 Skill：

```xml
<available_skills>
  <skill>
    <name>git-release</name>
    <description>Create consistent releases and changelogs</description>
  </skill>
  <skill>
    <name>k8s-debug</name>
    <description>Debug Kubernetes pod issues</description>
  </skill>
</available_skills>
```

Agent 根据任务需求按需调用：`skill({ name: "git-release" })`。

### 1.6 Skill 权限配置

```json
{
  "permission": {
    "skill": {
      "*": "allow",
      "pr-review": "allow",
      "internal-*": "deny",
      "experimental-*": "ask"
    }
  }
}
```

Per-Agent Skill 权限：

```json
{
  "agent": {
    "plan": {
      "permission": {
        "skill": { "internal-*": "allow" }
      }
    }
  }
}
```

Markdown Agent 格式：

```markdown
---
permission:
  skill:
    "documents-*": "allow"
---
```

### 1.7 禁用 Skill 工具

```json
// JSON 配置
{ "agent": { "plan": { "tools": { "skill": false } } } }
```

```markdown
---
tools:
  skill: false
---
```

### 1.8 Skill 排查

如果 Skill 未显示：

1. 确认 `SKILL.md` 拼写为全大写
2. 检查 frontmatter 包含 `name` 和 `description`
3. 确保 Skill 名称在所有位置中唯一
4. 检查权限——`deny` 的 Skill 对 Agent 不可见

---

## 2. 自定义命令系统

### 2.1 命令定义方式

**方式一：Markdown 文件（推荐）**

文件位置：
- 全局：`~/.config/opencode/commands/`
- 项目级：`.opencode/commands/`

`.opencode/commands/test.md`：

```markdown
---
description: Run tests with coverage
agent: build
model: anthropic/claude-sonnet-4-20250514
---

Run the full test suite with coverage report and show any failures.
Focus on the failing tests and suggest fixes.
```

**方式二：JSON 配置**

```json
{
  "command": {
    "test": {
      "template": "Run the full test suite with coverage report and show any failures.\nFocus on the failing tests and suggest fixes.",
      "description": "Run tests with coverage",
      "agent": "build",
      "model": "anthropic/claude-sonnet-4-20250514"
    }
  }
}
```

使用：在 TUI 中输入 `/test`。

### 2.2 模板语法

**$ARGUMENTS — 全部参数**：

```markdown
---
description: Create a new component
---

Create a new React component named $ARGUMENTS with TypeScript support.
Include proper typing and basic structure.
```

```
/component Button
```

`$ARGUMENTS` → `Button`。

**位置参数 $1, $2, $3...**：

```markdown
---
description: Create a new file with content
---

Create a file named $1 in the directory $2
with the following content: $3
```

```
/create-file config.json src "{ \"key\": \"value\" }"
```

**Shell 输出注入 `!command`**：

```markdown
---
description: Review recent changes
---

Recent git commits:
!`git log --oneline -10`

Current test results:
!`npm test 2>&1 | tail -20`

Review these changes and suggest improvements.
```

**文件引用 `@path`**：

```markdown
---
description: Review component
---

Review the component in @src/components/Button.tsx.
Check for performance issues and suggest improvements.
```

### 2.3 命令配置选项

| 选项 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `template` | string | ✅ | 命令提示词模板 |
| `description` | string | ❌ | 描述，显示在 TUI 命令列表中 |
| `agent` | string | ❌ | 指定执行 Agent（默认当前 Agent） |
| `model` | string | ❌ | 覆盖模型 |
| `subtask` | boolean | ❌ | 强制以 Subagent 子任务模式运行 |

### 2.4 内置命令

| 命令 | 说明 |
|------|------|
| `/init` | 分析项目生成 AGENTS.md |
| `/undo` | 撤销上一次变更 |
| `/redo` | 重做已撤销的变更 |
| `/share` | 分享当前会话 |
| `/help` | 显示帮助 |
| `/connect` | 连接 Provider |
| `/models` | 查看/切换模型 |
| `/theme` | 查看/切换主题 |

> 自定义命令可覆盖同名内置命令。

---

## 3. 实战 Skill & Command 设计

### 3.1 K8s 诊断 Skill

`.opencode/skills/k8s-debug/SKILL.md`：

```markdown
---
name: k8s-debug
description: Diagnose Kubernetes pod and node issues using kubectl
---

## What I do
- Check pod status, events, and logs
- Identify common failure patterns (CrashLoopBackOff, OOMKilled, Pending)
- Suggest remediation steps

## How to use
1. Run `kubectl get pods -A` to see overall status
2. For failing pods, check events: `kubectl describe pod <name> -n <ns>`
3. Check logs: `kubectl logs <name> -n <ns> --tail=100`
4. For node issues: `kubectl describe node <name>`

## Common patterns
- CrashLoopBackOff → Check logs and resource limits
- Pending → Check node resources and scheduling constraints
- OOMKilled → Increase memory limits
```

### 3.2 代码审查命令

`.opencode/commands/review.md`：

```markdown
---
description: Review staged changes
agent: plan
---

Here are the currently staged changes:
!`git diff --cached`

Review these changes for:
- Code quality and best practices
- Potential bugs and edge cases
- Security implications
- Performance considerations

Provide specific, actionable feedback.
```

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [04 - Agent 系统](./04-opencode-agents-system.md) | Skill 与 Agent 的交互 |
| [05 - 工具与权限](./05-opencode-tools-permissions.md) | skill 工具权限配置 |
| [12 - 进阶话题](./12-opencode-advanced-topics.md) | 团队级 Skill 共享 |

---

*本文档基于 OpenCode 官方文档（opencode.ai/docs/skills、opencode.ai/docs/commands）整理。*
