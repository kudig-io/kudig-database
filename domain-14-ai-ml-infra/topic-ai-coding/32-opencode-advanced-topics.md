---
title: 进阶话题与生产最佳实践
description: '# 进阶话题与生产最佳实践'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 开发工程师
- AI 工程师
estimated_read_time: 5min
intent_queries:
- 进阶话题与生产最佳实践 是什么
- 如何 进阶话题与生产最佳实践
trigger_keywords:
- 进阶话题与生产最佳实践
- ai
- coding
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# 进阶话题与生产最佳实践

> **文档类型**: 进阶实践专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, Auto Compact, Session Management, Security, Cost Control, Non-interactive, Team Collaboration, Configuration

---

## 概述

本文覆盖 OpenCode 的生产级进阶话题：Auto Compact 上下文管理、Session 高级操作、非交互模式自动化、安全加固策略、成本控制、团队协作模式、配置管理最佳实践以及常见故障排查。

---

## 1. Auto Compact — 上下文窗口管理

### 1.1 原理

当对话 token 使用接近模型上下文窗口的 **95%** 时，OpenCode 自动触发 Compaction Agent：

```
正常对话 → token 累积 → 达到 95% 阈值
    ↓
Compaction Agent 自动启动 → 生成对话摘要
    ↓
创建新会话 + 携带摘要上下文 → 无缝继续
```

1. 持续监控 token 使用量
2. 达到 95% 阈值时自动触发摘要
3. 创建新会话并携带压缩后的上下文
4. 用户可无缝继续工作，无需手动操作

```json
{
  "autoCompact": true
}
```

默认启用。设为 `false` 可禁用（不推荐，会导致上下文溢出错误）。

### 1.2 手动触发

使用快捷键 `<Leader>+C`（默认 `Ctrl+X` → `C`）手动触发 Compact，适用于：
- 长对话主动清理上下文
- 切换话题前重置语境

---

## 2. Session 高级操作

| 操作 | 方式 | 说明 |
|------|------|------|
| **创建新会话** | `<Leader>+N` | 全新空会话 |
| **切换会话** | `<Leader>+L` | 从历史会话列表选择 |
| **会话时间线** | `<Leader>+G` | 以时间线视图浏览会话 |
| **分叉会话** | API: `POST /session/:id/fork` | 从特定消息点创建分支 |
| **分享会话** | `/share` | 生成会话链接到剪贴板 |
| **导出会话** | `<Leader>+X` | 导出会话数据 |
| **撤销变更** | `<Leader>+U` / `/undo` | 撤销 Agent 的文件修改 |
| **重做变更** | `<Leader>+R` / `/redo` | 重做已撤销的变更 |
| **中断会话** | `Escape` | 中断正在运行的 Agent |

### 2.1 分享配置

```json
{
  "share": "manual"
}
```

| 模式 | 说明 |
|------|------|
| `"manual"` | 手动 `/share` 分享（**默认**） |
| `"auto"` | 新会话自动分享 |
| `"disabled"` | 完全禁用分享功能 |

---

## 3. 非交互模式

### 3.1 基础用法

```bash
# 单次提问，输出文本
opencode -p "Explain the use of context in Go"

# JSON 格式输出（适合脚本消费）
opencode -p "List all TODO comments" -f json

# 安静模式（隐藏 spinner，适合 CI/CD）
opencode -p "Fix the linting errors" -q

# 指定工作目录
opencode -p "Run tests and fix failures" -c /path/to/project
```

### 3.2 关键行为

- **所有权限自动批准**：非交互模式下，所有工具权限设为 `allow`
- **处理完毕自动退出**：输出结果到 stdout 后退出
- **支持 JSON 输出**：`-f json` 将结果包装为 JSON 对象

### 3.3 脚本集成示例

```bash
#!/bin/bash
# 自动修复 lint 错误
RESULT=$(opencode -p "Fix all ESLint errors in src/" -f json -q)
echo "$RESULT" | jq '.content'

# 生成 CHANGELOG
opencode -p "Generate a CHANGELOG entry from the last 10 commits" -q >> CHANGELOG.md

# CI 中检查代码质量
opencode -p "Review staged changes for security issues. Exit with status 1 if critical issues found." -q
```

---

## 4. 安全加固

