---
title: 存储知识词典
description: 涵盖 Kubernetes 存储全领域的完整术语体系，包括 PV/PVC、CSI、StorageClass、分布式存储、快照、对象存储等
summary: 存储领域词典，覆盖 CSI、PV/PVC、StorageClass、Ceph、Longhorn、Rook、快照、对象存储等核心概念
category: dictionary
tags:
- dictionary
- storage
- csi
- persistent-volume
- storageclass
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- 平台工程师
- SRE
- 数据库管理员
---

# 存储知识词典（Storage）

> 本词典覆盖 Kubernetes 存储领域的核心术语、技术组件及工程实践，是平台工程师和 SRE 管理持久化存储的权威参考。

## 领域概述

存储是 Kubernetes 有状态工作负载的基石，核心挑战包括：

- **持久化**：Pod 重启后数据不丢失
- **动态供给**：自动创建/删除存储卷
- **存储抽象**：CSI 统一接口，屏蔽底层差异
- **数据保护**：快照、备份、克隆
- **性能**：高 IOPS、低延迟、高吞吐

## 核心术语定义

### 存储抽象层

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| Volume | Pod 内容器共享的存储卷 | 生命周期与 Pod 绑定 |
| PersistentVolume (PV) | 集群级存储资源抽象 | 管理员创建/动态供给 |
| PersistentVolumeClaim (PVC) | 用户对存储的申请 | 类似 Pod 对资源的申请 |
| StorageClass | 存储类型定义，支持动态供给 | provisioner、parameters、reclaimPolicy |
| CSI | Container Storage Interface，存储插件标准 | 统一接口、插件化 |
| VolumeAttributesClass | 卷属性类，运行时修改卷参数 | K8s 1.31+ Beta |

### 卷类型

| 术语 | 定义 | 适用场景 |
|------|------|----------|
| emptyDir | Pod 内临时共享存储 | 缓存、Sidecar 共享 |
| hostPath | 挂载节点文件系统 | 开发测试、DaemonSet |
| ephemeral Volume | 内联临时卷（CSI/ConfigMap/Secret） | 临时数据、配置注入 |
| projected Volume | 多卷合并挂载 | ServiceAccount + ConfigMap |
| Local Ephemeral Storage | 节点本地临时存储 | 日志、缓存 |

### 动态供给与生命周期

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| Dynamic Provisioning | PVC 触发自动创建 PV | StorageClass + provisioner |
| Reclaim Policy | PV 释放后的处理策略 | Retain/Delete/Recycle |
| Volume Cloning | 从现有 PVC 克隆新卷 | CSI 支持、同 StorageClass |
| Volume Snapshot | 卷的时间点副本 | VolumeSnapshotClass、CSI 支持 |
| Volume Expansion | 在线/离线扩容 | allowVolumeExpansion: true |
| Storage Capacity | 存储容量跟踪与调度 | CSIStorageCapacity |

### 分布式存储系统

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Ceph | 统一分布式存储（块/文件/对象） | Rook 部署 |
| Rook | Ceph 的 K8s Operator | 自动化运维 |
| Longhorn | Rancher 轻量级分布式块存储 | 简单易用 |
| OpenEBS | 云原生存储平台，多引擎 | LocalPV/cStor/Mayastor |
| CubeFS | 京东开源分布式文件/对象存储 | 大规模场景 |
| HwameiStor | 华为开源本地存储管理 | 本地卷 HA |
| Piraeus | 基于 LINSTOR/DRBD 的高性能存储 | 低延迟 |

### 对象存储与数据平台

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| MinIO | S3 兼容对象存储 | 私有化部署 |
| Fluid | 数据编排与加速框架 | 数据集缓存、调度 |
| Vineyard | 内存数据管理器（AI 场景） | 零拷贝共享 |
| Composefs | 容器镜像文件系统优化 | 按需加载 |

