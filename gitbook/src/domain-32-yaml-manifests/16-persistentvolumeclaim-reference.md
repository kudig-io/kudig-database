# 16 - PersistentVolumeClaim YAML 配置参考

> **文档版本**: 2026-02  
> **适用范围**: Kubernetes v1.25 - v1.32  
> **资源类型**: PersistentVolumeClaim (PVC)  
> **API 版本**: v1  
> **用途**: 用户声明持久化存储需求

---

## 一、API 资源信息

### 1.1 基本信息

```yaml
# API 元数据
apiVersion: v1                    # PVC 在 v1 API 组中,核心稳定资源
kind: PersistentVolumeClaim       # 资源类型

metadata:
  name: mysql-data-pvc            # PVC 名称(命名空间内唯一)
  namespace: database             # 命名空间(必须与 Pod 相同)
  labels:                         # 标签用于分类
    app: mysql
    tier: database
  annotations:                    # 注解存储元数据
    volume.beta.kubernetes.io/storage-provisioner: "ebs.csi.aws.com"  # 动态供给器标识
```

**关键特性**:
- **命名空间资源**: PVC 属于特定 namespace,必须与使用它的 Pod 在同一 namespace
- **声明式**: 声明需求(容量、访问模式),由 Kubernetes 匹配或动态创建 PV
- **生命周期**: 可独立于 Pod 存在,数据持久化

### 1.2 核心字段结构

```yaml
spec:
  accessModes:                    # 访问模式(必填)
  resources:                      # 资源需求(必填)
  storageClassName:               # 存储类名称(可选)
  volumeMode:                     # 卷模式(可选)
  volumeName:                     # 静态绑定指定 PV(可选)
  selector:                       # 标签选择器(可选)
  dataSource:                     # 数据源(克隆/快照恢复,可选)
  dataSourceRef:                  # 数据源引用(v1.24+,可选)

status:
  phase:                          # 生命周期阶段
  accessModes:                    # 实际绑定的访问模式
  capacity:                       # 实际分配的容量
  allocatedResources:             # 已分配资源(v1.28+)
  resizeStatus:                   # 扩容状态(v1.27+)
  conditions:                     # 状态条件
```

---

## 二、完整字段详解

### 2.1 访问模式 (accessModes)

```yaml
spec:
  accessModes:
    - ReadWriteOnce               # RWO: 单节点读写(最常用)
    # - ReadOnlyMany              # ROX: 多节点只读
    # - ReadWriteMany             # RWX: 多节点读写
    # - ReadWriteOncePod          # RWOP: 单 Pod 读写(v1.29+ GA)
```

**模式说明**:

| 模式 | 缩写 | 使用场景 | 匹配规则 |
|------|------|----------|----------|
| ReadWriteOnce | RWO | 数据库、单实例应用 | PV 必须支持 RWO |
| ReadOnlyMany | ROX | 配置文件分发 | PV 必须支持 ROX |
| ReadWriteMany | RWX | 共享文件系统 | PV 必须支持 RWX |
| ReadWriteOncePod | RWOP | 强一致性要求 | PV 必须支持 RWOP (v1.29+) |

**注意事项**:
```yaml
# ✅ 推荐: 只声明一个模式
accessModes:
  - ReadWriteOnce

# ⚠️ 避免: 同时声明多个模式(除非明确需要)
accessModes:
  - ReadWriteOnce
  - ReadWriteMany
# 原因: 会减少可匹配的 PV 数量(必须 PV 同时支持两种模式)
```

### 2.2 资源需求 (resources)

```yaml
spec:
  resources:
    requests:
      storage: 50Gi               # 请求的最小容量(必填)
    # limits:                     # 限制最大容量(未实现,v1.32 不支持)
    #   storage: 100Gi
```

**容量匹配规则**:
```yaml
# 静态供给: PV 容量 >= PVC 请求容量
# 示例:
# PVC 请求 50Gi
# PV 提供 100Gi
# 结果: 绑定成功,但 PVC 只能看到 50Gi(实际可用 100Gi)

# 动态供给: StorageClass 根据 requests.storage 创建精确大小的 PV
# 示例:
# PVC 请求 50Gi
# 动态创建: 50Gi PV (或云供应商最小规格,如 AWS EBS 最小 1Gi)
```

**最佳实践**:
```yaml
# ✅ 根据应用实际需求预估
resources:
  requests:
    storage: 100Gi                # 预留 20-30% 增长空间

# ✅ 考虑文件系统开销
# 块存储格式化为 ext4 会损失约 5% 容量
# 请求 100Gi → 实际可用 ~95Gi

# ✅ 使用标准单位
# Ki, Mi, Gi, Ti (二进制 1024 进制)
# k, M, G, T (十进制 1000 进制,不推荐)
```

### 2.3 存储类 (storageClassName)

```yaml
spec:
  storageClassName: fast-ssd      # 指定存储类名称
  # storageClassName: ""          # 显式设置空字符串,不使用 StorageClass
  # 未设置: 使用默认 StorageClass 或匹配未设置 storageClassName 的 PV
```

**三种配置模式**:

| 配置 | 行为 | 使用场景 |
|------|------|----------|
| 设置具体类名 | 1. 匹配该 storageClassName 的 PV<br>2. 或触发动态供给 | 生产环境(推荐) |
| 空字符串 `""` | 仅匹配 storageClassName="" 的 PV,禁用动态供给 | 静态 PV 绑定 |
| 不设置该字段 | 1. 使用默认 StorageClass (如果有)<br>2. 或匹配未设置 storageClassName 的 PV | 快速开发,使用默认类 |

