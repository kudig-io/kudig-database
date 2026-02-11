# 15 - PersistentVolume YAML 配置参考

> **文档版本**: 2026-02  
> **适用范围**: Kubernetes v1.25 - v1.32  
> **资源类型**: PersistentVolume (PV)  
> **API 版本**: v1  
> **用途**: 集群级别持久化存储资源抽象

---

## 一、API 资源信息

### 1.1 基本信息

```yaml
# API 元数据
apiVersion: v1                    # PV 在 v1 API 组中,核心稳定资源
kind: PersistentVolume            # 资源类型

metadata:
  name: pv-example                # PV 名称(集群级别唯一,无 namespace)
  labels:                         # 标签用于分类和选择器匹配
    type: local
    environment: production
  annotations:                    # 注解存储元数据
    pv.kubernetes.io/provisioned-by: "manual"  # 供给方式标识
```

**关键特性**:
- **集群级别资源**: PV 不属于任何 namespace
- **生命周期独立**: PV 可以独立于 PVC 存在
- **容量单位**: 支持 Ki, Mi, Gi, Ti, Pi, Ei (二进制)

### 1.2 核心字段结构

```yaml
spec:
  capacity:                       # 存储容量(必填)
  accessModes:                    # 访问模式(必填)
  persistentVolumeReclaimPolicy:  # 回收策略
  storageClassName:               # 存储类名称
  volumeMode:                     # 卷模式
  mountOptions:                   # 挂载选项
  nodeAffinity:                   # 节点亲和性
  claimRef:                       # 绑定的 PVC 引用
  <volumeSource>:                 # 具体卷源(必填其一)

status:
  phase:                          # 生命周期阶段
  message:                        # 状态消息
  reason:                         # 状态原因
```

---

## 二、完整字段详解

### 2.1 容量声明 (capacity)

```yaml
spec:
  capacity:
    storage: 10Gi                 # 存储容量(必填)
    # 注意: v1.25-v1.32 仅支持 storage 维度
    # 未来可能支持 IOPS、吞吐量等多维度资源
```

**最佳实践**:
- 预留 10-15% 空间防止文件系统元数据占用
- 云盘场景严格匹配云供应商规格 (如阿里云盘最小 20Gi)
- 本地盘使用 `du -sh` 精确测量可用容量

### 2.2 访问模式 (accessModes)

```yaml
spec:
  accessModes:
    # ========== 四种标准模式 ==========
    - ReadWriteOnce               # RWO: 单节点读写(最常用)
    # - ReadOnlyMany              # ROX: 多节点只读
    # - ReadWriteMany             # RWX: 多节点读写(需特殊存储支持)
    # - ReadWriteOncePod          # RWOP: 单 Pod 读写(v1.27+ Beta,v1.29+ GA)
```

**模式说明**:

| 模式 | 缩写 | 并发特性 | 典型场景 | 存储类型支持 |
|------|------|----------|----------|--------------|
| ReadWriteOnce | RWO | 单节点多 Pod 可读写 | 数据库、有状态应用 | 块存储(云盘/iSCSI/Local) |
| ReadOnlyMany | ROX | 多节点只读共享 | 配置文件分发、静态资源 | NFS/CephFS |
| ReadWriteMany | RWX | 多节点读写共享 | 共享文件系统、日志聚合 | NFS/GlusterFS/CephFS |
| ReadWriteOncePod | RWOP | 单 Pod 独占读写 | 强一致性数据库 | CSI 驱动需明确支持 |

**选型决策树**:
```
是否需要多节点共享?
├─ 否 → 需要单 Pod 独占? 
│      ├─ 是 → RWOP (v1.29+)
│      └─ 否 → RWO (默认选择)
└─ 是 → 需要写入?
       ├─ 是 → RWX (需 NFS 等支持)
       └─ 否 → ROX
```

### 2.3 回收策略 (persistentVolumeReclaimPolicy)

```yaml
spec:
  persistentVolumeReclaimPolicy: Retain  # 回收策略(默认值)
  # 可选值: Retain | Delete | Recycle (已废弃)
```

**策略对比**:

| 策略 | PVC 删除后行为 | 数据保留 | PV 状态 | 适用场景 |
|------|----------------|----------|---------|----------|
| **Retain** | 保留 PV 和数据,需手动清理 | ✅ 保留 | Released | 生产环境(防数据丢失) |
| **Delete** | 自动删除 PV 和后端存储 | ❌ 删除 | - | 开发/测试(自动清理) |
| **Recycle** | 执行 `rm -rf` 清空数据 | ❌ 清空 | Available | v1.15 已废弃 |

**生产环境最佳实践**:
```yaml
# 关键数据使用 Retain
persistentVolumeReclaimPolicy: Retain

# 手动回收流程:
# 1. PVC 删除后,PV 变为 Released 状态
# 2. 备份数据: kubectl exec ... -- tar czf /backup/data.tar.gz /data
# 3. 清理数据: 登录节点删除或重新格式化
# 4. 删除 PV: kubectl delete pv <pv-name>
# 5. 重新创建 PV 供新 PVC 使用
```

### 2.4 存储类 (storageClassName)

```yaml
spec:
  storageClassName: fast-ssd      # 关联存储类名称
  # storageClassName: ""          # 显式设置空字符串表示不使用 StorageClass
  # 未设置时: 匹配同样未设置 storageClassName 的 PVC
```

**三种配置模式**:

| 配置 | 匹配规则 | 使用场景 |
|------|----------|----------|
| 设置具体类名 | 必须 PVC 的 storageClassName 完全一致 | 静态 PV 预分类 (ssd/hdd/nfs) |
| 空字符串 `""` | PVC 也必须显式设置空字符串 | 遗留系统兼容,不使用 StorageClass |
| 不设置该字段 | 匹配未设置 storageClassName 的 PVC | 早期版本(v1.6 之前)兼容 |

### 2.5 卷模式 (volumeMode)

```yaml
spec:
  volumeMode: Filesystem          # 文件系统模式(默认)
  # volumeMode: Block             # 块设备模式(v1.18+ GA)
```

**模式对比**:

