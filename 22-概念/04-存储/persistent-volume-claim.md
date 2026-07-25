---
title: PersistentVolumeClaim
summary: PersistentVolumeClaim（PVC）是 Kubernetes 中用户对持久化存储的声明式请求。Pod 通过挂载 PVC 来使用存储，而无需关心底层存储的具体实现。
category: concepts
tags:
- core-concept
- 存储
- visibility/public
tier: core
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-07
last_updated: 2026-07
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PersistentVolumeClaim

`PersistentVolumeClaim`（PVC）是 Kubernetes 中用户对持久化存储的声明式请求。Pod 通过挂载 PVC 来使用存储，而无需关心底层存储的具体实现。

## PVC 与 PV 的关系

PVC 与 `PersistentVolume`（PV）之间是**声明式绑定**关系：

- **PV** 由管理员或动态供给器创建，代表集群中的实际存储资源
- **PVC** 由用户创建，描述所需的存储规格
- Kubernetes 控制平面负责将满足条件的 PVC 与 PV 进行绑定

绑定完成后，PVC 进入 `Bound` 状态，可被 Pod 通过 `persistentVolumeClaim` 卷类型挂载。

## StorageClass 动态供给

当集群配置了 [[storage-classes]] 时，PVC 可以触发**动态供给**（Dynamic Provisioning）：

1. 用户在 PVC 中指定 `storageClassName`
2. 若不存在匹配的 PV，对应的 StorageClass 会调用 Provisioner 自动创建 PV
3. 新创建的 PV 与 PVC 自动绑定

常见 Provisioner 包括：

- `kubernetes.io/aws-ebs`
- `kubernetes.io/gce-pd`
- `kubernetes.io/azure-disk`
- 阿里云 `diskplugin.csi.alibabacloud.com`

## PVC 关键字段

| 字段 | 说明 |
|------|------|
| `accessModes` | 访问模式：`ReadWriteOnce`（单节点读写）、`ReadOnlyMany`（多节点只读）、`ReadWriteMany`（多节点读写） |
| `resources.requests.storage` | 请求的存储容量，如 `10Gi` |
| `storageClassName` | 指定使用的 StorageClass，空字符串表示使用默认类，不设置则不使用动态供给 |
| `volumeName` | 直接绑定到指定 PV（通常不推荐） |
| `selector` | 通过标签选择匹配的 PV |

## 绑定条件

PVC 与 PV 成功绑定需同时满足：

1. **容量匹配**：PV 的 `capacity` ≥ PVC 的 `resources.requests.storage`
2. **访问模式匹配**：PV 的 `accessModes` 包含 PVC 请求的访问模式
3. **StorageClass 匹配**：PV 的 `storageClassName` 与 PVC 的 `storageClassName` 一致（若 PVC 未设置，则匹配空类 PV）
4. **标签选择器匹配**（若 PVC 设置了 `selector`）

## 阿里云 ACK 存储

在阿里云 ACK 集群中，常见的 PVC 类型包括：

- **云盘 PVC**：基于 ESSD/SSD 云盘，支持 `ReadWriteOnce`，适合数据库等有状态应用
- **NAS PVC**：基于文件存储 NAS，支持 `ReadWriteMany`，适合共享存储场景
- **OSS PVC**：基于对象存储 OSS，适合静态资源、日志归档等大容量场景

不同存储类型的 IOPS、吞吐量和可用区限制各不相同，选型时需结合业务需求。

## 远程顾问诊断要点

PVC 一直停留在 `Pending` 状态是存储类问题的典型表现，诊断思路如下：

- **检查 StorageClass**：确认 `storageClassName` 是否拼写正确，对应的 StorageClass 是否存在且为默认类
- **检查可用 PV**：若未启用动态供给，确认是否有未绑定的 PV 满足容量和访问模式要求
- **检查 Zone 匹配**：云盘类存储通常有可用区约束，确保 Pod 调度节点与 PV/StorageClass 的可用区一致
- **检查资源配额**：确认命名空间的 `ResourceQuota` 是否限制了 PVC 或存储类的使用
- **查看事件**：`kubectl describe pvc <name>` 中的 Events 通常会给出具体失败原因

