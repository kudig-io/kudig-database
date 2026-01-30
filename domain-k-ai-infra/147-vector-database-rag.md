# 147 - 向量数据库与RAG架构

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 高级 | **参考**: [Milvus](https://milvus.io/docs) | [Weaviate](https://weaviate.io/developers/weaviate) | [LangChain](https://python.langchain.com/)

## 一、RAG系统架构全景

### 1.1 生产级RAG架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        Production RAG System Architecture                            │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │                           Data Ingestion Pipeline                            │    │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐           │    │
│  │  │  Docs   │  │  Parse  │  │  Chunk  │  │ Embed   │  │  Store  │           │    │
│  │  │ (PDF/   │─▶│ Extract │─▶│ Split   │─▶│ Vectors │─▶│ Vector  │           │    │
│  │  │ Web/DB) │  │ Clean   │  │ Overlap │  │ (E5/BGE)│  │   DB    │           │    │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘  └─────────┘           │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                         │                                            │
│  ┌──────────────────────────────────────▼──────────────────────────────────────┐    │
│  │                              Vector Database                                 │    │
│  │  ┌─────────────────────────────────────────────────────────────────────┐   │    │
│  │  │                     Milvus / Weaviate / Qdrant                      │   │    │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────────┐ │   │    │
│  │  │  │   Index     │  │   Storage   │  │      Query Engine          │ │   │    │
│  │  │  │  (HNSW/     │  │  (Segments, │  │  - ANN Search              │ │   │    │
│  │  │  │   IVF/      │  │   Shards,   │  │  - Hybrid Search           │ │   │    │
│  │  │  │   DiskANN)  │  │   Replicas) │  │  - Filtered Search         │ │   │    │
│  │  │  └─────────────┘  └─────────────┘  └─────────────────────────────┘ │   │    │
│  │  └─────────────────────────────────────────────────────────────────────┘   │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                         │                                            │
│  ┌──────────────────────────────────────▼──────────────────────────────────────┐    │
│  │                            Retrieval Pipeline                                │    │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐           │    │
│  │  │  Query  │  │ Rewrite │  │ Vector  │  │ Rerank  │  │ Context │           │    │
│  │  │  Input  │─▶│ Expand  │─▶│ Search  │─▶│ Filter  │─▶│ Build   │           │    │
│  │  │         │  │ (HyDE)  │  │ +BM25   │  │ (Cohere)│  │         │           │    │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘  └─────────┘           │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                         │                                            │
│  ┌──────────────────────────────────────▼──────────────────────────────────────┐    │
│  │                           Generation Pipeline                                │    │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────────┐ │    │
│  │  │   Prompt        │  │      LLM        │  │     Post-Processing        │ │    │
│  │  │   Template      │─▶│   (vLLM/TGI)    │─▶│   Citation, Validation     │ │    │
│  │  │   + Context     │  │   Generation    │  │   Fact-checking            │ │    │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────────────────┘ │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 向量数据库全面对比

| 数据库 | QPS | P99延迟 | 最大向量 | 分布式 | GPU加速 | 开源 | 托管服务 | 适用场景 |
|--------|-----|---------|----------|-------|---------|------|----------|---------|
| **Milvus** | 10K+ | <10ms | 10B+ | ✓ | ✓ | ✓ | Zilliz | 大规模生产 |
| **Weaviate** | 5K | <20ms | 1B+ | ✓ | ✗ | ✓ | Weaviate Cloud | 企业级 |
| **Qdrant** | 8K | <15ms | 1B+ | ✓ | ✗ | ✓ | Qdrant Cloud | 高性能 |
| **Pinecone** | 10K+ | <20ms | 1B+ | ✓ | ✓ | ✗ | Pinecone | 托管首选 |
| **Chroma** | 1K | <50ms | 10M | ✗ | ✗ | ✓ | - | 开发测试 |
| **pgvector** | 500 | <100ms | 100M | ✗ | ✗ | ✓ | - | PG用户 |
| **Elasticsearch** | 3K | <50ms | 1B+ | ✓ | ✗ | ✓ | Elastic Cloud | 混合搜索 |
| **Redis Stack** | 15K+ | <5ms | 100M | ✓ | ✗ | ✓ | Redis Cloud | 低延迟 |

### 1.3 Embedding模型对比

| 模型 | 维度 | MTEB分数 | 中文支持 | 速度 | 适用场景 |
|-----|------|---------|---------|------|---------|
| **text-embedding-3-large** | 3072 | 64.6 | ✓ | 中 | 通用首选 |
| **text-embedding-3-small** | 1536 | 62.3 | ✓ | 快 | 成本敏感 |
| **E5-large-v2** | 1024 | 62.0 | ✓ | 中 | 开源首选 |
| **BGE-large-zh-v1.5** | 1024 | 64.5 | ★★★ | 中 | 中文首选 |
| **Cohere embed-v3** | 1024 | 64.5 | ✓ | 快 | 多语言 |
| **Jina-embeddings-v2** | 768 | 60.4 | ✓ | 快 | 长文本 |
| **GTE-large** | 1024 | 63.1 | ✓ | 中 | 通用 |
| **instructor-xl** | 768 | 61.8 | ✓ | 慢 | 指令跟随 |

---

## 二、Milvus生产部署

### 2.1 Milvus集群架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           Milvus Cluster Architecture                                │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                              Access Layer                                     │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │   │
│  │  │   Proxy Node    │  │   Proxy Node    │  │   Proxy Node    │               │   │
│  │  │   (gRPC/HTTP)   │  │   (gRPC/HTTP)   │  │   (gRPC/HTTP)   │               │   │
│  │  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘               │   │
│  └───────────┼────────────────────┼────────────────────┼────────────────────────┘   │
│              │                    │                    │                             │
│  ┌───────────▼────────────────────▼────────────────────▼────────────────────────┐   │
│  │                           Coordinator Layer                                   │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │   │
│  │  │  Root Coord │  │ Query Coord │  │ Data Coord  │  │ Index Coord │          │   │
│  │  │  (DDL/DCL)  │  │ (Query Mgmt)│  │ (Data Mgmt) │  │ (Index Mgmt)│          │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                         │                                            │
│  ┌──────────────────────────────────────▼───────────────────────────────────────┐   │
│  │                              Worker Layer                                     │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                          Query Nodes                                     │ │   │
│  │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐       │ │   │
│  │  │  │  QN-1   │  │  QN-2   │  │  QN-3   │  │  QN-4   │  │  QN-5   │       │ │   │
│  │  │  │ Shard 1 │  │ Shard 2 │  │ Shard 3 │  │ Shard 1 │  │ Shard 2 │       │ │   │
│  │  │  │ (Rep 1) │  │ (Rep 1) │  │ (Rep 1) │  │ (Rep 2) │  │ (Rep 2) │       │ │   │
│  │  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘  └─────────┘       │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                          Data Nodes                                      │ │   │
│  │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐                                  │ │   │
│  │  │  │  DN-1   │  │  DN-2   │  │  DN-3   │                                  │ │   │
│  │  │  │ Insert  │  │ Insert  │  │ Insert  │                                  │ │   │
│  │  │  │ Buffer  │  │ Buffer  │  │ Buffer  │                                  │ │   │
│  │  │  └─────────┘  └─────────┘  └─────────┘                                  │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                          Index Nodes (GPU)                               │ │   │
│  │  │  ┌─────────┐  ┌─────────┐                                               │ │   │
│  │  │  │  IN-1   │  │  IN-2   │                                               │ │   │
│  │  │  │ A100 GPU│  │ A100 GPU│                                               │ │   │
│  │  │  └─────────┘  └─────────┘                                               │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                         │                                            │
│  ┌──────────────────────────────────────▼───────────────────────────────────────┐   │
│  │                              Storage Layer                                    │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │   │
│  │  │     etcd        │  │     Pulsar      │  │   MinIO/S3      │               │   │
│  │  │  (Metadata)     │  │  (Log Broker)   │  │  (Object Store) │               │   │
│  │  │  3 replicas     │  │  3 brokers      │  │  Tiered Storage │               │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Milvus Operator部署

```yaml
# Milvus Operator安装
apiVersion: v1
kind: Namespace
metadata:
  name: milvus
---
# 使用Helm安装Operator
# helm repo add milvus-operator https://zilliztech.github.io/milvus-operator/
# helm install milvus-operator milvus-operator/milvus-operator -n milvus
---
apiVersion: milvus.io/v1beta1
kind: Milvus
metadata:
  name: milvus-cluster
  namespace: milvus
spec:
  mode: cluster
  
  # 依赖组件配置
  dependencies:
    # etcd配置
    etcd:
      inCluster:
        deletionPolicy: Delete
        pvcDeletion: true
        values:
          replicaCount: 3
          resources:
            requests:
              cpu: "1"
              memory: "2Gi"
            limits:
              cpu: "2"
              memory: "4Gi"
          persistence:
            storageClass: fast-ssd
            size: 50Gi
    
    # Pulsar配置
    pulsar:
      inCluster:
        deletionPolicy: Delete
        pvcDeletion: true
        values:
          components:
            autorecovery: true
            proxy: true
            toolset: false
          broker:
            replicaCount: 3
            resources:
              requests:
                cpu: "2"
                memory: "8Gi"
          bookkeeper:
            replicaCount: 3
            resources:
              requests:
                cpu: "2"
                memory: "8Gi"
            volumes:
              journal:
                size: 100Gi
                storageClass: fast-ssd
              ledgers:
                size: 200Gi
                storageClass: fast-ssd
          zookeeper:
            replicaCount: 3
    
    # 对象存储配置
    storage:
      type: S3
      secretRef: milvus-s3-secret
      external: true
      endpoint: s3.amazonaws.com
      bucket: milvus-data
      useSSL: true
      useIAM: true
  
  # 组件配置
  components:
    # Proxy配置
    proxy:
      replicas: 3
      resources:
        requests:
          cpu: "2"
          memory: "4Gi"
        limits:
          cpu: "4"
          memory: "8Gi"
      serviceType: LoadBalancer
      annotations:
        service.beta.kubernetes.io/aws-load-balancer-type: nlb
        service.beta.kubernetes.io/aws-load-balancer-internal: "true"
    
    # QueryNode配置
    queryNode:
      replicas: 5
      resources:
        requests:
          cpu: "4"
          memory: "16Gi"
        limits:
          cpu: "8"
          memory: "32Gi"
    
    # DataNode配置
    dataNode:
      replicas: 3
      resources:
        requests:
          cpu: "2"
          memory: "8Gi"
        limits:
          cpu: "4"
          memory: "16Gi"
    
    # IndexNode配置 (GPU加速)
    indexNode:
      replicas: 2
      resources:
        requests:
          cpu: "4"
          memory: "16Gi"
          nvidia.com/gpu: "1"
        limits:
          cpu: "8"
          memory: "32Gi"
          nvidia.com/gpu: "1"
    
    # RootCoord配置
    rootCoord:
      replicas: 1
      resources:
        requests:
          cpu: "1"
          memory: "2Gi"
    
    # QueryCoord配置
    queryCoord:
      replicas: 1
      resources:
        requests:
          cpu: "1"
          memory: "2Gi"
    
    # DataCoord配置
    dataCoord:
      replicas: 1
      resources:
        requests:
          cpu: "1"
          memory: "2Gi"
    
    # IndexCoord配置
    indexCoord:
      replicas: 1
      resources:
        requests:
          cpu: "1"
          memory: "2Gi"
  
  # 配置参数
  config:
    # 通用配置
    common:
      gracefulTime: 5000
      gracefulStopTimeout: 30
    
    # Proxy配置
    proxy:
      maxTaskNum: 1024
      maxConnectionNum: 10000
      accessLog:
        enable: true
        filename: access.log
    
    # QueryNode配置
    queryNode:
      gracefulTime: 5000
      enableDisk: true
      cache:
        enabled: true
        memoryLimit: 8589934592  # 8GB
    
    # 索引配置
    indexNode:
      enableDisk: true
    
    # 数据配置
    dataCoord:
      segment:
        maxSize: 512
        sealProportion: 0.23
      compaction:
        enableAutoCompaction: true
---
apiVersion: v1
kind: Secret
metadata:
  name: milvus-s3-secret
  namespace: milvus
type: Opaque
stringData:
  accesskey: "AKIAXXXXXXXXXXXXXXXX"
  secretkey: "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
```

### 2.3 Collection和索引配置

```python
# milvus_collection.py - Milvus Collection管理
from pymilvus import (
    connections, Collection, FieldSchema, CollectionSchema, 
    DataType, utility, MilvusClient
)
from typing import List, Dict, Optional
import numpy as np

class MilvusCollectionManager:
    """Milvus Collection管理器"""
    
    def __init__(
        self,
        host: str = "milvus-cluster-proxy.milvus",
        port: int = 19530,
        db_name: str = "default"
    ):
        self.host = host
        self.port = port
        self.db_name = db_name
        
        # 连接Milvus
        connections.connect(
            alias="default",
            host=host,
            port=port,
            db_name=db_name
        )
    
    def create_collection(
        self,
        collection_name: str,
        dim: int = 1024,
        index_type: str = "HNSW",
        metric_type: str = "COSINE",
        enable_dynamic_field: bool = True,
    ):
        """创建Collection"""
        
        # 定义Schema
        fields = [
            FieldSchema(
                name="id",
                dtype=DataType.VARCHAR,
                is_primary=True,
                max_length=64
            ),
            FieldSchema(
                name="vector",
                dtype=DataType.FLOAT_VECTOR,
                dim=dim
            ),
            FieldSchema(
                name="text",
                dtype=DataType.VARCHAR,
                max_length=65535
            ),
            FieldSchema(
                name="metadata",
                dtype=DataType.JSON
            ),
            FieldSchema(
                name="created_at",
                dtype=DataType.INT64
            ),
        ]
        
        schema = CollectionSchema(
            fields=fields,
            description=f"RAG Collection: {collection_name}",
            enable_dynamic_field=enable_dynamic_field
        )
        
        # 创建Collection
        collection = Collection(
            name=collection_name,
            schema=schema,
            using="default",
            shards_num=4,  # 分片数
            num_partitions=16  # 分区数
        )
        
        # 创建索引
        index_params = self._get_index_params(index_type, metric_type)
        collection.create_index(
            field_name="vector",
            index_params=index_params,
            index_name="vector_index"
        )
        
        # 创建标量索引
        collection.create_index(
            field_name="created_at",
            index_name="created_at_index"
        )
        
        # 加载到内存
        collection.load()
        
        return collection
    
    def _get_index_params(
        self,
        index_type: str,
        metric_type: str
    ) -> dict:
        """获取索引参数"""
        
        index_configs = {
            "HNSW": {
                "index_type": "HNSW",
                "metric_type": metric_type,
                "params": {
                    "M": 16,  # 每个节点的连接数
                    "efConstruction": 256  # 构建时的搜索范围
                }
            },
            "IVF_FLAT": {
                "index_type": "IVF_FLAT",
                "metric_type": metric_type,
                "params": {
                    "nlist": 1024  # 聚类中心数
                }
            },
            "IVF_SQ8": {
                "index_type": "IVF_SQ8",
                "metric_type": metric_type,
                "params": {
                    "nlist": 1024
                }
            },
            "IVF_PQ": {
                "index_type": "IVF_PQ",
                "metric_type": metric_type,
                "params": {
                    "nlist": 1024,
                    "m": 16,  # 子向量数
                    "nbits": 8  # 每个子向量的位数
                }
            },
            "DISKANN": {
                "index_type": "DISKANN",
                "metric_type": metric_type,
                "params": {}
            },
            "GPU_IVF_FLAT": {
                "index_type": "GPU_IVF_FLAT",
                "metric_type": metric_type,
                "params": {
                    "nlist": 1024
                }
            },
            "GPU_IVF_PQ": {
                "index_type": "GPU_IVF_PQ",
                "metric_type": metric_type,
                "params": {
                    "nlist": 1024,
                    "m": 16,
                    "nbits": 8
                }
            }
        }
        
        return index_configs.get(index_type, index_configs["HNSW"])
    
    def insert_vectors(
        self,
        collection_name: str,
        vectors: List[List[float]],
        texts: List[str],
        ids: List[str],
        metadata: List[dict],
        batch_size: int = 1000
    ):
        """批量插入向量"""
        
        collection = Collection(collection_name)
        
        # 分批插入
        total = len(vectors)
        for i in range(0, total, batch_size):
            batch_end = min(i + batch_size, total)
            
            data = [
                ids[i:batch_end],
                vectors[i:batch_end],
                texts[i:batch_end],
                metadata[i:batch_end],
                [int(time.time()) for _ in range(batch_end - i)]
            ]
            
            collection.insert(data)
        
        # 刷新到存储
        collection.flush()
        
        return total
    
    def search(
        self,
        collection_name: str,
        query_vectors: List[List[float]],
        top_k: int = 10,
        filter_expr: str = None,
        output_fields: List[str] = None,
        search_params: dict = None
    ):
        """向量搜索"""
        
        collection = Collection(collection_name)
        
        if search_params is None:
            search_params = {
                "metric_type": "COSINE",
                "params": {"ef": 128}  # HNSW搜索参数
            }
        
        if output_fields is None:
            output_fields = ["id", "text", "metadata"]
        
        results = collection.search(
            data=query_vectors,
            anns_field="vector",
            param=search_params,
            limit=top_k,
            expr=filter_expr,
            output_fields=output_fields,
            consistency_level="Strong"
        )
        
        return results
    
    def hybrid_search(
        self,
        collection_name: str,
        query_vector: List[float],
        query_text: str,
        top_k: int = 10,
        alpha: float = 0.5,  # 向量权重
        filter_expr: str = None
    ):
        """混合搜索 (向量 + BM25)"""
        
        collection = Collection(collection_name)
        
        # 向量搜索
        vector_results = self.search(
            collection_name,
            [query_vector],
            top_k=top_k * 2,
            filter_expr=filter_expr
        )[0]
        
        # 组合分数
        combined_results = []
        for hit in vector_results:
            combined_score = hit.score * alpha
            combined_results.append({
                "id": hit.id,
                "score": combined_score,
                "text": hit.entity.get("text"),
                "metadata": hit.entity.get("metadata")
            })
        
        # 排序并返回top_k
        combined_results.sort(key=lambda x: x["score"], reverse=True)
        return combined_results[:top_k]

# 索引类型选择指南
INDEX_SELECTION_GUIDE = """
索引选择指南:

1. HNSW (推荐默认):
   - 优点: 高召回率, 低延迟
   - 缺点: 内存占用大
   - 适用: <1亿向量, 高精度需求

2. IVF_FLAT:
   - 优点: 内存占用适中
   - 缺点: 延迟较高
   - 适用: 精度敏感, 可接受延迟

3. IVF_PQ:
   - 优点: 极低内存占用
   - 缺点: 召回率下降
   - 适用: 超大规模, 成本敏感

4. DiskANN:
   - 优点: SSD存储, 无限扩展
   - 缺点: 延迟较高
   - 适用: >1亿向量

5. GPU索引:
   - 优点: 极高吞吐量
   - 缺点: 需要GPU
   - 适用: 高并发, 实时搜索
"""
```

### 2.4 Milvus监控配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: milvus-monitor
  namespace: milvus
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: milvus
  endpoints:
  - port: metrics
    interval: 15s
    path: /metrics
---
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: milvus-alerts
  namespace: milvus
spec:
  groups:
  - name: milvus-health
    rules:
    # 搜索延迟告警
    - alert: MilvusHighSearchLatency
      expr: |
        histogram_quantile(0.99, sum(rate(milvus_proxy_search_vectors_duration_seconds_bucket[5m])) by (le)) > 0.5
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Milvus搜索延迟过高"
        description: "P99延迟 {{ $value }}秒"
    
    # QPS下降告警
    - alert: MilvusLowQPS
      expr: |
        sum(rate(milvus_proxy_search_vectors_total[5m])) < 100
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Milvus QPS下降"
    
    # 内存使用告警
    - alert: MilvusHighMemoryUsage
      expr: |
        milvus_querynode_memory_usage_ratio > 0.85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "QueryNode内存使用率过高"
    
    # 磁盘空间告警
    - alert: MilvusLowDiskSpace
      expr: |
        milvus_storage_disk_usage_ratio > 0.80
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "存储磁盘空间不足"
    
    # 组件不健康
    - alert: MilvusComponentUnhealthy
      expr: |
        milvus_component_healthy == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Milvus组件不健康"
        description: "组件 {{ $labels.component }} 状态异常"
  
  - name: milvus-performance
    rules:
    # 搜索QPS
    - record: milvus:search_qps
      expr: sum(rate(milvus_proxy_search_vectors_total[5m]))
    
    # 插入QPS
    - record: milvus:insert_qps
      expr: sum(rate(milvus_proxy_insert_vectors_total[5m]))
    
    # 搜索延迟P50/P95/P99
    - record: milvus:search_latency_p50
      expr: |
        histogram_quantile(0.50, sum(rate(milvus_proxy_search_vectors_duration_seconds_bucket[5m])) by (le))
    
    - record: milvus:search_latency_p95
      expr: |
        histogram_quantile(0.95, sum(rate(milvus_proxy_search_vectors_duration_seconds_bucket[5m])) by (le))
    
    - record: milvus:search_latency_p99
      expr: |
        histogram_quantile(0.99, sum(rate(milvus_proxy_search_vectors_duration_seconds_bucket[5m])) by (le))
```

---

## 三、Weaviate部署

### 3.1 Weaviate集群配置

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: weaviate
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: weaviate
  namespace: weaviate
spec:
  serviceName: weaviate-headless
  replicas: 3
  selector:
    matchLabels:
      app: weaviate
  template:
    metadata:
      labels:
        app: weaviate
    spec:
      containers:
      - name: weaviate
        image: semitechnologies/weaviate:1.24.1
        
        env:
        # 集群配置
        - name: CLUSTER_HOSTNAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: CLUSTER_GOSSIP_BIND_PORT
          value: "7100"
        - name: CLUSTER_DATA_BIND_PORT
          value: "7101"
        - name: CLUSTER_JOIN
          value: "weaviate-0.weaviate-headless.weaviate.svc.cluster.local:7100,weaviate-1.weaviate-headless.weaviate.svc.cluster.local:7100,weaviate-2.weaviate-headless.weaviate.svc.cluster.local:7100"
        
        # 性能配置
        - name: QUERY_DEFAULTS_LIMIT
          value: "25"
        - name: QUERY_MAXIMUM_RESULTS
          value: "10000"
        - name: PERSISTENCE_DATA_PATH
          value: "/var/lib/weaviate"
        
        # 模块配置
        - name: ENABLE_MODULES
          value: "text2vec-openai,text2vec-cohere,text2vec-huggingface,generative-openai,generative-cohere,qna-openai,reranker-cohere"
        - name: DEFAULT_VECTORIZER_MODULE
          value: "text2vec-openai"
        
        # 认证配置
        - name: AUTHENTICATION_APIKEY_ENABLED
          value: "true"
        - name: AUTHENTICATION_APIKEY_ALLOWED_KEYS
          valueFrom:
            secretKeyRef:
              name: weaviate-secrets
              key: api-keys
        - name: AUTHENTICATION_APIKEY_USERS
          value: "admin,readonly"
        
        # 资源限制
        - name: LIMIT_RESOURCES
          value: "true"
        - name: MAXIMUM_CONCURRENT_GET_REQUESTS
          value: "500"
        
        ports:
        - containerPort: 8080
          name: http
        - containerPort: 50051
          name: grpc
        - containerPort: 7100
          name: gossip
        - containerPort: 7101
          name: data
        
        resources:
          requests:
            cpu: "4"
            memory: "16Gi"
          limits:
            cpu: "8"
            memory: "32Gi"
        
        volumeMounts:
        - name: data
          mountPath: /var/lib/weaviate
        
        livenessProbe:
          httpGet:
            path: /v1/.well-known/live
            port: 8080
          initialDelaySeconds: 120
          periodSeconds: 30
        
        readinessProbe:
          httpGet:
            path: /v1/.well-known/ready
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
      
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: weaviate
            topologyKey: kubernetes.io/hostname
  
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 500Gi
---
apiVersion: v1
kind: Service
metadata:
  name: weaviate
  namespace: weaviate
spec:
  type: LoadBalancer
  ports:
  - port: 8080
    targetPort: 8080
    name: http
  - port: 50051
    targetPort: 50051
    name: grpc
  selector:
    app: weaviate
---
apiVersion: v1
kind: Service
metadata:
  name: weaviate-headless
  namespace: weaviate
spec:
  clusterIP: None
  ports:
  - port: 7100
    name: gossip
  - port: 7101
    name: data
  selector:
    app: weaviate
---
apiVersion: v1
kind: Secret
metadata:
  name: weaviate-secrets
  namespace: weaviate
type: Opaque
stringData:
  api-keys: "admin-key-xxxxx,readonly-key-xxxxx"
```

### 3.2 Weaviate Schema和数据操作

```python
# weaviate_operations.py - Weaviate操作
import weaviate
from weaviate.classes.config import Configure, Property, DataType
from weaviate.classes.query import MetadataQuery, Filter
from typing import List, Dict, Optional
import json

class WeaviateManager:
    """Weaviate管理器"""
    
    def __init__(
        self,
        url: str = "http://weaviate.weaviate:8080",
        api_key: str = None,
        openai_api_key: str = None
    ):
        headers = {}
        if openai_api_key:
            headers["X-OpenAI-Api-Key"] = openai_api_key
        
        self.client = weaviate.connect_to_custom(
            http_host=url.replace("http://", "").split(":")[0],
            http_port=8080,
            http_secure=False,
            grpc_host=url.replace("http://", "").split(":")[0],
            grpc_port=50051,
            grpc_secure=False,
            auth_credentials=weaviate.auth.AuthApiKey(api_key) if api_key else None,
            headers=headers
        )
    
    def create_collection(
        self,
        name: str,
        vectorizer: str = "text2vec-openai",
        generative: str = "generative-openai",
        replication_factor: int = 3,
    ):
        """创建Collection"""
        
        # 配置向量化器
        vectorizer_config = None
        if vectorizer == "text2vec-openai":
            vectorizer_config = Configure.Vectorizer.text2vec_openai(
                model="text-embedding-3-small",
                vectorize_collection_name=False
            )
        elif vectorizer == "text2vec-cohere":
            vectorizer_config = Configure.Vectorizer.text2vec_cohere(
                model="embed-multilingual-v3.0"
            )
        
        # 配置生成器
        generative_config = None
        if generative == "generative-openai":
            generative_config = Configure.Generative.openai(
                model="gpt-4-turbo-preview"
            )
        
        # 创建Collection
        collection = self.client.collections.create(
            name=name,
            vectorizer_config=vectorizer_config,
            generative_config=generative_config,
            replication_config=Configure.replication(
                factor=replication_factor
            ),
            properties=[
                Property(
                    name="content",
                    data_type=DataType.TEXT,
                    vectorize_property_name=False,
                    tokenization=weaviate.classes.config.Tokenization.WORD
                ),
                Property(
                    name="title",
                    data_type=DataType.TEXT,
                    vectorize_property_name=False
                ),
                Property(
                    name="source",
                    data_type=DataType.TEXT,
                    vectorize_property_name=False,
                    skip_vectorization=True
                ),
                Property(
                    name="metadata",
                    data_type=DataType.OBJECT,
                    skip_vectorization=True
                ),
                Property(
                    name="created_at",
                    data_type=DataType.DATE,
                    skip_vectorization=True
                ),
            ],
            # 向量索引配置
            vector_index_config=Configure.VectorIndex.hnsw(
                distance_metric=weaviate.classes.config.VectorDistances.COSINE,
                ef_construction=256,
                max_connections=64,
                ef=128
            ),
            # 倒排索引配置
            inverted_index_config=Configure.inverted_index(
                bm25_b=0.75,
                bm25_k1=1.2,
                index_timestamps=True,
                index_null_state=True,
                index_property_length=True
            )
        )
        
        return collection
    
    def insert_data(
        self,
        collection_name: str,
        data: List[Dict],
        batch_size: int = 100
    ):
        """批量插入数据"""
        
        collection = self.client.collections.get(collection_name)
        
        with collection.batch.dynamic() as batch:
            for item in data:
                batch.add_object(
                    properties={
                        "content": item["content"],
                        "title": item.get("title", ""),
                        "source": item.get("source", ""),
                        "metadata": item.get("metadata", {}),
                        "created_at": item.get("created_at")
                    },
                    uuid=item.get("id"),
                    vector=item.get("vector")  # 可选,如果不提供则自动向量化
                )
        
        return len(data)
    
    def search(
        self,
        collection_name: str,
        query: str,
        top_k: int = 10,
        filters: dict = None,
        alpha: float = 0.5  # 混合搜索权重
    ):
        """混合搜索"""
        
        collection = self.client.collections.get(collection_name)
        
        # 构建过滤器
        filter_obj = None
        if filters:
            filter_obj = self._build_filter(filters)
        
        # 执行混合搜索
        response = collection.query.hybrid(
            query=query,
            alpha=alpha,  # 0=纯BM25, 1=纯向量
            limit=top_k,
            filters=filter_obj,
            return_metadata=MetadataQuery(
                score=True,
                explain_score=True
            )
        )
        
        results = []
        for obj in response.objects:
            results.append({
                "id": str(obj.uuid),
                "content": obj.properties.get("content"),
                "title": obj.properties.get("title"),
                "source": obj.properties.get("source"),
                "score": obj.metadata.score,
                "explain_score": obj.metadata.explain_score
            })
        
        return results
    
    def generate_with_context(
        self,
        collection_name: str,
        query: str,
        prompt: str,
        top_k: int = 5
    ):
        """RAG生成"""
        
        collection = self.client.collections.get(collection_name)
        
        response = collection.generate.near_text(
            query=query,
            limit=top_k,
            single_prompt=prompt,
            grouped_task=f"""
            Based on the following context, answer the question: {query}
            
            Context:
            {{content}}
            
            Answer:
            """
        )
        
        return {
            "generated": response.generated,
            "sources": [
                {
                    "content": obj.properties.get("content"),
                    "source": obj.properties.get("source")
                }
                for obj in response.objects
            ]
        }
    
    def _build_filter(self, filters: dict):
        """构建过滤器"""
        conditions = []
        
        for key, value in filters.items():
            if isinstance(value, dict):
                operator = value.get("operator", "equal")
                val = value.get("value")
                
                if operator == "equal":
                    conditions.append(Filter.by_property(key).equal(val))
                elif operator == "greater_than":
                    conditions.append(Filter.by_property(key).greater_than(val))
                elif operator == "less_than":
                    conditions.append(Filter.by_property(key).less_than(val))
                elif operator == "contains":
                    conditions.append(Filter.by_property(key).contains_any(val))
            else:
                conditions.append(Filter.by_property(key).equal(value))
        
        if len(conditions) == 1:
            return conditions[0]
        elif len(conditions) > 1:
            return Filter.all_of(conditions)
        return None
    
    def close(self):
        """关闭连接"""
        self.client.close()
```

---

## 四、Qdrant部署

### 4.1 Qdrant集群配置

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: qdrant
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: qdrant
  namespace: qdrant
spec:
  serviceName: qdrant-headless
  replicas: 3
  selector:
    matchLabels:
      app: qdrant
  template:
    metadata:
      labels:
        app: qdrant
    spec:
      containers:
      - name: qdrant
        image: qdrant/qdrant:v1.8.1
        
        env:
        - name: QDRANT__CLUSTER__ENABLED
          value: "true"
        - name: QDRANT__CLUSTER__P2P__PORT
          value: "6335"
        - name: QDRANT__SERVICE__GRPC_PORT
          value: "6334"
        - name: QDRANT__SERVICE__HTTP_PORT
          value: "6333"
        - name: QDRANT__STORAGE__STORAGE_PATH
          value: "/qdrant/storage"
        - name: QDRANT__STORAGE__SNAPSHOTS_PATH
          value: "/qdrant/snapshots"
        
        # 性能配置
        - name: QDRANT__STORAGE__ON_DISK_PAYLOAD
          value: "true"
        - name: QDRANT__STORAGE__OPTIMIZERS__INDEXING_THRESHOLD
          value: "20000"
        - name: QDRANT__STORAGE__PERFORMANCE__MAX_SEARCH_THREADS
          value: "0"  # 使用所有CPU
        
        # API Key认证
        - name: QDRANT__SERVICE__API_KEY
          valueFrom:
            secretKeyRef:
              name: qdrant-secrets
              key: api-key
        
        ports:
        - containerPort: 6333
          name: http
        - containerPort: 6334
          name: grpc
        - containerPort: 6335
          name: p2p
        
        resources:
          requests:
            cpu: "4"
            memory: "16Gi"
          limits:
            cpu: "8"
            memory: "32Gi"
        
        volumeMounts:
        - name: storage
          mountPath: /qdrant/storage
        - name: snapshots
          mountPath: /qdrant/snapshots
        
        livenessProbe:
          httpGet:
            path: /
            port: 6333
          initialDelaySeconds: 30
          periodSeconds: 30
        
        readinessProbe:
          httpGet:
            path: /readyz
            port: 6333
          initialDelaySeconds: 10
          periodSeconds: 10
  
  volumeClaimTemplates:
  - metadata:
      name: storage
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 500Gi
  - metadata:
      name: snapshots
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: standard
      resources:
        requests:
          storage: 100Gi
---
apiVersion: v1
kind: Service
metadata:
  name: qdrant
  namespace: qdrant
spec:
  type: LoadBalancer
  ports:
  - port: 6333
    targetPort: 6333
    name: http
  - port: 6334
    targetPort: 6334
    name: grpc
  selector:
    app: qdrant
```

### 4.2 Qdrant Python客户端

```python
# qdrant_client.py - Qdrant操作
from qdrant_client import QdrantClient, models
from typing import List, Dict, Optional
import uuid

class QdrantManager:
    """Qdrant管理器"""
    
    def __init__(
        self,
        url: str = "http://qdrant.qdrant:6333",
        api_key: str = None
    ):
        self.client = QdrantClient(
            url=url,
            api_key=api_key,
            prefer_grpc=True
        )
    
    def create_collection(
        self,
        name: str,
        vector_size: int = 1024,
        distance: str = "Cosine",
        on_disk: bool = False,
        quantization: str = None,  # "scalar", "product", "binary"
        replication_factor: int = 2,
        shard_number: int = 4
    ):
        """创建Collection"""
        
        # 量化配置
        quantization_config = None
        if quantization == "scalar":
            quantization_config = models.ScalarQuantization(
                scalar=models.ScalarQuantizationConfig(
                    type=models.ScalarType.INT8,
                    quantile=0.99,
                    always_ram=True
                )
            )
        elif quantization == "product":
            quantization_config = models.ProductQuantization(
                product=models.ProductQuantizationConfig(
                    compression=models.CompressionRatio.X16,
                    always_ram=True
                )
            )
        elif quantization == "binary":
            quantization_config = models.BinaryQuantization(
                binary=models.BinaryQuantizationConfig(
                    always_ram=True
                )
            )
        
        # 创建Collection
        self.client.create_collection(
            collection_name=name,
            vectors_config=models.VectorParams(
                size=vector_size,
                distance=getattr(models.Distance, distance.upper()),
                on_disk=on_disk
            ),
            hnsw_config=models.HnswConfigDiff(
                m=16,
                ef_construct=256,
                full_scan_threshold=10000,
                on_disk=on_disk
            ),
            optimizers_config=models.OptimizersConfigDiff(
                indexing_threshold=20000,
                memmap_threshold=50000
            ),
            quantization_config=quantization_config,
            replication_factor=replication_factor,
            shard_number=shard_number
        )
        
        # 创建payload索引
        self.client.create_payload_index(
            collection_name=name,
            field_name="source",
            field_schema=models.PayloadSchemaType.KEYWORD
        )
        self.client.create_payload_index(
            collection_name=name,
            field_name="created_at",
            field_schema=models.PayloadSchemaType.INTEGER
        )
        
        return True
    
    def upsert(
        self,
        collection_name: str,
        vectors: List[List[float]],
        payloads: List[Dict],
        ids: List[str] = None,
        batch_size: int = 100
    ):
        """批量更新/插入"""
        
        if ids is None:
            ids = [str(uuid.uuid4()) for _ in range(len(vectors))]
        
        points = [
            models.PointStruct(
                id=ids[i],
                vector=vectors[i],
                payload=payloads[i]
            )
            for i in range(len(vectors))
        ]
        
        # 分批上传
        for i in range(0, len(points), batch_size):
            batch = points[i:i + batch_size]
            self.client.upsert(
                collection_name=collection_name,
                points=batch,
                wait=True
            )
        
        return len(points)
    
    def search(
        self,
        collection_name: str,
        query_vector: List[float],
        top_k: int = 10,
        filter_conditions: Dict = None,
        score_threshold: float = None
    ):
        """向量搜索"""
        
        # 构建过滤器
        query_filter = None
        if filter_conditions:
            must_conditions = []
            for key, value in filter_conditions.items():
                if isinstance(value, dict):
                    if "gte" in value:
                        must_conditions.append(
                            models.FieldCondition(
                                key=key,
                                range=models.Range(gte=value["gte"])
                            )
                        )
                    if "lte" in value:
                        must_conditions.append(
                            models.FieldCondition(
                                key=key,
                                range=models.Range(lte=value["lte"])
                            )
                        )
                else:
                    must_conditions.append(
                        models.FieldCondition(
                            key=key,
                            match=models.MatchValue(value=value)
                        )
                    )
            
            if must_conditions:
                query_filter = models.Filter(must=must_conditions)
        
        results = self.client.search(
            collection_name=collection_name,
            query_vector=query_vector,
            limit=top_k,
            query_filter=query_filter,
            score_threshold=score_threshold,
            with_payload=True,
            with_vectors=False
        )
        
        return [
            {
                "id": str(hit.id),
                "score": hit.score,
                "payload": hit.payload
            }
            for hit in results
        ]
    
    def recommend(
        self,
        collection_name: str,
        positive_ids: List[str],
        negative_ids: List[str] = None,
        top_k: int = 10,
        filter_conditions: Dict = None
    ):
        """推荐搜索 (基于正负样本)"""
        
        results = self.client.recommend(
            collection_name=collection_name,
            positive=positive_ids,
            negative=negative_ids or [],
            limit=top_k,
            with_payload=True
        )
        
        return [
            {
                "id": str(hit.id),
                "score": hit.score,
                "payload": hit.payload
            }
            for hit in results
        ]
```

---

## 五、RAG Pipeline实现

### 5.1 完整RAG Pipeline

```python
# rag_pipeline.py - 生产级RAG Pipeline
from typing import List, Dict, Optional, AsyncIterator
import asyncio
from dataclasses import dataclass
from enum import Enum
import hashlib
import json

from langchain.text_splitter import RecursiveCharacterTextSplitter
from langchain.embeddings import HuggingFaceEmbeddings
from sentence_transformers import CrossEncoder
import tiktoken

class SearchStrategy(Enum):
    """搜索策略"""
    VECTOR = "vector"
    HYBRID = "hybrid"
    MULTI_QUERY = "multi_query"
    HYDE = "hyde"

@dataclass
class Document:
    """文档对象"""
    id: str
    content: str
    metadata: Dict
    embedding: Optional[List[float]] = None
    score: float = 0.0

@dataclass
class RAGResponse:
    """RAG响应"""
    answer: str
    sources: List[Document]
    confidence: float
    tokens_used: int

class RAGPipeline:
    """生产级RAG Pipeline"""
    
    def __init__(
        self,
        vector_db,  # Milvus/Weaviate/Qdrant客户端
        llm_client,  # vLLM/TGI客户端
        embedding_model: str = "BAAI/bge-large-zh-v1.5",
        reranker_model: str = "BAAI/bge-reranker-large",
        chunk_size: int = 512,
        chunk_overlap: int = 50,
    ):
        self.vector_db = vector_db
        self.llm_client = llm_client
        
        # 初始化Embedding模型
        self.embedding_model = HuggingFaceEmbeddings(
            model_name=embedding_model,
            model_kwargs={'device': 'cuda'},
            encode_kwargs={'normalize_embeddings': True}
        )
        
        # 初始化Reranker
        self.reranker = CrossEncoder(
            reranker_model,
            device='cuda'
        )
        
        # 文本分割器
        self.text_splitter = RecursiveCharacterTextSplitter(
            chunk_size=chunk_size,
            chunk_overlap=chunk_overlap,
            separators=["\n\n", "\n", "。", "！", "？", ".", "!", "?", " "]
        )
        
        # Token计数器
        self.tokenizer = tiktoken.get_encoding("cl100k_base")
    
    async def ingest_documents(
        self,
        documents: List[Dict],
        collection_name: str,
        batch_size: int = 100
    ):
        """文档摄入"""
        
        all_chunks = []
        
        for doc in documents:
            # 分块
            chunks = self.text_splitter.split_text(doc["content"])
            
            for i, chunk in enumerate(chunks):
                chunk_id = hashlib.md5(
                    f"{doc['id']}_{i}".encode()
                ).hexdigest()
                
                all_chunks.append({
                    "id": chunk_id,
                    "content": chunk,
                    "metadata": {
                        **doc.get("metadata", {}),
                        "source_id": doc["id"],
                        "chunk_index": i,
                        "total_chunks": len(chunks)
                    }
                })
        
        # 批量生成Embedding
        embeddings = await self._batch_embed(
            [c["content"] for c in all_chunks],
            batch_size
        )
        
        # 添加embedding到chunks
        for i, chunk in enumerate(all_chunks):
            chunk["embedding"] = embeddings[i]
        
        # 插入向量数据库
        await self._batch_insert(collection_name, all_chunks, batch_size)
        
        return len(all_chunks)
    
    async def query(
        self,
        question: str,
        collection_name: str,
        strategy: SearchStrategy = SearchStrategy.HYBRID,
        top_k: int = 10,
        rerank_top_k: int = 5,
        filter_conditions: Dict = None,
        stream: bool = False
    ) -> RAGResponse:
        """RAG查询"""
        
        # Step 1: 查询改写/扩展
        enhanced_queries = await self._enhance_query(question, strategy)
        
        # Step 2: 检索
        candidates = await self._retrieve(
            enhanced_queries,
            collection_name,
            top_k,
            filter_conditions,
            strategy
        )
        
        # Step 3: 重排序
        reranked_docs = await self._rerank(
            question,
            candidates,
            rerank_top_k
        )
        
        # Step 4: 构建上下文
        context = self._build_context(reranked_docs)
        
        # Step 5: 生成答案
        if stream:
            return self._generate_stream(question, context, reranked_docs)
        else:
            answer, tokens = await self._generate(question, context)
            
            # 计算置信度
            confidence = self._calculate_confidence(reranked_docs)
            
            return RAGResponse(
                answer=answer,
                sources=reranked_docs,
                confidence=confidence,
                tokens_used=tokens
            )
    
    async def _enhance_query(
        self,
        question: str,
        strategy: SearchStrategy
    ) -> List[str]:
        """查询增强"""
        
        if strategy == SearchStrategy.MULTI_QUERY:
            # 生成多个查询变体
            prompt = f"""Given the question: "{question}"
            
Generate 3 different search queries that would help find relevant information.
Output only the queries, one per line."""
            
            response = await self.llm_client.generate(prompt, max_tokens=200)
            queries = [question] + response.strip().split("\n")
            return queries[:4]
        
        elif strategy == SearchStrategy.HYDE:
            # 生成假设性答案
            prompt = f"""Question: {question}

Write a detailed answer to this question as if you were an expert. 
This will be used to search for similar content."""
            
            hypothetical_answer = await self.llm_client.generate(
                prompt, max_tokens=300
            )
            return [question, hypothetical_answer]
        
        return [question]
    
    async def _retrieve(
        self,
        queries: List[str],
        collection_name: str,
        top_k: int,
        filter_conditions: Dict,
        strategy: SearchStrategy
    ) -> List[Document]:
        """检索文档"""
        
        all_results = {}
        
        for query in queries:
            # 生成查询向量
            query_embedding = self.embedding_model.embed_query(query)
            
            if strategy == SearchStrategy.HYBRID:
                # 混合搜索
                results = await self.vector_db.hybrid_search(
                    collection_name,
                    query_embedding,
                    query,
                    top_k,
                    filter_conditions
                )
            else:
                # 纯向量搜索
                results = await self.vector_db.search(
                    collection_name,
                    query_embedding,
                    top_k,
                    filter_conditions
                )
            
            # 合并结果,保留最高分
            for r in results:
                doc_id = r["id"]
                if doc_id not in all_results or r["score"] > all_results[doc_id].score:
                    all_results[doc_id] = Document(
                        id=doc_id,
                        content=r["content"],
                        metadata=r.get("metadata", {}),
                        score=r["score"]
                    )
        
        # 按分数排序
        sorted_results = sorted(
            all_results.values(),
            key=lambda x: x.score,
            reverse=True
        )
        
        return sorted_results[:top_k * 2]  # 返回更多给reranker
    
    async def _rerank(
        self,
        question: str,
        documents: List[Document],
        top_k: int
    ) -> List[Document]:
        """重排序"""
        
        if not documents:
            return []
        
        # 准备输入对
        pairs = [(question, doc.content) for doc in documents]
        
        # 计算重排序分数
        scores = self.reranker.predict(pairs)
        
        # 更新分数
        for i, doc in enumerate(documents):
            doc.score = float(scores[i])
        
        # 排序并返回top_k
        reranked = sorted(documents, key=lambda x: x.score, reverse=True)
        return reranked[:top_k]
    
    def _build_context(
        self,
        documents: List[Document],
        max_tokens: int = 4000
    ) -> str:
        """构建上下文"""
        
        context_parts = []
        total_tokens = 0
        
        for i, doc in enumerate(documents):
            doc_text = f"[{i+1}] {doc.content}"
            doc_tokens = len(self.tokenizer.encode(doc_text))
            
            if total_tokens + doc_tokens > max_tokens:
                break
            
            context_parts.append(doc_text)
            total_tokens += doc_tokens
        
        return "\n\n".join(context_parts)
    
    async def _generate(
        self,
        question: str,
        context: str
    ) -> tuple:
        """生成答案"""
        
        prompt = f"""Based on the following context, answer the question accurately and concisely.
If the context doesn't contain enough information, say so.
Always cite the source numbers [1], [2], etc. when using information from the context.

Context:
{context}

Question: {question}

Answer:"""
        
        response = await self.llm_client.generate(
            prompt,
            max_tokens=1024,
            temperature=0.1
        )
        
        tokens = len(self.tokenizer.encode(prompt + response))
        
        return response, tokens
    
    async def _generate_stream(
        self,
        question: str,
        context: str,
        sources: List[Document]
    ) -> AsyncIterator[str]:
        """流式生成"""
        
        prompt = f"""Based on the following context, answer the question.

Context:
{context}

Question: {question}

Answer:"""
        
        async for chunk in self.llm_client.generate_stream(
            prompt,
            max_tokens=1024,
            temperature=0.1
        ):
            yield chunk
    
    def _calculate_confidence(self, documents: List[Document]) -> float:
        """计算置信度"""
        
        if not documents:
            return 0.0
        
        # 基于重排序分数计算
        avg_score = sum(doc.score for doc in documents) / len(documents)
        top_score = documents[0].score if documents else 0
        
        # 综合置信度
        confidence = (avg_score * 0.4 + top_score * 0.6)
        return min(max(confidence, 0.0), 1.0)
    
    async def _batch_embed(
        self,
        texts: List[str],
        batch_size: int
    ) -> List[List[float]]:
        """批量生成Embedding"""
        
        embeddings = []
        for i in range(0, len(texts), batch_size):
            batch = texts[i:i + batch_size]
            batch_embeddings = self.embedding_model.embed_documents(batch)
            embeddings.extend(batch_embeddings)
        
        return embeddings
    
    async def _batch_insert(
        self,
        collection_name: str,
        chunks: List[Dict],
        batch_size: int
    ):
        """批量插入"""
        
        for i in range(0, len(chunks), batch_size):
            batch = chunks[i:i + batch_size]
            await self.vector_db.insert(collection_name, batch)
```

### 5.2 高级检索策略

```python
# advanced_retrieval.py - 高级检索策略
from typing import List, Dict, Optional
import numpy as np
from collections import defaultdict

class AdvancedRetriever:
    """高级检索器"""
    
    def __init__(self, vector_db, embedding_model, llm_client):
        self.vector_db = vector_db
        self.embedding_model = embedding_model
        self.llm_client = llm_client
    
    async def parent_document_retrieval(
        self,
        query: str,
        collection_name: str,
        top_k: int = 5
    ) -> List[Dict]:
        """父文档检索 - 检索小块,返回大块"""
        
        # 搜索小块
        query_embedding = self.embedding_model.embed_query(query)
        small_chunks = await self.vector_db.search(
            collection_name + "_small",
            query_embedding,
            top_k * 3
        )
        
        # 获取对应的父文档ID
        parent_ids = set()
        for chunk in small_chunks:
            parent_id = chunk["metadata"].get("parent_id")
            if parent_id:
                parent_ids.add(parent_id)
        
        # 检索父文档
        parent_docs = await self.vector_db.get_by_ids(
            collection_name + "_large",
            list(parent_ids)[:top_k]
        )
        
        return parent_docs
    
    async def self_query_retrieval(
        self,
        query: str,
        collection_name: str,
        top_k: int = 5
    ) -> List[Dict]:
        """自查询检索 - LLM解析查询生成过滤条件"""
        
        # 使用LLM解析查询
        parse_prompt = f"""Parse the following query into search parameters.

Query: "{query}"

Output JSON with:
- search_query: the main search text
- filters: metadata filters (e.g., date, category, author)

Example output:
{{"search_query": "machine learning", "filters": {{"category": "technology", "year_gte": 2023}}}}

Output:"""
        
        parsed = await self.llm_client.generate(parse_prompt, max_tokens=200)
        
        try:
            params = json.loads(parsed)
            search_query = params.get("search_query", query)
            filters = params.get("filters", {})
        except:
            search_query = query
            filters = {}
        
        # 执行带过滤的搜索
        query_embedding = self.embedding_model.embed_query(search_query)
        results = await self.vector_db.search(
            collection_name,
            query_embedding,
            top_k,
            filter_conditions=filters
        )
        
        return results
    
    async def contextual_compression(
        self,
        query: str,
        documents: List[Dict],
        max_length: int = 500
    ) -> List[Dict]:
        """上下文压缩 - 提取与查询相关的部分"""
        
        compressed = []
        
        for doc in documents:
            prompt = f"""Extract only the parts of the following text that are relevant to the query.
If no parts are relevant, output "NOT_RELEVANT".

Query: {query}

Text: {doc["content"]}

Relevant extract:"""
            
            extract = await self.llm_client.generate(
                prompt,
                max_tokens=max_length
            )
            
            if extract.strip() != "NOT_RELEVANT":
                compressed.append({
                    **doc,
                    "content": extract,
                    "original_content": doc["content"]
                })
        
        return compressed
    
    async def ensemble_retrieval(
        self,
        query: str,
        collection_name: str,
        top_k: int = 10,
        weights: Dict[str, float] = None
    ) -> List[Dict]:
        """集成检索 - 多策略融合"""
        
        if weights is None:
            weights = {
                "vector": 0.4,
                "bm25": 0.3,
                "multi_query": 0.3
            }
        
        all_results = defaultdict(lambda: {"score": 0, "doc": None})
        
        # 向量检索
        query_embedding = self.embedding_model.embed_query(query)
        vector_results = await self.vector_db.search(
            collection_name,
            query_embedding,
            top_k * 2
        )
        
        for r in vector_results:
            all_results[r["id"]]["score"] += r["score"] * weights["vector"]
            all_results[r["id"]]["doc"] = r
        
        # BM25检索 (如果支持)
        if hasattr(self.vector_db, "bm25_search"):
            bm25_results = await self.vector_db.bm25_search(
                collection_name,
                query,
                top_k * 2
            )
            
            for r in bm25_results:
                all_results[r["id"]]["score"] += r["score"] * weights["bm25"]
                if all_results[r["id"]]["doc"] is None:
                    all_results[r["id"]]["doc"] = r
        
        # 多查询检索
        multi_queries = await self._generate_multi_queries(query)
        for mq in multi_queries:
            mq_embedding = self.embedding_model.embed_query(mq)
            mq_results = await self.vector_db.search(
                collection_name,
                mq_embedding,
                top_k
            )
            
            for r in mq_results:
                all_results[r["id"]]["score"] += (
                    r["score"] * weights["multi_query"] / len(multi_queries)
                )
                if all_results[r["id"]]["doc"] is None:
                    all_results[r["id"]]["doc"] = r
        
        # 合并排序
        sorted_results = sorted(
            [{"id": k, "score": v["score"], **v["doc"]} 
             for k, v in all_results.items() if v["doc"]],
            key=lambda x: x["score"],
            reverse=True
        )
        
        return sorted_results[:top_k]
    
    async def mmr_retrieval(
        self,
        query: str,
        collection_name: str,
        top_k: int = 10,
        lambda_mult: float = 0.5
    ) -> List[Dict]:
        """MMR (Maximal Marginal Relevance) - 多样性检索"""
        
        # 获取更多候选
        query_embedding = self.embedding_model.embed_query(query)
        candidates = await self.vector_db.search(
            collection_name,
            query_embedding,
            top_k * 4
        )
        
        if not candidates:
            return []
        
        # 获取候选向量
        candidate_embeddings = np.array([
            c.get("embedding", self.embedding_model.embed_query(c["content"]))
            for c in candidates
        ])
        query_embedding = np.array(query_embedding)
        
        # MMR选择
        selected = []
        selected_indices = set()
        
        for _ in range(min(top_k, len(candidates))):
            best_score = -float("inf")
            best_idx = -1
            
            for i, candidate in enumerate(candidates):
                if i in selected_indices:
                    continue
                
                # 相关性分数
                relevance = np.dot(query_embedding, candidate_embeddings[i])
                
                # 多样性分数
                diversity = 0
                if selected:
                    similarities = [
                        np.dot(candidate_embeddings[i], candidate_embeddings[j])
                        for j in selected_indices
                    ]
                    diversity = max(similarities)
                
                # MMR分数
                mmr_score = lambda_mult * relevance - (1 - lambda_mult) * diversity
                
                if mmr_score > best_score:
                    best_score = mmr_score
                    best_idx = i
            
            if best_idx >= 0:
                selected.append(candidates[best_idx])
                selected_indices.add(best_idx)
        
        return selected
    
    async def _generate_multi_queries(self, query: str) -> List[str]:
        """生成多个查询变体"""
        
        prompt = f"""Generate 3 different ways to ask this question:
"{query}"

Output only the questions, one per line."""
        
        response = await self.llm_client.generate(prompt, max_tokens=200)
        queries = response.strip().split("\n")
        return [q.strip() for q in queries if q.strip()][:3]
```

---

## 六、Embedding服务部署

### 6.1 TEI (Text Embeddings Inference) 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: tei-bge-large
  namespace: rag
spec:
  replicas: 4
  selector:
    matchLabels:
      app: tei
  template:
    metadata:
      labels:
        app: tei
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "80"
    spec:
      containers:
      - name: tei
        image: ghcr.io/huggingface/text-embeddings-inference:1.2
        
        args:
        - --model-id=BAAI/bge-large-zh-v1.5
        - --port=80
        - --max-concurrent-requests=512
        - --max-batch-tokens=16384
        - --max-client-batch-size=32
        
        env:
        - name: HUGGING_FACE_HUB_TOKEN
          valueFrom:
            secretKeyRef:
              name: hf-secrets
              key: token
        
        ports:
        - containerPort: 80
          name: http
        
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
            nvidia.com/gpu: "1"
          limits:
            cpu: "4"
            memory: "8Gi"
            nvidia.com/gpu: "1"
        
        livenessProbe:
          httpGet:
            path: /health
            port: 80
          initialDelaySeconds: 60
          periodSeconds: 30
        
        readinessProbe:
          httpGet:
            path: /health
            port: 80
          initialDelaySeconds: 30
          periodSeconds: 10
---
apiVersion: v1
kind: Service
metadata:
  name: tei-service
  namespace: rag
spec:
  ports:
  - port: 80
    targetPort: 80
  selector:
    app: tei
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: tei-hpa
  namespace: rag
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: tei-bge-large
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "100"
```

### 6.2 Reranker服务部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: reranker-service
  namespace: rag
spec:
  replicas: 2
  selector:
    matchLabels:
      app: reranker
  template:
    metadata:
      labels:
        app: reranker
    spec:
      containers:
      - name: reranker
        image: ghcr.io/huggingface/text-embeddings-inference:1.2
        
        args:
        - --model-id=BAAI/bge-reranker-v2-m3
        - --port=80
        - --max-concurrent-requests=128
        
        resources:
          requests:
            nvidia.com/gpu: "1"
          limits:
            nvidia.com/gpu: "1"
        
        ports:
        - containerPort: 80
---
apiVersion: v1
kind: Service
metadata:
  name: reranker-service
  namespace: rag
spec:
  ports:
  - port: 80
    targetPort: 80
  selector:
    app: reranker
```

---

## 七、监控与可观测性

### 7.1 RAG系统监控

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: rag-system-alerts
  namespace: rag
spec:
  groups:
  - name: rag-quality
    rules:
    # 检索质量指标
    - record: rag:retrieval_recall
      expr: |
        sum(rag_retrieval_relevant_docs) / sum(rag_retrieval_total_relevant)
    
    - record: rag:retrieval_precision
      expr: |
        sum(rag_retrieval_relevant_docs) / sum(rag_retrieval_returned_docs)
    
    # 响应质量告警
    - alert: RAGLowRetrievalRecall
      expr: rag:retrieval_recall < 0.7
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "RAG检索召回率低"
        description: "召回率 {{ $value | humanizePercentage }}"
    
    # 延迟告警
    - alert: RAGHighLatency
      expr: |
        histogram_quantile(0.99, sum(rate(rag_query_duration_seconds_bucket[5m])) by (le)) > 5
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "RAG查询延迟过高"
        description: "P99延迟 {{ $value }}秒"
    
    # Embedding服务告警
    - alert: EmbeddingServiceDown
      expr: up{job="tei-service"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Embedding服务不可用"
  
  - name: rag-performance
    rules:
    # QPS
    - record: rag:query_qps
      expr: sum(rate(rag_query_total[5m]))
    
    # 延迟分位数
    - record: rag:query_latency_p50
      expr: |
        histogram_quantile(0.50, sum(rate(rag_query_duration_seconds_bucket[5m])) by (le))
    
    - record: rag:query_latency_p95
      expr: |
        histogram_quantile(0.95, sum(rate(rag_query_duration_seconds_bucket[5m])) by (le))
    
    # Token使用
    - record: rag:tokens_per_query
      expr: |
        sum(rate(rag_tokens_used_total[5m])) / sum(rate(rag_query_total[5m]))
```

### 7.2 RAG评估指标

```python
# rag_evaluation.py - RAG评估
from typing import List, Dict
import numpy as np
from dataclasses import dataclass

@dataclass
class RAGMetrics:
    """RAG评估指标"""
    # 检索指标
    recall_at_k: float
    precision_at_k: float
    mrr: float  # Mean Reciprocal Rank
    ndcg: float  # Normalized Discounted Cumulative Gain
    
    # 生成指标
    faithfulness: float  # 答案是否基于上下文
    relevance: float  # 答案与问题的相关性
    coherence: float  # 答案的连贯性
    
    # 系统指标
    latency_p50: float
    latency_p99: float
    tokens_used: int

class RAGEvaluator:
    """RAG评估器"""
    
    def __init__(self, llm_client):
        self.llm_client = llm_client
    
    def evaluate_retrieval(
        self,
        retrieved_docs: List[str],
        relevant_docs: List[str],
        k: int = 5
    ) -> Dict[str, float]:
        """评估检索质量"""
        
        retrieved_set = set(retrieved_docs[:k])
        relevant_set = set(relevant_docs)
        
        # Recall@K
        recall = len(retrieved_set & relevant_set) / len(relevant_set) if relevant_set else 0
        
        # Precision@K
        precision = len(retrieved_set & relevant_set) / k
        
        # MRR
        mrr = 0
        for i, doc in enumerate(retrieved_docs):
            if doc in relevant_set:
                mrr = 1 / (i + 1)
                break
        
        # NDCG
        dcg = sum(
            (1 if doc in relevant_set else 0) / np.log2(i + 2)
            for i, doc in enumerate(retrieved_docs[:k])
        )
        idcg = sum(1 / np.log2(i + 2) for i in range(min(k, len(relevant_set))))
        ndcg = dcg / idcg if idcg > 0 else 0
        
        return {
            "recall_at_k": recall,
            "precision_at_k": precision,
            "mrr": mrr,
            "ndcg": ndcg
        }
    
    async def evaluate_generation(
        self,
        question: str,
        answer: str,
        context: str
    ) -> Dict[str, float]:
        """评估生成质量"""
        
        # 忠实度评估
        faithfulness_prompt = f"""Rate how faithful the answer is to the provided context.
Score from 0-1, where 1 means the answer only contains information from the context.

Context: {context}
Answer: {answer}

Score (0-1):"""
        
        faithfulness = await self._get_score(faithfulness_prompt)
        
        # 相关性评估
        relevance_prompt = f"""Rate how relevant the answer is to the question.
Score from 0-1, where 1 means the answer directly addresses the question.

Question: {question}
Answer: {answer}

Score (0-1):"""
        
        relevance = await self._get_score(relevance_prompt)
        
        # 连贯性评估
        coherence_prompt = f"""Rate the coherence and clarity of the answer.
Score from 0-1, where 1 means the answer is clear, well-structured, and easy to understand.

Answer: {answer}

Score (0-1):"""
        
        coherence = await self._get_score(coherence_prompt)
        
        return {
            "faithfulness": faithfulness,
            "relevance": relevance,
            "coherence": coherence
        }
    
    async def _get_score(self, prompt: str) -> float:
        """获取评分"""
        response = await self.llm_client.generate(prompt, max_tokens=10)
        try:
            score = float(response.strip())
            return min(max(score, 0), 1)
        except:
            return 0.5
```

---

## 八、性能优化

### 8.1 性能优化策略

| 优化项 | 方法 | 效果 | 适用场景 |
|--------|------|------|---------|
| **索引优化** | HNSW参数调优 (M=32, ef=256) | 召回率+5%, 延迟+10% | 高精度需求 |
| **量化索引** | Scalar/Product Quantization | 内存-50%, 延迟+20% | 大规模数据 |
| **批量请求** | Batch embedding/search | 吞吐量+5x | 批处理场景 |
| **缓存** | Redis缓存热门查询 | 命中率30%+, 延迟-80% | 重复查询多 |
| **预计算** | 预热索引到内存 | 首次延迟-90% | 冷启动优化 |
| **分片** | 按时间/主题分片 | 单分片查询延迟-50% | 数据量大 |
| **GPU加速** | GPU索引构建/搜索 | 吞吐量+10x | 高并发 |

### 8.2 缓存策略

```python
# rag_cache.py - RAG缓存
import redis
import hashlib
import json
from typing import Optional, List, Dict

class RAGCache:
    """RAG缓存管理"""
    
    def __init__(
        self,
        redis_url: str = "redis://redis:6379",
        embedding_ttl: int = 86400,  # 1天
        result_ttl: int = 3600,  # 1小时
    ):
        self.redis = redis.from_url(redis_url)
        self.embedding_ttl = embedding_ttl
        self.result_ttl = result_ttl
    
    def _hash_key(self, text: str) -> str:
        """生成缓存key"""
        return hashlib.md5(text.encode()).hexdigest()
    
    async def get_embedding(self, text: str) -> Optional[List[float]]:
        """获取缓存的embedding"""
        key = f"emb:{self._hash_key(text)}"
        cached = self.redis.get(key)
        if cached:
            return json.loads(cached)
        return None
    
    async def set_embedding(self, text: str, embedding: List[float]):
        """缓存embedding"""
        key = f"emb:{self._hash_key(text)}"
        self.redis.setex(key, self.embedding_ttl, json.dumps(embedding))
    
    async def get_search_result(
        self,
        query: str,
        collection: str,
        top_k: int
    ) -> Optional[List[Dict]]:
        """获取缓存的搜索结果"""
        key = f"search:{collection}:{top_k}:{self._hash_key(query)}"
        cached = self.redis.get(key)
        if cached:
            return json.loads(cached)
        return None
    
    async def set_search_result(
        self,
        query: str,
        collection: str,
        top_k: int,
        results: List[Dict]
    ):
        """缓存搜索结果"""
        key = f"search:{collection}:{top_k}:{self._hash_key(query)}"
        self.redis.setex(key, self.result_ttl, json.dumps(results))
    
    async def get_rag_response(
        self,
        query: str,
        collection: str
    ) -> Optional[Dict]:
        """获取缓存的RAG响应"""
        key = f"rag:{collection}:{self._hash_key(query)}"
        cached = self.redis.get(key)
        if cached:
            return json.loads(cached)
        return None
    
    async def set_rag_response(
        self,
        query: str,
        collection: str,
        response: Dict
    ):
        """缓存RAG响应"""
        key = f"rag:{collection}:{self._hash_key(query)}"
        self.redis.setex(key, self.result_ttl, json.dumps(response))
    
    def invalidate_collection(self, collection: str):
        """清除某个collection的缓存"""
        pattern = f"*:{collection}:*"
        keys = self.redis.keys(pattern)
        if keys:
            self.redis.delete(*keys)
```

---

## 九、快速参考

### 9.1 向量数据库选择

| 需求 | 推荐方案 | 理由 |
|-----|---------|------|
| 大规模生产 (>1亿向量) | Milvus | 分布式,GPU加速,高可用 |
| 企业级功能 | Weaviate | GraphQL,模块化,易集成 |
| 高性能Rust | Qdrant | 低延迟,高吞吐,轻量 |
| 零运维 | Pinecone | 全托管,免维护 |
| 快速原型 | Chroma | 简单,本地运行 |
| 现有PG用户 | pgvector | 无需新基础设施 |

### 9.2 Embedding模型选择

| 场景 | 推荐模型 | 维度 | 说明 |
|-----|---------|------|------|
| 通用英文 | text-embedding-3-small | 1536 | OpenAI,高质量 |
| 通用中文 | BGE-large-zh-v1.5 | 1024 | 开源最佳 |
| 多语言 | Cohere embed-v3 | 1024 | 100+语言 |
| 长文本 | Jina-embeddings-v2 | 768 | 8K上下文 |
| 成本敏感 | E5-small-v2 | 384 | 小模型 |

### 9.3 常用API

```python
# Milvus
from pymilvus import Collection
collection = Collection("my_collection")
results = collection.search(vectors, "embedding", {"metric_type": "COSINE"}, limit=10)

# Weaviate
client.query.get("Document", ["content"]).with_near_text({"concepts": ["query"]}).with_limit(10).do()

# Qdrant
client.search(collection_name="my_collection", query_vector=vector, limit=10)

# LangChain
from langchain.vectorstores import Milvus
vectorstore = Milvus.from_documents(docs, embedding, connection_args={"host": "localhost"})
results = vectorstore.similarity_search(query, k=10)
```

---

## 十、最佳实践

### RAG系统检查清单

- [ ] **数据准备**: 清洗、分块、去重
- [ ] **Embedding选择**: 根据语言和场景选择模型
- [ ] **向量数据库**: 根据规模和需求选择
- [ ] **索引配置**: HNSW参数调优
- [ ] **检索策略**: 混合搜索、重排序
- [ ] **提示工程**: 优化RAG提示模板
- [ ] **缓存策略**: 热门查询缓存
- [ ] **监控告警**: 延迟、召回率、错误率
- [ ] **评估体系**: 定期评估检索和生成质量
- [ ] **迭代优化**: 基于用户反馈持续改进

---

**相关文档**: [144-LLM推理服务](144-llm-inference-serving.md) | [142-LLM数据Pipeline](142-llm-data-pipeline.md) | [132-AI/ML工作负载](132-ai-ml-workloads.md)

**版本**: Milvus 2.3+ | Weaviate 1.24+ | Qdrant 1.8+ | LangChain 0.1+
