---
title: Storage 存储知识图谱索引
description: '## Storage 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- storage
- csi
- pvc
- pv
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- Storage 知识图谱 是什么
- Kubernetes 存储 相关文档
trigger_keywords:
- Storage
- 知识图谱
- index
- csi
---

# Storage 存储知识图谱索引

> 知识图谱：按关键字 **storage** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 存储知识域

- [01 - 存储架构概览与核心组件](./domain-6-storage/01-storage-architecture-overview.md)
- [02 - PV/PVC核心概念与企业级实践](./domain-6-storage/02-pv-architecture-fundamentals.md)
- [03 - PVC使用模式与最佳实践](./domain-6-storage/03-pvc-patterns-practices.md)
- [04 - StorageClass动态供给与多租户管理](./domain-6-storage/04-storageclass-dynamic-provisioning.md)
- [05 - CSI驱动集成与运维管理](./domain-6-storage/05-csi-drivers-integration.md)
- [06 - 存储基础概念详解](./domain-6-storage/06-storage-fundamental-concepts.md)
- [08 - 存储性能调优与优化策略](./domain-6-storage/08-storage-performance-tuning.md)
- [09 - PV/PVC故障排查与解决方案](./domain-6-storage/09-pv-pvc-troubleshooting.md)
- [10 - 存储备份与灾难恢复](./domain-6-storage/10-storage-backup-disaster-recovery.md)
- [13 - 存储安全与合规管理](./domain-6-storage/13-storage-security-compliance.md)
- [16 - CSI 迁移：从 In-Tree 存储插件到 CSI](./domain-6-storage/16-csi-migration-in-tree-to-csi.md)

### CSI 深度解析

- [CSI 容器存储接口深度解析 (Container Storage Interface Deep Dive)](./domain-3-control-plane/22-container-storage-deep-dive.md)

### YAML 配置参考

- [15 - PersistentVolume YAML 配置参考](./domain-32-yaml-manifests/15-persistentvolume-reference.md)
- [16 - PersistentVolumeClaim YAML 配置参考](./domain-32-yaml-manifests/16-persistentvolumeclaim-reference.md)
- [17 - StorageClass / VolumeSnapshot YAML 配置参考](./domain-32-yaml-manifests/17-storageclass-volumesnapshot.md)
- [18 - CSI 驱动资源 YAML 配置参考](./domain-32-yaml-manifests/18-csi-driver-resources.md)

### 术语词典 (存储相关)

- [Persistent Volumes（持久卷）](./topic-dictionary/storage/persistent-volumes.md)
- [Storage Classes（存储类）](./topic-dictionary/storage/storage-classes.md)
- [Volume Snapshots（卷快照）](./topic-dictionary/storage/volume-snapshots.md)
- [CSI Volume Cloning（CSI 卷克隆）](./topic-dictionary/storage/csi-volume-cloning.md)
- [Dynamic Volume Provisioning（动态卷供给）](./topic-dictionary/storage/dynamic-volume-provisioning.md)
- [Ephemeral Volumes（临时卷）](./topic-dictionary/storage/ephemeral-volumes.md)
- [Volume Health Monitoring（卷健康监控）](./topic-dictionary/storage/volume-health-monitoring.md)
- [高性能存储网络（RDMA / NVMe-oF）](./topic-dictionary/storage/high-performance-storage-networks.md)

## 关联文档 (K8s 集成)

### 故障排查

- [PV/PVC 存储深度排查与持久化治理指南](./topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md)
- [CSI 存储驱动深度排查与架构优化指南](./topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md)
- [CSI 快照与卷备份故障排查指南](./topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting.md)
- [存储 I/O 性能故障排查指南](./topic-structural-trouble-shooting/04-storage/04-storage-performance-troubleshooting.md)
- [StorageClass 配置与动态供给故障排查指南](./topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting.md)

### K8s 事件

- [11 - 存储与卷事件](./domain-33-kubernetes-events/11-storage-volume-events.md)

### 技能卡片

- [PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation](./topic-skills/07-pvc-storage-failure.md)

### FTA 故障树

- [CSI 存储异常 FTA 树](./topic-fta/list/csi-fta.md)

## 扩展参考

### 存储生态项目

- [Rook](./domain-34-cncf-landscape/graduated/rook/rook.md)
- [Longhorn](./domain-34-cncf-landscape/incubating/longhorn/longhorn.md)
- [OpenEBS](./domain-34-cncf-landscape/sandbox/openebs/openebs.md)
- [HwameiStor](./domain-34-cncf-landscape/sandbox/hwameistor/hwameistor.md)
- [K8up](./domain-34-cncf-landscape/sandbox/k8up/k8up.md)
- [Carina](./domain-34-cncf-landscape/sandbox/carina/carina.md)
- [Piraeus Datastore](./domain-34-cncf-landscape/sandbox/piraeus-datastore/piraeus-datastore.md)
- [CubeFS](./domain-34-cncf-landscape/graduated/cubefs/cubefs.md)

### 存储基础

- [02 - 块存储、文件存储、对象存储](./domain-16-storage-fundamentals/02-block-file-object-storage.md)
- [05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南](./domain-14-linux/05-linux-storage-management.md)
