---
title: storage
description: All pages tagged with storage
category: tag-index
tags:
- storage
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
---

# storage Tag Hub

> 存储领域页面 — CSI、PVC、PV、StorageClass、Snapshot、分布式存储、备份恢复等。

## K8s 存储 (K8s Storage)

- [[存储/K8s存储/01-storage-architecture-overview|存储架构概览]]
- [[存储/K8s存储/02-pv-architecture-fundamentals|PV 架构基础]]
- [[存储/K8s存储/03-pvc-patterns-practices|PVC 模式与实践]]
- [[存储/K8s存储/04-storageclass-dynamic-provisioning|StorageClass 动态供给]]
- [[存储/K8s存储/05-csi-drivers-integration|CSI 驱动集成]]
- [[存储/K8s存储/08-storage-performance-tuning|存储性能调优]]
- [[存储/K8s存储/09-pv-pvc-troubleshooting|PV/PVC 故障排查]]
- [[存储/K8s存储/10-storage-backup-disaster-recovery|存储备份与灾备]]
- [[存储/K8s存储/11-storage-advanced-features|存储高级特性]]
- [[存储/K8s存储/12-storage-monitoring-alerting|存储监控告警]]
- [[存储/K8s存储/13-storage-security-compliance|存储安全合规]]
- [[存储/K8s存储/17-volume-snapshot-scheduling|卷快照调度]]
- [[存储/K8s存储/18-storage-encryption-at-rest|静态加密存储]]

## 存储基础 (Storage Fundamentals)

- [[存储/存储基础/01-storage-technologies-overview|存储技术概览]]
- [[存储/存储基础/02-block-file-object-storage|块/文件/对象存储]]
- [[存储/存储基础/03-raid-storage-redundancy|RAID 存储冗余]]
- [[存储/存储基础/04-distributed-storage-systems|分布式存储系统]]
- [[存储/存储基础/05-storage-management-operations|存储管理运维]]
- [[存储/存储基础/06-storage-performance-iops|存储性能 IOPS]]

## 分布式存储 (Distributed Storage)

- [[存储/分布式存储/04-openebs-production|OpenEBS 生产实践]]
- [[存储/分布式存储/05-juicefs-distributed-filesystem|JuiceFS 分布式文件系统]]
- [[存储/分布式存储/06-nfs-csi-production-guide|NFS CSI 生产指南]]

## 有状态应用 (Stateful Applications)

- [[存储/有状态应用存储/01-stateful-app-storage-patterns|有状态应用存储模式]]
- [[存储/有状态应用存储/02-mysql-statefulset-production|MySQL StatefulSet 生产]]
- [[存储/有状态应用存储/03-postgresql-statefulset-production|PostgreSQL StatefulSet 生产]]
- [[存储/有状态应用存储/04-kafka-statefulset-production|Kafka StatefulSet 生产]]

## 概念 (Concepts)

- [[概念/storage-model|存储模型]]
- [[概念/pv|PersistentVolume (PV)]]
- [[概念/persistent-volume-claim|PersistentVolumeClaim]]
- [[概念/storageclass|StorageClass]]
- [[概念/csi-drivers|CSI 驱动]]
- [[概念/cloud-native-storage-systems|云原生存储系统]]
- [[概念/storage-performance-optimization|存储性能优化]]
- [[概念/storage-data-protection|存储数据保护]]
- [[概念/velero-disaster-recovery|Velero 灾难恢复]]

## 清单模式 (Manifest Patterns)

- [[清单模式/YAML参考/15-persistentvolume-reference|PersistentVolume YAML 参考]]
- [[清单模式/YAML参考/16-persistentvolumeclaim-reference|PersistentVolumeClaim YAML 参考]]
- [[清单模式/YAML参考/17-storageclass-volumesnapshot|StorageClass 卷快照]]
- [[清单模式/YAML参考/18-csi-driver-resources|CSI 驱动资源]]

## 故障诊断 (Troubleshooting)

- [[故障诊断/核心排障/04-storage-csi-troubleshooting|存储 CSI 排障]]
- [[故障诊断/资源排障/14-pvc-storage-troubleshooting|PVC 存储排障]]
- [[故障诊断/高级排障/structural-04-storage/04-storage-performance-troubleshooting|存储性能排障]]
- [[故障诊断/高级排障/structural-04-storage/05-storageclass-troubleshooting|StorageClass 排障]]

## 可靠性 (Reliability)

- [[可靠性/备份恢复/03-pv-backup-snapshot|PV 备份快照]]
- [[可靠性/灾难恢复/17-storage-backend-failure-playbook|存储后端故障 Playbook]]

## 云厂商存储 (Cloud Provider Storage)

- [[云厂商/AWS-EKS/04-eks-storage-efs-fsx|AWS EKS 存储 EFS/FSx]]
- [[云厂商/Azure-AKS/04-aks-storage-managed-disk|Azure AKS 存储托管磁盘]]
- [[云厂商/Google-GKE/04-gke-storage-filestore-gcs|GKE 存储 Filestore/GCS]]
- [[云厂商/阿里云/04-阿里云存储集成|阿里云存储集成]]
- [[云厂商/华为云CCE/03-cce-storage-evs-sfs|华为云 CCE 存储 EVS/SFS]]

## 知识字典 (Knowledge Dictionary)

- [[系统基础/知识字典/storage/csi|CSI]]
- [[系统基础/知识字典/storage/persistent-volume|PersistentVolume]]
- [[系统基础/知识字典/storage/persistent-volume-claim|PersistentVolumeClaim]]
- [[系统基础/知识字典/storage/storage-class|StorageClass]]
- [[系统基础/知识字典/storage/rook|Rook]]
- [[系统基础/知识字典/storage/ceph|Ceph]]
- [[系统基础/知识字典/storage/longhorn|Longhorn]]
- [[系统基础/知识字典/storage/openebs|OpenEBS]]

## 实体 (Entities)

- [[实体/rook|Rook]]
- [[实体/longhorn|Longhorn]]
- [[实体/openebs|OpenEBS]]
- [[实体/hwameistor|Hwameistor]]
- [[实体/csi-drivers|CSI Drivers]]
- [[实体/cncf-storage|CNCF Storage]]
- [[实体/k8s-storage-ecosystem|Kubernetes Storage Ecosystem]]

## 生态参考 (Ecosystem)

- [[生态参考/领域索引/csi-index|CSI 索引]]
- [[生态参考/领域索引/pvc-index|PVC 索引]]
- [[生态参考/领域索引/storage-index|存储索引]]
- [[生态参考/论文/07-kubernetes-csi-storage-deep-practice|Kubernetes CSI 深度实践]]

## Related Tags

- [[标签/k8s|k8s]]
- [[标签/reliability|reliability]]
- [[标签/best-practices|best-practices]]
