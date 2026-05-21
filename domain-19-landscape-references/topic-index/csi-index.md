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
- rook
- redis
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
prerequisites:
- kubectl-basics
- cncf-ecosystem
- redis-basics
---

# CSI (Container Storage Interface) 知识图谱索引

> 知识图谱索引：按关键字 **CSI** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### CSI 架构与核心概念
- [[domain-19-landscape-references/07-kubernetes-csi-storage-deep-practice|CSI 深度实践指南]]
- [[domain-01-cluster-fundamentals/22-container-storage-deep-dive|CSI 容器存储接口深度解析]]
- [[domain-04-storage-data/05-csi-drivers-integration|CSI 驱动集成与运维管理]]
- [[domain-04-storage-data/16-csi-migration-in-tree-to-csi|CSI 迁移：从 In-Tree 存储插件到 CSI]]
- [[domain-04-storage-data/01-storage-architecture-overview|存储架构概览与核心组件]]

### CSI 故障排查
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting|CSI 存储驱动故障排查]]
- [[domain-10-troubleshooting-diagnostics/04-storage-csi-troubleshooting|04 - Storage CSI 故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta|CSI FTA 故障树]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting|PV/PVC 存储深度排查与持久化治理指南]]

### CSI YAML 配置
- [[domain-18-manifests-patterns/17-storageclass-volumesnapshot|StorageClass / VolumeSnapshot YAML 配置参考]]
- [[domain-18-manifests-patterns/18-csi-driver-resources|CSI 驱动资源 YAML 配置参考]]

## 关联文档 (K8s集成)

### PV/PVC 与存储卷
- [[domain-04-storage-data/02-pv-architecture-fundamentals|PV/PVC核心概念与企业级实践]]
- [[domain-04-storage-data/03-pvc-patterns-practices|PVC模式与最佳实践]]
- [[domain-04-storage-data/06-storage-fundamental-concepts|存储基础概念详解]]
- [[domain-18-manifests-patterns/15-persistentvolume-reference|PersistentVolume YAML 配置参考]]
- [[domain-18-manifests-patterns/16-persistentvolumeclaim-reference|PersistentVolumeClaim YAML 配置参考]]

### 存储高级特性
- [[domain-04-storage-data/11-storage-advanced-features|存储高级特性]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting|CSI 快照与卷备份故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/04-storage-performance-troubleshooting|存储 I/O 性能故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting|StorageClass 配置与动态供给故障排查指南]]

### 存储日常运维
- [[domain-04-storage-data/07-storage-daily-operations|存储日常运维操作手册]]
- [[domain-04-storage-data/12-storage-monitoring-alerting|存储监控与告警体系]]
- [[domain-04-storage-data/08-storage-performance-tuning|存储性能调优指南]]
- [[domain-04-storage-data/10-storage-backup-disaster-recovery|存储备份与灾难恢复]]

### 云原生存储
- [[domain-04-storage-data/14-cloud-native-storage|云原生存储方案]]
- [[domain-04-storage-data/15-storage-disaster-recovery|存储灾难恢复方案]]

## 扩展参考

### 存储技能与故障处理
- [[domain-10-troubleshooting-diagnostics/topic-skills/07-pvc-storage-failure|PVC/PV/CSI 存储故障诊断与修复]]
- [[domain-04-storage-data/09-pv-pvc-troubleshooting|PV/PVC故障排查]]

### 存储术语词典
- [[domain-17-system-foundation/topic-dictionary/storage/csi-volume-cloning|CSI Volume Cloning]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume-snapshots|Volume Snapshots]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume-snapshot-classes|Volume Snapshot Classes]]
- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volumes|Persistent Volumes]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-classes|Storage Classes]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume-health-monitoring|Volume Health Monitoring]]
- [[domain-17-system-foundation/topic-dictionary/storage/ephemeral-volumes|Ephemeral Volumes]]
- [[domain-17-system-foundation/topic-dictionary/storage/dynamic-volume-provisioning|Dynamic Volume Provisioning]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume-attributes-classes|Volume Attributes Classes]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-capacity|Storage Capacity]]
- [[domain-17-system-foundation/topic-dictionary/storage/node-specific-volume-limits|Node Specific Volume Limits]]

### CSI Driver 特定内容
- [AWS EBS CSI Driver](https://github.com/kubernetes-sigs/aws-ebs-csi-driver)
- [GCE PD CSI Driver](https://github.com/kubernetes-sigs/gcp-compute-persistent-disk-csi-driver)
- [[domain-12-cloud-providers/04-alicloud-ack/|阿里云 CSI Driver]]
- [Azure Disk CSI Driver](https://github.com/kubernetes-sigs/azuredisk-csi-driver)
- [vSphere CSI Driver](https://github.com/kubernetes-sigs/vsphere-csi-driver)
- [Secret Store CSI Driver](https://github.com/kubernetes-sigs/secrets-store-csi-driver)

### 云厂商存储集成
- [[domain-12-cloud-providers/04-alicloud-ack/|ACK 云盘 CSI]]
- [[domain-12-cloud-providers/01-aws-eks/|AWS EBS CSI]]
- [[domain-12-cloud-providers/02-google-cloud-gke/|GCP GCE PD CSI]]
- [[domain-12-cloud-providers/03-azure-aks/|Azure Disk CSI]]
- [[domain-12-cloud-providers/05-tencent-tke/|腾讯云 CBS CSI]]

### 学习培训
- [[domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-20-csi-storage|Day 20: CSI 存储]]
- [[domain-11-production-operations/topic-learn/public-training/week-4-network-storage/|存储管理深入理解]]

### 生产运维
- [[domain-11-production-operations/21-storage-performance-optimization|存储性能优化]]
- [[domain-11-production-operations/16-enterprise-backup-strategy|企业级备份策略]]
- [[domain-11-production-operations/17-disaster-recovery-drills|灾难恢复演练]]

### Kubernetes 版本相关
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.25|CHANGELOG-1.25 - CSI Ephemeral Volume GA]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.23|CHANGELOG-1.23 - CSI Volume Mount Group]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.21|CHANGELOG-1.21 - CSI Health Monitoring]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.19|CHANGELOG-1.19 - CSI Health Monitoring Alpha]]

### CNCF 生态
- [[domain-19-landscape-references/graduated/kubernetes/kubernetes|Kubernetes]]
- [[domain-19-landscape-references/graduated/rook/rook|Rook]]
- [[domain-19-landscape-references/sandbox/openebs/openebs|OpenEBS]]
- [[domain-19-landscape-references/sandbox/longhorn/longhorn|Longhorn]]
- [[domain-19-landscape-references/graduated/cubefs/cubefs|CubeFS]]

### 其他相关
- [[domain-17-system-foundation/topic-dictionary/fundamentals/storage-versions|存储版本]]
- [[domain-17-system-foundation/topic-dictionary/storage/high-performance-storage-networks|高性能存储网络]]
- [[domain-04-storage-data/02-block-file-object-storage|块存储、文件存储、对象存储]]
