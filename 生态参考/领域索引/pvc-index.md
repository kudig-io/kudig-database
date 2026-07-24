---
title: PVC 知识图谱索引
description: '## PVC 知识图谱'
summary: '## PVC 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- pvc
- storage
- pv
- csi
- controller-manager
- rook
- statefulset
tier: core
created: '2026-05-23'
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
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PVC 知识图谱索引

> 知识图谱：按关键字 **pvc** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 存储知识域 (PV/PVC 核心)

- 01 - 存储架构概览与核心组件
- 02 - PV/PVC核心概念与企业级实践
- 03 - PVC使用模式与最佳实践
- 04 - StorageClass动态供给与多租户管理
- 05 - CSI驱动集成与运维管理
- 09 - PV/PVC故障排查与解决方案
- 10 - 存储备份与灾难恢复
- 16 - CSI 迁移：从 In-Tree 存储插件到 CSI
- [[存储/README.md|Storage Domain 存储领域知识库]]

### YAML 配置参考

- 15 - PersistentVolume YAML 配置参考
- 16 - PersistentVolumeClaim YAML 配置参考
- 17 - StorageClass / VolumeSnapshot YAML 配置参考
- 18 - CSI 驱动资源 YAML 配置参考

### 术语词典 (存储相关)

- [[系统基础/知识字典/storage/csi-volume-cloning.md|CSI Volume Cloning（CSI 卷克隆）]]
- [[系统基础/知识字典/storage/dynamic-volume-provisioning.md|Dynamic Volume Provisioning（动态卷供给）]]
- [[系统基础/知识字典/storage/ephemeral-volumes.md|Ephemeral Volumes（临时卷）]]
- [[系统基础/知识字典/storage/persistent-volumes.md|Persistent Volumes（持久卷）]]
- [[系统基础/知识字典/storage/storage-classes.md|Storage Classes（存储类）]]
- [[系统基础/知识字典/storage/volume-attributes-classes.md|Volume Attributes Classes（卷属性类）]]
- [[系统基础/知识字典/storage/volume-health-monitoring.md|Volume Health Monitoring（卷健康监控）]]
- [[系统基础/知识字典/storage/volume-snapshot-classes.md|Volume Snapshot Classes（卷快照类）]]
- [[系统基础/知识字典/storage/volume-snapshots.md|Volume Snapshots（卷快照）]]
- [[系统基础/知识字典/storage/volumes.md|Volumes（卷）]]

## 关联文档 (K8s 集成)

### 故障排查

- [[故障诊断/资源排障/14-pvc-storage-troubleshooting.md|14 - PVC与存储全面故障排查 (PVC & Storage Comprehensive Troubleshooting)]]
- [[故障诊断/核心排障/04-storage-csi-troubleshooting.md|04 - CSI 存储驱动故障排查 (CSI Driver Troubleshooting)]]
- [[故障诊断/资源排障/21-statefulset-troubleshooting.md|21 - StatefulSet 故障排查 (StatefulSet Troubleshooting)]]
- [[故障诊断/高级排障/04-storage/01-pv-pvc-troubleshooting.md|PV/PVC 存储深度排查与持久化治理指南]]
- [[故障诊断/高级排障/04-storage/02-csi-troubleshooting.md|CSI 存储驱动深度排查与架构优化指南]]
- [[故障诊断/高级排障/04-storage/03-snapshot-backup-troubleshooting.md|CSI 快照与卷备份故障排查指南]]
- [[故障诊断/高级排障/04-storage/04-storage-performance-troubleshooting.md|存储 I/O 性能故障排查指南]]
- [[故障诊断/高级排障/04-storage/05-storageclass-troubleshooting.md|StorageClass 配置与动态供给故障排查指南]]
- [[故障诊断/FTA故障树/list/csi-fta.md|CSI 存储异常 FTA 树]]

### 控制平面组件

- [[集群基础/控制平面/22-container-storage-deep-dive.md|22 container storage deep dive]]
- cloud-controller-manager 深度解析 (CCM Deep Dive)

### 技能卡片

- [[故障诊断/技能体系/07-pvc-storage-failure.md|PVC/PV/CSI 存储故障诊断与修复 / PVC/PV/CSI Storage Troubleshooting & Remediation]]
- [[故障诊断/技能体系/03-pod-pending.md|Pod Pending 调度失败诊断与修复]]

### K8s 事件

- 11 - 存储与卷事件

### 功能操作

- [[平台工程/代码分析/cluster-create/22-storage-volumes.md|存储与卷管理]]
- [[平台工程/代码分析/functions-node-create/14-storage-node.md|节点存储]]

### 设计原则

- 03 - 控制器模式与调谐循环 (Controller Pattern)

## 扩展参考

### 存储生态项目

- Rook
- Longhorn
- OpenEBS
- HwameiStor
- K8up
- Fluid
- Carina
- Piraeus Datastore

### 存储基础

- 05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南
- Domain-16 存储基础 — 开源项目索引

### 灾备与迁移

- [[发布变更/迁移方案/04-storage-data-migration.md|04 - 存储与数据迁移]]
- [[发布变更/迁移方案/06-stateful-services-migration.md|06 - 有状态服务迁移]]
- 17 - 灾难恢复演练
- [[故障诊断/FTA故障树/list/backup-restore-fta.md|备份/恢复异常 FTA 树]]

### 培训学习

- Kubernetes 存储体系全栈进阶培训 (从入门到专家)
- Day 26: 存储卷创建 & 删除
- Day 27: 存储卷挂载
- P4: 网络与存储综合实践


<!-- risk-assessed -->
