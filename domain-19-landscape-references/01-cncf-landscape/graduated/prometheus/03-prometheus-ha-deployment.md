---
title: Prometheus 高可用部署
description: 'title: Prometheus 高可用部署'
category: general
tags:
- cncf
- ecosystem
- prometheus
- deployment
- apiserver
- grafana
- minio
- statefulset
- job
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Prometheus 高可用部署 是什么
- 如何 Prometheus 高可用部署
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Prometheus
- 高可用部署
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
---

title: Prometheus 高可用部署
description: Prometheus 高可用部署指南，涵盖联邦集群、Thanos、Mimir架构、存储方案和容量规划
category: cncf-landscape
tags:
- k8s
- cncf
- prometheus
- high-availability
- thanos
- federation
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_reading_time: 10min
intent_queries:
- Prometheus 高可用部署
- Prometheus 联邦集群
- Thanos 架构
trigger_keywords:
- Prometheus
- HA
- 高可用
- 联邦
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/deployment-fta.md
  label: '故障树: deployment'
estimated_read_time: 10min
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
# Prometheus 高可用部署

> **适用版本**: Prometheus 2.50+ | **最后更新**: 2026-05

---

## 1. 高可用架构概述

### 1.1 架构演进

```
阶段一：单机部署
┌─────────────────────┐
│   Prometheus Server  │
│  ┌─────────────────┐ │
│  │  TSDB (本地)   │ │
│  └─────────────────┘ │
│  限制：单点、有限存储 │
└─────────────────────┘

阶段二：联邦集群
┌──────────┐  ┌──────────┐  ┌──────────┐
│Prometheus│  │Prometheus│  │Prometheus│
│  (Global)│◄─┤(DC1)    │  │(DC2)    │
└──────────┘  └──────────┘  └──────────┘
     │              │              │
     ▼              ▼              ▼
┌──────────┐  ┌──────────┐  ┌──────────┐
│  Targets  │  │  Targets │  │  Targets │
└──────────┘  └──────────┘  └──────────┘

阶段三：Thanos 架构
┌────────────────────────────────────────┐
│              Thanos Query               │
│         ┌──────────────────┐            │
│         │  Query Frontend  │            │
│         └──────────────────┘            │
└──────────────────┬─────────────────────┘
                  │
     ┌────────────┼────────────┐
     ▼            ▼            ▼
┌─────────┐  ┌─────────┐  ┌─────────┐
│ Sidecar │  │ Sidecar │  │ Ruler   │
│ (TSDB)  │  │ (TSDB)  │  │         │
└────┬────┘  └────┬────┘  └────┬────┘
     │            │            │
     └────────────┼────────────┘
                  ▼
         ┌─────────────────┐
         │  Object Store  │
         │  (S3/GCS/MinIO)│
         └─────────────────┘
```

### 1.2 选型决策

| 场景 | 推荐方案 | 原因 |
|:-----|:---------|:-----|
| < 100K series | 单机/联邦 | 简单易用 |
| 100K-500K series | Thanos | 成熟稳定 |
| 500K+ series | Thanos/Mimir | 水平扩展 |
| 多租户 | Mimir/Grafana Cloud | 原生多租户 |

---

## 2. Prometheus 联邦集群

### 2.1 联邦配置

```yaml
# 全局 Prometheus (federation)
global:
  scrape_interval: 30s
  evaluation_interval: 30s

scrape_configs:
  - job_name: 'federate'
    honor_labels: true
    metrics_path: /federate
    params:
      'match[]':
        - '{job="kubernetes-nodes"}'
        - '{job="kubernetes-pods"}'
        - '{__name__=~"node_.*"}'
    static_configs:
      - targets:
          - 'prometheus-dc1:9090'
          - 'prometheus-dc2:9090'
```

```yaml
# DC1 Prometheus (联邦被拉取端)
global:
  scrape_interval: 15s
  external_labels:
    cluster: dc1
    replica: a
```

### 2.2 联邦层级设计