**默认 StorageClass**:
```bash
# 查看默认 StorageClass
kubectl get storageclass
# NAME            PROVISIONER             RECLAIMPOLICY   VOLUMEBINDINGMODE      AGE
# gp2 (default)   kubernetes.io/aws-ebs   Delete          WaitForFirstConsumer   100d

# 设置默认 StorageClass
kubectl patch storageclass gp2 -p '{"metadata":{"annotations":{"storageclass.kubernetes.io/is-default-class":"true"}}}'

# 取消默认
kubectl patch storageclass gp2 -p '{"metadata":{"annotations":{"storageclass.kubernetes.io/is-default-class":"false"}}}'
```

**动态供给工作流程**:
```
PVC 创建 (storageClassName: fast-ssd)
         ↓
检查 StorageClass "fast-ssd" 是否存在
         ↓
调用 provisioner (如 ebs.csi.aws.com)
         ↓
创建后端存储资源 (如 AWS EBS 卷)
         ↓
自动创建 PV 对象
         ↓
PV 自动绑定到 PVC
         ↓
PVC 状态变为 Bound
```

### 2.4 卷模式 (volumeMode)

```yaml
spec:
  volumeMode: Filesystem          # 文件系统模式(默认)
  # volumeMode: Block             # 块设备模式(v1.18+ GA)
```

**模式对比**:

| 特性 | Filesystem | Block |
|------|------------|-------|
| 设备形式 | 目录 | 裸块设备(/dev/sdX) |
| 格式化 | 自动(ext4/xfs) | 应用层控制 |
| Pod 使用 | volumeMounts | volumeDevices |
| 适用场景 | 通用应用(95%+) | 数据库自管理存储 |

**块设备模式完整示例**:
```yaml
# PVC 配置
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: block-pvc
spec:
  accessModes:
  - ReadWriteOnce
  volumeMode: Block               # 块设备模式
  resources:
    requests:
      storage: 100Gi
  storageClassName: ebs-gp3

---
# Pod 配置
apiVersion: v1
kind: Pod
metadata:
  name: block-pod
spec:
  containers:
  - name: app
    image: postgres:15
    volumeDevices:                # 注意: 使用 volumeDevices 而非 volumeMounts
    - name: data
      devicePath: /dev/xvda       # 块设备路径(容器内)
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: block-pvc
```

**应用层使用块设备**:
```bash
# 在容器内
# 1. 检查设备
lsblk
# NAME   MAJ:MIN RM SIZE RO TYPE MOUNTPOINT
# xvda     8:0    0 100G  0 disk

# 2. 格式化(首次使用)
mkfs.ext4 /dev/xvda

# 3. 挂载(应用自己管理)
mkdir -p /data
mount /dev/xvda /data

# 4. 或直接使用裸设备(数据库引擎)
# PostgreSQL 可以配置 tablespace 直接使用块设备
```

### 2.5 静态绑定 (volumeName)

```yaml
spec:
  volumeName: pv-reserved-001     # 指定绑定的 PV 名称(可选)
```

**使用场景**:
1. **预留绑定**: 确保 PVC 绑定到特定 PV
2. **数据恢复**: 重新创建 PVC 绑定到保留的 PV (Retain 策略)
3. **测试环境**: 精确控制 PV-PVC 对应关系

**完整工作流程**:
```yaml
# 步骤 1: 创建带 claimRef 的 PV (可选,更严格)
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-reserved-001
spec:
  capacity:
    storage: 50Gi
  accessModes: [ReadWriteOnce]
  persistentVolumeReclaimPolicy: Retain
  storageClassName: manual
  claimRef:                       # 预绑定(可选)
    namespace: database
    name: mysql-pvc
  hostPath:
    path: /data/mysql

---
# 步骤 2: 创建指定 volumeName 的 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc
  namespace: database
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: manual
  volumeName: pv-reserved-001     # 强制绑定到此 PV
  resources:
    requests:
      storage: 50Gi

# 结果: PVC 立即绑定到 pv-reserved-001,即使有其他匹配的 PV
```

**注意事项**:
- volumeName 必须引用已存在的 PV
- 指定 volumeName 后,仍需满足其他匹配条件(容量、访问模式等)
- 常用于静态供给场景

### 2.6 标签选择器 (selector)

```yaml
spec:
  selector:                       # 通过标签过滤 PV(可选)
    matchLabels:                  # 精确匹配
      environment: production
      disk-type: ssd
    matchExpressions:             # 表达式匹配
    - key: tier
      operator: In
      values: [cache, database]
    - key: region
      operator: NotIn
      values: [us-east-1]
```

**选择器规则**:

| Operator | 描述 | 示例 |
|----------|------|------|
| In | 标签值在列表中 | `tier: In [cache, db]` |
| NotIn | 标签值不在列表中 | `region: NotIn [us-east-1]` |
| Exists | 标签键存在(不关心值) | `has-backup: Exists` |
| DoesNotExist | 标签键不存在 | `deprecated: DoesNotExist` |

**使用场景**:
```yaml
# 场景 1: 多层存储分级
# 高性能层
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: cache-pvc
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ""            # 不使用 StorageClass
  selector:
    matchLabels:
      disk-type: nvme             # 只匹配 NVMe 盘
      tier: premium
  resources:
    requests:
      storage: 100Gi

---
# 场景 2: 区域亲和性
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: web-assets-pvc
spec:
  accessModes: [ReadWriteMany]
  storageClassName: nfs
  selector:
    matchExpressions:
    - key: topology.kubernetes.io/zone
      operator: In
      values:
      - us-west-2a                # 只选择 2a 可用区的 PV
      - us-west-2b
  resources:
    requests:
      storage: 500Gi
```

**注意事项**:
- selector 与 storageClassName 同时使用时,必须两者都匹配
- 过于严格的 selector 可能导致无可用 PV
- 动态供给场景下 selector 通常不需要(StorageClass 已定义属性)

### 2.7 数据源 (dataSource)

```yaml
spec:
  dataSource:                     # 从现有卷克隆或快照恢复(v1.17+ GA)
    kind: PersistentVolumeClaim   # 来源类型: PVC | VolumeSnapshot
    name: source-pvc              # 来源名称
    apiGroup: ""                  # PVC 使用空字符串,VolumeSnapshot 使用 snapshot.storage.k8s.io
```

