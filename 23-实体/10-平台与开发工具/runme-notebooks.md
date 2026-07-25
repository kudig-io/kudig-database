---
title: Runme (entities)
description: '## 概述'
summary: 'Runme 是一个交互式 Markdown 运行时，可以将 Markdown 文档中的代码块转化为可执行的交互式笔记本。它让开发者可以直接在 VS Code 中运行 README、runbook 和文档中的命令，并保存执行结果。Runme 特别适合 DevOps、SRE 运维手册和开发文档的交互式执行。'
category: entities
tags:
- k8s
- cncf
- platform
- runme-notebooks
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Runme 是什么
- 如何 Runme
trigger_keywords:
- Runme
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Runme

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go, TypeScript

## 概述

Runme 是由 Stateful 公司开发的交互式 Markdown 执行环境，2023 年进入 CNCF Sandbox。它将普通的 Markdown 文档（如 README、runbook、操作手册）转化为**可执行的笔记本（Executable Notebook）**——文档中的代码块和 shell 命令可以直接在 VS Code 中运行，并将输出结果持久化保存到文档中。

Runme 的核心理念是"**文档即代码即执行（Docs as Code as Execution）**"。传统运维文档（Wiki/Confluence）容易过时，而 Runme 确保文档中的命令始终可执行、可验证。它与 Kubernetes 深度集成，可以直接在 Markdown 中运行 `kubectl` 命令并将集群状态输出嵌入文档。

## Key Features

- **可执行 Markdown**：Shell、Python、JavaScript、SQL 代码块直接在 VS Code 中运行
- **环境隔离**：每个 Notebook Session 使用独立的环境变量和 Shell 上下文
- **结果持久化**：命令输出保存为 Markdown 内容，支持后续对比和审计
- **VS Code 集成**：原生 VS Code 扩展，提供 Notebook UI（类似 Jupyter）
- **CLI 工具**：`runme` CLI 可在 CI/CD 中批量执行文档命令
- **GitHub Actions**：将 Runme Notebook 集成到 CI/CD 流水线自动验证文档

## Architecture

Runme 由 **VS Code Extension**（前端，基于 Jupyter Notebook API 渲染可执行单元格）、**Runme Kernel**（执行引擎，管理 Shell Session 和环境变量）和 **Notebook Serializer**（Markdown ↔ Notebook 双向转换器）构成。CLI 模式下提供 `runme run` 和 `runme fmt` 等命令，用于自动化执行。支持通过 gRPC 协议远程连接执行后端。

## K8s 集成

Runme 可以直接在 Markdown 中执行 Kubernetes 命令。配合 VS Code Kubernetes 扩展，开发者可以在文档中嵌入 `kubectl get pods`、`kubectl describe` 等命令并实时查看集群状态。也支持在 CI 中使用 `runme run --filename=ops.md` 批量执行运维脚本。

## 生产部署要点

- **文档即代码**：将 runbook 和文档作为代码纳入版本控制
- **环境隔离**：使用 Runme 的环境变量功能隔离不同环境配置
- **分段执行**：将长流程拆分为多个单元格，便于调试和复用
- **结果保存**：保存执行输出，便于问题排查和审计
- **协作分享**：使用 Runme Cloud 分享带结果的 Notebook

## 生产场景

1. **SRE 运维 Runbook**：将故障排查步骤文档化，值班工程师直接在 VS Code 中执行
2. **新成员 Onboarding**：环境配置文档中每一步命令都可执行，确保一致性
3. **Kubernetes 运维手册**：将 kubectl 操作序列保存为可执行的 Markdown 文档
4. **CI 文档验证**：GitHub Actions 中自动执行文档命令，确保文档不过时

## 安装与配置

```bash
# 安装 Runme CLI
brew install runme
# 或
curl -fsSL https://runme.dev/install.sh | bash

# VS Code 中安装扩展
# 搜索 "Runme" 并安装

# 在项目中使用
runme run --filename=README.md    # 执行 README 中的命令
runme fmt --filename=README.md     # 格式化 Markdown
runme list                         # 列出所有可执行单元格
```

### CI/CD 集成配置

```yaml
# GitHub Actions 中使用 Runme 验证文档
name: Validate Runbooks
on: [push, pull_request]
jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install Runme
        run: curl -fsSL https://runme.dev/install.sh | bash
      - name: Execute runbook
        run: runme run --filename=docs/runbook.md --yes
        env:
          KUBECONFIG: ${{ secrets.KUBECONFIG }}
```

## 运维操作

```bash
# 🟢 列出文档中所有可执行单元格
runme list --filename=ops-runbook.md

# 🟡 执行指定单元格
runme run --filename=ops-runbook.md --cell="check-cluster-status"

# 🟡 批量执行所有命令
runme run --filename=ops-runbook.md --yes

# 🟢 格式化 Markdown 文档
runme fmt --filename=README.md

# 🟢 查看执行历史
runme history

# 🟡 导出执行结果为 JSON
runme run --filename=ops.md --output=json > results.json
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 命令执行失败 | 环境变量未设置 | 检查 .env 文件 | 配置 Runme 环境变量 |
| kubectl 命令无权限 | kubeconfig 未配置 | `kubectl cluster-info` | 设置 KUBECONFIG 环境变量 |
| 单元格无法识别 | Markdown 格式不正确 | `runme list` | 使用 `runme fmt` 修复格式 |
| 执行超时 | 命令阻塞等待输入 | 检查命令是否需要交互 | 添加 --yes 或非交互标志 |
| 输出未保存 | 序列化器问题 | 检查 VS Code 扩展版本 | 更新 Runme 扩展到最新版 |

## 生产案例

### 案例1: SRE 团队 Runbook 标准化

**场景**: 故障处理文档过时，值班工程师无法确认命令是否有效  
**排查**: Wiki 文档中 30% 的命令已过时或参数错误  
**方案**: 迁移到 Runme Notebook + CI 自动验证 + 版本控制  
**效果**: 文档有效率 100%，MTTR 降低 40%  

### 案例2: 新成员 Onboarding 自动化

**场景**: 新工程师环境配置需 2 天，步骤经常遗漏  
**方案**: 将环境配置文档转为 Runme Notebook，每步可执行可验证  
**效果**: Onboarding 时间从 2天缩短到 2小时  

## 对比

| 特性 | Runme | Jupyter Notebook | Quarto |
|------|-------|------------------|--------|
| 文档格式 | Markdown | .ipynb (JSON) | Markdown/RMarkdown |
| Shell 执行 | ✅ 原生 | ⚠️ 需 Kernel | ⚠️ |
| VS Code 集成 | ✅ 深度 | ✅ | ⚠️ |
| Git 友好 | ✅ 纯文本 | ❌ JSON | ✅ |
| 运维场景 | ✅ 核心场景 | ❌ | ⚠️ |
| CI/CD 集成 | ✅ GitHub Actions | ⚠️ | ⚠️ |

## 检查清单

- [ ] 运维 Runbook 纳入版本控制
- [ ] CI 中配置文档自动验证
- [ ] 配置环境变量隔离不同环境
- [ ] 保存执行结果用于审计
- [ ] 定期运行文档验证确保命令有效
- [ ] 团队统一使用 Runme 格式规范

## 参考链接

- [[deployment]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kpt]] — kpt
- [[logging-operator]] — Logging Operator
- [[kubeclipper]] — KubeClipper
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/15-参考与索引/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