| 模式 | 设备形式 | 格式化 | 挂载点 | 适用场景 |
|------|----------|--------|--------|----------|
| **Filesystem** | 文件系统 | 自动格式化 | 目录路径 | 通用应用(90%+场景) |
| **Block** | 裸块设备 | 应用层控制 | 块设备(/dev/sdX) | 数据库(自管理存储层) |

**块设备模式示例**:
```yaml
# PV 配置
spec:
  volumeMode: Block
  capacity:
    storage: 100Gi
  csi:
    driver: ebs.csi.aws.com
    volumeHandle: vol-0abc123

# Pod 使用
spec:
  containers:
  - name: database
    volumeDevices:              # 注意: 使用 volumeDevices 而非 volumeMounts
    - name: data
      devicePath: /dev/xvda     # 块设备路径
```

### 2.6 挂载选项 (mountOptions)

```yaml
spec:
  mountOptions:                   # 传递给 mount 命令的选项(可选)
    - hard                        # NFS: 硬挂载(IO 失败时阻塞而非报错)
    - nfsvers=4.1                 # NFS: 强制使用 NFSv4.1 协议
    - rsize=1048576               # NFS: 读取缓冲区大小 1MB
    - wsize=1048576               # NFS: 写入缓冲区大小 1MB
    - timeo=600                   # NFS: 超时时间 60 秒(单位 0.1s)
    - retrans=2                   # NFS: 超时重试次数
    - noresvport                  # NFS: 客户端使用非特权端口
```

**注意事项**:
- ⚠️ **CSI 驱动兼容性**: 部分 CSI 驱动不支持 mountOptions,会忽略或报错
- ⚠️ **权限要求**: 某些选项需要容器特权模式 (privileged: true)
- ✅ **验证方法**: 检查节点挂载: `mount | grep <pv-name>`

**常见选项**:
```yaml
# NFS 生产推荐
mountOptions:
  - hard                          # 防止数据损坏
  - nfsvers=4.1                   # 性能更好
  - noresvport                    # 防止端口耗尽

# iSCSI 多路径
mountOptions:
  - ro                            # 只读挂载(ROX 模式)
  - noatime                       # 不更新访问时间(性能优化)

# Local PV (ext4)
mountOptions:
  - discard                       # SSD Trim 支持
  - nobarrier                     # 禁用写屏障(电池备份 RAID 卡)
```

### 2.7 节点亲和性 (nodeAffinity)

```yaml
spec:
  nodeAffinity:                   # Local PV 必填字段
    required:                     # 硬性要求(必须满足)
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - node-01               # 卷绑定到特定节点
        - key: topology.kubernetes.io/zone
          operator: In
          values:
          - us-west-2a            # 指定可用区
```

**使用场景**:
1. **Local PV**: 强制 Pod 调度到存储节点
2. **多可用区**: 确保 PV/Pod 在同一可用区(降低延迟)
3. **特殊硬件**: 绑定到有 GPU/NVMe 的节点

**注意**:
- Local PV 必须设置 nodeAffinity
- 与 StorageClass 的 `volumeBindingMode: WaitForFirstConsumer` 配合使用
- 影响 Pod 调度(只能调度到匹配节点)

### 2.8 绑定引用 (claimRef)

```yaml
spec:
  claimRef:                       # 预绑定指定 PVC(可选)
    namespace: database           # PVC 所在命名空间
    name: mysql-data-pvc          # PVC 名称
    uid: a1b2c3d4-...             # PVC UID(可选,更精确)
```

**使用场景**:
1. **静态供给**: 预留 PV 给特定 PVC
2. **数据恢复**: 新建 PVC 绑定到保留的 PV (Retain 策略场景)
3. **防止误用**: 避免 PV 被其他 PVC 意外绑定

**工作流程**:
```yaml
# 步骤 1: 创建带 claimRef 的 PV
apiVersion: v1
kind: PersistentVolume
metadata:
  name: reserved-pv
spec:
  capacity:
    storage: 50Gi
  accessModes: [ReadWriteOnce]
  claimRef:
    namespace: prod
    name: mysql-pvc               # 预留给 prod/mysql-pvc
  hostPath:
    path: /data/mysql

# 步骤 2: 创建匹配的 PVC
# 其他 PVC 即使匹配容量/访问模式,也无法绑定此 PV
```

---

## 三、卷源类型 (Volume Sources)

### 3.1 HostPath (开发/测试)

```yaml
spec:
  hostPath:
    path: /data/volumes/pv-001    # 节点本地路径(必填)
    type: DirectoryOrCreate       # 路径类型(可选)
    # 可选值: DirectoryOrCreate | Directory | FileOrCreate | 
    #        File | Socket | CharDevice | BlockDevice
```

**类型说明**:

| Type | 检查行为 | 不存在时 | 使用场景 |
|------|----------|----------|----------|
| DirectoryOrCreate | 目录 | 自动创建 | 通用场景 |
| Directory | 必须是已存在目录 | 报错 | 数据已存在 |
| FileOrCreate | 文件 | 创建空文件 | 配置文件 |
| File | 必须是已存在文件 | 报错 | 只读配置 |
| BlockDevice | 块设备 | 报错 | 裸盘 |

**安全警告**:
```yaml
# ⚠️ HostPath 存在严重安全风险,仅限以下场景:
# 1. 单节点开发环境
# 2. 节点级别的 DaemonSet 代理(如监控、日志)
# 3. 测试 Local PV 前的概念验证

# ❌ 生产环境禁止用于应用数据:
# - Pod 重新调度到其他节点时数据丢失
# - 无访问控制(任何 Pod 可访问相同路径)
# - 可能逃逸容器访问节点文件系统
```

### 3.2 Local (生产级本地卷)

```yaml
spec:
  local:
    path: /mnt/disks/ssd1         # 本地磁盘挂载点(必填)
    fsType: ext4                  # 文件系统类型(可选)
  nodeAffinity:                   # Local PV 必须配置
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - kube-node-03
```

**与 HostPath 区别**:

| 特性 | Local | HostPath |
|------|-------|----------|
| 节点绑定 | 强制(通过 nodeAffinity) | 无保证(Pod 可能漂移) |
| 卷类型检测 | 识别为本地卷 | 识别为节点路径 |
| 调度感知 | 调度器优先级考虑 | 不参与调度决策 |
| 生产支持 | ✅ 推荐 | ❌ 禁止 |

