---
title: LSP 代码智能与 Formatter
description: '**文档类型**: 技术深度专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, LSP, Language Server Protocol, Formatter, Code Intelligence,
  Diagnostics, ripgrep'
category: ai-coding
tags:
- ai
- coding
- copilot
- code-generation
- llm
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 开发工程师
- AI 工程师
estimated_read_time: 5min
intent_queries:
- LSP 代码智能与 Formatter 是什么
- 如何 LSP 代码智能与 Formatter
trigger_keywords:
- LSP
- 代码智能与
- Formatter
- ai
- coding
---

# LSP 代码智能与 Formatter

> **文档类型**: 技术深度专题 | **最后更新**: 2026-03 | **关键词**: OpenCode, LSP, Language Server Protocol, Formatter, Code Intelligence, Diagnostics, ripgrep

---

## 概述

OpenCode 是唯一原生集成 **LSP（Language Server Protocol）** 的 AI Coding Agent。这使得 LLM 不仅能读写代码，还能获得真实的编译器级诊断反馈，实现「修改→诊断→修复」的自动闭环。同时，OpenCode 内置 20+ Formatter，确保 AI 生成的代码自动遵循项目代码风格。

---

## 1. LSP 集成架构

### 1.1 诊断反馈闭环

```
Agent 修改文件 (edit/write)
    ↓
OpenCode 发送 textDocument/didChange → LSP Server
    ↓
LSP Server 返回 diagnostics (errors/warnings)
    ↓
Diagnostics 通过事件总线汇入全局诊断 Map
    ↓
Agent 获得诊断信息 → 自动分析并修复 → 再次触发诊断
    ↓
循环直到无错误或用户中断
```

这是 OpenCode 与其他 Coding Agent 的核心差异——LLM 不再「盲写」，而是像使用 IDE 一样获得实时反馈。

### 1.2 支持的 LSP 操作

| 操作 | 说明 |
|------|------|
| `goToDefinition` | 跳转到符号定义 |
| `findReferences` | 查找所有引用 |
| `hover` | 获取悬停信息（类型、文档） |
| `documentSymbol` | 列出文档内所有符号 |
| `workspaceSymbol` | 工作区符号搜索 |
| `goToImplementation` | 跳转到接口实现 |
| `prepareCallHierarchy` | 准备调用层次 |
| `incomingCalls` | 查找调用当前函数的位置 |
| `outgoingCalls` | 查找当前函数调用的位置 |

---

## 2. 内置 LSP Server（30+）

| LSP Server | 文件扩展名 | 要求 |
|-----------|-----------|------|
| **gopls** | .go | `go` 命令可用 |
| **typescript** | .ts, .tsx, .js, .jsx, .mjs, .cjs, .mts, .cts | `typescript` 依赖在项目中 |
| **pyright** | .py, .pyi | `pyright` 依赖已安装 |
| **rust-analyzer** | .rs | `rust-analyzer` 命令可用 |
| **clangd** | .c, .cpp, .cc, .cxx, .h, .hpp | 自动安装 |
| **jdtls** | .java | Java SDK 21+ 已安装 |
| **kotlin-ls** | .kt, .kts | 自动安装 |
| **lua-ls** | .lua | 自动安装 |
| **bash** | .sh, .bash, .zsh, .ksh | 自动安装 `bash-language-server` |
| **dart** | .dart | `dart` 命令可用 |
| **deno** | .ts, .tsx, .js, .jsx, .mjs | `deno` 命令可用 + `deno.json` 存在 |
| **svelte** | .svelte | 自动安装 |
| **vue** | .vue | 自动安装 |
| **astro** | .astro | 自动安装 |
| **ruby-lsp** | .rb, .rake, .gemspec, .ru | `ruby`/`gem` 命令可用 |
| **sourcekit-lsp** | .swift, .objc, .objcpp | Xcode (macOS) |
| **csharp** | .cs | .NET SDK 已安装 |
| **fsharp** | .fs, .fsi, .fsx | .NET SDK 已安装 |
| **elixir-ls** | .ex, .exs | `elixir` 命令可用 |
| **gleam** | .gleam | `gleam` 命令可用 |
| **hls** | .hs, .lhs | `haskell-language-server-wrapper` 可用 |
| **clojure-lsp** | .clj, .cljs, .cljc, .edn | `clojure-lsp` 命令可用 |
| **ocaml-lsp** | .ml, .mli | `ocamllsp` 命令可用 |
| **julials** | .jl | `julia` + `LanguageServer.jl` 已安装 |
| **nixd** | .nix | `nixd` 命令可用 |
| **terraform** | .tf, .tfvars | 自动安装 |
| **tinymist** | .typ, .typc | 自动安装 |
| **zls** | .zig, .zon | `zig` 命令可用 |
| **prisma** | .prisma | `prisma` 命令可用 |
| **eslint** | .ts, .tsx, .js, .jsx, .vue | `eslint` 依赖在项目中 |
| **oxlint** | .ts, .tsx, .js, .jsx, .vue, .svelte, .astro | `oxlint` 依赖在项目中 |
| **yaml-ls** | .yaml, .yml | 自动安装 |
| **php intelephense** | .php | 自动安装 |

