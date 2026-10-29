---
title: Elasticsearch on Kubernetes 生产部署
description: 'ECK Operator 安装配置、热温冷架构、ILM 索引生命周期管理、JVM 调优与集群运维'
summary: 'ECK Operator 安装配置、热温冷架构、ILM 索引生命周期管理、JVM 调优与集群运维'
category: database-middleware
tags:
- database
- k8s
- elasticsearch
- eck
- search
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- DBA
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Elasticsearch on Kubernetes 生产部署 是什么
- 如何 Elasticsearch on Kubernetes 生产部署
trigger_keywords:
- elasticsearch
- eck
- elastic
- 热温冷
- ilm
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


# Elasticsearch on Kubernetes 生产部署

## 1. ECK Operator 安装与配置

### 1.1 安装 Elastic Cloud on Kubernetes (ECK)

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Elastic Helm 仓库
helm repo add elastic https://helm.elastic.co
helm repo update

# 安装 CRD 和 Operator
kubectl create -f https://download.elastic.co/downloads/eck/2.16.0/crds.yaml
kubectl apply -f https://download.elastic.co/downloads/eck/2.16.0/operator.yaml

# 或通过 Helm 安装
helm install elastic-operator elastic/eck-operator \
  -n elastic-system --create-namespace \
  --set installCRDs=true \
  --set replicas=1 \
  --set webhook.enabled=true

# 验证 Operator 状态
kubectl -n elastic-system get pods
kubectl -n elastic-system logs -f statefulset/elastic-operator
```
### 1.2 生产级 Elasticsearch 集群定义

```yaml
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: prod-es
  namespace: logging
spec:
  version: 8.17.0
  image: docker.elastic.co/elasticsearch/elasticsearch:8.17.0
  nodeSets:
  # 主节点
  - name: master
    count: 3
    config:
      node.roles: ["master"]
      xpack.security.enabled: true
      xpack.security.transport.ssl.enabled: true
      xpack.security.http.ssl.enabled: true
    podTemplate:
      spec:
        containers:
        - name: elasticsearch
          resources:
            requests:
              memory: 2Gi
              cpu: "1"
            limits:
              memory: 2Gi
              cpu: "2"
        affinity:
          podAntiAffinity:
            requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  elasticsearch.k8s.elastic.co/statefulset-name: prod-es-es-master
              topologyKey: kubernetes.io/hostname
    volumeClaimTemplates:
    - metadata:
        name: elasticsearch-data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: gp3
        resources:
          requests:
            storage: 10Gi
  # 数据节点 - 热层
  - name: data-hot
    count: 3
    config:
      node.roles: ["data_hot", "data_content"]
      xpack.security.enabled: true
    podTemplate:
      spec:
        nodeSelector:
          node-role: elasticsearch-hot
        tolerations:
        - key: dedicated
          value: elasticsearch
          effect: NoSchedule
        containers:
        - name: elasticsearch
          resources:
            requests:
              memory: 16Gi
              cpu: "4"
            limits:
              memory: 16Gi
              cpu: "8"
    volumeClaimTemplates:
    - metadata:
        name: elasticsearch-data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: gp3-ssd
        resources:
          requests:
            storage: 500Gi
  # 数据节点 - 温层
  - name: data-warm
    count: 3
    config:
      node.roles: ["data_warm"]
      xpack.security.enabled: true
    podTemplate:
      spec:
        nodeSelector:
          node-role: elasticsearch-warm
        containers:
        - name: elasticsearch
          resources:
            requests:
              memory: 8Gi
              cpu: "2"
            limits:
              memory: 8Gi
              cpu: "4"
    volumeClaimTemplates:
    - metadata:
        name: elasticsearch-data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: gp3-hdd
        resources:
          requests:
            storage: 2Ti
