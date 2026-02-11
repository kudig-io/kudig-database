# 17 - StorageClass / VolumeSnapshot YAML 配置参考

> **文档版本**: 2026-02  
> **适用范围**: Kubernetes v1.25 - v1.32  
> **资源类型**: StorageClass, VolumeSnapshot, VolumeSnapshotClass, VolumeSnapshotContent  
> **API 版本**: storage.k8s.io/v1, snapshot.storage.k8s.io/v1  
> **用途**: 动态存储供给与卷快照管理

---

## 一、StorageClass (存储类)

### 1.1 API 资源信息

```yaml
# API 元数据
apiVersion: storage.k8s.io/v1    # StorageClass 在 storage.k8s.io API 组 (v1.6+ GA)
kind: StorageClass               # 资源类型

metadata:
  name: fast-ssd                 # 存储类名称(集群级别唯一)
  labels:                        # 标签用于分类
    performance: high
    cost: premium
  annotations:                   # 注解存储元数据
    storageclass.kubernetes.io/is-default-class: "true"  # 标记为默认存储类
```

**关键特性**:
- **集群级别资源**: StorageClass 不属于任何 namespace
- **动态供给**: 自动创建 PV 以满足 PVC 需求
- **参数化**: 通过 parameters 向 provisioner 传递配置
- **不可变**: 创建后大部分字段不可修改(需删除重建)

### 1.2 核心字段结构

```yaml
provisioner: ebs.csi.aws.com     # 供给器(必填)
parameters:                      # 供给器特定参数(可选)
  type: gp3
  iops: "3000"
reclaimPolicy: Delete            # 回收策略(默认 Delete)
volumeBindingMode: WaitForFirstConsumer  # 绑定模式(默认 Immediate)
allowVolumeExpansion: true       # 允许扩容(默认 false)
allowedTopologies:               # 拓扑约束(可选)
- matchLabelExpressions:
  - key: topology.kubernetes.io/zone
    values: [us-west-2a, us-west-2b]
mountOptions:                    # 挂载选项(可选)
- discard
- noatime
```

---

### 1.3 Provisioner (供给器)

```yaml
provisioner: ebs.csi.aws.com     # CSI 驱动名称(必填)
# 内置供给器(v1.26+ 已废弃,v1.31+ 移除):
# - kubernetes.io/aws-ebs
# - kubernetes.io/azure-disk
# - kubernetes.io/gce-pd
# 
# 现代 CSI 驱动:
# - ebs.csi.aws.com             # AWS EBS
# - disk.csi.azure.com          # Azure Disk
# - pd.csi.storage.gke.io       # Google Persistent Disk
# - diskplugin.csi.alibabacloud.com  # Alibaba Cloud Disk
# - csi.tigera.io               # Calico CSI
# - rook-ceph.rbd.csi.ceph.com  # Ceph RBD
```

**供给器类型**:

| 类型 | 示例 | 使用场景 |
|------|------|----------|
| 云供应商 CSI | ebs.csi.aws.com | 云环境托管集群 |
| 分布式存储 CSI | rook-ceph.rbd.csi.ceph.com | 自建集群,需高可用 |
| 本地存储 CSI | local.csi.k8s.io | 高性能本地盘 |
| 网络存储 CSI | nfs.csi.k8s.io | 共享文件系统 |

### 1.4 Parameters (参数)

```yaml
parameters:                      # 供给器特定参数(键值对)
  type: gp3                      # 参数名和值由 provisioner 定义
  iops: "3000"                   # 注意: 值必须是字符串类型
  encrypted: "true"              # 布尔值也需要引号
  kmsKeyId: "arn:aws:kms:..."    # 复杂值直接传递
  
  # 特殊前缀(v1.26+):
  csi.storage.k8s.io/provisioner-secret-name: csi-secret
  csi.storage.k8s.io/provisioner-secret-namespace: kube-system
  csi.storage.k8s.io/controller-expand-secret-name: csi-secret
  csi.storage.k8s.io/controller-expand-secret-namespace: kube-system
  csi.storage.k8s.io/node-stage-secret-name: csi-secret
  csi.storage.k8s.io/node-stage-secret-namespace: kube-system
  csi.storage.k8s.io/node-publish-secret-name: csi-secret
  csi.storage.k8s.io/node-publish-secret-namespace: kube-system
  csi.storage.k8s.io/fstype: ext4  # 指定文件系统类型
```

**主流云供应商参数**:

#### AWS EBS CSI
```yaml
parameters:
  type: gp3                      # 卷类型: gp2 | gp3 | io1 | io2 | st1 | sc1
  iops: "16000"                  # IOPS (仅 gp3/io1/io2)
  throughput: "1000"             # 吞吐量 MB/s (仅 gp3)
  encrypted: "true"              # 启用加密
  kmsKeyId: "arn:aws:kms:..."    # KMS 密钥 ARN
  fsType: ext4                   # 文件系统: ext4 | xfs
```

#### Azure Disk CSI
```yaml
parameters:
  skuName: Premium_LRS           # SKU: Premium_LRS | StandardSSD_LRS | Standard_LRS | UltraSSD_LRS
  cachingMode: ReadOnly          # 缓存模式: None | ReadOnly | ReadWrite
  diskEncryptionSetID: "/subscriptions/.../diskEncryptionSets/..."  # 加密集 ID
  fsType: ext4                   # 文件系统类型
```

