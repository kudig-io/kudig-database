---
title: Domain-16 存储基础 — 开源项目索引
description: '# Domain-16 存储基础 — 开源项目索引'
category: storage-fundamentals
tags:
- storage
- filesystem
- block
- rook
- ceph
- minio
- redis
- mysql
- postgresql
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 存储工程师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-16 存储基础 — 开源项目索引 是什么
- 如何 Domain-16 存储基础 — 开源项目索引
- Kubernetes 16 storage fundamentals 最佳实践
trigger_keywords:
- Domain-16
- 存储基础
- 开源项目索引
- storage
- fundamentals
prerequisites:
- kubectl-basics
- storage-basics
- redis-basics
- mysql-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
created: "2026-05-23"
---

# Domain-16 存储基础 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: Rook v1.16 / Longhorn v1.8 / CubeFS v3.5

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、Rook (CNCF Graduated)](#二rook-cncf-graduated)
- [三、Longhorn (CNCF Incubating)](#三longhorn-cncf-incubating)
- [四、CubeFS (CNCF Graduated)](#四cubefs-cncf-graduated)
- [五、其他云原生存储](#五其他云原生存储)
- [六、存储类别与 CSI](#六存储类别与-csi)
- [七、版本兼容矩阵](#七版本兼容矩阵)
- [八、存储选型指南](#八存储选型指南)

---

## 一、核心项目总览

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Rook** | 云原生存储编排 | Graduated | v1.16.0 | 12.5k+ | Apache-2.0 |
| **Longhorn** | 分布式块存储 | Incubating | v1.8.0 | 6k+ | Apache-2.0 |
| **CubeFS** | 分布式文件存储 | Graduated | v3.5.0 | 4.5k+ | Apache-2.0 |
| **OpenEBS** | K8s 原生存储 | 非 CNCF | v4.2.0 | 8.5k+ | Apache-2.0 |
| **Vitess** | MySQL 水平扩展 | Graduated | v21.0.0 | 18k+ | Apache-2.0 |
| **TiKV** | 分布式 KV 存储 | Graduated | v8.5.0 | 15k+ | Apache-2.0 |
| **Ceph** | 统一分布式存储 | 非 CNCF | v19.2.0 | 14k+ | LGPL-2.1 |
| **MinIO** | 高性能对象存储 | 非 CNCF | v2025.04 | 50k+ | AGPL-3.0 |
| **JuiceFS** | 云原生分布式文件系统 | 非 CNCF | v1.2.0 | 11k+ | Apache-2.0 |
| **Portworx** | 企业级容器存储 | Pure Storage | v3.2.0 | - | 商业 |
| **StorageOS** | 软件定义存储 | Ondat | v2.10.0 | - | 商业 |

---

## 二、Rook (CNCF Graduated)

### 2.1 Ceph 编排器

```yaml
# 核心能力
- 自动部署和管理 Ceph 集群
- 对象存储 (RGW / S3)
- 块存储 (RBD) ──► RWO PVC
- 文件系统 (CephFS) ──► RWX PVC
- 自动故障恢复与再平衡
- 通过 CRD 声明式管理
```

### 2.2 关键 CRD

| CRD | 作用 |
|:---|:---|
| CephCluster | Ceph 集群配置 (MON/OSD/MGR) |
| CephBlockPool | RBD 存储池 |
| CephFilesystem | CephFS 文件系统 |
| CephObjectStore | RGW 对象存储 |
| StorageClass | 与 PVC 绑定 |

### 2.3 部署示例

```yaml
apiVersion: ceph.rook.io/v1
kind: CephCluster
metadata:
  name: rook-ceph
  namespace: rook-ceph
spec:
  cephVersion:
    image: quay.io/ceph/ceph:v19.2.0
  dataDirHostPath: /var/lib/rook
  mon:
    count: 3
  storage:
    useAllNodes: true
    useAllDevices: true
```

**GitHub**: https://github.com/rook/rook
**文档**: https://rook.io/docs/rook/latest-release/

---

## 三、Longhorn (CNCF Incubating)

### 3.1 轻量级分布式块存储

```yaml
# 核心特性
- 基于微服务架构 (Controller + Replica)
- 每个卷有多个副本 (默认 3)
- 快照与备份 (增量备份至 S3/NFS)
- 跨可用区调度
- 卷扩容与迁移
- 直观的 Web UI 管理
- 自动故障恢复
```

### 3.2 架构

```
Volume (PVC)
├── Longhorn Engine (Controller) ──► iSCSI/NVMe-oF 目标
└── Longhorn Replica ──► 数据副本 (在每个节点上)
    ├── Replica 1 (Node A)
    ├── Replica 2 (Node B)
    └── Replica 3 (Node C)
```

### 3.3 v1.8 更新要点

- 改进的备份性能
- 增强的 UI 与监控集成
- 更好的大卷支持

**GitHub**: https://github.com/longhorn/longhorn
**文档**: https://longhorn.io/docs/

---

## 四、CubeFS (CNCF Graduated)

### 4.1 云原生分布式文件系统

> **2026.01 新晋 CNCF Graduated 项目**

```yaml
# 核心特性
- 元数据与数据分离架构
- 多租户与配额管理
- POSIX 兼容文件接口
- S3 兼容对象接口
- Hadoop 兼容 (HDFS 协议)
- 纠删码 (Erasure Code) 与副本双模式
- 适合大数据、AI 训练、内容存储
```

### 4.2 架构组件

| 组件 | 作用 |
|:---|:---|
| Master | 元数据管理、资源调度 |
| MetaNode | 文件系统元数据存储 |
| DataNode | 数据块存储 |
| ObjectNode | S3 协议网关 |
| Client | 挂载客户端 (Fuse / CSI) |

**GitHub**: https://github.com/cubefs/cubefs
**文档**: https://cubefs.io/docs/

---

## 五、其他云原生存储

### 5.1 OpenEBS

| 引擎 | 类型 | 特点 |
|:---|:---|:---|
| Mayastor | 块存储 | NVMe-oF, 高性能, 适合数据库 |
| cStor | 块存储 | 基于 ZFS, 快照/克隆 |
| Jiva | 块存储 | 轻量级, 基于 Longhorn 早期版本 |
| LocalPV | 本地卷 | 节点本地存储, 高性能 |

### 5.2 JuiceFS

- 云原生 POSIX 文件系统
- 数据存对象存储 (S3/OSS), 元数据存 Redis/PostgreSQL/MySQL/TiKV
- 与 Fluid 集成作为数据集缓存层
- 强一致性、快照、回收站

### 5.3 MinIO

- 高性能对象存储 (S3 兼容)
- 单节点到分布式模式
- 纠删码、对象锁定、版本控制
- 与 K8s 通过 CSI/S3 API 集成

---

## 六、存储类别与 CSI

### 6.1 CSI (Container Storage Interface)

- K8s 存储的标准接口
- 任何存储后端均可通过 CSI Driver 接入
- 支持动态卷供应、快照、扩容、克隆

### 6.2 常见 CSI Driver

| Driver | 后端 | 特点 |
|:---|:---|:---|
| Rook-Ceph | Ceph | 统一存储，RWO/RWX/RWM |
| Longhorn | Longhorn | 易用块存储，RWO |
| OpenEBS-cStor/Mayastor | OpenEBS | 多种引擎选择 |
| AWS EBS CSI | AWS EBS | 托管块存储 |
| Azure Disk CSI | Azure Disk | 托管块存储 |
| GCP PD CSI | GCE PD | 托管块存储 |
| NFS CSI | NFS | 通用文件存储 |
| JuiceFS CSI | JuiceFS | 云原生文件系统 |
| CubeFS CSI | CubeFS | 分布式文件存储 |

---

## 七、版本兼容矩阵

| 组件 | K8s v1.31 | v1.32 | v1.33 | 备注 |
|:---|:---|:---|:---|:---|
| Rook v1.16 | ✅ | ✅ | ✅ | Ceph v19 捆绑 |
| Longhorn v1.8 | ✅ | ✅ | ✅ | iscsiadm 依赖 |
| CubeFS v3.5 | ✅ | ✅ | ✅ | CSI Driver 独立 |
| OpenEBS v4.2 | ✅ | ✅ | ✅ | 引擎选择 |
| JuiceFS v1.2 | ✅ | ✅ | ✅ | 元数据服务兼容 |
| MinIO | ✅ | ✅ | ✅ | S3 API 兼容 |
| CSI Spec v1.11 | ✅ | ✅ | ✅ | 快照/扩容 |

---

## 八、存储选型指南

```
┌─────────────────────────────────────────────────────────────┐
│                    云原生存储选型决策树                        │
└─────────────────────────────────────────────────────────────┘

1. 需要共享文件存储 (RWX)?
   └─ Yes ──► CephFS / CubeFS / JuiceFS / NFS
   └─ No  ──► 继续...

2. 数据库/高性能块存储?
   └─ Yes ──► Longhorn / OpenEBS Mayastor / Ceph RBD
   └─ No  ──► 继续...

3. 对象存储需求 (S3 API)?
   └─ Yes ──► MinIO / Ceph RGW / CubeFS ObjectNode
   └─ No  ──► 继续...

4. 大数据/AI 训练场景?
   └─ Yes ──► CubeFS / JuiceFS + Fluid 缓存
   └─ No  ──► 继续...

5. 易用性优先，中小规模?
   └─ Yes ──► Longhorn (一键安装，Web UI)
   └─ No  ──► 继续...

6. 大规模企业级，统一存储?
   └─ Yes ──► Rook + Ceph 或 CubeFS
   └─ No  ──► 根据具体需求选择

7. 公有云托管?
   └─ Yes ──► 云厂商 CSI (EBS/Azure Disk/GCP PD)
   └─ No  ──► 自托管上述方案
```

---

## 参考链接

- [Rook 官方文档](https://rook.io/docs/rook/latest-release/)
- [Longhorn 官方文档](https://longhorn.io/docs/)
- [CubeFS 官方文档](https://cubefs.io/docs/)
- [OpenEBS 官方文档](https://openebs.io/docs/)
- [JuiceFS 官方文档](https://juicefs.com/docs/community/introduction/)
- [K8s 存储文档](https://kubernetes.io/docs/concepts/storage/)
- [CNCF 存储白皮书](https://github.com/cncf/tag-storage/blob/main/storage-whitepaper.md)

---

## Obsidian 相关文档

- domain-04-storage-data MOC
- [[domain-04-storage-data/README.md|Domain-16: 存储基础]]
- 01 - 存储技术概述
- 02 - 块存储、文件存储、对象存储
- 03 - RAID 与存储冗余
- 04 - 分布式存储系统
- 05 - 企业级存储管理与运维实践
- 06 - 存储性能与 IOPS
