---
title: Jaeger 生产环境部署
description: 'Jaeger 生产部署：all-in-one 到 production 架构迁移、存储后端 (ES/Cassandra)、采样策略、Agent/Collector 配置、与 OTel 集成'
summary: 'Jaeger 生产架构、存储后端、采样策略与 OpenTelemetry 集成'
category: observability
tags:
- jaeger
- distributed-tracing
- elasticsearch
- cassandra
- opentelemetry
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Jaeger 生产环境部署是什么
- 如何部署 Jaeger production 架构
trigger_keywords:
- Jaeger
- 分布式追踪
- Collector
- Agent
- 采样策略
prerequisites:
- kubectl-basics
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

# Jaeger 生产环境部署

## 概述

Jaeger 是 Uber 开源的分布式追踪系统，支持 OpenTracing 和 OpenTelemetry 标准。本文档涵盖从 all-in-one 开发模式到生产级架构的完整部署方案。

## 1. 架构演进

### 1.1 all-in-one 模式（开发/测试）

```
┌─────────────────────────────────┐
│         all-in-one              │
│  ┌───────┐ ┌──────────┐ ┌────┐ │
│  │ Agent │→│ Collector│→│ UI │ │
│  └───────┘ └──────────┘ └────┘ │
│           │                      │
│           ▼                      │
│     ┌─────────┐                  │
│     │ Badger  │                  │
│     │(嵌入式) │                  │
│     └─────────┘                  │
└─────────────────────────────────┘
```

### 1.2 Production 模式

```
┌──────────────────────────────────────────────────────────────┐
│                     Production Architecture                   │
│                                                              │
│  ┌─────────┐   ┌─────────┐   ┌─────────┐                    │
│  │ Agent   │   │ Agent   │   │ Agent   │  DaemonSet (每节点) │
│  │ (Node1) │   │ (Node2) │   │ (Node3) │                    │
│  └────┬────┘   └────┬────┘   └────┬────┘                    │
│       │             │             │                           │
│       └─────────────┼─────────────┘                          │
│                     ▼                                         │
│  ┌─────────────────────────────────┐                         │
│  │     Collector (Deployment)      │  多副本 + HPA           │
│  │  ┌────────┐ ┌────────┐ ┌────┐  │                         │
│  │  │Coll-1  │ │Coll-2  │ │Coll│  │                         │
│  │  └────┬───┘ └────┬───┘ └──┬─┘  │                         │
│  └───────┼──────────┼────────┼────┘                         │
│          └──────────┼────────┘                               │
│                     ▼                                         │
│  ┌─────────────────────────────────┐                         │
│  │     Storage Backend             │                         │
│  │  ┌──────────┐  ┌───────────┐   │                         │
│  │  │Elasticsearch│ │Cassandra │   │                         │
│  │  │ (推荐)    │  │ (大规模) │   │                         │
│  │  └──────────┘  └───────────┘   │                         │
│  └─────────────────────────────────┘                         │
│                     ▼                                         │
│  ┌─────────┐                                                 │
│  │   UI    │  Deployment + Ingress                           │
│  └─────────┘                                                 │
└──────────────────────────────────────────────────────────────┘
```

## 2. 生产部署配置

### 2.1 Jaeger Operator 部署

```bash
# 安装 Jaeger Operator
kubectl create namespace observability
kubectl apply -f https://github.com/jaegertracing/jaeger-operator/releases/download/v1.54.0/jaeger-operator.yaml -n observability
```

### 2.2 Jaeger CR 定义

```yaml
apiVersion: jaegertracing.io/v1
kind: Jaeger
metadata:
  name: production
  namespace: observability
spec:
  strategy: production

  collector:
    replicas: 3
    autoscale: true
    minReplicas: 2
    maxReplicas: 10
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: "2"
        memory: 2Gi
    options:
      collector:
        num-workers: 50
        queue-size: 5000
      es:
        server-urls: https://elasticsearch.observability.svc:9200
        index-prefix: jaeger
        num-shards: 5
        num-replicas: 1
        tls:
          ca: /es/certificates/ca.crt

  query:
    replicas: 2
    resources:
      requests:
        cpu: 200m
        memory: 256Mi
      limits:
        cpu: "1"
        memory: 1Gi
    options:
      query:
        base-path: /jaeger
      es:
        server-urls: https://elasticsearch.observability.svc:9200
        index-prefix: jaeger

  agent:
    strategy: DaemonSet
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 200m
        memory: 256Mi
    options:
      processor:
        jaeger-compact:
          server-host-port: ":6831"
        jaeger-binary:
          server-host-port: ":6832"

  storage:
    type: elasticsearch
    options:
      es:
        server-urls: https://elasticsearch.observability.svc:9200
        index-prefix: jaeger
        num-shards: 5
        num-replicas: 1
        bulk:
          size: 5242880
          workers: 1
          flush-interval: 200ms
    esIndexCleaner:
      enabled: true
      numberOfDays: 30
      schedule: "55 23 * * *"
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
    esRollover:
      enabled: true
      schedule: "0 */12 * * *"
      conditions: '{"max_age":"2d","max_docs":10000000}'
      readTTL: 168h  # 7 天
```