#### Alibaba Cloud Disk CSI
```yaml
parameters:
  type: cloud_essd               # 类型: cloud_efficiency | cloud_ssd | cloud_essd
  performanceLevel: PL1          # ESSD 性能级别: PL0 | PL1 | PL2 | PL3
  encrypted: "true"              # 启用加密
  fsType: ext4                   # 文件系统类型
```

#### Ceph RBD CSI
```yaml
parameters:
  clusterID: rook-ceph           # Ceph 集群 ID
  pool: replicapool              # 存储池名称
  imageFormat: "2"               # RBD 镜像格式
  imageFeatures: layering        # 镜像特性
  csi.storage.k8s.io/provisioner-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/provisioner-secret-namespace: rook-ceph
  csi.storage.k8s.io/controller-expand-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/controller-expand-secret-namespace: rook-ceph
  csi.storage.k8s.io/node-stage-secret-name: rook-csi-rbd-node
  csi.storage.k8s.io/node-stage-secret-namespace: rook-ceph
  csi.storage.k8s.io/fstype: ext4
```

### 1.5 ReclaimPolicy (回收策略)

```yaml
reclaimPolicy: Delete            # 默认值
# 可选值: Delete | Retain
```

**策略对比**:

| 策略 | PVC 删除后行为 | 数据保留 | 适用场景 |
|------|----------------|----------|----------|
| **Delete** | 自动删除 PV 和后端存储 | ❌ 删除 | 开发/测试环境,自动清理 |
| **Retain** | 保留 PV 和后端存储 | ✅ 保留 | 生产环境,防止数据丢失 |

```yaml
# ✅ 生产环境推荐
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: production-storage
provisioner: ebs.csi.aws.com
reclaimPolicy: Retain            # 保护数据
parameters:
  type: gp3
  encrypted: "true"

# ✅ 开发环境推荐
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: dev-storage
provisioner: ebs.csi.aws.com
reclaimPolicy: Delete            # 自动清理
parameters:
  type: gp3
```

### 1.6 VolumeBindingMode (绑定模式)

```yaml
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定(推荐)
# 可选值: Immediate | WaitForFirstConsumer
```

**模式对比**:

| 模式 | 绑定时机 | 优点 | 缺点 | 适用场景 |
|------|----------|------|------|----------|
| **Immediate** | PVC 创建立即绑定/供给 | 快速可用 | 可能拓扑不匹配 | 单可用区,无拓扑约束 |
| **WaitForFirstConsumer** | Pod 调度后绑定/供给 | 拓扑感知,优化调度 | PVC 长时间 Pending | 多可用区,Local PV |

**工作流程对比**:

```
========== Immediate 模式 ==========
PVC 创建
    ↓
立即触发动态供给
    ↓
在随机可用区创建 PV (如 us-west-2a)
    ↓
PVC 立即 Bound
    ↓
Pod 创建,调度到 us-west-2b
    ↓
❌ 跨可用区挂载失败!

========== WaitForFirstConsumer 模式 ==========
PVC 创建
    ↓
PVC 保持 Pending (不触发供给)
    ↓
Pod 创建,调度器评估节点
    ↓
选择最优节点 (如 us-west-2a 的 node-01)
    ↓
通知 PVC Controller: 节点在 us-west-2a
    ↓
在 us-west-2a 创建 PV
    ↓
PVC Bound
    ↓
✅ Pod 在同可用区成功挂载!
```

**最佳实践**:
```yaml
# ✅ 推荐: 云环境使用 WaitForFirstConsumer
volumeBindingMode: WaitForFirstConsumer
allowedTopologies:
- matchLabelExpressions:
  - key: topology.kubernetes.io/zone
    values: [us-west-2a, us-west-2b, us-west-2c]

# ⚠️ 仅在确定无拓扑约束时使用 Immediate
volumeBindingMode: Immediate
```

### 1.7 AllowVolumeExpansion (允许扩容)

```yaml
allowVolumeExpansion: true       # 允许 PVC 扩容(默认 false)
```

**前提条件**:
- CSI 驱动支持 `EXPAND_VOLUME` 能力
- PV 已绑定到 PVC

**使用示例**:
```yaml
# StorageClass 配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: expandable-storage
provisioner: ebs.csi.aws.com
allowVolumeExpansion: true       # 启用扩容
parameters:
  type: gp3

---
# PVC 扩容操作
# 步骤 1: 创建 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-pvc
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: expandable-storage
  resources:
    requests:
      storage: 100Gi

# 步骤 2: 修改 PVC 容量 (100Gi → 200Gi)
kubectl patch pvc data-pvc -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 步骤 3: 验证扩容
kubectl get pvc data-pvc
# CAPACITY 应更新为 200Gi
```

**注意事项**:
- ❌ 不支持缩容(容量只能增大)
- ⚠️ 某些文件系统需要 Pod 重启才能识别新容量
- ✅ 扩容期间 PVC 仍可正常使用(在线扩容)

