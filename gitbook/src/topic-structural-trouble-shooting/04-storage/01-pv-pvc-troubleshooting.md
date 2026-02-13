# PV/PVC 存储深度排查与持久化治理指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 资深专家级

---

## 0. 读者对象与价值
| 角色 | 目标 | 核心收获 |
| :--- | :--- | :--- |
| **初学者** | 解决 PVC 无法绑定或 Pod 挂载超时的基础问题 | 掌握 PV/PVC 生命周期、StorageClass 配置及常用描述命令。 |
| **中级运维** | 实施存储扩容、快照备份与多节点共享存储 | 理解 CSI 架构、掌握动态供给原理、解决多重挂载（Multi-Attach）冲突。 |
| **资深专家** | 解决大规模集群下的 I/O 瓶颈与数据一致性危机 | 深入底层：Attach/Detach 控制流、Mount 传播机制、块设备层故障诊断与专家级清理技巧。 |

---

## 0. 10 分钟快速诊断

1. **PVC 状态**：`kubectl get pvc -A -o wide`，关注 Pending/Bound；`kubectl describe pvc <name>` 看事件。
2. **PV/SC 对齐**：`kubectl get pv`、`kubectl get sc`，确认 StorageClass、`volumeBindingMode`、`reclaimPolicy` 是否符合预期。
3. **附件状态**：`kubectl get volumeattachment -o wide`，判断 Attach 是否卡住或已挂在旧节点。
4. **节点挂载**：在节点上 `findmnt -t csi`、`ls -l /dev/disk/by-id/ | grep <pv>`，确认设备与挂载存在。
5. **多点挂载**：若 `Multi-Attach`，先确认旧 Pod 是否已删除，必要时清理僵尸附件。
6. **快速缓解**：
   - Pending：检查后端配额/可用区/StorageClass，必要时扩容或调整 Topology。
   - Attach 卡住：谨慎删除 `VolumeAttachment` 并在云控制台确认解绑。
7. **证据留存**：保存 PVC/PV/VA 描述、CSI controller 日志与节点挂载快照。

---

## 1. 核心原理与底层机制

### 1.1 存储控制面与数据面分离
K8s 存储系统遵循高度解耦的 CSI（Container Storage Interface）标准：
- **控制面路径 (Control Plane Path)**：
  - **Provisioning**：`external-provisioner` 监听 PVC，在后端存储创建卷并创建 PV。
  - **Attaching**：`external-attacher` 将卷关联到特定节点。涉及 `VolumeAttachment` 对象。
- **数据面路径 (Data Plane Path)**：
  - **Mounting**：节点上的 `kubelet` 调用 `csi-node` 插件，将块设备格式化并挂载到 Pod 目录。
  - **Mount Propagation**：控制主机与容器间挂载点的可见性（None, HostToContainer, Bidirectional）。

#### 1.1.1 存储架构全景图

```
┌─────────────────────────────────────────────────────────────────┐
│                     Kubernetes Control Plane                     │
│                                                                   │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐           │
│  │ PVC Object  │──>│ PV Object   │──>│VolumeAttach │           │
│  │ (用户请求)  │   │ (存储资源)  │   │ment         │           │
│  └─────────────┘   └─────────────┘   └─────────────┘           │
│         │                  │                  │                  │
│         v                  v                  v                  │
│  ┌──────────────────────────────────────────────────┐          │
│  │          CSI Controller Plugin                    │          │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐   │          │
│  │  │Provisioner │ │  Attacher  │ │  Resizer   │   │          │
│  │  └────────────┘ └────────────┘ └────────────┘   │          │
│  └──────────────────────────────────────────────────┘          │
└─────────────────────────┬───────────────────────────────────────┘
                          │ gRPC Calls
                          v
┌─────────────────────────────────────────────────────────────────┐
│                      Storage Backend                             │
│         (云盘/NFS/Ceph/本地盘)                                   │
└─────────────────────────┬───────────────────────────────────────┘
                          │ Attach (关联)
                          v
┌─────────────────────────────────────────────────────────────────┐
│                        Worker Node                               │
│                                                                   │
│  ┌──────────────────────────────────────────────────┐          │
│  │             Kubelet                               │          │
│  │   ├─> Volume Manager (监控 Pod 卷需求)           │          │
│  │   ├─> Attach/Detach Controller (本地控制)       │          │
│  │   └─> Mount/Unmount (挂载到 Pod 目录)           │          │
│  └──────────────────────────────────────────────────┘          │
│                          │                                       │
│                          v                                       │
│  ┌──────────────────────────────────────────────────┐          │
│  │          CSI Node Plugin                          │          │
│  │   ├─> NodeStageVolume (格式化/全局挂载)         │          │
│  │   ├─> NodePublishVolume (挂载到 Pod 目录)       │          │
│  │   └─> NodeUnpublishVolume (卸载)                │          │
│  └──────────────────────────────────────────────────┘          │
│                          │                                       │
│                          v                                       │
│  ┌──────────────────────────────────────────────────┐          │
│  │      /var/lib/kubelet/pods/<uid>/volumes/        │          │
│  │          kubernetes.io~csi/<pv-name>/mount       │          │
│  └──────────────────────────────────────────────────┘          │
│                          │                                       │
│                          v                                       │
│  ┌──────────────────────────────────────────────────┐          │
│  │              Pod Container                        │          │
│  │         (挂载点: /data)                           │          │
│  └──────────────────────────────────────────────────┘          │
└─────────────────────────────────────────────────────────────────┘
```

---

#### 1.1.2 PV/PVC 生命周期状态机详解

**1. Provisioning（供应阶段）**

```
用户创建 PVC
  └─> API Server 接收请求
       └─> PV Controller 监听到 PVC
            ├─ 静态供应：匹配已有 PV（根据 capacity/storageClass/accessMode）
            │   └─> 绑定 PVC → PV (状态: Bound)
            │
            └─ 动态供应：调用 CSI Provisioner
                 └─> CreateVolume(size, parameters)
                      └─> 后端存储创建卷
                           └─> 创建 PV 对象
                                └─> 绑定 PVC → PV
```

