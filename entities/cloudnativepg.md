---
title: CloudNativePG (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- database
- cloudnativepg
- etcd
- prometheus
- grafana
- postgresql
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CloudNativePG 是什么
- 如何 CloudNativePG
trigger_keywords:
- CloudNativePG
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
created: "2026-05-23"
---

# CloudNativePG

> **CNCF 状态**: Sandbox | **类别**: Database | **主要语言**: Go

## 概述

CloudNativePG 是 Kubernetes 上的 PostgreSQL Operator，提供完整的数据库生命周期管理。它原生支持 PostgreSQL 流复制、自动故障转移、备份恢复和监控集成。

## 核心能力

- **高可用**: 基于 Patroni 的自动故障转移
- **声明式配置**: CRD 方式管理 PostgreSQL 集群
- **备份恢复**: 支持 S3/Azure/GCS 的连续归档和 PITR
- **原生集成**: 无需外部依赖（如 etcd）
- **监控**: 内置 Prometheus 指标导出
- **安全**: TLS 加密、证书轮换、密钥管理

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **副本数量**: 生产环境至少 3 个实例
- **资源配置**: 根据负载配置合理的 shared_buffers
- **备份策略**: 配置 WAL 归档和定期全量备份
- **监控告警**: 监控复制延迟和连接数
- **存储类型**: 使用高性能 SSD 存储

## 架构定位

在 CNCF 生态中，cloudnativepg 属于 **Database** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/prometheus-grafana|prometheus-grafana]]
- [[entities/crd-custom-resources|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/storage-model|storage-model]]

## Related

- [[bootc]] — bootc
- [[serverless-workflow]] — [[Serverless Workflow|Serverless Workflow]]
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 99-cloudnativepg-enterprise-guide
- cloudnativepg
- storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
