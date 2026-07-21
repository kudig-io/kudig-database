---
title: Elasticsearch & OpenSearch on K8s
description: 搜索引擎深度指南 — Elasticsearch/OpenSearch 架构、K8s 部署、ECK Operator、生产实践
summary: 搜索引擎完整指南，涵盖 Elasticsearch vs OpenSearch 对比、ECK Operator 部署、集群架构、索引管理、性能调优、生产运维
tags:
- elasticsearch
- opensearch
- eck
- search-engine
- kubernetes
difficulty: advanced
domain: 数据库中间件
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
---
# Elasticsearch & OpenSearch on K8s

## 1. 架构概述

### 1.1 Elasticsearch vs OpenSearch

| 特性 | Elasticsearch | OpenSearch |
|------|---------------|------------|
| 许可证 | Elastic License 2.0 | Apache 2.0 |
| 维护方 | Elastic | AWS/社区 |
| 最新版本 | 8.x | 2.x |
| 安全功能 | 内置（付费） | 内置（免费） |
| 告警 | 内置（付费） | 内置（免费） |
| K8s Operator | ECK | OpenSearch Operator |

### 1.2 集群架构

```
┌─────────────────────────────────────────────────────────┐
│                   Elasticsearch Cluster                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     │
│  │   Master    │  │   Master    │  │   Master    │     │
│  │   Node 1    │  │   Node 2    │  │   Node 3    │     │
│  └─────────────┘  └─────────────┘  └─────────────┘     │
│                                                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     │
│  │    Data     │  │    Data     │  │    Data     │     │
│  │   Node 1    │  │   Node 2    │  │   Node 3    │     │
│  └─────────────┘  └─────────────┘  └─────────────┘     │
│                                                         │
│  ┌─────────────┐  ┌─────────────┐                      │
│  │  Ingest     │  │  Coordinating│                     │
│  │   Node      │  │    Node      │                     │
│  └─────────────┘  └─────────────┘                      │
└─────────────────────────────────────────────────────────┘
```

## 2. ECK Operator 部署

### 2.1 安装 ECK

```bash
# 安装 CRD
kubectl apply -f https://download.elastic.co/downloads/eck/2.11.0/crds.yaml

# 安装 Operator
kubectl apply -f https://download.elastic.co/downloads/eck/2.11.0/operator.yaml
```

### 2.2 部署 Elasticsearch 集群

```yaml
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: production
  namespace: logging
spec:
  version: 8.12.0
  nodeSets:
    # Master 节点
    - name: master
      count: 3
      config:
        node.roles: ["master"]
        node.store.allow_mmap: false
      resources:
        requests:
          cpu: 1
          memory: 2Gi
        limits:
          memory: 2Gi
      volumeClaimTemplates:
        - metadata:
            name: elasticsearch-data
          spec:
            accessModes: ["ReadWriteOnce"]
            resources:
              requests:
                storage: 10Gi
            storageClassName: fast-ssd
    # Data 节点
    - name: data
      count: 3
      config:
        node.roles: ["data", "ingest"]
        node.store.allow_mmap: false
      resources:
        requests:
          cpu: 4
          memory: 16Gi
        limits:
          memory: 16Gi
      volumeClaimTemplates:
        - metadata:
            name: elasticsearch-data
          spec:
            accessModes: ["ReadWriteOnce"]
            resources:
              requests:
                storage: 500Gi
            storageClassName: fast-ssd
```

### 2.3 部署 Kibana

```yaml
apiVersion: kibana.k8s.elastic.co/v1
kind: Kibana
metadata:
  name: production
  namespace: logging
spec:
  version: 8.12.0
  count: 1
  elasticsearchRef:
    name: production
  http:
    tls:
      selfSignedCertificate:
        disabled: true
  resources:
    requests:
      cpu: 500m
      memory: 1Gi
```

## 3. 索引管理

### 3.1 Index Lifecycle Management (ILM)

```yaml
# ILM 策略
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: production
spec:
  # ... 集群配置
---
# 通过 API 创建 ILM 策略
PUT _ilm/policy/logs-policy
{
  "policy": {
    "phases": {
      "hot": {
        "min_age": "0ms",
        "actions": {
          "rollover": {
            "max_size": "50gb",
            "max_age": "1d"
          },
          "set_priority": {
            "priority": 100
          }
        }
      },
      "warm": {
        "min_age": "7d",
        "actions": {
          "shrink": {
            "number_of_shards": 1
          },
          "forcemerge": {
            "max_num_segments": 1
          },
          "set_priority": {
            "priority": 50
          }
        }
      },
      "cold": {
        "min_age": "30d",
        "actions": {
          "freeze": {},
          "set_priority": {
            "priority": 0
          }
        }
      },
      "delete": {
        "min_age": "90d",
        "actions": {
          "delete": {}
        }
      }
    }
  }
}
```

