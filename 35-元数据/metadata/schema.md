---
title: Wiki Frontmatter Schema
description: KUDIG 知识库 Frontmatter 元数据规范 — 定义所有页面的结构化元数据字段、页面分类、命名约定、Wikilink
  规范和内容指南
summary: 全库统一的 Frontmatter Schema 规范，涵盖必填字段、可选字段、页面分类体系、命名约定、Wikilink 规则、源归因和内容质量指南
category: references
tags:
- schema
- metadata
- frontmatter
- governance
- rag
- agent
tier: core
created: '2026-05-23'
last_updated: '2026-07-23'
difficulty: intermediate
audience:
- 所有工程师
- AI Agent
estimated_read_time: 10min
---

# Wiki Frontmatter Schema

> 本文档定义 KUDIG 知识库的元数据规范。所有页面必须遵循此 Schema。规范与 Agent 共同演进：Agent 提议变更，用户审批后生效。

## 页面分类体系 (Page Categories)

| Category | 目录 | 内容说明 | 典型示例 |
|---|---|---|---|
| **Entities** | `23-实体/` | 工具、产品、公司、人物 | Docker, Prometheus, CNCF |
| **Concepts** | `22-概念/` | 概念、模式、技术、方法 | Pod 生命周期, Raft 共识 |
| **Syntheses** | `24-综合/` | 跨域综合、对比分析、趋势 | 多云对比, 可观测性全景 |
| **Sources** | `25-研究/` | 研究性笔记、调研报告 | Service Mesh 演进研究 |
| **Skills** | `26-技能/` | Agent Skill 定义、能力模块 | K8s 排障技能 |
| **References** | 各域内 | 参考文档、速查表、配置模板 | YAML 参考, kubectl 速查 |
| **Index** | 各目录 | 索引页、导航入口 | index.md, README.md |

## 命名约定 (Naming Conventions)

| 规则 | 说明 | 示例 |
|------|------|------|
| 文件名 | `kebab-case.md`，ASCII 字符 | `pod-lifecycle.md` |
| 编号前缀 | 新增文件用 `NN-topic-name.md` | `01-cluster-architecture.md` |
| 标题 | frontmatter `title:` 用 Title Case 或中文 | `Pod 生命周期管理` |
| 目录 | `NN-` 数字前缀 + 中文简称，英文工具名保留 | `01-GitOps/`, `05-eBPF/`, `03-控制平面/` |
| 索引文件 | 每个目录必须有 `index.md` 或 `README.md` | — |

## Wikilink 规范

| 规则 | 说明 |
|------|------|
| 解析机制 | Obsidian 按**文件名**解析 wikilink，非 frontmatter title |
| 标准格式 | `[[路径/文件名\|显示文本]]` |
| 跨域引用 | `[[NN-中文域/README]]` 链接到域入口，如 `[[19-故障诊断/README]]` |
| 章节引用 | `[[文件名#章节名\|显示文本]]` |
| 完整性 | 每个提及的实体/概念应有对应页面（或在 "missing pages" 列表中） |
| 批量修复 | 批量创建页面后运行 lint/fix 流程自动修复 wikilink 格式 |

## Frontmatter 必填字段

使用简单无引号 `key: value` 格式 — 不用引号键、不用块标量、不用多行值。
Tags 和 aliases 使用 YAML 列表格式。

```yaml
title: <string>           # 页面标题（必填）
category: <string>        # 页面分类（必填）
tags:                     # 标签列表（必填，从 taxonomy.md 选取）
- tag1
- tag2
tier: <string>            # 内容层级（必填）
created: 'YYYY-MM-DD'     # 创建日期（必填）
last_updated: 'YYYY-MM-DD' # 最后更新（必填）
```

### Tier 层级定义

| Tier | 含义 | 典型内容 |
|------|------|----------|
| `core` | 核心知识，高频引用 | 架构设计、核心概念、关键工具 |
| `supporting` | 支撑知识，中频引用 | 实践指南、配置参考、对比分析 |
| `peripheral` | 外围知识，低频引用 | 历史记录、归档内容、边缘场景 |

### 特殊页面

`index.md` 和 `log.md` 使用简化 frontmatter：仅需 `title`、`category`、`tags`。
它们不参与孤立页检测和内容页 lint 检查。

## 可选 Frontmatter 字段

```yaml
description: <string>     # 页面描述（用于搜索和摘要）
summary: <string>         # 内容摘要（1-2 句）
difficulty: beginner | intermediate | advanced | expert
audience:                 # 目标受众
- SRE
- 平台工程师
status: active | review | stale | archived | done
sources:                  # 源文件归因
- raw/path/to/source.md
estimated_read_time: <string>  # 预估阅读时间
aliases:                  # 别名（用于 wikilink 解析）
- alternative-name
```

## 源归因规范 (Source Attribution)

| 方式 | 用途 | 示例 |
|------|------|------|
| frontmatter `sources:` | 文件级归因 | `sources: [raw/notes.md]` |
| `%%from: path%%` | 段落级归因 | `%%from: raw/interview.md%%` |
| `%%inferred%%` | LLM 综合多源 | 跨文档推理内容 |
| `%%ambiguous: reason%%` | 源冲突标注 | 当多个源不一致时 |

> ❗ **禁止使用** `^[...]` — 那是 Obsidian 的内联脚注语法，会干扰解析。

## 内容质量指南

### 写作原则

1. **定义先行**：第一句话定义实体/概念
2. **简洁精确**：Wiki 页面是参考材料，不是论文
3. **结构化**：使用列表、表格、代码块组织信息
4. **积极交叉引用**：提及的概念/实体必须链接
5. **跟踪开放问题**：每页可有 `## Open Questions` 章节
6. **显式标注矛盾**：不默默选择一方，明确注明分歧

### 单文件强化标准（9 个必备章节）

每个深度内容文件应包含：

1. 完整的 YAML frontmatter
2. 概述与核心概念（原理深度解析）
3. 架构设计与组件关系
4. 生产级配置与操作指南（含完整 YAML/命令示例）
5. 故障排查与诊断流程
6. 最佳实践与反模式
7. 性能调优与监控指标
8. 版本兼容性与升级注意事项
9. 参考链接与延伸阅读

## Schema 演进流程

当识别到规范需要变更时：

1. 在对话中提议变更
2. 用户审批后更新本文档
3. 在 `log.md` 中记录 Schema 变更
4. 通知所有 Agent 更新行为

### 变更历史

| 日期 | 变更 | 影响 |
|------|------|------|
| 2026-05-23 | 初始 Schema 定义 | 全库 |
| 2026-05-24 | 添加 Content Tags 补全 | taxonomy.md |
| 2026-07-10 | 目录扁平化重组 | domain-mapping.md |
| 2026-07-21 | 强化 tier 定义 + 单文件标准 | 全库 |
| 2026-07-23 | 一级/二级目录数字前缀有序化（`NN-`） | 全库、domain-mapping.md |

## Related

- [[35-元数据/metadata/taxonomy.md|Tag Taxonomy]] — 标签分类体系
- [[35-元数据/metadata/domain-mapping.md|Domain 映射]] — 目录结构规范
- [[35-元数据/corpus-config/rag-chunking-strategy.md|RAG 分块策略]] — 语料分块规范
