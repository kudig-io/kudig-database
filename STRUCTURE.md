---
title: KUDIG-DATABASE 目录结构规范
description: '## 设计原则'
category: general
tags:
- k8s
- llm
- rag
- agent
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
created: "2026-05-23"
---

# KUDIG-DATABASE 目录结构规范

> 本文档定义 KUDIG-DATABASE 的目录层级与用途，供贡献者和 AI Agent 参考。
>
> 最后更新：2026-05-21

---

## 设计原则

1. **双层结构**：提炼知识（concepts/ entities/ skills/ ...）与源文档（domain-*/ topic-*/ docs/）共存
2. **Agent 优先**：目录命名与层级设计以 AI Agent 语料加载为首要目标
3. **最小侵入**：不移动已有目录（避免破坏 manifest 与 wikilink），通过 `_` 前缀区分元数据/工具目录
4. **显式排除**：Agent 语料配置（corpus-config/profiles/）显式声明排除非语料目录

---

## 目录层级

### 第一层：Wiki 提炼知识（Agent 优先读取）

这些目录存放经 `wiki-ingest` 提炼后的知识页面，**所有页面均含 frontmatter**（title, category, tags, tier, sources, summary 等）。

| 目录 | 页数 | 内容 | chunking 策略 |
|:---|:---:|:---|:---|
| `concepts/` | ~62 | 核心概念、架构模式、设计原理、运维知识 | 按 H2 分块 |
| `entities/` | ~265 | 组件实体、CNCF 工具、云产品、运行时 | 按 H2 分块 |
| `skills/` | ~140 | 诊断排障、最佳实践、培训体系、FTA 方法 | 按 Section 分块 |
| `references/` | ~101 | 术语词典、命令速查、云厂商对比、规范 | 按 H2 或条目分块 |
| `synthesis/` | ~13 | 跨领域综合分析、问题全景、决策框架 | 整文档 |
| `journal/` | ~2 | 日志与变更记录 | — |
| `projects/` | ~1 | 项目知识 | — |

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

### 第三层：元数据与工具（Agent 不读取）

以 `_` 前缀标识，Agent 语料配置默认排除。

| 目录 | 用途 |
|:---|:---|
| `_archives/` | Wiki 归档快照（重建/恢复用） |
| `_meta/` | 元数据定义（taxonomy.md、schema.md、metadata/*.md） |
| `_raw/` | 草稿暂存区（未处理的零散笔记，wiki-ingest 会自动消化） |
| `_staging/` | 审核队列（WIKI_STAGED_WRITES=true 时 LLM 写入的待审页面） |
| `_reports/` | 质量报告与统计数据（QUALITY_REPORT、STATS、评估报告等） |
| `corpus-config/` | AI 语料配置（RAG profile、分块策略、Embedding 指南） |
| `assets/` | 图片、图表、附件 |

---

### 第四层：工程工具（非语料，CI/CD 与构建用）

Agent 语料配置显式排除。修改前请检查脚本硬编码路径。

| 目录 | 用途 |
|:---|:---|
| `web/` | Astro 静态站点项目（站点源码，`npm run build` 输出到 `site/`） |
| `scripts/` | 自动化脚本（export-corpus.sh 等） |
| `templates/` | 文档模板 |
| `prompts/` | Agent 提示词 |
| `site/` | Astro 构建输出（由 `web/` 生成，已 gitignore） |
| `man/` | 手册页 |

---

## 关键文件

| 文件 | 用途 |
|:---|:---|
| `index.md` | Wiki 主索引（自动维护） |
| `log.md` | 活动日志（摄入、更新、lint 记录） |
| `hot.md` | 热缓存（最近活动的语义快照） |
| `.manifest.json` | 摄入追踪清单（3,523 条源记录） |
| `AGENTS.md` | Agent 上下文与技能路由 |
| `README.md` | 项目介绍（人类阅读） |
| `mkdocs.yml` | ~~已移除~~（站点构建已迁移至 `web/` Astro） |
| `STRUCTURE.md` | 本文件（目录结构规范） |

---

## Agent 语料加载路径

```yaml
# 提炼知识层（优先）
include:
  - concepts/
  - entities/
  - skills/
  - references/
  - synthesis/

# 源文档层（深度兜底）
include:
  - domain-*/
  - topic-*/
  - docs/

# 显式排除（非语料）
exclude:
  - "_archives/"
  - "_meta/"
  - "_raw/"
  - "_staging/"
  - "_reports/"
  - "corpus-config/"
  - "assets/"
  - "web/"
  - "scripts/"
  - "templates/"
  - "prompts/"
  - "site/"
  - "man/"
  - ".git/"
  - ".ruff_cache/"
  - ".venv/"
```

详见 `corpus-config/profiles/` 下的场景化配置。

---

## 变更记录

| 日期 | 变更 |
|:---|:---|
| 2026-05-21 | 创建本文档；`metadata/` 移入 `_meta/metadata/`；`reports/` 改名为 `_reports/`；所有 profile 添加工具目录排除 |
| 2026-06-24 | 站点构建从 mdBook/mkDocs 迁移至 Astro（`web/`）；移除 `gitbook/`、`mkdocs.yml`；域结构确认为 20 个 Domain；工程工具层用 `web/` 取代 `gitbook/` |
