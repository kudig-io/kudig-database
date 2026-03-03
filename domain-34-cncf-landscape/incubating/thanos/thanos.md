# Thanos

> **成熟度**: Incubating | **加入时间**: 2019-08 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://thanos.io |
| **GitHub** | https://github.com/thanos-io/thanos |
| **文档** | https://thanos.io/tip/thanos/getting-started.md |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Observability |

---

## 项目概述

### 简介
Thanos 是 Prometheus 的高可用、长期存储解决方案。它无缝集成现有 Prometheus 部署，提供全局查询视图、无限数据保留和高可用性，同时保持 Prometheus 的简单性。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2017 | Improbable 公司创建 |
| 2019-08 | 加入 CNCF Sandbox |
| 2020-08 | 晋升为 CNCF Incubating |

### 核心定位
Thanos 是 Prometheus 生态的长期存储标准解决方案，实现多集群指标聚合、无限历史数据存储和全局查询能力。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      Thanos 架构                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Cluster A              Cluster B              Cluster C        │
│  ┌──────────────┐      ┌──────────────┐      ┌──────────────┐  │
│  │  Prometheus  │      │  Prometheus  │      │  Prometheus  │  │
│  │  + Sidecar   │      │  + Sidecar   │      │  + Sidecar   │  │
│  └──────┬───────┘      └──────┬───────┘      └──────┬───────┘  │
│         │                     │                     │           │
│         │ Upload              │ Upload              │ Upload    │
│         ▼                     ▼                     ▼           │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Object Storage                            ││
│  │                (S3 / GCS / Azure Blob)                       ││
│  └─────────────────────────────────────────────────────────────┘│
│         │                     │                     │           │
│         └──────────────┬──────┴──────────────┬──────┘           │
│                        ▼                     ▼                  │
│               ┌─────────────────┐    ┌─────────────────┐       │
│               │     Store       │    │   Compactor     │       │
│               │   Gateway       │    │  (压缩+下采样)  │       │
│               └────────┬────────┘    └─────────────────┘       │
│                        │                                        │
│         ┌──────────────┴──────────────┐                        │
│         ▼                              ▼                        │
│  ┌─────────────────┐          ┌─────────────────┐              │
│  │     Querier     │◄────────►│     Querier     │              │
│  │   (全局查询)    │   HA     │    (全局查询)   │              │
│  └────────┬────────┘          └─────────────────┘              │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                           │
│  │     Grafana     │                                           │
│  └─────────────────┘                                           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 功能 |
|:---|:---|
| **Sidecar** | 与 Prometheus 共存，上传数据到对象存储 |
| **Store Gateway** | 从对象存储读取历史数据 |
| **Querier** | 聚合查询多个数据源 |
| **Compactor** | 数据压缩和下采样 |
| **Ruler** | 分布式规则评估和告警 |
| **Receive** | 远程写入接收器 |

---

## 部署配置

### Sidecar 模式

```yaml
# Prometheus + Thanos Sidecar
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: prometheus
spec:
  template:
    spec:
      containers:
        - name: prometheus
          image: prom/prometheus:v2.48.0
          args:
            - --storage.tsdb.min-block-duration=2h
            - --storage.tsdb.max-block-duration=2h
          volumeMounts:
            - name: data
              mountPath: /prometheus
        
        - name: thanos-sidecar
          image: quay.io/thanos/thanos:v0.32.0
          args:
            - sidecar
            - --tsdb.path=/prometheus
            - --prometheus.url=http://localhost:9090
            - --objstore.config-file=/etc/thanos/objstore.yml
          volumeMounts:
            - name: data
              mountPath: /prometheus
            - name: objstore-config
              mountPath: /etc/thanos
```

### 对象存储配置

```yaml
# objstore.yml
type: S3
config:
  bucket: thanos-metrics
  endpoint: s3.amazonaws.com
  region: us-west-2
  access_key: ${AWS_ACCESS_KEY_ID}
  secret_key: ${AWS_SECRET_ACCESS_KEY}
```

### Querier 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: thanos-querier
spec:
  replicas: 2
  template:
    spec:
      containers:
        - name: querier
          image: quay.io/thanos/thanos:v0.32.0
          args:
            - query
            - --store=thanos-store-gateway:10901
            - --store=prometheus-0.prometheus:10901
            - --store=prometheus-1.prometheus:10901
            - --query.replica-label=replica
          ports:
            - containerPort: 10902  # HTTP
            - containerPort: 10901  # gRPC
```

---

## 数据流

```
┌─────────────────────────────────────────────────────────────────┐
│                    Thanos 数据流                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. 实时数据 (最近 2 小时)                                       │
│     Grafana ──► Querier ──► Sidecar ──► Prometheus              │
│                                                                  │
│  2. 历史数据                                                     │
│     Grafana ──► Querier ──► Store Gateway ──► Object Storage    │
│                                                                  │
│  3. 数据上传                                                     │
│     Prometheus ──► Sidecar ──► Object Storage (每 2 小时)        │
│                                                                  │
│  4. 数据压缩                                                     │
│     Compactor: 原始块 ──► 压缩块 ──► 下采样块                   │
│     保留策略: 5m (原始) → 1h (30天后) → 1d (365天后)            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 使用场景

### 1. 多集群监控
```promql
# 跨集群查询 CPU 使用率
sum by (cluster) (
  rate(container_cpu_usage_seconds_total[5m])
)
```

### 2. 长期趋势分析
```promql
# 查询 1 年内的请求趋势
sum(rate(http_requests_total[1d])) by (service)
```

---

## 参考资源

- [官方文档](https://thanos.io/tip/thanos/getting-started.md)
- [GitHub Repo](https://github.com/thanos-io/thanos)
- [CNCF 项目页面](https://www.cncf.io/projects/thanos/)
- [Kube-Thanos](https://github.com/thanos-io/kube-thanos)

---

**维护者**: Kudig Team | **许可证**: MIT
