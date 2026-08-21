---
title: KUDIG Database 目录结构评估与改进建议
category: references
tags:
- structure
- llm-wiki
- rag
- assessment
- best-practices
tier: supporting
created: '2026-07-09'
last_updated: '2026-07-09'
---

# KUDIG Database 目录结构评估与改进建议

> 评估范围：仓库根目录（第一层）及各内容目录下的第二层子目录。
> 评估基准：项目自身 `docs/STRUCTURE.md`、`README.md`、`_meta/taxonomy.md` 以及 LLM Wiki 通用最佳实践（`llm-wiki/SKILL.md`）。

---

## 1. 执行摘要

| 评估维度 | 评分（1–5） | 关键结论 |
|---|---|---|
| 命名规范与分类逻辑 | 2 | 中英文混用、编号规则不一致，源文档层未按 `domain-XX-slug` 命名 |
| 信息层级清晰度 | 2 | 提炼知识层与源文档层在根目录平铺共存，隐藏工具目录与源码目录混入根目录 |
| 检索与维护便利性 | 2 | `concepts/`/`entities/`/`skills/` 过度扁平（数百文件平铺），语料配置引用大量不存在的英文 domain 路径 |
| 语料库使用场景契合度 | 2 | 语料 profile 与实际目录脱节；`code/`、`release/`、隐藏 Agent 目录存在被误纳入语料的风险 |
| 与项目规范一致性 | 2 | `docs/STRUCTURE.md` 规划的 `domain-01..20/` 结构在实际仓库中不存在；`README.md` 与实际目录不完全一致 |

**总体判断**：当前仓库已经完成了“双层知识库”的内容积累，但目录结构处于“迁移中/半规范化”状态。建议先通过映射表和 `.gitignore` 等低风险手段止血，再集中一次批量重命名完成规范化。

---

## 2. 第一层（根目录）现状盘点

根目录共 **43 个顶层目录/文件**（含隐藏目录），远超 `docs/STRUCTURE.md` 提出的”非 domain 目录精简至 10 个”的目标。

### 2.1 提炼知识层（Agent 优先读取）

| 目录 | 类型 | Markdown 文件数 | 第二层现状 |
|---|---|---:|---|
| `concepts/` | 概念/模式/综合分析 | 280 | 仅 `case-studies/`，243 个文件直接堆放在根下 |
| `entities/` | 组件/工具/实体/术语 | 383 | 仅 `research/`，368 个文件直接堆放在根下 |
| `skills/` | 诊断/最佳实践/培训 | 152 | 有 `best-practices/`、`training-lecturer/`、`training-public/`，但 141 个文件仍平铺 |
| `synthesis/` | 跨域综合分析 | 10 | 无第二层子目录 |

### 2.2 源文档层（原始深度文档）

实际以 **20 个中文目录** 承载，而非规范中的 `domain-01..20-<slug>/`。

| 目录 | 域编号 | 推荐英文 slug | 第一层 Markdown | 第二层子目录数 |
|---|---|---|---|---:|
| `集群基础/` | 01 | `cluster-fundamentals` | 5 | 8 |
| `工作负载/` | 02 | `workloads-applications` | 11 | 4 |
| `网络/` | 03 | `networking-traffic` | 3 | 7 |
| `存储/` | 04 | `storage-data` | 4 | 5 |
| `安全/` | 05 | `security-compliance` | 5 | 8 |
| `可观测性/` | 06 | `observability` | 5 | 8 |
| `平台工程/` | 07 | `platform-engineering` | 7 | 6 |
| `发布变更/` | 08 | `release-change-management` | 3 | 6 |
| `可靠性/` | 09 | `reliability-engineering` | 4 | 9 |
| `故障诊断/` | 10 | `troubleshooting-diagnostics` | 4 | 11 |
| `生产运维/` | 11 | `production-operations` | 17 | 6 |
| `云厂商/` | 12 | `cloud-providers` | 3 | 16 |
| `容器运行时/` | 13 | `container-runtime` | 4 | 6 |
| `AI基础设施/` | 14 | `ai-ml-infra` | 3 | 5 |
| `专项技术/` | 15 | `specialized-tech` | 4 | 5 |
| `数据库中间件/` | 16 | `database-middleware` | 4 | 7 |
| `系统基础/` | 17 | `system-foundation` | 3 | 5 |
| `清单模式/` | 18 | `manifests-patterns` | 3 | 4 |
| `生态参考/` | 19 | `landscape-references` | 3 | 5 |
| `应用模式/` | 20 | `application-patterns` | 3 | 4 |

### 2.3 元数据/工程/遗留目录