## 3. 存储后端配置

### 3.1 Elasticsearch 后端

```yaml
# Elasticsearch 集群配置（ECK Operator）
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: jaeger-es
  namespace: observability
spec:
  version: 8.12.0
  nodeSets:
  - name: master
    count: 3
    config:
      node.roles: ["master"]
      xpack.security.transport.ssl.enabled: true
      xpack.security.http.ssl.enabled: true
    podTemplate:
      spec:
        containers:
        - name: elasticsearch
          resources:
            requests:
              cpu: "1"
              memory: 4Gi
            limits:
              cpu: "2"
              memory: 8Gi
        volumes:
        - name: es-data
          emptyDir: {}

  - name: data
    count: 3
    config:
      node.roles: ["data", "ingest"]
      xpack.security.transport.ssl.enabled: true
      xpack.security.http.ssl.enabled: true
    podTemplate:
      spec:
        containers:
        - name: elasticsearch
          resources:
            requests:
              cpu: "2"
              memory: 8Gi
            limits:
              cpu: "4"
              memory: 16Gi
    volumeClaimTemplates:
    - metadata:
        name: es-data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 500Gi
```

### 3.2 Cassandra 后端

```yaml
# Cassandra 集群配置（适用于大规模场景）
apiVersion: cassandra.datastax.com/v1beta1
kind: CassandraDatacenter
metadata:
  name: jaeger-cassandra
  namespace: observability
spec:
  clusterName: jaeger-cluster
  serverType: cassandra
  serverVersion: "4.1.0"
  size: 3
  storageConfig:
    cassandraDataVolumeClaimSpec:
      storageClassName: fast-ssd
      accessModes:
      - ReadWriteOnce
      resources:
        requests:
          storage: 500Gi
  config:
    jvm-server-options:
      initial_heap_size: "4G"
      max_heap_size: "4G"
    cassandra-yaml:
      num_tokens: 256
      concurrent_reads: 32
      concurrent_writes: 64
      memtable_allocation_type: offheap_objects
```

## 4. 采样策略

### 4.1 采样策略类型

| 策略 | 说明 | 适用场景 |
|------|------|---------|
| `const` | 固定采样（全采或全不采） | 开发环境、小流量 |
| `probabilistic` | 按概率采样 | 中等流量 |
| `rateLimiting` | 限制每秒采样数 | 成本控制 |
| `adaptive` | 自适应采样 | 生产推荐 |

### 4.2 Remote Sampling 配置

```yaml
# 采样策略配置（存储在 ConfigMap）
apiVersion: v1
kind: ConfigMap
metadata:
  name: jaeger-sampling-strategies
  namespace: observability
data:
  strategies.json: |
    {
      "default_strategy": {
        "type": "probabilistic",
        "param": 0.01
      },
      "service_strategies": [
        {
          "service": "payment-service",
          "type": "adaptive",
          "param": 0.5,
          "operation_strategies": [
            {
              "operation": "processPayment",
              "type": "probabilistic",
              "param": 1.0
            }
          ]
        },
        {
          "service": "api-gateway",
          "type": "rateLimiting",
          "param": 100
        }
      ],
      "operation_strategies": [
        {
          "operation": "/health",
          "type": "probabilistic",
          "param": 0.0
        },
        {
          "operation": "/metrics",
          "type": "probabilistic",
          "param": 0.0
        }
      ]
    }
```

### 4.3 Agent 自适应采样

```yaml
# Jaeger Agent 配置远程采样
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: jaeger-agent
  namespace: observability
spec:
  template:
    spec:
      containers:
      - name: jaeger-agent
        args:
        - --reporter.grpc.host-port=jaeger-collector:14250
        - --processor.jaeger-compact.server-host-port=:6831
        - --sampling.strategies-file=/etc/jaeger/sampling.json
        - --sampling.refresh-interval=1m
        volumeMounts:
        - name: sampling-config
          mountPath: /etc/jaeger
          readOnly: true
      volumes:
      - name: sampling-config
        configMap:
          name: jaeger-sampling-strategies
```

## 5. Agent 与 Collector 配置

### 5.1 Agent 高级配置

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: jaeger-agent
  namespace: observability
spec:
  selector:
    matchLabels:
      app: jaeger-agent
  template:
    metadata:
      labels:
        app: jaeger-agent
    spec:
      hostNetwork: true
      dnsPolicy: ClusterFirstWithHostNet
      tolerations:
      - operator: Exists
      containers:
      - name: jaeger-agent
        image: jaegertracing/jaeger-agent:1.54
        args:
        - --reporter.grpc.host-port=jaeger-collector.observability.svc:14250
        - --reporter.grpc.retry.max=10
        - --processor.jaeger-compact.server-host-port=:6831
        - --processor.jaeger-binary.server-host-port=:6832
        - --processor.zipkin-compact.server-host-port=:5775
        - --admin.http.host-port=:14271
        - --log-level=info
        ports:
        - containerPort: 6831
          protocol: UDP
          name: compact
        - containerPort: 6832
          protocol: UDP
          name: binary
        - containerPort: 5775
          protocol: UDP
          name: zipkin
        - containerPort: 14271
          protocol: TCP
          name: admin
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
        livenessProbe:
          httpGet:
            path: /
            port: 14271
          initialDelaySeconds: 5
        readinessProbe:
          httpGet:
            path: /
            port: 14271
          initialDelaySeconds: 5
