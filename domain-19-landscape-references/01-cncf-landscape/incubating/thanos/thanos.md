---
title: Thanos
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- grafana
- statefulset
- gateway
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Thanos 是什么
- 如何 Thanos
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Thanos
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
---

title: Thanos
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- statefulset
- gateway
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Thanos 是什么
- 如何 Thanos
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Thanos
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

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/thanos.md|thanos]]
- [[references/release-notes-observability|发布说明索引 — 可观测性]] — Cross-reference
- [[references/k8s-observability-ecosystem|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[concepts/observability-stack-evolution|可观测性栈演进]] — Cross-reference
- [[skills/monitoring-fta|监控与告警异常故障树分析]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.12|thanos v0.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.26|thanos v0.26 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.8|thanos v0.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.36|thanos v0.36 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.22|thanos v0.22 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.16|thanos v0.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.32|thanos v0.32 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.23|thanos v0.23 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.17|thanos v0.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.33|thanos v0.33 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.13|thanos v0.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.27|thanos v0.27 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.9|thanos v0.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.37|thanos v0.37 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.18|thanos v0.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.28|thanos v0.28 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.38|thanos v0.38 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.6|thanos v0.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.29|thanos v0.29 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.39|thanos v0.39 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.7|thanos v0.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.19|thanos v0.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.4|thanos v0.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.5|thanos v0.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.20|thanos v0.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.14|thanos v0.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.30|thanos v0.30 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.10|thanos v0.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.41|thanos v0.41 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.24|thanos v0.24 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.34|thanos v0.34 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.11|thanos v0.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.40|thanos v0.40 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.25|thanos v0.25 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.35|thanos v0.35 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.21|thanos v0.21 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.15|thanos v0.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/observability/thanos/RELEASE-NOTES-0.31|thanos v0.31 Release Notes]]
