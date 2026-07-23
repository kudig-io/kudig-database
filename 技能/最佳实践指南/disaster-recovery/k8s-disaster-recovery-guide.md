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

- **生产环境**：Kubernetes 集群 + [[系统基础/知识字典/storage/persistent-volumes.md|Persistent Volumes]]es（卷）|Volumes]] + etcd
- **备份层**：Velero 备份工具 + 定时备份任务
- **存储层**：S3/OSS 对象存储 + [[系统基础/知识字典/storage/volume-snapshots.md|Volume Snapshots]] + etcd 备份
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

- [[概念/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]]
- [[技能/backup-restore-etcd.md|Backup and Restore etcd]]
- [[概念/high-availability-patterns.md|High Availability Patterns]]

## 生产案例

### 案例 1: 误删 namespace 导致业务全部消失

| 时间 | 事件 |
|------|------|
| 10:00 | 运维误执行 `kubectl delete ns prod` |
| 10:01 | namespace 内所有资源开始终止 |
| 10:02 | 立即尝试取消删除，但 namespace 已进入 Terminating |
| 10:05 | 🔴 从 Velero 备份恢复 namespace |
| 10:30 | 业务恢复，丢失 10min 数据 |

**根因**: 未配置 namespace 删除保护(finalizer/webhook)，无 RBAC 限制。

### 案例 2: 区域故障导致单 AZ 集群不可用

**现象**: 云可用区故障，所有节点不可达。

**诊断**: 单 AZ 部署，无跨 AZ 容灾

**修复**: 🔴 切换到备用区域集群(需提前配置多 AZ/多区域)

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 集群完全不可用 | 启动 DR 流程 |
| P1 | 部分资源丢失 | 从备份恢复 |
| P2 | DR 演练 | 定期验证备份 |

## 面试要点

1. **Q: Kubernetes 灾备的 RPO/RTO 目标？**
   A: RPO(数据丢失): etcd 备份频率决定，30min 备份则 RPO=30min；RTO(恢复时间): 取决于备份大小和恢复流程，通常 30min-2h。关键业务 RPO<5min, RTO<15min。

2. **Q: 多区域容灾架构设计？**
   A: ① 多 AZ 部署(同区域) ② 多区域主备(跨区域) ③ 多区域双活(流量分配) ④ 数据同步(数据库主从/对象存储复制) ⑤ DNS 故障切换。

3. **Q: 灾备演练的关键步骤？**
   A: ① 定期备份验证(每月) ② 模拟故障场景(每季度) ③ 测量实际 RTO/RPO ④ 更新 Runbook ⑤ 培训团队 ⑥ 自动化恢复流程。

## Related

- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践
- [[概念/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[技能/backup-restore-etcd.md|backup-restore-etcd]] — Backup and Restore etcd


<!-- risk-assessed -->