### 1.8 AllowedTopologies (拓扑约束)

```yaml
allowedTopologies:               # 限制 PV 创建的拓扑域(可选)
- matchLabelExpressions:
  - key: topology.kubernetes.io/zone  # 标准拓扑标签
    values:
    - us-west-2a
    - us-west-2b
  - key: topology.kubernetes.io/region
    values:
    - us-west-2
```

**常用拓扑标签**:

| 标签 | 含义 | 示例值 |
|------|------|--------|
| topology.kubernetes.io/zone | 可用区 | us-west-2a, cn-hangzhou-a |
| topology.kubernetes.io/region | 区域 | us-west-2, cn-hangzhou |
| kubernetes.io/hostname | 节点主机名 | node-01, worker-03 |
| node.kubernetes.io/instance-type | 实例类型 | m5.large, ecs.g6.large |

**使用场景**:

#### 场景 1: 限定可用区
```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: zone-a-storage
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer
allowedTopologies:
- matchLabelExpressions:
  - key: topology.kubernetes.io/zone
    values:
    - us-west-2a                 # 仅在 2a 可用区创建 PV
parameters:
  type: gp3
```

#### 场景 2: Local PV 节点约束
```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-storage
provisioner: kubernetes.io/no-provisioner  # Local PV 无自动供给
volumeBindingMode: WaitForFirstConsumer
allowedTopologies:
- matchLabelExpressions:
  - key: kubernetes.io/hostname
    values:
    - node-01                    # 仅匹配 node-01 的 Local PV
    - node-02
    - node-03
```

### 1.9 MountOptions (挂载选项)

```yaml
mountOptions:                    # 传递给 mount 命令的选项(可选)
- discard                        # SSD Trim
- noatime                        # 不更新访问时间
- nodiratime                     # 目录也不更新访问时间
```

**注意事项**:
- ⚠️ **CSI 驱动兼容性**: 部分 CSI 驱动不支持 mountOptions
- ⚠️ **优先级**: PV.spec.mountOptions > StorageClass.mountOptions
- ⚠️ **验证**: 创建 PVC 后检查节点挂载: `mount | grep pvc-xxx`

**常用选项**:
```yaml
# NFS 推荐选项
mountOptions:
- hard
- nfsvers=4.1
- rsize=1048576
- wsize=1048576
- noresvport

# 本地盘性能优化
mountOptions:
- discard                        # SSD Trim
- noatime                        # 性能优化
- nodiratime
- nobarrier                      # 有电池备份 RAID 卡时

# 只读挂载
mountOptions:
- ro                             # 只读
- noatime
```

---

### 1.10 配置模板

#### 模板 1: AWS EBS 生产级配置
```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-gp3-encrypted
  labels:
    performance: high
    environment: production
  annotations:
    description: "Encrypted GP3 storage for production workloads"
provisioner: ebs.csi.aws.com
reclaimPolicy: Retain            # 生产必须 Retain
volumeBindingMode: WaitForFirstConsumer  # 拓扑感知
allowVolumeExpansion: true       # 允许扩容
parameters:
  type: gp3
  iops: "16000"                  # 高性能 IOPS
  throughput: "1000"             # 1000 MB/s
  encrypted: "true"              # 启用加密
  kmsKeyId: "arn:aws:kms:us-west-2:123456789012:key/abc-123"  # KMS 密钥
  fsType: ext4
allowedTopologies:
- matchLabelExpressions:
  - key: topology.kubernetes.io/zone
    values:
    - us-west-2a
    - us-west-2b
    - us-west-2c
```

#### 模板 2: Azure Disk 高性能配置
```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: azure-premium-ssd
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"  # 设为默认
provisioner: disk.csi.azure.com
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  skuName: Premium_LRS           # 高级 SSD
  cachingMode: ReadOnly          # 读缓存
  fsType: ext4
```

#### 模板 3: Ceph RBD 存储类
```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ceph-rbd
provisioner: rook-ceph.rbd.csi.ceph.com
reclaimPolicy: Delete
volumeBindingMode: Immediate
allowVolumeExpansion: true
parameters:
  clusterID: rook-ceph
  pool: replicapool              # 3 副本池
  imageFormat: "2"
  imageFeatures: layering
  csi.storage.k8s.io/provisioner-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/provisioner-secret-namespace: rook-ceph
  csi.storage.k8s.io/controller-expand-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/controller-expand-secret-namespace: rook-ceph
  csi.storage.k8s.io/node-stage-secret-name: rook-csi-rbd-node
  csi.storage.k8s.io/node-stage-secret-namespace: rook-ceph
  csi.storage.k8s.io/fstype: ext4
```

#### 模板 4: Local PV 存储类
```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-nvme
provisioner: kubernetes.io/no-provisioner  # Local PV 无自动供给
reclaimPolicy: Retain            # 数据保留
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定(必须)
```

#### 模板 5: NFS 存储类
```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-shared
provisioner: nfs.csi.k8s.io      # NFS CSI 驱动
reclaimPolicy: Delete
volumeBindingMode: Immediate
parameters:
  server: nfs.example.com        # NFS 服务器地址
  share: /exports/k8s            # 导出路径
mountOptions:
- hard
- nfsvers=4.1
- rsize=1048576
- wsize=1048576
- noresvport
```

