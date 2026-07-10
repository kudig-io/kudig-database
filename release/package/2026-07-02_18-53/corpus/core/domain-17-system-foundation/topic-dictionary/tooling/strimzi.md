---
title: Strimzi
description: Strimzi 是 CNCF 孵化项目，在 Kubernetes 上提供 Apache Kafka 的原生部署和管理能力。它通过 Operator
  模式自动化 ...
summary: Strimzi 是 CNCF 孵化项目，在 Kubernetes 上提供 Apache Kafka 的原生部署和管理能力。它通过 Operator
  模式自动化 ...
category: dictionary
tags:
- k8s
- glossary
- strimzi
- kafka
- streaming
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Strimzi 是什么
- Strimzi 详解
trigger_keywords:
- Strimzi
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Strimzi

> **英文名**: Strimzi

## 概述

Strimzi 是 CNCF 孵化项目，在 Kubernetes 上提供 Apache Kafka 的原生部署和管理能力。它通过 Operator 模式自动化 Kafka 集群的部署、扩缩容、升级和监控。

## 核心概念/原理

### 核心 CRD

| 资源 | 功能 |
|------|------|
| Kafka | Kafka 集群定义 |
| KafkaTopic | Topic 管理 |
| KafkaUser | 用户和 ACL 管理 |
| KafkaConnect | Kafka Connect 集群 |
| KafkaBridge | HTTP 桥接 |
| KafkaMirrorMaker | 跨集群镜像 |

### 部署模式

- **Ephemeral**：临时存储（测试）。
- **Persistent**：持久化存储（生产）。
- **JBOD**：多磁盘存储。

## 关键机制或特性

- **Operator 管理**：自动化 Kafka 集群生命周期。
- **Cruise Control**：自动分区重平衡。
- **Tiered Storage**：热/温/冷分层存储。
- **mTLS**：内置客户端和服务端加密。
- **OAuth/OIDC**：企业级认证集成。

## 使用场景与最佳实践

- K8s 中部署 Kafka 优先使用 Strimzi。
- 生产环境使用 Persistent 模式配置 3 副本。
- 配合 Cruise Control 实现分区自动平衡。
- 使用 KafkaUser CRD 管理客户端认证和 ACL。
- 监控 Kafka 的 lag 指标和分区状态。

## 参考链接

- [Strimzi Official](https://strimzi.io/)

## Related

- [[domain-17-system-foundation/知识字典/platform-engineering/operator-pattern.md|Operator Pattern]]
- [[domain-17-system-foundation/知识字典/storage/persistent-volume.md|Persistent Volume]]
- [[domain-17-system-foundation/知识字典/security/certificate.md|Certificate]]
- [[domain-17-system-foundation/知识字典/observability/prometheus.md|Prometheus]]
- [[domain-17-system-foundation/知识字典/workloads/statefulset.md|StatefulSet]]


<!-- risk-assessed -->