| 目录/文件 | 规划角色 | 现状问题 |
|---|---|---|
| `_archives/` | 归档 | 正常 |
| `_meta/` | 元数据/语料配置 | 正常 |
| `_reports/` | 报告 | 正常 |
| `assets/` | 图片/附件 | 正常 |
| `scripts/` | 脚本/模板/提示词 | 正常，但包含 `video-scripts/` 等可接受 |
| `web/` | Astro 站点 | 正常（`node_modules/` 已排除） |
| `docs/` | 映射/规范文档 | `javascripts/`、`stylesheets/`、`visualizations/` 更像 `web/` 资产 |
| `research/` | 研究资料 | 仅 2 个文件，按规范应并入 `entities/research/` |
| `release/` | 发布产物 | 含 `corpus 2/`（空格命名）等，未在 `docs/STRUCTURE.md` 中出现 |
| `tags/` | 标签索引 | 生成物，建议纳入 `_meta/tags/` 或仅保留生成脚本 |
| `code/` | 源码镜像 | 不在规范中，体积大，应迁出或显式排除 |
| `.claude/` `.codebuddy/` `.comate/` `.mimocode/` `.qoder/` `.understand-anything/` `.zcode/` `.zread/` | Agent 工具配置 | 未全部加入 `.gitignore`，存在被语料扫描和 Git 追踪的风险 |

---

## 3. 第二层目录现状与问题

### 3.1 编号体系不统一

| 模式 | 示例 | 问题 |
|---|---|---|
| `01-` 起始 | `安全/01-identity-access/` | 正常 |
| `00-` 起始 | `网络/00-core-k8s-networking/`、`工作负载/00-core-workloads/` | 与多数域的 `01-` 不一致 |
| 无编号 | `平台工程/build/`、`生态参考/topic-index/` | 无法通过排序判断章节顺序 |
| `topic-*` | `19-故障诊断/06-FTA故障树/`、`发布变更/topic-deployment/` | 部分跨域主题应归属到全局 `skills/` 或 `concepts/` |
| `98-merged-indexes` | 各域均有 | 生成索引位置固定，尚可接受 |
| `99-` 文件 | `99-production-readiness-operations-guide.md` | 作为域级入口文件，位置合理 |
| `_archived-release-notes` | `生态参考/_archived-release-notes/` | 下划线前缀按规范应仅用于根级元数据目录 |

### 3.2 提炼知识层扁平化严重

- `concepts/`：243/280 文件位于根下，仅有 `case-studies/` 一个逻辑分组。
- `entities/`：368/383 文件位于根下，仅有 `research/`。
- `skills/`：141/152 文件位于根下，培训相关文件散落在 `training-lecturer/`、`training-public/` 和根下。

**后果**：Agent 在检索时难以通过目录前缀做低成本过滤；人类维护者难以判断某概念应放在哪里。

### 3.3 语料配置与实际目录脱节

`_meta/corpus-config/profiles/` 中多个 profile 同时引用：
- 实际存在的中文目录（如 `集群基础/`、`19-故障诊断/06-FTA故障树/`）
- **已不存在的旧版英文目录**（如 `domain-1-architecture-fundamentals/`、`domain-12-troubleshooting/`、`生产运维/topic-learn/`）

这会导致语料生成时路径缺失、部分关键文档未被摄入。

---

## 4. 改进建议

### 4.1 第一层规范化（推荐最终形态）

```
.
├── concepts/                    # 提炼知识：核心概念、架构模式、综合分析
├── entities/                    # 提炼知识：组件实体、CNCF 工具、云产品、术语词典
├── skills/                      # 提炼知识：诊断排障、最佳实践、培训体系
├── synthesis/                   # 提炼知识：跨域综合分析
├── 集群基础/   # 源文档：集群基础
├── 工作负载/ # 源文档：工作负载
├── ...                          # domain-03..20
├── docs/                        # 映射/规范/评估文档
├── _meta/                       # 元数据、语料配置、日志
├── _reports/                    # 质量报告、评估、发布素材
├── _archives/                   # Wiki 归档快照
├── assets/                      # 图片、图表、附件
├── scripts/                     # 自动化脚本、模板、提示词
└── web/                         # Astro 静态站点
```

**迁移说明**：
- 将 20 个中文目录按 `_meta/domain-mapping.md` 的映射表批量重命名为 `domain-XX-<slug>/`。
- `research/` → `entities/research/`。
- `tags/` → 移入 `_meta/tags/` 或由脚本生成，不常驻根目录。
- `release/` → 建议合并到 `_reports/release-package/`，或至少重命名 `corpus 2/` 为 `corpus/`。
- `code/` → 建议迁出仓库，或放入 `assets/code-samples/` 并在语料配置中显式排除。
- 所有 Agent 工具目录加入 `.gitignore`（见第 5 节已实施改动）。