## 技术深度解析

### PV-PVC 绑定机制

Kubernetes 的 PV-PVC 绑定由 `PersistentVolumeController` 异步执行，绑定过程：

```
1. 用户创建 PVC（指定 capacity, accessModes, storageClassName）
2. PV Controller 检测到新 PVC
   → 如果 PVC 设置了 storageClassName: 触发动态供给流程
   → 如果 PVC 未设置 storageClassName: 查找匹配的静态 PV
3. 动态供给: StorageClass 的 Provisioner 调用云 API 创建存储卷
   → 创建成功: 生成新 PV 对象
   → 创建失败: PVC 保持 Pending，记录失败事件
4. PV Controller 将 PV 和 PVC 绑定（设置 claimRef）
5. PVC 状态变为 Bound
```

### CSI 快照机制

VolumeSnapshot 是 CSI 规范定义的存储快照能力：

```yaml
# 创建 VolumeSnapshot
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: db-snapshot-daily
spec:
  volumeSnapshotClassName: csi-snapshot-class
  source:
    persistentVolumeClaimName: database-pvc
---
# 从快照创建新 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: database-restored
spec:
  accessModes: ["ReadWriteOnce"]
  resources:
    requests:
      storage: 100Gi
  storageClassName: fast-ssd
  dataSource:
    name: db-snapshot-daily
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

### 生产 PVC 定义示例

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: database-storage
  labels:
    app: postgres
spec:
  accessModes:
    - ReadWriteOnce                   # 块存储仅支持 RWO
  storageClassName: fast-ssd          # 高性能 SSD StorageClass
  resources:
    requests:
      storage: 200Gi
```

## 最佳实践

- **为有状态应用使用 StatefulSet + volumeClaimTemplates**：StatefulSet 自动为每个 Pod 创建独立 PVC，确保数据隔离和稳定的 PVC 命名
- **选择正确的 accessMode**：数据库等需要独占访问的使用 RWO；文件共享场景使用 RWX（需 NAS/NFS 支持）；不要对块存储 PVC 请求 RWX
- **设置合理的容量**：PVC 容量一旦创建通常不可缩减（部分 CSI 驱动支持扩容），建议预留 30% 增长空间
- **配置 StorageClass 的 volumeBindingMode: WaitForFirstConsumer**：确保 PV 创建在 Pod 调度的节点所在可用区——避免跨区存储延迟
- **定期创建 VolumeSnapshot**：配合 Velero 实现定时快照备份，RPO 控制在 24 小时以内

## 常见陷阱

- **PVC Pending 因可用区不匹配**：云盘通常绑定特定可用区，如果 Pod 被调度到不同 AZ 的节点，PVC 无法挂载——使用 `WaitForFirstConsumer` 绑定模式
- **PVC 扩容需要存储驱动支持**：不是所有 CSI 驱动都支持在线扩容——扩容前确认 StorageClass 的 `allowVolumeExpansion: true`
- **删除 PVC 不会自动删除 PV**：默认 PV reclaimPolicy 为 Retain，删除 PVC 后 PV 变为 Released 状态但数据仍在——需要手动清理或设置 Delete 策略

更多存储排错方法请参考 [[19-故障诊断/02-资源排障/14-pvc-storage-troubleshooting.md|pvc-storage-troubleshooting]]，备份恢复策略参见 [[22-概念/04-存储/data-protection-k8s.md|data-protection-k8s]]。

## 源码实现分析

### PV Controller 绑定流程

```go
// k8s.io/kubernetes/pkg/controller/volume/persistentvolume/pv_controller.go
// PVC 绑定核心逻辑
func (ctrl *PersistentVolumeController) bind(ctx context.Context, volume *v1.PersistentVolume, claim *v1.PersistentVolumeClaim) error {
    // 1. 检查 PV 和 PVC 是否匹配（storageClass + accessModes + capacity）
    if !ctrl.isVolumeMatchToClaim(volume, claim) {
        return nil // 不匹配，跳过
    }
    
    // 2. 更新 PV.ClaimRef 指向 PVC
    volume.Spec.ClaimRef = &v1.ObjectReference{
        Kind: "PersistentVolumeClaim",
        Name: claim.Name, Namespace: claim.Namespace,
        UID:  claim.UID,
    }
    
    // 3. 更新 PVC.Status.Phase = Bound
    claim.Status.Phase = v1.ClaimBound
    claim.Spec.VolumeName = volume.Name
    // 绑定后 PVC 不可再修改 spec（immutable）
}
```