**关键参数对比**

| 参数 | 说明 | 静态供应 | 动态供应 |
|-----|------|---------|---------|
| **storageClassName** | 存储类名称 | 可选（留空匹配默认 PV） | **必须**（指定 StorageClass） |
| **volumeName** | 指定 PV 名称 | 可用于强制绑定 | 自动生成 |
| **volumeBindingMode** | 绑定时机 | Immediate（立即绑定） | WaitForFirstConsumer（延迟绑定） |

**2. Binding（绑定阶段）**

```bash
# 查看绑定状态
kubectl get pvc,pv
# NAME                     STATUS   VOLUME    CAPACITY   ACCESS MODES   STORAGECLASS
# persistentvolumeclaim/data-pvc   Bound    pv-12345  10Gi       RWO            fast-ssd

# 查看绑定细节
kubectl get pvc data-pvc -o jsonpath='{.spec.volumeName}'  # 绑定的 PV 名称
kubectl get pv pv-12345 -o jsonpath='{.spec.claimRef}'     # PV 的所有者
```

**3. Attaching（附着阶段）**

```
Pod 调度到节点 node-1
  └─> Attach/Detach Controller 检测到 Pod 使用 PV
       └─> 创建 VolumeAttachment 对象
            └─> CSI Attacher 监听到 VolumeAttachment
                 └─> 调用 ControllerPublishVolume(volumeId, nodeId)
                      └─> 后端存储将卷关联到节点（如云盘挂载到 ECS）
                           └─> VolumeAttachment.status.attached = true
```

**VolumeAttachment 状态检查**

```bash
# 查看所有附着记录
kubectl get volumeattachment -o wide
# NAME                                   ATTACHER        PV        NODE      ATTACHED
# csi-123abc...                          csi.example.com pv-12345  node-1    true

# 详细分析附着状态
kubectl get volumeattachment csi-123abc -o yaml
# status:
#   attached: true  # ← 关键字段
#   attachmentMetadata:
#     devicePath: "/dev/vdb"  # 节点上的设备路径
```

**4. Mounting（挂载阶段）**

```
节点上的 kubelet 检测到 VolumeAttachment.attached = true
  └─> Volume Manager 开始挂载流程
       ├─ NodeStageVolume (全局挂载)
       │   └─> 格式化块设备（如 mkfs.ext4）
       │        └─> 挂载到全局目录：/var/lib/kubelet/plugins/.../globalmount/
       │
       └─ NodePublishVolume (Pod 级挂载)
            └─> Bind Mount 到 Pod 目录：/var/lib/kubelet/pods/<uid>/volumes/.../mount
                 └─> 容器启动，挂载点传播到容器内
```

**挂载点层级关系**

```bash
# 节点上查看挂载链
findmnt -t ext4 | grep pv-12345
# TARGET                                SOURCE      FSTYPE
# /var/lib/kubelet/plugins/.../globalmount  /dev/vdb   ext4  ← 全局挂载
# /var/lib/kubelet/pods/.../volumes/.../mount  /dev/vdb   ext4  ← Pod 挂载（bind）
# /var/lib/docker/containers/.../mounts/data  /dev/vdb   ext4  ← 容器挂载（bind）
```

**5. Using（使用阶段）**

```bash
# 监控卷使用情况
kubectl get --raw /api/v1/nodes/<node>/proxy/stats/summary | jq '.pods[].volume[] | select(.name=="<pv-name>")'
# {
#   "name": "pv-12345",
#   "usedBytes": 1073741824,  # 已用 1GB
#   "capacityBytes": 10737418240,  # 总容量 10GB
#   "availableBytes": 9663676416,
#   "inodesFree": 655350,
#   "inodes": 655360
# }

# 设置告警阈值（Prometheus）
(kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes) > 0.8
```

**6. Reclaiming（回收阶段）**

