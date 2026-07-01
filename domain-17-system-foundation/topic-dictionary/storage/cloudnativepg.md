---
title: CloudNativePG 云原生 PostgreSQL
description: CloudNativePG 是 EDB 开源的 Kubernetes PostgreSQL Operator，以 GitOps 友好的方式管理
  PostgreS...
summary: CloudNativePG 是 EDB 开源的 Kubernetes PostgreSQL Operator，以 GitOps 友好的方式管理 PostgreS...
category: dictionary
tags:
- k8s
- glossary
- storage
- database
- operator
tier: core
created: 2026-05
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CloudNativePG 云原生 PostgreSQL 是什么
- CloudNativePG 详解
trigger_keywords:
- CloudNativePG 云原生 PostgreSQL
- CloudNativePG
- dictionary
prerequisites:
- kubernetes
---



# CloudNativePG 云原生 PostgreSQL（CloudNativePG）

## 概述

CloudNativePG 是 EDB 开源的 Kubernetes PostgreSQL Operator，以 GitOps 友好的方式管理 PostgreSQL 集群的全生命周期，支持高可用、备份恢复和滚动升级。

## 核心概念/原理

- **Kubernetes 原生**：通过 CRD 声明式管理 PostgreSQL 集群
- **高可用**：基于流复制的自动故障转移
- **GitOps 友好**：所有配置通过 YAML 声明
- **CNCF Sandbox**：活跃的 PostgreSQL on K8s 社区

## 关键机制或特性

- Cluster CRD 定义 PG 集群（实例数、存储、资源配置）
- 基于 Patroni 的高可用和自动故障转移
- 连续 WAL 归档和 PITR（Point-in-Time Recovery）
- 滚动升级和在线参数变更
- 读写分离连接池（内置 PgBouncer）
- 多集群部署支持

## 使用场景与最佳实践

- Kubernetes 上的 PostgreSQL 生产部署
- GitOps 方式管理数据库生命周期
- 需要自动故障转移的高可用数据库
- 数据库版本升级的零停机方案
- 多租户数据库实例管理

## 参考链接

- https://cloudnative-pg.io/
- https://github.com/cloudnative-pg/cloudnative-pg

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volumes.md|PV/PVC]]
- [[domain-17-system-foundation/topic-dictionary/operations/velero.md|Velero]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/rancher.md|Rancher]]
