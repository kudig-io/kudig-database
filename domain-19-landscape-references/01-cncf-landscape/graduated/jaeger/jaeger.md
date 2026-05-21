---
title: Jaeger
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- jaeger
- helm
- docker
- kafka
- elasticsearch
- rag
- agent
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Jaeger 是什么
- 如何 Jaeger
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Jaeger
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- tracing-basics
---

title: Jaeger
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- jaeger
- helm
- docker
- kafka
- elasticsearch
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
- Jaeger 是什么
- 如何 Jaeger
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Jaeger
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
# Jaeger

> **成熟度**: Graduated | **加入时间**: 2017-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.jaegertracing.io |
| **GitHub** | https://github.com/jaegertracing/jaeger |
| **文档** | https://www.jaegertracing.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Observability |

---

## 项目概述

### 简介
Jaeger 是一个开源的端到端分布式追踪系统，由 Uber 开发，用于监控和故障排查微服务架构。

### 核心定位
Jaeger 提供分布式上下文传播、分布式事务监控、根因分析、服务依赖分析和性能优化支持。

### 发展历程
- **2015**: Uber 内部开发
- **2017-04**: 开源
- **2017-09**: 加入 CNCF
- **2019-10**: 成为 CNCF 毕业项目

---

## 核心功能

### 主要特性
- **分布式追踪**: 跨服务请求追踪
- **根因分析**: 快速定位故障
- **服务依赖**: 可视化服务关系图
- **性能分析**: 延迟分析和优化
- **多存储后端**: Cassandra、Elasticsearch、Kafka

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                         Jaeger                              │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│
│  │   Agent     │ │  Collector  │ │       Query             ││
│  │ (UDP recv)  │ │  (Storage)  │ │     (UI/API)            ││
│  └─────────────┘ └─────────────┘ └─────────────────────────┘│
│  ┌─────────────────────────────────────────────────────────┐│
│  │              Storage (ES/Cassandra)                     ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装部署
```bash
# All-in-one (开发测试)
docker run -d --name jaeger \
  -p 16686:16686 \
  -p 6831:6831/udp \
  jaegertracing/all-in-one:latest

# Kubernetes (Helm)
helm repo add jaegertracing https://jaegertracing.github.io/helm-charts
helm install jaeger jaegertracing/jaeger
```

### 验证测试
```bash
# 访问 Jaeger UI
open http://localhost:16686
```

---

## 参考资源

- [官方文档](https://www.jaegertracing.io/docs)
- [GitHub Repo](https://github.com/jaegertracing/jaeger)
- [CNCF 项目页面](https://www.cncf.io/projects/jaeger/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[references/k8s-observability-ecosystem|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[references/observability-terms|K8s 可观测性术语参考]] — Cross-reference
- [[references/tooling-terms|K8s 工具链术语参考]] — Cross-reference
- [[synthesis/可观测性支柱 × Prometheus-Grafana|可观测性支柱 × Prometheus-Grafana]] — Cross-reference
- [[concepts/bp-observability|最佳实践：Observability]] — Cross-reference
- [[concepts/observability-stack-evolution|可观测性栈演进]] — Cross-reference
- [[skills/k8s-distributed-tracing-guide|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