```
用户删除 PVC
  └─> PV 根据 persistentVolumeReclaimPolicy 处理
       ├─ Retain（保留）
       │   └─> PV 状态变为 Released
       │        └─> 需手动删除 claimRef 才能重新绑定
       │
       ├─ Delete（删除）
       │   └─> CSI Provisioner 调用 DeleteVolume()
       │        └─> 后端存储删除卷
       │             └─> PV 对象被删除
       │
       └─ Recycle（已废弃）
            └─> 运行 `rm -rf /volume/*` 清理数据
```

**Retain 策略手动回收流程**

```bash
# 1. PVC 删除后，PV 状态变为 Released
kubectl get pv pv-12345
# NAME       STATUS     CLAIM            CAPACITY   RECLAIM POLICY
# pv-12345   Released   default/data-pvc 10Gi       Retain

# 2. 清理 claimRef 使 PV 可重新绑定
kubectl patch pv pv-12345 -p '{"spec":{"claimRef": null}}'

# 3. PV 状态恢复为 Available
kubectl get pv pv-12345
# NAME       STATUS      CLAIM   CAPACITY   RECLAIM POLICY
# pv-12345   Available   <none>  10Gi       Retain
```

---

### 1.2 生命周期详解

- **Binding**：PVC 匹配 PV 的过程。如果 `volumeBindingMode` 为 `WaitForFirstConsumer`，则绑定会推迟到 Pod 调度成功后。
- **Reclaiming**：当 PVC 删除时，PV 的处理方式（Retain: 保留数据；Delete: 连同后端卷一起删除）。

#### 1.2.1 WaitForFirstConsumer 延迟绑定机制

**工作原理**

```
用户创建 PVC (storageClass: fast-ssd, volumeBindingMode: WaitForFirstConsumer)
  └─> PVC 状态: Pending（不立即绑定）
       └─> 用户创建 Pod 使用该 PVC
            └─> 调度器选择节点 node-1（考虑拓扑约束）
                 └─> 调度器更新 PVC 的 annotations:
                      volume.kubernetes.io/selected-node: node-1
                      └─> PV Controller/Provisioner 检测到 annotation
                           └─> 在 node-1 所在可用区创建卷
                                └─> 绑定 PVC → PV
                                     └─> Pod 继续启动
```

**适用场景**

| 存储类型 | 是否需要 WaitForFirstConsumer | 原因 |
|---------|----------------------------|------|
| **云盘** | ✅ **强烈推荐** | 避免跨可用区挂载（网络延迟高/不支持） |
| **NFS/Ceph** | ❌ 不需要 | 网络存储，所有节点均可访问 |
| **本地盘** | ✅ **必须** | 只能在特定节点访问 |

**配置示例**

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
# 延迟绑定：等待 Pod 调度后再创建卷
volumeBindingMode: WaitForFirstConsumer
# 拓扑约束：限制卷创建在特定可用区
allowedTopologies:
- matchLabelExpressions:
  - key: topology.ebs.csi.aws.com/zone
    values:
    - us-west-2a
    - us-west-2b
```

**调试延迟绑定问题**

```bash
# 1. PVC 长时间 Pending
kubectl describe pvc data-pvc
# Events:
#   Type    Reason                Age   From                         Message
#   ----    ------                ----  ----                         -------
#   Normal  WaitForFirstConsumer  30s   persistentvolume-controller  
#           waiting for first consumer to be created before binding

# 2. Pod 调度失败
kubectl describe pod app-pod
# Events:
#   Type     Reason            Age   Message
#   Warning  FailedScheduling  1m    0/3 nodes are available: 
#            3 node(s) had volume node affinity conflict.
# ❌ 所有节点不在允许的可用区

# 3. 检查 Pod 的拓扑需求
kubectl get pod app-pod -o jsonpath='{.spec.nodeSelector}'
# {"topology.kubernetes.io/zone":"us-west-2c"}  # ← 与 allowedTopologies 不匹配

# 4. 解决方案：调整 Pod nodeSelector 或扩展 allowedTopologies
kubectl patch sc fast-ssd --type=json -p='[
  {"op":"add","path":"/allowedTopologies/0/matchLabelExpressions/0/values/-","value":"us-west-2c"}
]'
```

---

#### 1.2.2 Access Mode 深度解析

| 模式 | 缩写 | 语义 | 典型场景 | 存储支持 |
|-----|------|------|---------|---------|
| **ReadWriteOnce** | RWO | 单节点读写 | 数据库、单实例应用 | 云盘、本地盘 |
| **ReadOnlyMany** | ROX | 多节点只读 | 静态资源、共享配置 | NFS、Ceph |
| **ReadWriteMany** | RWX | 多节点读写 | 共享文件系统 | NFS、CephFS、GlusterFS |
| **ReadWriteOncePod** (v1.27+) | RWOP | 单 Pod 独占 | 防止多 Pod 冲突 | CSI 驱动需支持 |

**Multi-Attach 错误详解**

```
场景：
  节点 A 上 Pod-1 使用 RWO 云盘 vol-abc123
    └─> 节点 A 宕机，Pod-1 状态卡在 Terminating
         └─> 调度器将 Pod-1 重新调度到节点 B
              └─> 节点 B 尝试 Attach vol-abc123
                   └─> 后端存储拒绝：
                        ❌ "Volume is already attached to node-A"
```

**错误信息示例**

```bash
kubectl describe pod app-pod
# Events:
#   Type     Reason              Age   Message
#   Warning  FailedAttachVolume  2m    Multi-Attach error for volume "pv-abc123" 
#            Volume is already attached by node "node-a"

# 查看 VolumeAttachment 状态
kubectl get volumeattachment
# NAME                        ATTACHER        PV         NODE    ATTACHED
# csi-old-attach-node-a       csi.aws.com     pv-abc123  node-a  true   ← 旧附着未清理
# csi-new-attach-node-b       csi.aws.com     pv-abc123  node-b  false  ← 新附着失败
```

**解决步骤**

```bash
# 1. 确认旧节点状态
kubectl get node node-a
# NAME     STATUS     ROLES    AGE   VERSION
# node-a   NotReady   <none>   10d   v1.28.0  # ← 节点故障

# 2. 强制删除旧 Pod（触发卸载流程）
kubectl delete pod app-pod-old --force --grace-period=0

# 3. 等待旧 VolumeAttachment 自动清理（通常 6 分钟）
# 或手动删除（谨慎操作）
kubectl delete volumeattachment csi-old-attach-node-a

# 4. 在云控制台确认卷已从 node-a 解绑
aws ec2 describe-volumes --volume-ids vol-abc123
# "Attachments": []  # ✅ 无附着记录

# 5. 新 Pod 重新调度后成功挂载
kubectl get pod app-pod-new
# NAME           READY   STATUS    RESTARTS   AGE
# app-pod-new    1/1     Running   0          30s
```

---

#### 1.2.3 Volume Expansion（在线扩容）机制

**扩容流程**

```
用户修改 PVC.spec.resources.requests.storage (1Gi → 10Gi)
  └─> PVC Controller 检测到扩容请求
       └─> 调用 CSI Resizer
            └─> ControllerExpandVolume(volumeId, newSize)
                 └─> 后端存储扩展卷容量
                      └─> 更新 PV.spec.capacity (10Gi)
                           └─> kubelet 检测到容量变化
                                ├─ 块设备扩容：parted resize
                                └─ 文件系统扩容：
                                     ├─ ext4: resize2fs
                                     └─ xfs: xfs_growfs
```

**前置条件检查**

```bash
# 1. StorageClass 必须允许扩容
kubectl get sc fast-ssd -o jsonpath='{.allowVolumeExpansion}'
# true  # ✅ 支持扩容

# 2. CSI Driver 必须支持扩容能力
kubectl get csidriver ebs.csi.aws.com -o jsonpath='{.spec.volumeExpansion}'
# true  # ✅ 驱动支持

# 3. PVC 必须处于 Bound 状态
kubectl get pvc data-pvc -o jsonpath='{.status.phase}'
# Bound  # ✅ 已绑定
```

**扩容操作**

```bash
# 1. 修改 PVC 容量（声明式）
kubectl patch pvc data-pvc -p '{"spec":{"resources":{"requests":{"storage":"10Gi"}}}}'

# 2. 监控扩容进度
kubectl describe pvc data-pvc
# Conditions:
#   Type                  Status  LastTransitionTime  Reason
#   ----                  ------  ------------------  ------
#   FileSystemResizePending  True   2024-01-01 10:00:00  # ← 等待文件系统扩容
#   Resizing                True   2024-01-01 10:00:05  # ← 正在扩容

# 3. 查看 PV 容量是否已更新
kubectl get pv pv-12345 -o jsonpath='{.spec.capacity.storage}'
# 10Gi  # ✅ PV 容量已更新

# 4. 验证 Pod 内文件系统大小
kubectl exec app-pod -- df -h /data
# Filesystem      Size  Used Avail Use% Mounted on
# /dev/vdb        9.8G  100M  9.2G   2% /data  # ✅ 文件系统已扩容
```

**扩容失败排查**

```bash
# 场景：PV 容量已更新，但 Pod 内容量未变

# 1. 检查文件系统类型
kubectl exec app-pod -- mount | grep /data
# /dev/vdb on /data type xfs (rw,relatime)

# 2. 查看 kubelet 日志
journalctl -u kubelet | grep -i "expand\|resize"
# Jan 01 10:05:00 kubelet[1234]: NodeExpandVolume failed for volume "pv-12345": 
#   xfs_growfs command not found
# ❌ 节点缺少 xfsprogs 工具

# 3. 安装文件系统工具
apt-get install -y xfsprogs  # Debian/Ubuntu
yum install -y xfsprogs      # CentOS/RHEL

# 4. 重启 Pod 触发重新挂载（kubelet 会自动扩容）
kubectl rollout restart deployment app

# 5. 验证扩容成功
kubectl exec app-pod-new -- df -h /data
# Filesystem      Size  Used Avail Use% Mounted on
# /dev/vdb        9.8G  100M  9.2G   2% /data  # ✅ 扩容成功
```

---

## 2. 专家级故障矩阵与观测工具

### 2.1 专家级故障矩阵

| 现象分类 | 深度根因分析 | 关键观测对象/日志 |
| :--- | :--- | :--- |
| **PVC 永久 Pending** | 调度器无法满足存储亲和性（如跨 AZ）、SC 绑定的 Provisioner 不存在、云平台磁盘配额耗尽。 | `kubectl describe pvc`, `csi-provisioner` 日志 |
| **Multi-Attach Error** | 云盘通常只支持 RWO，旧 Pod 尚未完全 Detach（僵尸连接），新 Pod 尝试在另一节点 Attach。 | `kubectl get volumeattachment` |
| **Filesystem Resize 失败** | 块设备已扩容但文件系统未能在线扩展（多见于 XFS/EXT4 工具链缺失或内核限制）。 | `csi-resizer` 日志, 节点 `dmesg` |
| **Pod 卡在 Terminating** | 存储层挂死导致卸载失败，kubelet 无法删除 Pod 目录，进而阻塞容器删除。 | `kubelet` 日志, `grep "MountVolume.TearDown" /var/log/syslog` |

### 2.2 专家工具箱

```bash
# 1. 追踪卷在节点上的真实状态 (通过容器查看)
kubectl run debug --rm -it --privileged --image=busybox -- nsenter -t 1 -m -- lsblk

# 2. 检查僵尸 VolumeAttachment
kubectl get volumeattachment | grep <pv-name> | grep false  # 查找状态非 Ready 的附件

# 3. 查看 CSI 侧控制器详细交互 (外部 sidecars)
kubectl logs -n kube-system <csi-controller-pod> -c csi-attacher
kubectl logs -n kube-system <csi-controller-pod> -c csi-provisioner

# 4. 在节点上调试挂载点 (Findmnt 专家用法)
findmnt -lo source,target,fstype,label,options -t csi
```

---

## 3. 深度排查路径

### 3.1 第一阶段：生命周期状态验证
确认故障点是在"创建"、"关联"还是"挂载"阶段。

```bash
# 检查 PV 是否成功创建 (控制面第 1 步)
kubectl get pv -l kubernetes.io/created-by=external-provisioner

# 检查卷附件 (控制面第 2 步)
# 只要 Programmed 状态为 True，说明云平台已完成 Attach
kubectl get volumeattachment -o custom-columns=NAME:.metadata.name,ATTACHED:.status.attached,NODE:.spec.nodeName
```

### 3.2 第二阶段：节点侧 IO 链路分析
如果控制面正常但 Pod 无法启动，需进入节点检查。

```bash
# 检查块设备是否已识别
ls -l /dev/disk/by-id/ | grep <pv-handle>

# 检查 kubelet 卷目录
ls -R /var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~csi/<pv-name>/
```

---

## 4. 深度解决方案与生产最佳实践

### 4.1 解决 Multi-Attach 锁死问题
**策略**：强制清理。
1. **驱逐旧节点**：确保原节点上的 Pod 确实已销毁。
2. **清理 VolumeAttachment**：
   ```bash
   kubectl delete volumeattachment <va-name> --force --grace-period=0
   ```
   *注意：这只是删除 K8s 对象，云平台层面的解绑需在云控制台确认。*

### 4.2 应对"WaitForFirstConsumer"引发的调度困局
**现象**：Pod 无法调度，提示 `node(s) had volume node affinity conflict`。
**原因**：PVC 在第一个 Pod 调度后固定在 AZ-A，后续同一 PVC 的 Pod 如果必须在 AZ-B 运行，则产生冲突。
**对策**：检查 StorageClass 的 `allowedTopologies` 限制，确保存储与计算资源在同一可用区。

### 4.3 存储扩容失败的在线修复 (Online Resizing)
1. 确认 SC 开启了 `allowVolumeExpansion: true`。
2. 修改 PVC 后的操作：
   - 如果块设备扩容成功但文件系统没动，需手动在节点执行：
     ```bash
     resize2fs /dev/vdb  # 对于 ext4
     xfs_growfs /mount/point # 对于 xfs
     ```

---

## 5. 生产环境典型案例解析

### 5.1 案例一：StatefulSet 扩容后，所有 Pod 无法启动
- **根因分析**：存储类后端为局部盘或特定 AZ，扩容时由于剩余空间不足或资源耗尽，导致新 PV 无法 Provision。
- **对策**：监控云厂商配额，并使用跨 AZ 复制能力的存储服务。

### 5.2 案例二：节点宕机后，Pod 在新节点挂载卷时报错"Device busy"
- **根因分析**：后端存储认为卷仍挂载在已宕机节点的后端，尚未触发超时解绑。
- **对策**：配置云插件的 `NodeUnstage` 策略，或手动在云控制台卸载。

---

### 5.3 案例三：Multi-Attach 错误导致 StatefulSet 滚动更新失败

**故障现场**

- **现象**：数据库 StatefulSet 滚动更新时，新 Pod 卡在 ContainerCreating 状态超过 30 分钟
- **影响范围**：3 个 MySQL 实例（主从架构），主库 Pod 无法更新
- **业务影响**：
  - 主库运行旧版本（存在已知 Bug）
  - 无法应用安全补丁
  - 自动扩容被阻塞

**排查过程**

```bash
# 1. 检查 Pod 状态
kubectl get pods -l app=mysql
# NAME      READY   STATUS              RESTARTS   AGE
# mysql-0   1/1     Running             0          10d  # 主库（旧版本）
# mysql-1   1/1     Running             0          1h   # 从库（已更新）
# mysql-2   1/1     Running             0          1h   # 从库（已更新）

# 查看详细信息
kubectl describe pod mysql-0
# Events:
#   Type     Reason              Age                  Message
#   Warning  FailedAttachVolume  35m (x15 over 35m)   
#            Multi-Attach error for volume "pvc-mysql-mysql-0" 
#            Volume is already attached by node "node-old-123"

# 2. 检查 VolumeAttachment
kubectl get volumeattachment | grep pvc-mysql-mysql-0
# NAME                                  ATTACHER        PV              NODE           ATTACHED
# csi-attach-old-node-123               ebs.csi.aws.com pvc-mysql-0     node-old-123   true   ← 旧附着
# csi-attach-new-node-456               ebs.csi.aws.com pvc-mysql-0     node-456       false  ← 新附着失败

# 3. 检查旧节点状态
kubectl get node node-old-123
# NAME            STATUS     ROLES    AGE   VERSION
# node-old-123    NotReady   <none>   30d   v1.28.0  # ← 节点已下线

# 4. 检查旧 Pod 是否仍存在
kubectl get pod mysql-0-old --show-labels
# NAME         READY   STATUS        RESTARTS   AGE   LABELS
# mysql-0-old  1/1     Terminating   0          35m   controller-revision-hash=mysql-old
# ❌ Pod 卡在 Terminating 状态

# 5. 查看 Pod 终止日志
kubectl describe pod mysql-0-old
# Events:
#   Type    Reason        Age   Message
#   Normal  Killing       35m   Stopping container mysql
#   Warning UnmountFailed 34m   Failed to unmount volume: 
#           rpc error: code = DeadlineExceeded desc = context deadline exceeded
# ❌ 卸载超时（节点不可达，kubelet 无法调用 CSI NodeUnpublishVolume）

# 6. 检查云端卷附着状态
aws ec2 describe-volumes --volume-ids vol-0abc123
# "Attachments": [{
#     "Device": "/dev/xvdf",
#     "InstanceId": "i-old-123",  # ← 仍附着在旧节点
#     "State": "attached"
# }]

# 7. 根因分析：
# - 滚动更新时，旧 Pod 被删除但节点故障导致卸载失败
# - VolumeAttachment 未清理，云盘仍附着在旧节点
# - 新 Pod 调度到新节点，尝试附着同一云盘时被拒绝（RWO 限制）
# - kubelet 默认等待 6 分钟后强制删除 Pod，但云端卸载未触发
```

**应急措施**

```bash
# 方案 A：清理僵尸 VolumeAttachment（推荐）

# 1. 强制删除旧 Pod（触发删除流程）
kubectl delete pod mysql-0-old --force --grace-period=0
# Warning: Immediate deletion does not wait for confirmation that 
#          the running resource has been terminated.
# pod "mysql-0-old" force deleted

# 2. 等待 30 秒，检查 VolumeAttachment 是否自动清理
kubectl get volumeattachment | grep node-old-123
# （仍存在）

# 3. 手动删除僵尸 VolumeAttachment
kubectl delete volumeattachment csi-attach-old-node-123
# volumeattachment.storage.k8s.io "csi-attach-old-node-123" deleted

# 4. 验证新附着是否成功
kubectl get volumeattachment | grep pvc-mysql-mysql-0
# NAME                        ATTACHER        PV          NODE      ATTACHED
# csi-attach-new-node-456     ebs.csi.aws.com pvc-mysql-0 node-456  true  ✅

# 5. 验证 Pod 启动成功
kubectl get pod mysql-0
# NAME      READY   STATUS    RESTARTS   AGE
# mysql-0   1/1     Running   0          2m

# 方案 B：云控制台手动解绑（VolumeAttachment 删除失败时）

# 1. 在 AWS 控制台找到卷 vol-0abc123
# 2. 点击 "Actions" → "Detach Volume"
# 3. 确认解绑（忽略实例不可达警告）

# 4. 删除 K8s 中的 VolumeAttachment 对象
kubectl delete volumeattachment csi-attach-old-node-123 --force

# 5. 验证新 Pod 挂载成功
kubectl exec mysql-0 -- df -h /var/lib/mysql
# Filesystem      Size  Used Avail Use% Mounted on
# /dev/xvdf       100G   20G   80G  20% /var/lib/mysql  ✅
```

**长期优化**

```yaml
# 1. 配置节点故障快速驱逐策略
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-controller-manager
  namespace: kube-system
data:
  kube-controller-manager.yaml: |
    apiVersion: kubeadm.k8s.io/v1beta3
    kind: ClusterConfiguration
    controllerManager:
      extraArgs:
        # 节点心跳超时时间（默认 40s）
        node-monitor-grace-period: "40s"
        # Pod 驱逐超时时间（默认 5m）
        pod-eviction-timeout: "1m"  # ← 缩短至 1 分钟
        # VolumeAttachment 超时时间
        attach-detach-reconcile-sync-period: "30s"

# 2. 启用 CSI 卷附着保护机制
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: ebs.csi.aws.com
spec:
  # 附着时自动添加 Finalizer 保护
  attachRequired: true
  podInfoOnMount: true
  # 卷生命周期模式：持久化（Persistent）
  volumeLifecycleModes:
  - Persistent
  # 启用节点故障快速卸载
  storageCapacity: true

# 3. 配置 StatefulSet 滚动更新策略
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  # 滚动更新配置
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      # 最大不可用 Pod 数量（默认 1）
      maxUnavailable: 1
      # 分区更新：仅更新序号 >= partition 的 Pod
      partition: 0
  # Pod 管理策略
  podManagementPolicy: OrderedReady  # 顺序启动/停止
  # PVC 模板
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
  template:
    spec:
      # 优雅停机配置
      terminationGracePeriodSeconds: 60  # 给予 60 秒清理时间
      containers:
      - name: mysql
        image: mysql:8.0
        # PreStop Hook：确保数据刷盘
        lifecycle:
          preStop:
            exec:
              command:
              - /bin/sh
              - -c
              - |
                # 停止接收新连接
                mysql -uroot -p$MYSQL_ROOT_PASSWORD -e "SET GLOBAL read_only = ON;"
                # 等待现有事务完成
                sleep 10
                # 刷新 binlog
                mysql -uroot -p$MYSQL_ROOT_PASSWORD -e "FLUSH LOGS;"
                # 等待 IO 完成
                sync
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql

# 4. 监控 VolumeAttachment 异常
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: volume-attachment-alert
  namespace: kube-system
spec:
  groups:
  - name: storage
    interval: 30s
    rules:
    # VolumeAttachment 长时间未 attached
    - alert: VolumeAttachmentStuck
      expr: |
        (time() - kube_volumeattachment_created) > 300
        and kube_volumeattachment_status_attached == 0
      labels:
        severity: warning
      annotations:
        summary: "VolumeAttachment {{ $labels.volumeattachment }} 超过 5 分钟未附着"
        description: "检查节点状态和云端卷附着记录"
    
    # Pod 卡在 Terminating
    - alert: PodStuckTerminating
      expr: |
        kube_pod_deletion_timestamp > 0
        and (time() - kube_pod_deletion_timestamp) > 600
      labels:
        severity: critical
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 超过 10 分钟未删除"
        description: "可能存在卷卸载问题，检查 VolumeAttachment"

# 5. 定期清理僵尸 VolumeAttachment（CronJob）
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cleanup-stale-va
  namespace: kube-system
spec:
  schedule: "*/10 * * * *"  # 每 10 分钟执行
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: va-cleanup
          containers:
          - name: cleanup
            image: bitnami/kubectl:latest
            command:
            - /bin/bash
            - -c
            - |
              # 查找附着在 NotReady 节点的 VolumeAttachment
              for va in $(kubectl get volumeattachment -o json | \
                jq -r '.items[] | select(.status.attached==false and 
                       (.metadata.creationTimestamp | fromdateiso8601) < (now - 600)) | 
                       .metadata.name'); do
                
                node=$(kubectl get volumeattachment $va -o jsonpath='{.spec.nodeName}')
                node_status=$(kubectl get node $node -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}' 2>/dev/null || echo "NotFound")
                
                if [[ "$node_status" != "True" ]]; then
                  echo "Deleting stale VolumeAttachment: $va (node: $node, status: $node_status)"
                  kubectl delete volumeattachment $va --timeout=30s || true
                fi
              done
          restartPolicy: OnFailure