**支持的数据源类型**:

#### (1) 从 PVC 克隆

```yaml
# 原始 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: source-pvc
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ebs-gp3
  resources:
    requests:
      storage: 100Gi

---
# 克隆 PVC (创建副本)
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: cloned-pvc
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ebs-gp3       # 必须与源 PVC 相同
  dataSource:
    kind: PersistentVolumeClaim
    name: source-pvc              # 克隆源
  resources:
    requests:
      storage: 100Gi              # 可以 >= 源 PVC 容量
```

**克隆原理**:
```
源 PVC (100Gi, Bound)
        ↓
CSI Driver 执行克隆操作
  - 云盘: 调用云 API 克隆快照
  - Ceph: 执行 RBD clone
  - 本地: 使用 rsync/cp 复制
        ↓
创建新 PV (100Gi)
        ↓
新 PVC 绑定
```

**使用场景**:
- 开发/测试: 克隆生产数据到测试环境
- 蓝绿部署: 快速创建应用副本
- 数据分析: 复制生产数据库用于离线分析

#### (2) 从 VolumeSnapshot 恢复

```yaml
# 步骤 1: 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: db-snapshot-20260210
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: mysql-pvc

---
# 步骤 2: 从快照恢复到新 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc-restored
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ebs-gp3
  dataSource:
    kind: VolumeSnapshot
    name: db-snapshot-20260210
    apiGroup: snapshot.storage.k8s.io  # VolumeSnapshot 的 API 组
  resources:
    requests:
      storage: 100Gi              # 可以 >= 快照大小
```

**快照恢复原理**:
```
VolumeSnapshot 对象
        ↓
CSI Driver 从快照创建卷
  - AWS EBS: CreateVolume from Snapshot
  - Ceph: rbd clone
        ↓
新 PV 自动创建
        ↓
PVC 绑定到新 PV
```

**使用场景**:
- 灾难恢复: 从定时快照恢复数据
- 数据回滚: 恢复到已知良好状态
- 环境复制: 从快照快速创建新环境

### 2.8 数据源引用 (dataSourceRef)

```yaml
spec:
  dataSourceRef:                  # 扩展的数据源引用(v1.24+ Beta, v1.26+ GA)
    kind: VolumeSnapshot
    name: snapshot-001
    apiGroup: snapshot.storage.k8s.io
    namespace: source-ns          # v1.26+ 支持跨命名空间引用(需启用特性门)
```

**与 dataSource 区别**:

| 特性 | dataSource | dataSourceRef |
|------|------------|---------------|
| 跨命名空间 | ❌ 不支持 | ✅ 支持(v1.26+ Alpha) |
| 自定义资源 | ❌ 仅内置类型 | ✅ 支持任意 CRD |
| 命名空间字段 | ❌ 无 | ✅ 有 |
| 推荐使用 | 遗留 | 新项目推荐 |

**跨命名空间克隆示例** (v1.26+):
```yaml
# 启用特性门 (kube-apiserver 配置)
# --feature-gates=AnyVolumeDataSource=true,CrossNamespaceVolumeDataSource=true

# 源 PVC (命名空间 prod)
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: prod-db-pvc
  namespace: prod
spec:
  # ... 省略

---
# 克隆到不同命名空间 dev
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: dev-db-pvc
  namespace: dev
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ebs-gp3
  dataSourceRef:                  # 使用 dataSourceRef 而非 dataSource
    kind: PersistentVolumeClaim
    name: prod-db-pvc
    namespace: prod               # 跨命名空间引用
  resources:
    requests:
      storage: 100Gi
```

**注意事项**:
- 跨命名空间功能需要特性门启用(v1.26+ Alpha)
- 需要适当的 RBAC 权限
- CSI 驱动必须支持跨命名空间克隆

---

## 三、卷扩容 (Volume Expansion)

### 3.1 在线扩容

```yaml
# 前提条件:
# 1. StorageClass 设置 allowVolumeExpansion: true
# 2. CSI 驱动支持 EXPAND_VOLUME 能力

# 步骤 1: 编辑 PVC 增加容量
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: ebs-gp3
  resources:
    requests:
      storage: 200Gi              # 原 100Gi → 200Gi (只能增大不能减小)

# 步骤 2: 执行扩容
kubectl patch pvc mysql-pvc -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 步骤 3: 查看扩容状态
kubectl get pvc mysql-pvc
# NAME        STATUS   VOLUME      CAPACITY   ACCESS MODES   STORAGECLASS   AGE
# mysql-pvc   Bound    pvc-abc123  200Gi      RWO            ebs-gp3        5d

kubectl describe pvc mysql-pvc
# Events:
#   Type     Reason                      Age   Message
#   ----     ------                      ----  -------
#   Normal   VolumeResizeSuccessful      1m    Successfully resized volume pvc-abc123
#   Normal   FileSystemResizeSuccessful  30s   Successfully resized filesystem on volume pvc-abc123
```

**扩容阶段**:

| 阶段 | 操作层 | 状态字段 | 描述 |
|------|--------|----------|------|
| Controller Expand | 存储后端 | status.conditions[?(@.type=="Resizing")] | CSI 控制器调整后端卷大小 |
| Node Expand | 节点文件系统 | status.conditions[?(@.type=="FileSystemResizePending")] | Kubelet 调整文件系统大小 |
| Complete | - | status.capacity.storage | 扩容完成,容量更新 |

**扩容状态监控**:
```bash
# 实时监控扩容进度
kubectl get pvc mysql-pvc -o jsonpath='{.status.conditions[?(@.type=="Resizing")]}' | jq

# 查看扩容事件
kubectl get events --field-selector involvedObject.name=mysql-pvc --sort-by='.lastTimestamp'

# 检查文件系统大小(Pod 内)
kubectl exec mysql-0 -- df -h /var/lib/mysql
```

