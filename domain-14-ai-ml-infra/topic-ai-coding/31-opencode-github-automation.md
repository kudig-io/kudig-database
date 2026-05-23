---
title: GitHub 集成与 CI/CD 自动化
description: '**文档类型**: 自动化集成专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, GitHub, GitHub Actions, CI/CD, Issue Triage, PR Review,
  Automation, Headless'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
- job
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
- GitHub 集成与 CI/CD 自动化 是什么
- 如何 GitHub 集成与 CI/CD 自动化
trigger_keywords:
- GitHub
- 集成与
- CI
- CD
- 自动化
- ai
- coding
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# GitHub 集成与 CI/CD 自动化

> **文档类型**: 自动化集成专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, GitHub, GitHub Actions, CI/CD, Issue Triage, PR Review, Automation, Headless

---

## 概述

OpenCode 原生集成 GitHub 工作流。通过在 Issue 或 PR 评论中 mention `/opencode` 或 `/oc`，即可在 GitHub Actions Runner 中触发 Agent 执行任务——从 Issue 分析、代码修复到 PR 审查和定时维护，完全在 Runner 的安全隔离环境中运行。

---

## 1. 核心能力

| 能力 | 说明 |
|------|------|
| **Issue 分析** | 读取 Issue 全部上下文（包括评论），提供解释和建议 |
| **代码修复** | 创建新分支、实现修复、自动提交 PR |
| **PR 审查** | 分析代码变更、检查质量、提供审查意见 |
| **精准代码评论** | 在 PR Files 视图中对特定代码行评论触发，获得文件路径+行号+diff 上下文 |
| **定时任务** | Cron 驱动的自动化维护（如 TODO 扫描、依赖更新） |
| **安全隔离** | 在 GitHub Runner 中运行，不访问外部环境 |

---

## 2. 安装配置

### 2.1 自动安装（推荐）

```bash
opencode github install
```

自动完成 GitHub App 安装、Workflow 创建和 [[Secrets|Secrets]] 配置。

### 2.2 手动安装

**Step 1**：安装 GitHub App

