---
title: AI 语料库配置 (Corpus Config)
description: '# AI 语料库配置 (Corpus Config)'
category: general
tags:
- k8s
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI 语料库配置 (Corpus Config) 是什么
- 如何 AI 语料库配置 (Corpus Config)
trigger_keywords:
- AI
- 语料库配置
- Corpus
- Config
---

# AI 语料库配置 (Corpus Config)

> 面向 NotebookLM / IMA / RAG 等 AI 场景的语料配置和最佳实践

---

## 目录结构

```
corpus-config/
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
| SRE Agent | topic-fta + topic-skills + domain-12 | [rag-sre-profile.yaml](./profiles/rag-sre-profile.yaml) |
| K8s 学习助手 | topic-learn + topic-cheat-sheet + domain-1~6 | [rag-learning-profile.yaml](./profiles/rag-learning-profile.yaml) |
| 全知识库 | 全部目录 | [rag-full-profile.yaml](./profiles/rag-full-profile.yaml) |
