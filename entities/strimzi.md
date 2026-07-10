---
title: Strimzi [entities]
description: '## 概述'
summary: 'Strimzi 是在 Kubernetes 上运行 Apache Kafka 的开源项目，通过 Kubernetes Operator 模式简化 Kafka 集群的部署、配置和管理。它提供了声明式配置、自动化运维和无缝扩展能力。'
category: entities
tags:
- k8s
- cncf
- streaming
- strimzi
- prometheus
- grafana
- kafka
- crd
- operator
- serverless
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Strimzi 是什么
- 如何 Strimzi
trigger_keywords:
- Strimzi
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Strimzi

> **CNCF 状态**: Incubating | **类别**: Streaming | **主要语言**: Java

## 概述

Strimzi 是在 Kubernetes 上运行 Apache Kafka 的开源项目，通过 Kubernetes Operator 模式简化 Kafka 集群的部署、配置和管理。它提供了声明式配置、自动化运维和无缝扩展能力。

## 核心能力

- **Kubernetes 原生**: 使用 CRD 声明式管理 Kafka 集群
- **全组件覆盖**: Kafka Broker、ZooKeeper/KRaft、Connect、MirrorMaker、Bridge
- **自动化运维**: 滚动更新、自动恢复、证书轮换
- **安全集成**: TLS 加密、SASL 认证、OAuth 2.0、ACL 授权
- **监控集成**: Prometheus 指标导出、Grafana 仪表盘
- **多租户支持**: 命名空间隔离、资源配额管理

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **存储配置**: 使用高性能 SSD 存储，配置合适的 IOPS
- **资源隔离**: 为 Kafka 和 ZooKeeper 配置专用节点池
- **网络策略**: 限制 Kafka 集群的网络访问
- **备份策略**: 使用 MirrorMaker 2 进行跨集群复制
- **版本升级**: 使用 Strimzi 的滚动升级能力，零停机更新

## 架构定位

在 CNCF 生态中，strimzi 属于 **Streaming** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/storage-model.md|storage-model]]

## Related

- [[bootc]] — bootc
- [[serverless-workflow]] — Serverless Workflow
- [[cloudnativepg]] — CloudNativePG
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 08-kafka-kubernetes-strimzi
- strimzi
- [[entities/tremor.md|[[Tremor|Tremor]]]]
- [[entities/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
