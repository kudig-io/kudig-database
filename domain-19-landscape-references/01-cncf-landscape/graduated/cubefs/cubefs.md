---
title: CubeFS (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- cubefs
- prometheus
- grafana
- crd
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CubeFS 是什么
- 如何 CubeFS
trigger_keywords:
- CubeFS
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- observability-basics
created: "2026-05-23"
---

# CubeFS

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **多协议支持**: POSIX、S3、HDFS 接口兼容
- **弹性扩展**: 元数据和数据节点独立扩展
- **多租户**: 资源隔离、配额管理
- **纠删码**: 高效存储空间利用
- **多级缓存**: 本地缓存加速
- **AI/ML 优化**: 大规模数据集处理优化

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 至少 3 个 Master 节点
- MetaNode 使用 SSD
- DataNode 可使用 HDD
- 配置合理的副本数或纠删码策略
- 启用本地缓存
- 调整 Block Size

## 架构定位

在 CNCF 生态中，cubefs 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana|prometheus-grafana]]
- [[concepts/storage-model|storage-model]]
- [[entities/[[csi-drivers]]|csi-drivers]]

## Related

- [[stacker]] — Stacker
- [[opentelemetry]] — OpenTelemetry
- [[kusionstack]] — KusionStack
- [[fluentd]] — Fluentd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cubefs
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/storage-index|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-index/csi-index|CSI (Container Storage Interface) 知识图谱索引]]
