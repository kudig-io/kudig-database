---
title: PV/PVC 供给·绑定·生命周期深挖
summary: 面向面试与排障的 PV/PVC 全生命周期深挖笔记——静态/动态供给、绑定与保护 finalizer、CSI 挂载链路、卷拓扑、在线扩容、快照恢复、删除回收全链路。
category: concepts
tags:
- storage
- pv
- pvc
- csi
- lifecycle
- interview
- visibility/public
tier: core
sources:
- 36-报告/k8s-高级面试快问快答-沉淀.md
created: 2026-10-20
updated: 2026-10-20
last_updated: 2026-10
status: stable
difficulty: advanced
reading_level: advanced
audience:
- K8s 面试准备者
- SRE
- 平台工程师
estimated_read_time: 25min
intent_queries:
- PV 静态供给和动态供给怎么选
- PVC 绑定机制细节
- CSI Attach Stage Publish 链路
- PVC 删除卡 Terminating 怎么排查
- PVC 在线扩容原理
- VolumeSnapshot 快照恢复
trigger_keywords:
- PV PVC
- StorageClass
- 动态供给
- WaitForFirstConsumer
- VolumeAttachment
- finalizer
- 扩容
- 快照
k8s_versions:
- '1.27'
- '1.29'
- '1.30'
- '1.33'
---

# PV/PVC 供给·绑定·生命周期深挖

## 概述

本文是 [[22-概念/04-存储/pv.md|PV]] 与 [[22-概念/04-存储/persistent-volume-claim.md|PVC]] 基础笔记的进阶篇：把"PV 由管理员静态创建或 StorageClass 动态供给"这句话展开为**全生命周期**——供给、绑定、挂载、扩容、快照、删除回收，每一环都给出机制、卡点与面试答法。配套快问快答见 `36-报告/k8s-高级面试快问快答-沉淀.md` Q48–Q57（基础）与 Q79–Q86（进阶）。

```
供给(静态/动态) → 绑定(PV Controller) → 调度(拓扑感知)
  → Attach(AD Controller) → Stage/Publish(kubelet+CSI)
  → 使用(扩容/快照/监控) → 删除(finalizer 保护 → reclaimPolicy)
```

## 一、供给两条路：静态 vs 动态

**本质区别只有一个：底层卷谁来建、什么时候建。**

### 静态供给——存储已存在，K8s 只做"登记"

管理员把**真实存在的存储**（NFS export、云盘、本地盘、Ceph image）手工写成 PV 对象，集群不创建任何底层卷。之后 PVC 由 PV Controller 按**四条件**（容量 ≥、accessModes 包含、storageClassName 一致、selector 匹配）与现成 PV 匹配绑定。**先有卷后有申请**。

静态至今活着的三块自留地：

1. **local PV（本地盘）**：必须静态——`spec.local.path` + `nodeAffinity` 锁节点，盘的拓扑只有人知道。`local-volume-provisioner`（DaemonSet）可半自动发现挂载点生成 PV，但底层盘仍需人工铺设。代价：节点挂 = PV 永久不可用。
2. **已有存储资产**：公司 NAS/旧 SAN LUN，生命周期在集群外，K8s 只做登记 + 挂载。
3. **特殊参数卷**：云 API 表达不了的预配置卷（预格式化、专属实例盘）。

```yaml
# 静态 PV 双要素：volume source + nodeAffinity（local 卷必配）
spec:
  capacity: {storage: 500Gi}
  accessModes: [ReadWriteOnce]
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-storage
  local: {path: /mnt/ssd}
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - {key: kubernetes.io/hostname, operator: In, values: [node-1]}
```

### 动态供给——PVC 触发，provisioner 现场建卷

三要素：**StorageClass**（类目/模板）、**CSI 驱动**、**PVC 指向 SC**。

```
用户建 PVC（storageClassName: fast-ssd）
  → PV Controller 发现新 PVC
  → external-provisioner（sidecar，watch PVC）接单
  → 调 CSI CreateVolume RPC → 云 API 建卷
  → 自动创建 PV（名字固定 pvc-<pvc-uid>，claimRef 直指 PVC，跳过匹配）
  → PVC 转 Bound → 等 Pod 调度后再走挂载链路
```

关键细节：

- **PVC 不写 storageClassName ≠ 没有动态供给**：匹配打了 `storageclass.kubernetes.io/is-default-class: "true"` 注解的默认 SC；没有默认 SC 才 Pending。
- **`pvc-<uid>` 命名**：provisioner 按绑定目标命名，天然防重名、可追溯归属——看到这名字即动态卷。
- **reclaimPolicy / volumeBindingMode 继承自 SC**：不写 reclaimPolicy 默认 **Delete**。
- **parameters 透传云 API**：如 ACK 的 `type: cloud_essd`、`performanceLevel: PL1`。

