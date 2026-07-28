---
title: Milvus [entities]
description: '云原生向量数据库，支持十亿级向量的相似度检索，是 RAG 与 AI 应用的核心存储组件'
summary: 'Milvus 是 LF AI & Data 毕业项目，云原生架构的分布式向量数据库，支持 HNSW/IVF/DiskANN 等索引，广泛用于 RAG、推荐系统与多模态检索场景。'
category: entities
tags:
- milvus
- vector-database
- ai-ml
- rag
- k8s
- database
- storage
tier: supporting
created: '2026-07-27'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- AI工程师
estimated_read_time: 6min
intent_queries:
- Milvus 是什么
- 如何在 Kubernetes 上部署 Milvus
- Milvus 与 Qdrant 有什么区别
trigger_keywords:
- Milvus
- vector database
- 向量数据库
- ANN
prerequisites:
- kubectl-basics
---

# Milvus

> **归属**: LF AI & Data 毕业项目 | **类别**: Vector Database | **主要语言**: Go / C++

## 概述

Milvus 是由 Zilliz 开源的云原生分布式向量数据库，用于存储 embedding 向量并执行高性能近似最近邻（ANN）检索。其存算分离架构将协调、计算（QueryNode/DataNode/IndexNode）与存储（对象存储 + 消息队列）解耦，天然适配 Kubernetes 弹性伸缩，是 RAG（检索增强生成）知识库、语义搜索、推荐与多模态检索的主流选型。

## Key Features（核心能力）

- **多索引类型**：HNSW、IVF_FLAT/IVF_PQ、DiskANN、GPU 索引（CAGRA），按召回/延迟/成本权衡选择
- **标量过滤 + 混合检索**：向量相似度与标量条件（时间、标签）组合查询，支持稀疏+稠密混合检索
- **存算分离**：数据落地 S3/MinIO 对象存储，日志经 Pulsar/Kafka（2.5+ 支持 Woodpecker 去 MQ 化）
- **多租户**：Database / Collection / Partition Key 三级隔离
- **一致性级别可调**：Strong / Bounded / Eventually，按业务折衷延迟与新鲜度

## K8s 部署形态

| 形态 | 适用场景 | 说明 |
|------|----------|------|
| Milvus Standalone | 开发/PoC | 单 Pod，内置 etcd + 本地存储 |
| Milvus Cluster (Operator) | 生产 | milvus-operator 管理 CRD，组件独立扩缩 |
| Milvus Cluster (Helm) | 生产 | 官方 Chart，依赖 etcd/Pulsar/MinIO 子 Chart |

```bash
# 🟢 Operator 方式部署（推荐生产）
helm install milvus-operator milvus-operator/milvus-operator -n milvus-operator --create-namespace
kubectl apply -f - <<'YAML'
apiVersion: milvus.io/v1beta1
kind: Milvus
metadata:
  name: milvus-prod
spec:
  mode: cluster
  dependencies:
    storage:
      external: true          # 生产建议对接外部 S3/OSS
YAML
```

## 生产运维要点

- **资源画像**：QueryNode 内存 = 索引常驻内存，HNSW 约为原始向量 1.5~2 倍；容量规划先算向量总量 × 维度 × 4 字节
- 🟢 组件健康：`kubectl get milvus milvus-prod -o wide`；`birdwatcher` 工具检查 etcd 元数据
- 🟡 扩容 QueryNode 后需等待 segment 负载均衡完成，观察 `milvus_querynode_sq_req_latency` 指标
- 🔴 etcd 是元数据单点，必须独立三副本部署并纳入备份；对象存储数据丢失不可恢复
- 常见故障：insert 堆积（DataNode flush 慢，查对象存储写延迟）、查询超时（QueryNode OOM/索引未加载）

## 与 Qdrant 对比

| 维度 | Milvus | Qdrant |
|------|--------|--------|
| 架构 | 存算分离、组件多 | 单二进制、部署简单 |
| 规模上限 | 十亿级+ | 亿级 |
| 依赖 | etcd + 对象存储 (+MQ) | 本地盘/S3 快照 |
| 运维复杂度 | 高 | 低 |

选型：超大规模、多租户平台选 Milvus；中小规模、追求运维简单选 Qdrant。

## 相关阅读

- [[15-AI基础设施/05-K8s-AI基础设施/08-vector-database-k8s-milvus-qdrant|向量数据库上 K8s 生产实践（Milvus/Qdrant）]]
- [[07-数据库中间件/README|数据库中间件域总览]]
- [[15-AI基础设施/05-K8s-AI基础设施/11-llm-gateway-routing-cost|LLM Gateway 与推理路由]]