```

### 1.3 TLS 与安全配置

```yaml
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: prod-es
spec:
  http:
    tls:
      selfSignedCertificate:
        subjectAltNames:
        - dns: "es-internal.logging.svc.cluster.local"
        - ip: "10.0.1.100"
    service:
      spec:
        type: LoadBalancer
        annotations:
          service.beta.kubernetes.io/aws-load-balancer-type: nlb
          service.beta.kubernetes.io/aws-load-balancer-scheme: internal
---
# 自定义 CA 证书
apiVersion: v1
kind: Secret
metadata:
  name: prod-es-http-certs-public
  namespace: logging
type: Opaque
data:
  ca.crt: <base64-encoded-ca>
  tls.crt: <base64-encoded-cert>
  tls.key: <base64-encoded-key>
```

## 2. 热温冷架构设计

### 2.1 架构总览

```
┌─────────────────────────────────────────────────────────┐
│                    Ingest Pipeline                       │
│              (Fleet / Filebeat / Logstash)               │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  Hot Tier (SSD / gp3-ssd)                               │
│  node.roles: [data_hot, data_content]                   │
│  - 当前写入索引                                          │
│  - 最近 7 天数据                                         │
│  - I/O 密集型，高 IOPS                                   │
└────────────────────────┬────────────────────────────────┘
                         │ ILM rollover
                         ▼
┌─────────────────────────────────────────────────────────┐
│  Warm Tier (HDD / gp3)                                  │
│  node.roles: [data_warm]                                │
│  - 7-30 天数据                                           │
│  - force-merge 到 1 segment                              │
│  - 只读，查询优化                                        │
└────────────────────────┬────────────────────────────────┘
                         │ ILM delete / snapshot
                         ▼
┌─────────────────────────────────────────────────────────┐
│  Cold Tier (S3 / 对象存储)                               │
│  node.roles: [data_frozen]                              │
│  - 30+ 天数据，可搜索快照                                │
│  - Shared Snapshot Repository                           │
└─────────────────────────────────────────────────────────┘
```

### 2.2 节点角色与存储映射

| 层级 | node.roles | StorageClass | 磁盘类型 | 保留时间 | 副本数 |
|------|-----------|-------------|---------|---------|-------|
| Hot | data_hot, data_content | gp3-ssd | NVMe SSD | 7 天 | 1 |
| Warm | data_warm | gp3-hdd | HDD | 30 天 | 1 |
| Cold | data_frozen | S3 快照 | 对象存储 | 90 天 | 0 (快照) |

### 2.3 节点亲和与调度

```yaml
# Hot 节点 NodePool
apiVersion: v1
kind: Node
metadata:
  name: es-hot-node-01
  labels:
    node-role: elasticsearch-hot
    storage-type: ssd
spec:
  taints:
  - key: dedicated
    value: elasticsearch
    effect: NoSchedule
---
# Warm 节点 NodePool
apiVersion: v1
kind: Node
metadata:
  name: es-warm-node-01
  labels:
    node-role: elasticsearch-warm
    storage-type: hdd
