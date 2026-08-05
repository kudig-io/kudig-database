---
title: "向量数据库 K8s 部署运维（Milvus/Qdrant/Weaviate）"
description: "Milvus、Qdrant、Weaviate 向量数据库在 Kubernetes 上的集群部署、性能调优、备份恢复与故障排查"
summary: "覆盖 Milvus（etcd/MinIO/Pulsar 依赖）、Qdrant（分片/副本）、Weaviate 的 Helm 部署，HNSW 索引内存估算，存储规划，性能调优及生产运维最佳实践"
category: AI基础设施
tags:
- vector-database
- milvus
- qdrant
- weaviate
- hnsw
- rag
- embedding
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
estimated_read_time: 20min
intent_queries:
- "Milvus 集群在 K8s 上怎么部署"
- "向量数据库 HNSW 索引需要多少内存"
- "Qdrant 和 Milvus 怎么选"
trigger_keywords:
- milvus
- qdrant
- weaviate
- vector-database
- hnsw
- embedding
- rag
prerequisites:
- kubectl-basics
- helm-basics
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

# 向量数据库 K8s 部署运维（Milvus/Qdrant/Weaviate）

## 概述

向量数据库是 RAG（Retrieval-Augmented Generation）和语义搜索系统的核心基础设施。随着 LLM 应用大规模落地，向量数据库从单机嵌入式组件演进为分布式集群系统，对 Kubernetes 上的部署运维提出了更高要求：有状态服务管理、存储性能保障、内存容量规划、水平扩展等。

本文覆盖三大主流向量数据库（Milvus、Qdrant、Weaviate）以及 pgvector 扩展的架构对比、K8s 生产部署、HNSW 索引内存估算、性能调优、备份恢复和故障排查。帮助 AI 平台团队选择并运维适合自身场景的向量存储方案。

相关页面：[[15-AI基础设施/05-K8s-AI基础设施/09-vector-database-k8s-milvus-qdrant|RAG知识库架构]]、[[15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving|GPU调度与资源管理]]、[[22-概念/02-工作负载/statefulset|K8s有状态服务运维]]、[[23-实体/07-可观测性/prometheus|Prometheus监控体系]]、[[17-系统基础/06-知识字典/storage/persistent-volume|K8s存储与PV管理]]

## 架构与核心概念

### 向量数据库对比

| 维度 | Milvus 2.x | Qdrant | Weaviate | pgvector |
|------|-----------|--------|----------|----------|
| 架构 | 分布式（计算存储分离） | 分布式（Shared-nothing） | 分布式（模块化） | PostgreSQL 扩展 |
| 部署复杂度 | 高（依赖 etcd/MinIO/Pulsar） | 低（单二进制） | 中（单二进制 + 模块） | 低（PG 扩展） |
| 索引类型 | HNSW/IVF/DiskANN/GPU | HNSW | HNSW/Flat/Dynamic | HNSW/IVFFlat |
| 标量过滤 | 支持（混合查询） | 支持（Payload filtering） | 支持（属性过滤） | 支持（SQL WHERE） |
| 水平扩展 | 原生分布式 | Raft 分片 | Raft 副本 | 受限于 PG 架构 |
| 最大数据量 | 百亿级 | 十亿级 | 十亿级 | 千万级 |
| 多租户 | Partition/Collection | Collection + Payload | 原生多租户 | Schema 隔离 |
| 适用场景 | 大规模生产 RAG | 中小规模高性能 | 模块化 AI 应用 | 已有 PG 基础设施 |
| K8s Operator | 官方 Helm Chart | 官方 Helm Chart | 官方 Helm Chart | CloudNativePG |

### Milvus 架构

Milvus 2.x 采用计算存储分离的云原生架构，组件较多：

```
Milvus 集群组件:

接入层:
  - Proxy: 请求路由、负载均衡、认证

计算层（无状态，可水平扩展）:
  - Query Node: 向量检索和标量查询
  - Data Node: 数据写入和增量日志消费
  - Index Node: 离线索引构建

存储层:
  - etcd: 元数据存储（Collection schema、Partition 信息）
  - MinIO/S3: 向量数据和索引文件持久化
  - Pulsar/Kafka: 日志流（WAL），保证数据一致性

协调层:
  - Root Coord: DDL/DCL、时间戳分配
  - Query Coord: 查询路由和负载均衡
  - Data Coord: 数据写入管理
  - Index Coord: 索引构建任务调度
```

