---
title: 搜索引擎索引
description: 搜索引擎知识 — Elasticsearch/OpenSearch/Meilisearch 在 K8s 上的部署与运维
summary: 搜索引擎子目录，涵盖 Elasticsearch/OpenSearch 集群部署、向量搜索、日志分析、K8s Operator 管理
category: index
tags:
- index
- search-engine
- elasticsearch
- opensearch
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
---

# 搜索引擎

> Elasticsearch / OpenSearch / Meilisearch 在 Kubernetes 上的部署与运维实践。

## 文档

| 文件 | 内容 |
|------|------|
| [[07-数据库中间件/07-搜索引擎/01-elasticsearch-opensearch-k8s.md\|Elasticsearch/OpenSearch on K8s]] | ECK Operator 部署、集群架构、索引管理、ILM、性能调优、生产运维 |

## 核心知识

| 主题 | 说明 |
|------|------|
| ECK Operator | Elastic Cloud on K8s，官方 Operator 管理 ES 集群 |
| 集群规划 | Master/Data/Ingest/Coordinating 节点角色分离 |
| 分片策略 | 主分片/副本分片数量设计，避免过度分片 |
| 存储 | SSD 必选，PV 容量规划（数据 × 副本 × 1.5） |
| 向量搜索 | kNN/Dense Vector，RAG 场景的语义检索 |
| 日志分析 | ELK/EFK Stack，与 Loki 的选型对比 |
| 性能调优 | JVM 堆（≤31GB）、索引优化、查询优化 |

## Related

- [[07-数据库中间件/01-数据库/index.md|数据库]] — 关系型/NoSQL 数据库
- [[09-可观测性/README.md|可观测性知识域]] — 日志分析体系
- [[27-标签/storage|storage 标签枢纽]]
