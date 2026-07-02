---
title: KUDIG-DATABASE 目录结构规范
description: '## 设计原则'
summary: '## 设计原则'
category: general
tags:
- k8s
- llm
- rag
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG-DATABASE 目录结构规范 是什么
- 如何 KUDIG-DATABASE 目录结构规范
trigger_keywords:
- KUDIG-DATABASE
- 目录结构规范
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG-DATABASE 目录结构规范

> 本文档定义 KUDIG-DATABASE 的目录层级与用途，供贡献者和 AI Agent 参考。
>
> 最后更新：2026-06-24

---

## 设计原则

1. **精简三层**：提炼知识（concepts/ entities/ skills/）+ 源文档（domain-*/ docs/）+ 工程/元数据
2. **Agent 优先**：目录命名与层级设计以 AI Agent 语料加载为首要目标
3. **最小根目录**：非 domain 目录精简至 10 个，通过 `_` 前缀区分元数据/工具目录
4. **显式排除**：Agent 语料配置（_meta/corpus-config/profiles/）显式声明排除非语料目录

---

## 目录层级

### 第一层：Wiki 提炼知识（Agent 优先读取）

这些目录存放经 `wiki-ingest` 提炼后的知识页面，**所有页面均含 frontmatter**（title, category, tags, tier, sources, summary 等）。

| 目录 | 内容 | chunking 策略 |
|:---|:---|:---|
| `concepts/` | 核心概念、架构模式、设计原理、综合分析（含原 synthesis/） | 按 H2 分块 |
| `entities/` | 组件实体、CNCF 工具、云产品、术语词典（含原 references/、research/） | 按 H2 分块 |
| `skills/` | 诊断排障、最佳实践、培训体系、FTA 方法（含原 best-practices/） | 按 Section 分块 |

**Agent 加载策略**：优先索引此层，Token 效率高，元数据丰富。

---

### 第二层：源文档（Agent 深度回退）

这些目录存放原始技术文档，由 `wiki-ingest` 处理并生成第一层页面。保留在原处是为了 Git 版本控制和深度查询兜底。

| 目录类型 | 数量 | 说明 |
|:---|:---:|:---|
| `domain-1*/` ~ `domain-20*/` | 20 个 | 按技术域分类的深度文档（架构、网络、存储、安全、AI Infra 等） |
| `topic-*/` | 10 个 | 按主题分类的文档（FTA、Skills、Learn、Cheat Sheet、Dictionary 等） |
| `docs/` | 1 个 | 映射与规范文档（API-DOC-MAP、COMMAND-DOC-MAP、FRONTMATTER-SPEC 等） |

**Agent 加载策略**：当提炼知识层无法回答深度技术细节时，回退到源文档层。

---

### 第三层：元数据与归档（Agent 不读取）

以 `_` 前缀标识，Agent 语料配置默认排除。

| 目录 | 用途 |
|:---|:---|
| `_archives/` | Wiki 归档快照（重建/恢复用） |
| `_meta/` | 元数据、语料配置、日志摘要（taxonomy、schema、corpus-config、journal、projects） |
| `_reports/` | 质量报告、评估报告、发布素材（含原 release-notes/） |
| `assets/` | 图片、图表、附件 |

---

### 第四层：工程工具（非语料，CI/CD 与构建用）

Agent 语料配置显式排除。修改前请检查脚本硬编码路径。

| 目录 | 用途 |
|:---|:---|
| `web/` | Astro 静态站点项目（含可视化页面，`npm run build`） |
| `scripts/` | 自动化脚本、模板、提示词、手册页（含原 templates/、prompts/、man/、video-scripts/） |

---

## 关键文件

| 文件 | 用途 |
|:---|:---|
| `AGENTS.md` | Agent 上下文与技能路由 |
| `README.md` | 项目介绍（人类阅读） |
| `STRUCTURE.md` | 本文件（目录结构规范） |
| `hot.md` | 热缓存（最近活动的语义快照） |
| `log.md` | 活动日志（摄入、更新、lint 记录） |

---

## Agent 语料加载路径

```yaml
# 提炼知识层（优先）
include:
  - concepts/
  - entities/
  - skills/

# 源文档层（深度兜底）
include:
  - domain-*/
  - docs/

# 显式排除（非语料）
exclude:
  - "_archives/"
  - "_meta/"
  - "_reports/"
  - "assets/"
  - "web/"
  - "scripts/"
  - ".git/"
  - ".ruff_cache/"
  - ".venv/"
```

详见 `_meta/corpus-config/profiles/` 下的场景化配置。

---

## 变更记录

| 日期 | 变更 |
|:---|:---|
| 2026-05-21 | 创建本文档；`metadata/` 移入 `_meta/metadata/`；`reports/` 改名为 `_reports/`；所有 profile 添加工具目录排除 |
| 2026-06-24 | 站点构建从 mdBook/mkDocs 迁移至 Astro（`web/`）；移除 `gitbook/`、`mkdocs.yml`；域结构确认为 20 个 Domain；工程工具层用 `web/` 取代 `gitbook/` |
| 2026-06-24 | **目录精简收编**: 24 个非 domain 目录精简至 10 个。`references/`→`entities/`、`synthesis/`→`concepts/`、`best-practices/`→`skills/`、`research/`→`entities/research/`；`corpus-config/`+`journal/`+`projects/`→`_meta/`；`templates/`+`prompts/`+`man/`+`video-scripts/`→`scripts/`；`visualizations/`→`web/`；`release-notes/`→`_reports/`；根目录散落文件归位；~1547 条 wikilink 批量重写 |


<!-- risk-assessed -->
