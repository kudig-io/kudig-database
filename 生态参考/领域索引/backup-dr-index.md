---
title: Backup & DR 备份与灾备知识图谱索引
description: '## 知识图谱'
summary: '## 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- backup
- disaster-recovery
- velero
- etcd
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- Backup DR 备份灾备知识图谱 是什么
- 如何 Backup DR 备份灾备知识图谱
trigger_keywords:
- Backup
- DR
- 备份
- 灾备
- 知识图谱
- velero
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Backup & DR 备份与灾备知识图谱索引

> 知识图谱：按主题 **Backup & DR** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以备份与灾备为主题或直接面向备份运维场景。

### 灾备方案

- [[集群基础/控制平面/10-plane-backup-disaster-recovery.md|控制平面备份与灾备方案 (Control Plane Backup & Disaster Recovery)]]
- 存储备份与灾难恢复
- 存储灾备与迁移策略

### 灾备工具

- Velero 企业级备份恢复实践指南
- VMware vSphere 企业级灾备与业务连续性
- Veeam Backup & Replication 企业级备份恢复解决方案
- Commvault 企业级灾备与业务连续性深度实践
- Rubrik 企业级灾备与业务连续性深度实践

### CSI 存储备份

- CSI驱动集成与运维管理
- [[故障诊断/高级排障/04-storage/03-snapshot-backup-troubleshooting.md|CSI 快照与卷备份故障排查指南]]

### YAML 配置

- StorageClass / VolumeSnapshot YAML 配置参考
- CSI 驱动资源 YAML 配置参考

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及备份与灾备但以其他 K8s 组件为主题。

### etcd 备份

- [[平台工程/代码分析/functions-cluster-create/07-etcd.md|etcd 集群初始化细节]]
- [[平台工程/代码分析/functions-cluster-create/13-etcd-advanced.md|etcd 进阶: 数据存储与维护]]
- [[故障诊断/高级排障/01-control-plane/02-etcd-troubleshooting.md|etcd 故障排查指南]]
- [[故障诊断/FTA故障树/list/etcd-fta.md|etcd 异常 FTA 树]]

### 存储相关

- [[故障诊断/高级排障/04-storage/01-pv-pvc-troubleshooting.md|PV/PVC 存储深度排查与持久化治理指南]]
- [[故障诊断/高级排障/04-storage/02-csi-troubleshooting.md|CSI 存储驱动深度排查与架构优化指南]]
- [[故障诊断/高级排障/04-storage/05-storageclass-troubleshooting.md|StorageClass 配置与动态供给故障排查指南]]

### 故障排查

- [[故障诊断/基础设施排障/31-backup-restore-troubleshooting.md|备份恢复故障排查 (Backup and Restore Troubleshooting)]]
- [[故障诊断/高级排障/08-cluster-operations/01-cluster-maintenance-troubleshooting.md|集群运维与升级故障排查指南]]
- [[故障诊断/高级排障/08-cluster-operations/04-ha-disaster-recovery-troubleshooting.md|集群高可用与灾备故障排查指南]]

### FTA 故障树

- [[故障诊断/FTA故障树/list/backup-restore-fta.md|备份/恢复异常 FTA 树]]
- [[故障诊断/FTA故障树/list/csi-fta.md|CSI 存储异常 FTA 树]]
- [[故障诊断/FTA故障树/list/etcd-fta.md|etcd 异常 FTA 树]]

---

## 三、扩展参考

> 以下为运维相关参考，备份与灾备可参考存储、平台运维等章节。

### 生产运维

- [[概念/kubernetes-architecture-overview.md|kubernetes architecture overview]]
- 企业级备份策略
- 灾难恢复演练
- 跨区域容灾部署

### 术语词典

- [[系统基础/知识字典/operations/backup-disaster-recovery.md|备份与灾难恢复（Backup & Disaster Recovery）]]
- [[系统基础/知识字典/storage/persistent-volumes.md|Persistent Volumes（持久卷）]]
- [[系统基础/知识字典/storage/volume-snapshots.md|Volume Snapshots（卷快照）]]
- [[系统基础/知识字典/storage/volume-snapshot-classes.md|Volume Snapshot Classes（卷快照类）]]
- [[系统基础/知识字典/storage/csi-volume-cloning.md|CSI Volume Cloning（CSI 卷克隆）]]

### CNCF 生态

- etcd
- Longhorn
- K8up
- Kanister
- OpenEBS

### 迁移相关

- [[发布变更/迁移方案/04-storage-data-migration.md|存储与数据迁移]]
- [[发布变更/迁移方案/06-stateful-services-migration.md|有状态服务迁移]]


<!-- risk-assessed -->
