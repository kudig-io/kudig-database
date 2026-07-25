---
title: 元数据目录索引
description: KUDIG 知识库元数据管理中枢 — Schema 定义、分类体系、语料配置、知识图谱、变更日志
summary: 知识库元数据管理中枢，涵盖 Frontmatter Schema、Tag Taxonomy、Domain 映射、RAG 语料配置、知识图谱、质量审计
  等核心治理文件
category: index
tags:
- index
- meta
- schema
- taxonomy
- governance
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---

# 元数据目录索引

> 本目录是 KUDIG 知识库的**治理中枢**，定义全库的元数据规范、分类体系、语料配置和质量标准。

## 核心治理文件

| 文件 | 职责 | 重要度 |
|------|------|--------|
| [[35-元数据/metadata/schema.md\|Schema]] | Frontmatter 字段规范、页面分类、命名约定 | ⭐⭐⭐ |
| [[35-元数据/metadata/taxonomy.md\|Taxonomy]] | 标签分类体系（20 Domain + 内容标签） | ⭐⭐⭐ |
| [[35-元数据/metadata/domain-mapping.md\|Domain 映射]] | 20 中文知识域 + 14 支撑域目录结构 | ⭐⭐⭐ |
| [[35-元数据/journal/dashboard.md\|Dashboard]] | Dataview 动态数据视图 | ⭐⭐ |
| [[35-元数据/AGENTS.md\|Agents]] | AI Agent 协作规范 | ⭐⭐ |
| [[35-元数据/journal/hot.md\|Hot]] | 热点内容追踪 | ⭐ |
| [[35-元数据/journal/log.md\|Log]] | 变更日志 | ⭐ |

## 子目录

| 子目录 | 职责 | 内容 |
|--------|------|------|
| [[35-元数据/corpus-config/index.md\|corpus-config/]] | RAG 语料配置 | 分块策略、Embedding 指南、Profile 配置 |
| [[35-元数据/journal/index.md\|journal/]] | 消化日志 | 知识库变更摘要、热点追踪 |
| [[35-元数据/metadata/index.md\|metadata/]] | 元数据索引 | 知识图谱、标签索引、难度索引 |
| [[35-元数据/projects/index.md\|projects/]] | 项目计划 | 模板目录、语料改进计划 |

## 知识洞察

- [[35-元数据/journal/_insights.md|Insights]] — 知识库质量洞察
- [[35-元数据/journal/_insights 2.md|Insights 2]] — 补充洞察
- [[35-元数据/metadata/content-audit-2026-07-11.md|Content Audit]] — 2026-07-11 内容审计报告

## 使用指南

1. **新建页面**：先查阅 `schema.md` 确认 frontmatter 必填字段
2. **选择标签**：从 `taxonomy.md` 中选取规范标签
3. **确定目录**：按 `domain-mapping.md` 选择目标位置
4. **语料配置**：RAG 相关配置参考 `corpus-config/`
5. **质量检查**：定期查看 `dashboard.md` 中的陈旧内容视图