---

## 二、VolumeSnapshot (卷快照)

### 2.1 API 资源信息

```yaml
# API 元数据
apiVersion: snapshot.storage.k8s.io/v1  # VolumeSnapshot API (v1.20+ GA)
kind: VolumeSnapshot              # 资源类型

metadata:
  name: mysql-snapshot-20260210   # 快照名称(命名空间内唯一)
  namespace: database             # 命名空间(必须与源 PVC 相同)
  labels:                         # 标签
    app: mysql
    snapshot-type: manual
  annotations:                    # 注解
    description: "Daily backup snapshot"
```

**关键特性**:
- **命名空间资源**: VolumeSnapshot 属于特定 namespace
- **时间点快照**: 捕获 PVC 的特定时刻状态
- **恢复数据源**: 可作为新 PVC 的 dataSource
- **依赖 CSI**: 需要 CSI 驱动支持 CREATE_DELETE_SNAPSHOT 能力

### 2.2 核心字段结构

```yaml
spec:
  volumeSnapshotClassName: csi-snapclass  # 快照类名称(可选)
  source:                        # 快照来源(二选一)
    persistentVolumeClaimName: mysql-pvc  # 从 PVC 创建快照
    # volumeSnapshotContentName: snapcontent-001  # 从已有内容导入

status:
  readyToUse: true               # 快照是否就绪
  creationTime: "2026-02-10T02:00:00Z"  # 创建时间
  restoreSize: 100Gi             # 恢复大小
  boundVolumeSnapshotContentName: snapcontent-abc123  # 绑定的内容对象
  error:                         # 错误信息(如果失败)
    time: "2026-02-10T02:05:00Z"
    message: "snapshot creation failed"
```

### 2.3 从 PVC 创建快照

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-snapshot-manual
  namespace: database
  labels:
    app: mysql
    backup-type: manual
spec:
  volumeSnapshotClassName: csi-snapclass  # 指定快照类
  source:
    persistentVolumeClaimName: mysql-pvc  # 源 PVC 名称
```

**前提条件**:
1. CSI 驱动支持快照功能
2. VolumeSnapshotClass 已创建
3. 源 PVC 状态为 Bound

**创建流程**:
```
用户创建 VolumeSnapshot 对象
        ↓
Snapshot Controller 检测到新快照
        ↓
检查 VolumeSnapshotClass "csi-snapclass"
        ↓
调用 CSI External Snapshotter
        ↓
External Snapshotter 调用 CSI Driver 的 CreateSnapshot RPC
        ↓
CSI Driver 调用后端存储 API (如 AWS CreateSnapshot)
        ↓
返回 snapshotHandle (快照 ID)
        ↓
External Snapshotter 创建 VolumeSnapshotContent 对象
        ↓
VolumeSnapshot 绑定到 VolumeSnapshotContent
        ↓
status.readyToUse 设为 true
```

### 2.4 从快照内容导入

```yaml
# 场景: 导入已存在的云快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: imported-snapshot
  namespace: database
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    volumeSnapshotContentName: pre-existing-snapcontent  # 引用已有内容
```

### 2.5 快照恢复到 PVC

```yaml
# 从快照创建新 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc-restored
  namespace: database
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ebs-gp3       # 必须与源 PVC 的 StorageClass 兼容
  dataSource:
    kind: VolumeSnapshot
    name: mysql-snapshot-manual   # 快照名称
    apiGroup: snapshot.storage.k8s.io
  resources:
    requests:
      storage: 150Gi              # 可以 >= 快照大小(扩容恢复)
```

---

## 三、VolumeSnapshotClass (快照类)

### 3.1 API 资源信息

```yaml
# API 元数据
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass        # 资源类型

metadata:
  name: csi-snapclass            # 快照类名称(集群级别唯一)
  labels:
    retention: daily
  annotations:
    snapshot.storage.kubernetes.io/is-default-class: "true"  # 标记为默认
```

**关键特性**:
- **集群级别资源**: VolumeSnapshotClass 不属于任何 namespace
- **定义快照策略**: 配置快照驱动、删除策略等
- **不可变**: 创建后字段不可修改

### 3.2 核心字段结构

```yaml
driver: ebs.csi.aws.com          # CSI 驱动名称(必填)
deletionPolicy: Delete           # 删除策略(必填)
parameters:                      # 驱动特定参数(可选)
  snapshotType: standard         # 示例参数
```

### 3.3 Driver (驱动)

```yaml
driver: ebs.csi.aws.com          # 必须与 StorageClass 的 provisioner 匹配
```

**常见驱动**:
- AWS EBS: `ebs.csi.aws.com`
- Azure Disk: `disk.csi.azure.com`
- GCE PD: `pd.csi.storage.gke.io`
- Ceph RBD: `rook-ceph.rbd.csi.ceph.com`
- Alibaba Cloud: `diskplugin.csi.alibabacloud.com`

### 3.4 DeletionPolicy (删除策略)

```yaml
deletionPolicy: Delete           # VolumeSnapshot 删除后的行为(必填)
# 可选值: Delete | Retain
```

**策略对比**:

| 策略 | VolumeSnapshot 删除后 | 后端快照 | 适用场景 |
|------|----------------------|----------|----------|
| **Delete** | VolumeSnapshotContent 也删除 | 后端快照删除 | 临时备份,自动清理 |
| **Retain** | VolumeSnapshotContent 保留 | 后端快照保留 | 长期归档,合规要求 |

```yaml
# ✅ 临时快照(如测试用)
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: snapclass-delete
driver: ebs.csi.aws.com
deletionPolicy: Delete           # 自动清理