**完整生产配置**:
```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-node03-ssd1
  labels:
    node: kube-node-03
    disk-type: ssd
spec:
  capacity:
    storage: 500Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain    # 生产必须 Retain
  storageClassName: local-storage
  volumeMode: Filesystem
  local:
    path: /mnt/disks/ssd1
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - kube-node-03
```

**准备工作**(在节点执行):
```bash
# 1. 分区格式化磁盘
sudo fdisk /dev/nvme1n1          # 创建分区
sudo mkfs.ext4 /dev/nvme1n1p1    # 格式化

# 2. 创建挂载点
sudo mkdir -p /mnt/disks/ssd1

# 3. 配置自动挂载
echo 'UUID=xxx /mnt/disks/ssd1 ext4 defaults 0 2' | sudo tee -a /etc/fstab
sudo mount -a

# 4. 验证
df -h /mnt/disks/ssd1
```

### 3.3 NFS (网络文件系统)

```yaml
spec:
  nfs:
    server: nfs.example.com       # NFS 服务器地址(必填)
    path: /exports/k8s/pv-001     # 导出路径(必填)
    readOnly: false               # 只读模式(可选,默认 false)
  mountOptions:                   # NFS 优化选项
    - hard
    - nfsvers=4.1
    - rsize=1048576
    - wsize=1048576
```

**完整配置示例**:
```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: nfs-pv-shared
spec:
  capacity:
    storage: 100Gi
  accessModes:
  - ReadWriteMany                 # NFS 支持多节点读写
  persistentVolumeReclaimPolicy: Retain
  storageClassName: nfs
  mountOptions:
    - hard                        # 硬挂载(推荐)
    - nfsvers=4.1                 # NFSv4.1(性能更好)
    - noresvport                  # 非保留端口
  nfs:
    server: 192.168.1.100
    path: /data/k8s/shared
```

**NFS 服务器配置**(Ubuntu 示例):
```bash
# 安装 NFS 服务器
sudo apt-get install nfs-kernel-server

# 创建导出目录
sudo mkdir -p /data/k8s/shared
sudo chown nobody:nogroup /data/k8s/shared
sudo chmod 777 /data/k8s/shared  # 或根据需求设置

# 配置导出
sudo tee -a /etc/exports <<EOF
/data/k8s/shared 192.168.1.0/24(rw,sync,no_subtree_check,no_root_squash)
EOF

# 应用配置
sudo exportfs -ra
sudo systemctl restart nfs-kernel-server
```

**节点准备**(所有 Kubernetes 节点):
```bash
# 安装 NFS 客户端
sudo apt-get install nfs-common  # Ubuntu/Debian
sudo yum install nfs-utils       # CentOS/RHEL

# 测试挂载
sudo mount -t nfs4 nfs.example.com:/data/k8s/shared /mnt/test
```

### 3.4 iSCSI (企业级块存储)

```yaml
spec:
  iscsi:
    targetPortal: 192.168.1.200:3260  # iSCSI Target 地址:端口(必填)
    iqn: iqn.2024-01.com.example:storage.lun1  # IQN 标识符(必填)
    lun: 0                            # LUN 编号(必填)
    fsType: ext4                      # 文件系统类型(默认 ext4)
    readOnly: false                   # 只读模式
    chapAuthDiscovery: true           # 启用 CHAP 认证发现
    chapAuthSession: true             # 启用 CHAP 认证会话
    secretRef:                        # CHAP 认证凭据
      name: iscsi-chap-secret
    initiatorName: iqn.2024-01.k8s.node:initiator01  # 发起方 IQN(可选)
```

**CHAP 认证配置**:
```yaml
# Secret 对象
apiVersion: v1
kind: Secret
metadata:
  name: iscsi-chap-secret
  namespace: default              # 必须与 PVC 同命名空间
type: kubernetes.io/iscsi-chap
data:
  node.session.auth.username: aXNjc2l1c2Vy        # base64(iscsiuser)
  node.session.auth.password: cGFzc3dvcmQxMjM=    # base64(password123)
```

**节点准备**:
```bash
# 安装 iSCSI 发起方
sudo apt-get install open-iscsi    # Ubuntu
sudo yum install iscsi-initiator-utils  # CentOS

# 配置发起方名称
sudo vi /etc/iscsi/initiatorname.iscsi
InitiatorName=iqn.2024-01.k8s.node:initiator01

# 发现 Target
sudo iscsiadm -m discovery -t st -p 192.168.1.200:3260

# 登录 Target
sudo iscsiadm -m node -T iqn.2024-01.com.example:storage.lun1 -p 192.168.1.200:3260 --login

# 验证
lsblk  # 查看新增块设备
```

### 3.5 CSI (容器存储接口)

```yaml
spec:
  csi:
    driver: ebs.csi.aws.com       # CSI 驱动名称(必填)
    volumeHandle: vol-0abc123def  # 卷标识符(必填,驱动特定)
    fsType: ext4                  # 文件系统类型(可选)
    readOnly: false               # 只读模式
    volumeAttributes:             # 驱动特定参数(可选)
      type: gp3                   # AWS EBS gp3 类型
      iopsPerGB: "10"             # IOPS 配置
    controllerPublishSecretRef:   # Controller 阶段认证(可选)
      name: csi-secret
      namespace: kube-system
    nodeStageSecretRef:           # Node Stage 阶段认证(可选)
      name: csi-secret
      namespace: kube-system
    nodePublishSecretRef:         # Node Publish 阶段认证(可选)
      name: csi-secret
      namespace: kube-system
    controllerExpandSecretRef:    # 扩容阶段认证(可选)
      name: csi-secret
      namespace: kube-system
```

**主流 CSI 驱动示例**:

#### AWS EBS CSI
```yaml
spec:
  csi:
    driver: ebs.csi.aws.com
    volumeHandle: vol-0a1b2c3d4e5f6g7h8
    fsType: ext4
    volumeAttributes:
      type: gp3                   # 通用型 SSD
      iops: "3000"                # 基准 IOPS
      throughput: "125"           # 吞吐量 MB/s
```

