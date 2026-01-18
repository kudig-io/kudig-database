# 59 - 向量数据库与RAG架构

> **适用版本**: v1.25 - v1.32 | **参考**: [LangChain](https://python.langchain.com/)

## 一、向量数据库对比

| 数据库 | QPS | 延迟 | 特性 | 适用场景 |
|--------|-----|------|------|---------|
| **Milvus** | 10K+ | <10ms | 分布式、GPU加速 | 大规模生产 |
| **Weaviate** | 5K | <20ms | GraphQL、模块化 | 企业级 |
| **Qdrant** | 3K | <15ms | Rust高性能 | 中等规模 |
| **Chroma** | 1K | <50ms | 轻量级 | 开发测试 |
| **pgvector** | 500 | <100ms | PostgreSQL扩展 | 现有PG用户 |

## 二、Milvus部署

```yaml
apiVersion: milvus.io/v1beta1
kind: Milvus
metadata:
  name: milvus-cluster
spec:
  mode: cluster
  dependencies:
    etcd:
      inCluster:
        replicas: 3
    pulsar:
      inCluster:
        replicas: 3
    storage:
      type: S3
  components:
    queryNode:
      replicas: 2
      resources:
        limits:
          cpu: "4"
          memory: "16Gi"
    dataNode:
      replicas: 2
    indexNode:
      replicas: 2
---
apiVersion: v1
kind: Service
metadata:
  name: milvus-service
spec:
  ports:
  - port: 19530
    name: grpc
  - port: 9091
    name: metrics
```

## 三、RAG架构流程

```
用户查询 → Embedding → 向量检索 → Rerank → LLM生成 → 响应
   ↓         ↓          ↓          ↓        ↓        ↓
  Query   模型API    Milvus   重排序   vLLM    答案
```

## 四、RAG实现 (Python)

```python
from langchain.vectorstores import Milvus
from langchain.embeddings import HuggingFaceEmbeddings
from langchain.llms import VLLM

# 1. 初始化
embeddings = HuggingFaceEmbeddings(
    model_name="sentence-transformers/all-MiniLM-L6-v2"
)
vectorstore = Milvus(
    embedding_function=embeddings,
    connection_args={"host": "milvus", "port": "19530"}
)

# 2. 检索
def retrieve(query, top_k=5):
    docs = vectorstore.similarity_search(query, k=top_k)
    return docs

# 3. 生成
llm = VLLM(model="llama-2-7b-chat")

def generate_answer(query, context):
    prompt = f"Context: {context}\n\nQuestion: {query}\nAnswer:"
    return llm(prompt)

# 4. RAG Pipeline
def rag_query(query):
    docs = retrieve(query)
    context = "\n".join([doc.page_content for doc in docs])
    answer = generate_answer(query, context)
    return answer
```

## 五、性能优化

| 优化项 | 方法 | 效果 |
|--------|------|------|
| **索引类型** | HNSW优于IVF | 速度↑3x |
| **分片策略** | 按时间/主题分片 | 吞吐↑2x |
| **缓存** | Redis缓存热查询 | 命中率30% |
| **批处理** | 批量embedding | 速度↑5x |
| **GPU加速** | Milvus GPU版本 | 速度↑10x |

## 六、成本分析

**Milvus集群 (1M向量, 768维):**
- 存储: ~3GB (S3 Standard: $0.07/月)
- 计算: 2×4核16GB = ~$200/月
- 总成本: ~$200/月

**vs 托管服务 (Pinecone):**
- 1M向量 = $70/月
- 但无需运维

## 七、监控指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: milvus-metrics
spec:
  endpoints:
  - port: metrics
    interval: 15s
---
# 关键指标
- milvus_search_latency_seconds
- milvus_insert_qps
- milvus_cache_hit_ratio
- milvus_disk_usage_bytes
```

## 八、最佳实践

1. **向量维度**: 768维平衡性能和效果
2. **索引参数**: HNSW (M=16, efConstruction=200)
3. **分片数**: 每分片500万向量
4. **副本数**: 2-3个副本保证可用性
5. **定期压缩**: 每周压缩一次删除数据
6. **备份策略**: S3增量备份

---
**相关**: [115-AI数据Pipeline](../115-ai-data-pipeline.md) | **版本**: Milvus 2.3+, K8s v1.27+