### 3.2 离线扩容

某些存储系统需要 Pod 停止才能扩容:

```yaml
# 步骤 1: 缩容应用(副本数 0)
kubectl scale statefulset mysql --replicas=0

# 步骤 2: 等待 Pod 终止
kubectl wait --for=delete pod/mysql-0 --timeout=60s

# 步骤 3: 扩容 PVC
kubectl patch pvc mysql-pvc -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 步骤 4: 等待扩容完成
kubectl wait --for=condition=FileSystemResizeSuccessful pvc/mysql-pvc --timeout=300s

# 步骤 5: 恢复应用
kubectl scale statefulset mysql --replicas=3
```

### 3.3 扩容限制

```yaml
# ❌ 不支持的操作
# 1. 减小容量
spec:
  resources:
    requests:
      storage: 50Gi               # 从 100Gi 减到 50Gi (会被拒绝)

# 2. 修改 storageClassName
spec:
  storageClassName: fast-ssd      # 从 ebs-gp3 改为 fast-ssd (不可变字段)

# 3. 修改 accessModes
spec:
  accessModes:
  - ReadWriteMany                 # 从 RWO 改为 RWX (不可变字段)
```

**错误处理**:
```bash
# 扩容失败常见原因
kubectl describe pvc mysql-pvc
# Events:
#   Warning  VolumeResizeFailed  1m  external-resizer  rpc error: code = ResourceExhausted desc = Quota exceeded

# 解决方案:
# 1. 检查云账户配额
# 2. 检查 StorageClass 的 allowVolumeExpansion 设置
# 3. 检查 CSI 驱动日志
kubectl logs -n kube-system -l app=ebs-csi-controller
```

---

## 四、内部原理

### 4.1 PVC 生命周期

```yaml
status:
  phase: Bound                    # 当前阶段
  # 可能的值: Pending | Bound | Lost
```

**状态机**:
```
    创建 PVC
        ↓
   Pending (等待绑定)
        ↓
 ┌──────────────────┐
 │  静态供给?       │
 │  ├─是→ 查找匹配PV │
 │  └─否→ 触发动态供给│
 └──────┬───────────┘
        ↓
   (绑定成功)
        ↓
    Bound (已绑定)
        ↓
   (PV 被删除)
        ↓
    Lost (PV 丢失)
        ↓
   (PVC 删除)
        ↓
   (级联删除 PV)
```

**阶段详解**:

| Phase | 描述 | 常见原因 | 解决方法 |
|-------|------|----------|----------|
| **Pending** | 等待绑定 | 无匹配 PV 或等待动态供给 | 检查 PV 列表、StorageClass 配置 |
| **Bound** | 已绑定到 PV | 正常工作状态 | - |
| **Lost** | 绑定的 PV 丢失 | PV 被手动删除 | 恢复 PV 或重建 PVC |

### 4.2 动态供给流程

**组件角色**:
- **PVC Controller**: 监控 PVC,触发动态供给
- **External Provisioner**: CSI 外部供给器(Sidecar)
- **CSI Driver**: 实际执行存储创建

**详细流程**:
```
1. 用户创建 PVC (storageClassName: ebs-gp3)
        ↓
2. PVC Controller 检测到新 PVC
        ↓
3. 检查 StorageClass "ebs-gp3"
   - provisioner: ebs.csi.aws.com
   - volumeBindingMode: WaitForFirstConsumer
        ↓
4. 根据 volumeBindingMode 决定时机:
   ├─ Immediate: 立即创建 PV
   └─ WaitForFirstConsumer: 等待 Pod 创建
        ↓
5. Pod 创建,调度器选择节点 (如 node-01, zone=us-west-2a)
        ↓
6. PVC Controller 调用 External Provisioner
   参数: {capacity: 100Gi, zone: us-west-2a, ...}
        ↓
7. External Provisioner 调用 CSI Driver 的 CreateVolume RPC
        ↓
8. CSI Driver 调用云 API (如 AWS EC2 CreateVolume)
   返回: volumeHandle=vol-0abc123
        ↓
9. External Provisioner 创建 PV 对象
   spec.csi.volumeHandle: vol-0abc123
   spec.claimRef: 指向 PVC
        ↓
10. PV 自动绑定到 PVC
        ↓
11. PVC 状态变为 Bound
        ↓
12. Kubelet 挂载 PV 到 Pod
```

**延迟绑定 (WaitForFirstConsumer)**:

```yaml
# StorageClass 配置
volumeBindingMode: WaitForFirstConsumer

# 优点:
# 1. 拓扑感知: 确保 PV 创建在 Pod 所在可用区
# 2. 避免资源浪费: PVC 创建后不立即分配 PV
# 3. 调度优化: 考虑存储位置优化 Pod 调度

# 工作流程:
PVC 创建 → Pending 状态
         ↓
    Pod 创建并调度
         ↓
调度器选择节点(考虑存储拓扑)
         ↓
   通知 PVC Controller
         ↓
    触发动态供给
         ↓
  PV 创建在同一拓扑域
         ↓
     PVC 绑定
         ↓
    Pod 继续启动
```

### 4.3 绑定控制器算法

