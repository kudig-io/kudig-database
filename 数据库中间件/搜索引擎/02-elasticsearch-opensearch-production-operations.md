---
title: Elasticsearch and OpenSearch on Kubernetes — Production Operations
description: K8s 上 Elasticsearch/OpenSearch 生产运维 — ECK Operator、集群拓扑、分片策略、性能调优、日志管道、故障排查
summary: 在 Kubernetes 上运行生产级 Elasticsearch/OpenSearch 集群的完整实践
category: practice
tags:
- elasticsearch
- opensearch
- eck
- search
- logging
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: database
---
# Elasticsearch/OpenSearch Kubernetes 生产运维

> 使用 ECK Operator 在 K8s 上运行生产级搜索与日志集群。

## 架构选型

| 方案 | 适用场景 | 许可证 | 运维复杂度 |
|------|----------|--------|-----------|
| ECK (Elastic) | 企业搜索/日志/APM | Elastic License 2.0 | 中 |
| OpenSearch Operator | 开源替代/日志分析 | Apache 2.0 | 中 |
| 自建 StatefulSet | 简单场景/学习 | 取决于版本 | 高 |

## ECK Operator 部署

```bash
# 安装 ECK Operator
kubectl create -f https://download.elastic.co/downloads/eck/2.13.0/crds.yaml
kubectl apply -f https://download.elastic.co/downloads/eck/2.13.0/operator.yaml
```

## Elasticsearch 集群部署

### 生产集群（Hot-Warm-Cold 架构）

```yaml
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: logs
  namespace: logging
spec:
  version: 8.14.0
  nodeSets:
    # Master 节点（专用）
    - name: master
      count: 3
      config:
        node.roles: ["master"]
        xpack.ml.enabled: false
      podTemplate:
        spec:
          containers:
            - name: elasticsearch
              resources:
                requests:
                  cpu: "1"
                  memory: 4Gi
                limits:
                  memory: 4Gi
      volumeClaimTemplates:
        - metadata:
            name: elasticsearch-data
          spec:
            storageClassName: gp3-encrypted
            accessModes: ["ReadWriteOnce"]
            resources:
              requests:
                storage: 20Gi
    # Hot 节点（写入+近期查询）
    - name: hot
      count: 3
      config:
        node.roles: ["data_hot", "ingest", "transform"]
        node.attr.data: hot
        xpack.ml.enabled: false
      podTemplate:
        spec:
          initContainers:
            - name: sysctl
              securityContext:
                privileged: true
              command: ["sysctl", "-w", "vm.max_map_count=262144"]
          containers:
            - name: elasticsearch
              resources:
                requests:
                  cpu: "4"
                  memory: 16Gi
                limits:
                  memory: 16Gi
          affinity:
            podAntiAffinity:
              requiredDuringSchedulingIgnoredDuringExecution:
                - labelSelector:
                    matchLabels:
                      elasticsearch.k8s.elastic.co/cluster-name: logs
                  topologyKey: kubernetes.io/hostname
      volumeClaimTemplates:
        - metadata:
            name: elasticsearch-data
          spec:
            storageClassName: gp3-encrypted
            accessModes: ["ReadWriteOnce"]
            resources:
              requests:
                storage: 500Gi
    # Warm 节点（历史数据查询）
    - name: warm
      count: 2
      config:
        node.roles: ["data_warm"]
        node.attr.data: warm
        xpack.ml.enabled: false
      podTemplate:
        spec:
          containers:
            - name: elasticsearch
              resources:
                requests:
                  cpu: "2"
                  memory: 8Gi
                limits:
                  memory: 8Gi
      volumeClaimTemplates:
        - metadata:
            name: elasticsearch-data
          spec:
            storageClassName: st1  # 低成本存储
            accessModes: ["ReadWriteOnce"]
            resources:
              requests:
                storage: 2000Gi
  http:
    tls:
      selfSignedCertificate:
        disabled: false
```

### ILM 策略（索引生命周期）

```yaml
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: logs
spec:
  # ... (集群配置)
---
# 通过 API 创建 ILM 策略
# PUT _ilm/policy/logs-policy
{
  "policy": {
    "phases": {
      "hot": {
        "min_age": "0ms",
        "actions": {
          "rollover": {
            "max_size": "50gb",
            "max_age": "1d",
            "max_docs": 100000000
          },
          "set_priority": { "priority": 100 }
        }
      },
      "warm": {
        "min_age": "7d",
        "actions": {
          "allocate": {
            "include": { "data": "warm" },
            "number_of_replicas": 1
          },
          "forcemerge": { "max_num_segments": 1 },
          "shrink": { "number_of_shards": 1 },
          "set_priority": { "priority": 50 }
        }
      },
      "cold": {
        "min_age": "30d",
        "actions": {
          "allocate": { "number_of_replicas": 0 },
          "set_priority": { "priority": 0 }
        }
      },
      "delete": {
        "min_age": "90d",
        "actions": { "delete": {} }
      }
    }
  }
}
```