### 数据库存储

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| CloudNativePG | K8s 原生 PostgreSQL Operator | HA、备份、升级 |
| TiKV | 分布式事务 KV 存储 | TiDB 底层 |
| Vitess | MySQL 水平扩展方案 | YouTube 开源 |
| Oxia | 分布式协调服务（ZooKeeper 替代） | 云原生设计 |

## 技术组件索引

### 存储抽象类

- [[17-系统基础/06-知识字典/storage/volume.md|Volume（存储卷）]]
- [[17-系统基础/06-知识字典/storage/volumes.md|Volumes（卷管理）]]
- [[17-系统基础/06-知识字典/storage/persistent-volume.md|PersistentVolume]]
- [[17-系统基础/06-知识字典/storage/persistent-volumes.md|Persistent Volumes（综合）]]
- [[17-系统基础/06-知识字典/storage/persistent-volume-claim.md|PersistentVolumeClaim]]
- [[17-系统基础/06-知识字典/storage/storage-class.md|StorageClass]]
- [[17-系统基础/06-知识字典/storage/storage-classes.md|Storage Classes（综合）]]
- [[17-系统基础/06-知识字典/storage/csi.md|CSI（容器存储接口）]]
- [[17-系统基础/06-知识字典/storage/dynamic-volume-provisioning.md|动态供给]]
- [[17-系统基础/06-知识字典/storage/storage-capacity.md|存储容量]]
- [[17-系统基础/06-知识字典/storage/volume-attributes-classes.md|VolumeAttributesClass]]
- [[17-系统基础/06-知识字典/storage/node-specific-volume-limits.md|节点卷限制]]

### 卷类型类

- [[17-系统基础/06-知识字典/storage/emptydir.md|emptyDir]]
- [[17-系统基础/06-知识字典/storage/hostpath.md|hostPath]]
- [[17-系统基础/06-知识字典/storage/ephemeral-volumes.md|临时卷]]
- [[17-系统基础/06-知识字典/storage/projected-volumes.md|Projected Volumes]]
- [[17-系统基础/06-知识字典/storage/local-ephemeral-storage.md|本地临时存储]]

### 数据保护类

- [[17-系统基础/06-知识字典/storage/volume-snapshots.md|Volume Snapshots]]
- [[17-系统基础/06-知识字典/storage/volume-snapshot-classes.md|VolumeSnapshotClass]]
- [[17-系统基础/06-知识字典/storage/csi-volume-cloning.md|CSI Volume Cloning]]
- [[17-系统基础/06-知识字典/storage/volume-health-monitoring.md|卷健康监控]]

### 分布式存储类

- [[17-系统基础/06-知识字典/storage/ceph.md|Ceph]]
- [[17-系统基础/06-知识字典/storage/rook.md|Rook]]
- [[17-系统基础/06-知识字典/storage/longhorn.md|Longhorn]]
- [[17-系统基础/06-知识字典/storage/openebs.md|OpenEBS]]
- [[17-系统基础/06-知识字典/storage/cubefs.md|CubeFS]]
- [[17-系统基础/06-知识字典/storage/hwameistor.md|HwameiStor]]
- [[17-系统基础/06-知识字典/storage/piraeus-datastore.md|Piraeus]]

### 对象存储与数据平台类

- [[17-系统基础/06-知识字典/storage/minio.md|MinIO]]
- [[17-系统基础/06-知识字典/storage/fluid.md|Fluid]]
- [[17-系统基础/06-知识字典/storage/vineyard.md|Vineyard]]
- [[17-系统基础/06-知识字典/storage/composefs.md|Composefs]]
- [[17-系统基础/06-知识字典/storage/object-storage-and-data-pipelines.md|对象存储与数据管道]]
- [[17-系统基础/06-知识字典/storage/high-performance-storage-networks.md|高性能存储网络]]

### 数据库存储类

- [[17-系统基础/06-知识字典/storage/cloudnativepg.md|CloudNativePG]]
- [[17-系统基础/06-知识字典/storage/tikv.md|TiKV]]
- [[17-系统基础/06-知识字典/storage/vitess.md|Vitess]]
- [[17-系统基础/06-知识字典/storage/oxia.md|Oxia]]