```

## 3. 索引生命周期管理 (ILM)

### 3.1 ILM 策略定义

```json
PUT _ilm/policy/logs-lifecycle
{
  "policy": {
    "phases": {
      "hot": {
        "min_age": "0ms",
        "actions": {
          "rollover": {
            "max_primary_shard_size": "50gb",
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
          },
          "allocate": {
            "require": {
              "data": "warm"
            }
          }
        }
      },
      "cold": {
        "min_age": "30d",
        "actions": {
          "searchable_snapshot": {
            "snapshot_repository": "s3-repo",
            "force_merge_index": true
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

### 3.2 Index Template 绑定 ILM

```json
PUT _index_template/logs-template
{
  "index_patterns": ["logs-*"],
  "template": {
    "settings": {
      "index.lifecycle.name": "logs-lifecycle",
      "index.lifecycle.rollover_alias": "logs-write",
      "number_of_shards": 3,
      "number_of_replicas": 1,
      "index.refresh_interval": "30s",
      "index.codec": "best_compression"
    },
    "mappings": {
      "dynamic": "strict",
      "properties": {
        "@timestamp": { "type": "date" },
        "message": { "type": "text", "analyzer": "standard" },
        "level": { "type": "keyword" },
        "service": { "type": "keyword" },
        "host": { "type": "keyword" }
      }
    }
  }
}
```

### 3.3 ISM (OpenSearch 兼容配置)

```json
PUT _plugins/_ism/policies/logs-ism
{
  "policy": {
    "description": "Logs lifecycle policy",
    "default_state": "hot",
    "states": [
      {
        "name": "hot",
        "actions": [
          {
            "rollover": {
              "min_primary_shard_size": "50gb"
            }
          }
        ],
        "transitions": [
          {
            "state_name": "warm",
            "conditions": {
              "min_index_age": "7d"
            }
          }
        ]
      },
      {
        "name": "warm",
        "actions": [
          {
            "force_merge": {
              "max_num_segments": 1
            }
          },
          {
            "replica_count": {
              "number_of_replicas": 0
            }
          }
        ],
        "transitions": [
          {
            "state_name": "delete",
            "conditions": {
              "min_index_age": "30d"
            }
          }
        ]
      },
      {
        "name": "delete",
        "actions": [
          { "delete": {} }
        ]
      }
    ]
  }
}
```

## 4. JVM 堆内存调优

### 4.1 堆大小配置

```yaml
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: prod-es
spec:
  nodeSets:
  - name: data-hot
    count: 3
    config:
      # 堆大小不超过物理内存 50%，最大不超过 31GB
      # 通过 ES_JAVA_OPTS 设置
    podTemplate:
      spec:
        containers:
        - name: elasticsearch
          env:
          - name: ES_JAVA_OPTS
            value: "-Xms16g -Xmx16g"
          resources:
            requests:
              memory: 32Gi
              cpu: "4"
            limits:
              memory: 32Gi
              cpu: "8"
```

### 4.2 JVM 参数优化

```yaml
config:
  # 垃圾回收器 - JDK 17+ 默认 G1GC
  node.store.allow_mmap: false
  # 线程池配置
  thread_pool.write.queue_size: 1000
  thread_pool.search.queue_size: 1000
  # 断路器
  indices.breaker.total.limit: "70%"
  indices.breaker.request.limit: "60%"
  indices.breaker.fielddata.limit: "40%"
  # 查询缓存
  indices.queries.cache.size: "10%"
  indices.fielddata.cache.size: "20%"
```

### 4.3 操作系统级优化

```yaml
podTemplate:
  spec:
    initContainers:
    - name: sysctl
      securityContext:
        privileged: true
        runAsUser: 0
      command: ['sh', '-c', 'sysctl -w vm.max_map_count=262144']
    - name: ulimits
      securityContext:
        privileged: true
        runAsUser: 0
      command: ['sh', '-c', 'ulimit -n 65535 && ulimit -u 4096']
    containers:
    - name: elasticsearch
      securityContext:
        capabilities:
          drop: ["ALL"]
        runAsNonRoot: true
        runAsUser: 1000
```

## 5. 集群扩缩容

### 5.1 水平扩容

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 通过 ECK 扩展数据节点
kubectl patch elasticsearch prod-es -n logging --type merge \
  -p '{"spec":{"nodeSets":[{"name":"data-hot","count":5}]}}'

# 验证分片重新分配
kubectl exec -n logging prod-es-es-data-hot-0 -- \
  curl -s -k -u elastic:$PASSWORD \
  https://localhost:9200/_cluster/health?pretty

# 监控分片迁移进度
kubectl exec -n logging prod-es-es-data-hot-0 -- \
  curl -s -k -u elastic:$PASSWORD \
  https://localhost:9200/_cat/recovery?v&active_only=true
```
### 5.2 垂直扩容（滚动重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 修改资源配置触发滚动更新
kubectl patch elasticsearch prod-es -n logging --type merge \
  -p '{
    "spec":{
      "nodeSets":[
        {
          "name":"data-hot",
          "count":3,
          "podTemplate":{
            "spec":{
              "containers":[{
                "name":"elasticsearch",
                "resources":{
                  "requests":{"memory":"32Gi","cpu":"8"},
                  "limits":{"memory":"32Gi","cpu":"16"}
                }
              }]
            }
          }
        }
      ]
    }
  }'

# ECK 自动执行: drain node → migrate shards → recreate pod → rejoin
```
### 5.3 缩容与分片迁移

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 排空目标节点
kubectl exec -n logging prod-es-es-data-hot-2 -- \
  curl -s -k -u elastic:$PASSWORD -X PUT \
  "https://localhost:9200/_cluster/settings" \
  -H 'Content-Type: application/json' \
  -d '{"transient":{"cluster.routing.allocation.exclude._name":"prod-es-es-data-hot-2"}}'

# 2. 等待分片迁移完成
kubectl exec -n logging prod-es-es-data-hot-0 -- \
  curl -s -k -u elastic:$PASSWORD \
  "https://localhost:9200/_cat/allocation?v"

# 3. 缩减节点数
kubectl patch elasticsearch prod-es -n logging --type merge \
  -p '{"spec":{"nodeSets":[{"name":"data-hot","count":2}]}}'
```
## 6. 滚动重启

### 6.1 ECK 自动滚动重启

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 版本升级触发滚动重启
kubectl patch elasticsearch prod-es -n logging --type merge \
  -p '{"spec":{"version":"8.18.0"}}'

# 监控重启进度
kubectl get events -n logging --field-selector reason=ScalingReplicaSet
kubectl rollout status statefulset/prod-es-es-data-hot -n logging
```
### 6.2 手动滚动重启流程

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# manual-rolling-restart.sh
NAMESPACE="logging"
CLUSTER="prod-es"
NODESET="data-hot"
COUNT=3
PASSWORD=$(kubectl get secret ${CLUSTER}-es-elastic-user -n ${NAMESPACE} \
  -o jsonpath='{.data.elastic}' | base64 -d)

for i in $(seq 0 $((COUNT-1))); do
  POD="${CLUSTER}-es-${NODESET}-${i}"
  echo "=== Restarting ${POD} ==="

  # 1. 禁用分片分配
  kubectl exec -n ${NAMESPACE} ${POD} -- \
    curl -s -k -u elastic:${PASSWORD} -X PUT \
    "https://localhost:9200/_cluster/settings" \
    -H 'Content-Type: application/json' \
    -d '{"persistent":{"cluster.routing.allocation.enable":"primaries"}}'

  # 2. 执行同步刷新
  kubectl exec -n ${NAMESPACE} ${POD} -- \
    curl -s -k -u elastic:${PASSWORD} -X POST \
    "https://localhost:9200/_flush/synced"

  # 3. 删除 Pod 触发重建
  kubectl delete pod -n ${NAMESPACE} ${POD}

  # 4. 等待 Pod 就绪
  kubectl wait --for=condition=Ready pod/${POD} -n ${NAMESPACE} --timeout=300s

  # 5. 等待集群恢复 green
  while true; do
    HEALTH=$(kubectl exec -n ${NAMESPACE} ${POD} -- \
      curl -s -k -u elastic:${PASSWORD} \
      "https://localhost:9200/_cluster/health" | jq -r '.status')
    if [ "$HEALTH" = "green" ]; then
      echo "Cluster status: green"
      break
    fi
    echo "Waiting for green... current: ${HEALTH}"
    sleep 10
  done

  # 6. 恢复分片分配
  kubectl exec -n ${NAMESPACE} ${POD} -- \
    curl -s -k -u elastic:${PASSWORD} -X PUT \
    "https://localhost:9200/_cluster/settings" \
    -H 'Content-Type: application/json' \
    -d '{"persistent":{"cluster.routing.allocation.enable":"all"}}'
done
```
## 7. 快照备份与恢复

### 7.1 S3 快照仓库配置

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: s3-credentials
  namespace: logging
type: Opaque
data:
  access-key: <base64>
  secret-key: <base64>
---
# 注册快照仓库
# PUT _snapshot/s3-repo
# {
#   "type": "s3",
#   "settings": {
#     "bucket": "es-snapshots-prod",
#     "region": "us-east-1",
#     "base_path": "elasticsearch/prod",
#     "compress": true,
#     "max_snapshot_bytes_per_sec": "200mb",
#     "max_restore_bytes_per_sec": "200mb"
#   }
# }

# SLM 自动快照策略
# PUT _slm/policy/nightly-snapshot
# {
#   "schedule": "0 30 2 * * ?",
#   "name": "<nightly-snap-{now/d}>",
#   "repository": "s3-repo",
#   "config": {
#     "indices": ["*"],
#     "ignore_unavailable": true,
#     "include_global_state": false
#   },
#   "retention": {
#     "expire_after": "30d",
#     "min_count": 5,
#     "max_count": 50
#   }
# }
```

## 8. 性能监控

### 8.1 Prometheus 监控集成

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: elasticsearch-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      elasticsearch.k8s.elastic.co/cluster-name: prod-es
  endpoints:
  - port: https
    scheme: https
    path: /_prometheus/metrics
    tlsConfig:
      insecureSkipVerify: true
    bearerTokenSecret:
      name: prod-es-es-elastic-user
      key: elastic
    interval: 30s
---
# Grafana Dashboard 关键指标
# - elasticsearch_cluster_health_status
# - elasticsearch_cluster_health_number_of_nodes
# - elasticsearch_cluster_health_active_shards
# - elasticsearch_indices_docs_primary
# - elasticsearch_indices_store_size_bytes_primary
# - elasticsearch_jvm_memory_used_bytes
# - elasticsearch_thread_pool_*
# - elasticsearch_fs_io_stats_*
```

### 8.2 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: elasticsearch-alerts
  namespace: monitoring
spec:
  groups:
  - name: elasticsearch
    rules:
    - alert: ElasticsearchClusterRed
      expr: elasticsearch_cluster_health_status{color="red"} == 1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "ES 集群状态为 RED"
    - alert: ElasticsearchHighJVMUsage
      expr: |
        elasticsearch_jvm_memory_used_bytes{area="heap"} /
        elasticsearch_jvm_memory_max_bytes{area="heap"} > 0.85
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "ES JVM 堆内存使用率超过 85%"
    - alert: ElasticsearchDiskWatermarkHigh
      expr: elasticsearch_filesystem_data_available_bytes / elasticsearch_filesystem_data_size_bytes < 0.15
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "ES 磁盘可用空间低于 15%"
    - alert: ElasticsearchUnassignedShards
      expr: elasticsearch_cluster_health_unassigned_shards > 0
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "ES 存在未分配分片"
```

## 9. 故障排查速查

| 问题 | 排查命令 | 常见原因 |
|------|---------|---------|
| 集群 RED | `GET _cluster/health?level=indices` | 主分片丢失，检查磁盘/OOM |
| 分片未分配 | `GET _cluster/allocation/explain` | 磁盘水位线、节点不足 |
| 写入拒绝 | `GET _nodes/stats/thread_pool/write` | 写入队列满，增加节点或降低写入速率 |
| 查询慢 | `GET _nodes/stats/thread_pool/search` | 堆内存不足，优化查询 DSL |
| JVM OOM | 检查 Pod events + GC logs | 堆太小或 fielddata 缓存过大 |
| 磁盘满 | `GET _cat/allocation?v` | ILM 未生效，手动删除旧索引 |


<!-- risk-assessed -->
