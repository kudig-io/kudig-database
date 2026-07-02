---
title: Storage 存储知识图谱索引
description: '## Storage 知识图谱'
summary: '## Storage 知识图谱'
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Storage 存储知识图谱索引

> 知识图谱：按关键字 **storage** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 存储知识域

- 01 - 存储架构概览与核心组件
- 02 - PV/PVC核心概念与企业级实践
- 03 - PVC使用模式与最佳实践
- 04 - StorageClass动态供给与多租户管理
- 05 - CSI驱动集成与运维管理
- 06 - 存储基础概念详解
- 08 - 存储性能调优与优化策略
- 09 - PV/PVC故障排查与解决方案
- 10 - 存储备份与灾难恢复
- 13 - 存储安全与合规管理
- 16 - CSI 迁移：从 In-Tree 存储插件到 CSI

### CSI 深度解析

- [[domain-01-cluster-fundamentals/03-control-plane/22-container-storage-deep-dive.md|CSI 容器存储接口深度解析 (Container Storage Interface Deep Dive)]]

### YAML 配置参考

- 15 - PersistentVolume YAML 配置参考
- 16 - PersistentVolumeClaim YAML 配置参考
- 17 - StorageClass / VolumeSnapshot YAML 配置参考
- 18 - CSI 驱动资源 YAML 配置参考

### 术语词典 (存储相关)

- Persistent Volumes（持久卷）
- [[domain-17-system-foundation/topic-dictionary/storage/storage-classes.md|Storage Classes（存储类）]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume-snapshots.md|Volume Snapshots（卷快照）]]
- [[domain-17-system-foundation/topic-dictionary/storage/csi-volume-cloning.md|CSI Volume Cloning（CSI 卷克隆）]]
- [[domain-17-system-foundation/topic-dictionary/storage/dynamic-volume-provisioning.md|Dynamic Volume Provisioning（动态卷供给）]]
- [[domain-17-system-foundation/topic-dictionary/storage/ephemeral-volumes.md|Ephemeral Volumes（临时卷）]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume-health-monitoring.md|Volume Health Monitoring（卷健康监控）]]
- [[domain-17-system-foundation/topic-dictionary/storage/high-performance-storage-networks.md|高性能存储网络（RDMA / NVMe-oF）]]

## 关联文档 (K8s 集成)

### 故障排查

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md|PV/PVC 存储深度排查与持久化治理指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md|CSI 存储驱动深度排查与架构优化指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting.md|CSI 快照与卷备份故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/04-storage-performance-troubleshooting.md|存储 I/O 性能故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting.md|StorageClass 配置与动态供给故障排查指南]]

### K8s 事件

- 11 - 存储与卷事件

### 技能卡片

- [[domain-10-troubleshooting-diagnostics/topic-skills/07-pvc-storage-failure.md|PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation]]

### FTA 故障树

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta.md|CSI 存储异常 FTA 树]]

## 扩展参考

### 存储生态项目

- Rook
- Longhorn
- OpenEBS
- HwameiStor
- K8up
- Carina
- Piraeus Datastore
- CubeFS

### 存储基础

- 02 - 块存储、文件存储、对象存储
- 05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南


<!-- risk-assessed -->
