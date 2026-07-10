---
title: LlamaIndex 数据 Agent 深度指南
description: 'LlamaIndex 核心架构与 Data Agent 全面解析，涵盖 Vector Store Index、Knowledge Graph Index、RAG Pipeline 编排、Tool 抽象及 K8s 生产部署'
summary: 'LlamaIndex 核心架构与 Data Agent 全面解析'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- llamaindex
- rag
- vector-store
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
- 架构师
estimated_read_time: 20min
intent_queries:
- LlamaIndex 数据 Agent 是什么
- 如何 LlamaIndex 数据 Agent
- LlamaIndex RAG Pipeline
trigger_keywords:
- llamaindex
- rag
- vector-store-index
- knowledge-graph
- data-agent
prerequisites:
- llm-basics
- python-basics
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# LlamaIndex 数据 Agent 深度指南

## 1. LlamaIndex 核心架构

### 1.1 设计定位

LlamaIndex（原 GPT Index）专注于**数据连接与索引**，核心理念是将私有数据转化为 LLM 可查询的知识库。与 LangChain 的通用编排定位不同，LlamaIndex 的优势在于：

- **数据摄取管道（Ingestion Pipeline）**：从 160+ 数据源加载文档
- **索引抽象（Index）**：多种索引结构适配不同查询模式
- **查询引擎（Query Engine）**：将索引暴露为自然语言查询接口
- **Data Agent**：在索引之上构建工具调用型 Agent

```
┌─────────────────────────────────────────────────────┐
│                   LlamaIndex 架构                    │
│                                                     │
│  ┌───────────┐    ┌──────────┐    ┌──────────────┐  │
│  │ Data      │    │ Index    │    │ Query        │  │
│  │ Connectors│───→│ Builder  │───→│ Engine       │  │
│  └───────────┘    └──────────┘    └──────┬───────┘  │
│       │                                  │          │
│  ┌────┴────┐                        ┌────┴─────┐    │
│  │ Loader  │                        │  Agent   │    │
│  │ (160+)  │                        │ (Tool)   │    │
│  └─────────┘                        └──────────┘    │
│                                                     │
│  ┌─────────────────────────────────────────────┐    │
│  │         Storage Context Layer               │    │
│  │  ┌──────────┐ ┌──────────┐ ┌─────────────┐  │    │
│  │  │ Docstore│ │ Index    │ │ Vector      │  │    │
│  │  │         │ │ Store    │ │ Store       │  │    │
│  │  └──────────┘ └──────────┘ └─────────────┘  │    │
│  └─────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────┘
```

### 1.2 数据摄取管道

```python
from llama_index.core import (
    VectorStoreIndex,
    SimpleDirectoryReader,
    Settings,
    StorageContext,
)
from llama_index.llms.openai import OpenAI
from llama_index.embeddings.openai import OpenAIEmbedding

# 全局配置
Settings.llm = OpenAI(model="gpt-4o", temperature=0)
Settings.embed_model = OpenAIEmbedding(model="text-embedding-3-small")
Settings.chunk_size = 512
Settings.chunk_overlap = 64

# 从目录加载文档
documents = SimpleDirectoryReader(
    input_dir="./k8s-docs",
    recursive=True,
    required_exts=[".md", ".txt", ".pdf"],
).load_data()

# 自定义 Reader
from llama_index.core.readers.base import BaseReader

class K8sEventReader(BaseReader):
    """从 Kubernetes Event 日志加载数据。"""

    def load_data(self, namespace: str = "default", **kwargs):
        import subprocess
        result = subprocess.run(
            ["kubectl", "get", "events", "-n", namespace, "-o", "json"],
            capture_output=True, text=True
        )
        events = json.loads(result.stdout)["items"]
        documents = []
        for event in events:
            text = (
                f"Type: {event['type']}\n"
                f"Reason: {event['reason']}\n"
                f"Message: {event['message']}\n"
                f"Object: {event['involvedObject']['kind']}/"
                f"{event['involvedObject']['name']}"
            )
            documents.append(Document(text=text, metadata={
                "namespace": namespace,
                "timestamp": event.get("lastTimestamp", ""),
                "type": event["type"],
            }))
        return documents
```