### 其他

- [[17-系统基础/06-知识字典/storage/windows-storage.md|Windows 存储]]

## 存储架构模式

### CSI 架构

```
CSI 架构:

┌─────────────────────────────────────────┐
│  K8s Control Plane                       │
│  ├── kube-controller-manager            │
│  │   └── PersistentVolume Controller    │
│  └── external-provisioner/attacher      │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Worker Node                             │
│  ├── kubelet                            │
│  │   └── Volume Manager                 │
│  ├── CSI Driver (DaemonSet)             │
│  │   ├── NodeGetInfo                    │
│  │   ├── NodeStageVolume                │
│  │   └── NodePublishVolume              │
│  └── CSI Plugin (gRPC)                  │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Storage Backend                         │
│  ├── Ceph RBD / CephFS                  │
│  ├── Cloud Block Storage (EBS/PD)       │
│  ├── NFS / iSCSI                        │
│  └── Local Disk                         │
└─────────────────────────────────────────┘
```

### 存储类型选择

| 场景 | 推荐方案 | 关键考量 |
|------|----------|----------|
| 数据库 (RWO) | 云块存储 / Ceph RBD | IOPS、延迟 |
| 共享文件 (RWX) | CephFS / NFS / CubeFS | 多 Pod 共享访问 |
| 日志/缓存 | Local SSD / emptyDir | 高吞吐、可丢失 |
| AI 训练数据 | Fluid + 对象存储 | 缓存加速、大数据集 |
| 备份/归档 | 对象存储 (S3/MinIO) | 低成本、持久 |
| 开发测试 | Longhorn / hostPath | 简单、快速 |

## 生产最佳实践

### 存储配置

1. **StorageClass 设计**：按性能等级分层（ssd-fast/ssd-standard/hdd）
2. **Reclaim Policy**：生产用 Retain（防误删），测试用 Delete
3. **卷扩容**：启用 allowVolumeExpansion，支持在线扩容
4. **快照策略**：定期 VolumeSnapshot + 异地备份

### 性能优化

1. **文件系统**：XFS 优于 ext4（大文件、高并发）
2. **挂载选项**：noatime、nodiratime 减少元数据写入
3. **I/O 调度**：SSD 用 none/mq-deadline，HDD 用 bfq
4. **预分配**：数据库预分配空间，避免运行时扩容

### 数据保护

1. **3-2-1 备份**：3 份副本、2 种介质、1 份异地
2. **快照 + 备份**：快照用于快速恢复，备份用于灾难恢复
3. **定期演练**：每季度验证备份可恢复性
4. **加密**：静态加密（etcd/PV）+ 传输加密（TLS）

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| PVC Pending | StorageClass 不存在/配额不足 | `kubectl describe pvc`、检查 SC |
| Pod 挂载失败 | CSI Driver 异常/卷被占用 | 检查 CSI Pod 日志、VolumeAttachment |
| I/O 延迟高 | 存储后端过载/网络问题 | 检查存储集群状态、网络延迟 |
| 扩容失败 | 文件系统不支持/CSI 不支持 | 检查 StorageClass、文件系统类型 |
| 数据丢失 | ReclaimPolicy=Delete/PV 误删 | 检查 PV 状态、从快照恢复 |
| RWX 访问失败 | 存储不支持多节点访问 | 检查 AccessModes、使用 RWX 存储 |

## 学习路径

```
基础: Volume 类型 → PV/PVC → StorageClass
进阶: CSI 架构 → 动态供给 → 快照/克隆
高级: Ceph/Rook 部署 → 性能调优 → 数据保护
专家: 自定义 CSI Driver → 存储编排 (Fluid) → 混部存储
```

## 参考链接