```go
// 伪代码展示 PVC-PV 绑定逻辑
func bindPVCtoPV(pvc *PVC) error {
    // 1. 检查 PVC 是否已绑定
    if pvc.Spec.VolumeName != "" {
        // 静态绑定: 直接使用指定的 PV
        pv := getPV(pvc.Spec.VolumeName)
        return bindVolumes(pvc, pv)
    }
    
    // 2. 检查 StorageClass
    if pvc.Spec.StorageClassName != nil {
        sc := getStorageClass(*pvc.Spec.StorageClassName)
        if sc.Provisioner != "" {
            // 动态供给: 等待 Provisioner 创建 PV
            return waitForDynamicProvisioning(pvc, sc)
        }
    }
    
    // 3. 查找匹配的 Available PV
    pvs := findMatchingPVs(pvc)
    if len(pvs) == 0 {
        return errors.New("no matching PV found")
    }
    
    // 4. 选择最优 PV (最小满足原则)
    bestPV := selectBestPV(pvs, pvc)
    
    // 5. 双向绑定
    return bindVolumes(pvc, bestPV)
}

func findMatchingPVs(pvc *PVC) []*PV {
    candidates := []*PV{}
    
    for _, pv := range allPVs {
        // 过滤条件
        if pv.Status.Phase != "Available" { continue }
        if !matchStorageClassName(pv, pvc) { continue }
        if !matchAccessModes(pv, pvc) { continue }
        if pv.Spec.Capacity.Storage < pvc.Spec.Resources.Requests.Storage { continue }
        if pvc.Spec.Selector != nil && !matchSelector(pv.Labels, pvc.Spec.Selector) { continue }
        if pvc.Spec.VolumeMode != pv.Spec.VolumeMode { continue }
        
        candidates = append(candidates, pv)
    }
    
    return candidates
}
```

---

## 五、配置模板

### 5.1 最小配置 (开发环境)

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: dev-pvc
spec:
  accessModes:
  - ReadWriteOnce
  resources:
    requests:
      storage: 1Gi
```

### 5.2 生产级配置 (动态供给)

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data-pvc
  namespace: database
  labels:
    app: mysql
    tier: database
    environment: production
  annotations:
    description: "MySQL primary database persistent storage"
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: ebs-gp3-encrypted  # 加密的 gp3 存储类
  resources:
    requests:
      storage: 200Gi              # 预留足够空间
  # selector:                     # 可选: 精确控制 PV 选择
  #   matchLabels:
  #     tier: database
```

### 5.3 静态绑定配置

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: static-pvc
  namespace: default
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: ""            # 显式禁用动态供给
  volumeName: pv-reserved-001     # 静态绑定到指定 PV
  resources:
    requests:
      storage: 50Gi
```

### 5.4 克隆配置

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: cloned-pvc
  namespace: test
  labels:
    cloned-from: prod-db-pvc
    environment: test
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: ebs-gp3       # 必须与源 PVC 相同
  dataSource:
    kind: PersistentVolumeClaim
    name: prod-db-pvc
  resources:
    requests:
      storage: 100Gi              # 可以 >= 源 PVC
```

### 5.5 快照恢复配置

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: restored-pvc
  namespace: database
  labels:
    restored-from: snapshot-20260210
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: ebs-gp3
  dataSource:
    kind: VolumeSnapshot
    name: db-snapshot-20260210
    apiGroup: snapshot.storage.k8s.io
  resources:
    requests:
      storage: 150Gi              # 可以大于快照大小(扩容恢复)
```

### 5.6 共享存储配置 (RWX)

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: shared-data-pvc
  namespace: web
  labels:
    type: shared-storage
    usage: static-assets
spec:
  accessModes:
  - ReadWriteMany                 # 多节点读写
  storageClassName: nfs-shared
  resources:
    requests:
      storage: 500Gi
```

### 5.7 块设备配置

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: block-pvc
  namespace: database
  labels:
    app: postgresql
    volume-type: block
spec:
  accessModes:
  - ReadWriteOnce
  volumeMode: Block               # 块设备模式
  storageClassName: ebs-io2
  resources:
    requests:
      storage: 500Gi
```

---

## 六、生产案例

### 6.1 案例 1: StatefulSet 使用动态 PVC

**场景**: MySQL 主从复制,3 副本 StatefulSet

```yaml
apiVersion: v1
kind: StorageClass
metadata:
  name: mysql-storage
provisioner: ebs.csi.aws.com
parameters:
  type: gp3                       # gp3 类型
  iops: "16000"                   # 16000 IOPS
  throughput: "1000"              # 1000 MB/s
  encrypted: "true"               # 加密
  kmsKeyId: "arn:aws:kms:..."     # KMS 密钥
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定(拓扑感知)
allowVolumeExpansion: true        # 允许扩容
reclaimPolicy: Retain             # 保留数据

---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: database
spec:
  serviceName: mysql
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  volumeClaimTemplates:           # 自动创建 PVC
  - metadata:
      name: data
      labels:
        app: mysql
    spec:
      accessModes: [ReadWriteOnce]
      storageClassName: mysql-storage
      resources:
        requests:
          storage: 200Gi
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
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: root-password
        ports:
        - containerPort: 3306
          name: mysql
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"

# 效果:
# - 自动创建 3 个 PVC: data-mysql-0, data-mysql-1, data-mysql-2
# - 每个 PVC 绑定独立的 200Gi EBS 卷
# - Pod 删除后 PVC 保留,数据持久化
# - 扩容 StatefulSet 时自动创建新 PVC
```

**扩容数据卷**:
```bash
# 增加 mysql-0 的数据卷容量
kubectl patch pvc data-mysql-0 -n database -p '{"spec":{"resources":{"requests":{"storage":"300Gi"}}}}'

# 验证扩容
kubectl get pvc -n database data-mysql-0
# NAME            STATUS   VOLUME        CAPACITY   ACCESS MODES   STORAGECLASS     AGE
# data-mysql-0    Bound    pvc-abc123    300Gi      RWO            mysql-storage    10d
```

### 6.2 案例 2: 从快照恢复数据库

**场景**: 生产数据库误删除数据,需要从昨天快照恢复

```yaml
# 步骤 1: 查看可用快照
kubectl get volumesnapshot -n database
# NAME                        READYTOUSE   SOURCEPVC       AGE
# mysql-backup-20260209       true         mysql-pvc       1d
# mysql-backup-20260210       true         mysql-pvc       6h