---
# RBAC 权限
apiVersion: v1
kind: ServiceAccount
metadata:
  name: va-cleanup
  namespace: kube-system
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: va-cleanup
rules:
- apiGroups: ["storage.k8s.io"]
  resources: ["volumeattachments"]
  verbs: ["get", "list", "delete"]
- apiGroups: [""]
  resources: ["nodes"]
  verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: va-cleanup
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: va-cleanup
subjects:
- kind: ServiceAccount
  name: va-cleanup
  namespace: kube-system
```

**效果评估**

| 指标 | 优化前 | 优化后 | 改善 |
|-----|--------|--------|------|
| Pod 驱逐超时 | 5 分钟 | 1 分钟 | ↓ 80% |
| VolumeAttachment 清理 | 手动清理 | 自动清理（10 分钟） | ✅ 自动化 |
| 滚动更新恢复时间 | 35 分钟 | 5 分钟 | ↓ 86% |
| Multi-Attach 错误率 | 15%/月 | <1%/月 | ↓ 93% |

---

### 5.4 案例四：PVC 扩容后文件系统容量未变导致应用 OOM

**故障现场**

- **现象**：业务 Pod 频繁 OOMKilled，但内存使用正常（<500MB）
- **影响范围**：日志收集服务（50 个 Pod）
- **业务影响**：
  - 日志丢失率 30%
  - 磁盘写入失败（No space left on device）
  - 数据库慢查询日志缺失

**排查过程**

```bash
# 1. 检查 Pod 资源使用
kubectl top pod log-collector-abc123
# NAME                     CPU   MEMORY
# log-collector-abc123     50m   480Mi  # 内存使用正常