#### Azure Disk CSI
```yaml
spec:
  csi:
    driver: disk.csi.azure.com
    volumeHandle: /subscriptions/.../resourceGroups/.../providers/Microsoft.Compute/disks/disk-001
    fsType: ext4
    volumeAttributes:
      skuName: Premium_LRS        # 高级 SSD
      cachingMode: ReadOnly       # 缓存模式
```

#### Alibaba Cloud Disk CSI
```yaml
spec:
  csi:
    driver: diskplugin.csi.alibabacloud.com
    volumeHandle: d-bp1abc123def
    fsType: ext4
    volumeAttributes:
      type: cloud_essd            # ESSD 云盘
      performanceLevel: PL1       # 性能级别
```

### 3.6 云盘卷源 (已废弃)

> ⚠️ **废弃警告**: 以下内置云盘卷源在 v1.26+ 标记为废弃,v1.31+ 完全移除,请迁移到对应 CSI 驱动

```yaml
# ❌ 已废弃 - 仅用于遗留集群
spec:
  awsElasticBlockStore:           # 使用 ebs.csi.aws.com 替代
    volumeID: vol-xxx
    fsType: ext4
  
  azureDisk:                      # 使用 disk.csi.azure.com 替代
    diskName: disk-001
    diskURI: /subscriptions/.../disks/disk-001
  
  gcePersistentDisk:              # 使用 pd.csi.storage.gke.io 替代
    pdName: gke-disk-001
    fsType: ext4
```

**迁移指南**:
1. 安装对应云供应商的 CSI 驱动
2. 创建使用 CSI 的新 StorageClass
3. 通过 PVC 克隆或备份恢复迁移数据
4. 删除旧的 PV/PVC

---

## 四、生命周期管理

### 4.1 PV 生命周期阶段

```yaml
status:
  phase: Bound                    # 当前阶段
  # 可能的值: Available | Bound | Released | Failed
```

**阶段状态机**:

```
     创建 PV
        ↓
   Available (可用)
        ↓
    (PVC 绑定)
        ↓
     Bound (已绑定)
        ↓
    (PVC 删除)
        ↓
┌─────────────────────┐
│  Retain 策略        │ Delete 策略
│  Released (已释放)  │ (自动删除)
│        ↓            │
│  (手动清理)         │
│        ↓            │
│  Available          │
└─────────────────────┘
        ↓
   (清理失败)
        ↓
     Failed (失败)
```

**阶段详解**:

| Phase | 描述 | claimRef | 数据状态 | 后续操作 |
|-------|------|----------|----------|----------|
| **Available** | 未绑定,可被 PVC 申领 | null | - | 等待绑定 |
| **Bound** | 已绑定到 PVC | 非空 | 使用中 | 正常使用 |
| **Released** | PVC 已删除,PV 保留(Retain) | 非空 | 保留 | 手动清理+删除 |
| **Failed** | 自动回收失败 | - | 不确定 | 检查日志修复 |

### 4.2 绑定机制

**绑定算法**(PV Controller):
```go
// 伪代码展示绑定逻辑
func findMatchingPV(pvc *PVC) *PV {
    candidates := []PV{}
    
    // 1. 预筛选
    for pv in allPVs {
        if pv.Status.Phase != "Available" { continue }
        if pv.Spec.ClaimRef != nil && !matchClaimRef(pv, pvc) { continue }
        if pv.Spec.StorageClassName != pvc.Spec.StorageClassName { continue }
        if !matchAccessModes(pv, pvc) { continue }
        if pv.Spec.Capacity.Storage < pvc.Spec.Resources.Requests.Storage { continue }
        
        candidates = append(candidates, pv)
    }
    
    // 2. 按容量排序(最小满足优先)
    sort(candidates, by: capacity ascending)
    
    // 3. 检查 Selector(如果 PVC 有)
    for pv in candidates {
        if matchSelector(pv.Labels, pvc.Spec.Selector) {
            return pv
        }
    }
    
    return nil
}
```

**绑定条件检查清单**:
- ✅ PV 状态为 Available
- ✅ storageClassName 完全匹配(或都未设置)
- ✅ accessModes 交集非空(PV 支持 PVC 请求的模式)
- ✅ PV 容量 >= PVC 请求容量
- ✅ PVC 的 selector 匹配 PV 的 labels(如果有)
- ✅ PV 的 claimRef 为空或指向当前 PVC
- ✅ nodeAffinity 与 Pod 调度节点兼容(WaitForFirstConsumer 模式)

### 4.3 手动回收流程 (Retain 策略)

```bash
# 场景: PVC 被删除,PV 变为 Released 状态,需要回收复用

# 步骤 1: 查看 PV 状态
kubectl get pv my-pv -o yaml

# 输出示例:
# status:
#   phase: Released
# spec:
#   claimRef:                    # ← 包含已删除 PVC 的引用
#     namespace: default
#     name: old-pvc
#     uid: ...

# 步骤 2: 备份数据(根据卷类型)
# Local PV:
kubectl debug node/kube-node-01 -it --image=ubuntu
tar czf /host/backup/data.tar.gz /host/mnt/disks/ssd1/

# NFS:
tar czf /tmp/backup.tar.gz /mnt/nfs-mount/

# 步骤 3: 清理数据
# 登录节点或通过 Pod 执行
rm -rf /mnt/disks/ssd1/*         # Local
# 或重新格式化
mkfs.ext4 /dev/nvme1n1p1

# 步骤 4: 删除 claimRef 使 PV 重新 Available
kubectl patch pv my-pv -p '{"spec":{"claimRef":null}}'

# 步骤 5: 验证状态
kubectl get pv my-pv
# NAME    CAPACITY   ACCESS MODES   STATUS      CLAIM   STORAGECLASS   AGE
# my-pv   100Gi      RWO            Available           local-storage  10d

# 步骤 6: 新 PVC 绑定
kubectl apply -f new-pvc.yaml
```