### 4.2 第二层标准化模板（每个 domain）

```
domain-XX-<slug>/
├── 00-overview/                          # 域 overview、学习路径、快速入口
├── 01-<core-topic-1>/                    # 核心章节
├── 02-<core-topic-2>/
├── ...
├── 98-merged-indexes/                    # 自动生成索引（保留）
├── 99-production-readiness-operations-guide.md  # 域级生产就绪指南
└── topic-<cross-cutting>/                # 仅当子主题为跨域专题时使用
```

**针对当前 20 个域的具体推荐映射**，参见 `_meta/domain-mapping.md`。

### 4.3 提炼知识层第二层建议

| 目录 | 建议第二层分组 |
|---|---|
| `concepts/` | `concepts/<domain-slug>/`（按 20 个域归档）+ `concepts/case-studies/` + `concepts/patterns/` |
| `entities/` | `entities/tools/`、`entities/cloud-providers/`、`entities/projects/`、`entities/people/`、`entities/research/`、`entities/glossary/` |
| `skills/` | `skills/diagnostics/`、`skills/playbooks/`、`skills/best-practices/`、`skills/training/`、`skills/fta/` |
| `synthesis/` | `synthesis/cross-domain/`、`synthesis/case-studies/`、`synthesis/trend-analysis/` |

### 4.4 语料配置修复

- 统一使用 `domain-XX-<slug>/` 路径。
- 删除或替换所有引用旧版英文 domain 目录的条目。
- 显式 `exclude`：`code/`、`_archives/`、`_meta/`、`_reports/`、`assets/`、`web/`、`scripts/`、所有 Agent 工具目录、`release/`（若保留）。
- 为 `concepts/`、`entities/`、`skills/`、`synthesis/` 配置按 H2/Section 分块。

---

## 5. 已实施的低风险优化

为避免在未确认的情况下做大规模重命名，本次先完成以下不破坏现有路径的改进：

1. **新增 `.gitignore` 规则**：将 `.codebuddy/`、`.comate/`、`.mimocode/`、`.qoder/`、`.understand-anything/`、`.zcode/`、`.zread/` 等 Agent 工具目录加入忽略列表，防止其进入 Git 和语料扫描。
2. **新增 `_meta/domain-mapping.md`**：建立 20 个中文域目录 ↔ 域编号 ↔ 英文 slug ↔ taxonomy 标签 ↔ 当前第二层子目录 ↔ 推荐第二层结构的完整映射表，作为后续批量迁移的单一真相源。
3. **新增本文档 `docs/structure-assessment.md`**：将评估结论、评分、推荐结构落盘，便于团队审阅和持续跟踪。

---

## 6. 后续行动清单

| 优先级 | 行动 | 影响范围 | 建议执行方式 |
|---|---|---|---|
| P0 | 批量重命名 20 个中文域目录为 `domain-XX-<slug>/` | 全库 wikilink、profile、web 构建脚本 | 编写迁移脚本，批量重写 wikilink 和 profile 路径 |
| P0 | 修复语料 profile 中的失效路径 | `_meta/corpus-config/profiles/*.yaml` | 逐文件审计并替换为中文目录或迁移后的英文目录 |
| P1 | 为 `concepts/`/`entities/`/`skills/`/`synthesis/` 建立域级第二层子目录 | 提炼知识层所有页面 | 先建目录和索引，再逐步迁移；不要一次性移动未审阅页面 |
| P1 | 规范每个 domain 的第二层目录命名（统一 `00-overview/`、`NN-topic/`、`98-merged-indexes/`） | 20 个域 | 结合 P0 重命名一并完成 |
| P1 | 处置 `code/`、`research/`、`release/`、`tags/` | 根目录 | 按第 4.1 节建议迁移或排除 |
| P2 | 更新 `README.md` 和 `docs/STRUCTURE.md` 使其与实际目录一致 | 文档 | 在迁移完成后同步更新 |

---

## 7. 结论

KUDIG Database 已具备高质量的内容积累，但目录结构尚未完成从“中文域文件夹 + 扁平提炼层”到“规范化 LLM Wiki 双层结构”的最后一步。当前最大风险是：**语料配置引用大量不存在的路径，可能导致 RAG 召回缺失；根目录过度拥挤，降低 Agent 和人类维护者的导航效率**。

建议优先完成 `_meta/domain-mapping.md` 驱动的批量重命名和 profile 修复，再逐步细化提炼知识层的第二层分组。