# 2. 查看 Pod 日志
kubectl logs log-collector-abc123 | tail -20
# [ERROR] Failed to write log file: write /data/logs/app.log: no space left on device
# ❌ 磁盘空间不足

# 3. 检查 Pod 内磁盘使用
kubectl exec log-collector-abc123 -- df -h /data
# Filesystem      Size  Used Avail Use% Mounted on
# /dev/vdc        10G   10G    0G  100% /data  # ← 磁盘满了

# 4. 检查 PVC 容量
kubectl get pvc log-data -o jsonpath='{.spec.resources.requests.storage}'
# 50Gi  # PVC 声明 50GB

kubectl get pvc log-data -o jsonpath='{.status.capacity.storage}'
# 50Gi  # PVC 状态也是 50GB

# 5. 检查 PV 容量
kubectl get pv pvc-log-data -o jsonpath='{.spec.capacity.storage}'
# 50Gi  # PV 容量 50GB

# 6. 节点上检查块设备大小
kubectl debug node/worker-1 -it --image=busybox
# 在调试容器内：
chroot /host
lsblk | grep vdc
# vdc    253:32   0   50G  0 disk  # ← 块设备已扩容到 50GB

# 7. 检查文件系统大小
df -h | grep vdc
# /dev/vdc        10G   10G    0G  100% /var/lib/kubelet/pods/.../volumes/.../mount
# ❌ 文件系统仍为 10GB（未扩展）

