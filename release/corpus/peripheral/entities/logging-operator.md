---
title: Logging Operator [entities]
description: '## 概述'
summary: 'Logging Operator 是一个 Kubernetes Operator，用于自动化部署和配置 Kubernetes 集群的日志收集管道。它基于 Fluentd 和 Fluent Bit 构建，通过 CRD 声明式地管理日志的收集、过滤、转换和路由，支持将日志发送到 Elasticsearch、Loki、S3、Kafka 等多种后端。'
category: entities
tags:
- k8s
- cncf
- observability
- logging-operator
- prometheus
- grafana
- kafka
- elasticsearch
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Logging Operator 是什么
- 如何 Logging Operator
trigger_keywords:
- Logging
- Operator
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- logging-basics
---



# Logging Operator

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

Logging Operator 是一个 Kubernetes Operator，用于自动化部署和配置 Kubernetes 集群的日志收集管道。它基于 Fluentd 和 Fluent Bit 构建，通过 CRD 声明式地管理日志的收集、过滤、转换和路由，支持将日志发送到 Elasticsearch、Loki、S3、Kafka 等多种后端。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **缓冲配置**: 生产环境使用 PVC 持久化缓冲区，防止数据丢失
- **资源限制**: 为 Fluent Bit 和 Fluentd 设置合理的 CPU/内存限制
- **日志分级**: 使用 Flow 过滤掉 debug 级别日志减少存储开销
- **多输出**: 热数据发往 Elasticsearch/Loki，冷数据归档到 S3
- **多租户**: 利用 Flow/Output 的 Namespace 隔离实现多租户日志管理
- **监控缓冲**: 关注缓冲区使用率，避免因输出目标不可用导致缓冲溢出

## 架构定位

在 CNCF 生态中，logging-operator 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/storage-model.md|storage-model]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[opengemini]] — openGemini
- [[kmesh]] — Kmesh
- [[kpt]] — kpt
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[fluentd]] — Fluentd

- logging-operator
- [[entities/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