### Qdrant 架构

Qdrant 采用 Shared-nothing 架构，每个节点独立存储部分数据：

```
Qdrant 集群组件:

单节点内部:
  - REST/gRPC API 层
  - Collection 管理
  - Segment（数据分片单元）
  - HNSW 索引（内存 + mmap）
  - WAL（Write-Ahead Log）

集群模式:
  - Raft 共识（集群元数据）
  - Shard 分布（数据分片）
  - Replica 副本（高可用）
  - 自动 Shard 迁移和再平衡
```

### HNSW 索引内存估算

HNSW（Hierarchical Navigable Small World）是向量检索最常用的索引类型，内存规划是部署的关键：

```
HNSW 内存估算公式:

单向量内存占用 = 向量维度 × 4 bytes (float32)
索引额外开销 ≈ 单向量内存 × (M × 2) / 向量维度
  其中 M 为 HNSW 连接数（默认 16）

示例（768 维，1000 万向量，M=16）:
  原始向量: 768 × 4 × 10M = 28.6 GB
  索引开销: 28.6 × (16 × 2) / 768 ≈ 1.19 GB
  总计约: 30 GB

示例（1536 维 OpenAI embedding，500 万向量，M=16）:
  原始向量: 1536 × 4 × 5M = 28.6 GB
  索引开销: 28.6 × (16 × 2) / 1536 ≈ 0.6 GB
  总计约: 30 GB（含元数据约 35 GB）

经验法则:
  - 预留向量数据大小的 1.2-1.5 倍内存
  - 使用 mmap 模式可降低内存需求（牺牲 10-30% 延迟）
  - Qdrant 支持 on_disk payload 减少内存占用
```

## 生产部署

### Milvus 集群部署

```bash
# 🟡 中风险：部署 Milvus 集群（创建多个 StatefulSet 和依赖组件）
helm repo add milvus https://zilliztech.github.io/milvus-helm/
helm repo update

# 创建 namespace 和存储类
kubectl create namespace milvus-system
```

```yaml
# 🟡 中风险：Milvus Helm values 生产配置
# milvus-values.yaml
cluster:
  enabled: true

# 计算层配置
queryNode:
  replicas: 3
  resources:
    requests:
      cpu: "8"
      memory: "32Gi"
    limits:
      cpu: "16"
      memory: "64Gi"
  extraEnv:
  - name: QUERY_NODE_LOAD_MEMORY_LIMIT_MB
    value: "57344"  # 56GB，留 8GB 给系统

dataNode:
  replicas: 2
  resources:
    requests:
      cpu: "4"
      memory: "16Gi"

indexNode:
  replicas: 2
  resources:
    requests:
      cpu: "8"
      memory: "32Gi"

proxy:
  replicas: 2
  resources:
    requests:
      cpu: "2"
      memory: "8Gi"

# 依赖组件
etcd:
  replicaCount: 3
  persistence:
    storageClass: "gp3-encrypted"
    size: 20Gi

minio:
  mode: distributed
  replicas: 4
  persistence:
    storageClass: "gp3-encrypted"
    size: 200Gi

pulsar:
  enabled: true
  bookkeeper:
    volumes:
      journal:
        size: 100Gi
      ledgers:
        size: 200Gi
  broker:
    replicaCount: 2
    resources:
      requests:
        memory: "8Gi"

# 监控
metrics:
  enabled: true
  serviceMonitor:
    enabled: true
```

```bash
# 🟡 中风险：执行 Milvus 安装
helm install milvus milvus/milvus \
  --namespace milvus-system \
  -f milvus-values.yaml \
  --timeout 600s \
  --wait

# 验证所有组件就绪
kubectl get pods -n milvus-system -o wide
kubectl get statefulsets -n milvus-system
```

### Qdrant 集群部署