# 8. 查看扩容历史
kubectl describe pvc log-data
# Conditions:
#   Type                     Status  LastTransitionTime  Reason
#   ----                     ------  ------------------  ------
#   FileSystemResizePending  True    2024-01-01 09:00:00 WaitingForNodeResize
# ❌ 文件系统扩容被阻塞

# 9. 查看 kubelet 日志
ssh worker-1
journalctl -u kubelet | grep -i "expand\|resize" | tail -20
# Jan 01 09:00:05 kubelet[1234]: E0101 09:00:05.123456 volume_manager.go:456] 
#   NodeExpandVolume failed for volume "pvc-log-data": 
#   rpc error: code = Internal desc = Could not resize volume: filesystem busy
# ❌ 文件系统繁忙，无法在线扩容

# 10. 根因分析：
# - PVC 从 10Gi 扩容到 50Gi，后端云盘已扩容
# - kubelet 尝试在线扩展文件系统（resize2fs）
# - 但由于文件系统高负载（大量 IO），在线扩容失败
# - XFS 文件系统在某些内核版本不支持在线收缩（仅支持扩展）
# - 扩容操作卡在 FileSystemResizePending 状态
```

**应急措施**

```bash
# 方案 A：重启 Pod 触发离线扩容（推荐）

# 1. 临时清理磁盘空间（紧急）
kubectl exec log-collector-abc123 -- sh -c "
  find /data/logs -type f -mtime +7 -delete  # 删除 7 天前的日志
  du -sh /data/logs