### 4.1 权限最小化模板

```json
{
  "$schema": "https://opencode.ai/config.json",
  "permission": {
    "*": "ask",
    "read": {
      "*": "allow",
      "*.env": "deny",
      "*.env.*": "deny",
      "*.env.example": "allow",
      "**/.ssh/*": "deny",
      "**/credentials*": "deny"
    },
    "bash": {
      "*": "ask",
      "git status *": "allow",
      "git diff *": "allow",
      "git log *": "allow",
      "npm test *": "allow",
      "npm run lint *": "allow",
      "rm *": "deny",
      "rm -rf *": "deny",
      "sudo *": "deny",
      "chmod *": "deny",
      "chown *": "deny",
      "curl *": "ask",
      "wget *": "deny"
    },
    "edit": {
      "*": "ask",
      "src/**": "allow",
      "tests/**": "allow",
      "*.env": "deny",
      "*.env.*": "deny"
    },
    "external_directory": "ask",
    "doom_loop": "ask"
  }
}
```

### 4.2 安全检查清单

| 检查项 | 建议 |
|--------|------|
| `.env` 文件保护 | 默认 deny，确保不读取敏感变量 |
| `rm` / `sudo` 命令 | 全局 deny |
| `doom_loop` | 保持 ask，防止 Agent 无限循环 |
| `external_directory` | 保持 ask，限制跨目录访问 |
| Server 认证 | 生产环境必须设置 `OPENCODE_SERVER_PASSWORD` |
| Custom Tool 审查 | 定期审查 `.opencode/tools/` 中的自定义工具代码 |
| MCP Server 审查 | 仅启用可信的 MCP Server |
| 分享功能 | 敏感项目设置 `"share": "disabled"` |

### 4.3 Doom Loop 检测

当同一工具以**相同输入**被连续调用 3 次时，触发 `doom_loop` 权限检查。默认 `ask`，允许用户：
- 批准继续（可能是合理重试）
- 拒绝以中断无限循环

---

## 5. 成本控制策略

| 策略 | 实施方式 | 预期效果 |
|------|---------|---------|
| **small_model 分流** | `"small_model": "claude-haiku-4"` | 标题/摘要用低成本模型 |
| **Max Steps 限制** | `"steps": 20` 限制 Agent 迭代 | 防止失控的 token 消耗 |
| **MCP 精简** | 仅启用必要 MCP Server | 减少上下文 token |
| **Plan 优先** | 先 Plan 再 Build | 避免无用的代码修改 |
| **Auto Compact** | `"autoCompact": true` | 避免上下文溢出重试 |
| **快速模型用于探索** | Explore Agent 用 Haiku/mini | 只读操作用低成本模型 |
| **非交互 + quiet** | `-p "..." -q` | CI/CD 减少不必要开销 |

**成本优化配置示例**：

```json
{
  "$schema": "https://opencode.ai/config.json",
  "model": "anthropic/claude-sonnet-4-20250514",
  "small_model": "anthropic/claude-haiku-4-20250514",
  "autoCompact": true,
  "agent": {
    "plan": {
      "model": "anthropic/claude-haiku-4-20250514"
    },
    "explore": {
      "model": "anthropic/claude-haiku-4-20250514"
    },
    "build": {
      "steps": 30
    }
  }
}
```

---

## 6. 团队协作模式

### 6.1 项目级配置（纳入 Git）

推荐将以下文件纳入版本控制：

```
project-root/
├── opencode.json              # 项目配置
├── tui.json                   # TUI 配置（可选）
├── AGENTS.md                  # 项目描述（/init 生成）
└── .opencode/
    ├── agents/                # 自定义 Agent
    │   ├── review.md
    │   └── [[entities/kubernetes|k8s]]-ops.md
    ├── commands/              # 自定义命令
    │   ├── test.md
    │   └── deploy.md
    ├── skills/                # Agent Skill
    │   └── git-release/
    │       └── SKILL.md
    └── tools/                 # Custom Tools
        └── database.ts
```

### 6.2 组织级远程配置

通过 `.well-known/opencode` 端点分发组织默认：

