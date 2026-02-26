---
name: gitbook-mac-export-run
overview: 确认 GitBook 在 Windows/macOS 的兼容方式，并在 macOS 上通过脚本导出可离线打开的静态版本。
todos:
  - id: verify-compatibility
    content: 核对 gitbook 文档与脚本，确认 Windows 与 macOS 的入口与差异
    status: completed
  - id: prepare-mdbook
    content: 在 mac 上安装或确认 mdBook 可用，确保构建命令可执行
    status: completed
    dependencies:
      - verify-compatibility
  - id: export-static
    content: 执行 export-static.sh 生成 dist/ 与可选 zip 产物
    status: completed
    dependencies:
      - prepare-mdbook
  - id: validate-output
    content: 验证 dist/index.html 可打开并检查页面与搜索可用性
    status: completed
    dependencies:
      - export-static
---

## User Requirements

- 检查 Gitbook 在 Windows 与 macOS 上的运行与构建流程是否一致可用
- 在当前 mac 上以离线静态方式运行并输出可直接打开的本地页面
- 提供可验证的结果（输出目录与打开方式）

## Product Overview

本地文档浏览系统的跨平台运行检查与 mac 本地离线导出流程执行。

## Core Features

- 跨平台脚本可用性核对（Windows 与 macOS）
- mac 端离线静态导出并生成可本地打开的页面
- 结果校验（页面生成与打开路径确认）

## Tech Stack Selection

- 文档构建：mdBook
- 脚本运行：bash（macOS），PowerShell/CMD（Windows）
- 依赖安装：Rust + Cargo（用于安装 mdBook）

## Implementation Approach

- 基于现有脚本与文档核对跨平台入口与依赖差异，确认 Windows 与 macOS 各自的执行路径。
- 在 mac 上走静态导出脚本：更新符号链接、生成目录、临时调整配置并构建，最终恢复配置。
- 重点验证输出目录与可本地打开的入口文件，确保产物可用。

### Performance & Reliability

- 构建时间主要由 mdBook 编译与搜索索引生成决定，脚本已采用一次性构建避免重复遍历。
- 静态导出期间对配置文件采用备份与恢复，降低配置污染风险。

## Architecture Design

- 构建流程：脚本驱动 → 符号链接更新 → 目录生成 → mdBook 构建 → 产物输出
- 配置管理：`book.toml` 临时调整用于 file:// 兼容，构建后恢复

## Directory Structure Summary

本次计划不涉及代码修改，仅使用现有脚本与配置文件执行。

- `gitbook/README.md`：跨平台使用说明与入口脚本说明  
- `gitbook/BUILD-README.md`：Windows 构建细节与编码注意事项  
- `gitbook/build-scripts/start.sh`：macOS 本地服务脚本  
- `gitbook/build-scripts/refresh.sh`：macOS 重新构建脚本  
- `gitbook/build-scripts/export-static.sh`：macOS 静态导出脚本  
- `gitbook/book.toml`：构建配置（脚本临时改写并恢复）