# 安装部署与快速入门

> **文档类型**: 部署指南 | **最后更新**: 2026-03 | **关键词**: OpenCode, Installation, Configuration, Provider, AGENTS.md, Quick Start

---

## 概述

本文覆盖 OpenCode 的完整安装部署流程：从多平台安装方式、Provider API Key 配置、项目初始化到基础工作流掌握。无论你使用 macOS、Linux 还是 Windows，都可以在 5 分钟内开始使用 OpenCode。

---

## 1. 前置要求

### 1.1 终端要求

OpenCode TUI 需要支持 Truecolor（24-bit）的现代终端模拟器：

| 终端 | 平台 | 推荐度 |
|------|------|--------|
| WezTerm | 跨平台 | ★★★★★ |
| Alacritty | 跨平台 | ★★★★★ |
| Ghostty | Linux / macOS | ★★★★☆ |
| Kitty | Linux / macOS | ★★★★☆ |
| iTerm2 | macOS | ★★★★☆ |
| Windows Terminal | Windows | ★★★★☆ |

验证 Truecolor 支持：

```bash
echo $COLORTERM
# 应输出 truecolor 或 24bit
```

如需启用：

```bash
# 在 shell profile 中添加
export COLORTERM=truecolor
```

### 1.2 API Key

至少需要一个 LLM Provider 的 API Key。推荐新用户使用 **OpenCode Zen**（官方托管服务，内含免费额度）。

---

## 2. 安装方式

### 2.1 一键安装脚本（推荐）

```bash
curl -fsSL https://opencode.ai/install | bash
```

### 2.2 包管理器安装

**macOS / Linux (Homebrew)**：

```bash
# 推荐使用 OpenCode tap，更新更及时
brew install anomalyco/tap/opencode
```

**Arch Linux**：

```bash
sudo pacman -S opencode           # Stable
paru -S opencode-bin              # Latest from AUR
```

**Windows (Chocolatey)**：

```bash
choco install opencode
```

**Windows (Scoop)**：

```bash
scoop install opencode
```

### 2.3 Node.js / Bun

```bash
# npm
npm install -g opencode-ai

# bun
bun install -g opencode-ai

# pnpm
pnpm install -g opencode-ai

# yarn
yarn global add opencode-ai
```

### 2.4 Docker

```bash
docker run -it --rm ghcr.io/anomalyco/opencode
```

### 2.5 Mise

```bash
mise use -g github:anomalyco/opencode
```

### 2.6 从 Release 下载

