---
title: Embedding Pipeline 使用指南
summary: Embedding Pipeline 使用指南：python3 scripts/embedding-pipeline.py \
category: corpus-config
tags:
- rag
- embedding
- vector-search
- pipeline
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
---



# Embedding Pipeline 使用指南

> KUDIG-DATABASE 向量化 Pipeline：从 Markdown 语料到可检索向量索引的完整流程。

---

## 快速开始

### 1. 评估语料质量

```bash
python3 scripts/embedding-pipeline.py \
  --profile _meta/corpus-config/profiles/rag-sre-profile.yaml \
  --evaluate
```

输出示例：
```json
{
  "profile": "sre-ops-agent",
  "file_count": 206,
  "total_bytes": 7786786,
  "avg_file_bytes": 37799.9,
  "frontmatter": {
    "has_title": "200/206 (97.1%)",
    "has_tags": "183/206 (88.8%)",
    "has_category": "206/206 (100.0%)"
  }
}
```

### 2. 构建向量索引

```bash
# SRE Agent 语料（206 文件，~2s）
python3 scripts/embedding-pipeline.py \
  --profile _meta/corpus-config/profiles/rag-sre-profile.yaml

# 全库语料（2639 文件，~30s）
python3 scripts/embedding-pipeline.py \
  --profile _meta/corpus-config/profiles/rag-full-profile.yaml
```

### 3. 增量更新

```bash
python3 scripts/embedding-pipeline.py \
  --profile _meta/corpus-config/profiles/rag-sre-profile.yaml \
  --incremental
```

### 4. 语义搜索测试

```bash
python3 scripts/embedding-pipeline.py \
  --profile _meta/corpus-config/profiles/rag-sre-profile.yaml \
  --search "node notready kubelet certificate expired" \
  --top-k 5
```

---

## 架构

```
┌─────────────────┐     ┌─────────────┐     ┌─────────────┐     ┌──────────────┐
│  YAML Profile   │────▶│ File Walker │────▶│   Chunker   │────▶│  Embedding   │
│  (include/      │     │  (glob +    │     │ (by_h2/     │     │  Provider    │
│   exclude)      │     │  exclude)   │     │  by_h3/...) │     │  (mock/      │
└─────────────────┘     └─────────────┘     └─────────────┘     │  local/      │
                                                                │  openai)     │
                                                                └──────┬───────┘
                                                                       │
                                                                ┌──────▼───────┐
                                                                │  Vector Cache│
                                                                │  (.vector-   │
                                                                │   cache/)    │
                                                                └──────────────┘
```

---

## Profile 配置

```yaml
name: sre-ops-agent
description: "面向 SRE/运维的故障诊断 Agent 语料配置"

# 核心语料（必须导入）
core:
  - path: domain-10-troubleshooting-diagnostics/topic-fta/list/
    priority: critical
    chunking: by_h3

# 方法论语料（推荐导入）
methodology:
  - path: domain-10-troubleshooting-diagnostics/topic-febm/
    priority: high
    chunking: by_h2

# 参考语料（可选导入）
reference:
  - path: domain-17-system-foundation/topic-cheat-sheet/k8s.md
    priority: medium
    chunking: full_doc

# 排除规则
exclude:
  - "*.pdf"
  - "CHANGELOG.md"
```

### Chunking 策略

| 策略 | 说明 | 适用场景 |
|:---|:---|:---|
| `by_h2` | 按 `##` 二级标题分块 | domain-* 深度文档（默认） |
| `by_h3` | 按 `###` 三级标题分块 | FTA 故障树（推理骨架） |
| `by_section` | 智能混合（`##` 为主，过大块拆 `###`） | Skill 文档 |
| `full_doc` | 整文档作为一个 chunk | Cheat Sheet 速查卡 |

---

## Embedding Provider 配置

| Provider | 环境变量 | 向量维度 | 速度 | 质量 | 说明 |
|:---|:---|:---:|:---:|:---:|:---|
| `mock` (默认) | `EMBEDDING_PROVIDER=mock` | 384 | ⚡ 极快 | ⭐ 占位 | 确定性伪向量，用于 Pipeline 验证 |
| `local` | `EMBEDDING_PROVIDER=local` | 384/768 | 🐢 中等 | ⭐⭐⭐⭐ | sentence-transformers 本地模型 |
| `openai` | `EMBEDDING_PROVIDER=openai` | 1536 | 🌐 API | ⭐⭐⭐⭐⭐ | OpenAI text-embedding-3-small |

### 本地模型部署

```bash
pip install sentence-transformers
export EMBEDDING_PROVIDER=local
export LOCAL_MODEL_NAME=all-MiniLM-L6-v2
python3 scripts/embedding-pipeline.py --profile _meta/corpus-config/profiles/rag-full-profile.yaml
```

### OpenAI API

```bash
export EMBEDDING_PROVIDER=openai
export OPENAI_API_KEY=sk-xxx
python3 scripts/embedding-pipeline.py --profile _meta/corpus-config/profiles/rag-full-profile.yaml
```

---

## 输出目录结构

```
_meta/corpus-config/profiles/.vector-cache/<profile-name>/
├── chunks.jsonl        # 所有 chunk（文本 + 元数据）
├── embeddings.npy      # 向量矩阵（float32, N×D）
├── manifest.json       # 文件 hash 映射（增量更新用）
└── index.faiss         # FAISS 索引（需安装 faiss-cpu）
```

### chunks.jsonl 格式

```json
{
  "chunk_id": "a1b2c3d4e5f67890",
  "source_path": "domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-control-plane/SKILL.md",
  "text": "## 验证修复\n\n```bash\n./scripts/verify-control-plane.sh <namespace>\n```...",
  "metadata": {
    "source": "domain-10-troubleshooting-diagnostics/...",
    "domain": "domain-10-troubleshooting-diagnostics",
    "filename": "SKILL.md",
    "section_title": "## 验证修复",
    "chunk_index": 5,
    "total_chunks": 12,
    "title": "控制平面故障诊断与修复",
    "category": "skill"
  },
  "embedding": [0.023, -0.156, 0.089, ...]
}
```

---

## 性能基准

| 语料规模 | 文件数 | Chunk 数 | Mock | Local (CPU) | OpenAI API |
|:---|:---:|:---:|:---:|:---:|:---:|
| SRE Agent | 206 | 1,856 | ~2s | ~30s | ~20s |
| Full Corpus | 2,639 | ~25,000 | ~25s | ~8min | ~5min |

> Mock 模式仅用于 Pipeline 验证，生产环境请使用 Local 或 OpenAI Provider。

---

## 集成 RAG Agent

```python
import json
import numpy as np

# 加载 chunks
chunks = []
with open("_meta/corpus-config/profiles/.vector-cache/sre-ops-agent/chunks.jsonl") as f:
    for line in f:
        chunks.append(json.loads(line))

# 加载 embeddings
embeddings = np.load("_meta/corpus-config/profiles/.vector-cache/sre-ops-agent/embeddings.npy")

# 搜索（余弦相似度）
def cosine_search(query_vec, embeddings, top_k=5):
    scores = embeddings @ query_vec  # 已归一化
    top_indices = np.argsort(scores)[::-1][:top_k]
    return [(chunks[i], float(scores[i])) for i in top_indices]
```

---

## Related

- _meta/corpus-config/rag-chunking-strategy.md
- _meta/corpus-config/README.md
- topic-index 向量索引