**自动化回收脚本**:
```bash
#!/bin/bash
# reclaim-pv.sh - 自动回收 Released 状态的 PV

PV_NAME=$1

# 检查状态
PHASE=$(kubectl get pv "$PV_NAME" -o jsonpath='{.status.phase}')
if [ "$PHASE" != "Released" ]; then
    echo "PV 状态不是 Released,当前: $PHASE"
    exit 1
fi

# 获取卷类型和路径
VOLUME_TYPE=$(kubectl get pv "$PV_NAME" -o jsonpath='{.spec}' | jq -r 'keys[] | select(. != "capacity" and . != "accessModes" and . != "persistentVolumeReclaimPolicy")')
echo "卷类型: $VOLUME_TYPE"

# 根据类型清理(示例)
case $VOLUME_TYPE in
    local)
        NODE=$(kubectl get pv "$PV_NAME" -o jsonpath='{.spec.nodeAffinity.required.nodeSelectorTerms[0].matchExpressions[?(@.key=="kubernetes.io/hostname")].values[0]}')
        PATH=$(kubectl get pv "$PV_NAME" -o jsonpath='{.spec.local.path}')
        echo "在节点 $NODE 清理路径 $PATH"
        # 实际执行需要 SSH 或 kubectl debug
        ;;
    nfs)
        echo "NFS 卷需要手动清理服务器端数据"
        ;;
esac

# 移除 claimRef
kubectl patch pv "$PV_NAME" --type json -p '[{"op":"remove","path":"/spec/claimRef"}]'

echo "PV $PV_NAME 已回收"
```

---

## 五、内部原理

### 5.1 PV Controller 工作机制

**核心组件**:
- **PV Controller**: 运行在 kube-controller-manager 中
- **职责**: 监控 PV/PVC 对象,执行绑定、回收等操作

**控制循环**:
```
┌─────────────────────────────────────┐
│   Watch PVC 创建/更新事件            │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│   查找匹配的 Available PV           │
│   - 过滤条件: storageClassName,      │
│     accessModes, capacity, selector │
└──────────────┬──────────────────────┘
               ↓
          找到匹配?
        ↙         ↘
      是           否
      ↓             ↓
┌─────────────┐  ┌──────────────────┐
│  双向绑定    │  │ 等待动态供给或    │
│  - PV.spec. │  │ 新 PV 创建        │
│    claimRef │  └──────────────────┘
│  - PVC.spec.│
│    volumeName│
└──────┬──────┘
       ↓
┌─────────────────────────────────────┐
│   更新状态 Phase: Bound             │
└─────────────────────────────────────┘

┌─────────────────────────────────────┐
│   Watch PVC 删除事件                │
└──────────────┬──────────────────────┘
               ↓
      根据 reclaimPolicy
        ↙         ↘
   Retain        Delete
      ↓             ↓
┌──────────┐  ┌──────────────────┐
│ 状态设为  │  │ 调用卷插件删除    │
│ Released │  │ 后端存储,删除 PV  │
└──────────┘  └──────────────────┘
```

### 5.2 绑定优化算法

**最小满足原则**:
```yaml
# 示例场景
PVC 请求: 10Gi

可用 PV:
  - pv-small: 15Gi     # 最接近,优先选择
  - pv-medium: 50Gi
  - pv-large: 100Gi
  
# 结果: PVC 绑定到 pv-small (最小化资源浪费)
```

**延迟绑定 (WaitForFirstConsumer)**:
```yaml
# StorageClass 配置
volumeBindingMode: WaitForFirstConsumer

# 工作流程:
# 1. PVC 创建后保持 Pending 状态
# 2. Pod 被调度到节点时
# 3. 调度器将节点信息传递给 PV Controller
# 4. Controller 选择符合节点拓扑的 PV 绑定
# 5. PVC 变为 Bound, Pod 继续启动
```

### 5.3 存储拓扑感知

**原理**:
```yaml
# PV 定义可用区
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pv-zone-a
spec:
  capacity:
    storage: 100Gi
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: topology.kubernetes.io/zone
          operator: In
          values:
          - us-west-2a            # 限定可用区

# Pod 调度时:
# 1. 调度器识别 PV 的拓扑约束
# 2. 只考虑 us-west-2a 可用区的节点
# 3. 避免跨可用区挂载(降低延迟,避免网络费用)
```

**云环境最佳实践**:
```yaml
# StorageClass 定义拓扑要求
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
volumeBindingMode: WaitForFirstConsumer
allowedTopologies:                # 限制 PV 创建的拓扑域
- matchLabelExpressions:
  - key: topology.kubernetes.io/zone
    values:
    - us-west-2a
    - us-west-2b
    - us-west-2c
```

---

## 六、配置模板

### 6.1 最小配置 (开发环境)

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: dev-pv-001
spec:
  capacity:
    storage: 5Gi
  accessModes:
  - ReadWriteOnce
  hostPath:
    path: /tmp/k8s-pv-data
    type: DirectoryOrCreate
```

### 6.2 生产级 Local PV

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: prod-local-pv-node01-nvme0
  labels:
    node: kube-node-01
    disk-type: nvme
    environment: production
  annotations:
    volume.kubernetes.io/provisioned-by: "manual"
spec:
  capacity:
    storage: 1Ti
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain    # 生产必须
  storageClassName: local-nvme
  volumeMode: Filesystem
  mountOptions:
  - discard                                # SSD Trim
  - noatime                                # 性能优化
  local:
    path: /mnt/disks/nvme0n1
    fsType: ext4
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - kube-node-01
        - key: node.kubernetes.io/instance-type  # 额外拓扑约束
          operator: In
          values:
          - i3.2xlarge                     # AWS 实例类型
```

### 6.3 生产级 NFS PV

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: prod-nfs-shared-logs
  labels:
    type: nfs
    usage: shared-logs
spec:
  capacity:
    storage: 500Gi
  accessModes:
  - ReadWriteMany                          # NFS 支持多节点读写
  persistentVolumeReclaimPolicy: Retain
  storageClassName: nfs-shared
  mountOptions:
  - hard                                   # 硬挂载(防数据损坏)
  - nfsvers=4.1                            # NFSv4.1
  - rsize=1048576                          # 1MB 读缓冲
  - wsize=1048576                          # 1MB 写缓冲
  - timeo=600                              # 60s 超时
  - retrans=2                              # 重试 2 次
  - noresvport                             # 非保留端口
  nfs:
    server: nfs-server.example.com
    path: /exports/k8s/shared-logs