前往 [GitHub Releases](https://github.com/anomalyco/opencode/releases) 下载对应平台的二进制文件。

---

## 3. 配置 Provider

### 3.1 使用 OpenCode Zen（推荐新手）

OpenCode Zen 是官方团队精选并验证的模型列表，提供统一账单管理：

```bash
opencode
# 在 TUI 中运行
/connect
# 选择 opencode → 前往 opencode.ai/auth
# 登录 → 添加账单信息 → 复制 API Key → 粘贴
```

验证可用模型：

```
/models
```

### 3.2 使用 Anthropic (Claude)

```bash
/connect
# 选择 Anthropic
# 选择 Claude Pro/Max（OAuth 浏览器认证）
# 或选择 Create an API Key / Manually enter API Key
```

### 3.3 使用环境变量

```bash
# Anthropic
export ANTHROPIC_API_KEY="sk-ant-..."

# OpenAI
export OPENAI_API_KEY="sk-..."

# Google Gemini
export GEMINI_API_KEY="..."

# GitHub Copilot
export GITHUB_TOKEN="ghp_..."

# Groq
export GROQ_API_KEY="gsk_..."

# AWS Bedrock
export AWS_ACCESS_KEY_ID="..."
export AWS_SECRET_ACCESS_KEY="..."
export AWS_REGION="us-east-1"

# Azure OpenAI
export AZURE_OPENAI_ENDPOINT="https://RESOURCE.openai.azure.com/"
export AZURE_OPENAI_API_KEY="..."
```

### 3.4 凭证存储

API Key 通过 `/connect` 命令设置后，安全存储在 `~/.local/share/opencode/auth.json`。

---

## 4. 项目初始化

### 4.1 进入项目并启动

```bash
cd /path/to/your/project
opencode
```

### 4.2 运行 /init

在 TUI 中执行：

```
/init
```

OpenCode 将分析项目结构并生成 `AGENTS.md` 文件（类似 `.cursorrules`），帮助 Agent 理解：

- 项目技术栈与框架
- 代码风格与命名规范
- 目录结构与模块划分
- 测试与构建命令

> `AGENTS.md` 建议纳入 Git 版本控制，使团队共享 Agent 上下文。

### 4.3 项目配置文件

在项目根目录创建 `opencode.json`（可选但推荐）：

```json
{
  "$schema": "https://opencode.ai/config.json",
  "model": "anthropic/claude-sonnet-4-20250514",
  "small_model": "anthropic/claude-haiku-4-20250514",
  "autoupdate": true,
  "autoCompact": true
}
```

---

## 5. 基础工作流

### 5.1 提问（Ask Questions）

```
How is authentication handled in @packages/functions/src/api/index.ts
```

使用 `@` 前缀引用文件路径，OpenCode 会自动读取文件内容作为上下文。

也可以拖拽图片到终端，OpenCode 会自动识别并添加到提示中。

### 5.2 计划模式（Plan Mode）

按 `Tab` 切换到 Plan 模式（右下角显示标识），此模式下 Agent **不会修改任何文件**：

```
When a user deletes a note, we'd like to flag it as deleted in the database.
Then create a screen that shows all the recently deleted notes.
From this screen, the user can undelete a note or permanently delete it.
```

Plan 模式适合在执行前评审方案。

### 5.3 构建模式（Build Mode）

再次按 `Tab` 切回 Build 模式，让 Agent 执行计划：

```
Sounds good! Go ahead and make the changes.
```

### 5.4 直接构建

对于简单任务，可以跳过 Plan 直接在 Build 模式中描述需求：

```
We need to add authentication to the /settings route. Take a look at how this is
handled in the /notes route in @packages/functions/src/notes.ts and implement
the same logic in @packages/functions/src/settings.ts
```

### 5.5 撤销 / 重做

```
/undo    # 撤销上一次变更
/redo    # 重做已撤销的变更
```

### 5.6 分享会话

```
/share   # 生成会话链接并复制到剪贴板
```

---

## 6. 命令行参数

| 参数 | 短格式 | 说明 |
|------|--------|------|
| `--help` | `-h` | 显示帮助信息 |
| `--debug` | `-d` | 启用调试模式，查看详细日志 |
| `--cwd` | `-c` | 设置工作目录 |
| `--prompt` | `-p` | 非交互模式运行单次提示 |
| `--output-format` | `-f` | 非交互模式输出格式（text/json） |
| `--quiet` | `-q` | 非交互模式隐藏 spinner |

```bash
# 非交互模式 — 单次提问
opencode -p "Explain the use of context in Go"

# JSON 格式输出 — 适合脚本消费
opencode -p "List all TODO comments" -f json

# 安静模式 — 隐藏 spinner（CI/CD 场景）
opencode -p "Fix the linting errors" -q

# 指定工作目录
opencode -c /path/to/project

# 调试模式
opencode -d
```

---

## 7. 配置文件位置与优先级

| 优先级 | 位置 | 说明 |
|--------|------|------|
| 1（最低） | Remote `.well-known/opencode` | 组织级默认 |
| 2 | `~/.config/opencode/opencode.json` | 全局用户配置 |
| 3 | `OPENCODE_CONFIG` 环境变量 | 自定义路径覆盖 |
| 4（最高） | `./opencode.json` (项目根目录) | 项目级配置 |

配置文件支持 JSON 和 JSONC（JSON with Comments）格式，文件名支持 `opencode.json` 或 `opencode.jsonc`。

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [01 - 概述与架构](./01-opencode-overview-architecture.md) | 理解 OpenCode 全貌 |
| [03 - Provider 与模型管理](./03-opencode-providers-models.md) | 深入 Provider 配置 |
| [04 - Agent 系统](./04-opencode-agents-system.md) | 深入 Build/Plan 模式 |
| [09 - TUI 定制](./09-opencode-tui-customization.md) | 自定义快捷键和主题 |

---

*本文档基于 OpenCode 官方文档（opencode.ai/docs）整理。*