前往 [github.com/apps/opencode-agent](https://github.com/apps/opencode-agent)，安装到目标仓库。

**Step 2**：添加 Workflow 文件

`.github/workflows/opencode.yml`：

```yaml
name: opencode

on:
  issue_comment:
    types: [created]
  pull_request_review_comment:
    types: [created]

jobs:
  opencode:
    if: |
      contains(github.event.comment.body, '/oc') ||
      contains(github.event.comment.body, '/opencode')
    runs-on: ubuntu-latest
    permissions:
      id-token: write
    steps:
      - name: Checkout repository
        uses: actions/checkout@v6
        with:
          fetch-depth: 1
          persist-credentials: false

      - name: Run OpenCode
        uses: anomalyco/opencode/github@latest
        env:
          ANTHROPIC_API_KEY: ${{ secrets.ANTHROPIC_API_KEY }}
        with:
          model: anthropic/claude-sonnet-4-20250514
```

**Step 3**：添加 Secrets

在仓库 Settings → Secrets and variables → Actions 中添加 API Key。

---

## 3. 支持的事件类型

| 事件 | 触发方式 | 需要 prompt | 说明 |
|------|---------|:-----------:|------|
| `issue_comment` | Issue/PR 评论 mention `/oc` | ❌ | 从评论提取指令 |
| `pull_request_review_comment` | PR 代码行评论 mention `/oc` | ❌ | 附带文件路径、行号、diff |
| `issues` | Issue 创建/编辑 | ✅ | 自动触发需提供 prompt |
| `pull_request` | PR 创建/更新 | ❌ | 默认执行 PR 审查 |
| `schedule` | Cron 定时 | ✅ | 输出到日志和 PR |
| `workflow_dispatch` | 手动触发 | ✅ | 从 Actions 页面触发 |

---

## 4. Action 配置选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `model` | 使用的模型（**必填**），格式 `provider/model` | — |
| `agent` | 使用的 Agent（必须为 Primary Agent） | `default_agent` 或 `"build"` |
| `share` | 是否分享会话 | 公开仓库 `true` |
| `prompt` | 自定义提示词（覆盖默认行为） | — |
| `token` | GitHub Token（默认使用 GitHub App Token） | — |
| `use_github_token` | 使用 Runner 内置 `GITHUB_TOKEN` | `false` |

---

## 5. 实战示例

### 5.1 Issue 解释

在 GitHub Issue 中评论：

```
/opencode explain this issue
```

OpenCode 读取整个 Issue 线程（包括所有评论），回复清晰解释。

### 5.2 Issue 修复

```
/opencode fix this
```

OpenCode 将：创建新分支 → 实现修复 → 提交代码 → 开 PR。

### 5.3 PR 代码行评论

在 PR → Files 视图 → 选中特定代码行 → 留下评论：

```
/oc add error handling here
```

OpenCode 自动获得：
- 被审查的文件路径
- 具体代码行
- 周围 diff 上下文
- 行号信息

### 5.4 自动 PR 审查

```yaml
name: opencode-review

on:
  pull_request:
    types: [opened, synchronize, reopened, ready_for_review]

jobs:
  review:
    runs-on: ubuntu-latest
    permissions:
      id-token: write
      contents: read
      pull-requests: read
      issues: read
    steps:
      - uses: actions/checkout@v6
        with:
          persist-credentials: false
      - uses: anomalyco/opencode/github@latest
        env:
          ANTHROPIC_API_KEY: ${{ secrets.ANTHROPIC_API_KEY }}
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
        with:
          model: anthropic/claude-sonnet-4-20250514
          use_github_token: true
          prompt: |
            Review this pull request:
            - Check for code quality issues
            - Look for potential bugs
            - Suggest improvements
```

### 5.5 定时任务

```yaml
name: Scheduled OpenCode Task

on:
  schedule:
    - cron: "0 9 * * 1"  # 每周一 09:00 UTC

jobs:
  opencode:
    runs-on: ubuntu-latest
    permissions:
      id-token: write
      contents: write
      pull-requests: write
      issues: write
    steps:
      - uses: actions/checkout@v6
        with:
          persist-credentials: false
      - uses: anomalyco/opencode/github@latest
        env:
          ANTHROPIC_API_KEY: ${{ secrets.ANTHROPIC_API_KEY }}
        with:
          model: anthropic/claude-sonnet-4-20250514
          prompt: |
            Review the codebase for any TODO comments and create a summary.
            If you find issues worth addressing, open an issue to track them.
```

### 5.6 Issue 自动分流

```yaml
name: Issue Triage

on:
  issues:
    types: [opened]

jobs:
  triage:
    runs-on: ubuntu-latest
    permissions:
      id-token: write
      contents: write
      pull-requests: write
      issues: write
    steps:
      - name: Check account age
        id: check
        uses: actions/github-script@v7
        with:
          script: |
            const user = await github.rest.users.getByUsername({
              username: context.payload.issue.user.login
            });
            const created = new Date(user.data.created_at);
            const days = (Date.now() - created) / (1000 * 60 * 60 * 24);
            return days >= 30;
          result-encoding: string

      - uses: actions/checkout@v6
        if: steps.check.outputs.result == 'true'
        with:
          persist-credentials: false

      - uses: anomalyco/opencode/github@latest
        if: steps.check.outputs.result == 'true'
        env:
          ANTHROPIC_API_KEY: ${{ secrets.ANTHROPIC_API_KEY }}
        with:
          model: anthropic/claude-sonnet-4-20250514
          prompt: |
            Review this issue. If there's a clear fix or relevant docs:
            - Provide documentation links
            - Add error handling guidance for code examples
            Otherwise, do not comment.
```

---

## 6. 自定义 Prompt

通过 `prompt` 覆盖默认行为，定制 OpenCode 在 CI/CD 中的表现：

```yaml
- uses: anomalyco/opencode/github@latest
  with:
    model: anthropic/claude-sonnet-4-5
    prompt: |
      Review this pull request with focus on:
      1. [[domain-05-security-compliance/README|security]] vulnerabilities (OWASP Top 10)
      2. Performance regressions
      3. API backward compatibility
      4. Test coverage for new code
```

---

## 7. Token 与权限

### 7.1 默认（GitHub App Token）

使用 OpenCode GitHub App 的 Installation Token。提交/评论显示为 App 账号。

### 7.2 使用 GITHUB_TOKEN

无需安装 GitHub App：

```yaml
permissions:
  id-token: write
  contents: write
  pull-requests: write
  issues: write
```

```yaml
with:
  use_github_token: true
env:
  GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

### 7.3 使用 PAT

使用个人访问令牌，提交显示为个人账号：

```yaml
with:
  token: ${{ secrets.MY_PAT }}
```

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [10 - Server 模式](./10-opencode-server-api.md) | 理解底层 Server 架构 |
| [04 - Agent 系统](./04-opencode-agents-system.md) | 配置执行 Agent |
| [12 - 进阶话题](./12-opencode-advanced-topics.md) | 非交互模式与安全 |
| [domain-14-ai-ml-infra/topic-ai-agent/28](../domain-14-ai-ml-infra/topic-ai-agent/28-agent-cli-enterprise-automation.md) | Agent CLI 企业自动化通用指南 |

---

*本文档基于 OpenCode 官方文档（opencode.ai/docs/github）整理。*