### 3.2 Index Template

```json
PUT _index_template/logs-template
{
  "index_patterns": ["logs-*"],
  "template": {
    "settings": {
      "number_of_shards": 3,
      "number_of_replicas": 1,
      "index.lifecycle.name": "logs-policy",
      "index.lifecycle.rollover_alias": "logs"
    },
    "mappings": {
      "properties": {
        "@timestamp": { "type": "date" },
        "message": { "type": "text" },
        "level": { "type": "keyword" },
        "service": { "type": "keyword" },
        "trace_id": { "type": "keyword" }
      }
    }
  }
}
```

## 4. 性能调优

### 4.1 JVM 配置

```yaml
# ECK 自动配置 JVM 堆为容器内存的 50%
# 可通过 config 覆盖
spec:
  nodeSets:
    - name: data
      config:
        # JVM 选项
        -Xms8g
        -Xmx8g
        # GC 配置
        -XX:+UseG1GC
        -XX:G1HeapRegionSize=16m
        -XX:InitiatingHeapOccupancyPercent=40
```

### 4.2 索引优化

| 参数 | 推荐值 | 说明 |
|------|--------|------|
| number_of_shards | 数据量/50GB | 分片大小 |
| number_of_replicas | 1-2 | 副本数 |
| refresh_interval | 30s | 刷新间隔（写入密集） |
| translog.durability | async | 异步持久化（性能优先） |
| merge.scheduler.max_thread_count | 1 | SSD 优化 |

### 4.3 查询优化

```json
// 使用 filter 而非 query（可缓存）
GET logs-*/_search
{
  "query": {
    "bool": {
      "filter": [
        { "term": { "level": "error" } },
        { "range": { "@timestamp": { "gte": "now-1h" } } }
      ]
    }
  },
  "size": 100,
  "sort": [{ "@timestamp": "desc" }]
}
```

## 5. 生产运维

### 5.1 监控指标

```promql
# 集群状态
elasticsearch_cluster_health_status{color="red"} == 1

# JVM 堆使用率
elasticsearch_jvm_memory_used_bytes{area="heap"} /
elasticsearch_jvm_memory_max_bytes{area="heap"} > 0.85

# 索引速率
rate(elasticsearch_indices_indexing_index_total[5m])

# 查询延迟
elasticsearch_indices_search_query_time_seconds
```

### 5.2 备份恢复

```yaml
# 快照仓库
PUT _snapshot/backup-repo
{
  "type": "s3",
  "settings": {
    "bucket": "es-backups",
    "region": "us-east-1",
    "role_arn": "arn:aws:iam::123456789:role/es-backup"
  }
}

# 创建快照
PUT _snapshot/backup-repo/snapshot-2026-07-21
{
  "indices": "logs-*",
  "ignore_unavailable": true,
  "include_global_state": false
}
```

### 5.3 滚动重启

```bash
# 禁用分片分配
PUT _cluster/settings
{
  "persistent": {
    "cluster.routing.allocation.enable": "primaries"
  }
}

# 停止索引并刷新
POST _flush/synced

# 重启节点

# 重新启用分配
PUT _cluster/settings
{
  "persistent": {
    "cluster.routing.allocation.enable": null
  }
}
```

## 6. 故障排查

### 6.1 集群状态异常

```bash
# 查看集群健康
GET _cluster/health

# 查看未分配分片
GET _cat/shards?v&h=index,shard,prirep,state,unassigned.reason

# 查看热点节点
GET _cat/nodes?v&h=name,heap.percent,ram.percent,cpu,load_1m
```

### 6.2 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 集群 RED | 主分片未分配 | 检查节点状态、磁盘空间 |
| 集群 YELLOW | 副本分片未分配 | 增加节点或减少副本 |
| 写入拒绝 | 线程池满 | 增加线程池或降低写入速率 |
| 查询慢 | 大结果集/复杂聚合 | 优化查询、增加分片 |

## Related

- [[数据库中间件/搜索引擎/index.md|搜索引擎索引]]
- [[可观测性/日志/index.md|日志管理]]
- [[数据库中间件/README.md|数据库中间件]]
