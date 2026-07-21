---
title: storage
description: 存储体系标签枢纽 — 涵盖 CSI、PV/PVC、StorageClass、卷快照、分布式存储、有状态应用、备份恢复、存储性能调优等全部存储领域知识
category: tag-index
tags:
- storage
- csi
- persistent-volume
- storageclass
- distributed-storage
- backup
tier: core
difficulty: intermediate-to-advanced
domain: storage
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-11'
last_updated: '2026-07-21'
---

# storage Tag Hub

> 存储领域页面 — CSI、PVC、PV、StorageClass、Snapshot、分布式存储、备份恢复等。

## 核心定义

**Kubernetes 存储**是容器编排平台中管理数据持久化的子系统，通过 PV/PVC/StorageClass 抽象和 CSI（Container Storage Interface）插件体系，为有状态应用提供可靠、高性能的存储能力。

### 存储架构分层

| 层级 | 组件 | 职责 |
|------|------|------|
| 用户接口 | PVC (PersistentVolumeClaim) | 用户声明存储需求 |
| 控制平面 | PV Controller + CSI External Provisioner | 动态供给、绑定、回收 |
| 驱动接口 | CSI Driver (Controller + Node) | 与存储后端交互 |
| 存储后端 | 云磁盘/分布式存储/本地盘 | 实际数据存储 |
| 数据保护 | VolumeSnapshot + Velero | 快照、备份、恢复 |

### 存储类型对比

| 类型 | 访问模式 | 性能 | 典型场景 | 示例 |
|------|----------|------|----------|------|
| 块存储 | RWO | 极高 IOPS | 数据库、有状态应用 | AWS EBS, 阿里云 ESSD |
| 文件存储 | RWX | 中等 | 共享文件、日志 | NFS, EFS, NAS |
| 对象存储 | HTTP API | 高吞吐 | AI 训练、备份、归档 | S3, OSS, MinIO |
| 本地存储 | RWO | 极高 | 缓存、临时数据 | local-path, LVM |

### CSI 驱动生态

| CSI 驱动 | 存储后端 | 特色 |
|----------|----------|------|
| alibaba-cloud-csi | 阿里云 ESSD/NAS/OSS | 云原生、多协议 |
| aws-ebs-csi | AWS EBS | GP3/IO2 支持 |
| gcp-compute-csi | GCP PD | 区域持久化磁盘 |
| rook-ceph | Ceph | 开源分布式、块/文件/对象 |
| longhorn | Longhorn | 轻量级、UI 友好 |
| openebs | OpenEBS | 多引擎、简单部署 |
| nfs-csi | NFS Server | 共享存储、RWX |

## 生产实践要点

### 存储性能基准

| 指标 | SSD 目标 | HDD 目标 | 度量工具 |
|------|----------|----------|----------|
| IOPS (4K随机读) | > 50,000 | > 200 | fio |
| 吐吐量 (1M顺序) | > 500 MB/s | > 150 MB/s | fio |
| 延迟 (4K随机写) | < 1ms | < 10ms | fio |
| PVC 绑定时间 | < 10s | < 30s | kubectl |
| 快照创建 | < 5s | < 30s | kubectl |

### 常见存储故障快速定位

| 症状 | 可能原因 | 排查命令 |
|------|----------|----------|
| PVC Pending | SC 不存在/配额不足/驱动异常 | `kubectl describe pvc` |
| Pod 挂载失败 | 节点 CSI 插件异常/卷被占用 | `kubectl logs -n kube-system csi-plugin` |
| IO 延迟高 | 磁盘 IOPS 耗尽/网络拥塞 | `iostat -x 1`, `fio` |
| 卷扩容失败 | 文件系统不支持/SC 未开启 | `kubectl get sc -o yaml` |
| 快照失败 | VolumeSnapshotClass 缺失 | `kubectl get volumesnapshotclass` |

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