---
# 步骤 2: 创建恢复 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc-restored
  namespace: database
  labels:
    app: mysql
    restored: "true"
    restore-date: "2026-02-09"
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: ebs-gp3
  dataSource:
    kind: VolumeSnapshot
    name: mysql-backup-20260209  # 恢复到昨天状态
    apiGroup: snapshot.storage.k8s.io
  resources:
    requests:
      storage: 200Gi

---
# 步骤 3: 创建验证 Pod (检查数据完整性)
apiVersion: v1
kind: Pod
metadata:
  name: mysql-verify
  namespace: database
spec:
  containers:
  - name: mysql
    image: mysql:8.0
    command: ["/bin/bash"]
    args:
    - -c
    - |
      # 启动 MySQL
      docker-entrypoint.sh mysqld &
      # 等待就绪
      until mysqladmin ping -h localhost --silent; do
        echo "等待 MySQL 启动..."
        sleep 2
      done
      # 验证数据
      mysql -u root -p$MYSQL_ROOT_PASSWORD -e "SELECT COUNT(*) FROM mydb.users;"
      # 保持运行以便手动验证
      tail -f /dev/null
    env:
    - name: MYSQL_ROOT_PASSWORD
      valueFrom:
        secretKeyRef:
          name: mysql-secret
          key: root-password
    volumeMounts:
    - name: data
      mountPath: /var/lib/mysql
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: mysql-pvc-restored

---
# 步骤 4: 验证数据
kubectl exec -it mysql-verify -n database -- mysql -u root -p$MYSQL_ROOT_PASSWORD

# 步骤 5: 数据确认无误后,替换生产 PVC
# 5.1 停止应用
kubectl scale statefulset mysql -n database --replicas=0

# 5.2 备份当前 PVC (可选)
kubectl get pvc mysql-pvc -n database -o yaml > mysql-pvc-backup.yaml

# 5.3 删除当前 PVC (确保备份后)
kubectl delete pvc mysql-pvc -n database --wait=false

# 5.4 重命名恢复的 PVC
kubectl patch pvc mysql-pvc-restored -n database -p '{"metadata":{"name":"mysql-pvc"}}'

# 5.5 重启应用
kubectl scale statefulset mysql -n database --replicas=3
```

### 6.3 案例 3: 开发环境快速克隆生产数据

**场景**: 开发团队需要完整的生产数据副本进行测试

```yaml
# 前提: 生产 PVC 已存在
# prod namespace: mysql-pvc (200Gi)

# 步骤 1: 创建克隆 PVC (开发命名空间)
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc-dev
  namespace: dev
  labels:
    environment: dev
    cloned-from: prod-mysql-pvc
    clone-date: "2026-02-10"
  annotations:
    description: "Development clone of production MySQL data"
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: ebs-gp3       # 与生产相同
  dataSource:
    kind: PersistentVolumeClaim
    name: mysql-pvc
    # 注意: dataSource 不支持跨命名空间,需使用以下方法之一:
    # 方法 1: 先在 prod 创建快照,再从快照恢复到 dev
    # 方法 2: 使用 dataSourceRef + 特性门(v1.26+)
  resources:
    requests:
      storage: 200Gi

---
# 方法 1 推荐: 通过快照跨命名空间克隆
# 步骤 1.1: 在 prod 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-clone-snapshot
  namespace: prod
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: mysql-pvc

---
# 步骤 1.2: 在 dev 从快照恢复
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-pvc-dev
  namespace: dev
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: ebs-gp3
  dataSource:
    kind: VolumeSnapshot
    name: mysql-clone-snapshot
    apiGroup: snapshot.storage.k8s.io
  resources:
    requests:
      storage: 200Gi

---
# 步骤 2: 部署开发数据库
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mysql-dev
  namespace: dev
spec:
  replicas: 1
  selector:
    matchLabels:
      app: mysql
      environment: dev
  template:
    metadata:
      labels:
        app: mysql
        environment: dev
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        env:
        - name: MYSQL_ROOT_PASSWORD
          value: "dev-password"   # 开发环境密码
        ports:
        - containerPort: 3306
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        resources:
          requests:
            memory: "2Gi"         # 开发环境资源更小
            cpu: "1"
      volumes:
      - name: data
        persistentVolumeClaim:
          claimName: mysql-pvc-dev

---
# 步骤 3: 自动化清理脚本 (定期清理开发克隆)
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cleanup-dev-clones
  namespace: dev
spec:
  schedule: "0 2 * * 0"           # 每周日凌晨 2 点
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: pvc-cleaner
          containers:
          - name: cleanup
            image: bitnami/kubectl:latest
            command:
            - /bin/bash
            - -c
            - |
              # 删除超过 7 天的克隆 PVC
              kubectl get pvc -n dev -l cloned-from=prod-mysql-pvc -o json | \
              jq -r '.items[] | select(.metadata.creationTimestamp | fromdateiso8601 < (now - 604800)) | .metadata.name' | \
              xargs -r kubectl delete pvc -n dev
          restartPolicy: OnFailure
```

### 6.4 案例 4: 共享存储 Web 静态资源

**场景**: 10 个 Nginx Pod 共享静态资源(图片/CSS/JS)

```yaml
# 步骤 1: 创建共享 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: web-assets-pvc
  namespace: web
  labels:
    app: nginx
    type: shared-storage
spec:
  accessModes:
  - ReadWriteMany                 # 多节点读写
  storageClassName: nfs-shared    # NFS 存储类
  resources:
    requests:
      storage: 500Gi

---
# 步骤 2: 部署 Nginx
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
  namespace: web
spec:
  replicas: 10
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
      - name: nginx
        image: nginx:1.25
        ports:
        - containerPort: 80
        volumeMounts:
        - name: assets
          mountPath: /usr/share/nginx/html/assets
          readOnly: false         # 允许写入(上传新资源)
        resources:
          requests:
            memory: "128Mi"
            cpu: "100m"
          limits:
            memory: "256Mi"
            cpu: "200m"
      volumes:
      - name: assets
        persistentVolumeClaim:
          claimName: web-assets-pvc

