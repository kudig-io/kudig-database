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
---

# Backup & DR 备份与灾备知识图谱索引

> 知识图谱：按主题 **Backup & DR** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以备份与灾备为主题或直接面向备份运维场景。

### 灾备方案

- [控制平面备份与灾备方案 (Control Plane Backup & Disaster Recovery)](./domain-3-control-plane/10-plane-backup-disaster-recovery.md)
- [存储备份与灾难恢复](./domain-6-storage/10-storage-backup-disaster-recovery.md)
- [存储灾备与迁移策略](./domain-6-storage/15-storage-disaster-recovery.md)

### 灾备工具

- [Velero 企业级备份恢复实践指南](./domain-30-disaster-recovery-business-continuity/99-velero-backup-recovery-guide.md)
- [VMware vSphere 企业级灾备与业务连续性](./domain-30-disaster-recovery-business-continuity/01-vmware-vsphere-enterprise-dr.md)
- [Veeam Backup & Replication 企业级备份恢复解决方案](./domain-30-disaster-recovery-business-continuity/02-veeam-enterprise-backup.md)
- [Commvault 企业级灾备与业务连续性深度实践](./domain-30-disaster-recovery-business-continuity/05-commvault-enterprise-disaster-recovery.md)
- [Rubrik 企业级灾备与业务连续性深度实践](./domain-30-disaster-recovery-business-continuity/06-rubrik-enterprise-disaster-recovery.md)

### CSI 存储备份

- [CSI驱动集成与运维管理](./domain-6-storage/05-csi-drivers-integration.md)
- [CSI 快照与卷备份故障排查指南](./topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting.md)

### YAML 配置

- [StorageClass / VolumeSnapshot YAML 配置参考](./domain-32-yaml-manifests/17-storageclass-volumesnapshot.md)
- [CSI 驱动资源 YAML 配置参考](./domain-32-yaml-manifests/18-csi-driver-resources.md)

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及备份与灾备但以其他 K8s 组件为主题。

### etcd 备份

- [etcd 集群初始化细节](./topic-functions/cluster-create/07-etcd.md)
- [etcd 进阶: 数据存储与维护](./topic-functions/cluster-create/13-etcd-advanced.md)
- [etcd 故障排查指南](./topic-structural-trouble-shooting/01-control-plane/02-etcd-troubleshooting.md)
- [etcd 异常 FTA 树](./topic-fta/list/etcd-fta.md)

### 存储相关

- [PV/PVC 存储深度排查与持久化治理指南](./topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md)
- [CSI 存储驱动深度排查与架构优化指南](./topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md)
- [StorageClass 配置与动态供给故障排查指南](./topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting.md)

### 故障排查

- [备份恢复故障排查 (Backup and Restore Troubleshooting)](./domain-12-troubleshooting/31-backup-restore-troubleshooting.md)
- [集群运维与升级故障排查指南](./topic-structural-trouble-shooting/08-cluster-operations/01-cluster-maintenance-troubleshooting.md)
- [集群高可用与灾备故障排查指南](./topic-structural-trouble-shooting/08-cluster-operations/04-ha-disaster-recovery-troubleshooting.md)

### FTA 故障树

- [备份/恢复异常 FTA 树](./topic-fta/list/backup-restore-fta.md)
- [CSI 存储异常 FTA 树](./topic-fta/list/csi-fta.md)
- [etcd 异常 FTA 树](./topic-fta/list/etcd-fta.md)

---

## 三、扩展参考

> 以下为运维相关参考，备份与灾备可参考存储、平台运维等章节。

### 生产运维

- [Kubernetes 备份与恢复概述 (Backup & Recovery Overview)](./domain-9-platform-ops/12-backup-recovery-strategy.md)
- [企业级备份策略](./domain-18-production-operations/16-enterprise-backup-strategy.md)
- [灾难恢复演练](./domain-18-production-operations/17-disaster-recovery-drills.md)
- [跨区域容灾部署](./domain-18-production-operations/18-cross-region-disaster-recovery.md)

### 术语词典

- [备份与灾难恢复（Backup & Disaster Recovery）](./topic-dictionary/operations/backup-disaster-recovery.md)
- [Persistent Volumes（持久卷）](./topic-dictionary/storage/persistent-volumes.md)
- [Volume Snapshots（卷快照）](./topic-dictionary/storage/volume-snapshots.md)
- [Volume Snapshot Classes（卷快照类）](./topic-dictionary/storage/volume-snapshot-classes.md)
- [CSI Volume Cloning（CSI 卷克隆）](./topic-dictionary/storage/csi-volume-cloning.md)

### CNCF 生态

- [etcd](./domain-34-cncf-landscape/graduated/etcd/etcd.md)
- [Longhorn](./domain-34-cncf-landscape/incubating/longhorn/longhorn.md)
- [K8up](./domain-34-cncf-landscape/sandbox/k8up/k8up.md)
- [Kanister](./domain-34-cncf-landscape/sandbox/kanister/kanister.md)
- [OpenEBS](./domain-34-cncf-landscape/sandbox/openebs/openebs.md)

### 迁移相关

- [存储与数据迁移](./topic-migration/04-storage-data-migration.md)
- [有状态服务迁移](./topic-migration/06-stateful-services-migration.md)