# ✅ 生产备份快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: snapclass-retain
driver: ebs.csi.aws.com
deletionPolicy: Retain           # 保留快照
```

### 3.5 Parameters (参数)

```yaml
parameters:                      # CSI 驱动特定参数
  key1: value1
  key2: value2
```

**示例: AWS EBS 快照参数**
```yaml
parameters:
  # AWS EBS CSI 驱动参数较少,大多数配置在 StorageClass
  tagSpecification_1: "Name=k8s-snapshot"
  tagSpecification_2: "Environment=production"
```

**示例: Ceph RBD 快照参数**
```yaml
parameters:
  clusterID: rook-ceph
  csi.storage.k8s.io/snapshotter-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/snapshotter-secret-namespace: rook-ceph
```

### 3.6 配置模板

#### 模板 1: AWS EBS 快照类
```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: ebs-snapclass
  annotations:
    snapshot.storage.kubernetes.io/is-default-class: "true"  # 设为默认
driver: ebs.csi.aws.com
deletionPolicy: Delete           # 自动清理旧快照
parameters:
  tagSpecification_1: "Name=k8s-snapshot"
  tagSpecification_2: "ManagedBy=kubernetes"
```

#### 模板 2: Azure Disk 快照类
```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: azure-snapclass
driver: disk.csi.azure.com
deletionPolicy: Delete
parameters:
  incremental: "true"            # 增量快照(节省空间)
```

#### 模板 3: Ceph RBD 快照类
```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: ceph-snapclass
driver: rook-ceph.rbd.csi.ceph.com
deletionPolicy: Delete
parameters:
  clusterID: rook-ceph
  csi.storage.k8s.io/snapshotter-secret-name: rook-csi-rbd-provisioner
  csi.storage.k8s.io/snapshotter-secret-namespace: rook-ceph
```

---

## 四、VolumeSnapshotContent (快照内容)

### 4.1 API 资源信息

```yaml
# API 元数据
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotContent      # 资源类型

metadata:
  name: snapcontent-abc123       # 内容名称(集群级别唯一,通常自动生成)
  labels:
    app: mysql
```

**关键特性**:
- **集群级别资源**: VolumeSnapshotContent 不属于任何 namespace
- **后端快照映射**: 表示存储后端的实际快照
- **自动创建**: 通常由 External Snapshotter 自动创建(动态供给)
- **静态导入**: 也可手动创建以导入已有快照

### 4.2 核心字段结构

```yaml
spec:
  volumeSnapshotRef:             # 绑定的 VolumeSnapshot 引用
    name: mysql-snapshot-manual
    namespace: database
  source:                        # 快照来源(二选一)
    volumeHandle: vol-0abc123    # 从卷创建(动态)
    # snapshotHandle: snap-xyz789  # 导入已有快照(静态)
  driver: ebs.csi.aws.com        # CSI 驱动名称
  deletionPolicy: Delete         # 删除策略
  volumeSnapshotClassName: csi-snapclass  # 快照类名称

status:
  snapshotHandle: snap-0def456   # 后端快照 ID
  creationTime: 1707537600000000000  # 创建时间(纳秒)
  restoreSize: 107374182400      # 恢复大小(字节)
  readyToUse: true               # 快照是否就绪
  error:                         # 错误信息
    time: 1707537700000000000
    message: "snapshot failed"
```

### 4.3 动态创建 (自动)

```yaml
# 用户创建 VolumeSnapshot 后,External Snapshotter 自动创建 VolumeSnapshotContent
# 无需手动操作

# 示例: 查看自动创建的内容
kubectl get volumesnapshotcontent
# NAME                    READYTOUSE   RESTORESIZE   DELETIONPOLICY   DRIVER              AGE
# snapcontent-abc123      true         107374182400  Delete           ebs.csi.aws.com     5m

kubectl get volumesnapshotcontent snapcontent-abc123 -o yaml
# spec:
#   volumeSnapshotRef:
#     name: mysql-snapshot-manual
#     namespace: database
#   source:
#     volumeHandle: vol-0abc123
#   driver: ebs.csi.aws.com
#   deletionPolicy: Delete
# status:
#   snapshotHandle: snap-0def456   # 后端快照 ID
#   readyToUse: true
```

### 4.4 静态导入 (手动)

```yaml
# 场景: 导入已存在的云快照 (如从控制台手动创建的快照)

# 步骤 1: 创建 VolumeSnapshotContent (引用已有快照)
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotContent
metadata:
  name: imported-snapcontent
