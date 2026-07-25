---
title: AI 语料库配置 (Corpus Config)
description: 面向 RAG/NotebookLM/AI Agent 的语料配置和最佳实践，含分块策略、Embedding 选型、场景化 Profile
summary: KUDIG 知识库作为 AI 语料的配置指南，涵盖分块策略、Embedding 模型选型、场景化 Profile 配置和向量化 Pipeline
category: references
tags:
- rag
- embedding
- corpus-config
- agent
- pipeline
tier: supporting
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: intermediate
audience:
- AI 工程师
- 平台工程师
estimated_read_time: 8min
---

# AI 语料库配置 (Corpus Config)

> 面向 NotebookLM / IMA / RAG / AI Agent 等场景的语料配置和最佳实践。本目录定义了如何将 KUDIG 知识库转化为高质量的向量化语料。

---

## 目录结构

```
_meta/corpus-config/
├── README.md                           # 本文件
├── rag-chunking-strategy.md            # RAG 分块策略指南
├── embedding-guide.md                  # Embedding 选型与配置
└── profiles/                           # 场景化语料配置
    ├── notebooklm-profile.yaml         # NotebookLM 推荐配置
    ├── rag-sre-profile.yaml            # SRE 运维 Agent 语料
    ├── rag-learning-profile.yaml       # 学习场景语料
    └── rag-full-profile.yaml           # 全量语料配置
```

## 语料特点

本知识库作为 AI 语料具备以下优势：

| 特点 | 说明 |
|:---|:---|
| **结构化** | 统一的 Markdown 格式、标题层级、表格结构 |
| **领域专精** | 聚焦 Kubernetes + AI Infra，非泛化内容 |
| **生产级** | 所有配置经过验证，非玩具示例 |
| **多粒度** | Domain 深度文档 + Cheat Sheet 速查 + FTA 推理骨架 |
| **交叉引用** | 文档间建立了关联关系，增强语义理解 |

## 推荐使用场景

| 场景 | 推荐导入 | 配置文件 |
|:---|:---|:---|
| NotebookLM 播客 | topic-fta + topic-learn | [notebooklm-profile.yaml](./profiles/notebooklm-profile.yaml) |
| SRE Agent | topic-fta + topic-skills + 故障诊断 | [rag-sre-profile.yaml](./profiles/rag-sre-profile.yaml) |
| K8s 学习助手 | topic-learn + topic-cheat-sheet + domain-1~6 | [rag-learning-profile.yaml](./profiles/rag-learning-profile.yaml) |
| 全知识库 | 全部目录 | [rag-full-profile.yaml](./profiles/rag-full-profile.yaml) |

## 语料质量保障

### 质量检查清单

| 检查项 | 标准 | 工具 |
|--------|------|------|
| Frontmatter 完整性 | title + tags + category 必填 | `--evaluate` 模式 |
| 标题层级规范 | H1 唯一，H2/H3 递进 | Markdown lint |
| 内容完整性 | 无空章节、无 TODO | 手动审计 |
| 交叉引用有效性 | wikilink 指向存在的文件 | lint/fix 流程 |
| 分块友好性 | 每个 H2 章节可独立理解 | 分块测试 |

### 语料更新流程

```
1. 内容修改 → 2. 运行 --evaluate 检查质量
     → 3. 运行 --incremental 增量更新向量
     → 4. 运行 --search 验证检索效果
```

## Related

- [[35-元数据/corpus-config/rag-chunking-strategy.md|RAG 分块策略]] — 分块方法详解
- [[35-元数据/corpus-config/embedding-guide.md|Embedding Pipeline]] — 向量化完整流程
- [[35-元数据/metadata/schema.md|Wiki Schema]] — Frontmatter 规范
- [[35-元数据/metadata/taxonomy.md|Tag Taxonomy]] — 标签体系
