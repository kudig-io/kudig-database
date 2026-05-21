---
title: 存储知识全项目索引 (Storage Knowledge Base Index)
description: '# 存储知识全项目索引 (Storage Knowledge Base Index)'
category: general
tags:
- k8s
- prometheus
- grafana
- docker
- rook
- ceph
- minio
- mysql
- postgresql
- statefulset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 存储知识全项目索引 (Storage Knowledge Base Index) 是什么
- 如何 存储知识全项目索引 (Storage Knowledge Base Index)
trigger_keywords:
- 存储知识全项目索引
- Storage
- Knowledge
- Base
- Index
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- mysql-basics
- backup-basics
---

# 存储知识全项目索引 (Storage Knowledge Base Index)

> **最后更新**: 2026-05 | **文档总数**: 73 | **总行数**: 48,205 | **覆盖范围**: Kubernetes 存储、Linux 存储、硬件存储、边缘存储

---

## 快速导航

| 我要做什么 | 推荐阅读 |
|-----------|---------|
| 从零学习 K8s 存储 | [06-存储基础概念](domain-04-storage-data/06-storage-fundamental-concepts.md) → [端到端三步示例](domain-04-storage-data/06-storage-fundamental-concepts.md) |
| 理解 PV/PVC/SC 关系 | [三者关系全景图](domain-04-storage-data/06-storage-fundamental-concepts.md) |
| PVC 出问题了 | [PVC 状态速查](domain-04-storage-data/03-pvc-patterns-practices.md) → [深度排查](domain-04-storage-data/09-pv-pvc-troubleshooting.md) |
| 配置 StorageClass | [动态供给详解](domain-04-storage-data/04-storageclass-dynamic-provisioning.md) → [YAML 参考](domain-18-manifests-patterns/17-storageclass-volumesnapshot.md) |
| CSI 驱动开发/调试 | [CSI 深度解析](domain-01-cluster-fundamentals/22-container-storage-deep-dive.md) → [CSI 排障](domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md) |
| 性能调优 | [性能调优](domain-04-storage-data/08-storage-performance-tuning.md) → [生产优化](domain-11-production-operations/21-storage-performance-optimization.md) |
| 备份容灾 | [备份方案](domain-04-storage-data/10-storage-backup-disaster-recovery.md) → [灾备策略](domain-04-storage-data/15-storage-disaster-recovery.md) |
| 存储迁移 | [迁移指南](domain-08-release-change-management/topic-migration/04-storage-data-migration.md) → [CSI 迁移](domain-04-storage-data/16-csi-migration-in-tree-to-csi.md) |
| 快速查阅术语 | [字典条目](#字典条目-topic-dictionarystorage-17-篇) |
| 培训教学 | [全栈培训](domain-11-production-operations/topic-presentations/kubernetes-storage-presentation.md) → [4 周课程](#培训材料-topic-learn-4-篇) |

---

## 1. Kubernetes 存储核心域 (domain-04-storage-data)

> **核心存储知识体系，新手入门起点。共 17 篇内容文件 + 3 篇管理文件，16,799 行。**

### 入门与架构

| 编号 | 文档 | 行数 | 内容概要 |
|:---:|------|-----:|---------|
| 06 | [存储基础概念详解](domain-04-storage-data/06-storage-fundamental-concepts.md) | 978 | **入门起点**。PV/PVC/SC 三者关系全景图、端到端三步示例、访问模式、回收策略、诊断脚本、实操练习 |
| 01 | [存储架构概览与核心组件](domain-04-storage-data/01-storage-architecture-overview.md) | 1,638 | 全面架构概览：PV/PVC/StorageClass、动态供给、CSI 生态、性能优化、成本优化、监控告警 |
| 02 | [PV 核心概念与企业级实践](domain-04-storage-data/02-pv-architecture-fundamentals.md) | 1,080 | PV 分层架构、规格字段、生命周期状态机、绑定算法与保护机制、Local PV、企业模板 |
| 03 | [PVC 使用模式与最佳实践](domain-04-storage-data/03-pvc-patterns-practices.md) | 1,091 | PVC 状态详解、volumeMode 选择、跨命名空间限制、动态/静态供给、StatefulSet VCT、扩容（在线/离线）、快照克隆、ResourceQuota |

### 配置与驱动

| 编号 | 文档 | 行数 | 内容概要 |
|:---:|------|-----:|---------|
| 04 | [StorageClass 动态供给](domain-04-storage-data/04-storageclass-dynamic-provisioning.md) | 1,608 | 动态供给时序、SC 参数详解、变更影响、多云配置（8 家云厂商）、多租户隔离、企业四级模板、CSI 对比矩阵 |
| 05 | [CSI 驱动集成与运维](domain-04-storage-data/05-csi-drivers-integration.md) | 1,322 | CSI 架构、组件角色、RPC 规格、阿里云 CSI 部署、VolumeSnapshot、性能调优、安全加固 |
| 16 | [CSI 迁移：In-Tree 到 CSI](domain-04-storage-data/16-csi-migration-in-tree-to-csi.md) | 545 | 迁移背景与必要性、翻译机制原理、各云厂商迁移状态、5 阶段操作流程、评估/验证脚本、回滚方案 |

### 运维与性能

| 编号 | 文档 | 行数 | 内容概要 |
|:---:|------|-----:|---------|
| 07 | [存储日常运维操作手册](domain-04-storage-data/07-storage-daily-operations.md) | 607 | 资源查询、PVC 管理、卷扩容、备份恢复、CSI 驱动运维、日常巡检脚本、应急响应 |
| 08 | [存储性能调优](domain-04-storage-data/08-storage-performance-tuning.md) | 803 | 存储类型性能对比、StorageClass 性能配置、本地存储优化、CSI 调优、数据库优化、fio 基准测试 |
| 12 | [存储监控与告警](domain-04-storage-data/12-storage-monitoring-alerting.md) | 856 | 监控指标体系、KPI 定义、Prometheus 配置、自定义 Exporter、容量规划（sklearn）、Grafana 仪表板 |

### 故障排查

| 编号 | 文档 | 行数 | 内容概要 |
|:---:|------|-----:|---------|
| 09 | [PV/PVC 故障排查](domain-04-storage-data/09-pv-pvc-troubleshooting.md) | 1,736 | PV/PVC 生命周期诊断、PVC Pending 决策树、多云 StorageClass 示例、VolumeSnapshot 排障、Prometheus 规则 |

### 安全与合规

| 编号 | 文档 | 行数 | 内容概要 |
|:---:|------|-----:|---------|
| 11 | [存储高级特性](domain-04-storage-data/11-storage-advanced-features.md) | 914 | 快照高级配置、PVC 克隆、增量快照、在线扩容、存储 QoS、加密（静态+传输）、分层缓存 |
| 13 | [存储安全与合规](domain-04-storage-data/13-storage-security-compliance.md) | 1,062 | 静态加密（StorageClass+KMS）、RBAC、PodSecurityPolicy、存储审计、密钥轮换、合规检查 |

### 容灾与云原生

| 编号 | 文档 | 行数 | 内容概要 |
|:---:|------|-----:|---------|
| 10 | [存储备份与灾难恢复](domain-04-storage-data/10-storage-backup-disaster-recovery.md) | 621 | 多层备份策略、Velero 企业方案、VolumeSnapshot 自动化、MySQL/PostgreSQL 备份、跨区域 DR |
| 14 | [云原生存储方案](domain-04-storage-data/14-cloud-native-storage.md) | 758 | 多云架构、混合云策略、跨云数据同步、成本分析、云厂商对比、成熟度模型 |
| 15 | [存储灾备与迁移](domain-04-storage-data/15-storage-disaster-recovery.md) | 599 | 灾备分层架构、自动/手动故障转移、存储迁移、跨集群同步、灾备演练、RTO/RPO SLA 管理 |

### 索引与管理

| 文档 | 行数 | 说明 |
|------|-----:|------|
| [00-开源项目索引](domain-04-storage-data/00-open-source-projects-index.md) | 181 | Rook/Longhorn/CubeFS/OpenEBS/MinIO 等 11 个项目，CSI 兼容矩阵，版本兼容表，选型决策树 |
| [README](domain-04-storage-data/README.md) | 133 | 域索引页，学习路径（新手从 06 开始），技术栈覆盖 |
| [质量检查报告](domain-04-storage-data/quality-check-report.md) | 137 | 质量评估：16 篇文档，184 YAML + 80 Shell + 11 Python |
| [查漏补缺报告](domain-04-storage-data/completion-summary.md) | 131 | 内容完善记录，技术要素统计 |

---

## 2. 存储基础域 (domain-04-storage-data)

> **存储底层技术原理。不依赖 Kubernetes，涵盖块/文件/对象存储、RAID、分布式存储、硬件性能。共 7 篇内容文件，4,616 行。**

| 编号 | 文档 | 行数 | 内容概要 |
|:---:|------|-----:|---------|
| 01 | [存储技术概述](domain-04-storage-data/01-storage-technologies-overview.md) | 433 | 存储类型分类（块/文件/对象）、架构演进（DAS/SAN/NAS/SDS）、存储协议、云存储服务、性能指标 |
| 02 | [块/文件/对象存储](domain-04-storage-data/02-block-file-object-storage.md) | 617 | iSCSI/NFS/MinIO 配置、企业最佳实践、监控脚本、性能基准测试、Ceph 统一存储 |
| 03 | [RAID 与存储冗余](domain-04-storage-data/03-raid-storage-redundancy.md) | 449 | RAID 0/1/5/6/10 详解、mdadm 配置、热备盘、健康检查脚本、Prometheus 监控、故障恢复 SOP |
| 04 | [分布式存储系统](domain-04-storage-data/04-distributed-storage-systems.md) | 557 | Ceph/MinIO/GlusterFS 架构与部署、纠删码、存储选型决策矩阵 |
| 05 | [企业级存储管理与运维](domain-04-storage-data/05-storage-management-operations.md) | 1,614 | 日常巡检、容量规划、数据分层、备份恢复、故障处理、安全合规、自动化管理、成熟度模型 |
| 06 | [存储性能与 IOPS](domain-04-storage-data/06-storage-performance-iops.md) | 593 | IOPS/吞吐/延迟、fio 测试、I/O 调度器、文件系统调优、内核参数、Prometheus 告警 |
| — | [开源项目索引](domain-04-storage-data/00-open-source-projects-index.md) | 267 | Rook/Longhorn/CubeFS 等项目深度分析，CSI 驱动矩阵，K8s 版本兼容表 |

---

## 3. 控制平面与深度原理

> **CSI 架构原理、gRPC 协议、驱动开发。面向架构师和高级开发者。**

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [CSI 容器存储接口深度解析](domain-01-cluster-fundamentals/22-container-storage-deep-dive.md) | 2,457 | CSI 规范演进、gRPC 服务定义、Go 代码实现、Sidecar 容器、部署 YAML、卷挂载全流程、CSIDriver/CSINode API |
| [CSI 深度实践论文](domain-19-landscape-references/07-kubernetes-csi-storage-deep-practice.md) | 706 | 自定义 CSI 驱动开发、SC 参数优化、ReadWriteOncePod GA、Volume Group Snapshots、跨命名空间卷克隆 |

---

## 4. 故障排查专题

> **结构化排障体系。按场景分层，含诊断脚本和决策树。**

### 域级排障

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [CSI 存储驱动排障](domain-10-troubleshooting-diagnostics/04-storage-csi-troubleshooting.md) | 513 | CSI 驱动供给/挂载/卸载/快照/扩容故障、紧急恢复脚本、Prometheus 告警 |
| [PVC 与存储全面排障](domain-10-troubleshooting-diagnostics/14-pvc-storage-troubleshooting.md) | 827 | PVC 状态流程图、Pending 诊断、Multi-Attach 解决、扩容排障、多云命令参考 |

### 结构化排障系列 (domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage)

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [PV/PVC 深度排查](domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md) | 1,289 | PV/PVC 生命周期、PVC Pending 诊断、Multi-Attach 解决、块设备层诊断、专家清理技巧 |
| [CSI 存储驱动深度排查](domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md) | 2,075 | CSI 架构深度、gRPC 调试、Sidecar 日志分析、完整生命周期、版本兼容、专家检查清单 |
| [快照与备份排障](domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/03-snapshot-backup-troubleshooting.md) | 680 | VolumeSnapshot 创建/恢复/清理故障、Finalizer 阻塞、孤儿 VolumeSnapshotContent、配额限制 |
| [存储 I/O 性能排障](domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/04-storage-performance-troubleshooting.md) | 737 | 高延迟 I/O 诊断、吞吐瓶颈、存储饱和/抖动、iostat/fio 分析、文件系统参数调优 |
| [StorageClass 配置排障](domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting.md) | 744 | 动态供给失败、绑定模式/拓扑问题、卷扩容问题、主流 CSI 驱动速查表 |

### 技能卡

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [PVC/PV/CSI 存储故障诊断技能](domain-10-troubleshooting-diagnostics/topic-skills/07-pvc-storage-failure.md) | 1,885 | 结构化诊断技能、症状模式匹配、2 分钟快速分诊、3 阶段诊断流程、13 类根因（含概率）、分级修复操作 |

---

## 5. 生产运维与优化

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [存储性能优化](domain-11-production-operations/21-storage-performance-optimization.md) | 1,208 | 本地存储优化（NVMe/Local PV）、云 CSI 调优、Python 性能分析器、智能分层、Prometheus+Grafana 配置 |

---

## 6. YAML 配置参考

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [StorageClass / VolumeSnapshot YAML 参考](domain-18-manifests-patterns/17-storageclass-volumesnapshot.md) | 1,413 | StorageClass 完整参数 YAML、VolumeSnapshot/VolumeSnapshotContent YAML、多云模板、生产案例、K8s 版本兼容矩阵 |

---

## 7. Kubernetes 事件参考

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [存储与卷事件](domain-17-system-foundation/11-storage-volume-events.md) | 2,126 | 全部存储相关 K8s 事件目录（Attach/Mount/Provisioning/Resize）、事件消息模式、根因表、生产案例 |

---

## 8. 硬件存储技术

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [机械硬盘技术](domain-17-system-foundation/06-storage-hdd-technology.md) | 584 | HDD 物理结构、记录技术演进（LMR/PMR/SMR/HAMR）、企业级规格、SMART 监控脚本 |
| [SSD 固态硬盘技术](domain-17-system-foundation/07-storage-ssd-technology.md) | 625 | SSD 架构（NAND/FTL）、NAND 类型（SLC/MLC/TLC/QLC）、NVMe 协议、DWPD/TBW、TRIM 优化 |
| [存储设备故障排查](domain-17-system-foundation/12-storage-troubleshooting.md) | 542 | HDD/SSD/RAID 硬件故障诊断、SMART 脚本、NVMe 健康、RAID 控制器诊断、数据恢复 |

---

## 9. 边缘与分布式存储

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [边缘存储与网络](domain-15-specialized-tech/08-edge-storage-network.md) | 2,380 | 边缘存储架构、RocksDB/SQLite 本地引擎、Longhorn/MinIO 边缘部署、弱网优化（QUIC）、离线数据同步（CRDT） |

---

## 10. 容器与操作系统存储

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [Docker 存储与数据卷](domain-13-container-runtime/05-docker-storage-volumes.md) | 601 | Docker overlay2 驱动、named volumes、bind mounts、tmpfs、NFS 卷驱动、备份恢复自动化 |
| [Linux 存储管理](domain-17-system-foundation/05-linux-storage-management.md) | 651 | LVM（PV/VG/LV）、软件 RAID（mdadm）、I/O 调度器、fio 测试、磁盘配额、文件系统调优（ext4/XFS）、NFS/iSCSI |

---

## 11. 云厂商存储

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [ACK EBS 云盘存储](domain-12-cloud-providers/04-alicloud-ack/245-ack-ebs-storage.md) | 119 | 阿里云 ESSD 性能等级（PL0-PL3）、ACK CSI StorageClass、磁盘加密、快照管理、成本模型 |
| [存储与数据迁移](domain-08-release-change-management/topic-migration/04-storage-data-migration.md) | 551 | 自建 K8s → ACK 迁移、StorageClass 映射、NFS→NAS/Ceph→ESSD/Local PV→云盘迁移、Velero 工作流 |

---

## 12. 培训与教学

### 培训演示

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [K8s 存储全栈培训](domain-11-production-operations/topic-presentations/kubernetes-storage-presentation.md) | 419 | 5 阶段培训（入门→架构→生产→排障→安全），含实操练习、YAML 示例、告警配置 |

### 内部培训 (domain-11-production-operations/topic-learn/inner-training)

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [Day 26: 存储卷创建与删除](domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-26-storage-create-delete.md) | 221 | PV/PVC/StorageClass 关系、创建/删除实操 |
| [Day 27: 存储卷挂载](domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-27-storage-mount.md) | 272 | Volume/PVC/ConfigMap/Secret 挂载、StatefulSet volumeClaimTemplates |
| [P4: 网络与存储综合实践](domain-11-production-operations/topic-learn/inner-training/projects/p4-network-storage-practice.md) | 322 | 微服务应用部署：MySQL StatefulSet + PVC、NFS 共享存储、网络策略 |

### 公开培训 (domain-11-production-operations/topic-learn/public-training)

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [Day 14: 存储体系 + 综合实践](domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-14-storage-practice.md) | 444 | PV/PVC/SC 机制回顾、静态/动态供给实操、生产级应用编排项目 |

---

## 13. 源码与函数参考

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [集群存储与卷管理](domain-02-workloads-applications/topic-functions/cluster-create/22-storage-volumes.md) | 275 | 存储类型概览、CSI 架构图、kubeadm 存储、Local PV、拓扑调度、卷限制 |
| [节点存储](domain-02-workloads-applications/topic-functions/node-create/14-storage-node.md) | 393 | emptyDir/hostPath/PV/PVC、CSI Node 插件两阶段挂载、CSIDriver API、挂载传播、节点诊断脚本 |

---

## 14. 字典条目 (domain-17-system-foundation/topic-dictionary/storage)

> **快速查阅，每个条目 100-190 行，含 YAML 示例和速查表。共 17 篇，2,343 行。**

| 条目 | 行数 | 核心概念 |
|------|-----:|---------|
| [Volumes（卷）](domain-17-system-foundation/topic-dictionary/storage/volumes.md) | 164 | Volume 类型（emptyDir/hostPath/nfs/iscsi/cephfs 等） |
| [Persistent Volumes（持久卷）](domain-17-system-foundation/topic-dictionary/storage/persistent-volumes.md) | 187 | PV/PVC 生命周期、绑定、回收策略、访问模式 |
| [Storage Classes（存储类）](domain-17-system-foundation/topic-dictionary/storage/storage-classes.md) | 146 | StorageClass 参数、provisioner、volumeBindingMode |
| [Volume Snapshots（卷快照）](domain-17-system-foundation/topic-dictionary/storage/volume-snapshots.md) | 131 | VolumeSnapshot/VolumeSnapshotContent 创建与恢复 |
| [CSI Volume Cloning（卷克隆）](domain-17-system-foundation/topic-dictionary/storage/csi-volume-cloning.md) | 115 | PVC-to-PVC 克隆、dataSource 引用 |
| [Dynamic Volume Provisioning](domain-17-system-foundation/topic-dictionary/storage/dynamic-volume-provisioning.md) | 120 | 动态 vs 静态供给、provisioner 工作流 |
| [Ephemeral Volumes（临时卷）](domain-17-system-foundation/topic-dictionary/storage/ephemeral-volumes.md) | 136 | 临时内联 CSI 卷、ConfigMap/Secret 卷 |
| [Projected Volumes（投射卷）](domain-17-system-foundation/topic-dictionary/storage/projected-volumes.md) | 143 | 多源投射、serviceAccountToken 投射 |
| [Local Ephemeral Storage](domain-17-system-foundation/topic-dictionary/storage/local-ephemeral-storage.md) | 130 | 节点临时存储、驱逐阈值、资源配额 |
| [Volume Health Monitoring](domain-17-system-foundation/topic-dictionary/storage/volume-health-monitoring.md) | 121 | 卷健康状态检测、CSI NodeGetVolumeStats |
| [Volume Attributes Classes](domain-17-system-foundation/topic-dictionary/storage/volume-attributes-classes.md) | 153 | VolumeAttributesClass (v1.29+)、动态修改卷属性 |
| [Node Volume Limits](domain-17-system-foundation/topic-dictionary/storage/[[domain-17-system-foundation/topic-dictionary/storage/node-specific-volume-limits|node-specific-volume-limits]].md) | 114 | 节点卷挂载数量限制、云厂商限制 |
| [高性能存储网络](domain-17-system-foundation/topic-dictionary/storage/high-performance-storage-networks.md) | 145 | RDMA、NVMe-oF、RoCE、iWARP |
| [对象存储与数据流水线](domain-17-system-foundation/topic-dictionary/storage/object-storage-and-data-pipelines.md) | 178 | S3/MinIO 集成、COSI（容器对象存储接口） |
| [Storage Capacity](domain-17-system-foundation/topic-dictionary/storage/storage-capacity.md) | 105 | CSIStorageCapacity、拓扑感知供给 |
| [Volume Snapshot Classes](domain-17-system-foundation/topic-dictionary/storage/volume-snapshot-classes.md) | 109 | VolumeSnapshotClass 参数、驱动配置 |
| [Windows Storage](domain-17-system-foundation/topic-dictionary/storage/windows-storage.md) | 146 | Windows 节点存储、SMB 挂载、gMSA |

---

## 15. 维基草稿

| 文档 | 行数 | 内容概要 |
|------|-----:|---------|
| [存储体系全景文章](.zread/wiki/drafts/10-cun-chu-ti-xi-pv-pvc-storageclass-csi-qu-dong-yu-zai-bei-hui-fu.md) | 688 | 贯穿 15 篇存储域文档的综述文章，从 PVC 抽象层到 CSI 到灾备恢复的完整链路，含 Mermaid 架构图 |

---

## 学习路径

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          学习路径推荐                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  🔰 新手 (1-2 周)                                                      │
│  ├── 06-存储基础概念 (三者关系全景图 + 端到端示例)                       │
│  ├── 02-PV 核心概念 (生命周期 + 绑定机制)                               │
│  ├── 03-PVC 使用模式 (配置 + 扩容 + 快照)                               │
│  └── 04-StorageClass (动态供给 + 多云配置)                              │
│                                                                         │
│  🚀 进阶 (2-4 周)                                                      │
│  ├── 05-CSI 驱动集成 (架构 + 部署 + 运维)                               │
│  ├── 08-存储性能调优 (基准测试 + 调优参数)                               │
│  ├── 09-PV/PVC 故障排查 (诊断脚本 + 决策树)                             │
│  └── 16-CSI 迁移 (In-Tree → CSI 迁移路径)                               │
│                                                                         │
│  👨‍💻 专家 (持续)                                                        │
│  ├── 12-存储监控告警 (Prometheus + Grafana + 容量预测)                   │
│  ├── 13-存储安全合规 (加密 + RBAC + 审计)                                │
│  ├── 10/15-备份容灾 (Velero + 灾备演练 + SLA)                           │
│  ├── 22-CSI 深度解析 (gRPC + Go 实现 + 源码)                            │
│  └── 结构化排障系列 (5 篇专项排障指南)                                    │
│                                                                         │
│  🔧 专项                                                                │
│  ├── 硬件存储 → domain-31-hardware (HDD/SSD/RAID)                      │
│  ├── 边缘存储 → domain-37-edge-computing (弱网/离线同步)                 │
│  ├── Docker 存储 → domain-13-container-runtime (overlay2/卷驱动)                    │
│  ├── Linux 存储 → domain-17-system-foundation (LVM/RAID/IO调度)                     │
│  └── 快速查词 → domain-17-system-foundation/topic-dictionary/storage (17 个字典条目)                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 按维度统计

| 维度 | 文件数 | 总行数 |
|------|-------:|-------:|
| **K8s 存储核心** (domain-04-storage-data) | 17 | 15,381 |
| **存储基础原理** (domain-04-storage-data) | 7 | 4,616 |
| **CSI 深度原理** | 2 | 3,163 |
| **故障排查** (排障+技能卡) | 9 | 8,820 |
| **生产运维** | 1 | 1,208 |
| **YAML 参考** | 1 | 1,413 |
| **事件参考** | 1 | 2,126 |
| **硬件存储** | 3 | 1,751 |
| **边缘存储** | 1 | 2,380 |
| **容器/OS 存储** | 2 | 1,252 |
| **云厂商存储** | 2 | 670 |
| **培训教学** | 5 | 1,678 |
| **源码参考** | 2 | 668 |
| **字典条目** | 17 | 2,343 |
| **维基草稿** | 1 | 688 |
| **索引/管理文件** | 4 | 582 |
| **合计** | **73** | **48,205** |

---

**维护者**: Kusheet Project | **联系**: allen.galler@gmail.com | **最后更新**: 2026-05
