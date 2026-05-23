---
title: RAG 分块策略指南
description: '# RAG 分块策略指南'
category: general
tags:
- k8s
- etcd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- RAG 分块策略指南 是什么
- 如何 RAG 分块策略指南
trigger_keywords:
- RAG
- 分块策略指南
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# RAG 分块策略指南

> 基于 KUDIG-DATABASE 文档结构的最佳 RAG 分块策略

---

## 1. 推荐分块策略

### 策略一：按 Markdown 标题分块（推荐）

```python
from langchain.text_splitter import MarkdownHeaderTextSplitter

# 按 H1/H2 分块，保持知识完整性
splitter = MarkdownHeaderTextSplitter(
    headers_to_split_on=[
        ('#', 'title'),
        ('##', 'section'),
    ]
)
```

**适用**：domain-* 深度文档（每篇 10-60KB）

### 策略二：按固定大小 + 重叠分块

```python
from langchain.text_splitter import RecursiveCharacterTextSplitter

splitter = RecursiveCharacterTextSplitter(
    chunk_size=2000,       # 中文约 700 字
    chunk_overlap=200,     # 10% 重叠
    separators=['\n## ', '\n### ', '\n\n', '\n', ' ']
)
```

**适用**：topic-fta 大型文档（100KB+）

### 策略三：整文档作为单 Chunk

**适用**：topic-cheat-sheet 速查卡（10-50KB，内容紧凑）

---

## 2. 元数据增强

分块时建议携带以下元数据：

```python
metadata = {
    'source': 'domain-01-cluster-fundamentals/11-etcd-deep-dive.md',
    'domain': 'control-plane',
    'section': '## 3. Raft 共识协议',
    'difficulty': 'advanced',
    'tags': ['etcd', 'raft', 'consensus'],
    'k8s_versions': ['v1.25', 'v1.32'],
}
```

---

## 3. 不同目录的推荐策略

| 目录 | 分块策略 | chunk_size | 说明 |
|:---|:---|:---:|:---|
| domain-* | 按 H2 标题分块 | ~2000 | 每个章节独立 chunk |
| domain-10-troubleshooting-diagnostics/topic-fta/list/ | 按 H3 标题分块 | ~1500 | 每个底事件独立 chunk |
| domain-10-troubleshooting-diagnostics/topic-skills/ | 按 Section 分块 | ~3000 | 每个 Section 独立 chunk |
| domain-17-system-foundation/topic-cheat-sheet/ | 整文档 | 全文 | 速查卡保持完整 |
| domain-17-system-foundation/topic-dictionary/ | 按条目分块 | ~500 | 每个术语独立 chunk |
| domain-32-yaml/ | 按资源类型分块 | ~2000 | 每种 YAML 独立 |

---

## 4. Embedding 模型推荐

| 模型 | 维度 | 中文支持 | 推荐场景 |
|:---|:---:|:---:|:---|
| text-embedding-3-large | 3072 | 好 | 通用场景 |
| text-embedding-3-small | 1536 | 好 | 成本敏感 |
| bge-large-zh-v1.5 | 1024 | 优秀 | 中文优先 |
| bge-m3 | 1024 | 优秀 | 多语言混合 |

---

## 5. 完整示例

```python
from langchain.document_loaders import DirectoryLoader
from langchain.text_splitter import MarkdownHeaderTextSplitter
from langchain.embeddings import OpenAIEmbeddings
from langchain.vectorstores import Chroma

# 1. 加载文档
loader = DirectoryLoader(
    './domain-10-troubleshooting-diagnostics/',
    glob='**/*.md',
    show_progress=True
)
docs = loader.load()

# 2. 分块
splitter = MarkdownHeaderTextSplitter(
    headers_to_split_on=[('#', 'title'), ('##', 'section')]
)
chunks = []
for doc in docs:
    chunks.extend(splitter.split_text(doc.page_content))

# 3. Embedding + 向量化
embeddings = OpenAIEmbeddings(model='text-embedding-3-large')
vectorstore = Chroma.from_documents(chunks, embeddings)

# 4. 检索
results = vectorstore.similarity_search('[[concepts/pod-lifecycle|pod]] CrashLoopBackOff 怎么排查', k=5)
```