### 选型口径（45 秒答法）

云环境**一律动态** + `WaitForFirstConsumer`（先调度后供卷，保证卷与 Pod 同 AZ），关键数据 SC 显式 `reclaimPolicy: Retain`；本地盘、NFS 等生命周期在集群外的存储走静态。混合集群两条路并存：数据库云盘动态供给，日志/共享目录 NAS 静态。

**一句话记忆：静态 = 先有卷等人认领，动态 = 先有单子再造卷。**

## 二、绑定机制细节

### 双向指针与不可变约束

绑定由 `persistentvolume-controller` 异步执行，命中四条件后写**双向指针**：PV 的 `spec.claimRef` 指向 PVC，PVC 的 `spec.volumeName` 指向 PV，phase 双双转 Bound。**绑定后 PVC 的 spec 不可再改（immutable）**——storageClassName、capacity（只能加大，见扩容）、accessModes 全部锁死，换卷只能新建 PVC 迁数据。

PVC 侧两个直通开关：`volumeName` 直绑指定 PV（运维迁移常用）；`selector` 按 label 筛 PV。

### storageClassName 匹配语义（高频追问）

| PV 侧 storageClassName | PVC 侧 | 能否绑定 |
|---|---|---|
| `nfs-static` | `nfs-static` | ✅ |
| `""`（空串） | 不写 | ✅（空类只匹配空类） |
| 不写 | 不写 | ✅（同为"无类"） |
| `nfs-static` | 不写 | ❌（PVC 会去找默认 SC） |

静态 PV 写 SC 名只是**匹配凭据**，不代表动态供给——没有 provisioner 就不会建新卷。

### 保护 finalizer：删除不是即刻生效

- **`kubernetes.io/pvc-protection`**：PVC 被任何 Pod 使用时自动加上；删除 PVC 会卡在 Terminating，直到所有使用它的 Pod 消失。
- **`kubernetes.io/pv-protection`**：PV 绑定期间同理，防止 PV 在使用中被删。

这是"删了 PVC 但卷还在被 Pod 挂着"的机制保障，也是排查 PVC/PV 卡 Terminating 的第一入口。

### Released PV 的三种复活方式

reclaimPolicy=Retain 下删 PVC，PV 转 Released（数据完好，黄金恢复窗口）：

1. **原卷接回**：新建同规格 PVC 写 `volumeName` 直绑旧 PV（注意先清 PV 的 claimRef 才能接受新 claim）。
2. **重新上架**：`kubectl patch pv pvc-xxx --type=json -p='[{"op":"remove","path":"/spec/claimRef"}]'` 让 PV 回 Available（数据仍在底层卷，风险：可能被别人的 PVC 匹配走）。
3. **导出后废弃**：起临时 Pod 挂卷备份数据，再删 PV 连底层卷。

## 三、挂载链路：从调度到 Pod 内可见

Pod 调度之后，卷要过**三道门**，卡在哪道门看什么报错：

| 阶段 | 执行者 | CSI RPC | 典型报错（Pod Events） |
|------|--------|---------|----------------------|
| Attach：卷挂到节点 | AD Controller（KCM 内）建 VolumeAttachment 对象 → CSI external-attacher | ControllerPublishVolume | `FailedAttachVolume` / `Multi-Attach error` |
| Stage：格式化+挂到节点全局目录 | kubelet（每节点每卷一次） | NodeStageVolume | `FailedMount`（超时反复重试） |
| Publish：bind mount 进 Pod 目录 | kubelet（每 Pod 一次） | NodePublishVolume | `FailedMount` |

- **Multi-Attach** 只发生在 Attach 层：RWO 卷已挂在节点 A，Pod 却调度到节点 B——常见根因是节点假死未 detach 强制调度、或 StatefulSet 多副本共享 RWO PVC。
- VolumeAttachment 对象卡住（`attacher-kubernetes.io/csi-*` finalizer）：节点假死 detach 不下来，需 force detach（删 VolumeAttachment 让 attacher 重试，云盘侧确认无残留挂载）。
- 详细机制见 [[22-概念/04-存储/01-volume-attachment-controller.md|VolumeAttachment 与 Attach/Detach 控制器]]。

## 四、卷拓扑与容量感知调度