```

### 6.4 生产级 CSI PV (AWS EBS)

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: prod-ebs-pv-postgres
  labels:
    app: postgresql
    tier: database
  annotations:
    volume.kubernetes.io/provisioned-by: "ebs.csi.aws.com"
spec:
  capacity:
    storage: 200Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: ebs-gp3
  volumeMode: Filesystem
  csi:
    driver: ebs.csi.aws.com
    volumeHandle: vol-0a1b2c3d4e5f6g7h8   # 已存在的 EBS 卷 ID
    fsType: ext4
    volumeAttributes:
      type: gp3                            # gp3 类型
      iops: "16000"                        # 16000 IOPS
      throughput: "1000"                   # 1000 MB/s 吞吐
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: topology.kubernetes.io/zone
          operator: In
          values:
          - us-west-2a                     # 必须与 EBS 卷同可用区
```

---

## 七、生产案例

### 7.1 案例 1: 高性能数据库 Local PV

**场景**: PostgreSQL 数据库,要求低延迟、高 IOPS

**架构设计**:
```
节点: 3 个数据节点(kube-node-01/02/03)
磁盘: 每节点 1 块 NVMe SSD (1TB)
应用: StatefulSet 3 副本,每副本独立 Local PV
```

**PV 配置**:
```yaml
# pv-postgres-0.yaml (为 postgres-0 Pod 准备)
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-postgres-0
  labels:
    app: postgresql
    replica: "0"
spec:
  capacity:
    storage: 900Gi                         # 预留 10% 空间
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-nvme
  volumeMode: Filesystem
  mountOptions:
  - noatime                                # 不更新访问时间
  - nodiratime                             # 目录也不更新
  - discard                                # 支持 Trim
  local:
    path: /mnt/disks/nvme0n1
    fsType: ext4
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - kube-node-01                   # 绑定到 node-01

---
# pv-postgres-1.yaml (为 postgres-1 Pod 准备)
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-postgres-1
  labels:
    app: postgresql
    replica: "1"
spec:
  capacity:
    storage: 900Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-nvme
  local:
    path: /mnt/disks/nvme0n1
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - kube-node-02                   # 绑定到 node-02

---
# pv-postgres-2.yaml (类似,绑定到 node-03)
# ...
```

**PVC 配置** (StatefulSet volumeClaimTemplates):
```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgresql
spec:
  serviceName: postgres
  replicas: 3
  selector:
    matchLabels:
      app: postgresql
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: [ReadWriteOnce]
      storageClassName: local-nvme
      resources:
        requests:
          storage: 900Gi
  template:
    spec:
      containers:
      - name: postgres
        image: postgres:15
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
```

**节点准备脚本**:
```bash
#!/bin/bash
# prepare-local-pv.sh - 在所有数据节点执行

# 1. 识别 NVMe 磁盘
DEVICE="/dev/nvme0n1"

# 2. 分区(如果未分区)
if ! lsblk "$DEVICE"1 >/dev/null 2>&1; then
    echo "创建分区..."
    sudo parted -s "$DEVICE" mklabel gpt
    sudo parted -s "$DEVICE" mkpart primary ext4 0% 100%
fi

# 3. 格式化
PARTITION="${DEVICE}p1"
sudo mkfs.ext4 -F -m 1 "$PARTITION"  # -m 1 保留 1% 给 root

# 4. 创建挂载点
sudo mkdir -p /mnt/disks/nvme0n1

# 5. 获取 UUID
UUID=$(sudo blkid -s UUID -o value "$PARTITION")

# 6. 配置自动挂载
if ! grep -q "$UUID" /etc/fstab; then
    echo "UUID=$UUID /mnt/disks/nvme0n1 ext4 defaults,noatime,nodiratime,discard 0 2" | sudo tee -a /etc/fstab
fi

# 7. 挂载
sudo mount -a

# 8. 验证
df -h /mnt/disks/nvme0n1
sudo fio --name=randwrite --ioengine=libaio --iodepth=16 --rw=randwrite --bs=4k --direct=1 --size=1G --numjobs=4 --runtime=60 --group_reporting --filename=/mnt/disks/nvme0n1/test
sudo rm /mnt/disks/nvme0n1/test
```

**性能验证**:
```bash
# 在 Pod 内测试
kubectl exec -it postgresql-0 -- bash

# 随机写测试
fio --name=randwrite --ioengine=libaio --iodepth=32 --rw=randwrite --bs=8k --direct=1 --size=10G --runtime=60 --group_reporting --filename=/var/lib/postgresql/data/test

# 期望结果 (NVMe):
# - IOPS: > 50,000
# - 延迟: < 1ms
```

### 7.2 案例 2: 共享文件系统 NFS PV

**场景**: 多个 Web 服务器共享静态资源(图片、CSS、JS)

**架构设计**:
```
NFS 服务器: 1 台专用服务器 (nfs.example.com)
容量: 2TB
客户端: 10 个 Nginx Pod (Deployment)
访问模式: ReadWriteMany (多 Pod 并发读写)
```

**NFS 服务器配置**:
```bash
# 安装 NFS 服务器 (Ubuntu 22.04)
sudo apt-get update
sudo apt-get install -y nfs-kernel-server

# 创建导出目录
sudo mkdir -p /exports/k8s/web-assets
sudo chown nobody:nogroup /exports/k8s/web-assets
sudo chmod 755 /exports/k8s/web-assets

# 配置导出
cat <<EOF | sudo tee /etc/exports
/exports/k8s/web-assets 10.0.0.0/8(rw,sync,no_subtree_check,no_root_squash,no_all_squash)
EOF

# 应用配置
sudo exportfs -ra
sudo systemctl enable nfs-kernel-server
sudo systemctl restart nfs-kernel-server

# 验证
sudo exportfs -v
showmount -e localhost
```

**PV 配置**:
```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: nfs-pv-web-assets
  labels:
    type: nfs
    usage: web-assets
spec:
  capacity:
    storage: 2Ti
  accessModes:
  - ReadWriteMany                          # 多节点读写
  persistentVolumeReclaimPolicy: Retain
  storageClassName: nfs
  mountOptions:
  - hard                                   # 硬挂载
  - nfsvers=4.1                            # NFSv4.1
  - rsize=1048576                          # 1MB 读缓冲
  - wsize=1048576                          # 1MB 写缓冲
  - timeo=600                              # 60s 超时
  - retrans=2                              # 重试 2 次
  - noresvport                             # 非保留端口
  nfs:
    server: nfs.example.com
    path: /exports/k8s/web-assets
```