> LSP Server 在检测到对应文件扩展名且满足依赖要求时**自动启用和启动**，无需手动配置。

---

## 3. LSP 配置

### 3.1 自定义 LSP Server

```json
{
  "$schema": "https://opencode.ai/config.json",
  "lsp": {
    "custom-lsp": {
      "command": ["custom-lsp-server", "--stdio"],
      "extensions": [".custom", ".myext"]
    }
  }
}
```

### 3.2 配置选项

| 属性 | 类型 | 说明 |
|------|------|------|
| `disabled` | boolean | 禁用此 LSP Server |
| `command` | string[] | 启动命令 |
| `extensions` | string[] | 处理的文件扩展名 |
| `env` | object | 启动时环境变量 |
| `initialization` | object | 初始化选项（发送给 LSP Server） |

### 3.3 传递初始化选项

```json
{
  "lsp": {
    "typescript": {
      "initialization": {
        "preferences": {
          "importModuleSpecifierPreference": "relative"
        }
      }
    },
    "rust": {
      "env": { "RUST_LOG": "debug" }
    }
  }
}
```

### 3.4 禁用 LSP

```json
// 禁用所有 LSP
{ "lsp": false }

// 禁用特定 LSP
{ "lsp": { "typescript": { "disabled": true } } }
```

---

## 4. 内置 Formatter（20+）

| Formatter | 文件扩展名 | 触发条件 |
|-----------|-----------|---------|
| **prettier** | .js, .ts, .html, .css, .md, .json, .yaml 等 | `package.json` 中有 prettier 依赖 |
| **biome** | .js, .ts, .html, .css, .md, .json 等 | `biome.json(c)` 配置文件存在 |
| **gofmt** | .go | `gofmt` 命令可用 |
| **cargofmt** | .rs | `cargo fmt` 命令可用 |
| **rustfmt** | .rs | `rustfmt` 命令可用 |
| **ruff** | .py, .pyi | `ruff` 命令可用 + 配置文件 |
| **uv** | .py, .pyi | `uv` 命令可用 |
| **shfmt** | .sh, .bash | `shfmt` 命令可用 |
| **clang-format** | .c, .cpp, .h, .hpp, .ino 等 | `.clang-format` 配置存在 |
| **dart** | .dart | `dart` 命令可用 |
| **gleam** | .gleam | `gleam` 命令可用 |
| **terraform** | .tf, .tfvars | `terraform` 命令可用 |
| **mix** | .ex, .exs, .eex, .heex 等 | `mix` 命令可用 |
| **rubocop** | .rb, .rake, .gemspec | `rubocop` 命令可用 |
| **standardrb** | .rb, .rake, .gemspec | `standardrb` 命令可用 |
| **pint** | .php | `laravel/pint` 依赖在 `composer.json` |
| **nixfmt** | .nix | `nixfmt` 命令可用 |
| **ocamlformat** | .ml, .mli | `ocamlformat` 可用 + `.ocamlformat` 存在 |
| **ormolu** | .hs | `ormolu` 命令可用 |
| **ktlint** | .kt, .kts | `ktlint` 命令可用 |
| **zig** | .zig, .zon | `zig` 命令可用 |
| **air** | .R | `air` 命令可用 |

### 4.1 Formatter 工作原理

文件被 `write` 或 `edit` 工具修改后，OpenCode 自动：
1. 检查文件扩展名 → 匹配可用 Formatter
2. 运行对应 Formatter 命令
3. 应用格式化结果（后台进行，不阻塞对话）

### 4.2 自定义 Formatter

```json
{
  "$schema": "https://opencode.ai/config.json",
  "formatter": {
    "custom-markdown-formatter": {
      "command": ["deno", "fmt", "$FILE"],
      "extensions": [".md"]
    },
    "prettier": {
      "command": ["npx", "prettier", "--write", "$FILE"],
      "environment": { "NODE_ENV": "development" },
      "extensions": [".js", ".ts", ".jsx", ".tsx"]
    }
  }
}
```

`$FILE` 占位符在运行时被替换为被格式化的文件路径。

### 4.3 禁用 Formatter

```json
// 禁用所有 Formatter
{ "formatter": false }

// 禁用特定 Formatter
{ "formatter": { "prettier": { "disabled": true } } }
```

---

## 5. .ignore 文件

ripgrep（grep/glob/list 底层引擎）默认遵守 `.gitignore`。若需搜索被 ignore 的文件，在项目根目录创建 `.ignore`：

```
!node_modules/
!dist/
!build/
```

`!` 前缀表示**取消忽略**，允许 ripgrep 搜索这些目录。

---

## 关联文档

| 文档 | 关系 |
|------|------|
| [05 - 工具与权限](./05-opencode-tools-permissions.md) | lsp 工具权限配置 |
| [01 - 概述与架构](./01-opencode-overview-architecture.md) | LSP 在架构中的位置 |
| [12 - 进阶话题](./12-opencode-advanced-topics.md) | 故障排查 |

---

*本文档基于 OpenCode 官方文档（opencode.ai/docs/lsp、opencode.ai/docs/formatters）整理。*
