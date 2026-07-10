---
title: K8up 备份 Operator
description: K8up 是 VSHN 开源的 Kubernetes 备份 Operator，基于 restic 实现增量备份，通过 CRD 声明式管理
  PVC 数据的自动备份...
summary: K8up 是 VSHN 开源的 Kubernetes 备份 Operator，基于 restic 实现增量备份，通过 CRD 声明式管理 PVC
  数据的自动备份...
category: dictionary
tags:
- k8s
- glossary
- operations
- backup
- operator
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8up 备份 Operator 是什么
- K8up 详解
trigger_keywords:
- K8up 备份 Operator
- K8up
- dictionary
prerequisites:
- kubernetes
---



# K8up 备份 Operator（K8up）

## 概述

K8up 是 VSHN 开源的 Kubernetes 备份 Operator，基于 restic 实现增量备份，通过 CRD 声明式管理 PVC 数据的自动备份和恢复，是 Velero 的轻量级替代方案。

## 核心概念/原理

- **Operator 模式**：通过 CRD 声明式管理备份策略
- **restic 后端**：基于 restic 的增量、加密、去重备份
- **PVC 级别**：自动发现并备份集群中的所有 PVC
- **多后端**：支持 S3/GCS/Azure/Swift 等存储后端

## 关键机制或特性

- Schedule CRD 定义备份计划（Cron 表达式）
- PreBackupPod 备份前执行自定义脚本（如数据库 dump）
- 自动 PVC 发现和备份
- Restore CRD 管理恢复操作
- Archive CRD 归档旧备份
- 与 Prometheus 集成导出备份指标

## 使用场景与最佳实践

- 有状态应用的定时备份
- 数据库的 Pre-backup dump + 增量备份
- 轻量级备份方案（替代 Velero 的全集群备份）
- 多租户环境的独立备份策略
- 备份合规和保留策略管理

## 参考链接

- https://k8up.io/
- https://github.com/k8up-io/k8up

## Related

- [[系统基础/知识字典/operations/velero.md|Velero]]
- [[系统基础/知识字典/storage/persistent-volumes.md|PV/PVC]]
- [[系统基础/知识字典/operations/backup-disaster-recovery.md|备份与灾难恢复]]
