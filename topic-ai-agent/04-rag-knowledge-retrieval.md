# RAG 检索增强生成深度指南

> **文档类型**: 核心技术专题 | **最后更新**: 2026-03 | **关键词**: RAG, 检索增强生成, Embedding, 向量数据库, 混合检索, Re-ranking, 分块策略, Chunking, Weaviate, Milvus, pgvector

---

## 概述

RAG（Retrieval-Augmented Generation）是将外部知识库与 LLM 生成能力结合的核心技术，是解决 LLM 知识截止日期、领域知识缺失和幻觉问题的标准方案。本文覆盖从数据准备、分块策略、Embedding 选型、向量库对比，到混合检索、Re-ranking、Advanced RAG 和生产优化的全链路工程实践。

---

## 1. RAG 架构全景

### 1.1 基础 RAG 流程

```
                    ┌─────────────────────────────────────────┐
                    │              离线阶段（索引构建）            │
                    │                                          │
                    │  文档 → 预处理 → 分块 → Embedding → 向量库  │
                    └─────────────────────────────────────────┘
                                         │ 索引
                                         ▼
用户查询 → Query Embedding → [向量相似检索] → Top-K 候选文档
                │                              │
                │                              ▼ Re-ranking
                │                    精排后的相关文档
                │                              │
                └──────────────────────────────┘
                                 │
                                 ▼
                         LLM 生成回答（携带检索上下文）
```

### 1.2 RAG 演进路径

```
Naive RAG（朴素 RAG）
  ↓ 解决检索精度问题
Advanced RAG
  - Pre-retrieval: 查询改写、HyDE 假设文档扩展
  - Retrieval: 混合检索、多路召回
  - Post-retrieval: Re-ranking、上下文压缩
  ↓ 解决复杂推理问题
Modular RAG（模块化 RAG）
  - 路由（根据查询类型选择不同检索策略）
  - 迭代检索（Iterative RAG）
  - 递归检索（Recursive RAG）
  ↓ 结合 Agent 能力
Agentic RAG
  - Agent 自主决定是否检索、检索什么
  - 多轮检索、自我反思
  - 工具调用 + RAG 混合
```

---

## 2. 数据准备与分块策略

### 2.1 文档预处理

```python
import re
from pathlib import Path
from typing import Optional

class DocumentPreprocessor:
    """生产级文档预处理器"""
    
    def preprocess_markdown(self, content: str, source_path: str) -> dict:
        """处理 Markdown 文档（针对 kudig-database 优化）"""
        
        # 1. 提取元数据
        metadata = self._extract_metadata(content, source_path)
        
        # 2. 清理内容
        cleaned = self._clean_markdown(content)
        
        # 3. 保留代码块标记（重要！代码块不应在中间分割）
        code_blocks = self._extract_code_blocks(cleaned)
        
        return {
            "content": cleaned,
            "metadata": metadata,
            "code_blocks": code_blocks,
        }
    
    def _extract_metadata(self, content: str, source_path: str) -> dict:
        """提取文档元数据"""
        path = Path(source_path)
        
        # 提取 Markdown frontmatter 中的关键词等
        keywords = []
        keyword_match = re.search(r'\*\*关键词\*\*[：:]\s*(.+)', content)
        if keyword_match:
            keywords = [k.strip() for k in keyword_match.group(1).split(',')]
        
        return {
            "source": source_path,
            "domain": path.parent.name,
            "filename": path.stem,
            "keywords": keywords,
            "doc_type": self._infer_doc_type(content),
        }
    
    def _clean_markdown(self, content: str) -> str:
        """清理 Markdown 格式"""
        # 移除 HTML 注释
        content = re.sub(r'<!--.*?-->', '', content, flags=re.DOTALL)
        # 规范化空行
        content = re.sub(r'\n{3,}', '\n\n', content)
        return content.strip()
```

### 2.2 分块策略详解

分块（Chunking）是 RAG 质量的关键决策，不同策略各有权衡：

#### 固定大小分块

```python
from langchain.text_splitter import RecursiveCharacterTextSplitter

# 通用配置
splitter = RecursiveCharacterTextSplitter(
    chunk_size=1000,        # 每块约 1000 字符
    chunk_overlap=200,      # 200 字符重叠，保证上下文连续性
    separators=[
        "\n## ",   # 优先按二级标题分割
        "\n### ",  # 其次按三级标题
        "\n\n",    # 再按段落
        "\n",      # 再按行
        " ",       # 最后按空格
        ""
    ],
    length_function=len,
)
```

