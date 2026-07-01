---
title: CSI 驱动
summary: CSI 驱动：CSI（Container Storage Interface）是 Kubernetes 存储子系统的标准插件接口，定义了一套 gRPC
  协议规范，使存储厂商无需修改 Kubernetes 核心代码即可提供存储能力。
category: concepts
tags:
- csi
- storage
- drivers
- k8s
tier: core
created: 2026-05-23
updated: 2026-05-24
last_updated: 2026-05-24
status: active
---



# CSI 驱动

## 1. 概述

CSI（Container Storage Interface）是 Kubernetes 存储子系统的标准插件接口，定义了一套 gRPC 协议规范，使存储厂商无需修改 Kubernetes 核心代码即可提供存储能力。

### 架构组成

```
kubelet ──gRPC──▶ CSI Node Driver（DaemonSet）
                    ├── NodePublishVolume（挂载）
                    ├── NodeUnpublishVolume（卸载）
                    └── NodeGetVolumeStats（指标）

kube-controller-manager
  └── external-attacher ──gRPC──▶ CSI Controller Driver（StatefulSet/Deployment）
                                    ├── CreateVolume / DeleteVolume
                                    ├── ControllerPublishVolume（attach）
                                    ├── CreateSnapshot / DeleteSnapshot
                                    ├── CreateVolumeGroupSnapshot
                                    └── CloneVolume
```

### 核心组件

| 组件 | 职责 | 部署方式 |
|------|------|----------|
| CSI Controller Plugin | 卷的创建、删除、快照、克隆、attach/detach | Deployment / StatefulSet |
| CSI Node Plugin | 卷的 mount/unmount、统计信息采集 | DaemonSet |
| external-provisioner | 监听 PVC，调用 CreateVolume | Sidecar |
| external-attacher | 监听 VolumeAttachment，调用 ControllerPublish | Sidecar |
| external-resizer | 监听 PVC 扩容请求 | Sidecar |
| external-snapshotter | 监听 VolumeSnapshot，调用 CreateSnapshot | Sidecar |
| external-populator | 数据导入填充卷 | Sidecar |
| liveness-probe | 健康检查 | Sidecar |

### 与 in-tree 驱动的关系

Kubernetes 早期将 AWS EBS、GCE PD 等存储驱动直接编译在 kubelet 和 controller-manager 中（in-tree）。CSI 的出现使得存储驱动可以独立于 Kubernetes 版本迭代，in-tree 驱动通过 CSI Migration 机制逐步迁移至 CSI 实现。

> 详见 [[concepts/storage-model.md|storage model]] 了解存储整体架构，[[concepts/pv.md|pv]] 了解 PersistentVolume，[[concepts/storageclass.md|storageclass]] 了解 StorageClass 动态供应。

---

## 2. CSI 迁移状态（In-Tree → CSI）

截至 2025-2026 年，所有主要 in-tree 存储驱动已完成 CSI 迁移并进入 GA 状态：

| In-Tree 驱动 | CSI 驱动 | 迁移 GA 版本 | 当前状态 |
|---------------|----------|-------------|---------|
| `kubernetes.io/aws-ebs` | `ebs.csi.aws.com` | v1.25 | ✅ GA，in-tree 已废弃 |
| `kubernetes.io/gce-pd` | `pd.csi.storage.gke.io` | v1.25 | ✅ GA，in-tree 已废弃 |
| `kubernetes.io/azure-disk` | `disk.csi.azure.com` | v1.26 | ✅ GA |
| `kubernetes.io/azure-file` | `file.csi.azure.com` | v1.26 | ✅ GA |
| `kubernetes.io/vsphere-volume` | `csi.vsphere.vmware.com` | v1.26 | ✅ GA |
| `kubernetes.io/cinder` | `cinder.csi.openstack.org` | v1.26 | ✅ GA |
| `kubernetes.io/portworx-volume` | `pxd.portworx.com` | — | CSI 原生 |
| `kubernetes.io/storageos` | `csi.storageos.com` | — | CSI 原生 |

### 迁移机制

- CSI Migration 通过 feature gate 控制：`CSIMigration{Provider}=true`
- 迁移后，in-tree PVC/PV API 不变，kubelet 透明代理到 CSI 驱动
- 从 Kubernetes 1.31 起，多数迁移 feature gate 已锁定为 `true`，不可关闭
- Kubernetes 1.32+ 逐步移除 in-tree 代码路径

### 迁移注意事项