**PVC 配置**:
```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: web-assets-pvc
  namespace: web
spec:
  accessModes:
  - ReadWriteMany
  storageClassName: nfs
  resources:
    requests:
      storage: 2Ti
```

**Deployment 配置**:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-web
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
          mountPath: /usr/share/nginx/html/assets  # 挂载到 Nginx 资源目录
          readOnly: false                          # 允许写入(上传新资源)
      volumes:
      - name: assets
        persistentVolumeClaim:
          claimName: web-assets-pvc
```

**节点准备** (所有 Kubernetes 节点):
```bash
# 安装 NFS 客户端
sudo apt-get install -y nfs-common  # Ubuntu
# sudo yum install -y nfs-utils     # CentOS

# 测试挂载
sudo mkdir -p /mnt/test-nfs
sudo mount -t nfs4 nfs.example.com:/exports/k8s/web-assets /mnt/test-nfs
ls -la /mnt/test-nfs
sudo umount /mnt/test-nfs
```

**监控脚本**:
```bash
#!/bin/bash
# nfs-monitor.sh - 监控 NFS 性能

NFS_SERVER="nfs.example.com"
EXPORT_PATH="/exports/k8s/web-assets"

while true; do
    echo "===== $(date) ====="
    
    # NFS 统计
    nfsstat -c | grep -A 10 "Client rpc stats"
    
    # 挂载点 IO
    iostat -x 5 1 | grep nfs
    
    # 连接数
    ss -tn | grep ":2049" | wc -l
    
    sleep 60
done
```

---

## 八、故障排查

### 8.1 PV 无法绑定

**症状**:
```bash
kubectl get pvc
# NAME        STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE
# my-pvc      Pending   ""       ""         ""             local-storage  5m
```

**排查步骤**:
```bash
# 1. 检查 PVC 详情
kubectl describe pvc my-pvc
# Events:
#   Type     Reason              Age   From                         Message
#   ----     ------              ----  ----                         -------
#   Warning  ProvisioningFailed  3m    persistentvolume-controller  no persistent volumes available for this claim

# 2. 检查是否有匹配的 PV
kubectl get pv
# 确认:
# - 是否有 Available 状态的 PV
# - storageClassName 是否匹配
# - accessModes 是否兼容
# - capacity 是否满足

# 3. 检查 StorageClass
kubectl get storageclass local-storage -o yaml
# 确认:
# - provisioner 是否正确
# - volumeBindingMode (WaitForFirstConsumer 需要 Pod 创建)

# 4. 检查 PVC 的 Selector
kubectl get pvc my-pvc -o yaml | grep -A 5 selector
# 如果有 selector, 确认 PV 的 labels 是否匹配

# 5. 检查日志
kubectl logs -n kube-system -l component=kube-controller-manager | grep persistentvolume
```

**常见原因**:

| 问题 | 解决方案 |
|------|----------|
| storageClassName 不匹配 | PV 和 PVC 必须完全一致(包括空字符串 "") |
| PV 容量小于 PVC 请求 | 调整 PV 容量或减少 PVC 请求 |
| accessModes 不兼容 | PV: [RWO], PVC: [RWX] 无法匹配 |
| PVC selector 不匹配 PV labels | 添加匹配的 labels 到 PV |
| WaitForFirstConsumer 需 Pod | 创建使用该 PVC 的 Pod 才会触发绑定 |
| PV 已有 claimRef | 删除或修改 claimRef 字段 |

### 8.2 Local PV 挂载失败

**症状**:
```bash
kubectl describe pod my-pod
# Events:
#   Warning  FailedMount  10s  kubelet  MountVolume.SetUp failed for volume "local-pv-001" : 
#            stat /mnt/disks/ssd1: no such file or directory
```

**排查步骤**:
```bash
# 1. 确认 PV 绑定的节点
kubectl get pv local-pv-001 -o yaml | grep -A 10 nodeAffinity

# 2. 登录到对应节点
ssh user@kube-node-01

# 3. 检查路径是否存在
ls -ld /mnt/disks/ssd1
stat /mnt/disks/ssd1

# 4. 检查挂载
mount | grep /mnt/disks/ssd1
df -h /mnt/disks/ssd1

# 5. 检查权限
ls -la /mnt/disks/ssd1

# 6. 检查文件系统
sudo file -s /dev/nvme0n1p1
sudo fsck -n /dev/nvme0n1p1    # 只读检查
```

**解决方案**:
```bash
# 问题 1: 路径不存在
sudo mkdir -p /mnt/disks/ssd1
sudo mount /dev/nvme0n1p1 /mnt/disks/ssd1

# 问题 2: 权限不足
sudo chmod 755 /mnt/disks/ssd1
sudo chown root:root /mnt/disks/ssd1

# 问题 3: 未挂载
# 添加到 /etc/fstab 并挂载
UUID=$(sudo blkid -s UUID -o value /dev/nvme0n1p1)
echo "UUID=$UUID /mnt/disks/ssd1 ext4 defaults 0 2" | sudo tee -a /etc/fstab
sudo mount -a

# 问题 4: 文件系统损坏
sudo umount /mnt/disks/ssd1
sudo fsck -y /dev/nvme0n1p1    # 自动修复
sudo mount /mnt/disks/ssd1
```

### 8.3 NFS 挂载超时

**症状**:
```bash
kubectl describe pod web-001
# Events:
#   Warning  FailedMount  1m  kubelet  
#            Unable to attach or mount volumes: timeout expired waiting for volumes to attach or mount
```

**排查步骤**:
```bash
# 1. 在节点测试 NFS 连接
ssh user@kube-node-01
showmount -e nfs.example.com
# Export list for nfs.example.com:
# /exports/k8s/web-assets 10.0.0.0/8

# 2. 手动挂载测试
sudo mkdir -p /mnt/test-nfs
sudo mount -t nfs4 -o hard,nfsvers=4.1 nfs.example.com:/exports/k8s/web-assets /mnt/test-nfs
# 如果超时: mount.nfs4: Connection timed out

