---
title: Cortex (entities)
description: '## 概述'
summary: 'Cortex 是多租户、水平可扩展的 Prometheus 即服务解决方案。它为 Prometheus 提供长期存储、高可用性和全局视图能力，适合大规模 Kubernetes 监控场景。'
category: entities
tags:
- k8s
- cncf
- observability
- cortex
- prometheus
- grafana
- containerd
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cortex 是什么
- 如何 Cortex
trigger_keywords:
- Cortex
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---



# Cortex

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go

## 概述

Cortex 是多租户、水平可扩展的 Prometheus 即服务解决方案。它为 Prometheus 提供长期存储、高可用性和全局视图能力，适合大规模 Kubernetes 监控场景。

## 核心能力

- **多租户**: 完全隔离的租户数据和查询
- **水平扩展**: 所有组件可独立扩展
- **长期存储**: 支持 S3、GCS、Azure Blob 等对象存储
- **高可用**: 数据复制和问题自动转移
- **兼容 Prometheus**: 完全兼容 PromQL 和 remote write
- **全局视图**: 聚合多个 Prometheus 的数据

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **容量规划**: 根据每秒样本数和活跃序列数规划 Ingester 资源
- **存储选择**: 使用对象存储而非本地磁盘
- **查询优化**: 启用 Query Frontend 缓存和分片
- **租户隔离**: 为不同租户配置合理的限制
- **监控告警**: 监控 Cortex 组件健康状态

## 架构定位

在 CNCF 生态中，cortex 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[concepts/observability-pillars.md|observability-pillars]]
- [[concepts/storage-model.md|storage-model]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[keylime]] — Keylime
- [[openebs]] — OpenEBS
- [[05-containerd-windows-support]] — containerd Windows 支持
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cortex
- [[entities/observability-terms.md|K8s 可观测性术语参考]] — Cross-reference
- [[entities/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