```
# 检查 CSI Migration 是否已启用
kubectl get csinode <node-name> -o jsonpath='{.spec.drivers[*].name}'

# 验证 PV 实际使用的驱动
kubectl get pv <pv-name> -o jsonpath='{.spec.csi.driver}'
```

---

## 3. CSI 核心能力

### 3.1 Volume Snapshots（GA v1.20）

允许对持久卷创建时间点快照，用于备份和恢复。

```yaml
# VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-snapclass
driver: ebs.csi.aws.com
deletionPolicy: Delete
parameters:
  encrypted: "true"
---
# 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: my-snap
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: my-pvc
```

前置条件：需安装 [external-snapshotter](https://github.com/kubernetes-csi/external-snapshotter) 及 CRD。

### 3.2 Volume Group Snapshots（GA v1.36）

支持对一组相关卷（如数据库多盘）创建一致性快照组。

```yaml
apiVersion: groupsnapshot.storage.k8s.io/v1beta1
kind: VolumeGroupSnapshot
metadata:
  name: db-consistent-snap
spec:
  source:
    selector:
      matchLabels:
        app: postgres
```

- v1.27 引入 Alpha，v1.32 Beta，v1.36 GA
- 需 CSI 驱动实现 `CREATE_DELETE_GET_VOLUME_GROUP_SNAPSHOT` 能力

### 3.3 Volume Cloning（GA v1.17）

通过现有 PVC 克隆创建新卷，无需快照中转，速度更快。

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: cloned-pvc
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: gp3
  dataSource:
    name: source-pvc
    kind: PersistentVolumeClaim
  resources:
    requests:
      storage: 100Gi
```

- 克隆要求源 PVC 与目标 PVC 使用相同的 StorageClass（或驱动支持跨类克隆）
- CSI 驱动需报告 `CLONE_VOLUME` 能力

### 3.4 Volume Populators（GA v1.33）

允许从任意数据源（非快照/非 PVC）填充卷内容，如从 S3、镜像仓库、备份系统导入数据。

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: imported-data
spec:
  dataSourceRef:
    apiGroup: k8s.cni.cncf.io
    kind: VolumeImportSource
    name: s3-import
  resources:
    requests:
      storage: 50Gi
```

- 需安装 any-populator 或第三方 populator（如 volume-data-source-validator）
- v1.24 Alpha，v1.28 Beta，v1.33 GA

### 3.5 VolumeAttributesClass（GA v1.34）

允许在不重建 PVC 的情况下动态修改卷属性（如 IOPS、吞吐量、加密策略）。

```yaml
apiVersion: storage.k8s.io/v1
kind: VolumeAttributesClass
metadata:
  name: high-iops
driverName: ebs.csi.aws.com
parameters:
  iops: "6000"
  throughput: "400"
---
# 应用到 PVC
# kubectl patch pvc my-pvc --type=merge -p '{"spec":{"volumeAttributesClassName":"high-iops"}}'
```

- v1.29 Alpha，v1.31 Beta，v1.34 GA
- 取代了传统 StorageClass 中的静态参数方案，支持运行时调参

---

## 4. CSI 拓扑感知供应

### 概念

当存储资源仅在特定区域/可用区可用时，CSI 拓扑感知确保 Pod 调度到有存储可用的节点上。

### 工作流程

1. CSI Node Driver 在注册时上报节点拓扑标签：`topology.ebs.csi.aws.com/zone=us-east-1a`
2. CSI Controller 创建卷时返回卷的可访问拓扑
3. kube-scheduler 通过 `VolumeBinding` 调度插件过滤不可用节点

### 配置示例

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-topology
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定，感知 Pod 调度拓扑
parameters:
  type: gp3
allowedTopologies:
  - matchLabelExpressions:
      - key: topology.ebs.csi.aws.com/zone
        values: ["us-east-1a", "us-east-1b"]
```

### 关键点

- `WaitForFirstConsumer` 是拓扑感知的核心机制，避免卷创建后 Pod 无法调度
- `Immediate` 模式下 CSI 驱动根据 `allowedTopologies` 选择区域
- 多区域集群务必使用 `WaitForFirstConsumer`

---

## 5. CSI 安全加固

### 5.1 未授权卷模式转换防护（GA v1.30）

防止 PVC 通过 `volumeMode` 字段在 `Filesystem` 与 `Block` 之间非预期转换，避免权限提升。

- v1.28 引入 `PreventVolumeModeConversion` feature gate
- v1.30 GA，VolumeSnapshot 与 PVC 之间的 `volumeMode` 转换默认阻止
- 需要在 VolumeSnapshotClass 中显式设置 `conversionPolicy: None` 才允许转换

### 5.2 PV 删除保护

- 当 PVC 被删除时，PV 的回收策略决定卷的生命周期
- 推荐生产环境使用 `Retain` 策略，避免误删数据
- CSI 驱动的 `deletionPolicy` 控制快照删除行为

```yaml
# StorageClass 回收策略
reclaimPolicy: Retain        # 生产环境推荐
volumeBindingMode: WaitForFirstConsumer
```

### 5.3 SELinux 卷标签改进（GA v1.36）

- 早期版本中，SELinux 重标签（relabeling）在卷挂载时由 kubelet 递归执行，大目录耗时极长
- v1.27 引入 `SELinuxMount` feature gate，由 CSI Node Driver 在 mount 时设置标签，避免递归 relabel
- v1.36 GA，大幅减少 Pod 启动时间（尤其在多 Pod 共享同一 PVC 场景）

```bash
# 检查节点 SELinux 状态
getenforce
# 查看卷标签
ls -Z /var/lib/kubelet/pods/<pod-uid>/volumes/
```

### 5.4 其他安全建议

- 使用 `--feature-gates=CSIDriverSELinuxRelabelPolicy=true` 启用驱动级 SELinux 策略
- 限制 CSI 驱动的 RBAC 权限为最小集
- 对 CSI gRPC socket 设置 Unix 权限 `0600`
- 定期审计 CSI 驱动镜像的 CVE

---

## 6. CSI 驱动列表

### 云厂商驱动

| 驱动 | 仓库 | 说明 |
|------|------|------|
| AWS EBS CSI | `kubernetes-sigs/aws-ebs-csi-driver` | EBS 卷，支持快照/克隆/拓扑 |
| GCE PD CSI | `kubernetes-sigs/gcp-compute-persistent-disk-csi-driver` | GCE 持久盘 |
| Azure Disk CSI | `kubernetes-sigs/azuredisk-csi-driver` | Azure 托管磁盘 |
| Azure File CSI | `kubernetes-sigs/azurefile-csi-driver` | Azure 文件共享 |
| Alibaba Cloud CSI | `kubernetes-sigs/alibaba-cloud-csi-driver` | 阿里云云盘/NAS/OSS |

### 开源存储驱动

| 驱动 | 仓库 | 说明 |
|------|------|------|
| Ceph CSI | `ceph/ceph-csi` | RBD / CephFS |
| Longhorn | `longhorn/longhorn` | Rancher 轻量级分布式块存储 |
| OpenEBS | `openebs/openebs` | 本地 PV / Mayastor / cStor |
| Rook | `rook/rook` | Ceph 编排，配合 ceph-csi |
| TopoLVM | `topolvm/topoLVM` | 基于 LVM 的本地卷 |
| NFS CSI | `kubernetes-csi/csi-driver-nfs` | NFS v3/v4 卷 |

### 商业存储驱动

| 驱动 | 仓库 | 说明 |
|------|------|------|
| Portworx | `libopenstorage/openstorage` | 企业级混合云存储 |
| NetApp Trident | `NetApp/trident` | NetApp ONTAP/SolidFire/E-Series |
| Pure Storage | `purestorage/pure-csi` | FlashArray / FlashBlade |
| vSphere CSI | `kubernetes-sigs/vsphere-csi-driver` | VMware vSAN / VMFS / NFS |

### 驱动选择指南

```
需求评估流程：
├── 云原生环境？→ 优先选择对应云厂商 CSI 驱动
├── 自建集群？
│   ├── 需要分布式块存储 → Ceph CSI / Longhorn / OpenEBS Mayastor
│   ├── 需要共享文件存储 → NFS CSI / CephFS CSI
│   └── 需要本地高性能 → TopoLVM / OpenEBS LocalPV
└── 混合云/企业级 → Portworx / NetApp Trident / Pure Storage
```

---

## 7. 常见问题排查

### 7.1 PVC 停留在 Pending

```bash
# 1. 检查 PVC 事件
kubectl describe pvc <pvc-name>

# 2. 常见原因：
#    - StorageClass 不存在或 provisioner 不匹配
#    - CSI Controller Pod 异常
#    - 存储配额不足
#    - allowedTopologies 限制导致无可用区域

# 3. 检查 CSI Controller 日志
kubectl logs -n kube-system deployment/ebs-csi-controller -c ebs-plugin
```

### 7.2 Pod 挂载卷失败（MountVolume）

```bash
# 1. 检查 CSI Node Driver 是否在目标节点运行
kubectl get pods -n kube-system -l app=ebs-csi-node -o wide

# 2. 检查 Node Driver 日志
kubectl logs -n kube-system ds/ebs-csi-node -c ebs-plugin

# 3. 检查 VolumeAttachment 状态
kubectl get volumeattachment | grep <pv-name>

# 4. 常见原因：
#    - 节点缺少 iscsi/nfs-utils/nvme-cli 等依赖
#    - SELinux / AppArmor 阻止访问
#    - 设备路径冲突（/dev/sdX 命名冲突）
#    - CSIDriver 对象的 attachRequired 配置错误
```

### 7.3 快照创建失败

```bash
# 1. 确认 snapshot-controller 和 CSI snapshotter sidecar 已部署
kubectl get pods -n kube-system | grep snapshot

# 2. 检查 VolumeSnapshot 状态
kubectl get volumesnapshot <name> -o yaml

# 3. 确认 VolumeSnapshotClass 的 driver 与 PVC 对应的 provisioner 一致

# 4. 检查 external-snapshotter 日志
kubectl logs -n kube-system deployment/snapshot-controller
```

### 7.4 卷扩容卡住

```bash
# 1. 检查 PVC conditions
kubectl get pvc <pvc-name> -o yaml | grep -A5 conditions

# 2. 确认 StorageClass 允许扩容（allowVolumeExpansion: true）

# 3. 块存储扩容通常需要文件系统 resize：
#    - ext4: 自动 resize2fs
#    - xfs: 自动 xfs_growfs
#    - 需要 Pod 重启触发 NodeExpandVolume（部分驱动）

# 4. 检查 CSI Controller 日志中的 resizer 侧车
kubectl logs -n kube-system deploy/ebs-csi-controller -c csi-resizer
```

### 7.5 CSI Migration 导致的问题

```bash
# 1. 检查 in-tree PV 是否已迁移
kubectl get pv <pv-name> -o jsonpath='{.spec}'

# 2. 如果 PV 仍使用 in-tree spec 但节点已迁移，
#    确认 kubelet feature gate 已启用

# 3. 降级场景需注意：CSI Migration 启用后创建的 PV
#    在降级版本中可能无法识别
```

---

## 8. 最佳实践

### 部署与运维

- **始终使用 Helm / Operator 部署 CSI 驱动**，避免手动 YAML 带来的版本不一致
- **CSI Controller 使用高可用部署**：`replicas: 2`，通过 leader election 保证单活跃实例
- **CSI Node Driver 使用优先级类**：设置 `priorityClassName: system-node-critical` 确保不被驱逐
- **定期更新 CSI 驱动版本**，跟进安全补丁和新功能

### StorageClass 设计

```yaml
# 生产环境推荐配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gp3-retain
provisioner: ebs.csi.aws.com
reclaimPolicy: Retain              # 生产保留数据
volumeBindingMode: WaitForFirstConsumer  # 拓扑感知
allowVolumeExpansion: true         # 允许在线扩容
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"
```

### 安全

- 使用 `Retain` 回收策略保护生产数据
- 启用卷加密（驱动参数或 KMS 集成）
- 限制 PVC/PV 的 RBAC 权限，避免普通用户直接操作 PV
- 定期审计 CSI 驱动镜像漏洞

### 性能

- 根据 IOPS/吞吐需求选择正确的存储类型和 `volumeAttributesClass`
- 使用 `VolumeAttributesClass` 运行时调参，避免 StorageClass 爆炸
- 对于 I/O 密集型工作负载，考虑本地 NVMe（TopoLVM / OpenEBS LocalPV）
- 使用 Volume Cloning 替代快照+恢复，减少 RTO

### 监控

- 启用 CSI 驱动的 `/metrics` 端点，集成 Prometheus
- 关注关键指标：`csi_operations_seconds`、`csi_operations_errors_total`
- 监控 `kubelet_volume_stats_*` 系列指标获取卷使用率
- 配置告警：卷使用率 >80%、挂载失败、快照过期等

---

## 相关概念

- [[concepts/pv.md|pv]] — PersistentVolume 和 PersistentVolumeClaim
- [[concepts/storageclass.md|storageclass]] — StorageClass 动态供应
- [[concepts/storage-model.md|storage model]] — Kubernetes 存储模型总览
- [[domain-19-landscape-references/98-merged-indexes/index.md|index]] — 存储与数据领域索引

## Related

- [[concepts/cloud-native-storage-systems.md|cloud native storage systems]] — 云原生存储系统架构
- [[concepts/storage-performance-optimization.md|storage performance optimization]] — 存储性能优化策略
- [[concepts/storage-data-protection.md|storage data protection]] — 存储数据保护与灾备