### 1.3 Ingestion Pipeline（生产级）

```python
from llama_index.core.ingestion import IngestionPipeline
from llama_index.core.node_parser import SentenceSplitter
from llama_index.core.extractors import (
    TitleExtractor,
    QuestionsAnsweredExtractor,
    SummaryExtractor,
)
from llama_index.core.ingestion.cache import IngestionCache

# 生产级摄取管道
pipeline = IngestionPipeline(
    transformations=[
        SentenceSplitter(chunk_size=512, chunk_overlap=64),
        TitleExtractor(llm=Settings.llm, nodes=5),
        QuestionsAnsweredExtractor(llm=Settings.llm, questions=3),
        SummaryExtractor(llm=Settings.llm, summaries=["self"]),
        Settings.embed_model,
    ],
    # 缓存避免重复处理
    cache=IngestionCache(
        collection="k8s_docs",
        persist_dir="./cache"
    ),
    vector_store=vector_store,  # 直接写入向量数据库
)

# 执行摄取
nodes = pipeline.run(documents=documents)

# 增量摄取（只处理新文档）
from llama_index.core.ingestion import IngestionPipeline
pipeline.run(
    documents=new_documents,
    in_place=True,
    show_progress=True,
)
```

---

## 2. 索引类型详解

### 2.1 Vector Store Index

最常用的索引类型，基于向量相似度检索：

```python
from llama_index.core import VectorStoreIndex, StorageContext
from llama_index.vector_stores.qdrant import QdrantVectorStore
import qdrant_client

# Qdrant 向量存储
client = qdrant_client.QdrantClient(host="qdrant", port=6333)
vector_store = QdrantVectorStore(
    client=client,
    collection_name="k8s_knowledge",
)

# 构建索引
storage_context = StorageContext.from_defaults(vector_store=vector_store)
index = VectorStoreIndex(
    nodes=nodes,
    storage_context=storage_context,
    show_progress=True,
)

# 查询
query_engine = index.as_query_engine(
    similarity_top_k=5,
    response_mode="compact",  # 紧凑模式减少 token
)
response = query_engine.query("Pod OOMKilled 的常见原因？")
print(response.source_nodes)  # 查看命中的文档片段
```

**向量存储后端对比：**

| 后端 | 分布式 | 混合搜索 | 适用场景 |
|------|--------|----------|---------|
| Qdrant | 是 | 是 | 生产推荐 |
| Chroma | 否 | 是 | 开发测试 |
| Pinecone | 是 | 是 | 全托管 SaaS |
| Weaviate | 是 | 是 | 需要 BM25 混合 |
| Milvus | 是 | 是 | 大规模向量 |
| pgvector | 集成 PG | 否 | 已有 PostgreSQL |

### 2.2 Knowledge Graph Index

构建实体-关系图谱，适合结构化知识查询：

```python
from llama_index.core import KnowledgeGraphIndex
from llama_index.core.storage.storage_context import StorageContext
from llama_index.graph_stores.neo4j import Neo4jGraphStore

# Neo4j 图存储
graph_store = Neo4jGraphStore(
    url="bolt://neo4j:7687",
    username="neo4j",
    password="password",
    database="k8s_graph",
)

storage_context = StorageContext.from_defaults(graph_store=graph_store)

# 构建知识图谱索引（自动提取实体和关系）
kg_index = KnowledgeGraphIndex(
    nodes=nodes,
    storage_context=storage_context,
    max_triplets_per_chunk=5,
    include_embeddings=True,  # 同时生成嵌入用于混合查询
)

# 查询图谱
kg_query_engine = kg_index.as_query_engine(
    response_mode="tree_summarize",
    verbose=True,
)
response = kg_query_engine.query("哪些 Deployment 依赖了 Redis？")
```

### 2.3 Summary Index

适合文档摘要和全局概览查询：

```python
from llama_index.core import SummaryIndex

summary_index = SummaryIndex(nodes=nodes)
summary_engine = summary_index.as_query_engine(
    response_mode="tree_summarize",  # 递归汇总
)
response = summary_engine.query("总结这份 K8s 运维手册的核心要点")
```

### 2.4 复合索引策略

