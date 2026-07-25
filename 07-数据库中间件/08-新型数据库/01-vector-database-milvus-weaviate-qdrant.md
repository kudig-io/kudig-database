---
title: "向量数据库 K8s 运维（Milvus/Weaviate/Qdrant/pgvector）"
description: "覆盖 Milvus、Weaviate、Qdrant、pgvector 等向量数据库在 Kubernetes 上的部署、调优与运维实践"
summary: "向量数据库架构对比（Milvus/Weaviate/Qdrant/pgvector/Chroma），Helm 集群部署，索引类型（HNSW/IVF_FLAT/IVF_PQ/DiskANN），内存规划与性能调优，备份恢复与监控告警，OOM 与查询超时故障排查"
category: 数据库中间件
tags:
- database
- vector-database
- milvus
- qdrant
- weaviate
- pgvector
- ai-infrastructure
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 应用开发者
estimated_read_time: 20min
intent_queries:
- "向量数据库如何在 K8s 上部署"
- "Milvus 集群运维最佳实践"
- "Qdrant 和 Weaviate 如何选择"
trigger_keywords:
- 向量数据库
- Milvus
- Qdrant
- Weaviate
- pgvector
- HNSW
- embedding
prerequisites:
- kubectl-basics
- database-basics
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

# 向量数据库 K8s 运维（Milvus/Weaviate/Qdrant/pgvector）

## 概述

向量数据库是 AI 基础设施的核心组件，用于存储和检索高维向量数据（Embedding），支撑语义搜索、推荐系统、RAG（Retrieval-Augmented Generation）等场景。随着大语言模型在企业中的广泛应用，向量数据库已成为 [[15-AI基础设施/]] 中不可或缺的一环。

本文覆盖主流向量数据库在 Kubernetes 上的生产级部署与运维，包括 Milvus、Weaviate、Qdrant、pgvector 和 Chroma，帮助平台工程师和 SRE 完成选型、部署、调优和故障排查。

## 架构与核心概念

### 向量数据库对比

| 特性 | Milvus | Weaviate | Qdrant | pgvector | Chroma |
|------|--------|----------|--------|----------|--------|
| 架构模式 | 分布式（存算分离） | 分布式（模块化） | 分布式（Raft 共识） | PostgreSQL 扩展 | 嵌入式/Client-Server |
| 部署复杂度 | 高（依赖 etcd/MinIO/Pulsar） | 中 | 低 | 低（复用 PG 集群） | 极低 |
| 水平扩展 | 原生支持 | 原生支持 | 原生支持 | 受限于 PG | 不支持 |
| 最大数据规模 | 十亿级 | 亿级 | 亿级 | 千万级 | 百万级 |
| 索引类型 | HNSW/IVF/DiskANN | HNSW | HNSW | HNSW/IVF | HNSW |
| 多租户 | Partition/Collection | 原生多租户 | Collection + Payload | Schema 隔离 | Collection |
| 混合查询 | 标量过滤 + 向量 | GraphQL + 向量 | Payload 过滤 + 向量 | SQL + 向量 | 元数据过滤 |
| 适用场景 | 大规模生产 AI 系统 | 企业级语义搜索 | 中小规模高性能检索 | 已有 PG 生态 | 原型/小规模 |

### Milvus 架构

Milvus 采用存算分离架构，核心组件包括：

- **Proxy**：接入层，处理客户端请求
- **Query Node**：执行向量检索
- **Data Node**：处理数据写入和日志消费
- **Index Node**：构建向量索引
- **Coord（Root/Data/Query/Index）**：协调服务
- **依赖组件**：etcd（元数据）、MinIO/S3（对象存储）、Pulsar/Kafka（消息队列）

### Qdrant 架构

Qdrant 使用 Rust 编写，架构简洁：

- **Consensus Layer**：基于 Raft 的分布式共识
- **Storage Layer**：分片（Shard）+ 副本（Replica）
- **API Layer**：REST + gRPC 接口
- **Snapshot**：内置快照机制用于备份

### 索引类型详解

| 索引类型 | 构建速度 | 查询速度 | 内存占用 | 召回率 | 适用场景 |
|---------|---------|---------|---------|--------|---------|
| HNSW | 慢 | 极快 | 高 | 95%+ | 低延迟在线查询 |
| IVF_FLAT | 中 | 快 | 中 | 90%+ | 平衡性能与内存 |
| IVF_PQ | 快 | 快 | 低 | 85%+ | 大规模数据、内存受限 |
| DiskANN | 中 | 中 | 极低 | 90%+ | 超大规模、SSD 存储 |
| FLAT (暴力搜索) | 无需构建 | 极慢 | 无额外 | 100% | 小数据集精确搜索 |