```

### 5.2 Collector 高级配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: jaeger-collector
  namespace: observability
spec:
  replicas: 3
  selector:
    matchLabels:
      app: jaeger-collector
  template:
    metadata:
      labels:
        app: jaeger-collector
    spec:
      containers:
      - name: jaeger-collector
        image: jaegertracing/jaeger-collector:1.54
        args:
        - --collector.num-workers=50
        - --collector.queue-size=5000
        - --collector.grpc.host-port=:14250
        - --collector.http.host-port=:14268
        - --collector.zipkin.host-port=:9411
        - --es.server-urls=https://elasticsearch.observability.svc:9200
        - --es.index-prefix=jaeger
        - --es.num-shards=5
        - --es.num-replicas=1
        - --es.bulk.size=5242880
        - --es.bulk.workers=1
        - --es.bulk.flush-interval=200ms
        - --sampling.strategies-file=/etc/jaeger/sampling.json
        - --admin.http.host-port=:14269
        ports:
        - containerPort: 14250
          name: grpc
        - containerPort: 14268
          name: http
        - containerPort: 9411
          name: zipkin
        - containerPort: 14269
          name: admin
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: "2"
            memory: 2Gi
        livenessProbe:
          httpGet:
            path: /
            port: 14269
          initialDelaySeconds: 5
        readinessProbe:
          httpGet:
            path: /
            port: 14269
          initialDelaySeconds: 5
        volumeMounts:
        - name: sampling-config
          mountPath: /etc/jaeger
          readOnly: true
        - name: es-tls
          mountPath: /es/certificates
          readOnly: true
      volumes:
      - name: sampling-config
        configMap:
          name: jaeger-sampling-strategies
      - name: es-tls
        secret:
          secretName: jaeger-es-tls
```

## 6. 与 OpenTelemetry 集成

### 6.1 OTel Collector → Jaeger

```yaml
# OpenTelemetry Collector 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: otel-collector-config
  namespace: observability
data:
  config.yaml: |
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: 0.0.0.0:4317
          http:
            endpoint: 0.0.0.0:4318
      jaeger:
        protocols:
          thrift_compact:
            endpoint: 0.0.0.0:6831
          thrift_binary:
            endpoint: 0.0.0.0:6832
          grpc:
            endpoint: 0.0.0.0:14250

    processors:
      batch:
        timeout: 5s
        send_batch_size: 1024
      memory_limiter:
        check_interval: 1s
        limit_mib: 512
        spike_limit_mib: 128

    exporters:
      otlp/jaeger:
        endpoint: jaeger-collector.observability.svc:4317
        tls:
          insecure: true

    service:
      pipelines:
        traces:
          receivers: [otlp, jaeger]
          processors: [memory_limiter, batch]
          exporters: [otlp/jaeger]
```

### 6.2 应用直接发送到 OTel Collector

```yaml
# 应用 Deployment 配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  template:
    spec:
      containers:
      - name: app
        env:
        - name: OTEL_EXPORTER_OTLP_ENDPOINT
          value: "http://otel-collector.observability.svc:4317"
        - name: OTEL_SERVICE_NAME
          value: "my-app"
        - name: OTEL_TRACES_SAMPLER
          value: "parentbased_traceidratio"
        - name: OTEL_TRACES_SAMPLER_ARG
          value: "0.1"
        - name: OTEL_RESOURCE_ATTRIBUTES
          value: "service.version=1.0.0,deployment.environment=production"
```

## 7. 最佳实践

```
Jaeger 生产部署检查清单：

□ 使用 production 策略部署（非 all-in-one）
□ 配置 Elasticsearch 或 Cassandra 作为存储后端
□ 启用 Collector 自动伸缩
□ 配置合适的采样策略（推荐 adaptive）
□ 设置存储保留策略（ES Index Cleaner/Rollover）
□ 配置 Agent DaemonSet（每节点一个）
□ 启用 Collector 队列监控
□ 配置 TLS 加密通信
□ 使用 OTel Collector 作为统一入口
□ 定期清理过期 Trace 数据
```

## Related

- [[domain-06-observability/04-tracing/02-grafana-tempo-tracing|Grafana Tempo]]
- [[domain-06-observability/04-tracing/03-opentelemetry-collector-patterns|OTel Collector 配置模式]]

## See Also

- [Jaeger 文档](https://www.jaegertracing.io/docs/)
- [Jaeger Operator](https://www.jaegertracing.io/docs/latest/operator/)
