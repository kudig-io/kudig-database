---
title: PVC 知识图谱索引
description: '## PVC 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- pvc
- storage
- pv
- csi
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- PVC 知识图谱 是什么
- PVC 存储 相关文档
trigger_keywords:
- PVC
- 知识图谱
- index
- storage
---

# PVC 知识图谱索引

> 知识图谱：按关键字 **pvc** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 存储知识域 (PV/PVC 核心)

- [01 - 存储架构概览与核心组件](./domain-6-storage/01-storage-architecture-overview.md)
- [02 - PV/PVC核心概念与企业级实践](./domain-6-storage/02-pv-architecture-fundamentals.md)
- [03 - PVC使用模式与最佳实践](./domain-6-storage/03-pvc-patterns-practices.md)
- [04 - StorageClass动态供给与多租户管理](./domain-6-storage/04-storageclass-dynamic-provisioning.md)
- [05 - CSI驱动集成与运维管理](./domain-6-storage/05-csi-drivers-integration.md)
- [09 - PV/PVC故障排查与解决方案](./domain-6-storage/09-pv-pvc-troubleshooting.md)
- [10 - 存储备份与灾难恢复](./domain-6-storage/10-storage-backup-disaster-recovery.md)
- [16 - CSI 迁移：从 In-Tree 存储插件到 CSI](./domain-6-storage/16-csi-migration-in-tree-to-csi.md)
- [Storage Domain 存储领域知识库](./domain-6-storage/README.md)

### YAML 配置参考

- [15 - PersistentVolume YAML 配置参考](./domain-32-yaml-manifests/15-persistentvolume-reference.md)
- [16 - PersistentVolumeClaim YAML 配置参考](./domain-32-yaml-manifests/16-persistentvolumeclaim-reference.md)
- [17 - StorageClass / VolumeSnapshot YAML 配置参考](./domain-32-yaml-manifests/17-storageclass-volumesnapshot.md)
- [18 - CSI 驱动资源 YAML 配置参考](./domain-32-yaml-manifests/18-csi-driver-resources.md)

### 术语词典 (存储相关)

- [CSI Volume Cloning（CSI 卷克隆）](./topic-dictionary/storage/csi-volume-cloning.md)
- [Dynamic Volume Provisioning（动态卷供给）](./topic-dictionary/storage/dynamic-volume-provisioning.md)
- [Ephemeral Volumes（临时卷）](./topic-dictionary/storage/ephemeral-volumes.md)
- [Persistent Volumes（持久卷）](./topic-dictionary/storage/persistent-volumes.md)
- [Storage Classes（存储类）](./topic-dictionary/storage/storage-classes.md)
- [Volume Attributes Classes（卷属性类）](./topic-dictionary/storage/volume-attributes-classes.md)
- [Volume Health Monitoring（卷健康监控）](./topic-dictionary/storage/volume-health-monitoring.md)
- [Volume Snapshot Classes（卷快照类）](./topic-dictionary/storage/volume-snapshot-classes.md)
- [Volume Snapshots（卷快照）](./topic-dictionary/storage/volume-snapshots.md)
- [Volumes（卷）](./topic-dictionary/storage/volumes.md)

## 关联文档 (K8s 集成)

### 故障排查

- [14 - PVC与存储全面故障排查 (PVC & Storage Comprehensive Troubleshooting)](./domain-12-troubleshooting/14-pvc-storage-troubleshooting.md)
- [04 - CSI 存储驱动故障排查 (CSI Driver Troubleshooting)](./domain-12-troubleshooting/04-storage-csi-troubleshooting.md)
- [21 - StatefulSet 故障排查 (StatefulSet Troubleshooting)](./domain-12-troubleshooting/21-statefulset-troubleshooting.md)
- [PV/PVC 存储深度排查与持久化治理指南](./topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md)
- [CSI 存储驱动深度排查与架构优化指南](./topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md)
- [CSI 快照与卷备份故障排查指南](./topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting.md)
- [存储 I/O 性能故障排查指南](./topic-structural-trouble-shooting/04-storage/04-storage-performance-troubleshooting.md)
- [StorageClass 配置与动态供给故障排查指南](./topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting.md)
- [CSI 存储异常 FTA 树](./topic-fta/list/csi-fta.md)

### 控制平面组件

- [CSI 容器存储接口深度解析 (Container Storage Interface Deep Dive)](./domain-3-control-plane/22-container-storage-deep-dive.md)
- [cloud-controller-manager 深度解析 (CCM Deep Dive)](./domain-3-control-plane/14-cloud-controller-manager-deep-dive.md)

### 技能卡片

- [PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation](./topic-skills/07-pvc-storage-failure.md)
- [Pod Pending 调度失败诊断与修复](./topic-skills/03-pod-pending.md)

### K8s 事件

- [11 - 存储与卷事件](./domain-33-kubernetes-events/11-storage-volume-events.md)

### 功能操作

- [存储与卷管理](./topic-functions/cluster-create/22-storage-volumes.md)
- [节点存储](./topic-functions/node-create/14-storage-node.md)

### 设计原则

- [03 - 控制器模式与调谐循环 (Controller Pattern)](./domain-2-design-principles/03-controller-pattern.md)

## 扩展参考

### 存储生态项目

- [Rook](./domain-34-cncf-landscape/graduated/rook/rook.md)
- [Longhorn](./domain-34-cncf-landscape/incubating/longhorn/longhorn.md)
- [OpenEBS](./domain-34-cncf-landscape/sandbox/openebs/openebs.md)
- [HwameiStor](./domain-34-cncf-landscape/sandbox/hwameistor/hwameistor.md)
- [K8up](./domain-34-cncf-landscape/sandbox/k8up/k8up.md)
- [Fluid](./domain-34-cncf-landscape/incubating/fluid/fluid.md)
- [Carina](./domain-34-cncf-landscape/sandbox/carina/carina.md)
- [Piraeus Datastore](./domain-34-cncf-landscape/sandbox/piraeus-datastore/piraeus-datastore.md)

### 存储基础

- [05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南](./domain-14-linux/05-linux-storage-management.md)
- [Domain-16 存储基础 — 开源项目索引](./domain-16-storage-fundamentals/00-open-source-projects-index.md)

### 灾备与迁移

- [04 - 存储与数据迁移](./topic-migration/04-storage-data-migration.md)
- [06 - 有状态服务迁移](./topic-migration/06-stateful-services-migration.md)
- [17 - 灾难恢复演练](./domain-18-production-operations/17-disaster-recovery-drills.md)
- [备份/恢复异常 FTA 树](./topic-fta/list/backup-restore-fta.md)

### 培训学习

- [Kubernetes 存储体系全栈进阶培训 (从入门到专家)](./topic-presentations/kubernetes-storage-presentation.md)
- [Day 26: 存储卷创建 & 删除](./topic-learn/inner-training/week-4-network-storage/day-26-storage-create-delete.md)
- [Day 27: 存储卷挂载](./topic-learn/inner-training/week-4-network-storage/day-27-storage-mount.md)
- [P4: 网络与存储综合实践](./topic-learn/inner-training/projects/p4-network-storage-practice.md)
