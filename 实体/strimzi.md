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
last_updated: 2026-07
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

Strimzi 是在 Kubernetes 上运行 Apache Kafka 的开源 Operator，由 Red Hat 开发并开源，2022 年加入 CNCF Incubating。它通过 Kubernetes Operator 模式简化 Kafka 集群的部署、配置和管理，提供了声明式配置、自动化运维和无缝扩展能力。Strimzi 覆盖了 Kafka 全生态组件（Broker、ZooKeeper/KRaft、Connect、MirrorMaker、Bridge），是目前 Kafka on Kubernetes 最成熟的开源方案。

## 核心特性

- **全组件管理**: Kafka Broker、ZooKeeper/KRaft、Kafka Connect、MirrorMaker 2、Kafka Bridge
- **CRD 声明式**: Kafka、KafkaTopic、KafkaUser、KafkaConnect 等 CRD 管理
- **滚动升级**: 零停机的 Kafka 版本升级和配置变更
- **安全集成**: TLS 加密、SASL/SCRAM 认证、OAuth 2.0、ACL 授权
- **监控集成**: 内置 Prometheus 指标和 Grafana 仪表盘
- **Topic/User 管理**: 通过 CRD 声明式管理 Kafka Topic 和用户

## 架构

Strimzi 的核心是 Cluster Operator，监听 Kafka CRD，管理 Kafka 集群的全生命周期。架构包含：Cluster Operator（管理 Kafka/ZooKeeper/Connect 集群）、Topic Operator（管理 KafkaTopic CRD 到 Topic 的同步）、User Operator（管理 KafkaUser CRD 到用户/ACL 的同步）。Kafka Broker 以 StatefulSet 运行，数据存储在 PVC 上。每个 Pod 包含 Kafka 进程和 Stunnel（TLS 代理）、Cruise Control（分区重平衡）。Entity Operator（Topic + User Operator）作为单独的 Deployment 运行。

## Kubernetes 集成

Strimzi 完全基于 Kubernetes CRD。Kafka CRD 定义集群规格（Broker 数、存储、网络、安全）。KafkaTopic CRD 声明式创建和管理 Topic（分区数、副本数、配置）。KafkaUser CRD 管理用户认证和 ACL 权限。Operator 通过 Kubernetes API Server 管理资源，无需外部工具。StorageClass 配置决定数据持久化方式。支持 PodAntiAffinity 实现跨可用区分布。

## 生产使用场景

1. **事件流平台**: 在 Kubernetes 上运行 Kafka 作为微服务的事件流基础设施
2. **CDC 数据管道**: 使用 Kafka Connect 连接数据库变更数据
3. **跨集群复制**: 使用 MirrorMaker 2 实现灾备和多区域复制
4. **Kafka 即服务**: 为多团队提供 Kafka 实例的自服务平台

## 安装

```bash
# Helm 安装
helm repo add strimzi https://strimzi.io/charts/
helm install strimzi strimzi/strimzi-kafka-operator --namespace kafka --create-namespace
# 创建 Kafka 集群
kubectl apply -f - <<EOF
apiVersion: kafka.strimzi.io/v1beta2
kind: Kafka
metadata: { name: my-cluster }
spec:
  kafka:
    replicas: 3
    storage: { type: jbod, volumes: [{ id: 0, type: persistent-claim, size: 100Gi }] }
  zookeeper:
    replicas: 3
    storage: { type: persistent-claim, size: 10Gi }
  entityOperator:
    topicOperator: {}
    userOperator: {}
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Strimzi** | CNCF Incubating、Red Hat 支持 | 资源开销大 |
| Confluent for K8s | Confluent 官方、功能丰富 | 商业许可 |
| Koperator (Banzaicloud) | 轻量级 | 社区较小 |
| Bitnami Kafka Chart | 简单快速 | 运维自动化能力弱 |

## 架构定位

在 CNCF 生态中，Strimzi 属于 **Streaming** 类别，是 Kafka on Kubernetes 的标杆项目。它将复杂的 Kafka 运维转化为声明式的 K8s 资源管理。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/storage-model.md|storage-model]]

## Related

- [[bootc]] — bootc
- [[serverless-workflow]] — Serverless Workflow
- [[cloudnativepg]] — CloudNativePG
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 08-kafka-kubernetes-strimzi
- strimzi
- [[实体/tremor.md|[[Tremor|Tremor]]]]
- [[实体/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