- **拓扑键**：CSI 驱动经 `NodeGetInfo` 在各节点的 CSINode 上注册拓扑键（如 `topology.kubernetes.io/zone`），CreateVolume 时携带 `accessible_topology`。
- **WaitForFirstConsumer 的底层**：调度器先定 Pod 落点，把拓扑需求传给 provisioner，卷建在 Pod 所在 AZ——从根上消灭"Immediate 模式建卷在 AZ-a、Pod 调到 AZ-b 挂不上"。
- **静态卷拓扑**：local PV 的 nodeAffinity 直接参与调度过滤，无可用节点则 Pod Pending。
- **CSIStorageCapacity**：SC 开启后，调度器可感知各拓扑域剩余容量，避免"调过去才发现建不出卷"。详见 [[22-概念/04-存储/02-csi-storage-capacity-tracking.md|CSIStorageCapacity 与存储容量感知调度]]。

## 五、在线扩容链路

前提：SC `allowVolumeExpansion: true`（后开对存量 PVC 同样生效）。

```
kubectl patch pvc 调大 requests.storage
  → CSI ControllerExpandVolume（扩云盘，控制面）
  → NodeExpandVolume（扩文件系统，节点面）
      ├─ 在线：多数驱动直接 resize2fs/xfs_growfs
      └─ 离线：Event 显示 FileSystemResizePending，Pod 重启后扩展
```

- PVC 的 storage 是 immutable 约束里**只能加大**的方向，扩容不可逆，**缩容一律不支持**（想缩 = 新卷 + 迁移 + 切流）。
- 容量规划：初始给足 + 预留 ~30% 增长。
- 排障：卡 `Resizing` 看 csi-provisioner / node plugin 日志（云盘配额、驱动不支持）。

## 六、快照与恢复（VolumeSnapshot）

CRD 三件套（external-snapshotter 提供）：

| 对象 | 作用域 | 角色 | 类比 |
|------|--------|------|------|
| VolumeSnapshot | namespace | 用户声明"给这个 PVC 拍快照" | PVC |
| VolumeSnapshotContent | cluster | 快照的底层实体 | PV |
| VolumeSnapshotClass | cluster | 驱动 + 参数（deletionPolicy） | StorageClass |

- 动态快照：CSI `CreateSnapshot`；预置快照：`source.volumeHandle` 直接引用。
- **恢复**：新 PVC 写 `dataSource`（kind: VolumeSnapshot）→ CSI CreateVolume 时携带快照内容。
- **一致性**：快照是崩溃一致性（crash-consistent）；数据库要应用一致需先暂停写入/fsfreeze 或依赖 WAL。deletionPolicy 选 Retain 防误删快照内容。
- 与 Velero 的关系：Velero 用 VolumeSnapshot 做集群级备份恢复编排；裸 CSI 快照只管卷不管对象。

## 七、删除与回收全链路

```
kubectl delete pvc
  → 有 pvc-protection finalizer 且 Pod 还在用 → 卡 Terminating（等 Pod 消失）
  → finalizer 清除，PVC 删除，PV 转 Released
      ├─ Retain：PV 留着等人工处置（数据在）
      └─ Delete：PV 加 deletionTimestamp
            → external-provisioner 发现 claimRef 指向已消失的 PVC
            → CSI DeleteVolume（删底层云盘）→ PV 对象删除
```

卡 Terminating 的三个层次：①Pod 还在用（正常等待，先删 Pod）；②finalizer 残留（确认无使用后 patch 清空 finalizers——**先确认数据不要了**）；③底层卷删除失败（云侧资源锁/依赖，看 provisioner 日志）。

## 八、进阶特性

### RWOP（ReadWriteOncePod）：1.22 alpha → 1.27 beta 默认开启 → 1.29 GA

RWO 约束的是**节点**（同节点多 Pod 可共享），RWOP 收紧到**全集群单 Pod**——第二个 Pod 挂载同一 PVC 会被调度器和 kubelet 拒绝。适合独占语义场景（单写者数据库、加密卷）。要求所有节点 kubelet 版本 ≥1.29 才能用 GA 语义。

### Generic Ephemeral Volume：PVC 的"随 Pod 生死"形态

`spec.volumes[].ephemeral.volumeClaimTemplate`：Pod 启动时自动建 PVC（动态供给），Pod 删除时 PVC 跟着删。与 emptyDir 区别：**支持全部 PVC 能力**（动态供给、扩容、快照、任意 SC）；与普通 PVC 区别：生命周期随 Pod，不需要手工管理。典型：作业型任务要真持久卷但不留遗产。

### fsGroup 与挂载权限