"
# 1.5G    /data/logs  # 释放 8.5GB 空间

# 2. 滚动重启 Deployment（触发离线扩容）
kubectl rollout restart deployment log-collector

# 3. 等待新 Pod 启动
kubectl get pods -l app=log-collector -w
# NAME                        READY   STATUS    AGE
# log-collector-new-xyz456    1/1     Running   30s  ✅

# 4. 验证新 Pod 文件系统容量
kubectl exec log-collector-new-xyz456 -- df -h /data
# Filesystem      Size  Used Avail Use% Mounted on
# /dev/vdc        49G   1.5G   45G   4% /data  # ✅ 文件系统已扩容至 50GB

# 5. 验证 PVC 状态
kubectl describe pvc log-data | grep "File.*Resize"
# （无 FileSystemResizePending 条件）✅ 扩容完成

# 方案 B：节点侧手动扩容（Pod 无法重启时）

# 1. SSH 到节点
ssh worker-1

# 2. 找到挂载点
mount | grep pvc-log-data
# /dev/vdc on /var/lib/kubelet/pods/<uid>/volumes/kubernetes.io~csi/pvc-log-data/mount type ext4

# 3. 停止 Pod（减少 IO）
kubectl delete pod log-collector-abc123 --grace-period=60

# 4. 等待 Pod 完全停止（确认卸载）
while mount | grep -q pvc-log-data; do
  echo "Waiting for unmount..."
  sleep 5
done

# 5. 手动扩展文件系统
resize2fs /dev/vdc  # ext4
# 或
xfs_growfs /var/lib/kubelet/pods/.../mount  # xfs

# 6. 验证扩容
df -h | grep vdc
# /dev/vdc        49G   1.5G   45G   4% ...  ✅ 扩容成功

# 7. 新 Pod 自动调度并挂载
kubectl get pod -l app=log-collector
# NAME                        READY   STATUS    AGE
# log-collector-new-xyz456    1/1     Running   10s  ✅
```

**长期优化**

```yaml
# 1. 预防性扩容策略（基于监控告警）
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pvc-usage-alert
  namespace: default
spec:
  groups:
  - name: storage-usage
    interval: 1m
    rules:
    # PVC 使用率 >70% 预警
    - alert: PVC_HighUsage
      expr: |
        (kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes) > 0.7
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "PVC {{ $labels.persistentvolumeclaim }} 使用率 >70%（{{ $value | humanizePercentage }}）"
        description: "建议提前扩容，避免磁盘满导致应用故障"
    
    # PVC 使用率 >90% 紧急告警
    - alert: PVC_CriticalUsage
      expr: |
        (kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes) > 0.9
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "PVC {{ $labels.persistentvolumeclaim }} 使用率 >90%（{{ $value | humanizePercentage }}）"
        description: "立即扩容！磁盘即将满"