```python
from llama_index.core import ComposableGraph
from llama_index.core.indices.keyword_table import SimpleKeywordTableIndex

# 组合多种索引
vector_index = VectorStoreIndex(nodes, storage_context=ctx)
keyword_index = SimpleKeywordTableIndex(nodes, storage_context=ctx)

# 构建组合图
graph = ComposableGraph.from_indices(
    SimpleKeywordTableIndex,
    children_indices=[vector_index, keyword_index],
    index_summaries=[
        "向量语义检索，适合自然语言问答",
        "关键词精确匹配，适合技术术语查询"
    ],
)

# 自动路由到合适的子索引
query_engine = graph.as_query_engine(
    query_configs=[
        {"index_struct_type": "keyword_table", "query_mode": "simple"},
        {"index_struct_type": "simple_dict", "query_mode": "default"},
    ],
)
```

---

## 3. Data Agent

### 3.1 OpenAI Function Agent

使用 OpenAI 函数调用能力构建 Agent：

```python
from llama_index.core.agent import OpenAIAgent
from llama_index.core.tools import QueryEngineTool, ToolMetadata

# 将查询引擎包装为工具
tools = [
    QueryEngineTool(
        query_engine=kg_query_engine,
        metadata=ToolMetadata(
            name="k8s_knowledge_base",
            description=(
                "Kubernetes 知识库，包含 Pod、Service、Deployment 等资源的"
                "文档、最佳实践和故障排查指南。适合回答 K8s 相关问题。"
            ),
        ),
    ),
    QueryEngineTool(
        query_engine=log_query_engine,
        metadata=ToolMetadata(
            name="k8s_event_log",
            description=(
                "Kubernetes 集群事件日志查询工具。"
                "查询 Pod 调度失败、容器崩溃、资源不足等实时事件。"
            ),
        ),
    ),
]

# 创建 Function Agent
agent = OpenAIAgent.from_tools(
    tools=tools,
    llm=OpenAI(model="gpt-4o"),
    verbose=True,
    system_prompt=(
        "你是 KuDig K8s 运维专家。"
        "优先使用知识库查询文档，使用事件日志查询实时状态。"
        "回答要包含具体命令和引用来源。"
    ),
)

# 流式交互
response = agent.chat("default 命名空间下 nginx Pod 一直重启，帮我排查")
print(response)

# 流式输出
stream_response = agent.stream_chat("分析集群资源使用情况")
for token in stream_response.response_gen:
    print(token, end="", flush=True)
```

### 3.2 ReAct Agent

基于 ReAct 推理范式的 Agent：

```python
from llama_index.core.agent import ReActAgent
from llama_index.core.tools import FunctionTool

# 自定义函数工具
def query_pod_status(namespace: str, pod_name: str) -> str:
    """查询指定 Pod 的状态详情。"""
    import subprocess
    result = subprocess.run(
        ["kubectl", "get", "pod", pod_name, "-n", namespace, "-o", "yaml"],
        capture_output=True, text=True, timeout=30
    )
    return result.stdout

def describe_node(node_name: str) -> str:
    """查看节点的资源分配和健康状态。"""
    import subprocess
    result = subprocess.run(
        ["kubectl", "describe", "node", node_name],
        capture_output=True, text=True, timeout=30
    )
    return result.stdout

# 包装为 LlamaIndex 工具
tools = [
    FunctionTool.from_defaults(fn=query_pod_status),
    FunctionTool.from_defaults(fn=describe_node),
    QueryEngineTool(
        query_engine=knowledge_engine,
        metadata=ToolMetadata(
            name="knowledge",
            description="K8s 知识库，查询最佳实践和排障指南"
        ),
    ),
]

# 创建 ReAct Agent
react_agent = ReActAgent.from_tools(
    tools=tools,
    llm=OpenAI(model="gpt-4o"),
    verbose=True,
    max_iterations=10,  # 最大推理步数
)

response = react_agent.chat("检查 node-1 的资源使用情况，是否有 Pod 被驱逐？")
```

### 3.3 Multi-Document Agent

多文档 Agent，每个文档拥有独立的子 Agent：

