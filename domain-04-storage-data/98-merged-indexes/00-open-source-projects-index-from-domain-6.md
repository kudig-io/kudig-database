---
title: Domain-6 存储 — 开源项目索引
description: '# Domain-6 存储 — 开源项目索引'
category: storage
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- rook
- ceph
- minio
- mysql
- crd
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 存储工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-6 存储 — 开源项目索引 是什么
- 如何 Domain-6 存储 — 开源项目索引
- Kubernetes 6 storage 最佳实践
trigger_keywords:
- Domain-6
- 存储
- 开源项目索引
- storage
prerequisites:
- kubectl-basics
- storage-basics
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
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-04-storage-data/
  label: '相关知识域: domain-04-storage-data'
created: "2026-05-23"
---

# Domain-6 存储 — 开源项目索引

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-05

---

<!-- chunk: 核心存储项目 -->
## 核心存储项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Rook** | 云原生存储编排 (Ceph/EdgeFS/NFS) | Graduated | v1.16.0 | 12.5k+ | Apache-2.0 |
| **Longhorn** | 分布式块存储 | Incubating | v1.8.0 | 6k+ | Apache-2.0 |
| **CubeFS** | 分布式文件/对象存储 | Graduated | v3.5.0 | 4.5k+ | Apache-2.0 |
| **OpenEBS** | K8s 原生存储引擎 | 非 CNCF | v4.2.0 | 8.5k+ | Apache-2.0 |
| **Vitess** | MySQL 水平扩展 | Graduated | v21.0.0 | 18k+ | Apache-2.0 |
| **TiKV** | 分布式 KV 存储 | Graduated | v8.5.0 | 15k+ | Apache-2.0 |
| **Ceph** | 统一分布式存储 | 不适用 | v19.2 (Reef) | 14k+ | LGPL-2.1 |
| **MinIO** | 高性能对象存储 | 不适用 | v2026.04 | 50k+ | AGPL-3.0 |
| **JuiceFS** | 云原生分布式文件系统 | 不适用 | v1.3.0 | 10k+ | Apache-2.0 |
| **Portworx** | 企业级云原生存储 | 不适用 | v3.2 | 2k+ | 商业 |
| **StorageOS** | 容器原生存储 | 不适用 | v2.10 | 1k+ | 商业 |

### CSI 生态组件（Kubernetes SIG）

| 组件 | 作用 | 最新版本 |
|:---|:---|:---|
| **CSI Spec** | 容器存储接口规范 | v1.11.0 |
| **Snapshot Controller** | 卷快照管理 | v8.2.0 |
| **external-provisioner** | CSI 动态供给辅助 | v5.2.0 |
| **external-attacher** | CSI 挂载辅助 | v4.8.0 |
| **external-resizer** | CSI 卷扩容辅助 | v1.13.0 |
| **external-snapshotter** | CSI 快照辅助 | v8.2.0 |
| **node-driver-registrar** | CSI 节点驱动注册 | v2.13.0 |
| **livenessprobe** | CSI 健康检查 | v2.15.0 |
| **csi-driver-host-path** | 测试用 HostPath CSI | v1.17.0 |

---

<!-- chunk: Rook — 云原生存储编排 -->
## Rook — 云原生存储编排

```yaml
apiVersion: ceph.rook.io/v1
kind: CephCluster
metadata:
  name: rook-ceph
  namespace: rook-ceph
spec:
  cephVersion:
    image: quay.io/ceph/ceph:v19
  dataDirHostPath: /var/lib/rook
  mon:
    count: 3
    allowMultiplePerNode: false
  storage:
    useAllNodes: true
    useAllDevices: true
```

| 特性 | 说明 |
|------|------|
| 存储类型 | 块 (RBD)、文件 (CephFS)、对象 (RGW) |
| 部署方式 | Operator + CRD |
| 后端支持 | Ceph、EdgeFS、NFS、Cassandra |
| K8s 兼容 | v1.22+ |
| 生产就绪 | 是，广泛使用 |

---

<!-- chunk: Longhorn — 分布式块存储 -->
## Longhorn — 分布式块存储

```yaml
apiVersion: longhorn.io/v1beta2
kind: Volume
metadata:
  name: test-volume
  namespace: longhorn-system
spec:
  numberOfReplicas: 3
  size: "10Gi"
  frontend: blockdev
  dataEngine: v1
```

| 特性 | 说明 |
|------|------|
| v1.8 更新 | Data Engine V2 (SPDK)、快照备份增强 |
| 存储类型 | 块存储 (iSCSI) |
| 副本机制 | 同步复制，默认 3 副本 |
| 备份支持 | NFS、S3 兼容后端 |
| K8s 兼容 | v1.25+ |

---

<!-- chunk: CubeFS — 分布式文件/对象存储 -->
## CubeFS — 分布式文件/对象存储

