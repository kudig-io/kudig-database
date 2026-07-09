---
title: Velero
description: Velero 是 CNCF 孵化项目，提供 Kubernetes 集群资源和持久卷的备份、恢复和迁移能力。它是 Kubernetes 灾备方案的标准工具，支持将...
summary: Velero 是 CNCF 孵化项目，提供 Kubernetes 集群资源和持久卷的备份、恢复和迁移能力。它是 Kubernetes 灾备方案的标准工具，支持将...
category: dictionary
tags:
- k8s
- glossary
- velero
- backup
- disaster-recovery
- cncf
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Velero 是什么
- Velero 详解
trigger_keywords:
- Velero
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Velero

> **英文名**: Velero

## 概述

Velero 是 CNCF 孵化项目，提供 Kubernetes 集群资源和持久卷的备份、恢复和迁移能力。它是 Kubernetes 灾备方案的标准工具，支持将备份数据存储到 S3、GCS、Azure Blob 等对象存储。

## 核心概念/原理

### 核心概念

- **Backup**：集群资源 + PV 数据的一次备份。
- **Restore**：从备份恢复资源到集群。
- **Schedule**：定时自动备份策略。
- **Backup Storage Location**：备份存储目标（S3/GCS 等）。
- **Volume Snapshot Location**：PV 快照存储目标。

### 备份范围

| 类型 | 说明 |
|------|------|
| 集群资源 | 所有 K8s API 资源（YAML） |
| PV 数据 | 通过 CSI 快照或 Restic/Kopia |
| 命名空间级 | 按 Namespace 选择性备份 |

## 关键机制或特性

- **CSI 快照**：使用 CSI VolumeSnapshot 实现 PV 的即时快照。
- **Restic/Kopia**：文件级备份，适用于不支持 CSI 快照的存储。
- **资源过滤**：按 Label、Namespace、资源类型选择性备份。
- **跨集群迁移**：备份源集群，恢复到目标集群。
- 支持备份前的 Hook（如数据库 flush）。

## 使用场景与最佳实践

- 生产集群必须配置定期备份策略。
- 使用 Schedule 资源定义每日/每周自动备份。
- 定期测试 Restore 流程确保备份可用。
- 备份数据加密存储，配置合理的保留策略。
- 使用 Velero 进行集群迁移（on-prem → 云）。

## 参考链接

- [Velero Official](https://velero.io/)

## Related

- [[系统基础/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[系统基础/topic-dictionary/storage/storage-class.md|Storage Class]]
- [[系统基础/topic-dictionary/operations/upgrade.md|Upgrade]]
- [[系统基础/topic-dictionary/workloads/statefulset.md|StatefulSet]]
- [[系统基础/topic-dictionary/storage/rook.md|Rook]]


<!-- risk-assessed -->