## 索引模板

```json
// PUT _index_template/logs-template
{
  "index_patterns": ["logs-*"],
  "template": {
    "settings": {
      "number_of_shards": 3,
      "number_of_replicas": 1,
      "index.lifecycle.name": "logs-policy",
      "index.lifecycle.rollover_alias": "logs",
      "index.codec": "best_compression",
      "index.refresh_interval": "30s",
      "index.translog.durability": "async",
      "index.translog.sync_interval": "30s"
    },
    "mappings": {
      "dynamic": "strict",
      "properties": {
        "@timestamp": { "type": "date" },
        "message": { "type": "text" },
        "level": { "type": "keyword" },
        "service": { "type": "keyword" },
        "trace_id": { "type": "keyword" },
        "kubernetes": {
          "properties": {
            "namespace": { "type": "keyword" },
            "pod_name": { "type": "keyword" },
            "container_name": { "type": "keyword" }
          }
        }
      }
    }
  }
}
```

## 性能调优

### JVM 配置

```yaml
# ECK 自动设置 JVM heap = 50% 容器内存
# 确保 limits.memory = 2 × 期望 heap
# 例: 8Gi heap → 16Gi memory limit
config:
  node.store.allow_mmap: true  # 使用 mmap（需足够 vm.max_map_count）
```

### 关键参数

| 参数 | 推荐值 | 说明 |
|------|--------|------|
| heap | ≤ 31GB（压缩指针） | 不超过物理内存 50% |
| refresh_interval | 30s（日志）/ 1s（搜索） | 减少 I/O |
| translog.durability | async（日志）/ request（搜索） | 写入性能 vs 持久性 |
| number_of_shards | 数据量 / 50GB | 每分片 10-50GB |
| number_of_replicas | 1（生产）/ 0（临时导入） | 可用性 vs 写入速度 |
| thread_pool.write.queue_size | 1000 | 写入队列 |

### 集群级调优

```bash
# 减少副本（大批量导入时）
PUT /logs-*/_settings
{"index.number_of_replicas": 0}

# 增加刷新间隔
PUT /logs-*/_settings
{"index.refresh_interval": "60s"}

# 导入完成后恢复
PUT /logs-*/_settings
{"index.number_of_replicas": 1, "index.refresh_interval": "30s"}
```

## 监控告警

```promql
# 集群健康
elasticsearch_cluster_health_status{color="red"} == 1  # 红色告警
elasticsearch_cluster_health_number_of_nodes < 3  # 节点不足

# 磁盘使用
elasticsearch_filesystem_data_available_bytes / elasticsearch_filesystem_data_size_bytes < 0.15

# JVM 堆内存
elasticsearch_jvm_memory_used_bytes{area="heap"} / elasticsearch_jvm_memory_max_bytes{area="heap"} > 0.85

# 索引速率
rate(elasticsearch_indices_indexing_index_total[5m])

# 搜索延迟
elasticsearch_indices_search_query_time_seconds / elasticsearch_indices_search_query_total
```

## 故障排查

| 症状 | 原因 | 排查 |
|------|------|------|
| 集群 RED | 主分片未分配 | `GET _cluster/allocation/explain` |
| 集群 YELLOW | 副本未分配 | 检查节点数/磁盘 |
| 写入拒绝 | 写入队列满 | 增加节点/减少写入速率 |
| OOM | Heap 过大/聚合过多 | 调整 heap + 限制聚合 |
| 磁盘水位线 | 磁盘 > 85% | 扩容/删除旧索引/ILM |
| 慢查询 | 映射不当/大聚合 | 慢查询日志 + Profile API |

```bash
# 集群状态
kubectl exec -it logs-es-master-0 -n logging -- curl -sk https://localhost:9200/_cluster/health?pretty

# 未分配分片
kubectl exec -it logs-es-master-0 -n logging -- curl -sk https://localhost:9200/_cluster/allocation/explain?pretty

# 节点资源
kubectl exec -it logs-es-master-0 -n logging -- curl -sk https://localhost:9200/_cat/nodes?v&h=name,heap.percent,ram.percent,cpu,load_1m,disk.used_percent

# 热线程
kubectl exec -it logs-es-master-0 -n logging -- curl -sk https://localhost:9200/_nodes/hot_threads
```

## Related

- [[数据库中间件/搜索引擎/index.md|搜索引擎]]
- [[数据库中间件/搜索引擎/01-elasticsearch-opensearch-k8s.md|ES/OpenSearch 基础]]
- [[可观测性/日志/index.md|日志]]
