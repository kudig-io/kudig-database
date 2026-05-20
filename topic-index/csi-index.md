---
title: CSI (Container Storage Interface) 知识图谱索引
description: CSI 容器存储接口知识图谱索引，聚合 CSI 架构、存储驱动、Snapshot、故障排查等所有相关内容
category: index
tags:
- k8s
- index
- catalog
- csi
- storage
- volume
- snapshot
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CSI 知识图谱索引 是什么
- CSI 存储相关内容
trigger_keywords:
- CSI
- Container Storage Interface
- 存储
- PV
- PVC
- Snapshot
---

# CSI (Container Storage Interface) 知识图谱索引

> 知识图谱索引：按关键字 **CSI** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### CSI 架构与核心概念
- [CSI 深度实践指南](./domain-19-papers/07-kubernetes-csi-storage-deep-practice.md)
- [CSI 容器存储接口深度解析](./domain-3-control-plane/22-container-storage-deep-dive.md)
- [CSI 驱动集成与运维管理](./domain-6-storage/05-csi-drivers-integration.md)
- [CSI 迁移：从 In-Tree 存储插件到 CSI](./domain-6-storage/16-csi-migration-in-tree-to-csi.md)
- [存储架构概览与核心组件](./domain-6-storage/01-storage-architecture-overview.md)

### CSI 故障排查
- [CSI 存储驱动故障排查](./topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md)
- [04 - Storage CSI 故障排查](./domain-12-troubleshooting/04-storage-csi-troubleshooting.md)
- [CSI FTA 故障树](./topic-fta/list/csi-fta.md)
- [PV/PVC 存储深度排查与持久化治理指南](./topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md)

### CSI YAML 配置
- [StorageClass / VolumeSnapshot YAML 配置参考](./domain-32-yaml-manifests/17-storageclass-volumesnapshot.md)
- [CSI 驱动资源 YAML 配置参考](./domain-32-yaml-manifests/18-csi-driver-resources.md)

## 关联文档 (K8s集成)

### PV/PVC 与存储卷
- [PV/PVC核心概念与企业级实践](./domain-6-storage/02-pv-architecture-fundamentals.md)
- [PVC模式与最佳实践](./domain-6-storage/03-pvc-patterns-practices.md)
- [存储基础概念详解](./domain-6-storage/06-storage-fundamental-concepts.md)
- [PersistentVolume YAML 配置参考](./domain-32-yaml-manifests/15-persistentvolume-reference.md)
- [PersistentVolumeClaim YAML 配置参考](./domain-32-yaml-manifests/16-persistentvolumeclaim-reference.md)

### 存储高级特性
- [存储高级特性](./domain-6-storage/11-storage-advanced-features.md)
- [CSI 快照与卷备份故障排查指南](./topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting.md)
- [存储 I/O 性能故障排查指南](./topic-structural-trouble-shooting/04-storage/04-storage-performance-troubleshooting.md)
- [StorageClass 配置与动态供给故障排查指南](./topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting.md)

### 存储日常运维
- [存储日常运维操作手册](./domain-6-storage/07-storage-daily-operations.md)
- [存储监控与告警体系](./domain-6-storage/12-storage-monitoring-alerting.md)
- [存储性能调优指南](./domain-6-storage/08-storage-performance-tuning.md)
- [存储备份与灾难恢复](./domain-6-storage/10-storage-backup-disaster-recovery.md)

### 云原生存储
- [云原生存储方案](./domain-6-storage/14-cloud-native-storage.md)
- [存储灾难恢复方案](./domain-6-storage/15-storage-disaster-recovery.md)

## 扩展参考

### 存储技能与故障处理
- [PVC/PV/CSI 存储故障诊断与修复](./topic-skills/07-pvc-storage-failure.md)
- [PV/PVC故障排查](./domain-6-storage/09-pv-pvc-troubleshooting.md)

