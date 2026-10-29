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
last_updated: 2026-10
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
> 最后更新：2026-10-26

---

## 设计原则

1. **四分区三层**：`wiki/` 源文档与提炼层 + `skills/` 技能卡 + `code/` 可执行资产 + `maintaining/` 维护资产
2. **Agent 优先**：目录命名与层级设计以 AI Agent 语料加载为首要目标
3. **分区内连续编号**：只有需要稳定阅读顺序的内容目录使用 `NN-` 前缀；代码目录使用英文语义名
4. **显式排除**：Agent 语料配置（`maintaining/02-元数据/corpus-config/profiles/`）显式声明排除非语料目录

---

## 目录层级

### 顶层四分区

| 顶层目录 | 内容 | Agent 语料 |
|:---|:---|:---:|
| `wiki/` | 语料正文：31 个编号知识域（编号 01-31，连续） | 是（主体） |
| `skills/` | Agent 技能卡：`00-总览` + 8 个技能域（编号 00-08） | 是 |
| `code/` | 可执行资产：`scripts/`、`kubernetes-hardware/`、`GTM/`、`repo/`（本地检出） | 否 |
| `maintaining/` | 维护资产：`01-文档/`、`02-元数据/`、`03-发布/`、`04-报告/` + 根级设计文档 | 部分 |

**约束**：顶层只允许这四个目录，加根级工程文件（`README.md`、`Makefile`、`pyproject.toml`、`pyrightconfig.json`、`requirements.txt`、`uv.lock`、`LICENSE`）。新增内容必须归入四分区之一。

---

### wiki/ 内的知识域分组

| 分组 | 目录 | 说明 |
|:---|:---|:---|
| 源文档域（01-21） | `01-集群基础` … `21-生态参考` | 按技术域组织的深度文档，Agent 深度回退层 |
| 提炼层（22-25） | `22-概念`、`23-实体`、`24-综合`、`25-研究` | 概念/实体/综合页面，Agent 优先读取 |
| 技术支撑（26-27） | `26-标签`、`27-资产` | 标签索引、图片与附件（`27-资产` 不进语料） |
| 专题与派生（28-31） | `28-源码分析`、`29-消化`、`30-百炼智能体`、`31-归档` | 源码解读、二次创作、Agentic Ops、归档快照 |

**编号约束**：每个分区独立从 `01` 连续编号，不跨分区占用编号；跨分区引用必须写全路径（如 `wiki/28-源码分析/...`、`maintaining/02-元数据/...`），不得只写 `NN-域名`。

---

### 域内结构

主流约定为三层：`NN-域名/` → `NN-子域/` → 文档。域根目录通常包含：

- `README.md`：本域说明 + 二级子目录表格（`readme-sync-check.py` 会校验）
- `index.md`：本域全量索引
- `00-总览/`：域级导读（31 个域中 24 个具备，非强制）

---

## 关键文件

| 文件 | 用途 |
|:---|:---|
| `README.md` | 项目介绍、阅读路径、质量门禁（人类入口） |
| `wiki/index.md` | 知识库首页索引（Obsidian vault 入口） |
| `skills/README.md` | 技能库入口 |
| `maintaining/01-文档/specs/STRUCTURE.md` | 本文件（目录结构规范） |
| `maintaining/02-元数据/AGENTS.md` | Agent 上下文与技能路由 |
| `Makefile` | 统一任务入口（语料生成、GTM 同步） |

---

## Agent 语料加载路径

```yaml
include:
  # 源文档域 + 提炼层：wiki/01-* ~ wiki/25-*
  - wiki/
  # 技能卡
  - skills/
  # 规范文档
  - maintaining/01-文档/

exclude:
  - wiki/27-资产/       # 二进制附件
  - wiki/31-归档/       # 归档快照
  - code/
  - maintaining/03-发布/
  - maintaining/02-元数据/
  - maintaining/04-报告/
  - .git/
  - .ruff_cache/
  - .venv/
```

权威判定以 `.github/workflows/quality.yml` 中的 `DOMAINS` / `CONTENT` 集合为准，场景化配置见 `maintaining/02-元数据/corpus-config/`。

---

## 变更记录

| 日期 | 变更 |
|:---|:---|
| 2026-05-21 | 创建本文档；`metadata/` 移入 `_meta/metadata/`；`reports/` 改名为 `_reports/`；所有 profile 添加工具目录排除 |
| 2026-06-24 | 站点构建从 mdBook/mkDocs 迁移至 Astro（`web/`）；移除 `gitbook/`、`mkdocs.yml`；域结构确认为 20 个 Domain；工程工具层用 `web/` 取代 `gitbook/` |
| 2026-06-24 | **目录精简收编**: 24 个非 domain 目录精简至 10 个。`references/`→`entities/`、`synthesis/`→`concepts/`、`best-practices/`→`skills/`、`research/`→`entities/research/`；`corpus-config/`+`journal/`+`projects/`→`_meta/`；`templates/`+`prompts/`+`man/`+`video-scripts/`→`scripts/`；`visualizations/`→`web/`；`release-notes/`→`_reports/`；根目录散落文件归位；~1547 条 wikilink 批量重写 |
| 2026-10-25 | **顶层四分区重构**：数字编号知识域（01-25 / 27 / 28 / 34 / 37 / 38 / 39）迁入 `wiki/`；`26-技能` 迁入 `skills/` 并去掉编号层；`31-脚本`、`kubernetes-hardware`、`GTM` 迁入 `code/`；`29-文档`、`32-发布`、`35-元数据`、`36-报告` 与根级设计文档迁入 `maintaining/`。同步重写 37,433 条 wikilink、2,892 条相对 md 链接与 34,098 处纯文本路径；CI 与脚本的仓库根探测层级 +1 |
| 2026-10-26 | **二级目录编号重整**：各分区改为独立连续编号——`wiki/` 由 01-39 断号收敛为 01-31（`27-标签`→`26-标签`、`28-资产`→`27-资产`、`34-源码分析`→`28-源码分析`、`38-消化`→`29-消化`、`39-百炼智能体`→`30-百炼智能体`、`37-归档`→`31-归档`）；`maintaining/` 重排为 `01-文档`、`02-元数据`、`03-发布`、`04-报告`；`code/31-脚本` 改为 `code/scripts`；`skills/` 保持 `00-总览` + 01-08。同步替换全库路径引用，历史审计报告与一次性迁移脚本保持原样 |


<!-- risk-assessed -->
