---
title: CubeFS (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
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
tier: peripheral
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

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

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[concepts/storage-model.md|storage-model]]
- [[entities/csi-drivers.md|csi-drivers]]

## Related

- [[stacker]] — Stacker
- [[opentelemetry]] — OpenTelemetry
- [[kusionstack]] — KusionStack
- [[fluentd]] — Fluentd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cubefs
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/storage-index.md|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-index/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
