---
title: Meta 元数据索引
description: KUDIG 知识库元数据治理中枢 — 定义全库 Schema、分类体系、语料配置、知识图谱与质量标准
summary: 仓库元数据、Schema 定义和分类体系的入口文件，提供全库治理规范的快速导航
category: index
tags:
- index
- meta
- schema
- taxonomy
- governance
- visibility/public
tier: core
sources:
- _meta/
created: '2026-05-24'
updated: '2026-07-21'
last_updated: '2026-07-21'
status: reviewed
---

# Meta 元数据索引

> 仓库元数据、Schema 定义和分类体系。本目录是知识库的「治理层」，为所有内容页面提供统一的元数据规范和质量标准。

## 核心职责

| 职责 | 对应文件 | 说明 |
|------|----------|------|
| 元数据规范 | [[35-元数据/metadata/schema.md\|schema.md]] | Frontmatter 必填/可选字段、页面分类、命名约定、Wikilink 规范 |
| 标签体系 | [[35-元数据/metadata/taxonomy.md\|taxonomy.md]] | 20 个 Domain Tag + 内容标签 + 实体类型标签 |
| 目录映射 | [[35-元数据/metadata/domain-mapping.md\|domain-mapping.md]] | 20 中文知识域 + 14 支撑域的完整目录结构 |
| 数据视图 | [[35-元数据/journal/dashboard.md\|dashboard.md]] | Dataview 动态查询（全库总览/标签分组/陈旧内容） |
| Agent 规范 | [[35-元数据/AGENTS.md\|AGENTS.md]] | AI Agent 协作规则与行为约束 |

## 元数据索引文件

| 文件 | 用途 |
|------|------|
| [[35-元数据/metadata/knowledge-map.md\|knowledge-map.md]] | 知识模块依赖关系图 + 学习路径 |
| [[35-元数据/metadata/tags-index.md\|tags-index.md]] | 标签使用统计与索引 |
| [[35-元数据/metadata/difficulty-index.md\|difficulty-index.md]] | 按难度分级的内容索引 |

## RAG 语料配置

| 文件 | 用途 |
|------|------|
| [[35-元数据/corpus-config/rag-chunking-strategy.md\|rag-chunking-strategy.md]] | 分块策略指南（按标题/固定大小/整文档） |
| [[35-元数据/corpus-config/embedding-guide.md\|embedding-guide.md]] | Embedding Pipeline 完整使用指南 |
| [[35-元数据/corpus-config/README.md\|corpus-config README]] | 语料配置总览 |

## 变更日志与审计

- [[35-元数据/journal/log.md|log.md]] — 全库变更日志
- [[35-元数据/journal/index.md|journal/]] — 消化日志（按日期记录知识库变更摘要）
- [[35-元数据/metadata/content-audit-2026-07-11.md|content-audit]] — 内容审计报告

## 治理原则

1. **Schema 先行**：任何新页面必须符合 `schema.md` 定义的 frontmatter 规范
2. **标签受控**：仅使用 `taxonomy.md` 中已定义的标签，新标签需讨论后添加
3. **目录规范**：新页面按 `domain-mapping.md` 归入对应知识域
4. **只增不减**：严禁删除现有文件，仅新增或修改内容
5. **版本追溯**：所有重组使用 `git mv`，可通过 `git log --follow` 追溯

## Related

- [[27-标签/07-参考与最佳实践/visibility-public|visibility-public Hub]] — tag hub
- [[35-元数据/index.md|元数据目录索引]] — 完整索引