#### 语义分块（推荐用于技术文档）

```python
from llama_index.core.node_parser import SemanticSplitterNodeParser
from llama_index.embeddings.openai import OpenAIEmbedding

# 语义分块：基于语义相似度决定分割点
semantic_splitter = SemanticSplitterNodeParser(
    buffer_size=1,                   # 比较相邻句子的语义相似度
    breakpoint_percentile_threshold=95,  # 相似度低于 95th 百分位时分割
    embed_model=OpenAIEmbedding(model="text-embedding-3-small"),
)

# 对 kudig-database 技术文档的效果优于固定大小分块
nodes = semantic_splitter.get_nodes_from_documents(documents)
```

#### 层次化分块（Parent-Child Chunking）

最适合技术文档的策略：**父块保留上下文，子块保证精确检索**：

```python
from langchain.retrievers import ParentDocumentRetriever
from langchain.storage import InMemoryStore
from langchain.text_splitter import RecursiveCharacterTextSplitter
from langchain_community.vectorstores import Chroma

# 大块（父）：用于 LLM 上下文，保留完整语义
parent_splitter = RecursiveCharacterTextSplitter(chunk_size=2000, chunk_overlap=200)

# 小块（子）：用于向量检索，精确匹配
child_splitter = RecursiveCharacterTextSplitter(chunk_size=400, chunk_overlap=50)

vectorstore = Chroma(embedding_function=embedding_model)
store = InMemoryStore()  # 生产用 Redis 或数据库

retriever = ParentDocumentRetriever(
    vectorstore=vectorstore,
    docstore=store,
    child_splitter=child_splitter,
    parent_splitter=parent_splitter,
)

# 检索时：子块命中 → 返回对应父块（包含完整上下文）
retriever.add_documents(documents)
relevant_docs = retriever.get_relevant_documents("Pod Pending 的原因")
```

### 2.3 分块策略选型指南

| 场景 | 推荐策略 | chunk_size | overlap |
|------|---------|-----------|---------|
| 通用文档 | 递归字符分块 | 800-1200 | 150-200 |
| 技术文档（如 kudig-database）| 父子分块 | 父:2000 / 子:400 | 200/50 |
| 代码文件 | 基于 AST 的代码分块 | 按函数/类 | 无 |
| 表格/结构化数据 | 按行分块 + 保留表头 | 按行数 | 保留表头 |
| 对话记录/日志 | 时间窗口分块 | 按时间段 | 1-2 条 |
| 长文档（>100页）| 语义分块 | 动态 | - |

---

## 3. Embedding 模型选型

### 3.1 主流 Embedding 模型对比

| 模型 | 维度 | 最大 Token | MTEB 得分 | 中文能力 | 成本 | 特点 |
|------|------|-----------|---------|---------|------|------|
| **text-embedding-3-large** | 3072 | 8191 | 64.6 | ★★★★☆ | $0.13/1M | OpenAI 最强 |
| **text-embedding-3-small** | 1536 | 8191 | 62.3 | ★★★★☆ | $0.02/1M | 性价比最高 |
| **BGE-M3** | 1024 | 8192 | 54.9 | ★★★★★ | 开源免费 | 多语言最强 |
| **Jina Embeddings v3** | 1024 | 8192 | 65.0 | ★★★★☆ | $0.02/1M | 最新最强 |
| **BGE-large-zh-v1.5** | 1024 | 512 | - | ★★★★★ | 开源免费 | 中文专项 |
| **m3e-large** | 768 | 512 | - | ★★★★☆ | 开源免费 | 国产中文 |

### 3.2 Embedding 维度与精度权衡

```python
# text-embedding-3 支持可变维度（缩减维度降低成本）
from openai import OpenAI

client = OpenAI()

# 高精度（归档/离线场景）
embedding_full = client.embeddings.create(
    model="text-embedding-3-large",
    input="Pod Pending 资源不足",
    dimensions=3072,  # 最高维度
).data[0].embedding

# 平衡（在线检索推荐）
embedding_balanced = client.embeddings.create(
    model="text-embedding-3-large",
    input="Pod Pending 资源不足",
    dimensions=1024,  # 降维 67%，存储和查询更快，精度损失约 3%
).data[0].embedding

# 轻量（高频简单场景）
embedding_lite = client.embeddings.create(
    model="text-embedding-3-small",
    input="Pod Pending 资源不足",
    dimensions=512,
).data[0].embedding
```

---

