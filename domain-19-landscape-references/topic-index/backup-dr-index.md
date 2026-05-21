---
title: Backup & DR 备份与灾备知识图谱索引
description: '## 知识图谱'
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

# Backup & DR 备份与灾备知识图谱索引

> 知识图谱：按主题 **Backup & DR** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以备份与灾备为主题或直接面向备份运维场景。

### 灾备方案

- [[domain-01-cluster-fundamentals/10-plane-backup-disaster-recovery|控制平面备份与灾备方案 (Control Plane Backup & Disaster Recovery)]]
- [[domain-04-storage-data/10-storage-backup-disaster-recovery|存储备份与灾难恢复]]
- [[domain-04-storage-data/15-storage-disaster-recovery|存储灾备与迁移策略]]

### 灾备工具

- [[domain-09-reliability-engineering/99-velero-backup-recovery-guide|Velero 企业级备份恢复实践指南]]
- [[domain-09-reliability-engineering/01-vmware-vsphere-enterprise-dr|VMware vSphere 企业级灾备与业务连续性]]
- [[domain-09-reliability-engineering/02-veeam-enterprise-backup|Veeam Backup & Replication 企业级备份恢复解决方案]]
- [[domain-09-reliability-engineering/05-commvault-enterprise-disaster-recovery|Commvault 企业级灾备与业务连续性深度实践]]
- [[domain-09-reliability-engineering/06-rubrik-enterprise-disaster-recovery|Rubrik 企业级灾备与业务连续性深度实践]]

### CSI 存储备份

- [[domain-04-storage-data/05-csi-drivers-integration|CSI驱动集成与运维管理]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting|CSI 快照与卷备份故障排查指南]]

### YAML 配置

- [[domain-18-manifests-patterns/17-storageclass-volumesnapshot|StorageClass / VolumeSnapshot YAML 配置参考]]
- [[domain-18-manifests-patterns/18-csi-driver-resources|CSI 驱动资源 YAML 配置参考]]

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及备份与灾备但以其他 K8s 组件为主题。

### etcd 备份

- [[domain-02-workloads-applications/topic-functions/cluster-create/07-etcd|etcd 集群初始化细节]]
- [[domain-02-workloads-applications/topic-functions/cluster-create/13-etcd-advanced|etcd 进阶: 数据存储与维护]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/01-control-plane/02-etcd-troubleshooting|etcd 故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/etcd-fta|etcd 异常 FTA 树]]

### 存储相关

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting|PV/PVC 存储深度排查与持久化治理指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting|CSI 存储驱动深度排查与架构优化指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting|StorageClass 配置与动态供给故障排查指南]]

### 故障排查

- [[domain-10-troubleshooting-diagnostics/31-backup-restore-troubleshooting|备份恢复故障排查 (Backup and Restore Troubleshooting)]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/08-cluster-operations/01-cluster-maintenance-troubleshooting|集群运维与升级故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/08-cluster-operations/04-ha-disaster-recovery-troubleshooting|集群高可用与灾备故障排查指南]]

### FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/backup-restore-fta|备份/恢复异常 FTA 树]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta|CSI 存储异常 FTA 树]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/etcd-fta|etcd 异常 FTA 树]]

---

## 三、扩展参考

> 以下为运维相关参考，备份与灾备可参考存储、平台运维等章节。

### 生产运维

- [[domain-07-platform-engineering/12-backup-recovery-strategy|Kubernetes 备份与恢复概述 (Backup & Recovery Overview)]]
- [[domain-11-production-operations/16-enterprise-backup-strategy|企业级备份策略]]
- [[domain-11-production-operations/17-disaster-recovery-drills|灾难恢复演练]]
- [[domain-11-production-operations/18-cross-region-disaster-recovery|跨区域容灾部署]]

### 术语词典

- [[domain-17-system-foundation/topic-dictionary/operations/backup-disaster-recovery|备份与灾难恢复（Backup & Disaster Recovery）]]
- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volumes|Persistent Volumes（持久卷）]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume-snapshots|Volume Snapshots（卷快照）]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume-snapshot-classes|Volume Snapshot Classes（卷快照类）]]
- [[domain-17-system-foundation/topic-dictionary/storage/csi-volume-cloning|CSI Volume Cloning（CSI 卷克隆）]]

### CNCF 生态

- [[domain-19-landscape-references/graduated/etcd/etcd|etcd]]
- [[domain-19-landscape-references/incubating/longhorn/longhorn|Longhorn]]
- [[domain-19-landscape-references/sandbox/k8up/k8up|K8up]]
- [[domain-19-landscape-references/sandbox/kanister/kanister|Kanister]]
- [[domain-19-landscape-references/sandbox/openebs/openebs|OpenEBS]]

### 迁移相关

- [[domain-08-release-change-management/topic-migration/04-storage-data-migration|存储与数据迁移]]
- [[domain-08-release-change-management/topic-migration/06-stateful-services-migration|有状态服务迁移]]
