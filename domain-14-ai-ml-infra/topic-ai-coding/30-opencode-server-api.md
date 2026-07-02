---
title: Server 模式与 HTTP API
description: '**文档类型**: 平台集成专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, Server, HTTP
  API, OpenAPI 3.1, SDK, SSE, Headless, Stainless, Hono'
summary: '**文档类型**: 平台集成专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, Server, HTTP API,
  OpenAPI 3.1, SDK, SSE, Headless, Stainless, Hono'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
- rag
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
- Server 模式与 HTTP API 是什么
- 如何 Server 模式与 HTTP API
trigger_keywords:
- Server
- 模式与
- HTTP
- API
- ai
- coding
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Server 模式与 HTTP API

> **文档类型**: 平台集成专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, Server, HTTP API, OpenAPI 3.1, SDK, SSE, Headless, Stainless, Hono

---

## 概述

OpenCode 的 **Client/Server 架构** 使其成为唯一提供完整 HTTP API 的 AI Coding Agent。通过 `opencode serve` 启动无头服务端，任何 HTTP 客户端（Web App、Mobile App、脚本、IDE 插件）都可以通过 OpenAPI 3.1 端点与 Agent 交互。

---

## 1. 架构原理

```
┌──────────────┐     HTTP      ┌──────────────────┐
│  Go TUI      │◄────────────►│  Bun HTTP Server  │
│ (默认客户端)  │     SSE       │  (Hono + OpenAPI) │
└──────────────┘               └────────┬─────────┘
                                        │
┌──────────────┐                ┌───────▼─────────┐
│  Desktop App │◄──────────►│  AI SDK + Tools  │
└──────────────┘               └───────┬─────────┘
                                       │
┌──────────────┐               ┌───────▼─────────┐
│  自定义客户端  │◄──────────►│  SQLite [[domain-04-storage-data/README.md|storage]]  │
│  (SDK/脚本)   │               └─────────────────┘
└──────────────┘
```

当运行 `opencode` 时，会同时启动 TUI 客户端和 HTTP Server。Server 端口随机分配，TUI 通过 HTTP/SSE 与 Server 通信。也可通过 `opencode serve` 单独启动 Server。

---

## 2. 启动 Server

### 2.1 独立 Server 模式

```bash
opencode serve [--port 4096] [--hostname 0.0.0.0] [--cors http://localhost:5173]
```

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `--port` | 监听端口 | 4096 |
| `--hostname` | 监听地址 | 127.0.0.1 |
| `--mdns` | 启用 mDNS 服务发现 | false |
| `--mdns-domain` | mDNS 域名 | opencode.local |
| `--cors` | 允许的浏览器源（可多次指定） | [] |

```bash
# 多 CORS 源
opencode serve --cors http://localhost:5173 --cors https://app.example.com
```

### 2.2 认证保护

```bash
OPENCODE_SERVER_PASSWORD=your-password opencode serve
OPENCODE_SERVER_USERNAME=admin OPENCODE_SERVER_PASSWORD=secret opencode serve
```

使用 HTTP Basic Auth 保护。

### 2.3 JSON 配置

```json
{
  "$schema": "https://opencode.ai/config.json",
  "server": {
    "port": 4096,
    "hostname": "0.0.0.0",
    "mdns": true,
    "mdnsDomain": "myproject.local",
    "cors": ["http://localhost:5173"]
  }
}
```

### 2.4 连接已有 Server

TUI 启动时随机分配端口。若需连接其 Server，可通过 `--hostname` 和 `--port` 指定固定地址。

---

## 3. OpenAPI Spec

Server 发布 **OpenAPI 3.1** 规范：

```
http://<hostname>:<port>/doc
```

例如 `http://localhost:4096/doc`。可用于：
- 生成客户端 SDK
- 在 Swagger Explorer 中查看 API
- 检查请求/响应类型

---

## 4. 核心 API 端点

### 4.1 全局

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/global/health` | 健康检查（`{ healthy: true, version }`） |
| GET | `/global/event` | 全局事件 SSE 流 |

### 4.2 会话管理

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/session` | 列出所有会话 |
| POST | `/session` | 创建新会话（body: `{ parentID?, title? }`） |
| GET | `/session/:id` | 获取会话详情 |
| DELETE | `/session/:id` | 删除会话及所有数据 |
| PATCH | `/session/:id` | 更新会话属性（body: `{ title? }`） |
| GET | `/session/:id/children` | 获取子会话列表 |
| GET | `/session/:id/todo` | 获取会话任务列表 |
| POST | `/session/:id/init` | 分析项目生成 AGENTS.md |
| POST | `/session/:id/fork` | 在指定消息处分叉会话 |
| POST | `/session/:id/abort` | 中止运行中的会话 |
| POST | `/session/:id/share` | 分享会话 |
| DELETE | `/session/:id/share` | 取消分享 |
| GET | `/session/:id/diff` | 获取会话文件变更 diff |
| POST | `/session/:id/summarize` | 生成会话摘要 |
| POST | `/session/:id/revert` | 撤销消息 |
| POST | `/session/:id/unrevert` | 恢复所有已撤销消息 |
| GET | `/session/status` | 所有会话状态 |
| POST | `/session/:id/permissions/:permissionID` | 响应权限请求 |

