---
title: Thanos and Long-Term Metrics Storage on Kubernetes
description: K8s 长期指标存储 — Thanos 架构、对象存储后端、降采样、高可用 Prometheus、Grafana 集成、容量规划
summary: 使用 Thanos 构建 Kubernetes 长期指标存储与多集群监控聚合的生产实践
category: practice
tags:
- thanos
- prometheus
- long-term-storage
- metrics
- observability
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: database
---
# Thanos 长期指标存储生产实践

> 构建跨集群、长期保留的 Prometheus 指标存储体系。

## 架构全景

```
┌─────────────────────────────────────────────────────────┐
│  Cluster A                    Cluster B                  │
│  ┌──────────┐               ┌──────────┐               │
│  │Prometheus│               │Prometheus│               │
│  │  + Sidecar│              │  + Sidecar│              │
│  └─────┬────┘               └─────┬────┘               │
└────────┼──────────────────────────┼─────────────────────┘
         │                          │
         ▼                          ▼
┌─────────────────────────────────────────────────────────┐
│  Object Storage (S3/GCS/MinIO)                          │
│  ┌─────────────────────────────────────────┐            │
│  │  TSDB Blocks (2h → 降采样 → 长期保留)   │            │
│  └─────────────────────────────────────────┘            │
└────────────────────────┬────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┐
         ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│ Thanos Query │ │ Thanos       │ │ Thanos       │
│ (全局查询)   │ │ Compactor    │ │ Store Gateway│
│              │ │ (压缩/降采样)│ │ (历史查询)   │
└──────┬───────┘ └──────────────┘ └──────────────┘
       │
       ▼
┌──────────────┐
│   Grafana    │
└──────────────┘
```

## 部署（kube-thanos / Helm）

### Prometheus + Thanos Sidecar

```yaml
# Prometheus 配置（启用 Thanos Sidecar）
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: main
  namespace: monitoring
spec:
  replicas: 2  # HA 双副本
  thanos:
    image: quay.io/thanos/thanos:v0.35.1
    version: v0.35.1
    objectStorageConfig:
      name: thanos-objstore-config
      key: objstore.yml
    resources:
      requests:
        cpu: 100m
        memory: 256Mi
  retention: 6h  # 本地仅保留 6h（长期存对象存储）
  retentionSize: 10GB
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: gp3-encrypted
        resources:
          requests:
            storage: 50Gi
  resources:
    requests:
      cpu: "1"
      memory: 4Gi
    limits:
      memory: 8Gi
  externalLabels:
    cluster: production-cn-east
    region: cn-east-1
    environment: production
---
# 对象存储配置
apiVersion: v1
kind: Secret
metadata:
  name: thanos-objstore-config
  namespace: monitoring
stringData:
  objstore.yml: |
    type: S3
    config:
      bucket: thanos-metrics
      endpoint: s3.amazonaws.com
      region: cn-east-1
      access_key: ${AWS_ACCESS_KEY_ID}
      secret_key: ${AWS_SECRET_ACCESS_KEY}
      insecure: false
```

### Thanos Query（全局查询层）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: thanos-query
  namespace: monitoring
spec:
  replicas: 2
  selector:
    matchLabels:
      app: thanos-query
  template:
    metadata:
      labels:
        app: thanos-query
    spec:
      containers:
        - name: query
          image: quay.io/thanos/thanos:v0.35.1
          args:
            - query
            - --http-address=0.0.0.0:9090
            - --grpc-address=0.0.0.0:10901
            - --query.replica-label=prometheus_replica
            - --query.replica-label=rule_replica
            - --query.auto-downsampling
            - --query.max-concurrent=20
            - --query.timeout=2m
            # 发现 Sidecar
            - --endpoint=dnssrv+_grpc._tcp.thanos-sidecar.monitoring.svc.cluster.local
            # 发现 Store Gateway
            - --endpoint=dnssrv+_grpc._tcp.thanos-store.monitoring.svc.cluster.local
            # 发现 Ruler
            - --endpoint=dnssrv+_grpc._tcp.thanos-ruler.monitoring.svc.cluster.local
          ports:
            - name: http
              containerPort: 9090
            - name: grpc
              containerPort: 10901
          resources:
            requests:
              cpu: 500m
              memory: 1Gi
            limits:
              memory: 2Gi
          livenessProbe:
            httpGet:
              path: /-/healthy
              port: 9090
          readinessProbe:
            httpGet:
              path: /-/ready
              port: 9090
```

### Thanos Store Gateway

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: thanos-store
  namespace: monitoring
spec:
  replicas: 2
  serviceName: thanos-store
  selector:
    matchLabels:
      app: thanos-store
  template:
    metadata:
      labels:
        app: thanos-store
    spec:
      containers:
        - name: store
          image: quay.io/thanos/thanos:v0.35.1
          args:
            - store
            - --http-address=0.0.0.0:9090
            - --grpc-address=0.0.0.0:10901
            - --objstore.config-file=/etc/thanos/objstore.yml
            - --data-dir=/var/thanos/store
            - --index-cache-size=1GB
            - --chunk-pool-size=2GB
            - --store.grpc.series-max-concurrency=40
            - --block-sync-concurrency=20
            - --min-time=-90d  # 提供 90 天内数据
          ports:
            - name: http
              containerPort: 9090
            - name: grpc
              containerPort: 10901
          resources:
            requests:
              cpu: "1"
              memory: 4Gi
            limits:
              memory: 8Gi
          volumeMounts:
            - name: config
              mountPath: /etc/thanos
            - name: data
              mountPath: /var/thanos/store
      volumes:
        - name: config
          secret:
            secretName: thanos-objstore-config
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        storageClassName: gp3-encrypted
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 100Gi
```