## 4. 向量数据库选型

### 4.1 主流向量库对比

| 特性 | Chroma | Weaviate | Qdrant | Milvus | pgvector |
|------|-------|---------|-------|-------|---------|
| **定位** | 开发/原型 | 生产级 | 生产级 | 超大规模 | PostgreSQL 扩展 |
| **部署方式** | 嵌入式/服务 | 分布式服务 | 服务/云 | 分布式集群 | PostgreSQL 插件 |
| **向量规模** | <1M | 1M-100M | 1M-100M | >1B | <10M |
| **混合搜索** | ❌ | ✅ 原生 | ✅ 原生 | ✅ | ✅(需手动) |
| **元数据过滤** | ✅ 基础 | ✅ GraphQL | ✅ 强大 | ✅ | ✅ SQL |
| **多租户** | ❌ | ✅ | ✅ | ✅ | 基于 Schema |
| **水平扩展** | ❌ | ✅ | ✅ | ✅ | 受限 |
| **K8s 部署** | 简单 | Helm 完整 | Helm 完整 | Operator | 直接使用 |
| **Managed 服务** | ✅ | ✅ WCS | ✅ Qdrant Cloud | ✅ Zilliz | ✅ Supabase |

### 4.2 Qdrant 生产部署（推荐）

Qdrant 在性能、功能和易用性上综合最优：

```yaml
# Qdrant K8s 部署
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: qdrant
  namespace: ai-infra
spec:
  serviceName: qdrant
  replicas: 3  # 生产建议 3 副本
  selector:
    matchLabels:
      app: qdrant
  template:
    spec:
      containers:
      - name: qdrant
        image: qdrant/qdrant:v1.9.0
        ports:
        - containerPort: 6333  # HTTP
        - containerPort: 6334  # gRPC
        env:
        - name: QDRANT__SERVICE__API_KEY
          valueFrom:
            secretKeyRef:
              name: qdrant-secret
              key: api-key
        - name: QDRANT__CLUSTER__ENABLED
          value: "true"
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"
        volumeMounts:
        - name: qdrant-storage
          mountPath: /qdrant/storage
  volumeClaimTemplates:
  - metadata:
      name: qdrant-storage
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: "fast-ssd"
      resources:
        requests:
          storage: 100Gi
```

```python
from qdrant_client import QdrantClient
from qdrant_client.models import (
    Distance, VectorParams, PointStruct,
    Filter, FieldCondition, MatchValue, SearchParams
)

client = QdrantClient(
    url="http://qdrant.ai-infra.svc:6333",
    api_key="your-api-key",
)

# 创建集合（K8s 知识库）
client.create_collection(
    collection_name="kudig_knowledge",
    vectors_config=VectorParams(
        size=1536,           # text-embedding-3-small 维度
        distance=Distance.COSINE,
        on_disk=True,        # 大规模数据存储到磁盘
    ),
    # 启用 Payload 索引（加速元数据过滤）
    on_disk_payload=True,
)

# 创建元数据索引
client.create_payload_index(
    collection_name="kudig_knowledge",
    field_name="domain",
    field_schema="keyword",
)

client.create_payload_index(
    collection_name="kudig_knowledge",
    field_name="doc_type",
    field_schema="keyword",
)

# 带元数据过滤的语义检索
results = client.search(
    collection_name="kudig_knowledge",
    query_vector=query_embedding,
    query_filter=Filter(
        must=[
            FieldCondition(
                key="domain",
                match=MatchValue(value="domain-12-troubleshooting")
            )
        ]
    ),
    limit=10,
    search_params=SearchParams(hnsw_ef=128, exact=False),
)
```

---

## 5. 混合检索（Hybrid Search）

单纯向量检索对精确匹配（如专有名词、错误代码）效果差，混合检索结合稠密向量和稀疏 BM25 检索：

### 5.1 BM25 + 向量检索融合

```python
from langchain_community.retrievers import BM25Retriever
from langchain.retrievers import EnsembleRetriever
from langchain_community.vectorstores import Qdrant as QdrantVectorStore

# 1. 向量检索器（语义相关性）
vector_retriever = QdrantVectorStore(
    client=qdrant_client,
    collection_name="kudig_knowledge",
    embedding=embedding_model,
).as_retriever(search_kwargs={"k": 10})

# 2. BM25 检索器（关键词精确匹配）
bm25_retriever = BM25Retriever.from_documents(
    documents,
    k=10,
)

# 3. 融合检索（Reciprocal Rank Fusion）
ensemble_retriever = EnsembleRetriever(
    retrievers=[bm25_retriever, vector_retriever],
    weights=[0.4, 0.6],  # BM25:向量 = 4:6（可调）
)

# 使用混合检索
docs = ensemble_retriever.get_relevant_documents(
    "CrashLoopBackOff OOMKilled 内存不足"
)
```