```yaml
# 🟡 中风险：Qdrant Helm values 生产配置
# qdrant-values.yaml
replicaCount: 5

config:
  cluster:
    enabled: true
    p2p:
      port: 6335
    consensus:
      tick_period_ms: 100

  storage:
    performance:
      max_search_threads: 4
      max_optimization_threads: 2
    optimizers:
      memmap_threshold_kb: 204800
      indexing_threshold_kb: 20480
    mmap:
      mmap_advice: Normal
      on_disk_payload: true  # 减少内存占用

  service:
    http_port: 6333
    grpc_port: 6334

resources:
  requests:
    cpu: "8"
    memory: "48Gi"
  limits:
    cpu: "16"
    memory: "64Gi"

persistence:
  enabled: true
  storageClassName: "gp3-encrypted"
  size: 500Gi
  accessModes:
  - ReadWriteOnce

service:
  type: ClusterIP
  ports:
  - name: http
    port: 6333
  - name: grpc
    port: 6334

metrics:
  enabled: true
  serviceMonitor:
    enabled: true
```

```bash
# 🟡 中风险：安装 Qdrant 集群
helm repo add qdrant https://qdrant.github.io/qdrant-helm
helm repo update

helm install qdrant qdrant/qdrant \
  --namespace qdrant-system \
  --create-namespace \
  -f qdrant-values.yaml \
  --wait --timeout 300s

# 验证集群状态
kubectl exec -n qdrant-system qdrant-0 -- \
  curl -s http://localhost:6333/cluster | jq .result.status
```

### Weaviate 部署

```bash
# 🟡 中风险：部署 Weaviate
helm repo add weaviate https://weaviate.github.io/weaviate-helm
helm repo update

helm install weaviate weaviate/weaviate \
  --namespace weaviate-system \
  --create-namespace \
  --set replicas=3 \
  --set resources.requests.memory=32Gi \
  --set resources.requests.cpu=4 \
  --set persistence.size=200Gi \
  --set persistence.storageClass=gp3-encrypted \
  --set authentication.apikey.enabled=true \
  --set monitoring.enabled=true \
  --wait --timeout 300s
```

## 运维操作

### Milvus 运维

```bash
# 🟢 低风险：查看 Collection 状态
kubectl exec -n milvus-system deploy/milvus-proxy -- \
  python3 -c "
from pymilvus import connections, Collection, utility
connections.connect(host='localhost', port='19530')
print('Collections:', utility.list_collections())
for name in utility.list_collections():
    c = Collection(name)
    c.load()
    print(f'{name}: {c.num_entities} entities, {c.partitions} partitions')
"

# 🟢 低风险：检查 Milvus 组件健康
kubectl get pods -n milvus-system -l app.kubernetes.io/instance=milvus
kubectl top pods -n milvus-system --sort-by=memory

# 🟡 中风险：扩容 Query Node（应对查询负载增长）
helm upgrade milvus milvus/milvus \
  --namespace milvus-system \
  --reuse-values \
  --set queryNode.replicas=5

# 🔴 高风险：手动触发 Segment Compaction（可能影响查询性能）
kubectl exec -n milvus-system deploy/milvus-proxy -- \
  python3 -c "
from pymilvus import connections, Collection
connections.connect(host='localhost', port='19530')
c = Collection('my_collection')
c.compact()
c.wait_for_compaction_completed()
print('Compaction completed')
"
```

### Qdrant 运维

```bash
# 🟢 低风险：查看 Collection 和分片状态
QDRANT_POD="qdrant-0"
kubectl exec -n qdrant-system $QDRANT_POD -- \
  curl -s http://localhost:6333/collections/my_collection | jq .result

# 🟢 低风险：查看集群健康和分片分布
kubectl exec -n qdrant-system $QDRANT_POD -- \
  curl -s http://localhost:6333/cluster | jq .result

# 🟡 中风险：创建 Collection（指定分片和副本数）
kubectl exec -n qdrant-system $QDRANT_POD -- \
  curl -s -X PUT http://localhost:6333/collections/documents \
  -H "Content-Type: application/json" \
  -d '{
    "vectors": {
      "size": 1536,
      "distance": "Cosine",
      "on_disk": false
    },
    "shard_number": 10,
    "replication_factor": 2,
    "write_consistency_factor": 1,
    "hnsw_config": {
      "m": 16,
      "ef_construct": 200,
      "on_disk": false
    },
    "optimizers_config": {
      "indexing_threshold": 20000
    }
  }'

# 🟡 中风险：创建快照备份
kubectl exec -n qdrant-system $QDRANT_POD -- \
  curl -s -X POST http://localhost:6333/collections/documents/snapshots

# 🟢 低风险：查看快照列表
kubectl exec -n qdrant-system $QDRANT_POD -- \
  curl -s http://localhost:6333/collections/documents/snapshots | jq .
```

