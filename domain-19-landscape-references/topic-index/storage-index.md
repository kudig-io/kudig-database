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
- rook
- rag
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
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# Storage 存储知识图谱索引

> 知识图谱：按关键字 **storage** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 存储知识域

- [[domain-04-storage-data/01-storage-architecture-overview|01 - 存储架构概览与核心组件]]
- [[domain-04-storage-data/02-pv-architecture-fundamentals|02 - PV/PVC核心概念与企业级实践]]
- [[domain-04-storage-data/03-pvc-patterns-practices|03 - PVC使用模式与最佳实践]]
- [[domain-04-storage-data/04-storageclass-dynamic-provisioning|04 - StorageClass动态供给与多租户管理]]
- [[domain-04-storage-data/05-csi-drivers-integration|05 - CSI驱动集成与运维管理]]
- [[domain-04-storage-data/06-storage-fundamental-concepts|06 - 存储基础概念详解]]
- [[domain-04-storage-data/08-storage-performance-tuning|08 - 存储性能调优与优化策略]]
- [[domain-04-storage-data/09-pv-pvc-troubleshooting|09 - PV/PVC故障排查与解决方案]]
- [[domain-04-storage-data/10-storage-backup-disaster-recovery|10 - 存储备份与灾难恢复]]
- [[domain-04-storage-data/13-storage-security-compliance|13 - 存储安全与合规管理]]
- [[domain-04-storage-data/16-csi-migration-in-tree-to-csi|16 - CSI 迁移：从 In-Tree 存储插件到 CSI]]

### CSI 深度解析

- [[domain-01-cluster-fundamentals/22-container-storage-deep-dive|CSI 容器存储接口深度解析 (Container Storage Interface Deep Dive)]]

### YAML 配置参考

- [[domain-18-manifests-patterns/15-persistentvolume-reference|15 - PersistentVolume YAML 配置参考]]
- [[domain-18-manifests-patterns/16-persistentvolumeclaim-reference|16 - PersistentVolumeClaim YAML 配置参考]]
- [[domain-18-manifests-patterns/17-storageclass-volumesnapshot|17 - StorageClass / VolumeSnapshot YAML 配置参考]]
- [[domain-18-manifests-patterns/18-csi-driver-resources|18 - CSI 驱动资源 YAML 配置参考]]

### 术语词典 (存储相关)

- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volumes|Persistent Volumes（持久卷）]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-classes|Storage Classes（存储类）]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume-snapshots|Volume Snapshots（卷快照）]]
- [[domain-17-system-foundation/topic-dictionary/storage/csi-volume-cloning|CSI Volume Cloning（CSI 卷克隆）]]
- [[domain-17-system-foundation/topic-dictionary/storage/dynamic-volume-provisioning|Dynamic Volume Provisioning（动态卷供给）]]
- [[domain-17-system-foundation/topic-dictionary/storage/ephemeral-volumes|Ephemeral Volumes（临时卷）]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume-health-monitoring|Volume Health Monitoring（卷健康监控）]]
- [[domain-17-system-foundation/topic-dictionary/storage/high-performance-storage-networks|高性能存储网络（RDMA / NVMe-oF）]]

## 关联文档 (K8s 集成)

### 故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting|PV/PVC 存储深度排查与持久化治理指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting|CSI 存储驱动深度排查与架构优化指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting|CSI 快照与卷备份故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/04-storage-performance-troubleshooting|存储 I/O 性能故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting|StorageClass 配置与动态供给故障排查指南]]

### K8s 事件

- [[domain-17-system-foundation/11-storage-volume-events|11 - 存储与卷事件]]

### 技能卡片

- [[domain-10-troubleshooting-diagnostics/topic-skills/07-pvc-storage-failure|PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation]]

### FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta|CSI 存储异常 FTA 树]]

## 扩展参考

### 存储生态项目

- [[domain-19-landscape-references/graduated/rook/rook|Rook]]
- [[domain-19-landscape-references/incubating/longhorn/longhorn|Longhorn]]
- [[domain-19-landscape-references/sandbox/openebs/openebs|OpenEBS]]
- [[domain-19-landscape-references/sandbox/hwameistor/hwameistor|HwameiStor]]
- [[domain-19-landscape-references/sandbox/k8up/k8up|K8up]]
- [[domain-19-landscape-references/sandbox/carina/carina|Carina]]
- [[domain-19-landscape-references/sandbox/piraeus-datastore/piraeus-datastore|Piraeus Datastore]]
- [[domain-19-landscape-references/graduated/cubefs/cubefs|CubeFS]]

### 存储基础

- [[domain-04-storage-data/02-block-file-object-storage|02 - 块存储、文件存储、对象存储]]
- [[domain-17-system-foundation/05-linux-storage-management|05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南]]