```json
{
  "mcp": {
    "jira": { "type": "remote", "url": "https://jira.example.com/mcp", "enabled": false },
    "confluence": { "type": "remote", "url": "https://wiki.example.com/mcp", "enabled": false }
  },
  "permission": {
    "bash": { "rm -rf *": "deny", "sudo *": "deny" }
  }
}
```

### 6.3 配置合并规则

```
Remote (.well-known/opencode)
    ↓ 合并
Global (~/.config/opencode/opencode.json)
    ↓ 合并
Custom (OPENCODE_CONFIG env)
    ↓ 合并
Project (./opencode.json)           ← 最高优先级
```

**合并行为**：配置文件是**合并**而非替换。后者仅覆盖冲突的键，非冲突设置全部保留。

---

## 7. 环境变量参考

| 变量 | 说明 |
|------|------|
| `OPENCODE_CONFIG` | 自定义配置文件路径 |
| `OPENCODE_CONFIG_DIR` | 自定义配置目录（搜索 agents/commands/plugins） |
| `OPENCODE_CONFIG_CONTENT` | 运行时配置内容（JSON 字符串） |
| `OPENCODE_TUI_CONFIG` | 自定义 TUI 配置文件 |
| `OPENCODE_SERVER_PASSWORD` | Server HTTP Basic Auth 密码 |
| `OPENCODE_SERVER_USERNAME` | Server HTTP Basic Auth 用户名（默认 `opencode`） |
| `COLORTERM` | 终端色彩支持标识（应为 `truecolor` 或 `24bit`） |
| `SHELL` | 默认 Shell（如 `/bin/zsh`） |

---

## 8. Shell 配置

```json
{
  "shell": {
    "path": "/bin/zsh",
    "args": ["-l"]
  }
}
```

默认使用 `$SHELL` 环境变量，回退到 `/bin/bash`。适用于需要特定 Shell 环境（如 zsh + oh-my-zsh 加载路径）的场景。

---

## 9. 故障排查

| 问题 | 排查方法 |
|------|---------|
| **Skill 不显示** | 检查 `SKILL.md` 大小写、frontmatter 包含 `name` + `description`、名称唯一性 |
| **LSP 不启动** | 确认语言依赖已安装（如 `go`、`typescript`、`pyright`）；检查 `opencode -d` 日志 |
| **MCP 连接失败** | `opencode mcp debug <name>` 检查连接和 OAuth 状态 |
| **主题色彩异常** | 验证 `echo $COLORTERM` 输出 `truecolor`；设置 `export COLORTERM=truecolor` |
| **Agent 无响应** | `opencode -d` 启用调试模式查看详细日志 |
| **Token 溢出** | 启用 `autoCompact: true`、减少 MCP Server、限制 `steps` |
| **Formatter 不工作** | 确认工具命令可用（如 `prettier`、`gofmt`）、配置文件存在 |
| **Custom Tool 错误** | 检查 `.opencode/tools/` 中 TypeScript 语法；确保 `@opencode-ai/plugin` 可用 |
| **Server 无法访问** | 检查端口占用、防火墙规则；Server 认证是否正确 |
| **GitHub Action 失败** | 检查 [[Secrets|Secrets]] 配置、权限设置、GitHub App 安装状态 |

### 调试模式

```bash
# 启用调试日志
opencode -d

# 查看日志
# TUI 中 Ctrl+L

# 调试 LSP
# 在 opencode.json 中设置
{ "debugLSP": true }
```

---

## 10. 升级与自动更新

```json
{
  "autoupdate": true
}
```

启用后，OpenCode 在启动时自动检查并安装新版本。也可手动升级：

```bash
# Homebrew
brew upgrade opencode

# npm
npm update -g opencode-ai

# 安装脚本
curl -fsSL https://opencode.ai/install | bash
```

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [01 - 概述与架构](./01-opencode-overview-architecture.md) | 架构基础 |
| [05 - 工具与权限](./05-opencode-tools-permissions.md) | 安全权限详解 |
| [06 - MCP 集成](./06-opencode-mcp-integration.md) | MCP 故障排查 |
| [10 - Server API](./10-opencode-server-api.md) | Server 安全配置 |
| [11 - GitHub CI/CD](./11-opencode-github-automation.md) | CI/CD 自动化 |

---

*本文档基于 OpenCode 官方文档（opencode.ai/docs）和生产实践整理。*