# 3. 网络连通性
ping nfs.example.com
telnet nfs.example.com 2049       # NFSv4 端口

# 4. 防火墙检查
sudo iptables -L -n | grep 2049

# 5. NFS 服务器端检查
# 登录 NFS 服务器
sudo systemctl status nfs-kernel-server
sudo exportfs -v
sudo cat /var/log/syslog | grep nfs
```

**解决方案**:
```bash
# 问题 1: 防火墙阻止
# NFS 服务器端开放端口
sudo ufw allow from 10.0.0.0/8 to any port 2049
sudo ufw allow from 10.0.0.0/8 to any port 111    # portmapper
sudo ufw reload

# 问题 2: NFS 服务未运行
sudo systemctl start nfs-kernel-server
sudo systemctl enable nfs-kernel-server

# 问题 3: 导出配置错误
# 修正 /etc/exports
sudo vi /etc/exports
# 确保: /exports/k8s/web-assets 10.0.0.0/8(rw,sync,no_subtree_check,no_root_squash)
sudo exportfs -ra

# 问题 4: 客户端未安装 NFS 工具
sudo apt-get install -y nfs-common  # Ubuntu
sudo yum install -y nfs-utils       # CentOS
```

### 8.4 PV Released 无法复用

**症状**:
```bash
kubectl get pv
# NAME     CAPACITY   ACCESS MODES   RECLAIM POLICY   STATUS     CLAIM             AGE
# my-pv    100Gi      RWO            Retain           Released   default/old-pvc   10d

# 新建 PVC 无法绑定此 PV
```

**原因**: PV 的 `spec.claimRef` 仍指向已删除的 PVC

**解决方案**:
```bash
# 方法 1: 移除 claimRef
kubectl patch pv my-pv -p '{"spec":{"claimRef":null}}'

# 方法 2: 使用 JSON Patch
kubectl patch pv my-pv --type json -p '[{"op":"remove","path":"/spec/claimRef"}]'

# 方法 3: 编辑 YAML
kubectl edit pv my-pv
# 删除 spec.claimRef 整个字段

# 验证
kubectl get pv my-pv
# STATUS 应变为 Available
```

---

## 九、最佳实践总结

### 9.1 容量规划

```yaml
# ✅ 推荐
capacity:
  storage: 90Gi                   # 预留 10% 空间

# ❌ 避免
capacity:
  storage: 100Gi                  # 磁盘 100GB, PV 100Gi 会导致空间不足
```

### 9.2 回收策略

```yaml
# ✅ 生产环境
persistentVolumeReclaimPolicy: Retain    # 防止数据意外丢失

# ✅ 开发/测试环境
persistentVolumeReclaimPolicy: Delete    # 自动清理节省空间
```

### 9.3 存储类选择

```yaml
# ✅ 显式设置
storageClassName: local-ssd              # 明确分类

# ⚠️ 谨慎使用
storageClassName: ""                     # 仅用于不使用 StorageClass 的场景

# ❌ 避免
# 不设置 storageClassName                # 可能匹配意外的 PVC
```

### 9.4 访问模式

```yaml
# ✅ 根据应用特性选择
# 数据库、单实例应用 → RWO
# 共享文件系统 → RWX
# 静态资源分发 → ROX
# 强一致性数据库(v1.29+) → RWOP

# ❌ 避免
accessModes: [ReadWriteOnce, ReadWriteMany]  # 不要同时声明多个模式
```

### 9.5 监控与告警

```bash
# 关键指标
# 1. PV 容量使用率 (kubelet 报告)
kubectl get --raw /api/v1/nodes/<node>/proxy/stats/summary | jq '.pods[].volume[] | select(.pvcRef.name=="my-pvc") | .usedBytes, .capacityBytes'

# 2. PV 状态
kubectl get pv -o json | jq -r '.items[] | select(.status.phase!="Bound") | .metadata.name + ": " + .status.phase'

# 3. PVC 绑定延迟
kubectl get events --field-selector involvedObject.kind=PersistentVolumeClaim --sort-by='.lastTimestamp'
```

### 9.6 备份策略

```yaml
# 方案 1: VolumeSnapshot (CSI 驱动支持)
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: my-pvc-snapshot
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: my-pvc

# 方案 2: 应用层备份 (通用)
# CronJob 定期执行
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-pv
spec:
  schedule: "0 2 * * *"           # 每天凌晨 2 点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: alpine:3.18
            command:
            - sh
            - -c
            - tar czf /backup/data-$(date +%Y%m%d).tar.gz /data
            volumeMounts:
            - name: data
              mountPath: /data
              readOnly: true
            - name: backup
              mountPath: /backup
          volumes:
          - name: data
            persistentVolumeClaim:
              claimName: my-pvc
          - name: backup
            nfs:
              server: backup.example.com
              path: /backups
          restartPolicy: OnFailure
```

---

## 十、参考资源

### 10.1 官方文档

- [Persistent Volumes](https://kubernetes.io/docs/concepts/storage/persistent-volumes/)
- [Storage Classes](https://kubernetes.io/docs/concepts/storage/storage-classes/)
- [Volume Snapshots](https://kubernetes.io/docs/concepts/storage/volume-snapshots/)

### 10.2 相关 KEP

- [KEP-1412: Immutable Secrets and ConfigMaps](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/1412-immutable-secrets-configmaps)
- [KEP-1432: Volume Health Monitoring](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/1432-volume-health-monitor)
- [KEP-2485: ReadWriteOncePod Access Mode](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/2485-read-write-once-pod-pv-access-mode)

### 10.3 版本差异

| 功能 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|-------|-------|-------|
| ReadWriteOncePod | Alpha | Alpha | Beta | Beta | GA | GA | GA | GA |
| 内置云盘卷源 | ✅ | Deprecated | Deprecated | Deprecated | Deprecated | Deprecated | ❌ | ❌ |
| CSI Migration | Beta | Beta | GA | GA | GA | GA | GA | GA |

---

**文档维护**: 2026-02  
**下一次审阅**: 2026-08 (Kubernetes v1.33 发布后)