### 5.2 Qdrant 原生混合检索

```python
from qdrant_client.models import SparseVector, NamedSparseVector, NamedVector, Query

# 使用 FastEmbed 生成稀疏向量
from fastembed import SparseTextEmbedding

sparse_model = SparseTextEmbedding(model_name="Qdrant/bm25")
sparse_embeddings = list(sparse_model.embed(["Pod Pending 资源不足"]))

# 混合检索请求
results = client.query_points(
    collection_name="kudig_knowledge",
    prefetch=[
        # 向量检索分支
        models.Prefetch(
            query=dense_vector,
            using="dense",
            limit=20,
        ),
        # 稀疏（BM25）检索分支
        models.Prefetch(
            query=models.SparseVector(
                indices=sparse_embeddings[0].indices.tolist(),
                values=sparse_embeddings[0].values.tolist(),
            ),
            using="sparse",
            limit=20,
        ),
    ],
    # RRF 融合两路结果
    query=models.FusionQuery(fusion=models.Fusion.RRF),
    limit=10,
)
```

---

## 6. Re-ranking（重排序）

Re-ranking 是 RAG 管道中提升检索精度最有效的手段，将初筛的 Top-50 结果重新排序取 Top-5：

```python
from sentence_transformers import CrossEncoder
import torch

class Reranker:
    def __init__(self, model_name: str = "BAAI/bge-reranker-v2-m3"):
        # Cross-encoder 模型：直接对 (query, doc) 对打分，精度远高于 Bi-encoder
        self.model = CrossEncoder(
            model_name,
            device="cuda" if torch.cuda.is_available() else "cpu",
        )
    
    def rerank(
        self, 
        query: str, 
        documents: list[str], 
        top_k: int = 5
    ) -> list[tuple[str, float]]:
        """重新排序文档"""
        # 构建 (query, doc) 对
        pairs = [[query, doc] for doc in documents]
        
        # Cross-encoder 打分
        scores = self.model.predict(pairs)
        
        # 按分数排序
        scored_docs = sorted(
            zip(documents, scores),
            key=lambda x: x[1],
            reverse=True
        )
        
        return scored_docs[:top_k]

# 在 RAG 管道中使用
reranker = Reranker("BAAI/bge-reranker-v2-m3")

# 1. 宽召回（Top-30）
candidates = retriever.get_relevant_documents(query, k=30)

# 2. 精排（Top-5）
reranked = reranker.rerank(
    query=query,
    documents=[doc.page_content for doc in candidates],
    top_k=5
)

# 3. 用精排结果生成答案
context = "\n\n".join([doc for doc, score in reranked])
```

---

## 7. Advanced RAG 技术

### 7.1 查询改写（Query Rewriting）

```python
# HyDE（Hypothetical Document Embeddings）：生成假设性答案再检索
def hyde_retrieval(query: str, retriever) -> list:
    """HyDE：先生成假设性文档，再用其向量检索真实文档"""
    
    # Step 1: 让 LLM 生成一个假设性答案
    hyde_prompt = f"""请生成一段详细的技术文档段落来回答以下 K8s 问题：
    问题: {query}
    
    生成一个专业的、技术准确的段落，即使你不确定答案也要生成。"""
    
    hypothetical_doc = llm.invoke(hyde_prompt).content
    
    # Step 2: 用假设文档的向量去检索真实文档（效果远好于直接用问题检索）
    results = retriever.get_relevant_documents(hypothetical_doc)
    return results

# Multi-query Retrieval：从多个角度改写查询
from langchain.retrievers.multi_query import MultiQueryRetriever

multi_query_retriever = MultiQueryRetriever.from_llm(
    retriever=base_retriever,
    llm=llm,
    prompt=PromptTemplate.from_template("""
    你是一个 K8s 运维专家。
    原始问题: {question}
    
    请从 3 个不同角度改写这个问题，以检索更全面的信息：
    1. 从症状角度
    2. 从根因角度  
    3. 从解决方案角度
    
    输出 3 个改写后的查询，每行一个。
    """)
)
```

### 7.2 上下文压缩（Context Compression）