| 特性 | 说明 |
|------|------|
| 存储类型 | 文件 (POSIX)、对象 (S3)、块 (iSCSI) |
| 元数据 | 基于 BTree 的高性能元数据集群 |
| 数据存储 | 副本 / 纠删码 |
| K8s 兼容 | v1.20+ |
| CSI 驱动 | `csi.cubefs.io` |

---

<!-- chunk: OpenEBS — K8s 原生存储引擎 -->
## OpenEBS — K8s 原生存储引擎

| 引擎 | 类型 | 适用场景 |
|------|------|---------|
| **LocalPV** | 本地存储 | 高性能、低延迟 |
| **Mayastor** (v4) | NVMe-oF 分布式块 | 高性能分布式 |
| **Jiva** | iSCSI 分布式块 | 小规模部署 |
| **NFS Provisioner** | NFS 动态供给 | 共享文件 |

---

<!-- chunk: CSI 驱动兼容性矩阵 -->
## CSI 驱动兼容性矩阵

| 云厂商/方案 | CSI 驱动 | K8s 版本 | 块 | 文件 | 对象 |
|------------|---------|---------|:--:|:----:|:----:|
| 阿里云 | `diskplugin.csi.alibabacloud.com` | v1.20+ | ✅ | ✅ (NAS) | ✅ (OSS) |
| AWS | `ebs.csi.aws.com` | v1.23+ | ✅ | ✅ (EFS) | - |
| GCP | `pd.csi.storage.gke.io` | v1.23+ | ✅ | ✅ (Filestore) | - |
| Azure | `disk.csi.azure.com` | v1.21+ | ✅ | ✅ (File) | - |
| VMware | `csi.vsphere.vmware.com` | v1.22+ | ✅ | - | - |
| DigitalOcean | `dobs.csi.digitalocean.com` | v1.21+ | ✅ | - | - |
| Oracle Cloud | `blockvolume.csi.oraclecloud.com` | v1.22+ | ✅ | - | - |
| IBM Cloud | `vpc.block.csi.ibm.io` | v1.23+ | ✅ | - | - |
| Ceph (Rook) | `rbd.csi.ceph.com` / `cephfs.csi.ceph.com` | v1.22+ | ✅ | ✅ | ✅ (RGW) |
| Longhorn | `driver.longhorn.io` | v1.25+ | ✅ | - | - |
| NFS | `nfs.csi.k8s.io` | v1.22+ | - | ✅ | - |
| MinIO | `minio.direct.csi.io` | v1.21+ | - | - | ✅ |
| CubeFS | `csi.cubefs.io` | v1.20+ | ✅ | ✅ | ✅ |

---

<!-- chunk: Kubernetes 版本兼容性 -->
## Kubernetes 版本兼容性

| K8s 版本 | CSI Spec | CSI Migration | 存储变更 |
|---------|----------|---------------|---------|
| v1.31 | v1.11 | GA (全部) | ReadWriteOncePod GA |
| v1.32 | v1.11 | GA (全部) | Volume Group Snapshots Alpha |
| v1.33 | v1.11 | GA (全部) | 跨命名空间卷克隆 Beta |

---

<!-- chunk: 存储选型决策指南 -->
## 存储选型决策指南

```
需要什么类型的存储?
├─ 块存储 (高性能、独占)
│   ├─ 云上? → 云盘 CSI (ESSD/EBS/PD)
│   ├─ 自建? → Ceph RBD / Longhorn
│   └─ 本地? → Local PV + NVMe
├─ 文件存储 (共享、POSIX)
│   ├─ 云上? → NAS / Filestore / Azure File
│   ├─ 自建? → CephFS / CubeFS / NFS
│   └─ 边缘? → JuiceFS
├─ 对象存储 (海量、S3)
│   ├─ 自建? → MinIO / Ceph RGW
│   └─ 云上? → OSS / S3 / GCS / Blob
└─ 数据库存储
    ├─ 关系型 → Vitess (MySQL分片)
    └─ KV型 → TiKV
```

---

<!-- chunk: 参考链接 -->
## 参考链接

- [Kubernetes 存储文档](https://kubernetes.io/docs/concepts/storage/)
- [CSI 规范](https://github.com/container-storage-interface/spec)
- [Rook 文档](https://rook.io/docs/)
- [Longhorn 文档](https://longhorn.io/docs/)
- [CubeFS 文档](https://cubefs.io/docs/)
- [OpenEBS 文档](https://openebs.io/docs/)
- [MinIO 文档](https://min.io/docs/)

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-04-storage-data MOC
- [[domain-04-storage-data/README.md|Storage Domain 存储领域知识库]]
- 存储架构概览与核心组件
- PV/PVC 核心概念与企业级实践
- 03 - PVC使用模式与最佳实践
- StorageClass 动态供给与多租户管理
- 05 - CSI驱动集成与运维管理
- 06 - 存储基础概念详解
- 07 - 存储日常运维操作手册
- 08 - 存储性能调优与优化策略
- 09 - PV/PVC故障排查与解决方案
- 10 - 存储备份与灾难恢复