### 4.3 消息

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/session/:id/message` | 列出消息（query: `limit?`） |
| POST | `/session/:id/message` | 发送消息并**等待回复** |
| GET | `/session/:id/message/:messageID` | 获取消息详情 |
| POST | `/session/:id/prompt_async` | **异步**发送消息（204 No Content） |
| POST | `/session/:id/command` | 执行斜杠命令 |
| POST | `/session/:id/shell` | 执行 Shell 命令 |

### 4.4 文件操作

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/find?pattern=<pat>` | 搜索文件内容（ripgrep） |
| GET | `/find/file?query=<q>` | 按名称查找文件（模糊匹配） |
| GET | `/find/symbol?query=<q>` | 查找工作区符号 |
| GET | `/file?path=<path>` | 列出文件和目录 |
| GET | `/file/content?path=<p>` | 读取文件内容 |
| GET | `/file/status` | 追踪文件的状态 |

### 4.5 配置与状态

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/config` | 获取配置 |
| PATCH | `/config` | 更新配置 |
| GET | `/config/providers` | 列出 Provider 和默认模型 |
| GET | `/provider` | 列出所有 Provider |
| GET | `/agent` | 列出所有 Agent |
| GET | `/command` | 列出所有命令 |
| GET | `/lsp` | LSP Server 状态 |
| GET | `/formatter` | Formatter 状态 |
| GET | `/mcp` | MCP Server 状态 |
| POST | `/mcp` | 动态添加 MCP Server |

### 4.6 事件流

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/event` | SSE 事件流（首事件 `server.connected`） |

### 4.7 TUI 远程控制

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/tui/append-prompt` | 追加文本到输入框 |
| POST | `/tui/submit-prompt` | 提交当前输入 |
| POST | `/tui/clear-prompt` | 清除输入 |
| POST | `/tui/execute-command` | 执行命令 |
| POST | `/tui/show-toast` | 显示通知 |
| POST | `/tui/open-help` | 打开帮助 |
| POST | `/tui/open-sessions` | 打开会话选择器 |
| POST | `/tui/open-models` | 打开模型选择器 |
| POST | `/tui/open-themes` | 打开主题选择器 |

### 4.8 认证

| 方法 | 路径 | 说明 |
|------|------|------|
| PUT | `/auth/:id` | 设置 Provider 认证凭证 |
| GET | `/provider/auth` | 获取 Provider 认证方式 |
| POST | `/provider/:id/oauth/authorize` | 发起 OAuth 授权 |
| POST | `/provider/:id/oauth/callback` | 处理 OAuth 回调 |

---

## 5. SDK 生成

OpenCode 使用 **Stainless** 从 OpenAPI Spec 自动生成类型安全的客户端 SDK：

```typescript
import { OpenCode } from "@opencode-ai/sdk"

const client = new OpenCode({ baseURL: "http://localhost:4096" })

// 创建会话
const session = await client.session.create({ title: "Bug Fix" })

// 发送消息并等待回复
const response = await client.session.message.create(session.id, {
  parts: [{ type: "text", text: "Fix the auth bug in login.ts" }]
})

// 异步发送（不等待）
await client.session.promptAsync(session.id, {
  parts: [{ type: "text", text: "Refactor the database layer" }]
})

// 监听事件
const events = client.event.stream()
for await (const event of events) {
  console.log(event.type, event.data)
}
```

---

## 6. mDNS 服务发现

启用 mDNS 后，同一网络内的设备可自动发现 OpenCode Server：

```json
{
  "server": {
    "mdns": true,
    "mdnsDomain": "myproject.local"
  }
}
```

适用于移动设备远程访问、多台开发机协作等场景。

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [01 - 概述与架构](./01-opencode-overview-architecture.md) | Client/Server 架构详解 |
| [11 - GitHub CI/CD](./11-opencode-github-automation.md) | 基于 Server 的自动化 |
| [12 - 进阶话题](./12-opencode-advanced-topics.md) | 非交互模式与安全 |

---

*本文档基于 OpenCode 官方文档（opencode.ai/docs/server）整理。*


<!-- risk-assessed -->
