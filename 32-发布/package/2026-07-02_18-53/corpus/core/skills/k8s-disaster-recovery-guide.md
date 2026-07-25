---
title: Kubernetes 灾难恢复最佳实践
description: '# Kubernetes 灾难恢复最佳实践'
summary: '本指南提供生产环境 Kubernetes 灾难恢复配置的最佳实践，涵盖从备份策略到业务连续性的全方位内容 ^[inferred]。'
category: skills
tags:
- k8s
- disaster-recovery
- backup
- velero
- business-continuity
- etcd
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 灾难恢复最佳实践 是什么
- 如何 Kubernetes 灾难恢复最佳实践
trigger_keywords:
- Kubernetes
- 灾难恢复最佳实践
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 灾难恢复最佳实践

## 概述

本指南提供生产环境 Kubernetes 灾难恢复配置的最佳实践，涵盖从备份策略到业务连续性的全方位内容 ^[inferred]。

## 灾难恢复架构

五层架构 ^[inferred]：

- **生产环境**：Kubernetes 集群 + [[domain-17-system-foundation/知识字典/storage/persistent-volumes.md|Persistent Volumes]]es（卷）|Volumes]] + etcd
- **备份层**：Velero 备份工具 + 定时备份任务
- **存储层**：S3/OSS 对象存储 + [[domain-17-system-foundation/知识字典/storage/volume-snapshots.md|Volume Snapshots]] + etcd 备份
- **恢复层**：Velero Restore 任务 + 集群恢复 + 数据恢复
- **灾备环境**：灾备集群 + 灾备数据

## 备份策略

### 每日备份

- 时间：每天凌晨 2 点（`0 2 * * *`）^[inferred]
- 范围：production 和 staging 命名空间
- 包含资源：[[Deployments|deployments]]、services、configmaps、secrets、PVCs、PVs ^[inferred]
- 包含卷快照：`snapshotVolumes: true` ^[inferred]
- 保留期：720 小时（30 天）^[inferred]

### 每周备份

- 时间：每周日凌晨 3 点（`0 3 * * 0`）^[inferred]
- 范围：所有命名空间
- 保留期：2160 小时（90 天）^[inferred]

## 关键配置

### Velero 配置

- BackupStorageLocation：配置 S3 bucket 和 region ^[inferred]
- VolumeSnapshotLocation：配置云服务商和 region ^[inferred]

### etcd 备份

etcd 是集群状态的核心，必须单独备份。可使用 `etcdctl snapshot save` 进行定期备份 ^[inferred]。

## 恢复策略

- 优先恢复关键服务：使用 `--include-namespaces` 和 `--include-resources` 限定范围 ^[inferred]
- 并行恢复：使用 `--restore-volumes=true` 和 `--namespace-mappings` ^[inferred]
- 恢复后验证：检查所有资源状态和服务可用性 ^[inferred]

## 恢复演练

定期执行恢复演练 ^[inferred]：
1. 选择最新备份
2. 创建恢复任务到隔离命名空间（`--namespace-mappings production:production-drill`）
3. 等待恢复完成
4. 验证恢复结果

## 常见陷阱

### 备份策略不当

备份频率不够或不包含关键资源会导致数据丢失。应配置日备份和周备份，包含卷快照 ^[inferred]。

### 备份验证缺失

未验证备份有效性会导致灾难恢复时失败。应定期检查备份状态、详情和日志，并执行恢复演练 ^[inferred]。

### 恢复流程不优化

恢复流程不优化会导致恢复时间长。应优先恢复关键服务，使用并行恢复策略 ^[inferred]。

## 验证方法

- 检查 Velero 版本和备份存储位置
- 检查备份策略和备份状态：`velero schedule get`、`velero backup get`
- 检查恢复任务：`velero restore get`

## 相关资源

- [[concepts/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]]
- [[skills/backup-restore-etcd.md|Backup and Restore etcd]]
- [[concepts/high-availability-patterns.md|High Availability Patterns]]

## Related

- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践
- [[concepts/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[skills/backup-restore-etcd.md|backup-restore-etcd]] — Backup and Restore etcd


<!-- risk-assessed -->
