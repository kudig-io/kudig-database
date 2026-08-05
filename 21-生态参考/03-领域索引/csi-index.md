---
title: CSI (Container Storage Interface) 知识图谱索引
description: CSI 容器存储接口知识图谱索引，聚合 CSI 架构、存储驱动、Snapshot、故障排查等所有相关内容
summary: CSI 容器存储接口知识图谱索引，聚合 CSI 架构、存储驱动、Snapshot、故障排查等所有相关内容
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CSI (Container Storage Interface) 知识图谱索引

> 知识图谱索引：按关键字 **CSI** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### CSI 架构与核心概念
- CSI 深度实践指南
- CSI 容器存储接口深度解析
- CSI 驱动集成与运维管理
- CSI 迁移：从 In-Tree 存储插件到 CSI
- 存储架构概览与核心组件

### CSI 故障排查
- CSI 存储驱动故障排查
- [[19-故障诊断/01-核心排障/04-storage-csi-troubleshooting.md|04 - Storage CSI 故障排查]]
- [[19-故障诊断/06-FTA故障树/list/csi-fta.md|CSI FTA 故障树]]
- [[19-故障诊断/04-高级排障/structural-04-storage/01-pv-pvc-troubleshooting.md|PV/PVC 存储深度排查与持久化治理指南]]

### CSI YAML 配置
- StorageClass / VolumeSnapshot YAML 配置参考
- CSI 驱动资源 YAML 配置参考

## 关联文档 (K8s集成)

### PV/PVC 与存储卷
- PV/PVC核心概念与企业级实践
- PVC模式与最佳实践
- 存储基础概念详解
- PersistentVolume YAML 配置参考
- PersistentVolumeClaim YAML 配置参考

### 存储高级特性
- 存储高级特性
- [[19-故障诊断/04-高级排障/structural-04-storage/03-snapshot-backup-troubleshooting.md|CSI 快照与卷备份故障排查指南]]
- [[19-故障诊断/04-高级排障/structural-04-storage/04-storage-performance-troubleshooting.md|存储 I/O 性能故障排查指南]]
- [[19-故障诊断/04-高级排障/structural-04-storage/05-storageclass-troubleshooting.md|StorageClass 配置与动态供给故障排查指南]]

### 存储日常运维
- 存储日常运维操作手册
- 存储监控与告警体系
- 存储性能调优指南
- 存储备份与灾难恢复

### 云原生存储
- 云原生存储方案
- 存储灾难恢复方案

## 扩展参考

### 存储技能与故障处理
- [[19-故障诊断/08-技能体系/08-pvc-storage-failure.md|PVC/PV/CSI 存储故障诊断与修复]]
- PV/PVC故障排查

### 存储术语词典
- [[17-系统基础/06-知识字典/storage/csi-volume-cloning.md|CSI Volume Cloning]]
- [[17-系统基础/06-知识字典/storage/volume-snapshots.md|Volume Snapshots]]
- [[17-系统基础/06-知识字典/storage/volume-snapshot-classes.md|Volume Snapshot Classes]]
- [[17-系统基础/06-知识字典/storage/persistent-volumes.md|Persistent Volumes]]
- [[17-系统基础/06-知识字典/storage/storage-classes.md|Storage Classes]]
- [[17-系统基础/06-知识字典/storage/volume-health-monitoring.md|Volume Health Monitoring]]
- [[17-系统基础/06-知识字典/storage/ephemeral-volumes.md|Ephemeral Volumes]]
- [[17-系统基础/06-知识字典/storage/dynamic-volume-provisioning.md|Dynamic Volume Provisioning]]
- [[17-系统基础/06-知识字典/storage/volume-attributes-classes.md|Volume Attributes Classes]]
- [[17-系统基础/06-知识字典/storage/storage-capacity.md|Storage Capacity]]
- [[17-系统基础/06-知识字典/storage/node-specific-volume-limits.md|Node Specific Volume Limits]]

### CSI Driver 特定内容
- [AWS EBS CSI Driver](https://github.com/kubernetes-sigs/aws-ebs-csi-driver)
- [GCE PD CSI Driver](https://github.com/kubernetes-sigs/gcp-compute-persistent-disk-csi-driver)
- 阿里云 CSI Driver
- [Azure Disk CSI Driver](https://github.com/kubernetes-sigs/azuredisk-csi-driver)
- [vSphere CSI Driver](https://github.com/kubernetes-sigs/vsphere-csi-driver)
- [Secret Store CSI Driver](https://github.com/kubernetes-sigs/secrets-store-csi-driver)

### 云厂商存储集成
- ACK 云盘 CSI
- AWS EBS CSI
- GCP GCE PD CSI
- Azure Disk CSI
- 腾讯云 CBS CSI

### 学习培训
- Day 20: CSI 存储
- 存储管理深入理解

### 生产运维
- 存储性能优化
- 企业级备份策略
- 灾难恢复演练

### Kubernetes 版本相关
- [[37-归档/release-notes/kubernetes/CHANGELOG-1.25.md|CHANGELOG-1.25 - CSI Ephemeral Volume GA]]
- CHANGELOG-1.23 - CSI Volume Mount Group
- CHANGELOG-1.21 - CSI Health Monitoring
- CHANGELOG-1.19 - CSI Health Monitoring Alpha

### CNCF 生态
- Kubernetes
- Rook
- OpenEBS
- Longhorn
- CubeFS

### 其他相关
- [[17-系统基础/06-知识字典/fundamentals/storage-versions.md|存储版本]]
- [[17-系统基础/06-知识字典/storage/high-performance-storage-networks.md|高性能存储网络]]
- 块存储、文件存储、对象存储


<!-- risk-assessed -->