## 生产部署

### Milvus 集群部署（Helm）

Milvus 依赖组件较多，生产环境建议使用 Helm Chart 统一部署：

```yaml
# 🟡 中风险：创建 Milvus 集群及依赖组件，会占用大量集群资源
# milvus-values.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: milvus-values
  namespace: vector-db
data:
  values.yaml: |
    cluster:
      enabled: true
    etcd:
      replicaCount: 3
      persistence:
        enabled: true
        storageClass: "gp3-encrypted"
        size: 20Gi
    minio:
      mode: distributed
      replicas: 4
      persistence:
        enabled: true
        storageClass: "gp3-encrypted"
        size: 100Gi
    pulsar:
      enabled: true
      components:
        bookkeeper: true
        broker: true
        proxy: true
      bookkeeper:
        replicaCount: 3
      broker:
        replicaCount: 2
    proxy:
      replicas: 2
      resources:
        requests:
          cpu: "1"
          memory: 2Gi
        limits:
          cpu: "2"
          memory: 4Gi
    queryNode:
      replicas: 3
      resources:
        requests:
          cpu: "4"
          memory: 16Gi
        limits:
          cpu: "8"
          memory: 32Gi
    dataNode:
      replicas: 2
      resources:
        requests:
          cpu: "2"
          memory: 8Gi
    indexNode:
      replicas: 2
      resources:
        requests:
          cpu: "4"
          memory: 16Gi
```

安装命令：

```bash
# 🟡 中风险：部署 Milvus 集群到 vector-db namespace
helm repo add milvus https://zilliztech.github.io/milvus-helm/
helm repo update
kubectl create namespace vector-db
helm install milvus milvus/milvus \
  -n vector-db \
  -f milvus-values.yaml \
  --timeout 10m \
  --wait
```

### Qdrant 集群部署

```yaml
# 🟡 中风险：部署 Qdrant 集群（3 节点，含分片和副本配置）
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: qdrant
  namespace: vector-db
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
        image: qdrant/qdrant:v1.12.1
        ports:
        - containerPort: 6333
          name: http
        - containerPort: 6334
          name: grpc
        env:
        - name: QDRANT__CLUSTER__ENABLED
          value: "true"
        - name: QDRANT__CLUSTER__P2P__PORT
          value: "6335"
        - name: QDRANT__STORAGE__STORAGE_PATH
          value: /qdrant/storage
        - name: QDRANT__STORAGE__SNAPSHOTS_PATH
          value: /qdrant/snapshots
        resources:
          requests:
            cpu: "2"
            memory: 8Gi
          limits:
            cpu: "4"
            memory: 16Gi
        volumeMounts:
        - name: storage
          mountPath: /qdrant/storage
        - name: snapshots
          mountPath: /qdrant/snapshots
        livenessProbe:
          httpGet:
            path: /healthz
            port: 6333
          initialDelaySeconds: 10
          periodSeconds: 15
        readinessProbe:
          httpGet:
            path: /readyz
            port: 6333
          initialDelaySeconds: 5
          periodSeconds: 10
  volumeClaimTemplates:
  - metadata:
      name: storage
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: gp3-encrypted
      resources:
        requests:
          storage: 100Gi
  - metadata:
      name: snapshots
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: gp3-encrypted
      resources:
        requests:
          storage: 50Gi
```

### 内存规划与存储估算

向量数据库的内存需求主要由索引类型和数据规模决定：

**估算公式（HNSW 索引）：**
```
内存 ≈ 向量数 × 维度 × 4字节 × (1 + M×2/维度) + 元数据开销
示例：1000万条 768维向量，M=16
≈ 10M × 768 × 4 × (1 + 32/768) + 2GB
≈ 30.7GB × 1.04 + 2GB ≈ 34GB
```

**生产建议：**
- HNSW 索引：预留向量数据 1.5-2 倍内存
- IVF_PQ 索引：内存需求降低至 1/4-1/8
- DiskANN：内存仅需 PQ 压缩后的码本，适合超大规模
- 始终设置 resource limits 防止 OOM 影响节点稳定性

## 运维操作

### 索引构建与调优

```bash
# 🟢 低风险：查看 Milvus collection 索引状态
kubectl exec -n vector-db milvus-proxy-0 -- \
  python -c "
from pymilvus import connections, Collection, utility
connections.connect(host='localhost', port='19530')
col = Collection('my_embeddings')
print('Index state:', col.index().params)
print('Num entities:', col.num_entities)
print('Memory usage:', utility.get_query_segment_info('my_embeddings'))
"
```