spec:
  volumeSnapshotRef:             # 将要绑定的 VolumeSnapshot
    name: imported-snapshot
    namespace: database
  source:
    snapshotHandle: snap-0xyz789  # 已存在的云快照 ID
  driver: ebs.csi.aws.com
  deletionPolicy: Retain         # 保留后端快照(推荐)
  volumeSnapshotClassName: csi-snapclass

---
# 步骤 2: 创建 VolumeSnapshot (引用上述内容)
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: imported-snapshot
  namespace: database
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    volumeSnapshotContentName: imported-snapcontent  # 引用已有内容

---
# 步骤 3: 从导入的快照恢复 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: restored-from-import
  namespace: database
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ebs-gp3
  dataSource:
    kind: VolumeSnapshot
    name: imported-snapshot      # 使用导入的快照
    apiGroup: snapshot.storage.k8s.io
  resources:
    requests:
      storage: 100Gi
```

---

## 五、生产案例

### 5.1 案例 1: 定时备份快照

**场景**: 每天凌晨 2 点自动创建 MySQL 数据库快照

```yaml
# 步骤 1: 创建 VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: daily-backup-snapclass
driver: ebs.csi.aws.com
deletionPolicy: Retain           # 保留快照用于长期归档
parameters:
  tagSpecification_1: "Name=daily-backup"
  tagSpecification_2: "Retention=30days"

---
# 步骤 2: 创建 CronJob 定时快照
apiVersion: batch/v1
kind: CronJob
metadata:
  name: mysql-daily-snapshot
  namespace: database
spec:
  schedule: "0 2 * * *"           # 每天凌晨 2 点
  successfulJobsHistoryLimit: 7  # 保留 7 天历史
  failedJobsHistoryLimit: 3
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-creator  # 需要 RBAC 权限
          containers:
          - name: create-snapshot
            image: bitnami/kubectl:latest
            command:
            - /bin/bash
            - -c
            - |
              set -e
              DATE=$(date +%Y%m%d-%H%M%S)
              SNAPSHOT_NAME="mysql-backup-${DATE}"
              
              # 创建快照
              cat <<EOF | kubectl apply -f -
              apiVersion: snapshot.storage.k8s.io/v1
              kind: VolumeSnapshot
              metadata:
                name: ${SNAPSHOT_NAME}
                namespace: database
                labels:
                  app: mysql
                  backup-type: daily
                  backup-date: "$(date +%Y%m%d)"
              spec:
                volumeSnapshotClassName: daily-backup-snapclass
                source:
                  persistentVolumeClaimName: mysql-pvc
              EOF
              
              # 等待快照就绪
              echo "等待快照 ${SNAPSHOT_NAME} 就绪..."
              kubectl wait --for=condition=ReadyToUse volumesnapshot/${SNAPSHOT_NAME} -n database --timeout=600s
              
              echo "✅ 快照 ${SNAPSHOT_NAME} 创建成功"
              
              # 可选: 清理超过 30 天的快照
              echo "清理旧快照..."
              kubectl get volumesnapshot -n database -l backup-type=daily -o json | \
              jq -r --arg date "$(date -d '30 days ago' +%Y%m%d)" \
              '.items[] | select(.metadata.labels["backup-date"] < $date) | .metadata.name' | \
              xargs -r kubectl delete volumesnapshot -n database
              
              echo "✅ 备份任务完成"
          restartPolicy: OnFailure

---
# 步骤 3: 创建 RBAC 权限
apiVersion: v1
kind: ServiceAccount
metadata:
  name: snapshot-creator
  namespace: database

---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: snapshot-creator
  namespace: database
rules:
- apiGroups: ["snapshot.storage.k8s.io"]
  resources: ["volumesnapshots"]
  verbs: ["get", "list", "create", "delete"]
