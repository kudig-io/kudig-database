---
title: 向量数据库与 RAG 基础设施
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- postgresql
- statefulset
- job
- cronjob
- llm
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 向量数据库与 RAG 基础设施 是什么
- 如何 向量数据库与 RAG 基础设施
trigger_keywords:
- 向量数据库与
- RAG
- 基础设施
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 向量数据库与 RAG 基础设施

## 概述

**RAG（Retrieval-Augmented Generation，检索增强生成）** 是 2025–2026 年企业级 LLM 应用的核心架构模式。RAG 通过将用户查询与私有知识库中的相关文档片段进行语义匹配，再将检索结果注入 LLM Prompt，从而显著提升回答的准确性、时效性和可溯源性。支撑 RAG 的底层基础设施是**向量数据库（Vector Database）** 和 **Embedding Pipeline**。

## 核心概念/原理

### 1. Embedding 与向量空间

Embedding 模型将文本、图像、代码等非结构化数据转换为高维数值向量。语义相似的内容在向量空间中距离更近。常用的 Embedding 模型包括：
- **OpenAI text-embedding-3**
- **Sentence-BERT / HuggingFace Embedding Models**
- **Multilingual Embedding**（如 BGE、E5 系列）

### 2. 向量数据库核心能力

向量数据库专门优化了高维向量的**近似最近邻搜索（ANN, Approximate Nearest Neighbor）**，核心特性包括：
- **低延迟检索**：目标 < 100ms 的向量查询响应
- **混合查询**：支持向量相似度 + 标量过滤（metadata filtering）的组合查询
- **分布式扩展**：水平扩展以支持十亿级向量
- **实时更新**：支持增量写入和索引更新

主流向量数据库：
| 产品 | 特点 | 部署模式 |
|------|------|----------|
| **Pinecone** | 全托管、易用 | SaaS |
| **Weaviate** | 模块化、GraphQL 接口 | 自托管 / SaaS |
| **Milvus/Zilliz** | 云原生、大规模 | 自托管 / SaaS |
| **Qdrant** | Rust 实现、高性能 | 自托管 / SaaS |
| **Chroma** | 轻量、开发友好 | 本地 / 轻量部署 |
| **pgvector** | PostgreSQL 扩展 | 与现有数据库共用 |

### 3. RAG 架构组件

典型的 RAG Pipeline 包含以下阶段：
1. **数据摄取（Ingestion）**：从文档、数据库、API 中提取原始数据
2. **分块（Chunking）**：将长文档切分为适合模型上下文的片段
3. **向量化（Embedding）**：通过 Embedding 服务将文本块转为向量
4. **索引与存储（Indexing）**：将向量 + 元数据写入向量数据库
5. **检索（Retrieval）**：根据用户查询检索最相关的 Top-K 文档块
6. **重排序（Reranking）**：使用更精确的 Cross-Encoder 模型对候选结果重排
7. **生成（Generation）**：将检索结果注入 Prompt，由 LLM 生成最终回答

## 关键机制或特性

### [[Kubernetes|Kubernetes]] 上的 RAG 基础设施部署

```yaml
# 向量数据库部署示例（Qdrant StatefulSet）
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: qdrant
spec:
  serviceName: qdrant
  replicas: 3
  template:
    spec:
      containers:
      - name: qdrant
        image: qdrant/qdrant:latest
        resources:
          requests:
            memory: "8Gi"
            cpu: "2"
        volumeMounts:
        - name: data
          mountPath: /qdrant/storage
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
```

### 分块策略

| 策略 | 优点 | 缺点 |
|------|------|------|
| 固定长度分块 | 简单、均匀 | 可能切断语义边界 |
| 递归字符分块 | 保留段落结构 | 块大小不均匀 |
| 语义分块 | 以语义边界切分 | 计算开销更大 |

### 索引算法

- **HNSW（Hierarchical Navigable Small World）**：最常用的 ANN 索引，平衡查询速度与召回率
- **IVF（Inverted File Index）**：适合十亿级向量，查询速度稍慢但内存占用更低
- **FLAT**：暴力搜索，召回率 100%，仅适合小规模数据

## 使用场景

1. **企业知识库问答**：基于内部文档、产品手册、技术规范构建智能客服
2. **代码辅助生成**：将代码仓库向量化为 Embedding，为 Copilot 类工具提供上下文
3. **个性化推荐系统**：利用用户行为和内容向量的相似度进行精准推荐
4. **多模态检索**：结合文本、图像 Embedding 构建跨模态搜索平台

## 最佳实践/注意事项

- **检索延迟目标**：同步 RAG 流程中，向量检索应控制在 100ms 以内，避免拖累整体用户体验
- **定期刷新索引**：业务文档会不断更新，应通过 Kubernetes [[CronJob|CronJob]] 或 Airflow 编排定期重索引任务
- **混合检索优于纯向量检索**：结合关键词搜索（BM25）和向量相似度搜索可显著提升召回率
- **重排序提升精度**：先用 ANN 快速召回 Top-50，再用更精确的 Reranker 筛选 Top-5 给 LLM
- **元数据过滤隔离租户**：在多租户场景中，通过 Namespace 隔离向量数据库实例，或在查询中加入 tenant_id 标量过滤
- **监控检索质量**：定期评估检索的命中率、召回率，以及生成结果的幻觉率（Hallucination Rate）

## 参考链接

- [Pinecone Documentation](https://docs.pinecone.io/)
- [Weaviate Documentation](https://weaviate.io/developers/weaviate)
- [Qdrant Documentation](https://qdrant.tech/documentation/)
- [Milvus Documentation](https://milvus.io/docs)
- [LangChain RAG Concepts](https://python.langchain.com/docs/concepts/rag/)

## Related

- [[17-系统基础/06-知识字典/workloads/pod.md|Pod]]
- [[17-系统基础/06-知识字典/fundamentals/container.md|Container]]
- [[17-系统基础/06-知识字典/fundamentals/node.md|Node]]
- [[17-系统基础/06-知识字典/fundamentals/namespace.md|Namespace]]
- [[17-系统基础/06-知识字典/fundamentals/cluster.md|Cluster]]


<!-- risk-assessed -->