### Qdrant 快照备份

```bash
# 🟡 中风险：创建 Qdrant collection 快照
curl -X POST "http://qdrant-0.qdrant-headless.vector-db.svc:6333/collections/my_collection/snapshots" \
  -H "Content-Type: application/json"

# 🟢 低风险：查看快照列表
curl "http://qdrant-0.qdrant-headless.vector-db.svc:6333/collections/my_collection/snapshots"
```

### 性能调优参数

**HNSW 索引参数调优（查询延迟 vs 召回率）：**

| 参数 | 作用 | 推荐范围 | 影响 |
|------|------|---------|------|
| M | 每层连接数 | 16-64 | 越大召回越高，内存越大 |
| efConstruction | 构建时搜索宽度 | 128-512 | 越大索引质量越高，构建越慢 |
| ef (search) | 查询时搜索宽度 | 64-256 | 越大召回越高，延迟越大 |
| nprobe (IVF) | 探测聚类数 | 16-128 | 越大召回越高，延迟越大 |

## 故障排查

### OOM（内存溢出）

```bash
# 🟢 低风险：检查向量数据库 Pod 内存使用
kubectl top pods -n vector-db -l app=milvus-querynode
kubectl describe pod -n vector-db milvus-querynode-0 | grep -A5 "Last State"

# 🟢 低风险：查看 OOM 事件
kubectl get events -n vector-db --field-selector reason=OOMKilled --sort-by='.lastTimestamp'
```

**常见原因与解决方案：**
1. **索引加载超出内存限制**：减小 collection 分片数或增加 Query Node 副本
2. **并发查询过多**：限制 `queryNode.gracefulStopTimeout` 和并发数
3. **段合并（Compaction）峰值**：调整 `dataNode.segment.syncPeriod`

### 索引构建慢

```bash
# 🟢 低风险：检查 Milvus Index Node 状态和日志
kubectl logs -n vector-db milvus-indexnode-0 --tail=100 | grep -i "build\|progress\|error"

# 🟢 低风险：查看索引构建队列
kubectl exec -n vector-db milvus-proxy-0 -- \
  python -c "
from pymilvus import connections, utility
connections.connect(host='localhost', port='19530')
print(utility.get_server_version())
"
```

### 查询超时

排查路径：
1. 检查 `ef` 参数是否过大导致搜索范围过广
2. 确认标量过滤条件是否命中索引
3. 查看 Query Node 的 CPU/内存利用率
4. 检查网络延迟（特别是跨 AZ 查询）

```bash
# 🟢 低风险：Qdrant 查询性能诊断
curl -X POST "http://qdrant-0.qdrant-headless.vector-db.svc:6333/collections/my_collection/points/search" \
  -H "Content-Type: application/json" \
  -d '{
    "vector": [0.1, 0.2, ...],
    "limit": 10,
    "params": {"hnsw_ef": 128, "exact": false},
    "with_payload": true
  }' 2>&1 | python3 -c "import sys,json; r=json.load(sys.stdin); print(f'Time: {r[\"time\"]}s')"
```

## 最佳实践

1. **资源隔离**：向量数据库使用独立 Node Pool，配合 taint/toleration 避免资源争抢
2. **存储选型**：HNSW 索引使用本地 SSD（Local PV）获得最佳性能；IVF/DiskANN 可使用网络存储
3. **渐进式扩容**：先增加副本数，再增加分片数，避免一次性大规模 rebalance
4. **索引策略**：在线查询用 HNSW（ef=128），离线批量用 IVF_FLAT，超大规模用 DiskANN
5. **备份策略**：每日全量快照 + WAL 增量，参考 [[12-可靠性/01-备份恢复/]] 制定 RPO/RTO
6. **监控集成**：将向量数据库 metrics 接入 [[09-可观测性/]] 平台，设置 P99 延迟和召回率告警
7. **数据管线**：Embedding 生成管线参考 [[07-数据库中间件/06-数据流/]] 中的流处理模式
8. **Operator 管理**：如使用 Milvus Operator，参考 [[07-数据库中间件/05-Operator管理/]] 中的 CRD 管理实践

## Related

- [[15-AI基础设施/]]
- [[07-数据库中间件/01-数据库/]]
- [[09-可观测性/]]
- [[12-可靠性/01-备份恢复/]]
- [[07-数据库中间件/06-数据流/]]
