---
title: Cortex
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- grafana
- helm
- docker
- minio
- job
- gateway
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Cortex 是什么
- 如何 Cortex
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Cortex
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
---

title: Cortex
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- helm
- docker
- minio
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Cortex 是什么
- 如何 Cortex
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Cortex
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Cortex

> **成熟度**: Incubating | **加入时间**: 2018-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://cortexmetrics.io |
| **GitHub** | https://github.com/cortexproject/cortex |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Observability & Monitoring |

---

## 项目概述

Cortex 是多租户、水平可扩展的 Prometheus 即服务解决方案。它为 Prometheus 提供长期存储、高可用性和全局视图能力，适合大规模 Kubernetes 监控场景。

## 核心特性

- **多租户**: 完全隔离的租户数据和查询
- **水平扩展**: 所有组件可独立扩展
- **长期存储**: 支持 S3、GCS、Azure Blob 等对象存储
- **高可用**: 数据复制和故障自动转移
- **兼容 Prometheus**: 完全兼容 PromQL 和 remote write
- **全局视图**: 聚合多个 Prometheus 的数据

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      Cortex Architecture                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Write Path                              │ │
│  │                                                            │ │
│  │  ┌───────────┐    ┌─────────────┐    ┌────────────────┐  │ │
│  │  │Prometheus │───▶│ Distributor │───▶│   Ingester     │  │ │
│  │  │(remote    │    │  (Routing)  │    │ (In-memory +   │  │ │
│  │  │ write)    │    │             │    │  WAL)          │  │ │
│  │  └───────────┘    └─────────────┘    └───────┬────────┘  │ │
│  │                                              │            │ │
│  │                                              │ flush      │ │
│  │                                              ▼            │ │
│  │                              ┌────────────────────────┐  │ │
│  │                              │    Object Storage      │  │ │
│  │                              │  (S3/GCS/Azure/Minio)  │  │ │
│  │                              └────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Read Path                               │ │
│  │                                                            │ │
│  │  ┌──────────┐    ┌─────────────┐    ┌─────────────────┐  │ │
│  │  │ Grafana  │───▶│Query Frontend│───▶│    Querier      │  │ │
│  │  │ (PromQL) │    │ (Caching +  │    │ (Merge Results) │  │ │
│  │  │          │    │  Splitting) │    │                 │  │ │
│  │  └──────────┘    └─────────────┘    └────────┬────────┘  │ │
│  │                                              │            │ │
│  │                       ┌──────────────────────┴───────┐   │ │
│  │                       │                              │   │ │
│  │                       ▼                              ▼   │ │
│  │              ┌─────────────┐              ┌───────────┐  │ │
│  │              │  Ingester   │              │Store      │  │ │
│  │              │ (Recent)    │              │Gateway    │  │ │
│  │              └─────────────┘              │(Historical│  │ │
│  │                                           └───────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   Supporting Components                    │ │
│  │  ┌───────────┐  ┌────────────┐  ┌──────────────────────┐ │ │
│  │  │   Ring    │  │ Compactor  │  │     Ruler            │ │ │
│  │  │(Hash Ring)│  │ (TSDB      │  │ (Recording Rules +   │ │ │
│  │  │           │  │ Compaction)│  │  Alerting Rules)     │ │ │
│  │  └───────────┘  └────────────┘  └──────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 功能 |
|------|------|
| Distributor | 接收写入请求，验证和分发到 Ingester |
| Ingester | 内存中暂存时序数据，定期 flush 到存储 |
| Querier | 从 Ingester 和 Store 合并查询结果 |
| Query Frontend | 查询缓存、分片和排队 |
| Store Gateway | 从对象存储查询历史数据 |
| Compactor | TSDB 块压缩和去重 |
| Ruler | 执行 Recording Rules 和 Alerting Rules |

---

## 快速开始

### Docker Compose 部署

```yaml
# docker-compose.yml
version: '3.8'
services:
  cortex:
    image: quay.io/cortexproject/cortex:v1.16.0
    command:
      - -config.file=/etc/cortex/config.yaml
      - -target=all
    ports:
      - "9009:9009"
    volumes:
      - ./config.yaml:/etc/cortex/config.yaml
      - cortex-data:/data
    
  minio:
    image: minio/minio
    command: server /data
    ports:
      - "9000:9000"
    environment:
      MINIO_ACCESS_KEY: cortex
      MINIO_SECRET_KEY: supersecret
    volumes:
      - minio-data:/data

volumes:
  cortex-data:
  minio-data:
```

### 单体配置

```yaml
# config.yaml
auth_enabled: false

server:
  http_listen_port: 9009
  grpc_listen_port: 9095

distributor:
  shard_by_all_labels: true
  ring:
    kvstore:
      store: inmemory

ingester:
  lifecycler:
    ring:
      kvstore:
        store: inmemory
      replication_factor: 1

storage:
  engine: blocks

blocks_storage:
  backend: s3
  s3:
    endpoint: minio:9000
    bucket_name: cortex
    access_key_id: cortex
    secret_access_key: supersecret
    insecure: true
  bucket_store:
    sync_dir: /data/tsdb-sync
  tsdb:
    dir: /data/tsdb

compactor:
  data_dir: /data/compactor
  sharding_ring:
    kvstore:
      store: inmemory

ruler:
  enable_api: true
  storage:
    type: local
    local:
      directory: /data/rules

ruler_storage:
  backend: local
  local:
    directory: /data/rules
```

### Helm 部署（微服务模式）

```bash
# 添加仓库
helm repo add cortex-helm https://cortexproject.github.io/cortex-helm-chart

# 安装
helm install cortex cortex-helm/cortex \
  --namespace cortex \
  --create-namespace \
  --values values.yaml
```

```yaml
# values.yaml
config:
  auth_enabled: true
  
  limits:
    ingestion_rate: 100000
    ingestion_burst_size: 200000
    max_series_per_user: 1000000
    max_series_per_metric: 50000

  storage:
    engine: blocks
    
  blocks_storage:
    backend: s3
    s3:
      bucket_name: cortex-blocks
      endpoint: s3.amazonaws.com

distributor:
  replicas: 3
  resources:
    requests:
      cpu: 100m
      memory: 512Mi

ingester:
  replicas: 3
  persistentVolume:
    enabled: true
    size: 50Gi
  resources:
    requests:
      cpu: 500m
      memory: 2Gi

querier:
  replicas: 2
  resources:
    requests:
      cpu: 100m
      memory: 512Mi

query_frontend:
  replicas: 2

compactor:
  replicas: 1
  persistentVolume:
    enabled: true
    size: 100Gi
```

---

## Prometheus 配置

```yaml
# prometheus.yml
global:
  external_labels:
    cluster: production
    region: us-west-2

remote_write:
  - url: http://cortex:9009/api/v1/push
    headers:
      X-Scope-OrgID: tenant-1
    queue_config:
      capacity: 10000
      max_shards: 50
      max_samples_per_send: 5000
```

---

## 多租户配置

### 租户限制

```yaml
# runtime-config.yaml
overrides:
  tenant-1:
    ingestion_rate: 200000
    ingestion_burst_size: 400000
    max_series_per_user: 2000000
    max_global_series_per_metric: 100000
    
  tenant-2:
    ingestion_rate: 50000
    max_series_per_user: 500000
```

### 查询时指定租户

```bash
# HTTP Header
curl -H "X-Scope-OrgID: tenant-1" \
  "http://cortex:9009/api/v1/query?query=up"

# 多租户查询（需要配置）
curl -H "X-Scope-OrgID: tenant-1|tenant-2" \
  "http://cortex:9009/api/v1/query?query=up"
```

---

## Recording Rules 和 Alerting Rules

```yaml
# rules.yaml
groups:
  - name: example
    rules:
      - record: job:http_requests:rate5m
        expr: sum(rate(http_requests_total[5m])) by (job)
      
      - alert: HighErrorRate
        expr: |
          sum(rate(http_requests_total{status=~"5.."}[5m])) by (job)
          /
          sum(rate(http_requests_total[5m])) by (job)
          > 0.05
        for: 10m
        labels:
          severity: critical
        annotations:
          summary: "High error rate on {{ $labels.job }}"
```

---

## 监控 Cortex 自身

```yaml
# 关键指标
- cortex_distributor_received_samples_total
- cortex_ingester_memory_series
- cortex_ingester_active_series
- cortex_querier_request_duration_seconds
- cortex_compactor_blocks_cleaned_total
```

---

## 最佳实践

1. **容量规划**: 根据每秒样本数和活跃序列数规划 Ingester 资源
2. **存储选择**: 使用对象存储而非本地磁盘
3. **查询优化**: 启用 Query Frontend 缓存和分片
4. **租户隔离**: 为不同租户配置合理的限制
5. **监控告警**: 监控 Cortex 组件健康状态

---

## 参考资源

- [官方文档](https://cortexmetrics.io/docs)
- [GitHub Repo](https://github.com/cortexproject/cortex)
- [Helm Chart](https://github.com/cortexproject/cortex-helm-chart)
- [运维指南](https://cortexmetrics.io/docs/guides/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/promql.md|promql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[references/observability-terms|K8s 可观测性术语参考]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