# 2. 自动扩容脚本（Kubernetes Operator 或 CronJob）
apiVersion: batch/v1
kind: CronJob
metadata:
  name: auto-expand-pvc
  namespace: default
spec:
  schedule: "*/30 * * * *"  # 每 30 分钟检查
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: pvc-expander
          containers:
          - name: expander
            image: bitnami/kubectl:latest
            command:
            - /bin/bash
            - -c
            - |
              # 查询使用率 >80% 的 PVC
              kubectl get pvc --all-namespaces -o json | \
              jq -r '.items[] | 
                select(.status.capacity.storage != null) | 
                "\(.metadata.namespace)/\(.metadata.name) \(.spec.storageClassName)"' | \
              while read ns_pvc sc; do
                ns=$(echo $ns_pvc | cut -d/ -f1)
                pvc=$(echo $ns_pvc | cut -d/ -f2)
                
                # 获取使用率
                used=$(kubectl get --raw "/api/v1/nodes/<node>/proxy/stats/summary" | \
                       jq -r ".pods[].volume[] | select(.pvcRef.name==\"$pvc\") | .usedBytes" 2>/dev/null || echo "0")
                capacity=$(kubectl get pvc -n $ns $pvc -o jsonpath='{.status.capacity.storage}' | \
                           numfmt --from=iec 2>/dev/null || echo "0")
                
                if [[ $capacity -gt 0 ]]; then
                  usage=$((100 * used / capacity))
                  
                  if [[ $usage -gt 80 ]]; then
                    echo "[$(date)] PVC $ns/$pvc usage: ${usage}%, expanding..."
                    
                    # 扩容 20%
                    new_size=$((capacity * 120 / 100))
                    new_size_gb=$((new_size / 1024 / 1024 / 1024))Gi
                    
                    kubectl patch pvc -n $ns $pvc -p "{\"spec\":{\"resources\":{\"requests\":{\"storage\":\"$new_size_gb\"}}}}"
                    
                    echo "Expanded $ns/$pvc to $new_size_gb"
                  fi
                fi
              done
          restartPolicy: OnFailure
---
# RBAC 权限
apiVersion: v1
kind: ServiceAccount
metadata:
  name: pvc-expander
  namespace: default
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: pvc-expander
rules:
- apiGroups: [""]
  resources: ["persistentvolumeclaims"]
  verbs: ["get", "list", "patch"]
- apiGroups: [""]
  resources: ["nodes/proxy"]
  verbs: ["get"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: pvc-expander
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: pvc-expander
subjects:
- kind: ServiceAccount
  name: pvc-expander
  namespace: default

# 3. 优化文件系统选择
# ext4: 支持在线扩展（resize2fs），但大文件性能较差
# xfs: 高性能，支持在线扩展（xfs_growfs），推荐生产环境
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd-xfs
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
  # 指定文件系统类型
  csi.storage.k8s.io/fstype: xfs  # ← 使用 XFS
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer

# 4. 配置应用日志轮转（防止磁盘满）
apiVersion: v1
kind: ConfigMap
metadata:
  name: logrotate-config
data:
  logrotate.conf: |
    /data/logs/*.log {
        daily                # 每天轮转
        rotate 7             # 保留 7 天
        missingok            # 文件不存在不报错
        compress             # 压缩旧日志
        delaycompress        # 延迟压缩（保留最近一天未压缩）
        notifempty           # 空文件不轮转
        create 0640 root root  # 新文件权限
        sharedscripts        # 多个日志共享脚本
        postrotate
            # 通知应用重新打开日志文件
            killall -HUP log-collector || true
        endscript
    }
---
apiVersion: batch/v1
kind: CronJob
metadata:
  name: logrotate
spec:
  schedule: "0 2 * * *"  # 每天凌晨 2 点执行
  jobTemplate:
    spec:
      template:
        spec:
          volumes:
          - name: logs
            persistentVolumeClaim:
              claimName: log-data
          - name: config
            configMap:
              name: logrotate-config
          containers:
          - name: logrotate
            image: alpine:latest
            command:
            - /bin/sh
            - -c
            - |
              apk add --no-cache logrotate
              logrotate -f /etc/logrotate.conf
            volumeMounts:
            - name: logs
              mountPath: /data/logs
            - name: config
              mountPath: /etc/logrotate.conf
              subPath: logrotate.conf
          restartPolicy: OnFailure
```

**效果评估**

| 指标 | 优化前 | 优化后 | 改善 |
|-----|--------|--------|------|
| OOMKilled 频率 | 10 次/天 | 0 次/月 | ✅ 完全消除 |
| 磁盘满事件 | 5 次/周 | 0 次/月 | ✅ 完全消除 |
| 日志丢失率 | 30% | <0.1% | ↓ 99.7% |
| 扩容响应时间 | 手动（2 小时） | 自动（30 分钟） | ↓ 75% |
| 磁盘使用率 | 平均 95% | 平均 60% | ↓ 37% |

---

## 附录：存储系统巡检表
- [ ] **回收策略**：关键业务 PV 是否设置为 `Retain`？
- [ ] **绑定模式**：StorageClass 是否根据 AZ 部署需求配置了 `WaitForFirstConsumer`？
- [ ] **高可用**：CSI 控制器是否配置了 `replicas >= 2` 且有领导者选举日志？
- [ ] **监控告警**：是否监控了 PVC 使用率 (`kubelet_volume_stats_used_bytes`)？
- [ ] **备份容灾**：重要卷是否配置了 VolumeSnapshot 策略？
- [ ] **权限检查**：所有节点是否都具备访问后端存储（如 NFS 服务端、云厂商 API）的网络连通性？