- apiGroups: [""]
  resources: ["persistentvolumeclaims"]
  verbs: ["get", "list"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: snapshot-creator
  namespace: database
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: Role
  name: snapshot-creator
subjects:
- kind: ServiceAccount
  name: snapshot-creator
  namespace: database
```

### 5.2 案例 2: 蓝绿部署使用快照

**场景**: 升级应用前创建快照,升级失败快速回滚

```yaml
# 步骤 1: 创建快照(升级前)
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-pre-upgrade-v2
  namespace: database
  labels:
    app: mysql
    upgrade-version: v2.0
    snapshot-purpose: pre-upgrade
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: mysql-pvc

---
# 步骤 2: 等待快照就绪
# kubectl wait --for=condition=ReadyToUse volumesnapshot/mysql-pre-upgrade-v2 -n database --timeout=300s

---
# 步骤 3: 升级应用 (修改 StatefulSet 镜像等)
# kubectl set image statefulset/mysql mysql=mysql:8.0.35 -n database

---
# 步骤 4: 如果升级失败,从快照回滚

# 4.1 停止应用
# kubectl scale statefulset mysql -n database --replicas=0

# 4.2 从快照恢复到新 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc-rollback
  namespace: database
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ebs-gp3
  dataSource:
    kind: VolumeSnapshot
    name: mysql-pre-upgrade-v2
    apiGroup: snapshot.storage.k8s.io
  resources:
    requests:
      storage: 200Gi

---
# 4.3 修改 StatefulSet 使用新 PVC (或删除旧 PVC,重命名新 PVC)
# kubectl delete pvc mysql-pvc -n database
# kubectl patch pvc mysql-pvc-rollback -n database -p '{"metadata":{"name":"mysql-pvc"}}'  # 不支持重命名
# 推荐: 修改 StatefulSet 的 volumeClaimTemplates 或直接编辑 Pod

# 4.4 重启应用
# kubectl scale statefulset mysql -n database --replicas=3
```

### 5.3 案例 3: 跨环境数据迁移

**场景**: 从生产环境快照迁移数据到测试环境

```yaml
# 步骤 1: 在生产环境创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: prod-db-snapshot
  namespace: prod
  labels:
    environment: production
    migration: to-dev
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: mysql-pvc

---
# 步骤 2: 导出快照信息 (获取后端快照 ID)
# kubectl get volumesnapshot prod-db-snapshot -n prod -o jsonpath='{.status.boundVolumeSnapshotContentName}' | \
# xargs kubectl get volumesnapshotcontent -o jsonpath='{.status.snapshotHandle}'
# 输出: snap-0xyz789 (AWS EBS 快照 ID)

---
# 步骤 3: 在测试环境导入快照
# (假设生产和测试在同一云账户/区域,快照可见)
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotContent
metadata:
  name: imported-prod-snapshot-content
spec:
  volumeSnapshotRef:
    name: prod-db-snapshot-imported
    namespace: dev
  source:
    snapshotHandle: snap-0xyz789  # 生产快照 ID
  driver: ebs.csi.aws.com
  deletionPolicy: Retain         # 不删除生产快照
  volumeSnapshotClassName: csi-snapclass

---
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: prod-db-snapshot-imported
  namespace: dev
  labels:
    environment: dev
    source: production
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    volumeSnapshotContentName: imported-prod-snapshot-content

---
# 步骤 4: 在测试环境从快照创建 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc-dev
  namespace: dev
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ebs-gp3
  dataSource:
    kind: VolumeSnapshot
    name: prod-db-snapshot-imported
    apiGroup: snapshot.storage.k8s.io
  resources:
    requests:
      storage: 200Gi

---
# 步骤 5: 部署测试应用
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-dev
  namespace: dev
spec:
  serviceName: mysql
  replicas: 1
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        env:
        - name: MYSQL_ROOT_PASSWORD
          value: "dev-password"   # 测试环境密码
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
      volumes:
      - name: data
        persistentVolumeClaim:
          claimName: mysql-pvc-dev
```

---

## 六、故障排查

### 6.1 快照一直 Pending

**症状**:
```bash
kubectl get volumesnapshot mysql-snapshot -n database
# NAME              READYTOUSE   SOURCEPVC     AGE
# mysql-snapshot    false        mysql-pvc     5m
```

**排查步骤**:
```bash
# 1. 查看详细信息
kubectl describe volumesnapshot mysql-snapshot -n database
# Events:
#   Type     Reason               Age   Message
#   ----     ------               ----  -------
#   Warning  CreateSnapshotFailed 3m    Failed to create snapshot: PVC must be Bound

# 2. 确认源 PVC 状态
kubectl get pvc mysql-pvc -n database
# STATUS 必须是 Bound

# 3. 检查 VolumeSnapshotClass
kubectl get volumesnapshotclass csi-snapclass -o yaml

# 4. 检查 CSI Driver 是否支持快照
kubectl get csidrivers ebs.csi.aws.com -o yaml | grep -A 5 volumeSnapshotDataSource
# - CREATE_DELETE_SNAPSHOT 必须存在

# 5. 检查 External Snapshotter 日志
kubectl logs -n kube-system -l app=snapshot-controller
kubectl logs -n kube-system -l app=csi-snapshotter
```

**常见原因**:

| 错误信息 | 原因 | 解决方案 |
|----------|------|----------|
| PVC must be Bound | 源 PVC 未绑定 | 等待 PVC 绑定或检查 PV |
| VolumeSnapshotClass not found | 快照类不存在 | 创建对应的 VolumeSnapshotClass |
| driver does not support snapshots | CSI 驱动不支持快照 | 升级驱动或使用其他备份方式 |
| quota exceeded | 云账户快照配额不足 | 删除旧快照或增加配额 |

### 6.2 从快照恢复失败

**症状**:
```bash
kubectl describe pvc restored-pvc -n database
# Events:
#   Warning  ProvisioningFailed  1m  Failed to provision volume: snapshot is not ReadyToUse
```

**排查清单**:
```bash
# 1. 确认快照状态
kubectl get volumesnapshot mysql-snapshot -n database -o jsonpath='{.status.readyToUse}'
# 必须返回 true

# 2. 确认 StorageClass 兼容
# 源 PVC 的 StorageClass 与新 PVC 的 StorageClass 必须使用相同 driver
kubectl get pvc mysql-pvc -n database -o jsonpath='{.spec.storageClassName}'
kubectl get storageclass <class-name> -o jsonpath='{.provisioner}'

# 3. 检查快照大小
kubectl get volumesnapshot mysql-snapshot -n database -o jsonpath='{.status.restoreSize}'
# 新 PVC 的 requests.storage 必须 >= restoreSize

# 4. 检查 CSI Driver 日志
kubectl logs -n kube-system -l app=ebs-csi-controller -c csi-provisioner
```

### 6.3 快照无法删除

**症状**:
```bash
kubectl delete volumesnapshot mysql-snapshot -n database
# volumesnapshot "mysql-snapshot" deleted
# (命令挂起,快照不消失)

kubectl get volumesnapshot -n database
# NAME              READYTOUSE   SOURCEPVC     AGE
# mysql-snapshot    true         mysql-pvc     10d  (状态 Terminating)
```

**原因**: 快照有 finalizers 保护

**排查步骤**:
```bash
# 1. 检查 finalizers
kubectl get volumesnapshot mysql-snapshot -n database -o jsonpath='{.metadata.finalizers}'
# ["snapshot.storage.kubernetes.io/volumesnapshot-as-source-protection"]

# 2. 检查是否有 PVC 正在从该快照恢复
kubectl get pvc --all-namespaces -o json | \
  jq -r '.items[] | select(.spec.dataSource.name=="mysql-snapshot") | .metadata.namespace + "/" + .metadata.name'

# 3. 删除使用该快照的 PVC
kubectl delete pvc <pvc-name> -n <namespace>

# 4. 如果确认无引用,强制移除 finalizer (危险操作)
kubectl patch volumesnapshot mysql-snapshot -n database -p '{"metadata":{"finalizers":null}}'

# 5. 检查 VolumeSnapshotContent 状态
kubectl get volumesnapshotcontent
# 如果 deletionPolicy=Retain, 内容会保留,需手动删除
kubectl delete volumesnapshotcontent <content-name>
```

---

## 七、最佳实践总结

### 7.1 StorageClass 最佳实践

```yaml
# ✅ 显式命名,语义清晰
metadata:
  name: prod-db-encrypted-ssd    # 而非 "sc-001"

# ✅ 生产环境使用 Retain
reclaimPolicy: Retain

# ✅ 启用拓扑感知
volumeBindingMode: WaitForFirstConsumer
allowedTopologies:
- matchLabelExpressions:
  - key: topology.kubernetes.io/zone
    values: [us-west-2a, us-west-2b]

# ✅ 允许扩容
allowVolumeExpansion: true

# ✅ 加密敏感数据
parameters:
  encrypted: "true"
  kmsKeyId: "arn:aws:kms:..."
```

### 7.2 VolumeSnapshot 最佳实践

```yaml
# ✅ 定时备份
# 使用 CronJob 自动创建快照

# ✅ 标签标识快照用途
metadata:
  labels:
    backup-type: daily           # daily | weekly | manual
    retention: 30days            # 保留期限
    environment: production      # 环境标识

# ✅ 快照命名包含时间戳
metadata:
  name: mysql-backup-20260210-020000

# ✅ 测试恢复流程
# 定期验证从快照恢复的完整性

# ✅ 清理过期快照
# 自动删除超过保留期的快照
```

### 7.3 监控告警

```yaml
# Prometheus 告警规则
groups:
- name: storage-alerts
  rules:
  # 快照创建失败
  - alert: VolumeSnapshotFailed
    expr: |
      kube_volumesnapshot_status_ready_to_use == 0
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "快照 {{ $labels.volumesnapshot }} 创建失败"
  
  # 快照超过保留期未清理
  - alert: VolumeSnapshotExpired
    expr: |
      (time() - kube_volumesnapshot_creation_time) > (30 * 24 * 3600)  # 30 天
    labels:
      severity: info
    annotations:
      summary: "快照 {{ $labels.volumesnapshot }} 超过保留期"
  
  # StorageClass 无法供给
  - alert: StorageClassProvisionFailed
    expr: |
      rate(storage_operation_duration_seconds_count{operation_name="provision",status="fail"}[5m]) > 0
    labels:
      severity: critical
    annotations:
      summary: "StorageClass {{ $labels.storage_class }} 供给失败"
```

---

## 八、参考资源

### 8.1 官方文档

- [Storage Classes](https://kubernetes.io/docs/concepts/storage/storage-classes/)
- [Volume Snapshots](https://kubernetes.io/docs/concepts/storage/volume-snapshots/)
- [CSI Volume Cloning](https://kubernetes.io/docs/concepts/storage/volume-pvc-datasource/)

### 8.2 相关 KEP

- [KEP-1495: Volume Health Monitoring](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/1495-volume-health-monitor)
- [KEP-1900: CSI Driver Service Account Token](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/1900-csi-driver-service-account-token)
- [KEP-2589: VolumeSnapshot Ready Time](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/2589-volumesnapshot-ready-time)

### 8.3 版本差异

| 功能 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|-------|-------|-------|
| VolumeSnapshot GA | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 跨命名空间快照数据源 | ❌ | Alpha | Alpha | Alpha | Beta | Beta | Beta | GA |
| 内置云存储供给器 | Deprecated | Deprecated | Deprecated | Deprecated | Deprecated | Deprecated | ❌ | ❌ |
| VolumeAttributesClass | ❌ | ❌ | Alpha | Alpha | Beta | Beta | GA | GA |

---

**文档维护**: 2026-02  
**下一次审阅**: 2026-08 (Kubernetes v1.33 发布后)