### 备份恢复

```bash
# 🔴 高风险：Milvus 数据备份（使用 milvus-backup 工具）
kubectl apply -f - <<EOF
apiVersion: batch/v1
kind: Job
metadata:
  name: milvus-backup-$(date +%Y%m%d)
  namespace: milvus-system
spec:
  template:
    spec:
      containers:
      - name: backup
        image: zilliz/milvus-backup:v0.4.0
        command:
        - /milvus-backup
        - create
        - --name
        - "backup-$(date +%Y%m%d-%H%M)"
        env:
        - name: MILVUS_ADDRESS
          value: "milvus-proxy.milvus-system:19530"
        - name: STORAGE_ADDRESS
          value: "minio.milvus-system:9000"
        - name: STORAGE_BUCKET
          value: "milvus-backups"
        volumeMounts:
        - name: backup-storage
          mountPath: /backups
      volumes:
      - name: backup-storage
        persistentVolumeClaim:
          claimName: milvus-backup-pvc
      restartPolicy: Never
EOF

# 🔴 高风险：Qdrant 快照恢复到新 Collection
kubectl exec -n qdrant-system qdrant-0 -- \
  curl -s -X POST http://localhost:6333/collections/documents_restored/snapshots/recover \
  -H "Content-Type: application/json" \
  -d '{"location": "/qdrant/snapshots/documents/snapshot-2026-07-19.snapshot"}'
```

## 故障排查

### 内存 OOM

```bash
# 🟢 低风险：诊断向量数据库 OOM
# Step 1: 确认 OOM 事件
kubectl get events -n qdrant-system --field-selector reason=OOMKilled --sort-by=.lastTimestamp
kubectl describe pod qdrant-0 -n qdrant-system | grep -A5 "Last State"

# Step 2: 检查实际内存使用
kubectl top pod qdrant-0 -n qdrant-system --containers
kubectl exec -n qdrant-system qdrant-0 -- cat /proc/meminfo | head -5

# Step 3: 检查 Collection 内存占用（Qdrant）
kubectl exec -n qdrant-system qdrant-0 -- \
  curl -s http://localhost:6333/collections/documents | \
  jq '.result.vectors_count, .result.indexed_vectors_count'

# 解决方案:
# 1. 启用 mmap: on_disk_payload=true, vectors on_disk=true
# 2. 增加 Pod memory limit
# 3. 减少 HNSW M 参数（牺牲召回率换内存）
# 4. 分片数据到更多节点
```

### 查询延迟高

```bash
# 🟢 低风险：诊断查询延迟
# Milvus: 检查 Query Node 负载
kubectl exec -n milvus-system deploy/milvus-proxy -- \
  python3 -c "
from pymilvus import connections, Collection
connections.connect(host='localhost', port='19530')
c = Collection('my_collection')
# 检查索引状态
print('Index:', c.index().params)
print('Loaded:', c.get_load_state())
"

# Qdrant: 检查 Segment 优化状态
kubectl exec -n qdrant-system qdrant-0 -- \
  curl -s http://localhost:6333/collections/documents | \
  jq '.result.optimizer_status, .result.segments_count'

# 常见原因及解决:
# - 索引未构建完成 → 等待 indexing 完成
# - ef 参数过低 → 提高 search ef（如 ef=128→256）
# - 数据未分片 → 增加 shard_number
# - 内存不足触发 mmap → 增加内存或启用 on_disk
```

### 常见故障速查表

