---
title: AI 存储与高级存储
description: AI 工作负载存储与高级存储技术知识目录
summary: 覆盖 MinIO 对象存储、高性能并行文件系统、云 CSI 驱动、数据分层、拓扑感知、文件系统选型、基准测试、多租户隔离、Velero 备份、存储混沌工程
category: 存储
tags:
- storage
- ai
- csi
- object-storage
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- AI 工程师
estimated_read_time: 5min
intent_queries:
- "AI 存储有哪些内容"
- "高级存储知识目录"
trigger_keywords:
- AI存储
- 对象存储
- CSI
- 高性能存储
prerequisites:
- kubectl-basics
- storage-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# AI 存储与高级存储

本目录收录 AI 工作负载存储与高级存储技术的深度实践文章，面向 SRE、平台工程师和 AI 工程师，覆盖从对象存储到并行文件系统、从云 CSI 驱动到混沌工程的完整知识体系。

## 文章目录

### 对象存储与 AI 数据底座

- [[06-存储/07-AI存储与高级/01-minio-object-storage-ai.md|MinIO 对象存储 for AI/ML]] — MinIO 纠删码架构、K8s Operator 部署、AI 训练数据集/模型 artifact/Checkpoint 存储、S3 API 集成、Site Replication 灾备与故障排查。

### 高性能并行文件系统

- [[06-存储/07-AI存储与高级/02-high-perf-ai-storage-weka-lustre.md|AI 高性能存储：WekaFS/Lustre/BeeGFS/Alluxio]] — AI 训练存储需求分析、WekaFS CSI 部署、Lustre 架构集成、BeeGFS 并行文件系统、Alluxio 数据编排缓存加速、NVMe-oF 块存储及选型对比。

### 云厂商 CSI 驱动

- [[06-存储/07-AI存储与高级/03-cloud-csi-drivers-aws-azure-gcp.md|云厂商 CSI 驱动对比：AWS/Azure/GCP/Alibaba]] — AWS EBS/EFS、Azure Disk/File、GCP PD/Filestore、阿里云 ESSD/NAS CSI 驱动的功能对比、部署配置、性能特征与故障排查。

### 数据生命周期管理

- [[06-存储/07-AI存储与高级/04-data-tiering-ilm-archival.md|数据分层与生命周期管理]] — Hot/Warm/Cold/Archive 分层模型、StorageClass 分层策略、对象存储生命周期规则、AI 数据版本化管理与存储成本优化。

### 存储调度与拓扑

- [[06-存储/07-AI存储与高级/05-csi-topology-awareness.md|CSI 拓扑感知调度]] — CSI TopologyKey、AllowedTopologies、WaitForFirstConsumer 绑定模式、跨 AZ 卷迁移限制及 Pod 调度与卷拓扑冲突排查。

### 文件系统选型

- [[06-存储/07-AI存储与高级/06-filesystem-comparison-ext4-xfs-zfs.md|K8s 节点文件系统对比：ext4/XFS/ZFS/Btrfs]] — 四大文件系统特性对比、OverlayFS 底层选择、tmpfs/shm 在 AI 训练中的配置、文件系统参数调优与 inode 耗尽排查。

### 性能基准测试

- [[06-存储/07-AI存储与高级/07-storage-benchmarking-methodology.md|存储性能基准测试方法论]] — fio/IOR/mdtest/DLIO 工具链、AI 训练 I/O 模式建模、K8s 基准测试 Job 设计、指标解读与测试报告规范。

### 多租户隔离

- [[06-存储/07-AI存储与高级/08-storage-multitenant-isolation.md|存储多租户隔离]] — Namespace 级 PVC ResourceQuota、StorageClass RBAC 访问控制、CSI 驱动级隔离、存储网络分离与审计合规。

### 备份与恢复

- [[06-存储/07-AI存储与高级/09-velero-production-deep-dive.md|Velero 生产深度指南]] — Velero 架构（Server/Kopia/BSL/VSL）、生产部署、备份策略、恢复操作、CSI 快照集成、跨集群迁移与性能调优。

### 存储韧性验证

- [[06-存储/07-AI存储与高级/10-storage-chaos-engineering.md|存储混沌工程]] — 磁盘故障/网络分区/延迟注入、LitmusChaos 存储实验、Chaos Mesh IOChaos、PV/PVC 故障场景、数据一致性验证与演练剧本。

## 学习路径建议

**入门路径**：01 (MinIO) → 03 (云 CSI) → 05 (拓扑感知) → 09 (Velero)

**AI 平台路径**：02 (高性能存储) → 06 (文件系统) → 07 (基准测试) → 01 (MinIO)

**平台工程路径**：04 (数据分层) → 08 (多租户) → 10 (混沌工程) → 09 (Velero)

## 关联知识

本目录与以下知识库模块紧密关联：

- [[06-存储/01-K8s存储/06-csi-drivers-integration.md|K8s 存储 / CSI 驱动集成]] — CSI 驱动基础与 in-tree 迁移
- [[06-存储/03-分布式存储/05-juicefs-distributed-filesystem.md|分布式存储 / JuiceFS]] — 分布式文件系统实践
- [[06-存储/02-存储基础/02-block-file-object-storage.md|存储基础 / 块文件对象存储]] — 存储类型基础概念
- [[12-可靠性/01-备份恢复/03-pv-backup-snapshot.md|备份恢复 / PV 备份快照]] — PV 级数据保护
- [[12-可靠性/02-灾难恢复/01-multi-region-dr-architecture.md|灾难恢复 / 多区域架构]] — 跨地域灾备设计
- [[15-AI基础设施/01-基础设施/06-ai-data-pipeline.md|AI 基础设施 / 数据管线]] — AI 数据流全链路
