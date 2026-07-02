---
title: AI 语料库配置：RAG 分块策略、场景化 Profile 与向量库构建
description: '# AI 语料库配置'
summary: '# AI 语料库配置'
category: reference
tags:
- k8s
- rag
- chunking
- vector-database
- profile
- corpus
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI 语料库配置：RAG 分块策略、场景化 Profile 与向量库构建 是什么
- 如何 AI 语料库配置：RAG 分块策略、场景化 Profile 与向量库构建
trigger_keywords:
- AI
- 语料库配置：RAG
- 分块策略
- 场景化
- Profile
- 与向量库构建
prerequisites:
- kubectl-basics
---



# AI 语料库配置

## RAG 分块策略

三种分块方式：

1. **按 Markdown 标题分块**（推荐）：保持知识完整性
2. **语义分块**：基于 Embedding 相似度切分
3. **混合分块**：标题分块 + 语义二次细分

最佳实践：
- 块大小：200-800 tokens
- 重叠：50-100 tokens
- 保留元数据（标题层级、来源文档）

## 场景化 Profile

不同使用场景需要不同的分块和检索策略：

| 场景 | 分块粒度 | 检索策略 | Top-K |
|------|----------|----------|-------|
| 故障排查 | 细粒度 | 关键词 + 语义 | 3-5 |
| 学习参考 | 中粒度 | 纯语义 | 5-10 |
| 代码生成 | 代码块级 | 关键词为主 | 3 |

## 向量库构建流程

```
源文档 → 清洗 → 分块 → Embedding → 入库 → 索引优化 → 测试
```

---

> 来源：.zread/wiki/drafts/19-ai-yu-liao-ku-pei-zhi-*.md

## Related

- [[entities/kudig-rag-chunking-strategy.md|kudig-rag-chunking-strategy]] — RAG 分块策略指南与 Manpage 安装指南
- [[entities/k8s-ai-agent-engineering.md|k8s-ai-agent-engineering]] — AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署