| 故障现象 | 可能原因 | 排查方法 | 解决方案 |
|---------|---------|---------|---------|
| Pod OOMKilled | HNSW 索引超出内存限制 | `kubectl top pod` + Collection 向量数 | 增加内存/启用 mmap/分片 |
| 索引构建慢 | Index Node 资源不足或数据量大 | 检查 Index Node CPU/内存 | 扩容 Index Node/分批构建 |
| 查询 P99 > 500ms | ef 参数过高或节点过载 | 检查 search params 和节点负载 | 降低 ef/扩容 Query Node |
| 写入超时 | WAL 积压或 MinIO 慢 | 检查 Pulsar lag 和 MinIO 延迟 | 扩容 Data Node/检查存储 |
| 集群脑裂（Qdrant） | 网络分区导致 Raft 分裂 | `curl /cluster` 查看 peer 状态 | 修复网络/手动恢复 peer |
| etcd 连接失败（Milvus） | etcd 集群不健康 | `etcdctl endpoint health` | 修复 etcd/检查 PVC |
| 数据不一致 | 副本同步延迟 | 检查 replication lag | 调整 write_consistency_factor |

## 最佳实践

### 存储规划

1. **SSD 必选**：向量数据库对 IOPS 敏感，必须使用 SSD（gp3/io2），禁止使用 HDD
2. **容量预留**：预留 2-3 倍原始数据大小（索引 + WAL + 临时文件）
3. **存储分离**：WAL 和数据文件使用不同 PV，避免 IO 竞争
4. **Milvus MinIO**：生产环境使用外部 S3（如 AWS S3）替代内置 MinIO，降低运维复杂度

### 性能调优

```yaml
# 🟡 中风险：Qdrant Collection 索引优化配置
# 高召回率场景（推荐搜索）
hnsw_config:
  m: 32          # 更多连接，更高召回
  ef_construct: 400  # 构建时搜索宽度
  full_scan_threshold: 10000

# 低延迟场景（实时查询）
hnsw_config:
  m: 16
  ef_construct: 200
  on_disk: false  # 索引保持在内存

# 查询时参数调整
search_params:
  hnsw_ef: 128    # 查询时搜索宽度（越大越准，越慢）
  quantization:
    scalar:
      type: int8   # 8bit 量化，内存减少 4x，精度损失 <1%
```

### 监控告警

```yaml
# 🟢 低风险：Prometheus 告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: vector-db-alerts
  namespace: monitoring
spec:
  groups:
  - name: qdrant.rules
    rules:
    - alert: QdrantHighMemoryUsage
      expr: container_memory_working_set_bytes{namespace="qdrant-system"} / container_spec_memory_limit_bytes{namespace="qdrant-system"} > 0.85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Qdrant 内存使用超过 85%"
    - alert: QdrantSlowQueries
      expr: rate(qdrant_response_duration_seconds_sum{status="ok"}[5m]) / rate(qdrant_response_duration_seconds_count{status="ok"}[5m]) > 0.5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Qdrant 平均查询延迟超过 500ms"
  - name: milvus.rules
    rules:
    - alert: MilvusQueryNodeOOM
      expr: milvus_querynode_memory_usage_bytes / milvus_querynode_memory_limit_bytes > 0.9
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "Milvus Query Node 内存即将耗尽"
```

### 选型决策树

- **数据量 > 10 亿向量**：选 Milvus（原生分布式，计算存储分离）
- **数据量 1000 万 - 10 亿，追求低延迟**：选 Qdrant（Rust 实现，单节点性能优异）
- **需要多模态（文本 + 图像 + 向量）**：选 Weaviate（模块化向量化器）
- **已有 PostgreSQL，数据量 < 5000 万**：选 pgvector（零额外基础设施）
- **多租户 SaaS**：Weaviate（原生多租户）或 Qdrant（Collection per tenant）

## Related

- [[15-AI基础设施/05-K8s-AI基础设施/09-vector-database-k8s-milvus-qdrant|RAG知识库架构]]
- [[22-概念/02-工作负载/statefulset|K8s有状态服务运维]]
- [[23-实体/07-可观测性/prometheus|Prometheus监控体系]]
- [[17-系统基础/06-知识字典/storage/persistent-volume|K8s存储与PV管理]]
- [[15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving|GPU调度与资源管理]]