```python
from llama_index.core.agent import FnAgentWorker
from llama_index.core import SummaryIndex

# 为每个文档创建子 Agent
doc_agents = []
for doc_path in doc_files:
    docs = SimpleDirectoryReader(input_files=[doc_path]).load_data()
    index = VectorStoreIndex.from_documents(docs)

    doc_agent = OpenAIAgent.from_tools(
        index.as_query_engine().as_tools(
            tool_metadata=ToolMetadata(
                name=f"doc_{Path(doc_path).stem}",
                description=f"查询文档 {Path(doc_path).name}"
            )
        ),
        system_prompt=f"你负责回答关于 {Path(doc_path).name} 的问题。",
    )
    doc_agents.append(doc_agent)

# 创建顶层 Agent 管理多个子 Agent
top_agent = FnAgentWorker(
    agents=doc_agents,
    llm=OpenAI(model="gpt-4o"),
).as_agent()
```

---

## 4. RAG Pipeline 高级特性

### 4.1 混合检索（Hybrid Search）

```python
from llama_index.core.vector_stores import (
    VectorStoreQuery,
    MetadataFilters,
    ExactMatchFilter,
)
from llama_index.core.retrievers import VectorIndexRetriever

# 向量检索 + 元数据过滤
retriever = VectorIndexRetriever(
    index=index,
    similarity_top_k=10,
    filters=MetadataFilters(
        filters=[
            ExactMatchFilter(key="namespace", value="production"),
            ExactMatchFilter(key="severity", value="critical"),
        ]
    ),
)

# 混合检索（向量 + BM25）
from llama_index.core.retrievers import QueryFusionRetriever

hybrid_retriever = QueryFusionRetriever(
    retrievers=[vector_retriever, bm25_retriever],
    similarity_top_k=5,
    num_queries=4,  # 生成多个查询变体
    mode="reciprocal_rerank",  # RRF 融合
)
```

### 4.2 节点后处理

```python
from llama_index.core.postprocessor import (
    SimilarityPostprocessor,
    KeywordNodePostprocessor,
    SentenceEmbeddingPostprocessor,
    MetadataReplacementPostProcessor,
)

query_engine = index.as_query_engine(
    node_postprocessors=[
        # 过滤低相似度节点
        SimilarityPostprocessor(similarity_cutoff=0.7),
        # 关键词过滤
        KeywordNodePostprocessor(required_keywords=["OOM", "memory"]),
        # 用原始文本替换 chunk
        MetadataReplacementPostProcessor(target_metadata_key="window"),
        # 嵌入相似度过滤
        SentenceEmbeddingPostprocessor(embedding_cutoff=0.75),
    ],
)
```

### 4.3 响应合成策略

```python
from llama_index.core.response_synthesizers import (
    ResponseMode,
    get_response_synthesizer,
)

# 不同响应模式
strategies = {
    # 简单拼接上下文，一次性调用 LLM
    "compact": ResponseMode.COMPACT,
    # 递归汇总（适合长文档）
    "tree_summarize": ResponseMode.TREE_SUMMARIZE,
    # 逐节点生成，最后聚合
    "accumulate": ResponseMode.ACCUMULATE,
    # 紧凑 + refine 迭代
    "compact_accumulate": ResponseMode.COMPACT_ACCUMULATE,
}

# Refine 模式：逐步精炼答案
refine_synthesizer = get_response_synthesizer(
    response_mode=ResponseMode.REFINE,
    verbose=True,
)
```

### 4.4 评估框架

```python
from llama_index.core.evaluation import (
    FaithfulnessEvaluator,
    RelevancyEvaluator,
    CorrectnessEvaluator,
    BatchEvalRunner,
)

# 忠实度评估（答案是否基于上下文）
faithfulness = FaithfulnessEvaluator(llm=OpenAI(model="gpt-4o-mini"))

# 相关性评估（答案是否回答了问题）
relevancy = RelevancyEvaluator(llm=OpenAI(model="gpt-4o-mini"))

# 正确性评估（与标准答案对比）
correctness = CorrectnessEvaluator(llm=OpenAI(model="gpt-4o-mini"))

# 批量评估
runner = BatchEvalRunner(
    evaluators={
        "faithfulness": faithfulness,
        "relevancy": relevancy,
    },
    workers=8,
)
eval_results = await runner.aevaluate_queries(
    query_engine=query_engine,
    queries=test_queries,
    reference_answers=reference_answers,
)
```

