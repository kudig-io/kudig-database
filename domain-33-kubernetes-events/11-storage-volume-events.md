# 11 - 存储与卷事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档详细记录所有存储相关事件,包括卷挂载、卷附加、PV/PVC 管理和 CSI 驱动事件。**

---

## 目录

- [一、事件总览](#一事件总览)
- [二、卷生命周期状态与事件关系](#二卷生命周期状态与事件关系)
- [三、CSI 驱动架构与事件流](#三csi-驱动架构与事件流)
- [四、kubelet 卷挂载事件](#四kubelet-卷挂载事件)
- [五、PV Controller 动态供应事件](#五pv-controller-动态供应事件)
- [六、卷扩容事件](#六卷扩容事件)
- [七、卷回收与删除事件](#七卷回收与删除事件)
- [八、综合排查案例](#八综合排查案例)
- [九、生产环境最佳实践](#九生产环境最佳实践)

---

## 一、事件总览

### 1.1 本文档覆盖的事件列表

#### **kubelet Volume Events (卷操作相关)**

| 事件原因 (Reason) | 类型 | 生产频率 | 适用版本 | 简要说明 |
|:---|:---|:---|:---|:---|
| `SuccessfulAttachVolume` | Normal | 高频 | v1.0+ | AttachVolume.Attach 成功 |
| `FailedAttachVolume` | Warning | 中频 | v1.0+ | AttachVolume.Attach 失败 |
| `SuccessfulMountVolume` | Normal | 高频 | v1.0+ | MountVolume.SetUp 成功 |
| `FailedMount` | Warning | 中频 | v1.0+ | 无法附加或挂载卷 |
| `FailedUnmount` | Warning | 低频 | v1.0+ | 无法卸载卷 |
| `FailedMapVolume` | Warning | 罕见 | v1.14+ | MapVolume 失败 (块设备映射) |
| `VolumeResizeSuccessful` | Normal | 低频 | v1.11+ | 卷扩容成功 |
| `VolumeResizeFailed` | Warning | 低频 | v1.11+ | 卷扩容失败 |
| `FileSystemResizeSuccessful` | Normal | 低频 | v1.15+ | 文件系统扩容成功 |
| `FileSystemResizeFailed` | Warning | 低频 | v1.15+ | 文件系统扩容失败 |

#### **PV Controller Events (PV/PVC 管理相关)**

| 事件原因 (Reason) | 类型 | 生产频率 | 适用版本 | 简要说明 |
|:---|:---|:---|:---|:---|
| `ProvisioningSucceeded` | Normal | 中频 | v1.4+ | 成功动态供应卷 |
| `ProvisioningFailed` | Warning | 中频 | v1.4+ | 动态供应卷失败 |
| `ExternalProvisioning` | Normal | 中频 | v1.4+ | 等待外部 Provisioner |
| `FailedBinding` | Warning | 中频 | v1.0+ | 没有可用的 PV 匹配 |
| `WaitForFirstConsumer` | Normal | 中频 | v1.12+ | 等待首次消费者绑定 |
| `VolumeRecycled` | Normal | 罕见 | v1.0+ (Deprecated) | 卷回收成功 |
| `VolumeRecycleFailed` | Warning | 罕见 | v1.0+ (Deprecated) | 卷回收失败 |
| `VolumeDeleted` | Normal | 低频 | v1.0+ | 卷删除成功 |
| `VolumeFailedDelete` | Warning | 低频 | v1.0+ | 卷删除失败 |
| `ExternalExpanding` | Normal | 低频 | v1.16+ | 等待外部扩容器 |
| `Resizing` | Normal | 低频 | v1.16+ | 外部扩容器正在调整卷大小 |
| `FileSystemResizeRequired` | Normal | 低频 | v1.15+ | 需要文件系统扩容 |

**事件来源**: 
- kubelet: `source.component: kubelet`
- PV Controller: `source.component: persistentvolume-controller`

### 1.2 快速索引

| 问题场景 | 关注事件 | 跳转章节 |
|:---|:---|:---|
| Pod 无法启动 (卷未挂载) | `FailedAttachVolume`, `FailedMount` | [四.2](#42-failedattachvolume---卷附加失败) [四.4](#44-failedmount---卷挂载失败) |
| PVC 一直 Pending | `ProvisioningFailed`, `FailedBinding`, `WaitForFirstConsumer` | [五.2](#52-provisioningfailed---动态供应失败) [五.4](#54-failedbinding---pvc-绑定失败) |
| 卷扩容失败 | `VolumeResizeFailed`, `FileSystemResizeFailed` | [六.1](#61-volumeresizefailed---卷扩容失败) [六.3](#63-filesystemresizefailed---文件系统扩容失败) |
| 卷无法删除 | `VolumeFailedDelete` | [七.2](#72-volumefaileddelete---卷删除失败) |
| 卷性能问题 | `SuccessfulMountVolume`, `SuccessfulAttachVolume` 慢 | [九.5](#95-卷性能监控与优化) |

---

## 二、卷生命周期状态与事件关系

### 2.1 卷生命周期完整流程

```
卷生命周期阶段               产生的主要事件
════════════════════════════════════════════════════════════════════════════

┌──────────────────────┐
│  1. PVC 创建 (Pending) │
└──────────┬───────────┘
           │
           ├──▶ ExternalProvisioning      [PV Controller]
           │    (等待外部 Provisioner,如 CSI driver)
           │
           ├──▶ ProvisioningSucceeded     [PV Controller]
           │    (动态供应成功,创建 PV)
           │
           ├──▶ ProvisioningFailed        [PV Controller]
           │    (供应失败,检查 StorageClass/权限)
           │
           ├──▶ FailedBinding              [PV Controller]
           │    (静态供应:找不到匹配的 PV)
           │
           └──▶ WaitForFirstConsumer       [PV Controller]
                (延迟绑定模式,等待 Pod 调度)

┌──────────────────────┐
│  2. PVC 绑定 (Bound)   │
└──────────┬───────────┘
           │
           │ [Pod 被调度到节点,kubelet 开始处理卷]
           │
           ├──▶ SuccessfulAttachVolume    [kubelet]
           │    (Attach 阶段:AD Controller 将卷附加到节点)
           │
           ├──▶ FailedAttachVolume        [kubelet]
           │    (附加失败,检查云控制器/节点)
           │
           ├──▶ SuccessfulMountVolume     [kubelet]
           │    (Mount 阶段:将卷挂载到 Pod 容器路径)
           │
           └──▶ FailedMount                [kubelet]
                (挂载失败,常见原因:权限/格式化/设备不存在)

┌──────────────────────┐
│  3. 卷使用中          │
└──────────┬───────────┘
           │
           ├──▶ VolumeResizeSuccessful    [kubelet]
           │    (控制平面扩容成功)
           │
           ├──▶ VolumeResizeFailed        [kubelet]
           │    (控制平面扩容失败)
           │
           ├──▶ FileSystemResizeSuccessful [kubelet]
           │    (节点文件系统扩容成功)
           │
           └──▶ FileSystemResizeFailed     [kubelet]
                (节点文件系统扩容失败)

┌──────────────────────┐
│  4. Pod 删除          │
└──────────┬───────────┘
           │
           ├──▶ (Unmount)                 [kubelet,无显式事件]
           │    (卸载卷,但通常不产生 Normal 事件)
           │
           └──▶ FailedUnmount              [kubelet]
                (卸载失败,通常因进程占用)

┌──────────────────────┐
│  5. PVC 删除          │
└──────────┬───────────┘
           │
           │ [根据 reclaimPolicy 处理 PV]
           │
           ├──▶ VolumeDeleted              [PV Controller]
           │    (reclaimPolicy: Delete,删除成功)
           │
           ├──▶ VolumeFailedDelete         [PV Controller]
           │    (删除失败,可能需手动干预)
           │
           └──▶ (Retain/Recycle)           [PV Controller]
                (Retain:保留 PV,Recycle:回收数据,已弃用)
```

### 2.2 PV 与 PVC 状态转换

```
PV 状态               PVC 状态              触发事件
═══════════════════════════════════════════════════════════════════

Available ───────┐    
(可用,未绑定)    │
                 │
                 ├──▶ (静态供应等待)
                 │
                 │    Pending ───────┐
                 │    (等待绑定)     │
                 │                   ├──▶ FailedBinding
                 │                   │     (找不到匹配 PV)
                 │                   │
Bound ◀──────────┼────▶ Bound        │
(已绑定)         │    (已绑定)      │
                 │                   │
                 │                   └──▶ WaitForFirstConsumer
                 │                        (延迟绑定模式)
                 │
Released ────────┤
(PVC 已删除,     │
 等待回收)       │
                 ├──▶ VolumeDeleted
                 │     (Delete 策略)
                 │
                 └──▶ VolumeRecycleFailed
                      (Recycle 策略失败,已弃用)

Failed
(回收/删除失败)
```

### 2.3 卷类型与事件关系

| 卷类型 | Attach 阶段 | Mount 阶段 | 常见事件 |
|:---|:---|:---|:---|
| **云盘 (EBS/Disk)** | 需要 (AD Controller) | 需要 | `FailedAttachVolume`, `FailedMount` |
| **文件存储 (NFS/CephFS)** | 不需要 | 需要 | `FailedMount` |
| **ConfigMap/Secret** | 不需要 | 需要 (内存) | `FailedMount` (权限) |
| **EmptyDir** | 不需要 | 需要 (本地) | `FailedMount` (节点磁盘满) |
| **HostPath** | 不需要 | 需要 (直接挂载) | `FailedMount` (路径不存在) |
| **CSI 卷** | 可选 (Plugin 定义) | 需要 | `FailedAttachVolume`, `FailedMount` |

---

## 三、CSI 驱动架构与事件流

### 3.1 CSI 组件架构

```
┌────────────────────────────────────────────────────────────────┐
│                  Kubernetes 控制平面                              │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐      ┌──────────────────┐                │
│  │ PV Controller   │      │ AD Controller    │                │
│  │ (Bind PV/PVC)   │      │ (Attach/Detach)  │                │
│  └────────┬────────┘      └────────┬─────────┘                │
│           │                        │                           │
│           │ 1. ExternalProvisioning│ 3. FailedAttachVolume    │
│           │ 2. ProvisioningSucceeded                          │
│           │                        │                           │
│  ┌────────▼────────────────────────▼─────────┐                │
│  │  CSI External Components (Sidecar)        │                │
│  │  ┌──────────────┐ ┌─────────────────┐    │                │
│  │  │ external-    │ │ external-       │    │                │
│  │  │ provisioner  │ │ attacher        │    │                │
│  │  └──────┬───────┘ └────────┬────────┘    │                │
│  │         │                  │              │                │
│  │  ┌──────▼──────────────────▼────────┐    │                │
│  │  │   CSI Controller Plugin          │    │                │
│  │  │   (StatefulSet/Deployment)       │    │                │
│  │  └──────────────┬───────────────────┘    │                │
│  └─────────────────┼────────────────────────┘                │
│                    │ gRPC (Unix Socket)                       │
└────────────────────┼──────────────────────────────────────────┘
                     │
┌────────────────────▼──────────────────────────────────────────┐
│                    工作节点 (Node)                             │
├───────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌─────────────────┐                                          │
│  │    kubelet      │                                          │
│  │  (Volume Mgr)   │                                          │
│  └────────┬────────┘                                          │
│           │                                                    │
│           │ 4. SuccessfulMountVolume                          │
│           │ 5. FailedMount                                    │
│           │                                                    │
│  ┌────────▼──────────────────────┐                            │
│  │  CSI Node Plugin               │                            │
│  │  (DaemonSet)                  │                            │
│  │  - NodeStageVolume            │                            │
│  │  - NodePublishVolume          │                            │
│  └────────┬──────────────────────┘                            │
│           │                                                    │
│           │ Mount 到 Pod 容器路径                              │
│           │                                                    │
│  ┌────────▼──────────────────────┐                            │
│  │  /var/lib/kubelet/pods/       │                            │
│  │  <pod-uid>/volumes/           │                            │
│  └───────────────────────────────┘                            │
└───────────────────────────────────────────────────────────────┘
```

### 3.2 CSI 卷操作流程与事件

```
步骤                  CSI 接口                  Kubernetes 事件
═══════════════════════════════════════════════════════════════════

1. 动态供应
   ├─▶ CreateVolume (Controller)    ExternalProvisioning
   └─▶ (供应商创建云盘)             ProvisioningSucceeded
                                     ProvisioningFailed

2. 卷附加 (Attach)
   ├─▶ ControllerPublishVolume      SuccessfulAttachVolume
   └─▶ (将卷附加到节点)             FailedAttachVolume

3. 节点暂存 (Stage)
   ├─▶ NodeStageVolume               (无显式事件)
   └─▶ (格式化、挂载到全局目录)     FailedMount (失败时)

4. 卷发布 (Publish)
   ├─▶ NodePublishVolume             SuccessfulMountVolume
   └─▶ (Bind Mount 到 Pod 路径)     FailedMount

5. 卷扩容
   ├─▶ ControllerExpandVolume       VolumeResizeSuccessful
   │   (控制平面扩容)               VolumeResizeFailed
   │
   └─▶ NodeExpandVolume              FileSystemResizeSuccessful
       (节点文件系统扩容)           FileSystemResizeFailed

6. 卷卸载 (Unpublish)
   └─▶ NodeUnpublishVolume           FailedUnmount (失败时)

7. 节点卸载 (Unstage)
   └─▶ NodeUnstageVolume             FailedUnmount (失败时)

8. 卷分离 (Detach)
   └─▶ ControllerUnpublishVolume     (无显式事件)

9. 卷删除
   └─▶ DeleteVolume                  VolumeDeleted
                                     VolumeFailedDelete
```

### 3.3 StorageClass 与 ReclaimPolicy

| 字段 | 值 | 说明 | 影响的事件 |
|:---|:---|:---|:---|
| **provisioner** | `kubernetes.io/aws-ebs`<br>`ebs.csi.aws.com`<br>`disk.csi.azure.com` | 指定动态供应器 | `ExternalProvisioning` |
| **volumeBindingMode** | `Immediate` | PVC 创建时立即绑定 PV | `ProvisioningSucceeded` |
|  | `WaitForFirstConsumer` | 等待 Pod 调度后再绑定 | `WaitForFirstConsumer` |
| **reclaimPolicy** | `Delete` | PVC 删除时删除 PV 和底层存储 | `VolumeDeleted` |
|  | `Retain` | PVC 删除时保留 PV (需手动删除) | 无自动删除事件 |
|  | `Recycle` (Deprecated) | 删除 PV 数据后回收 PV | `VolumeRecycleFailed` |
| **allowVolumeExpansion** | `true` | 允许卷在线扩容 | `VolumeResizeSuccessful` |
|  | `false` | 禁止扩容 | - |

**StorageClass 示例**:
```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
reclaimPolicy: Delete              # PVC 删除时自动删除 EBS 卷
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定,避免跨 AZ 问题
allowVolumeExpansion: true         # 支持在线扩容
```

---

## 四、kubelet 卷挂载事件

### 4.1 SuccessfulAttachVolume - 卷附加成功

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `SuccessfulAttachVolume` |
| **Type** | `Normal` |
| **Source** | `kubelet, <node-name>` |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 (每次 Pod 启动使用持久化卷) |

#### **事件含义**

AttachDetach Controller (AD Controller) 成功将卷附加到节点。对于云盘类型 (AWS EBS、Azure Disk、GCE PD),这一步骤会调用云提供商 API 将卷附加到 VM 实例。

#### **典型事件消息**

```
AttachVolume.Attach succeeded for volume "pvc-abc123"
```

#### **事件触发条件**

1. **云盘类型卷** (EBS/Disk/PD):
   - AD Controller 调用云 API 成功
   - 卷状态从 `Attaching` → `Attached`
   
2. **CSI 卷** (如果 CSI Driver 支持 Attach):
   - `ControllerPublishVolume` RPC 调用成功
   - `VolumeAttachment` 对象状态更新为 `Attached: true`

#### **关联 Kubernetes 对象**

```yaml
# VolumeAttachment (CSI)
apiVersion: storage.k8s.io/v1
kind: VolumeAttachment
metadata:
  name: csi-abc123
spec:
  attacher: ebs.csi.aws.com
  nodeName: ip-10-0-1-100
  source:
    persistentVolumeName: pvc-abc123
status:
  attached: true  # ✅ Attach 成功
  attachmentMetadata:
    DevicePath: /dev/xvdba
```

#### **排查路径**

**正常流程**:
```
1. Pod 调度到节点
2. AD Controller 检测到需要 Attach 的卷
3. 调用云 API (如 AWS AttachVolume)
4. 等待卷附加到节点 (通常 10-30s)
5. 触发 SuccessfulAttachVolume 事件
6. kubelet 继续执行 Mount 操作
```

**性能优化**:
- **Attach 时间过长** → 检查云 API 响应时间、配额限制
- **频繁 Attach/Detach** → 考虑使用本地存储 (Local PV/HostPath)

---

### 4.2 FailedAttachVolume - 卷附加失败

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `FailedAttachVolume` |
| **Type** | `Warning` |
| **Source** | `attachdetach-controller` (显示为 `kubelet` 或 `controller-manager`) |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### **事件含义**

AttachDetach Controller 无法将卷附加到节点。这通常是云控制器或节点层面的问题,会导致 Pod 无法启动 (一直 `ContainerCreating` 状态)。

#### **典型事件消息**

```
AttachVolume.Attach failed for volume "pvc-abc123" : 
rpc error: code = Internal desc = Could not attach volume "vol-0abc123" 
to node "i-0def456": VolumeInUse: vol-0abc123 is already attached to an instance
```

```
AttachVolume.Attach failed for volume "pvc-xyz789" : 
googleapi: Error 400: The disk resource 'projects/my-project/zones/us-central1-a/disks/pvc-xyz789' 
is already being used by 'projects/my-project/zones/us-central1-b/instances/node-2'
```

#### **常见原因与解决方案**

| 错误消息 | 根因 | 解决方案 | 严重程度 |
|:---|:---|:---|:---|
| `VolumeInUse: already attached` | 卷已附加到其他节点 (云盘不支持多 Attach) | 1. 等待原 Pod 删除<br>2. 手动 Detach (云控制台)<br>3. 强制删除旧 Pod | **高** |
| `InvalidVolume.NotFound` | 卷不存在或已被删除 | 检查 PV 定义的 `volumeHandle` 是否正确 | **高** |
| `InvalidParameterValue: zone mismatch` | 卷和节点不在同一可用区 | 1. 使用 `WaitForFirstConsumer`<br>2. 配置 Pod 拓扑约束 | **中** |
| `exceeded maximum number of volumes` | 节点达到最大卷数限制 (AWS:39) | 1. 使用更大实例类型<br>2. 减少 Pod 密度 | **中** |
| `VolumeLimitExceeded` | Pod 使用卷数超过节点限制 | 同上 | **中** |
| `Throttling: Rate exceeded` | 云 API 限流 | 1. 减少 Pod 创建速率<br>2. 请求提高配额 | **低** |
| `Unauthorized: IAM permissions` | 节点 IAM 角色缺少权限 | 添加 `ec2:AttachVolume` 权限 | **高** |

#### **排查步骤**

**步骤 1: 检查 VolumeAttachment 对象**
```bash
kubectl get volumeattachment -o wide
kubectl describe volumeattachment csi-<hash>
```

**步骤 2: 检查卷状态 (云控制台)**
```bash
# AWS
aws ec2 describe-volumes --volume-ids vol-0abc123

# 输出示例 (问题场景)
"Attachments": [
  {
    "State": "attached",
    "InstanceId": "i-old-instance",  # ❌ 附加到已删除的节点
    "Device": "/dev/xvdba"
  }
]
```

**步骤 3: 强制 Detach (谨慎操作)**
```bash
# AWS
aws ec2 detach-volume --volume-id vol-0abc123 --force

# 等待 Detach 完成
aws ec2 describe-volumes --volume-ids vol-0abc123 --query 'Volumes[0].State'
```

**步骤 4: 检查节点卷数限制**
```bash
# 查看节点已附加卷数
kubectl get node <node> -o json | jq '.status.volumesAttached | length'

# 查看节点最大卷数限制
kubectl get node <node> -o json | jq '.status.allocatable."attachable-volumes-aws-ebs"'
```

#### **生产案例: 节点异常导致卷泄漏**

**现象**:
- Pod 无法启动,一直 `ContainerCreating`
- 事件: `FailedAttachVolume: VolumeInUse`
- 云控制台显示卷附加到已终止的节点

**根因**:
节点突然宕机,AD Controller 未及时 Detach 卷

**解决方案**:
```bash
# 1. 强制删除旧 Pod (如果存在)
kubectl delete pod <old-pod> --grace-period=0 --force

# 2. 手动 Detach 卷
aws ec2 detach-volume --volume-id vol-xxx --force

# 3. 等待 30s 后,新 Pod 自动重试 Attach
```

**预防措施**:
- 配置节点优雅终止 (Node Draining)
- 监控 `VolumeAttachment` 对象泄漏

---

### 4.3 SuccessfulMountVolume - 卷挂载成功

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `SuccessfulMountVolume` |
| **Type** | `Normal` |
| **Source** | `kubelet, <node-name>` |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### **事件含义**

kubelet 成功将卷挂载到 Pod 的容器路径。这是卷生命周期的最后一步,完成后容器才能访问存储。

#### **典型事件消息**

```
MountVolume.SetUp succeeded for volume "pvc-abc123"
```

```
MountVolume.SetUp succeeded for volume "config-volume" (UniqueName: "kubernetes.io/configmap/<pod-uid>-config-volume")
```

#### **挂载路径结构**

```
# 全局挂载路径 (NodeStageVolume)
/var/lib/kubelet/plugins/kubernetes.io/csi/volumeDevices/publish/<volume-id>/

# Pod 容器路径 (NodePublishVolume)
/var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~csi/<pvc-name>/
│
└─▶ Bind Mount 到容器 ─▶ /data (容器内路径)
```

#### **不同卷类型的挂载行为**

| 卷类型 | 挂载方式 | 挂载时间 | 备注 |
|:---|:---|:---|:---|
| **PV (云盘)** | CSI NodeStageVolume + NodePublishVolume | 10-60s | 需格式化、Attach |
| **ConfigMap** | 内存 tmpfs | <1s | 数据存储在内存 |
| **Secret** | 内存 tmpfs | <1s | 自动 Base64 解码 |
| **EmptyDir** | 本地目录 | <1s | 随 Pod 删除而删除 |
| **HostPath** | Bind Mount | <1s | 直接挂载主机路径 |
| **NFS/CephFS** | 网络文件系统 | 5-30s | 依赖网络稳定性 |

---

### 4.4 FailedMount - 卷挂载失败

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `FailedMount` |
| **Type** | `Warning` |
| **Source** | `kubelet, <node-name>` |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 (生产环境常见问题) |

#### **事件含义**

kubelet 无法挂载卷到 Pod。这是 **最常见的存储问题**,会导致 Pod 一直停留在 `ContainerCreating` 状态。

#### **典型事件消息**

```
MountVolume.SetUp failed for volume "pvc-abc123" : 
rpc error: code = Internal desc = failed to mount device: 
/dev/xvdba at /var/lib/kubelet/plugins/.../globalmount: 
exit status 32: mount: /var/lib/kubelet/plugins/...: 
wrong fs type, bad option, bad superblock on /dev/xvdba
```

```
Unable to attach or mount volumes: unmounted volumes=[data], 
unattached volumes=[config data kube-api-access-xxx]: 
timed out waiting for the condition
```

#### **常见原因与解决方案**

| 错误消息 | 根因 | 解决方案 | 严重程度 |
|:---|:---|:---|:---|
| `wrong fs type, bad superblock` | 磁盘未格式化或文件系统损坏 | 1. 检查 PV `fsType`<br>2. 手动格式化 (⚠️ 数据丢失) | **高** |
| `device not found: /dev/xvdba` | Attach 成功但设备未识别 | 1. 等待内核识别设备 (1-5s)<br>2. 检查 `dmesg` 日志 | **高** |
| `target is busy` | 卷被其他进程占用 | 1. 检查僵尸进程<br>2. 重启 kubelet | **中** |
| `permission denied` | SELinux/AppArmor 阻止挂载 | 配置 Pod `securityContext.seLinuxOptions` | **中** |
| `no space left on device` | 节点磁盘已满 | 清理磁盘或扩容节点卷 | **高** |
| `too many levels of symbolic links` | 挂载路径循环依赖 | 删除异常挂载点 | **低** |
| `configmap "xxx" not found` | ConfigMap 不存在 | 创建 ConfigMap | **高** |
| `secret "xxx" not found` | Secret 不存在 | 创建 Secret | **高** |

#### **排查步骤**

**步骤 1: 检查 Pod 事件**
```bash
kubectl describe pod <pod> | grep -A 10 "Events:"
```

**步骤 2: 检查节点挂载状态**
```bash
# 查看全局挂载
ssh <node>
mount | grep <volume-id>

# 查看 kubelet 日志
journalctl -u kubelet -f | grep -i mount

# 查看设备识别情况
lsblk
dmesg | grep -i xvdba
```

**步骤 3: 检查 CSI Node 插件日志**
```bash
kubectl logs -n kube-system <csi-node-pod> -c csi-driver
```

**步骤 4: 检查文件系统**
```bash
# 检查设备文件系统类型
file -s /dev/xvdba

# 输出示例 (未格式化)
/dev/xvdba: data  # ❌ 未格式化

# 输出示例 (已格式化)
/dev/xvdba: Linux rev 1.0 ext4 filesystem data  # ✅ ext4
```

#### **生产案例: ConfigMap 不存在导致 Pod 无法启动**

**现象**:
- Pod 一直 `ContainerCreating`
- 事件: `MountVolume.SetUp failed for volume "config": configmap "app-config" not found`

**根因**:
Helm Chart 定义了 ConfigMap 挂载,但未实际创建 ConfigMap

**解决方案**:
```bash
# 创建 ConfigMap
kubectl create configmap app-config --from-file=config.yaml

# Pod 自动重试挂载
```

---

### 4.5 FailedUnmount - 卷卸载失败

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `FailedUnmount` |
| **Type** | `Warning` |
| **Source** | `kubelet, <node-name>` |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |

#### **事件含义**

kubelet 无法卸载卷。这会导致 Pod 删除卡住 (Terminating 状态),并可能阻止卷被其他 Pod 使用。

#### **典型事件消息**

```
Unable to unmount volume "pvc-abc123": 
UnmountVolume.TearDown failed for volume "pvc-abc123" : 
target is busy: [/var/lib/kubelet/pods/<pod-uid>/volumes/...]
```

#### **常见原因**

1. **进程占用挂载点**:
   - 容器内进程未正常终止
   - 僵尸进程持有文件句柄

2. **内核 bug**:
   - NFS 卷 hang 住
   - 驱动异常

3. **节点资源耗尽**:
   - 无法执行 umount 命令

#### **排查步骤**

```bash
# 1. 检查占用进程
lsof +D /var/lib/kubelet/pods/<pod-uid>/volumes/<volume>

# 2. 强制杀死进程
kill -9 <pid>

# 3. 手动卸载
umount -l /var/lib/kubelet/pods/<pod-uid>/volumes/<volume>

# 4. 如果仍失败,重启 kubelet
systemctl restart kubelet
```

---

### 4.6 FailedMapVolume - 块设备映射失败

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `FailedMapVolume` |
| **Type** | `Warning` |
| **Source** | `kubelet, <node-name>` |
| **适用版本** | v1.14+ |
| **生产频率** | 罕见 (仅 Block 模式卷) |

#### **事件含义**

kubelet 无法将块设备映射到 Pod。这只影响 `volumeMode: Block` 的 PV (原始块设备,非文件系统)。

#### **典型事件消息**

```
MapVolume.MapPodDevice failed for volume "pvc-abc123" : 
failed to create symbolic link for raw-block device: 
/dev/xvdba to /var/lib/kubelet/pods/<pod-uid>/volumeDevices/...
```

#### **使用场景**

```yaml
# PVC 示例 (Block 模式)
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: block-pvc
spec:
  volumeMode: Block  # ✅ 块设备模式 (非 Filesystem)
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 10Gi

---
# Pod 使用
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: app
    volumeDevices:  # ⚠️ 使用 volumeDevices 而非 volumeMounts
    - name: data
      devicePath: /dev/xvda  # 容器内设备路径
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: block-pvc
```

**应用场景**: 数据库 (如 PostgreSQL) 直接操作块设备以获得最大性能

---

## 五、PV Controller 动态供应事件

### 5.1 ExternalProvisioning - 等待外部 Provisioner

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `ExternalProvisioning` |
| **Type** | `Normal` |
| **Source** | `persistentvolume-controller` |
| **适用版本** | v1.4+ |
| **生产频率** | 中频 |

#### **事件含义**

PV Controller 检测到 PVC 需要动态供应,正在等待外部 Provisioner (如 CSI Driver) 创建 PV。

#### **典型事件消息**

```
waiting for a volume to be created, either by external provisioner "ebs.csi.aws.com" or manually created by system administrator
```

#### **流程说明**

```
1. 用户创建 PVC
2. PV Controller 检查 StorageClass.provisioner
3. 触发 ExternalProvisioning 事件
4. CSI External Provisioner 监听 PVC 创建
5. 调用 CSI Controller Plugin 的 CreateVolume RPC
6. 云提供商创建卷 (如 AWS CreateVolume API)
7. External Provisioner 创建 PV 对象
8. PV Controller 绑定 PV 和 PVC
9. 触发 ProvisioningSucceeded 事件
```

#### **排查超时问题**

如果 PVC 长时间停留在此状态:

```bash
# 1. 检查 CSI Controller 组件状态
kubectl get pod -n kube-system -l app=ebs-csi-controller

# 2. 查看 external-provisioner 日志
kubectl logs -n kube-system <csi-controller-pod> -c csi-provisioner

# 3. 检查 StorageClass 是否存在
kubectl get storageclass <storageclass-name>

# 4. 检查 CSI Driver 是否注册
kubectl get csidrivers
```

---

### 5.2 ProvisioningFailed - 动态供应失败

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `ProvisioningFailed` |
| **Type** | `Warning` |
| **Source** | `persistentvolume-controller` |
| **适用版本** | v1.4+ |
| **生产频率** | 中频 |

#### **事件含义**

外部 Provisioner 创建卷失败。这是 PVC 无法绑定的主要原因之一。

#### **典型事件消息**

```
Failed to provision volume with StorageClass "fast-ssd": 
rpc error: code = ResourceExhausted desc = 
You have exceeded your maximum gp3 volume limit in us-east-1
```

```
Failed to provision volume: 
InvalidParameterValue: The availability zone 'us-east-1d' does not exist
```

#### **常见原因与解决方案**

| 错误消息 | 根因 | 解决方案 | 严重程度 |
|:---|:---|:---|:---|
| `ResourceExhausted: exceeded volume limit` | 云账号配额不足 | 请求提高配额或删除未使用卷 | **高** |
| `InvalidParameterValue: invalid zone` | StorageClass 参数错误 | 修正 `parameters.zone` | **高** |
| `Unauthorized: IAM permissions` | CSI Controller IAM 角色缺少权限 | 添加 `ec2:CreateVolume` 权限 | **高** |
| `InvalidParameterCombination: iops not supported` | gp2 卷不支持 IOPS 参数 | 使用 gp3 或移除 `iops` 参数 | **中** |
| `VolumeLimitExceeded` | 请求的卷大小超过限制 | 降低卷大小或使用其他卷类型 | **中** |
| `storageclass.storage.k8s.io "xxx" not found` | StorageClass 不存在 | 创建 StorageClass | **高** |

#### **排查步骤**

```bash
# 1. 检查 PVC 详细信息
kubectl describe pvc <pvc-name>

# 2. 检查 CSI Controller 日志
kubectl logs -n kube-system <csi-controller-pod> -c csi-provisioner -f

# 3. 检查云控制台 (AWS 示例)
aws ec2 describe-volumes --filters "Name=tag:kubernetes.io/cluster/<cluster-name>,Values=owned"

# 4. 检查 IAM 权限
aws iam get-role-policy --role-name <csi-controller-role> --policy-name <policy-name>
```

#### **生产案例: 云配额不足导致 StatefulSet 扩容失败**

**现象**:
- StatefulSet 扩容时新 Pod 一直 Pending
- PVC 事件: `ProvisioningFailed: ResourceExhausted`

**根因**:
AWS 账号 EBS 卷数量达到配额上限 (默认 5000)

**解决方案**:
```bash
# 1. 清理未使用的 PV
kubectl get pv | grep Released | awk '{print $1}' | xargs kubectl delete pv

# 2. 请求提高 AWS Service Quotas
aws service-quotas request-service-quota-increase \
  --service-code ebs \
  --quota-code L-D18FCD1D \
  --desired-value 10000

# 3. 临时方案: 使用 Local PV
```

---

### 5.3 ProvisioningSucceeded - 动态供应成功

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `ProvisioningSucceeded` |
| **Type** | `Normal` |
| **Source** | `persistentvolume-controller` |
| **适用版本** | v1.4+ |
| **生产频率** | 中频 |

#### **事件含义**

外部 Provisioner 成功创建 PV,PVC 进入 `Bound` 状态。

#### **典型事件消息**

```
Successfully provisioned volume pvc-abc123 using ebs.csi.aws.com
```

#### **关联对象**

```yaml
# PV (自动创建)
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pvc-abc123
  annotations:
    pv.kubernetes.io/provisioned-by: ebs.csi.aws.com
spec:
  capacity:
    storage: 10Gi
  csi:
    driver: ebs.csi.aws.com
    volumeHandle: vol-0abc123  # AWS EBS Volume ID
  claimRef:
    name: data-db-0
    namespace: default
```

---

### 5.4 FailedBinding - PVC 绑定失败

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `FailedBinding` |
| **Type** | `Warning` |
| **Source** | `persistentvolume-controller` |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### **事件含义**

PV Controller 找不到满足 PVC 要求的可用 PV。这通常发生在 **静态供应** 场景。

#### **典型事件消息**

```
no persistent volumes available for this claim and no storage class is set
```

```
Failed to bind volumes: timeout expired waiting for volumes to bind
```

#### **常见原因**

1. **静态供应场景**: 没有预先创建匹配的 PV
2. **容量不足**: PV 容量小于 PVC 请求
3. **访问模式不匹配**: PV 是 `ReadWriteOnce`,但 PVC 请求 `ReadWriteMany`
4. **标签选择器不匹配**: PVC 的 `selector` 与 PV 标签不匹配

#### **解决方案**

```bash
# 1. 检查可用 PV
kubectl get pv -o wide

# 2. 检查 PVC 要求
kubectl get pvc <pvc-name> -o yaml

# 3. 创建匹配的 PV (静态供应)
kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolume
metadata:
  name: manual-pv
spec:
  capacity:
    storage: 10Gi
  accessModes:
    - ReadWriteOnce
  hostPath:
    path: /mnt/data
EOF
```

---

### 5.5 WaitForFirstConsumer - 等待首次消费者

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `WaitForFirstConsumer` |
| **Type** | `Normal` |
| **Source** | `persistentvolume-controller` |
| **适用版本** | v1.12+ |
| **生产频率** | 中频 |

#### **事件含义**

StorageClass 配置了 `volumeBindingMode: WaitForFirstConsumer`,PV Controller 等待 Pod 调度后再绑定 PVC。这是推荐的最佳实践,可避免跨可用区问题。

#### **典型事件消息**

```
waiting for first consumer to be created before binding
```

#### **延迟绑定的优势**

```
传统 Immediate 绑定模式:
1. PVC 创建 → PV 立即供应 (假设在 us-east-1a)
2. Pod 创建 → 调度器选择节点 (可能在 us-east-1b)
3. ❌ Pod 无法启动 (跨 AZ 卷无法 Attach)

WaitForFirstConsumer 模式:
1. PVC 创建 → 保持 Pending (不立即供应)
2. Pod 创建 → 调度器选择节点 (us-east-1a)
3. PV 供应 → 在 Pod 所在 AZ 创建卷 (us-east-1a)
4. ✅ Pod 正常启动
```

#### **配置示例**

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer  # ✅ 延迟绑定
allowedTopologies:
- matchLabelExpressions:
  - key: topology.ebs.csi.aws.com/zone
    values:
    - us-east-1a
    - us-east-1b
```

---

## 六、卷扩容事件

### 6.1 VolumeResizeFailed - 卷扩容失败

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `VolumeResizeFailed` |
| **Type** | `Warning` |
| **Source** | `external-resizer` (CSI) 或 `kubelet` |
| **适用版本** | v1.11+ |
| **生产频率** | 低频 |

#### **事件含义**

控制平面卷扩容失败 (CSI ControllerExpandVolume)。这是卷扩容的第一阶段,失败后不会进行文件系统扩容。

#### **典型事件消息**

```
resize volume "pvc-abc123" by resizer "ebs.csi.aws.com" failed: 
rpc error: code = Internal desc = 
Could not resize volume "vol-0abc123": 
InvalidParameterValue: Volume vol-0abc123 is not in a state that allows modification
```

#### **常见原因**

| 错误消息 | 根因 | 解决方案 |
|:---|:---|:---|
| `not in a state that allows modification` | 卷正在被修改或创建中 | 等待前一次操作完成 |
| `does not support online resizing` | 卷类型不支持在线扩容 | 停止 Pod 后再扩容 |
| `maximum volume size exceeded` | 请求大小超过卷类型限制 | 降低目标大小 |
| `allowVolumeExpansion is false` | StorageClass 禁止扩容 | 修改 StorageClass (⚠️ 不影响已创建 PV) |

#### **卷扩容流程**

```
步骤 1: 用户编辑 PVC,增加 spec.resources.requests.storage
    ↓
步骤 2: external-resizer 监听 PVC 变化
    ↓
步骤 3: 调用 CSI ControllerExpandVolume (控制平面扩容)
    ├─▶ ✅ VolumeResizeSuccessful
    └─▶ ❌ VolumeResizeFailed (云 API 失败)
    ↓
步骤 4: kubelet 调用 NodeExpandVolume (节点文件系统扩容)
    ├─▶ ✅ FileSystemResizeSuccessful
    └─▶ ❌ FileSystemResizeFailed (文件系统错误)
```

#### **排查步骤**

```bash
# 1. 检查 PVC 状态
kubectl get pvc <pvc-name> -o yaml

# 输出示例 (扩容中)
status:
  capacity:
    storage: 10Gi  # ❌ 仍是旧值
  conditions:
  - type: Resizing
    status: "True"
    message: "Waiting for user to (re-)start a pod..."

# 2. 检查 external-resizer 日志
kubectl logs -n kube-system <csi-controller-pod> -c csi-resizer

# 3. 检查云卷状态
aws ec2 describe-volumes-modifications --volume-ids vol-0abc123
```

---

### 6.2 VolumeResizeSuccessful - 卷扩容成功

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `VolumeResizeSuccessful` |
| **Type** | `Normal` |
| **Source** | `external-resizer` (CSI) |
| **适用版本** | v1.11+ |
| **生产频率** | 低频 |

#### **事件含义**

控制平面成功扩容卷 (如 AWS ModifyVolume API),但文件系统尚未扩容。

#### **典型事件消息**

```
External resizer ebs.csi.aws.com has successfully resized volume pvc-abc123
```

#### **注意事项**

⚠️ **卷扩容成功 ≠ 可用空间增加**

```bash
# 控制平面扩容完成
aws ec2 describe-volumes --volume-ids vol-0abc123
# Size: 20 GB (已扩容)

# 但容器内文件系统仍是 10 GB
kubectl exec <pod> -- df -h /data
# /dev/xvdba  10G  9.5G  500M  96% /data  ❌ 仍是 10 GB

# 需等待 FileSystemResizeSuccessful 事件
```

---

### 6.3 FileSystemResizeFailed - 文件系统扩容失败

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `FileSystemResizeFailed` |
| **Type** | `Warning` |
| **Source** | `kubelet, <node-name>` |
| **适用版本** | v1.15+ |
| **生产频率** | 低频 |

#### **事件含义**

kubelet 无法扩容文件系统 (如 `resize2fs` 命令失败)。这会导致控制平面扩容成功,但容器内看不到新空间。

#### **典型事件消息**

```
File system resize failed for volume "pvc-abc123": 
resize2fs: Bad magic number in super-block while trying to open /dev/xvdba
```

#### **常见原因**

1. **文件系统损坏**: 超级块错误
2. **文件系统不支持在线扩容**: 如 ext3
3. **Pod 未重启**: 某些情况需要重启 Pod
4. **权限不足**: kubelet 无法执行 `resize2fs`

#### **解决方案**

```bash
# 1. 检查文件系统类型
kubectl exec <pod> -- df -T /data
# /dev/xvdba ext4 ...  # ✅ ext4 支持在线扩容

# 2. 手动触发扩容 (进入节点)
ssh <node>
resize2fs /dev/xvdba

# 3. 验证扩容
kubectl exec <pod> -- df -h /data
# /dev/xvdba  20G  9.5G  10.5G  48% /data  ✅ 已扩容
```

---

### 6.4 FileSystemResizeSuccessful - 文件系统扩容成功

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `FileSystemResizeSuccessful` |
| **Type** | `Normal` |
| **Source** | `kubelet, <node-name>` |
| **适用版本** | v1.15+ |
| **生产频率** | 低频 |

#### **事件含义**

kubelet 成功扩容文件系统,容器内可见新空间。

#### **典型事件消息**

```
MountVolume.NodeExpandVolume succeeded for volume "pvc-abc123"
```

#### **完整扩容流程示例**

```bash
# 1. 编辑 PVC
kubectl patch pvc data-db-0 -p '{"spec":{"resources":{"requests":{"storage":"20Gi"}}}}'

# 2. 查看事件
kubectl get events --field-selector involvedObject.name=data-db-0 --sort-by='.lastTimestamp'

# 时间线:
# 0s   Normal  ExternalExpanding         waiting for external resizer
# 30s  Normal  Resizing                  external resizer is resizing volume
# 45s  Normal  VolumeResizeSuccessful    control plane resize succeeded
# 60s  Normal  FileSystemResizeRequired  require file system resize of volume
# 75s  Normal  FileSystemResizeSuccessful filesystem resize succeeded

# 3. 验证 PVC 状态
kubectl get pvc data-db-0 -o jsonpath='{.status.capacity.storage}'
# 20Gi  ✅

# 4. 验证容器内空间
kubectl exec db-0 -- df -h /var/lib/postgresql/data
# /dev/xvdba  20G  ...  ✅
```

---

### 6.5 ExternalExpanding 和 Resizing - 扩容进行中

#### **事件基本信息**

| 事件 | 含义 |
|:---|:---|
| `ExternalExpanding` | 等待 external-resizer 处理 |
| `Resizing` | external-resizer 正在调用 CSI ControllerExpandVolume |

#### **典型事件消息**

```
waiting for an external controller to resize volume pvc-abc123
```

```
External resizer is resizing volume pvc-abc123
```

#### **区别说明**

```
ExternalExpanding (等待阶段)
    ↓
Resizing (调用云 API)
    ↓
VolumeResizeSuccessful (控制平面完成)
    ↓
FileSystemResizeRequired (等待节点扩容)
    ↓
FileSystemResizeSuccessful (完全完成)
```

---

### 6.6 FileSystemResizeRequired - 需要文件系统扩容

#### **事件含义**

控制平面扩容完成,但需要 kubelet 执行文件系统扩容 (通常在 Pod 重启或卷重新挂载时自动完成)。

#### **典型事件消息**

```
Require file system resize of volume on node
```

#### **触发条件**

- 对于某些 CSI Driver,控制平面扩容后需要 Pod 重启才能触发文件系统扩容
- 解决方案: 滚动重启 Pod

```bash
kubectl rollout restart statefulset <name>
```

---

## 七、卷回收与删除事件

### 7.1 VolumeDeleted - 卷删除成功

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `VolumeDeleted` |
| **Type** | `Normal` |
| **Source** | `persistentvolume-controller` |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |

#### **事件含义**

PV Controller 成功删除 PV (当 `reclaimPolicy: Delete` 时)。

#### **典型事件消息**

```
Volume pvc-abc123 has been successfully deleted
```

#### **触发条件**

```yaml
# PV 配置
apiVersion: v1
kind: PersistentVolume
metadata:
  name: pvc-abc123
spec:
  persistentVolumeReclaimPolicy: Delete  # ✅ 删除策略
  claimRef:
    name: data-db-0
    namespace: default
```

**删除流程**:
```
1. kubectl delete pvc data-db-0
2. PVC 状态 → Terminating
3. PV 状态 → Released
4. PV Controller 调用 CSI DeleteVolume
5. 云提供商删除卷 (AWS DeleteVolume API)
6. 触发 VolumeDeleted 事件
7. PV 对象被删除
```

---

### 7.2 VolumeFailedDelete - 卷删除失败

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `VolumeFailedDelete` |
| **Type** | `Warning` |
| **Source** | `persistentvolume-controller` |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |

#### **事件含义**

PV Controller 无法删除 PV。这会导致 PV 和云卷泄漏,需要手动清理。

#### **典型事件消息**

```
Failed to delete volume pvc-abc123: 
rpc error: code = Internal desc = 
DeleteVolume failed for volume vol-0abc123: 
VolumeInUse: Volume vol-0abc123 is currently attached to i-0def456
```

#### **常见原因**

| 错误消息 | 根因 | 解决方案 |
|:---|:---|:---|
| `VolumeInUse: attached to instance` | 卷仍附加到节点 | 手动 Detach 卷后删除 |
| `has snapshot dependencies` | 卷有快照依赖 | 先删除快照 |
| `AccessDenied: IAM permissions` | IAM 角色缺少 `ec2:DeleteVolume` 权限 | 添加权限 |
| `InvalidVolume.NotFound` | 卷已被手动删除 | 移除 PV finalizer |

#### **手动清理步骤**

```bash
# 1. 检查 PV 状态
kubectl get pv pvc-abc123 -o yaml

# 2. 移除 finalizer (允许删除 PV 对象)
kubectl patch pv pvc-abc123 -p '{"metadata":{"finalizers":null}}'

# 3. 手动删除云卷
aws ec2 delete-volume --volume-id vol-0abc123

# 4. 删除 PV
kubectl delete pv pvc-abc123
```

---

### 7.3 VolumeRecycleFailed - 卷回收失败 (已弃用)

#### **事件基本信息**

| 字段 | 值 |
|:---|:---|
| **Reason** | `VolumeRecycleFailed` |
| **Type** | `Warning` |
| **适用版本** | v1.0+ (Deprecated in v1.14) |
| **生产频率** | 罕见 (已弃用) |

#### **事件含义**

⚠️ **已弃用**: `reclaimPolicy: Recycle` 策略已在 v1.14 弃用,不建议使用。

**Recycle 策略**: 删除 PVC 后,PV Controller 会删除卷内数据 (如 `rm -rf /volume/*`),然后将 PV 状态设为 `Available`,供其他 PVC 使用。

**弃用原因**:
- 安全风险: 数据删除不彻底
- 不支持云存储
- 推荐使用 `Delete` 或 `Retain`

---

## 八、综合排查案例

### 案例 1: Pod 一直 ContainerCreating (FailedMount)

#### **现象**

```bash
kubectl get pod
# NAME   READY   STATUS              RESTARTS   AGE
# web-0  0/1     ContainerCreating   0          5m
```

#### **排查步骤**

```bash
# 1. 检查事件
kubectl describe pod web-0 | grep -A 20 "Events:"

# 输出:
# Warning  FailedMount  kubelet  MountVolume.SetUp failed for volume "pvc-abc123": 
# rpc error: code = Internal desc = failed to mount device: /dev/xvdba: 
# exit status 32: mount: wrong fs type

# 2. 诊断: 磁盘未格式化或文件系统损坏
```

#### **解决方案**

**方法 1: 重新格式化 (⚠️ 数据丢失)**

```bash
# 进入节点
ssh <node>

# 检查设备
lsblk | grep xvdba
# xvdba  259:0  0  10G  0 disk

# 格式化 (⚠️ 删除所有数据)
mkfs.ext4 /dev/xvdba

# 重启 Pod
kubectl delete pod web-0
```

**方法 2: 修复文件系统**

```bash
# 尝试修复
e2fsck -f /dev/xvdba

# 如果成功,重新挂载
mount /dev/xvdba /mnt/test
```

---

### 案例 2: StatefulSet 扩容时 PVC 无法创建 (ProvisioningFailed)

#### **现象**

```bash
kubectl scale sts db --replicas=5

# 新 Pod 一直 Pending
kubectl get pod db-4
# NAME   READY   STATUS    RESTARTS   AGE
# db-4   0/1     Pending   0          2m

kubectl get pvc data-db-4
# NAME         STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS
# data-db-4    Pending                                      fast-ssd
```

#### **排查步骤**

```bash
# 1. 检查 PVC 事件
kubectl describe pvc data-db-4

# 输出:
# Warning  ProvisioningFailed  persistentvolume-controller
# Failed to provision volume: rpc error: ResourceExhausted: 
# You have exceeded your maximum gp3 volume limit

# 2. 检查云配额
aws service-quotas get-service-quota \
  --service-code ebs \
  --quota-code L-D18FCD1D

# 输出:
# Value: 5000 (已达上限)
```

#### **解决方案**

**临时方案**: 清理未使用卷

```bash
# 查找 Released 状态的 PV
kubectl get pv | grep Released

# 删除
kubectl delete pv <pv-name>

# 删除云卷
aws ec2 describe-volumes --filters "Name=status,Values=available"
aws ec2 delete-volume --volume-id vol-xxx
```

**长期方案**: 提高配额

```bash
# 请求提高配额到 10000
aws service-quotas request-service-quota-increase \
  --service-code ebs \
  --quota-code L-D18FCD1D \
  --desired-value 10000
```

---

### 案例 3: PVC 扩容后容器内空间未增加

#### **现象**

```bash
# 扩容 PVC
kubectl patch pvc data-db-0 -p '{"spec":{"resources":{"requests":{"storage":"20Gi"}}}}'

# PVC 显示已扩容
kubectl get pvc data-db-0
# NAME        STATUS   VOLUME      CAPACITY   ...
# data-db-0   Bound    pvc-abc123  20Gi       ...

# 但容器内仍是 10 GB
kubectl exec db-0 -- df -h /var/lib/postgresql/data
# /dev/xvdba  10G  9.5G  500M  96% /data  ❌
```

#### **排查步骤**

```bash
# 1. 检查 PVC 事件
kubectl get events --field-selector involvedObject.name=data-db-0

# 输出:
# Normal  VolumeResizeSuccessful     external resizer succeeded
# Normal  FileSystemResizeRequired   require file system resize

# 2. 诊断: 文件系统扩容未触发 (需要 Pod 重启)
```

#### **解决方案**

```bash
# 滚动重启 StatefulSet
kubectl rollout restart sts db

# 等待 Pod 重启
kubectl rollout status sts db

# 验证扩容
kubectl exec db-0 -- df -h /var/lib/postgresql/data
# /dev/xvdba  20G  9.5G  10.5G  48% /data  ✅
```

---

### 案例 4: PVC 删除后卷未自动删除 (VolumeFailedDelete)

#### **现象**

```bash
# 删除 PVC
kubectl delete pvc data-test

# PVC 卡在 Terminating
kubectl get pvc
# NAME        STATUS        VOLUME      ...
# data-test   Terminating   pvc-xyz789  ...

# PV 也无法删除
kubectl get pv pvc-xyz789
# NAME         STATUS     CLAIM   ...
# pvc-xyz789   Released   ...
```

#### **排查步骤**

```bash
# 1. 检查 PV 事件
kubectl describe pv pvc-xyz789

# 输出:
# Warning  VolumeFailedDelete  persistentvolume-controller
# Failed to delete volume: VolumeInUse: Volume is currently attached

# 2. 检查云卷状态
aws ec2 describe-volumes --volume-ids vol-xyz789

# 输出:
# "State": "in-use",
# "Attachments": [{"InstanceId": "i-terminated-instance"}]  ❌
```

#### **解决方案**

```bash
# 1. 强制 Detach 卷
aws ec2 detach-volume --volume-id vol-xyz789 --force

# 2. 等待 5 分钟,PV Controller 自动重试删除
# 或手动删除:
aws ec2 delete-volume --volume-id vol-xyz789

# 3. 移除 PV finalizer
kubectl patch pv pvc-xyz789 -p '{"metadata":{"finalizers":null}}'

# 4. 删除 PV 和 PVC 对象
kubectl delete pv pvc-xyz789
kubectl delete pvc data-test --grace-period=0 --force
```

---

## 九、生产环境最佳实践

### 9.1 StorageClass 配置最佳实践

#### **推荐配置**

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
  annotations:
    storageclass.kubernetes.io/is-default-class: "false"  # ❌ 避免设置默认 StorageClass
provisioner: ebs.csi.aws.com
parameters:
  type: gp3               # ✅ 使用 gp3 (比 gp2 性价比高)
  iops: "3000"            # ✅ 设置合理 IOPS
  throughput: "125"       # ✅ 设置吞吐量
  encrypted: "true"       # ✅ 启用加密
  fsType: ext4            # ✅ 明确指定文件系统
reclaimPolicy: Delete     # ✅ 自动删除卷 (避免泄漏)
volumeBindingMode: WaitForFirstConsumer  # ✅ 延迟绑定 (避免跨 AZ)
allowVolumeExpansion: true  # ✅ 支持在线扩容
```

#### **生产环境分级 StorageClass**

```yaml
# 高性能数据库卷
---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: db-ssd-high-performance
provisioner: ebs.csi.aws.com
parameters:
  type: io2
  iops: "64000"  # 最高性能
  throughput: "1000"
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

# 通用应用卷
---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: general-purpose
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

# 低成本归档卷
---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: archive-hdd
provisioner: ebs.csi.aws.com
parameters:
  type: sc1  # 冷 HDD
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: false
```

---

### 9.2 卷监控与告警

#### **关键指标**

```yaml
# Prometheus 监控规则
groups:
- name: storage
  rules:
  # PVC 长时间 Pending
  - alert: PVCPendingTooLong
    expr: |
      kube_persistentvolumeclaim_status_phase{phase="Pending"} == 1
      and
      time() - kube_persistentvolumeclaim_created > 300
    for: 5m
    annotations:
      summary: "PVC {{ $labels.persistentvolumeclaim }} Pending 超过 5 分钟"

  # 卷附加失败
  - alert: VolumeAttachFailed
    expr: |
      increase(kubelet_volume_manager_attach_errors_total[5m]) > 0
    annotations:
      summary: "节点 {{ $labels.node }} 卷附加失败"

  # 卷挂载失败
  - alert: VolumeMountFailed
    expr: |
      increase(kubelet_volume_manager_mount_errors_total[5m]) > 0
    annotations:
      summary: "节点 {{ $labels.node }} 卷挂载失败"

  # 节点卷数接近上限
  - alert: NodeVolumeLimitNearExceeded
    expr: |
      (
        kubelet_volume_stats_used_bytes /
        kubelet_volume_stats_capacity_bytes
      ) > 0.85
    annotations:
      summary: "PVC {{ $labels.persistentvolumeclaim }} 使用率 > 85%"

  # PV 删除失败 (泄漏)
  - alert: PVDeleteFailed
    expr: |
      kube_persistentvolume_status_phase{phase="Failed"} == 1
    for: 1h
    annotations:
      summary: "PV {{ $labels.persistentvolume }} 删除失败超过 1 小时"
```

---

### 9.3 卷性能优化

#### **EBS 卷优化 (AWS)**

```yaml
# 1. 使用 gp3 代替 gp2 (性价比高 20%)
parameters:
  type: gp3
  iops: "3000"      # gp2 只有 3000 IOPS (100 GB 卷)
  throughput: "125" # gp2 无此参数

# 2. 数据库工作负载使用 io2
parameters:
  type: io2
  iops: "64000"  # 最高 64000 IOPS

# 3. 启用 EBS 优化实例
# 选择 EBS-Optimized 实例类型 (如 m5.large)
```

#### **卷 I/O 性能测试**

```bash
# 进入 Pod
kubectl exec -it <pod> -- bash

# 顺序写性能
dd if=/dev/zero of=/data/test bs=1M count=1024 oflag=direct
# 1024+0 records in
# 1024+0 records out
# 1073741824 bytes (1.1 GB) copied, 8.5 s, 126 MB/s  ✅

# 随机写性能
fio --name=randwrite --ioengine=libaio --iodepth=32 \
    --rw=randwrite --bs=4k --direct=1 \
    --size=1G --numjobs=4 --runtime=60 \
    --filename=/data/test
```

---

### 9.4 卷容量管理

#### **自动扩容策略**

```yaml
# 使用 Prometheus Operator 自动扩容
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: auto-expand-pvc
spec:
  groups:
  - name: storage
    rules:
    - alert: PVCUsageHigh
      expr: |
        (kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes) > 0.85
      for: 5m
      annotations:
        action: |
          kubectl patch pvc {{ $labels.persistentvolumeclaim }} \
            -p '{"spec":{"resources":{"requests":{"storage":"{{ $value | humanize }}"}}}}'
```

#### **容量规划**

```bash
# 查看所有 PVC 使用率
kubectl get pvc -A -o json | jq -r '
  .items[] | 
  select(.status.phase == "Bound") | 
  "\(.metadata.namespace)/\(.metadata.name): \(.status.capacity.storage)"
'

# 统计总卷容量
kubectl get pv -o json | jq '[.items[].spec.capacity.storage | rtrimstr("Gi") | tonumber] | add'
```

---

### 9.5 卷备份与恢复

#### **使用 VolumeSnapshot (CSI)**

```yaml
# 1. 创建 VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: ebs-snapshot-class
driver: ebs.csi.aws.com
deletionPolicy: Delete  # 或 Retain (保留快照)

---
# 2. 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: db-backup-20260210
spec:
  volumeSnapshotClassName: ebs-snapshot-class
  source:
    persistentVolumeClaimName: data-db-0

---
# 3. 从快照恢复
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-db-0-restored
spec:
  dataSource:
    name: db-backup-20260210
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 20Gi
```

#### **定时备份 CronJob**

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: daily-db-snapshot
spec:
  schedule: "0 2 * * *"  # 每天凌晨 2 点
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-creator
          containers:
          - name: create-snapshot
            image: bitnami/kubectl:latest
            command:
            - /bin/sh
            - -c
            - |
              kubectl create -f - <<EOF
              apiVersion: snapshot.storage.k8s.io/v1
              kind: VolumeSnapshot
              metadata:
                name: db-backup-$(date +%Y%m%d)
              spec:
                volumeSnapshotClassName: ebs-snapshot-class
                source:
                  persistentVolumeClaimName: data-db-0
              EOF
          restartPolicy: OnFailure
```

---

### 9.6 多租户存储隔离

#### **使用 ResourceQuota 限制存储**

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: storage-quota
  namespace: team-a
spec:
  hard:
    requests.storage: 100Gi  # 最多 100 GB
    persistentvolumeclaims: 10  # 最多 10 个 PVC
    <storageclass-name>.storageclass.storage.k8s.io/requests.storage: 50Gi  # 按 StorageClass 限制
```

#### **使用 LimitRange 限制 PVC 大小**

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: pvc-limit
  namespace: team-a
spec:
  limits:
  - type: PersistentVolumeClaim
    max:
      storage: 50Gi  # 单个 PVC 最大 50 GB
    min:
      storage: 1Gi   # 单个 PVC 最小 1 GB
```

---

### 9.7 故障恢复剧本

#### **卷附加失败 (VolumeInUse)**

```bash
# 1. 确认原 Pod 已删除
kubectl get pod -A -o wide | grep <node-name>

# 2. 强制删除旧 Pod (如果仍存在)
kubectl delete pod <pod> --grace-period=0 --force

# 3. 检查云卷附加状态
aws ec2 describe-volumes --volume-ids <volume-id>

# 4. 如果卷附加到已终止节点,强制 Detach
aws ec2 detach-volume --volume-id <volume-id> --force

# 5. 等待 30s,观察 Pod 是否自动恢复
kubectl get pod <pod> -w
```

#### **卷挂载失败 (FailedMount)**

```bash
# 1. 检查事件详细错误
kubectl describe pod <pod> | grep -A 10 FailedMount

# 2. 如果是文件系统错误
ssh <node>
lsblk | grep <device>
file -s /dev/<device>

# 3. 尝试手动挂载
mount /dev/<device> /mnt/test
# 如果失败,检查 dmesg
dmesg | tail -50

# 4. 修复文件系统
e2fsck -f /dev/<device>

# 5. 删除 Pod 重新挂载
kubectl delete pod <pod>
```

#### **PVC 无法删除 (Terminating)**

```bash
# 1. 检查是否有 Pod 使用 PVC
kubectl get pod -A -o json | \
  jq -r '.items[] | select(.spec.volumes[]?.persistentVolumeClaim.claimName == "<pvc-name>") | .metadata.name'

# 2. 删除使用 PVC 的 Pod
kubectl delete pod <pod>

# 3. 如果仍无法删除,移除 finalizer
kubectl patch pvc <pvc-name> -p '{"metadata":{"finalizers":null}}'

# 4. 强制删除
kubectl delete pvc <pvc-name> --grace-period=0 --force
```

---

## 十、相关文档参考

### 10.1 相关事件文档

| 文档 | 相关事件 |
|:---|:---|
| [02 - Pod 与容器生命周期事件](./02-pod-container-lifecycle-events.md) | `FailedMount` → Pod 无法启动 |
| [05 - 调度与抢占事件](./05-scheduling-preemption-events.md) | `WaitForFirstConsumer` → 延迟绑定等待调度 |
| [06 - 节点生命周期事件](./06-node-lifecycle-condition-events.md) | `FailedAttachVolume` → 节点异常导致 Attach 失败 |

### 10.2 Kubernetes 官方文档

- [Persistent Volumes](https://kubernetes.io/docs/concepts/storage/persistent-volumes/)
- [CSI Volume Plugins](https://kubernetes.io/docs/concepts/storage/volumes/#csi)
- [Storage Classes](https://kubernetes.io/docs/concepts/storage/storage-classes/)
- [Volume Snapshots](https://kubernetes.io/docs/concepts/storage/volume-snapshots/)

### 10.3 CSI 驱动文档

- [AWS EBS CSI Driver](https://github.com/kubernetes-sigs/aws-ebs-csi-driver)
- [Azure Disk CSI Driver](https://github.com/kubernetes-sigs/azuredisk-csi-driver)
- [GCE PD CSI Driver](https://github.com/kubernetes-sigs/gcp-compute-persistent-disk-csi-driver)

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 11/15