### Thanos Compactor（降采样）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: thanos-compactor
  namespace: monitoring
spec:
  replicas: 1  # 必须单副本
  selector:
    matchLabels:
      app: thanos-compactor
  template:
    metadata:
      labels:
        app: thanos-compactor
    spec:
      containers:
        - name: compactor
          image: quay.io/thanos/thanos:v0.35.1
          args:
            - compact
            - --http-address=0.0.0.0:9090
            - --objstore.config-file=/etc/thanos/objstore.yml
            - --data-dir=/var/thanos/compact
            - --retention.resolution-raw=30d   # 原始精度保留 30 天
            - --retention.resolution-5m=180d   # 5m 降采样保留 180 天
            - --retention.resolution-1h=2y     # 1h 降采样保留 2 年
            - --compact.concurrency=4
            - --downsample.concurrency=4
            - --delete-delay=48h
            - --wait
          resources:
            requests:
              cpu: "1"
              memory: 4Gi
            limits:
              memory: 8Gi
          volumeMounts:
            - name: config
              mountPath: /etc/thanos
            - name: data
              mountPath: /var/thanos/compact
      volumes:
        - name: config
          secret:
            secretName: thanos-objstore-config
        - name: data
          emptyDir: {}
```

## 降采样策略

| 精度 | 保留期 | 用途 | 存储开销 |
|------|--------|------|----------|
| raw（原始） | 30 天 | 实时告警/详细排查 | 100% |
| 5m | 180 天 | 趋势分析/容量规划 | ~20% |
| 1h | 2 年 | 长期趋势/合规审计 | ~5% |

## Grafana 集成

```yaml
# Grafana DataSource 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: grafana-datasources
  namespace: monitoring
data:
  datasources.yaml: |
    apiVersion: 1
    datasources:
      - name: Thanos
        type: prometheus
        access: proxy
        url: http://thanos-query.monitoring:9090
        isDefault: true
        jsonData:
          timeInterval: "30s"
          queryTimeout: "60s"
          httpMethod: POST
```

## 容量规划

### 存储估算公式

```
每日存储 = 活跃时间序列数 × 采样间隔 × 每样本字节 × 86400
示例：100 万时间序列 × 30s 间隔 × 1.5 字节 × 86400 / 30 = ~12.5 GB/天（原始）
```

| 集群规模 | 活跃序列 | 日增量(raw) | 30天(raw) | 1年(含降采样) |
|----------|----------|-------------|-----------|---------------|
| 小（50节点） | 50万 | ~6 GB | ~180 GB | ~300 GB |
| 中（200节点） | 200万 | ~25 GB | ~750 GB | ~1.2 TB |
| 大（1000节点） | 1000万 | ~125 GB | ~3.7 TB | ~6 TB |

## 高可用设计

| 组件 | HA 策略 | 注意事项 |
|------|---------|----------|
| Prometheus | 双副本 + externalLabels | replica-label 去重 |
| Query | 2+ 副本 + LB | 无状态 |
| Store Gateway | 2+ 副本 | 缓存预热 |
| Compactor | 单副本（必须） | 多副本会损坏数据 |
| Ruler | 双副本 + replica-label | 避免重复告警 |

## 故障排查

```bash
# 检查组件状态
kubectl get pods -n monitoring -l app.kubernetes.io/name=thanos
kubectl logs -n monitoring thanos-query-0 --tail=50

# 检查对象存储连通性
kubectl exec -n monitoring thanos-store-0 -- \
  thanos tools bucket ls --objstore.config-file=/etc/thanos/objstore.yml

# 查看块信息
kubectl exec -n monitoring thanos-store-0 -- \
  thanos tools bucket inspect --objstore.config-file=/etc/thanos/objstore.yml

# Query 连接的 Store 数量
curl -s http://thanos-query:9090/api/v1/status/flags | jq '.data["query.replica-label"]'
curl -s http://thanos-query:9090/stores | grep -c "UP"
```

| 症状 | 原因 | 解决 |
|------|------|------|
| 查询超时 | 时间范围过大/序列过多 | 缩小范围 + 降采样 |
| 数据间隙 | Sidecar 上传失败 | 检查对象存储权限 |
| 重复数据 | replica-label 未配置 | 添加 --query.replica-label |
| Compactor 停止 | 磁盘满/权限错误 | 检查 data-dir + 对象存储 |
| 内存 OOM | 查询并发过高 | 限制 max-concurrent |

## Related

- [[07-数据库中间件/04-时序数据库/index.md|时序数据库]]
- [[07-数据库中间件/04-时序数据库/01-prometheus-tsdb-deep-dive.md|Prometheus TSDB]]
- [[09-可观测性/02-指标/index.md|指标监控]]