---
# 步骤 3: 创建文件上传 Job (管理资源)
apiVersion: batch/v1
kind: Job
metadata:
  name: upload-assets
  namespace: web
spec:
  template:
    spec:
      containers:
      - name: uploader
        image: alpine:3.18
        command:
        - /bin/sh
        - -c
        - |
          # 从 S3 同步资源到共享存储
          apk add --no-cache aws-cli
          aws s3 sync s3://my-bucket/assets/ /assets/ --delete
          echo "资源同步完成"
        volumeMounts:
        - name: assets
          mountPath: /assets
        env:
        - name: AWS_ACCESS_KEY_ID
          valueFrom:
            secretKeyRef:
              name: aws-credentials
              key: access-key
        - name: AWS_SECRET_ACCESS_KEY
          valueFrom:
            secretKeyRef:
              name: aws-credentials
              key: secret-key
      volumes:
      - name: assets
        persistentVolumeClaim:
          claimName: web-assets-pvc
      restartPolicy: OnFailure
```

---

## 七、故障排查

### 7.1 PVC 一直 Pending

**症状**:
```bash
kubectl get pvc
# NAME      STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE
# my-pvc    Pending   ""       ""         ""             ebs-gp3        5m
```

**排查步骤**:
```bash
# 1. 查看详细信息
kubectl describe pvc my-pvc
# Events:
#   Type     Reason              Age   Message
#   ----     ------              ----  -------
#   Warning  ProvisioningFailed  3m    Failed to provision volume: rpc error: code = ResourceExhausted

# 2. 检查 StorageClass
kubectl get storageclass ebs-gp3 -o yaml

# 3. 检查 CSI Driver Pods
kubectl get pods -n kube-system -l app=ebs-csi-controller
kubectl logs -n kube-system -l app=ebs-csi-controller -c csi-provisioner

# 4. 检查 WaitForFirstConsumer 模式
kubectl get storageclass ebs-gp3 -o jsonpath='{.volumeBindingMode}'
# 如果是 WaitForFirstConsumer, PVC 需要等待 Pod 创建

# 5. 创建测试 Pod 触发绑定
kubectl run test-pod --image=nginx --overrides='
{
  "spec": {
    "volumes": [{
      "name": "data",
      "persistentVolumeClaim": {"claimName": "my-pvc"}
    }],
    "containers": [{
      "name": "nginx",
      "image": "nginx",
      "volumeMounts": [{"name": "data", "mountPath": "/data"}]
    }]
  }
}'
```

**常见原因**:

| 错误信息 | 原因 | 解决方案 |
|----------|------|----------|
| no persistent volumes available | 无匹配 PV(静态供给) | 创建匹配的 PV 或启用动态供给 |
| StorageClass not found | storageClassName 不存在 | 修正 storageClassName 或创建 StorageClass |
| quota exceeded | 云账户配额不足 | 增加配额或删除未使用的卷 |
| WaitForFirstConsumer | 延迟绑定等待 Pod | 创建使用该 PVC 的 Pod |
| insufficient capacity | 节点存储空间不足 | 清理空间或添加节点 |

### 7.2 克隆/快照恢复失败

**症状**:
```bash
kubectl describe pvc cloned-pvc
# Events:
#   Warning  ProvisioningFailed  1m  Failed to provision volume: source PVC must be Bound
```

**排查清单**:
```bash
# 1. 确认源 PVC 状态
kubectl get pvc source-pvc
# STATUS 必须是 Bound

# 2. 确认 StorageClass 一致
kubectl get pvc source-pvc -o jsonpath='{.spec.storageClassName}'
kubectl get pvc cloned-pvc -o jsonpath='{.spec.storageClassName}'
# 必须完全相同

# 3. 检查 CSI 驱动是否支持克隆
kubectl get csidrivers ebs.csi.aws.com -o yaml | grep -A 5 volumeLifecycleModes
# 应包含: Persistent

# 4. 检查快照对象状态(快照恢复场景)
kubectl get volumesnapshot my-snapshot -o jsonpath='{.status.readyToUse}'
# 应返回: true

# 5. 检查 CSI Snapshotter 日志
kubectl logs -n kube-system -l app=ebs-csi-controller -c csi-snapshotter
```

### 7.3 PVC 扩容卡住

**症状**:
```bash
kubectl get pvc mysql-pvc
# NAME        STATUS   VOLUME      CAPACITY   ACCESS MODES   STORAGECLASS   AGE
# mysql-pvc   Bound    pvc-abc123  100Gi      RWO            ebs-gp3        5d
# (已修改 PVC 为 200Gi,但 CAPACITY 仍显示 100Gi)

kubectl describe pvc mysql-pvc
# Conditions:
#   Type                      Status  Message
#   ----                      ------  -------
#   FileSystemResizePending   True    Waiting for user to restart Pod
```

**原因**: 某些文件系统需要 Pod 重启才能识别新容量

**解决方案**:
```bash
# 方法 1: 滚动重启 Pod
kubectl rollout restart statefulset mysql

# 方法 2: 手动删除 Pod (StatefulSet 会自动重建)
kubectl delete pod mysql-0

# 方法 3: 进入 Pod 手动触发文件系统扩展
kubectl exec mysql-0 -- resize2fs /dev/sda1    # ext4
kubectl exec mysql-0 -- xfs_growfs /data       # xfs

# 验证
kubectl exec mysql-0 -- df -h /var/lib/mysql
```

### 7.4 PVC 无法删除

**症状**:
```bash
kubectl delete pvc my-pvc
# pvc "my-pvc" deleted
# (命令挂起,PVC 不消失)