```yaml
# 顶层全局聚合
scrape_configs:
  - job_name: 'global-federate'
    honor_labels: true
    metrics_path: /federate
    params:
      'match[]':
        # 聚合关键指标
        - '{__name__=~"node_cpu|node_memory|node_network"}'
        - '{job="kubernetes-pods"}'
        - '{__name__=~"apiserver_.*"}'
    static_configs:
      - targets:
          - 'prometheus-dc1:9090'
          - 'prometheus-dc2:9090'
          - 'prometheus-dc3:9090'
```

---

## 3. Thanos 架构部署

### 3.1 核心组件

| 组件 | 功能 | 必需 |
|:-----|:-----|:-----|
| **Sidecar** | 连接 Prometheus 与对象存储 | ✓ |
| **Store** | 从对象存储读取历史数据 | ✓ |
| **Query** | 统一查询入口 | ✓ |
| **Query Frontend** | 查询缓存与负载均衡 | 可选 |
| **Ruler** | 在对象存储上执行告警/记录规则 | 可选 |
| **Receive** | 接收远程写入数据 | 可选 |

### 3.2 Kubernetes 部署

```yaml
# Thanos Sidecar (与 Prometheus 同 Pod)
apiVersion: v1
kind: ConfigMap
metadata:
  name: thanos-sidecar-config
  namespace: monitoring
data:
  config.yaml: |
    type: S3
    config:
      bucket: prometheus-data
      endpoint: minio.monitoring.svc:9000
      access_key: thanos
      secret_key: secret
      insecure: false
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: prometheus
  namespace: monitoring
spec:
  serviceName: prometheus
  replicas: 2
  selector:
    matchLabels:
      app: prometheus
  template:
    metadata:
      labels:
        app: prometheus
    spec:
      containers:
      - name: prometheus
        image: prom/prometheus:v2.50.0
        args:
          - '--storage.tsdb.path=/prometheus'
          - '--storage.tsdb.retention.time=15d'
          - '--web.enable-lifecycle'
        volumeMounts:
          - name: prometheus-data
            mountPath: /prometheus
      - name: thanos-sidecar
        image: quay.io/thanos/thanos:v0.34.0
        args:
          - sidecar
          - '--prometheus.url=http://localhost:9090'
          - '--objstore.config-file=/etc/thanos/config.yaml'
        volumeMounts:
          - name: thanos-config
            mountPath: /etc/thanos
            readOnly: true
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            memory: 256Mi
```

### 3.3 Thanos Store Gateway

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: thanos-store
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: thanos-store
  template:
    metadata:
      labels:
        app: thanos-store
    spec:
      containers:
      - name: thanos
        image: quay.io/thanos/thanos:v0.34.0
        args:
          - store
          - '--objstore.config-file=/etc/thanos/config.yaml'
          - '--data-dir=/data'
          - '--index-cache-size=250MB'
          - '--chunk-pool-size=2GB'
        resources:
          requests:
            cpu: 500m
            memory: 2Gi
          limits:
            memory: 4Gi
        volumeMounts:
          - name: thanos-config
            mountPath: /etc/thanos
          - name: thanos-data
            mountPath: /data
```

### 3.4 Thanos Query

```yaml
apiVersion: v1
kind: Service
metadata:
  name: thanos-query
  namespace: monitoring
spec:
  type: ClusterIP
  ports:
    - name: http
      port: 9090
      targetPort: 9090
  selector:
    app: thanos-query
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: thanos-query
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: thanos-query
  template:
    metadata:
      labels:
        app: thanos-query
    spec:
      containers:
      - name: thanos
        image: quay.io/thanos/thanos:v0.34.0
        args:
          - query
          - '--store=dnssrv+_grpc._tcp.thanos-store.monitoring.svc.cluster.local'
          - '--store=dnssrv+_grpc._tcp.prometheus.monitoring.svc.cluster.local'
          - '--query.timeout=2m'
          - '--query.max-concurrent=20'
        ports:
          - name: http
            containerPort: 9090
```

---

## 4. Prometheus Remote Write

### 4.1 Remote Write 配置

```yaml
global:
  external_labels:
    cluster: dc1
    env: production