容器内"Permission denied"十有八九是 UID/GID 与卷上文件属主不匹配：`spec.securityContext.fsGroup` 让 kubelet 递归 chgrp 挂载目录（大目录首次挂载慢），`fsGroupChangePolicy: OnRootMismatch` 优化。NFS 的 root squash（root 被映射为 nobody）是另一类经典坑——客户端 root 写不进 export。

### volumeMode: Block

裸块设备直通容器（不经文件系统），数据库自管理存储格式/仿真场景用。`volumeDevices` 替代 `volumeMounts`，应用自己负责格式化与并发。

## 九、监控与告警要点

- **容量**：kubelet volume stats → `kubelet_volume_stats_available_bytes` / `capacity_bytes` / `used_bytes`；>85% 告警（磁盘满是常见故障源）。
- **inode**：`kubelet_volume_stats_inodes_free`——小文件场景（日志碎片、session 文件）容量没满 inode 先爆。
- **对象状态**：PVC phase 非 Bound、PV phase Released/Failed 数量、VolumeSnapshot 失败数。
- **性能**：云盘 IOPS/吞吐限速纳入告警（Q5 的常见故障源）。

## 十、排错决策树

```
PVC Pending
  ├─ describe pvc: waiting for first consumer → 查 Pod 为什么没调度（正常等待）
  ├─ ProvisioningFailed → csi-provisioner 日志（配额/AZ 无货/RAM）
  └─ no matching volume → 四条件复查（静态）
Pod 卡 ContainerCreating
  ├─ FailedAttachVolume / Multi-Attach → VolumeAttachment 卡住？节点假死？
  └─ FailedMount → stage/publish 层，驱动日志 + 节点路径
PVC 卡 Terminating
  ├─ Pod 还在用（pvc-protection）→ 先删 Pod
  └─ finalizer 残留 → 确认后清 finalizer
删 PVC 后数据要恢复
  └─ PV 是 Released → volumeName 接回 / 清 claimRef 上架（黄金窗口）
```

## 最佳实践

1. 云上动态供给 SC 必配 `volumeBindingMode: WaitForFirstConsumer` + `reclaimPolicy: Retain`（关键数据）。
2. 数据库形态固定：StatefulSet + 每副本独立 RWO PVC + Retain + 定期 VolumeSnapshot。
3. local PV 必配 nodeAffinity，且接受"节点即数据"的运维模型（drain 前先想数据）。
4. 容量监控双指标：bytes + inodes，>85% 告警。
5. 快照当一等公民：deletionPolicy: Retain，恢复演练纳入季度流程。
6. 扩容演练前置：确认 SC 开 allowVolumeExpansion、驱动支持在线扩，别等磁盘 95% 才第一次试。

## 常见陷阱

- **静态 PV 写了 storageClassName 就以为会动态供给**——没有 provisioner，PVC 只会匹配不会建卷。
- **空串语义混淆**：PV `""` 只匹配"不写 SC"的 PVC；PVC 不写是找默认 SC。
- **Immediate 模式跨 AZ**：建卷快但 Pod 挂不上，云上必踩。
- **RWO 语义误解**：Once 是节点级；同节点共享合法，跨节点才 Multi-Attach。
- **删 PVC 顺手删数据**：reclaimPolicy=Delete 时删 PVC = 删云盘，无确认无回收站。
- **Released PV 清 claimRef 后裸奔**：回 Available 后可能被别人的 PVC 匹配走，敏感数据先备份。
- **快照当备份**：crash-consistent + 与原卷同生命周期区域，关键数据要 Velero/跨区域副本。
- **扩容当缩容用**：storage 只能加大，规划失误没有后悔药。

## 相关链接

- [[22-概念/04-存储/pv.md|PersistentVolume (PV)]] — 基础概念
- [[22-概念/04-存储/persistent-volume-claim.md|PersistentVolumeClaim]] — 基础概念与排障
- [[22-概念/04-存储/storageclass.md|StorageClass]] — 动态供给类目
- [[22-概念/04-存储/01-volume-attachment-controller.md|VolumeAttachment 与 Attach/Detach 控制器]] — 挂载链路细节
- [[22-概念/04-存储/02-csi-storage-capacity-tracking.md|CSIStorageCapacity]] — 容量感知调度
- [[22-概念/04-存储/csi-drivers.md|CSI Drivers]] — 驱动体系
- [[22-概念/04-存储/data-protection-k8s.md|data-protection-k8s]] — 备份恢复
- [[36-报告/k8s-高级面试快问快答-沉淀.md|K8s 高级面试快问快答沉淀]] — Q48–Q57 基础 + Q79–Q86 进阶配套卡片