kubectl get pvc
# NAME      STATUS        VOLUME      CAPACITY   ACCESS MODES   AGE
# my-pvc    Terminating   pvc-abc123  100Gi      RWO            10d
```

**原因**: PVC 有 finalizers 保护,通常因为仍被 Pod 使用

**排查步骤**:
```bash
# 1. 检查 finalizers
kubectl get pvc my-pvc -o jsonpath='{.metadata.finalizers}'
# ["kubernetes.io/pvc-protection"]

# 2. 查找使用该 PVC 的 Pod
kubectl get pods --all-namespaces -o json | \
  jq -r '.items[] | select(.spec.volumes[]?.persistentVolumeClaim.claimName=="my-pvc") | .metadata.namespace + "/" + .metadata.name'

# 3. 删除使用该 PVC 的 Pod
kubectl delete pod <pod-name>

# 4. 如果 Pod 已删除但 PVC 仍卡住,强制移除 finalizer (危险操作)
kubectl patch pvc my-pvc -p '{"metadata":{"finalizers":null}}'

# 5. 检查 PV 回收策略
kubectl get pv pvc-abc123 -o jsonpath='{.spec.persistentVolumeReclaimPolicy}'
# 如果是 Delete, PV 会自动删除
# 如果是 Retain, PV 变为 Released, 需要手动处理
```

---

## 八、最佳实践总结

### 8.1 命名规范

```yaml
# ✅ 推荐: 语义化命名
metadata:
  name: mysql-primary-data-pvc    # 清晰表达用途
  labels:
    app: mysql
    component: primary
    data-type: persistent

# ❌ 避免: 无意义命名
metadata:
  name: pvc-001                   # 难以识别用途
```

### 8.2 资源规划

```yaml
# ✅ 容量预留 20-30%
resources:
  requests:
    storage: 120Gi                # 实际需求 100Gi, 预留 20Gi

# ✅ 考虑文件系统开销
# 请求 100Gi → 格式化后 ~95Gi 可用

# ✅ 云盘对齐供应商规格
# AWS EBS gp3: 最小 1Gi, 最大 16TiB
# Azure Disk: 最小 4Gi
# Alibaba Cloud: 最小 20Gi
```

### 8.3 存储类选择

```yaml
# ✅ 显式指定 storageClassName
spec:
  storageClassName: production-ssd  # 明确存储类型

# ⚠️ 谨慎使用默认类
# 未设置 storageClassName 时使用默认类,可能不符合需求

# ❌ 避免空字符串(除非明确需要静态 PV)
spec:
  storageClassName: ""            # 仅用于静态供给
```

### 8.4 数据保护

```yaml
# ✅ 重要数据使用 Retain 策略的 StorageClass
reclaimPolicy: Retain             # PVC 删除后 PV 保留

# ✅ 定期创建快照
apiVersion: batch/v1
kind: CronJob
metadata:
  name: daily-snapshot
spec:
  schedule: "0 2 * * *"           # 每天凌晨 2 点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: snapshotter
            image: bitnami/kubectl:latest
            command:
            - /bin/bash
            - -c
            - |
              DATE=$(date +%Y%m%d-%H%M%S)
              cat <<EOF | kubectl apply -f -
              apiVersion: snapshot.storage.k8s.io/v1
              kind: VolumeSnapshot
              metadata:
                name: mysql-pvc-$DATE
              spec:
                volumeSnapshotClassName: csi-snapclass
                source:
                  persistentVolumeClaimName: mysql-pvc
              EOF
          restartPolicy: OnFailure

# ✅ 标签标识重要性
metadata:
  labels:
    data-criticality: high        # 标记数据重要性
    backup-policy: daily          # 标记备份策略
```

### 8.5 监控告警

```yaml
# Prometheus 告警规则示例
groups:
- name: pvc-alerts
  rules:
  # PVC 使用率 > 80%
  - alert: PVCHighUsage
    expr: |
      (kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes) > 0.8
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "PVC {{ $labels.persistentvolumeclaim }} 使用率超过 80%"
  
  # PVC 长时间 Pending
  - alert: PVCPending
    expr: |
      kube_persistentvolumeclaim_status_phase{phase="Pending"} == 1
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "PVC {{ $labels.persistentvolumeclaim }} 长时间 Pending"
  
  # 扩容失败
  - alert: PVCResizeFailed
    expr: |
      kube_persistentvolumeclaim_status_condition{condition="Resizing",status="False"} == 1
    labels:
      severity: critical
    annotations:
      summary: "PVC {{ $labels.persistentvolumeclaim }} 扩容失败"
```

---

## 九、参考资源

### 9.1 官方文档

- [Persistent Volumes](https://kubernetes.io/docs/concepts/storage/persistent-volumes/)
- [Dynamic Volume Provisioning](https://kubernetes.io/docs/concepts/storage/dynamic-provisioning/)
- [Volume Snapshots](https://kubernetes.io/docs/concepts/storage/volume-snapshots/)
- [Storage Capacity Tracking](https://kubernetes.io/docs/concepts/storage/storage-capacity/)

### 9.2 相关 KEP

- [KEP-1495: Volume Health Monitoring](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/1495-volume-health-monitor)
- [KEP-1682: Skip Volume Ownership Change](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/1682-csi-driver-skip-permission)
- [KEP-1790: Recover from Expansion Failure](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/1790-recover-resize-failure)
- [KEP-3294: Cross Namespace Data Sources](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/3294-provision-volumes-from-cross-namespace-snapshots)

### 9.3 版本差异

| 功能 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|-------|-------|-------|
| dataSourceRef 跨命名空间 | ❌ | Alpha | Alpha | Alpha | Beta | Beta | Beta | GA |
| 扩容失败恢复 | Beta | Beta | GA | GA | GA | GA | GA | GA |
| ReadWriteOncePod | Alpha | Alpha | Beta | Beta | GA | GA | GA | GA |
| VolumeAttributesClass | ❌ | ❌ | Alpha | Alpha | Beta | Beta | GA | GA |

---

**文档维护**: 2026-02  
**下一次审阅**: 2026-08 (Kubernetes v1.33 发布后)