```
┌─────────────────────────────────────────────────────────┐
│           PVC 绑定与动态供给流程                        │
├─────────────────────────────────────────────────────────┤
│  PVC Created (Pending)                                  │
│       │                                                 │
│       ├─── 静态绑定 ──▶ 匹配已有 PV (storageClass匹配)  │
│       │                                                 │
│       └─── 动态供给 ──▶ Provisioner Watch PVC           │
│                │                                        │
│                ▼                                        │
│         CSI CreateVolume RPC                            │
│                │                                        │
│                ▼                                        │
│         PV Created ──▶ PVC Bound ──▶ Pod Mount          │
│                                                         │
│  关键事件: ProvisioningSucceeded / FailedBinding         │
└─────────────────────────────────────────────────────────┘
```

### CSI 驱动挂载路径

```go
// k8s.io/kubernetes/pkg/volume/csi/csi_attacher.go
// CSI 卷挂载到节点
func (c *csiAttacher) Attach(spec *volume.Spec, nodeName types.NodeName) (string, error) {
    // 1. 调用 CSI Controller: ControllerPublishVolume
    // 云盘 attach 到 VM（如 AWS EBS AttachVolume）
    
    // 2. kubelet 调用 NodeStageVolume（格式化 + 挂载到全局目录）
    // /var/lib/kubelet/plugins/kubernetes.io/csi/pv/<pv-name>/globalmount
    
    // 3. kubelet 调用 NodePublishVolume（bind mount 到 Pod 目录）
    // /var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~csi/<pv-name>/mount
}
```

### 生产运维：PVC 故障诊断

```bash
# 🟢 检查 PVC 状态和事件
kubectl get pvc -A | grep -v Bound
kubectl describe pvc <name> -n <ns>  # 查看 Events

# 🟢 检查 PV 状态
kubectl get pv | grep -E "Released|Failed|Available"

# 🟡 强制删除卡在 Terminating 的 PVC（确认 Pod 已停止）
kubectl patch pvc <name> -n <ns> -p '{"metadata":{"finalizers":null}}'
# 🔴 强制删除可能导致数据丢失，必须先确认无 Pod 使用

# 🟢 检查 CSI 驱动状态
kubectl get csidrivers
kubectl get csinodes
kubectl logs -n kube-system -l app=csi-provisioner --tail=50
```

## 面试要点

1. **PVC Pending 的常见原因有哪些？**
   - StorageClass 不存在或拼写错误
   - WaitForFirstConsumer 模式下 Pod 未调度（PV 等待 Pod 确定节点）
   - 动态供给失败（CSI Provisioner 日志查看具体错误）
   - 容量/访问模式不匹配（静态绑定时）

2. **PV 的 reclaimPolicy 有什么区别？**
   - Delete：PVC 删除后自动删除 PV 和底层存储（云盘被删除）
   - Retain：PVC 删除后 PV 变为 Released，数据保留需手动清理
   - Recycle：已弃用，仅 NFS 场景使用
   - 生产建议：重要数据用 Retain + 定期快照

3. **CSI 驱动的 Attach/Mount 流程是什么？**
   - ControllerPublishVolume：云盘 attach 到节点（类似 AWS AttachVolume）
   - NodeStageVolume：格式化 + 挂载到全局目录（每节点一次）
   - NodePublishVolume：bind mount 到 Pod 目录（每 Pod 一次）
   - 故障排查：检查 CSINode 注册、kubelet CSI socket、云 API 配额

4. **WaitForFirstConsumer 和 Immediate 绑定模式的区别？**
   - Immediate：PVC 创建即绑定 PV，可能跨可用区导致挂载失败
   - WaitForFirstConsumer：等 Pod 调度确定后再绑定，保证同 AZ
   - 云环境必须用 WaitForFirstConsumer，本地存储可用 Immediate

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
