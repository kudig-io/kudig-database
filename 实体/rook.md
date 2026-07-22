---
title: Rook (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- storage
- rook
- kubelet
- prometheus
- grafana
- containerd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Rook 是什么
- 如何 Rook
trigger_keywords:
- Rook
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Rook

> **CNCF 状态**: Graduated | **类别**: Storage | **主要语言**: Go

## 概述

Rook 是一个云原生存储编排器，2018 年成为 CNCF 毕业项目（Graduated）。它将分布式存储系统（如 Ceph、NFS、EdgeFS）转化为自我管理、自我扩展、自我修复的存储服务。Rook 通过 Kubernetes Operator 模式自动化存储集群的部署、配置、扩缩容、升级和灾难恢复，大幅降低分布式存储在 Kubernetes 上运行的运维复杂度。Ceph 是 Rook 最成熟和广泛使用的存储后端。

## 核心特性

- **Ceph 全功能管理**: 通过 CRD 管理 Ceph Cluster、Block (RBD)、File (CephFS)、Object (RGW)
- **声明式运维**: 使用 CRD 声明式配置存储集群，自动调谐到期望状态
- **自动扩缩容**: 自动添加/移除 OSD，动态扩展存储容量
- **数据恢复**: 自动处理磁盘/节点故障，数据重平衡
- **安全加密**: 支持 LUKS 全盘加密和 KMS 集成
- **监控集成**: 内置 Prometheus 指标和 Grafana 仪表盘

## 架构

Rook 的核心是 Ceph Operator，它监听 CephCluster CRD 并管理整个 Ceph 集群的生命周期。架构包含：Rook Operator（主控制器）、Ceph CSI Driver（提供 Kubernetes CSI 接口）、Ceph Mon（监控集群状态）、Ceph OSD（存储数据）、Ceph MDS（CephFS 元数据）、Ceph RGW（S3 对象存储接口）。Operator 自动配置 Mon 仲裁、OSD 放置组和 CRUSH Map，无需手动操作 Ceph 命令行工具。所有组件以 Pod 形式运行，由 Kubernetes 编排。

## Kubernetes 集成

Rook 通过 CSI（Container Storage Interface）与 Kubernetes 深度集成。Ceph CSI 提供三种存储接口：RBD（块存储，支持 RWO/RWX）、CephFS（文件存储，支持 RWX）、RGW（对象存储，S3 API）。Rook Operator 监听 CephCluster CRD，自动部署和管理 Ceph 组件。StorageClass 配置引用 Rook 管理的存储池，实现动态卷置备。支持 Volume Snapshot、PVC Clone 和跨命名空间共享。

## 生产使用场景

1. **统一存储平台**: 一个 Ceph 集群同时提供块存储（数据库）、文件存储（共享目录）和对象存储（备份）
2. **混合云存储**: 在裸金属数据中心使用 Rook/Ceph 替代云厂商托管存储
3. **高性能数据库**: 使用 RBD 块存储为 PostgreSQL、MongoDB 提供低延迟持久卷
4. **S3 兼容对象存储**: 使用 RGW 为应用程序提供 AWS S3 兼容的 API

## 安装

```bash
helm repo add rook-release https://charts.rook.io/release
helm install rook-ceph rook-release/rook-ceph -n rook-ceph --create-namespace
# 创建 Ceph 集群
kubectl apply -f https://raw.githubusercontent.com/rook/rook/master/deploy/examples/cluster.yaml

# 验证集群状态
kubectl get cephcluster -n rook-ceph
kubectl get pods -n rook-ceph | grep -E 'osd|mon|mgr'
```

### StorageClass 配置

```yaml
# CephBlockPool + StorageClass
apiVersion: ceph.rook.io/v1
kind: CephBlockPool
metadata:
  name: replicapool
  namespace: rook-ceph
spec:
  failureDomain: host
  replicated:
    size: 3
---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: rook-ceph-block
provisioner: rook-ceph.rbd.csi.ceph.com
parameters:
  clusterID: rook-ceph
  pool: replicapool
  imageFormat: "2"
  imageFeatures: layering
reclaimPolicy: Delete
allowVolumeExpansion: true
```

## 运维操作

```bash
# 🟢 查看 Ceph 集群健康
kubectl -n rook-ceph exec deploy/rook-ceph-tools -- ceph status
kubectl -n rook-ceph exec deploy/rook-ceph-tools -- ceph df

# 🟢 查看 OSD 状态
kubectl -n rook-ceph exec deploy/rook-ceph-tools -- ceph osd tree
kubectl get cephcluster -n rook-ceph -o yaml | grep -A5 status

# 🟡 扩展 OSD（添加新磁盘）
kubectl apply -f cluster-updated.yaml  # 修改 deviceFilter

# 🟡 故障转移（标记 OSD out）
kubectl -n rook-ceph exec deploy/rook-ceph-tools -- ceph osd out osd.3

# 🔴 删除 CephCluster（数据不可恢复）
kubectl delete cephcluster -n rook-ceph
# 清理节点磁盘
kubectl -n rook-ceph exec deploy/rook-ceph-tools -- ceph osd purge <id> --yes-i-really-mean-it
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Cluster HEALTH_WARN | OSD 近满/时钟偏移 | `ceph status` | 扩容/同步 NTP |
| PVC Pending | StorageClass 未就绪 | `kubectl describe pvc` | 检查 pool 和 provisioner |
| OSD CrashLoopBackOff | 磁盘损坏/权限 | `kubectl logs osd-pod` | 替换磁盘/检查 udev |
| Mon 仲裁丢失 | 多数 Mon 不可用 | `ceph mon stat` | 恢复 Mon 数据 |
| IO 延迟飙升 | OSD 过载/网络 | `ceph osd perf` | 重平衡/检查网络 |

```
排查流程:
├── 集群不健康
│   ├── ceph status → 查看 WARN/ERR 详情
│   ├── ceph health detail → 具体告警
│   └── kubectl get cephcluster -o yaml → Operator 状态
├── PVC 无法绑定
│   ├── kubectl describe pvc → Events
│   ├── kubectl get sc → StorageClass 存在
│   └── kubectl logs -n rook-ceph csi-rbdplugin → CSI 日志
└── OSD 异常
    ├── ceph osd tree → 查看状态
    ├── kubectl logs rook-ceph-osd-<id> → 错误日志
    └── dmesg | grep -i error → 磁盘硬件错误
```

## 生产案例

### 案例 1: 节点故障后数据恢复

- **场景**: 存储节点宕机，3 个 OSD 离线，集群 HEALTH_WARN
- **排查**: `ceph osd tree` 显示 3 个 OSD down；等待 mon_osd_down_out_interval(600s) 后自动 out
- **方案**: 等待 Ceph 自动重平衡；若节点无法恢复，添加新节点+新 OSD；确认 PG 状态恢复 active+clean
- **效果**: 数据零丢失，恢复时间 45min（取决于数据量）

### 案例 2: PVC 扩容导致 IO 暂停

- **场景**: 在线扩容 RBD 卷时应用 IO 暂停 30s
- **排查**: CSI 插件执行 rbd resize 时持有排他锁
- **方案**: 升级 Rook 到 1.14+（支持非阻塞 resize）；设置扩容窗口在低峰期
- **效果**: 扩容 IO 中断从 30s 降低到 <1s

## 替代方案

| 项目 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **Rook/Ceph** | 功能全面（块/文件/对象）、CNCF 毕业 | 资源开销大、学习曲线陡 | 企业级统一存储 |
| Longhorn | 部署简单、UI 友好 | 仅块存储、大规模性能有限 | 小集群/开发环境 |
| OpenEBS | 多引擎、CSI 原生 | 引擎选择复杂、功能分散 | 多场景灵活选择 |
| Portworx | 企业级、高性能 | 商业产品、厂商锁定 | 高性能数据库 |

## 架构定位

在 CNCF 生态中，Rook 属于 **Storage** 类别，是分布式存储编排的标杆项目。它将存储管理能力以 Kubernetes 原生方式（CRD + Operator）交付，是云原生存储领域最成熟的开源方案。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/autoscaling-strategies.md|autoscaling-strategies]]
- [[概念/storage-model.md|storage-model]]

## Related

- [[实体/virtual-kubelet.md|kubelet]]]] — Virtual Kubelet
- [[kudo]] — KUDO
- [[02-containerd-v2-features]] — containerd 2.0 新特性
- [[karmada]] — Karmada
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- rook
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- [[归档/release-notes/storage/rook/RELEASE-NOTES-1.18.md|RELEASE-NOTES-1.18]]
- [[归档/release-notes/storage/rook/RELEASE-NOTES-1.19.md|RELEASE-NOTES-1.19]]
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- [[归档/release-notes/storage/rook/RELEASE-NOTES-1.16.md|RELEASE-NOTES-1.16]]
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- [[归档/release-notes/storage/rook/RELEASE-NOTES-1.12.md|RELEASE-NOTES-1.12]]
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.6
- [[归档/release-notes/storage/rook/RELEASE-NOTES-1.13.md|RELEASE-NOTES-1.13]]
- RELEASE-NOTES-0.7
- [[归档/release-notes/storage/rook/RELEASE-NOTES-1.17.md|RELEASE-NOTES-1.17]]
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-0.4
- [[归档/release-notes/storage/rook/RELEASE-NOTES-1.14.md|RELEASE-NOTES-1.14]]
- RELEASE-NOTES-1.1
- [[归档/release-notes/storage/rook/RELEASE-NOTES-1.15.md|RELEASE-NOTES-1.15]]
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.5
- [[实体/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[概念/storage-tool-evolution.md|存储工具演进]] — Cross-reference
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[实体/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[生态参考/领域索引/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