---

## 5. 与 LangChain 对比选型

| 维度 | LlamaIndex | LangChain |
|------|-----------|-----------|
| 核心定位 | 数据索引与 RAG | 通用 LLM 编排 |
| 数据连接 | 160+ 原生连接器 | 需第三方集成 |
| 索引类型 | Vector/KG/Summary/Tree | 无原生索引抽象 |
| Agent 能力 | OpenAI/ReAct Agent | 更丰富（多框架） |
| 状态管理 | 基础 | LangGraph 强大 |
| 学习曲线 | 中 | 中低 |
| 生产成熟度 | 高 | 高 |

**选型建议：**
- 数据密集型 RAG 应用 → LlamaIndex
- 复杂 Agent 编排 → LangChain + LangGraph
- 两者混用 → LlamaIndex 做数据层，LangChain 做编排层

```python
# 混合使用示例
from llama_index.core import VectorStoreIndex
from langchain.agents import AgentExecutor, create_openai_functions_agent

# LlamaIndex 提供数据工具
llama_index = VectorStoreIndex.from_documents(docs)
query_engine = llama_index.as_query_engine()
llama_tool = query_engine.as_tool("k8s_docs", "查询 K8s 文档")

# LangChain 做 Agent 编排
agent = create_openai_functions_agent(llm, [llama_tool, other_tools])
executor = AgentExecutor(agent=agent, tools=[llama_tool, other_tools])
```

---

## 6. K8s 部署

### 6.1 Docker 化

```dockerfile
FROM python:3.11-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY src/ ./src/

# 健康检查
HEALTHCHECK --interval=30s --timeout=10s --retries=3 \
    CMD curl -f http://localhost:8000/healthz || exit 1

EXPOSE 8000
CMD ["uvicorn", "src.main:app", "--host", "0.0.0.0", "--port", "8000"]
```

### 6.2 K8s 资源配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llamaindex-agent
  namespace: ai-agents
spec:
  replicas: 2
  selector:
    matchLabels:
      app: llamaindex-agent
  template:
    metadata:
      labels:
        app: llamaindex-agent
    spec:
      serviceAccountName: llamaindex-agent
      containers:
        - name: agent
          image: registry.example.com/llamaindex-agent:1.0.0
          resources:
            requests:
              cpu: "500m"
              memory: "1Gi"
            limits:
              cpu: "2000m"
              memory: "4Gi"
          env:
            - name: OPENAI_API_KEY
              valueFrom:
                secretKeyRef:
                  name: llm-secrets
                  key: openai-api-key
            - name: QDRANT_URL
              value: "http://qdrant:6333"
            - name: NEO4J_URL
              value: "bolt://neo4j:7687"
          ports:
            - containerPort: 8000
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8000
            initialDelaySeconds: 15
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /readyz
              port: 8000
            initialDelaySeconds: 5
            periodSeconds: 5
```

### 6.3 RBAC 配置

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: llamaindex-agent
  namespace: ai-agents
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: k8s-reader
  namespace: default
rules:
  - apiGroups: [""]
    resources: ["pods", "services", "events", "nodes"]
    verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: llamaindex-reader
  namespace: default
subjects:
  - kind: ServiceAccount
    name: llamaindex-agent
    namespace: ai-agents
roleRef:
  kind: Role
  name: k8s-reader
  apiGroup: rbac.authorization.k8s.io
```

---

## Related

- [[AI基础设施/03-agent-runtime/01-langchain-langgraph-deep-dive|LangChain/LangGraph 深度指南]]
- [[AI基础设施/03-agent-runtime/07-agent-framework-selection-guide|Agent 框架选型决策树]]

## See Also

- [[AI基础设施/03-agent-runtime/03-crewai-multi-agent-framework|CrewAI 多 Agent 框架]]
- [[AI基础设施/03-agent-runtime/05-dify-agent-platform|Dify Agent 平台]]


<!-- risk-assessed -->