remote_write:
  - url: https://thanos-receive.example.com/api/v1/receive
    tls_config:
      ca_file: /etc/prometheus/certs/ca.crt
    bearer_token_file: /var/run/secrets/tokens/token
    queue_config:
      capacity: 10000
      max_shards: 30
      min_shards: 5
      max_samples_per_send: 2000
      batch_send_deadline: 30s
    metadata_config:
      send: true
      send_interval: 1m
```

### 4.2 Mimir Remote Write

```yaml
remote_write:
  - url: http://mimir-distributed-gateway.monitoring.svc:8080/api/v1/push
    queue_config:
      capacity: 500000
      max_shards: 50
      min_shards: 10
      max_samples_per_send: 5000
      batch_send_deadline: 20s
    metadata_config:
      send: true
      send_interval: 1m
```

---

## 5. 数据存储容量规划

### 5.1 存储估算公式

```
存储需求 = (样本数 × 样本大小 × 保留时间) × 压缩比 × 副本数

示例:
- 100,000 时间序列
- 每样本 1KB
- 保留 30 天
- 压缩比 10:1
- 副本数 1

存储 = 100,000 × 1KB × 30天 × 24小时 × 2采样/分 / 10 / 1024 / 1024
     ≈ 13 GB
```

### 5.2 存储规划表

| 时间序列数 | 保留时间 | 原始存储 | 压缩后存储 |
|:--------:|:-------:|:-------:|:---------:|
| 10K | 15d | 13 GB | 1.3 GB |
| 50K | 30d | 130 GB | 13 GB |
| 100K | 30d | 260 GB | 26 GB |
| 200K | 90d | 1.5 TB | 150 GB |
| 500K | 90d | 3.7 TB | 370 GB |

### 5.3 TSDB 资源配置

```yaml
# Prometheus 资源配置建议
resources:
  requests:
    cpu: "2"
    memory: "8Gi"
  limits:
    cpu: "4"
    memory: "16Gi"

# 存储配置
args:
  - '--storage.tsdb.path=/prometheus'
  - '--storage.tsdb.wal-compression'
  - '--storage.tsdb.retention.time=15d'
  - '--storage.tsdb.blocks-rotation=2h'
```

---

## 6. 监控 Prometheus 自身

### 6.1 Prometheus 自身指标

```yaml
# Prometheus 自身监控
scrape_configs:
  - job_name: 'prometheus'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: /metrics
    relabel_configs:
      - source_labels: [__address__]
        target_label: instance
        replacement: 'prometheus-{{ $env }}'
```

### 6.2 关键监控指标

```promql
# 查询处理时间
prometheus_engine_query_duration_seconds

# 样本采集速率
rate(prometheus_tsdb_head_samples_appended_total[5m])

# 告警队列长度
prometheus_notifications_queue_length

# 远程写入速率
rate(prometheus_remote_storage_samples_pending_total[5m])

# TSDB 压缩
rate(prometheus_tsdb_compactions_failed_total[5m])
```

### 6.3 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: prometheus-self-monitoring
  namespace: monitoring
spec:
  groups:
    - name: prometheus-self
      rules:
        - alert: PrometheusHighQueryLatency
          expr: |
            histogram_quantile(0.99, 
              rate(prometheus_engine_query_duration_seconds_bucket[5m])
            ) > 2
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Prometheus 查询延迟过高"
            
        - alert: PrometheusTSDBCompressionFailing
          expr: |
            rate(prometheus_tsdb_compactions_failed_total[5m]) > 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "TSDB 压缩失败"
            
        - alert: PrometheusRemoteWriteBehind
          expr: |
            rate(prometheus_remote_storage_samples_pending_total[5m]) > 10000
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "远程写入队列积压"
```

## Related

- [[domain-19-landscape-references/04-cncf-fta-index.md|04-cncf-fta-index]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/thanos.md|thanos]]
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference

## See Also

- [[domain-19-landscape-references/graduated/prometheus/prometheus.md|prometheus]]
- [[domain-19-landscape-references/graduated/prometheus/02-prometheus-promql-advanced.md|02-prometheus-promql-advanced]]
- [[domain-19-landscape-references/graduated/prometheus/prometheus.md|prometheus]]
- [[domain-19-landscape-references/graduated/prometheus/02-prometheus-promql-advanced.md|02-prometheus-promql-advanced]]
