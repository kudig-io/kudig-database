---
title: MinIO
description: MinIO 是高性能的 S3 兼容对象存储系统，可在任何基础设施上部署。在 Kubernetes 中常用作 Thanos、Loki、Velero
  等工具的对象存...
summary: MinIO 是高性能的 S3 兼容对象存储系统，可在任何基础设施上部署。在 Kubernetes 中常用作 Thanos、Loki、Velero
  等工具的对象存...
category: dictionary
tags:
- k8s
- glossary
- minio
- storage
- s3
- object-storage
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- MinIO 是什么
- MinIO 详解
trigger_keywords:
- MinIO
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# MinIO

> **英文名**: MinIO

## 概述

MinIO 是高性能的 S3 兼容对象存储系统，可在任何基础设施上部署。在 Kubernetes 中常用作 Thanos、Loki、Velero 等工具的对象存储后端，是云存储的私有化部署首选。

## 核心概念/原理

### 核心特性

| 特性 | 说明 |
|------|------|
| S3 兼容 | 完全兼容 AWS S3 API |
| 高性能 | 单节点可达 100GB/s+ 吞吐 |
| 纠删码 | 数据冗余和自愈 |
| 加密 | 服务端加密（SSE-S3/SSE-KMS） |
| 多租户 | 支持多租户隔离 |

### K8s 中使用场景

- Thanos 长期存储后端
- Loki 日志存储
- Velero 备份目标
- Harbor 镜像存储

## 关键机制或特性

- **Erasure Coding**：自动数据冗余，容忍多磁盘故障。
- **Bucket Notification**：对象变更事件通知（Webhook/Kafka）。
- **Replication**：跨集群/跨站点数据复制。
- **Site Replication**：多站点双活部署。
- **Console**：Web UI 管理存储桶和对象。

## 使用场景与最佳实践

- 需要私有 S3 兼容存储时部署 MinIO。
- 作为 Thanos/Loki/Velero 的对象存储后端。
- 使用 MinIO Operator 在 K8s 中管理 MinIO 集群。
- 配置纠删码确保数据可靠性。
- 启用 TLS 加密和 IAM 策略控制访问。

## 参考链接

- [MinIO Official](https://min.io/)

## Related

- [[系统基础/知识字典/observability/thanos.md|Thanos]]
- [[系统基础/知识字典/observability/loki.md|Loki]]
- [[系统基础/知识字典/operations/velero.md|Velero]]
- [[系统基础/知识字典/tooling/harbor.md|Harbor]]
- [[系统基础/知识字典/storage/persistent-volume.md|Persistent Volume]]


<!-- risk-assessed -->
