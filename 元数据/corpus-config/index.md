---
title: Corpus Config 索引
description: RAG 语料配置目录索引 — 分块策略、Embedding 指南、Profile 配置
summary: RAG 语料配置目录，含分块策略指南、Embedding Pipeline 使用指南、场景化 Profile 配置
category: index
tags:
- index
- rag
- corpus-config
tier: supporting
created: '2026-07-02'
last_updated: '2026-07-21'
---

# Corpus Config

> RAG 语料配置目录 — 定义如何将 KUDIG 知识库转化为高质量向量化语料。

## 核心文档

| 文件 | 职责 |
|------|------|
| [[元数据/corpus-config/README.md\|README]] | 语料配置总览、场景推荐、质量保障 |
| [[元数据/corpus-config/rag-chunking-strategy.md\|RAG 分块策略]] | 按标题/固定大小/整文档分块方法 |
| [[元数据/corpus-config/embedding-guide.md\|Embedding Pipeline]] | 向量化完整流程、Provider 配置、性能基准 |

## 子目录

| 子目录 | 内容 |
|--------|------|
| `profiles/` | 场景化语料配置 (YAML Profile) |
| `scripts/` | 语料处理脚本 |