```python
from langchain.retrievers import ContextualCompressionRetriever
from langchain.retrievers.document_compressors import LLMChainExtractor

# 从检索到的文档中提取与问题直接相关的片段
compressor = LLMChainExtractor.from_llm(
    llm=ChatOpenAI(model="gpt-4o-mini"),  # 用便宜的模型做压缩
)

compression_retriever = ContextualCompressionRetriever(
    base_compressor=compressor,
    base_retriever=base_retriever,
)

# 只返回文档中与查询直接相关的句子/段落
compressed_docs = compression_retriever.get_relevant_documents(
    "Pod Pending 的诊断步骤"
)
```

### 7.3 迭代 RAG（Iterative/Recursive RAG）

```python
class IterativeRAG:
    """多轮迭代检索：每次检索后判断是否需要更多信息"""
    
    def __init__(self, retriever, llm, max_iterations: int = 3):
        self.retriever = retriever
        self.llm = llm
        self.max_iterations = max_iterations
    
    def answer(self, question: str) -> dict:
        collected_context = []
        queries_used = []
        
        for i in range(self.max_iterations):
            # 检索
            search_query = self._generate_query(question, collected_context, i)
            queries_used.append(search_query)
            new_docs = self.retriever.get_relevant_documents(search_query)
            
            # 合并去重
            for doc in new_docs:
                if doc.page_content not in [c.page_content for c in collected_context]:
                    collected_context.append(doc)
            
            # 判断信息是否充足
            sufficiency_check = self.llm.invoke(f"""
            问题: {question}
            已收集的信息: {[d.page_content for d in collected_context]}
            
            以上信息是否足够回答问题？只回答 "YES" 或 "NO"，
            如果 NO，说明还缺少什么信息。
            """).content
            
            if sufficiency_check.startswith("YES"):
                break
        
        # 最终生成
        final_answer = self._generate_answer(question, collected_context)
        return {
            "answer": final_answer,
            "sources": collected_context,
            "queries_used": queries_used,
            "iterations": i + 1
        }
```

---

## 8. RAG 评估指标

### 8.1 RAGAS 评估框架

```python
from ragas import evaluate
from ragas.metrics import (
    faithfulness,           # 忠实度：答案是否基于检索内容
    answer_relevancy,       # 答案相关性：答案是否回答了问题
    context_precision,      # 上下文精确率：检索内容是否都有用
    context_recall,         # 上下文召回率：是否检索到了所有必要信息
    context_entity_recall,  # 实体召回率
    answer_correctness,     # 答案正确性
)
from datasets import Dataset

# 构建评估数据集
eval_data = {
    "question": [
        "Pod Pending 最常见的原因是什么？",
        "如何查看 K8s 节点的资源使用情况？",
    ],
    "answer": [
        "Pod Pending 最常见原因包括：资源不足、节点亲和性不匹配...",
        "使用 kubectl top nodes 命令...",
    ],
    "contexts": [
        ["当调度器无法找到合适的节点时，Pod 会处于 Pending 状态..."],
        ["kubectl top 命令需要 metrics-server 支持..."],
    ],
    "ground_truth": [
        "Pod Pending 的常见原因：1. 资源不足 2. 节点亲和性 3. Taint/Toleration...",
        "kubectl top nodes 显示节点 CPU/内存使用量，需要 metrics-server...",
    ],
}

dataset = Dataset.from_dict(eval_data)
result = evaluate(
    dataset=dataset,
    metrics=[faithfulness, answer_relevancy, context_precision, context_recall],
    llm=llm,
    embeddings=embedding_model,
)

print(result)
# Output:
# {'faithfulness': 0.92, 'answer_relevancy': 0.88, 
#  'context_precision': 0.85, 'context_recall': 0.79}
```

### 8.2 RAG 质量基准目标

| 指标 | 可接受 | 优秀 | 说明 |
|------|-------|------|------|
| **Faithfulness** | >0.80 | >0.95 | 答案必须基于检索内容，不能编造 |
| **Answer Relevancy** | >0.75 | >0.90 | 回答的确是问题问的内容 |
| **Context Precision** | >0.70 | >0.85 | 检索内容的信噪比要高 |
| **Context Recall** | >0.65 | >0.80 | 关键信息不能遗漏 |
| **检索延迟** | <500ms | <200ms | P95 延迟 |
| **端到端延迟** | <3s | <1.5s | 含检索+生成总时间 |

---

## 9. 生产 RAG Pipeline 完整实现