- https://kubernetes.io/docs/concepts/storage/
- https://kubernetes-csi.github.io/docs/
- https://rook.io/
- https://longhorn.io/
- https://openebs.io/
- https://ceph.io/
- https://min.io/

## Related

- [[17-系统基础/06-知识字典/configuration/resource-management-for-pods-and-containers.md|资源管理]]
- [[17-系统基础/06-知识字典/workloads/statefulset.md|StatefulSet 工作负载]]
- [[17-系统基础/06-知识字典/operations/backup-disaster-recovery.md|备份与灾难恢复]]
- [[17-系统基础/06-知识字典/specialized-workloads/hpc-and-bioinformatics.md|HPC 存储]]

## 存储配置示例

### 完整的动态供给配置

```yaml
# StorageClass: SSD 高性能
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ssd-fast
provisioner: kubernetes.io/aws-ebs  # 或 csi driver
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定
mountOptions:
- noatime
- nodiratime
---
# PVC: 数据库存储
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: ssd-fast
  resources:
    requests:
      storage: 100Gi
---
# VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-snapshot
driver: ebs.csi.aws.com
deletionPolicy: Retain
---
# VolumeSnapshot: 每日快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: postgres-snapshot-$(date +%Y%m%d)
spec:
  volumeSnapshotClassName: csi-snapshot
  source:
    persistentVolumeClaimName: postgres-data
```

### StatefulSet + PVC 模板

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
spec:
  serviceName: postgres
  replicas: 3
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: ssd-fast
      resources:
        requests:
          storage: 100Gi
  template:
    spec:
      containers:
      - name: postgres
        image: postgres:16
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
        resources:
          requests:
            cpu: "2"
            memory: 4Gi
          limits:
            memory: 8Gi
```

## 生产案例研究

### 案例：数据库存储故障恢复

**背景：** 某公司 PostgreSQL 主库 PV 损坏，数据不可读。

**恢复过程：**
1. 从最近 VolumeSnapshot 创建新 PVC（2min）
2. 从新 PVC 启动 PostgreSQL Pod（3min）
3. 验证数据完整性（5min）
4. 切换应用连接到新实例（2min）
总恢复时间: 12min（RPO: 快照间隔 1h）

**改进措施：**
- 快照频率从每日改为每小时
- 增加跨 AZ 快照复制
- 定期演练恢复流程

## 常用运维命令速查

```bash
# === PV/PVC 管理 ===
# 查看 PV 状态
kubectl get pv
# 查看 PVC 状态
kubectl get pvc -A
# 查看 PV 详情
kubectl describe pv my-pv
# 查看 PVC 绑定事件
kubectl describe pvc my-pvc | grep -A10 Events

# === StorageClass ===
# 查看 StorageClass
kubectl get sc
# 查看默认 StorageClass
kubectl get sc -o jsonpath='{.items[?(@.metadata.annotations.storageclass\.kubernetes\.io/is-default-class=="true")].metadata.name}'

# === 快照 ===
# 创建快照
kubectl create -f snapshot.yaml
# 查看快照
kubectl get volumesnapshots -A
# 从快照恢复
kubectl create -f pvc-from-snapshot.yaml

# === 扩容 ===
# 扩容 PVC
kubectl patch pvc my-pvc -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'
# 检查扩容状态
kubectl describe pvc my-pvc | grep -i "resizing\|FileSystemResizePending"