### 存储术语词典
- [CSI Volume Cloning](./topic-dictionary/storage/csi-volume-cloning.md)
- [Volume Snapshots](./topic-dictionary/storage/volume-snapshots.md)
- [Volume Snapshot Classes](./topic-dictionary/storage/volume-snapshot-classes.md)
- [Persistent Volumes](./topic-dictionary/storage/persistent-volumes.md)
- [Storage Classes](./topic-dictionary/storage/storage-classes.md)
- [Volume Health Monitoring](./topic-dictionary/storage/volume-health-monitoring.md)
- [Ephemeral Volumes](./topic-dictionary/storage/ephemeral-volumes.md)
- [Dynamic Volume Provisioning](./topic-dictionary/storage/dynamic-volume-provisioning.md)
- [Volume Attributes Classes](./topic-dictionary/storage/volume-attributes-classes.md)
- [Storage Capacity](./topic-dictionary/storage/storage-capacity.md)
- [Node Specific Volume Limits](./topic-dictionary/storage/node-specific-volume-limits.md)

### CSI Driver 特定内容
- [AWS EBS CSI Driver](https://github.com/kubernetes-sigs/aws-ebs-csi-driver)
- [GCE PD CSI Driver](https://github.com/kubernetes-sigs/gcp-compute-persistent-disk-csi-driver)
- [阿里云 CSI Driver](./domain-17-cloud-provider/04-alicloud-ack/)
- [Azure Disk CSI Driver](https://github.com/kubernetes-sigs/azuredisk-csi-driver)
- [vSphere CSI Driver](https://github.com/kubernetes-sigs/vsphere-csi-driver)
- [Secret Store CSI Driver](https://github.com/kubernetes-sigs/secrets-store-csi-driver)

### 云厂商存储集成
- [ACK 云盘 CSI](./domain-17-cloud-provider/04-alicloud-ack/)
- [AWS EBS CSI](./domain-17-cloud-provider/01-aws-eks/)
- [GCP GCE PD CSI](./domain-17-cloud-provider/02-google-cloud-gke/)
- [Azure Disk CSI](./domain-17-cloud-provider/03-azure-aks/)
- [腾讯云 CBS CSI](./domain-17-cloud-provider/05-tencent-tke/)

### 学习培训
- [Day 20: CSI 存储](./topic-learn/inner-training/week-4-network-storage/day-20-csi-storage.md)
- [存储管理深入理解](./topic-learn/public-training/week-4-network-storage/)

### 生产运维
- [存储性能优化](./domain-18-production-operations/21-storage-performance-optimization.md)
- [企业级备份策略](./domain-18-production-operations/16-enterprise-backup-strategy.md)
- [灾难恢复演练](./domain-18-production-operations/17-disaster-recovery-drills.md)

### Kubernetes 版本相关
- [CHANGELOG-1.25 - CSI Ephemeral Volume GA](./topic-release-notes/kubernetes/CHANGELOG-1.25.md)
- [CHANGELOG-1.23 - CSI Volume Mount Group](./topic-release-notes/kubernetes/CHANGELOG-1.23.md)
- [CHANGELOG-1.21 - CSI Health Monitoring](./topic-release-notes/kubernetes/CHANGELOG-1.21.md)
- [CHANGELOG-1.19 - CSI Health Monitoring Alpha](./topic-release-notes/kubernetes/CHANGELOG-1.19.md)

### CNCF 生态
- [Kubernetes](./domain-34-cncf-landscape/graduated/kubernetes/kubernetes.md)
- [Rook](./domain-34-cncf-landscape/graduated/rook/rook.md)
- [OpenEBS](./domain-34-cncf-landscape/sandbox/openebs/openebs.md)
- [Longhorn](./domain-34-cncf-landscape/sandbox/longhorn/longhorn.md)
- [CubeFS](./domain-34-cncf-landscape/graduated/cubefs/cubefs.md)

### 其他相关
- [存储版本](./topic-dictionary/fundamentals/storage-versions.md)
- [高性能存储网络](./topic-dictionary/storage/high-performance-storage-networks.md)
- [块存储、文件存储、对象存储](./domain-16-storage-fundamentals/02-block-file-object-storage.md)