```python
from langchain.chains import RetrievalQAWithSourcesChain
from langchain.prompts import PromptTemplate

# 生产级 RAG Prompt（K8s 运维场景）
K8S_RAG_PROMPT = PromptTemplate.from_template("""
你是一个 Kubernetes 生产运维专家，基于以下知识库内容回答问题。

【知识库来源】
{summaries}

【问题】
{question}

【回答规范】
1. 只基于知识库中的信息回答，不要编造
2. 如果知识库信息不足，明确说明"知识库中没有相关信息"
3. 提供具体的 kubectl 命令或 YAML 示例
4. 指出操作风险（如有）
5. 在回答末尾标注参考来源

回答：
""")

class ProductionRAGPipeline:
    def __init__(
        self,
        vectorstore,
        llm,
        embedding_model,
        reranker: Optional[Reranker] = None,
        enable_hybrid_search: bool = True,
    ):
        self.vectorstore = vectorstore
        self.llm = llm
        self.reranker = reranker
        
        # 配置检索器
        base_retriever = vectorstore.as_retriever(
            search_type="mmr",  # Max Marginal Relevance，减少重复
            search_kwargs={
                "k": 20,
                "fetch_k": 50,   # 先取 50，MMR 筛选出 20
                "lambda_mult": 0.7,  # 相关性 vs 多样性权衡
            }
        )
        
        self.retriever = base_retriever
    
    def query(
        self,
        question: str,
        domain_filter: Optional[str] = None,
        top_k: int = 5,
    ) -> dict:
        """执行 RAG 查询"""
        
        # 1. 检索
        raw_docs = self.retriever.get_relevant_documents(question)
        
        # 2. 域过滤（可选）
        if domain_filter:
            raw_docs = [d for d in raw_docs 
                       if d.metadata.get("domain") == domain_filter]
        
        # 3. Re-ranking（可选）
        if self.reranker and raw_docs:
            reranked = self.reranker.rerank(
                query=question,
                documents=[d.page_content for d in raw_docs],
                top_k=top_k,
            )
            final_docs = [content for content, score in reranked]
        else:
            final_docs = [d.page_content for d in raw_docs[:top_k]]
        
        # 4. 生成
        context = "\n\n---\n\n".join(final_docs)
        response = self.llm.invoke(
            K8S_RAG_PROMPT.format(
                summaries=context,
                question=question,
            )
        )
        
        return {
            "answer": response.content,
            "sources": [d.metadata.get("source") for d in raw_docs[:top_k]],
            "retrieved_count": len(raw_docs),
        }
```

---

## 10. 最佳实践与常见坑

### 最佳实践

- **元数据设计先行**：向量存储的元数据字段直接影响过滤效率，在建库前仔细设计
- **先优化检索，再优化生成**：RAG 质量差通常是检索问题，不要急于换模型
- **Re-ranking 显著提升质量**：从 Top-30 重排到 Top-5 通常比增大检索数量效果更好
- **多路召回融合**：BM25 + 向量的组合几乎在所有场景下优于单一检索方式
- **定期评估**：用 RAGAS 建立自动化质量监控，发现知识库更新后的退化

### 常见坑

- **chunk_size 过大**：单块信息太多导致向量语义模糊，影响检索精度（推荐 400-800 字符子块）
- **忽略重叠**：chunk_overlap 设为 0 导致句子在块边界被切断，损失上下文
- **不做元数据索引**：向量库中没有索引元数据字段，过滤查询速度极慢
- **Embedding 维度不匹配**：索引时 1536 维，查询时用了 3072 维的 API，结果毫无意义
- **忘记 Embedding 失效**：更新了分块策略或 Embedding 模型后，必须重新索引所有文档

---

## 关联文档

| 文档 | 关联内容 |
|------|---------|
| [03 - Agent 框架对比](./03-agent-frameworks-comparison.md) | LlamaIndex/LangChain RAG 实现 |
| [07 - 记忆管理](./07-memory-context-management.md) | 长期记忆 vs RAG 的边界 |
| [08 - 评测与可观测性](./08-agent-evaluation-observability.md) | RAGAS 评估框架详细配置 |
| [domain-11-ai-infra/20-vector-database-rag.md](../domain-11-ai-infra/20-vector-database-rag.md) | 向量数据库基础设施 |
| [15 - Agent 语料库差距分析](./15-agent-corpus-gap-analysis.md) | kudig-database 作为 RAG 语料的分析 |

---

*本文档为 kudig-database 项目 topic-ai-agent 专题原创内容。*