# === CSI 诊断 ===
# 查看 CSI Driver Pod
kubectl get pods -n kube-system -l app=csi-driver
# 查看 VolumeAttachment
kubectl get volumeattachments
# 查看 CSI 日志
kubectl logs -n kube-system -l app=csi-driver -c csi-plugin
```

## 常见问题 FAQ

**Q1: PV 和 PVC 是什么关系？**

A: PV 是存储资源（供给），PVC 是存储申请（消费）。类似 Node 和 Pod 的关系：
- PV: 管理员创建或 StorageClass 动态创建
- PVC: 用户创建，声明需要多大、什么类型的存储
- 绑定: PVC 找到匹配的 PV 后绑定
- 释放: PVC 删除后，PV 根据 reclaimPolicy 处理

**Q2: WaitForFirstConsumer 和 Immediate 有什么区别？**

A: 
- Immediate: PVC 创建时立即供给 PV（可能调度到无节点的 AZ）
- WaitForFirstConsumer: 等 Pod 调度后再供给（保证 PV 和 Pod 同 AZ）
生产环境强烈建议 WaitForFirstConsumer，避免跨 AZ 挂载失败。

**Q3: 如何实现在线扩容？**

A: 
1. StorageClass 设置 `allowVolumeExpansion: true`
2. 修改 PVC 的 requests.storage 为更大值
3. CSI Driver 扩容底层卷
4. kubelet 扩容文件系统（可能需要重启 Pod）
注意：只能扩不能缩；XFS/ext4 支持在线扩容。

**Q4: Ceph 和 Longhorn 怎么选？**

A: 
- Ceph (Rook): 功能全面（块/文件/对象），适合大规模生产，但运维复杂
- Longhorn: 简单易用，UI 友好，适合中小规模/开发测试
- 云环境: 优先用云块存储（EBS/PD），无需自运维

**Q5: RWX 存储有哪些选择？**

A: ReadWriteMany（多节点读写）选择：
- CephFS: 功能全面，性能较好
- NFS: 简单，但单点故障风险
- CubeFS: 大规模场景，高性能
- 云文件存储: EFS/Filestore（托管服务）
注意：块存储（RBD/EBS）不支持 RWX。

## 缩略语表

| 缩写 | 全称 | 说明 |
|------|------|------|
| PV | PersistentVolume | 持久卷 |
| PVC | PersistentVolumeClaim | 持久卷申请 |
| CSI | Container Storage Interface | 容器存储接口 |
| SC | StorageClass | 存储类 |
| RWO | ReadWriteOnce | 单节点读写 |
| RWX | ReadWriteMany | 多节点读写 |
| ROX | ReadOnlyMany | 多节点只读 |
| RWOP | ReadWriteOncePod | 单 Pod 读写 (K8s 1.29+) |
| IOPS | Input/Output Operations Per Second | 每秒 I/O 操作数 |
| RPO | Recovery Point Objective | 恢复点目标 |
| RTO | Recovery Time Objective | 恢复时间目标 |

## 版本兼容性矩阵

| 组件 | K8s 1.28 | K8s 1.29 | K8s 1.30 | K8s 1.31 |
|------|-----------|-----------|-----------|----------|
| CSI Spec | v1.8 | v1.9 | v1.10 | v1.11 |
| Rook/Ceph | v1.13+ | v1.14+ | v1.15+ | v1.16+ |
| Longhorn | v1.5+ | v1.6+ | v1.7+ | v1.8+ |
| OpenEBS | v3.10+ | v4.0+ | v4.1+ | v4.2+ |
| VolumeSnapshot | v1 (GA) | v1 | v1 | v1 |
| RWOP | Beta | Beta | GA | GA |
| VolumeAttributesClass | - | Alpha | Alpha | Beta |
| DRA | Alpha | Alpha | Beta | Beta |

## 存储性能基准参考

| 存储类型 | 随机读 IOPS | 顺序吞吐 | 延迟 | 适用场景 |
|----------|-------------|----------|------|----------|
| 本地 NVMe SSD | 500K+ | 3-6 GB/s | <100μs | 数据库、缓存 |
| 云块存储 (gp3) | 16K | 1 GB/s | ~1ms | 通用数据库 |
| Ceph RBD (SSD) | 50-100K | 500MB-1GB/s | 1-5ms | 生产数据库 |
| CephFS | 10-50K | 200-500MB/s | 5-20ms | 共享文件 |
| NFS | 5-20K | 100-500MB/s | 10-50ms | 开发测试 |
| 对象存储 (S3) | N/A | 100MB-10GB/s | 10-100ms | 备份、AI 数据 |

