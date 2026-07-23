---
title: VolumeAttachment 与 Attach/Detach 控制器
summary: 深度解析 VolumeAttachment 对象、AD Controller 与 CSI external-attacher 的卷挂接/卸接生命周期。
category: 存储
tags:
- volumeattachment
- attach-detach-controller
- csi
- external-attacher
- volume-lifecycle
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 存储架构师
- SRE
- CSI 驱动开发者
estimated_read_time: 20min
intent_queries:
- VolumeAttachment 是什么
- Attach/Detach Controller 如何工作
- CSI external-attacher 原理
- 卷挂接卡住如何排查
trigger_keywords:
- VolumeAttachment
- Attach/Detach
- AD Controller
- external-attacher
- 卷挂接
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。
>
> 卷挂接（attach/detach）处于存储生命周期最敏感的位置：**强制删除 VolumeAttachment 或直接移除 finalizer 可能让控制器与底层云盘状态脱节，导致后续 attach 永久失败或多节点同时挂载引发文件系统损坏**。任何对 VolumeAttachment 的写操作都必须在确认目标设备已从节点卸载后进行。

---

# 19 - VolumeAttachment 与 Attach/Detach 控制器

> **适用版本**: Kubernetes v1.28 - v1.33 | **源码参照**: `kube-controller-manager` / `csi-external-attacher` | **最后更新**: 2026-07

<!-- chunk: 目录 -->
## 目录

1. [概述：attach 与 mount 的本质区别](#1-概述attach-与-mount-的本质区别)
2. [VolumeAttachment 对象模型](#2-volumeattachment-对象模型)
3. [两条挂接路径：in-tree AD Controller vs CSI external-attacher](#3-两条挂接路径in-tree-ad-controller-vs-csi-external-attacher)
4. [AD Controller 内部机制](#4-ad-controller-内部机制)
5. [external-attacher 工作流（CSI 侧）](#5-external-attacher-工作流csi-侧)
6. [VolumeAttachment 生命周期状态机](#6-volumeattachment-生命周期状态机)
7. [与 PV/PVC/kubelet 的协作时序](#7-与-pvpvckubelet-的协作时序)
8. [生产排障](#8-生产排障)
9. [监控与告警](#9-监控与告警)
10. [设计权衡与已知边界](#10-设计权衡与已知边界)
11. [相关文档](#11-相关文档)

---

<!-- chunk: 1. 概述 -->
## 1. 概述：attach 与 mount 的本质区别

在 Kubernetes 的存储生命周期中，存在两个常被混淆、但实现机制与责任主体完全不同的动作：

| 概念 | 作用域 | 执行者 | 目标 | 典型 API |
|:---|:---|:---|:---|:---|
| **Attach / Detach** | 节点级（Node） | 控制平面 + CSI Controller | 让一个块设备/卷"对某节点可见"（云盘挂载到 VM、iSCSI 登录、FCLUN 发现） | `ControllerPublishVolume` / `ControllerUnpublishVolume` |
| **Mount / Unmount** | Pod 级（Pod） | kubelet（节点上的 VolumeManager） | 在节点路径上挂载文件系统（`/var/lib/kubelet/pods/.../volumes/...`） | `NodeStageVolume` / `NodePublishVolume` |

一句话区分：

- **Attach** 回答的是"这块云盘现在属于哪个节点"（云 API 层面的 AttachDisk）。
- **Mount** 回答的是"这个 Pod 怎么在节点上访问那块已经 attach 过来的盘"（Linux `mount(2)`）。

```
                  控制平面（controller-manager / CSI Controller Pod）
                            │
            ┌───────────────┴───────────────┐
            │   attach / detach（节点级）     │  ◀── 本文档主题
            │   ControllerPublishVolume       │
            └───────────────┬─────────────────┘
                            ▼
              ┌─────────────────────────────┐
              │      节点 Node-A            │
              │  ┌────────────────────┐     │
              │  │  设备 /dev/xvdba    │ ◀── attach 后设备对节点可见
              │  └─────────┬──────────┘     │
              │            │ mount           │
              │            ▼                │
              │  kubelet VolumeManager       │  ◀── mount/unmount（Pod 级）
              │  NodeStage/NodePublishVolume │
              │            │                │
              │            ▼                │
              │  Pod 挂载点 /var/lib/...     │
              └─────────────────────────────┘
```

### VolumeAttachment 是什么

`VolumeAttachment` 是 Kubernetes（`storage.k8s.io/v1`）中**记录"某个 PV 已被（或正在被）attach 到某个节点"这一事实的集群级对象**。它本身不承载用户数据，而是 AD Controller 与 external-attacher 之间通信的"契约"：

- AD Controller 创建一个 `VolumeAttachment`（`spec.attacher` 指向某个 CSI driver name，`spec.nodeName` 指向目标节点，`spec.source.persistentVolumeName` 指向 PV）。
- CSI 侧的 external-attacher Watch 到这个对象，调用 `ControllerPublishVolume`，成功后把 `status.attached` 置为 `true`。
- kubelet 在 mount 阶段会先确认对应的 `VolumeAttachment.status.attached == true`，否则等待。

因此，VolumeAttachment 是 **attach 阶段在 etcd 中的物化表示**，也是排障时定位"卷为什么没挂上"的第一入口。

> **本篇定位**：库内已有 [[集群基础/控制平面/22-container-storage-deep-dive.md|容器存储深度剖析]] 讲存储全链路、[[存储/K8s存储/05-csi-drivers-integration.md|CSI 驱动集成]] 讲 CSI 生态，二者仅在排障章节提及 `kubectl get volumeattachment`。本文做**独立深度补强**，聚焦 VolumeAttachment 对象模型、AD Controller（kube-controller-manager 内部）与 external-attacher（CSI 侧）的协作、in-tree 与 CSI 两条路径、以及 VolumeAttachment 生命周期状态机。

---

<!-- chunk: 2. 对象模型 -->
## 2. VolumeAttachment 对象模型

### 2.1 完整对象结构

```yaml
apiVersion: storage.k8s.io/v1
kind: VolumeAttachment
metadata:
  name: csi-3f8a2c1b-diskplugin-csi-alibabacloud-com    # 由 AD Controller 生成，通常 csi-<uid-of-pv-with-attacher>
  # 关键 finalizer：external-attacher 在完成 ControllerUnpublishVolume 前不会让它消失
  finalizers:
  - external-attacher/diskplugin-csi-alibabacloud-com
  # 资源归属
  labels:
    # 部分 CSI 驱动（含 ATTACHER_TAG 的 external-attacher）会打标签便于筛选
spec:
  # attacher：必填。CSI 路径下 = CSIDriver.spec.attacherRequired / 通常是 CSI driver name
  # in-tree 路径下 = 内置 volume plugin 名（如 kubernetes.io/aws-ebs）
  attacher: diskplugin.csi.alibabacloud.com
  # nodeName：必填。卷要 attach 到的目标节点名（node.kubernetes.io/hostname 对应值）
  nodeName: cn-hangzhou.10.0.1.23
  source:
    # persistentVolumeName：必填，二选一（另一个内联卷源已废弃）。指向被 attach 的 PV
    persistentVolumeName: pvc-55a1b2c3-d4e5-6789-0123-456789abcdef
  # 部分版本支持 spec.source.inlineVolumeSpec（用于独立于 PV 的 attach，CSI 专用，很少用）
status:
  # attached：核心字段。true=已成功 attach；false=未 attach 或正在 attach
  attached: true
  # attachmentMetadata：CSI 驱动返回的设备路径等，供 kubelet 在 mount 阶段使用
  attachmentMetadata:
    # 设备名（云盘场景）/ iSCSI target 信息（块场景）
    devicePath: /dev/vdb
  # attachError / detachError：上一次失败时的错误对象（非 nil 时附带 message + time）
  # 控制器看到 error 后会进入重试循环
  attachError: null
  detachError: null
```

### 2.2 字段语义详解

| 字段 | 类型 | 含义 | 谁写入 |
|:---|:---|:---|:---|
| `spec.attacher` | string | 卷 attach 的实现者；CSI 路径 = CSI driver name | AD Controller 创建时填 |
| `spec.nodeName` | string | 目标节点名 | AD Controller |
| `spec.source.persistentVolumeName` | string | 关联的 PV 名 | AD Controller |
| `metadata.finalizers` | []string | 删除保护；CSI 路径含 `external-attacher/<driver>` | external-attacher 加/删 |
| `status.attached` | bool | attach 是否成功 | external-attacher（CSI）/ in-tree plugin（直接写） |
| `status.attachmentMetadata` | map | 设备路径、LUN、target 等 | external-attacher / in-tree plugin |
| `status.attachError` | VolumeError | attach 失败信息 | 控制器 |
| `status.detachError` | VolumeError | detach 失败信息 | 控制器 |

### 2.3 `attacher` 字段的两种语义

**`attacher` 不是"标签"，而是路由键**——它决定由谁来处理这个 VolumeAttachment：

```
AD Controller 创建 VA 时：
    spec.attacher = ?
        │
        ├─ PV.spec.csi.driver 存在  ──▶  attacher = PV.spec.csi.driver（CSI driver name）
        │                                  ▒ 由对应 external-attacher（带 --driver-name 过滤）认领
        │
        └─ PV 用 in-tree source（awsElasticBlockStore/gcePersistentDisk/...）
                                        ▒
                                        ▼
             若 CSIMigration 启用  ──▶  attacher = 翻译后的 CSI driver name（ebs.csi.aws.com 等）
             若 CSIMigration 未启用 ──▶  attacher = in-tree plugin key（kubernetes.io/aws-ebs）
                                        ▒
                                        ▼
                  in-tree 路径下 AD Controller 自己直接 attach，不创建 VA（见第 3 节）
```

### 2.4 VolumeAttachment 与 PV/PVC 的关系图

```
┌─────────────┐     bound      ┌─────────────┐   source.persistentVolumeName   ┌──────────────────────┐
│     PVC     │ ─────────────▶ │     PV      │ ──────────────────────────────▶ │   VolumeAttachment   │
│ (namespace) │                │ (cluster)   │                                  │       (cluster)      │
└─────────────┘                └─────────────┘                                  │  spec.attacher       │
                                     ▲                                         │  spec.nodeName       │
                                     │ claimRef                                │  status.attached     │
                                     │                                         └──────────┬───────────┘
                                     ▼                                                    │
                              ┌─────────────┐                                            │ AD Controller 创建
                              │  StorageClass│                                            ▼
                              └─────────────┘                            ┌────────────────────────────┐
                                                                         │ external-attacher (CSI)   │
                                                                         │ ControllerPublishVolume   │
                                                                         └────────────────────────────┘
```

注意三个对象都是**集群级（cluster-scoped）**的：PV、VolumeAttachment 不属于任何 namespace，因为 attach 是节点级资源操作。

---

<!-- chunk: 3. 两条路径 -->
## 3. 两条挂接路径：in-tree AD Controller vs CSI external-attacher

Kubernetes 的卷挂接经过了一次重大架构演进——从"内置云厂商代码"到"CSI 解耦"。理解这两条路径是理解 VolumeAttachment 行为的关键。

### 3.1 in-tree 路径（Legacy，逐步弃用）

**前提**：PV 使用 `awsElasticBlockStore` / `gcePersistentDisk` / `azureDisk` 等 in-tree volume source，且对应 `CSIMigration` feature gate **关闭**。

**特征**：

```
                 kube-controller-manager 进程内
   ┌──────────────────────────────────────────────────────────┐
   │  AttachDetachController（AD Controller）                  │
   │     │                                                     │
   │     │  直接持有云厂商 SDK 调用权限                         │
   │     ▼                                                     │
   │  AWS EBS / GCE PD / Azure Disk in-tree volume plugin      │
   │     │                                                     │
   │     │  AttachDisk(region, volumeId, instanceId)           │
   │     ▼                                                     │
   │  云 API（AWS EC2 / GCE Compute / Azure Compute）          │
   └──────────────────────────────────────────────────────────┘
                              │
                              ▼
                   （通常）不创建 VolumeAttachment 对象
                   attach 状态仅存在于 AD Controller 的内存
                   actual state of world cache 中
```

关键点：

- **in-tree 路径下 VolumeAttachment 对象可能根本不存在**。AD Controller 调用 in-tree plugin 的 `Attach()`/`Detach()`，状态保存在内存的 actual/desired state of world，不一定物化到 etcd。这也是为什么有些老集群 `kubectl get volumeattachment` 是空的。
- 云厂商的鉴权凭证必须配置在 kube-controller-manager（cloud-provider 配置）中。
- AD Controller 直接持有云 API 调用，**与 kube-controller-manager 进程强耦合**。

### 3.2 CSI 路径（Modern，推荐）

**前提**：PV 使用 `spec.csi.driver` 字段；或 in-tree PV 但 `CSIMigration` 已启用（in-tree 操作被翻译为对 CSI driver 的调用）。

**特征**：

```
    kube-controller-manager                    CSI Controller Pod（独立 Deployment）
   ┌────────────────────────────┐             ┌─────────────────────────────────────┐
   │  AD Controller              │             │  external-attacher sidecar          │
   │     │                       │             │     │ Watch(VA, attacher=me)         │
   │     │ 不直接调云 API         │             │     │                                │
   │     ▼                       │  创建 VA    │     │ VA.attached=false ─▶ ControllerPublishVolume
   │  CSIAttacher（一个包装）    │ ──────────▶ │     │ VA 被删 ──────────▶ ControllerUnpublishVolume
   │     │                       │             │     │                                │
   │     │ 仅：Create VA /        │             │     ▼                                │
   │     │ Delete VA / 等 status  │ ◀────────── │  csi-plugin（CSI driver 主容器）    │
   │     │                       │  改 status  │     │ gRPC ControllerPublishVolume    │
   └─────┴────────────────────────┘             │     ▼                                │
                                                 │  云 API / 存储后端                   │
                                                 └─────────────────────────────────────┘
```

关键点：

- AD Controller **不再调用云 API**。它只负责"声明意图"——创建/删除 VolumeAttachment 对象，然后等待。
- 真正的 attach 工作由 **CSI Controller Pod 内的 external-attacher sidecar** 完成，通过 gRPC 调用驱动主容器的 `ControllerPublishVolume` / `ControllerUnpublishVolume`。
- 这是一次彻底的**控制平面解耦**：kube-controller-manager 不再需要任何云厂商凭证，CSI 驱动作为独立进程运行。

### 3.3 CSIMigration：连接两条路径的桥梁

绝大多数现代集群仍可能存在 in-tree PV（历史遗留）。`CSIMigration` feature gate 让这些 PV 也走 CSI 路径：

```
旧 PV (awsElasticBlockStore source)
        │
        │  CSIMigration=on（v1.17 GA，v1.25 默认开）
        ▼
AD Controller 在 attach 时把 in-tree spec 翻译为 CSI 调用
        │
        ▼
spec.attacher = ebs.csi.aws.com（CSI driver name）
创建 VolumeAttachment，由 AWS EBS CSI external-attacher 处理
```

详见 [[存储/K8s存储/16-csi-migration-in-tree-to-csi.md|CSI 迁移：in-tree 到 CSI]]。

### 3.4 两条路径对比

| 维度 | in-tree AD Controller | CSI external-attacher |
|:---|:---|:---|
| **执行位置** | kube-controller-manager 进程内 | 独立 CSI Controller Pod 内 sidecar |
| **凭证持有** | cloud-provider 配置（kube-controller-manager） | CSI driver 自己的 Secret/云凭证 |
| **VolumeAttachment 对象** | 通常不创建（内存态） | **必创建**，是核心载体 |
| **attach API** | 云厂商 SDK（AttachDisk 等） | CSI gRPC `ControllerPublishVolume` |
| **detach API** | 云厂商 SDK（DetachDisk 等） | CSI gRPC `ControllerUnpublishVolume` |
| **状态可见性** | 仅 klog 日志 + actual/desired state cache | `kubectl get volumeattachment` 直接可见 |
| **失败重试** | AD Controller reconcile loop | external-attacher reconcile loop |
| **可扩展性** | 需修改 k8s 核心代码 | 任何厂商实现 CSI spec 即可 |
| **演进趋势** | 冻结，长期移除（已停止接受新 in-tree plugin） | 所有新存储类型的唯一选择 |
| **代表 driver name** | `kubernetes.io/aws-ebs` | `ebs.csi.aws.com` |
| **kube-controller-manager 是否需云凭证** | 是 | 否 |

> **判断当前集群走哪条路**：
> - 若 `kubectl get volumeattachment` 在 Pod 用 PV 后**出现对象** → CSI 路径（含 CSIMigration）。
> - 若始终为空但 Pod 卷能正常挂载 → 纯 in-tree 路径（极少数老集群）。

---

<!-- chunk: 4. AD Controller 内部机制 -->
## 4. AD Controller 内部机制

AD Controller（AttachDetachController）位于 `kube-controller-manager`，是 kube-controller-manager 启动的众多 controller 之一。源码位于 `pkg/controller/volume/attachdetach/`。

### 4.1 整体架构

```
   kube-controller-manager
   ┌────────────────────────────────────────────────────────────────────┐
   │  AttachDetachController                                            │
   │                                                                    │
   │  ┌──────────────────────┐    ┌──────────────────────────────────┐  │
   │  │  PodInformer          │    │  desiredStateOfWorld（应 attach）│  │
   │  │  (Watch Pod 调度)     │───▶│  map[node]map[volume]            │  │
   │  └──────────────────────┘    └──────────────────────────────────┘  │
   │                                      ▲                            │
   │  ┌──────────────────────┐            │ diff                       │
   │  │  NodeInformer        │            │                            │
   │  │  (Watch Node 状态)   │     ┌──────┴───────────┐                 │
   │  └──────────────────────┘     │  Reconciler      │                 │
   │                                │  (reconcile loop)│                 │
   │  ┌──────────────────────┐     └──────┬───────────┘                 │
   │  │  PVC/PV Informer     │            │                             │
   │  │  (解析卷来源)         │            ▼                             │
   │  └──────────────────────┘    ┌──────────────────────────────────┐  │
   │                               │  actualStateOfWorld（已 attach）│  │
   │                               │  map[node]map[volume]           │  │
   │                               └──────────────────────────────────┘  │
   │                                            │                       │
   │                                            ▼                       │
   │                               ┌──────────────────────────────────┐ │
   │                               │  Attacher/Detacher goroutines    │ │
   │                               │  (实际创建/删除 VolumeAttachment │ │
   │                               │   或调用 in-tree plugin)         │ │
   │                               └──────────────────────────────────┘ │
   └────────────────────────────────────────────────────────────────────┘
```

### 4.2 两个核心缓存

AD Controller 维护两个 in-memory 状态，reconcile loop 持续让二者收敛：

#### desiredStateOfWorld（"应该是什么"）

由 PodInformer 维护。对每个 Pod：

- 若 Pod 被调度到节点 N（`pod.spec.nodeName` 非空），且 Pod 引用了 PV，则记录 `(node=N, volume=V)` 应 attach。
- 若 Pod 被删除/调度到别的节点，则移除该 `(node, volume)` 对（若该 volume 在该 node 上没有其他 Pod 引用）。

#### actualStateOfWorld（"现在是什么"）

由 Attacher/Detacher goroutine 维护。记录已确认 attach 成功的 `(node, volume)` 对。CSI 路径下，attach 成功的判据是 VolumeAttachment.status.attached==true。

#### 收敛逻辑

```
对每个 desired 中的 (node, volume)：
    若 actual 中不存在 ──▶ 触发 attach（CSI: 创建 VolumeAttachment）

对每个 actual 中的 (node, volume)：
    若 desired 中不存在 ──▶ 触发 detach（CSI: 删除 VolumeAttachment）

对每对都存在但状态异常的：标记 retry
```

### 4.3 触发源

| 事件 | 效果 |
|:---|:---|
| Pod 调度到节点（`pod.spec.nodeName` 由空变为非空） | desiredStateOfWorld += (node, volume) |
| Pod 被删除 | desiredStateOfWorld -= (node, volume)（若该卷无其他 Pod 引用） |
| Pod 迁移到其他节点 | 旧节点 -= volume；新节点 += volume |
| Node 标记 NotReady（部分场景） | 触发 force-detach 评估 |
| 定时 reconcile（`--attach-detach-reconcile-sync-period`，默认 1m） | 全量对账，捕捉遗漏事件 |

### 4.4 关键启动参数

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 kube-controller-manager 的 AD Controller 相关参数（静态 Pod 配置）
kubectl -n kube-system get pod kube-controller-manager-<master> -o yaml | grep -E "attach-detach|disable-attach"

# 常见参数（仅供参考，不要直接修改运行中的 master）
#   --attach-detach-reconcile-sync-period=1m0s        # reconcile 全量同步周期
#   --disable-attach-detach-reconcile-sync            # 禁用定时全量同步（仅事件驱动，调试用）
#   --enable-attach-detach-reconciler-sync=true       # 总开关
```

| 参数 | 默认 | 含义 |
|:---|:---|:---|
| `--attach-detach-reconcile-sync-period` | `1m0s` | 全量 reconcile 间隔 |
| `--disable-attach-detach-reconcile-sync` | false | 关闭定时全量同步（仅用于排障，禁止生产开启） |
| `--controllers` | `*` | 显式控制 AD Controller 是否启用，例如 `--controllers=-attachdetach` 可禁用 |

> ⚠️ **🟡 中危知识** — 生产环境禁用 AD Controller 会导致新 Pod 永远无法挂卷。`--controllers=-attachdetach` 仅在故障注入测试中使用。

### 4.5 in-tree 与 CSI 在 AD Controller 内部的分叉

AD Controller 内部对每个 volume 调用一个统一的 `Attacher` 接口。**两种实现共享相同的 reconcile 框架，但执行路径不同**：

```go
// 简化的伪代码（来源：pkg/controller/volume/attachdetach/reconciler）
func (rc *reconciler) reconcile() {
    // 1. 处理 unmount 剩余后再 detach
    for _, attachedVolume := range rc.actualStateOfWorld.GetAttachedVolumes() {
        if !rc.desiredStateOfWorld.VolumeExists(attachedVolume.NodeName, attachedVolume.VolumeName) {
            // 调用 Detach
            err := rc.attacher.Detach(attachedVolume.VolumeName, attachedVolume.NodeName)
            // CSI 路径：这里实际是调用 csiAttacher.Detach() → 删除 VolumeAttachment
            // in-tree 路径：这里直接调用云 SDK DetachDisk
        }
    }

    // 2. attach 新卷
    for _, desiredVolume := range rc.desiredStateOfWorld.GetVolumesToAttach() {
        if !rc.actualStateOfWorld.VolumeExists(desiredVolume.NodeName, desiredVolume.VolumeName) {
            err := rc.attacher.Attach(desiredVolume.VolumeName, desiredVolume.NodeName)
            // CSI 路径：csiAttacher.Attach() → 创建 VolumeAttachment，然后等 attached=true
            // in-tree 路径：直接调用云 SDK AttachDisk
        }
    }
}
```

CSI 路径下，`csiAttacher.Attach()` 的行为是：

1. 创建 VolumeAttachment 对象（含 finalizer 注入由 external-attacher 完成）。
2. **阻塞等待** VolumeAttachment.status.attached 变 true（带超时与重试）。
3. 等到 true 后，把 (node, volume) 加入 actualStateOfWorld。

这就是为什么 CSI 路径下 attach 是异步的、可见的——AD Controller 自己只发"指令"，真正干活的是 external-attacher。

---

<!-- chunk: 5. external-attacher 工作流 -->
## 5. external-attacher 工作流（CSI 侧）

external-attacher 是 [kubernetes-csi/external-attacher](https://github.com/kubernetes-csi/external-attacher) 项目，作为 sidecar 与 CSI driver 主容器一同运行在 CSI Controller Pod 中（注意：不是 csi-node DaemonSet，是 Controller 侧 Deployment）。

### 5.1 部署形态

```yaml
# CSI Controller Pod（典型结构，节选）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: csi-disk-controller
  namespace: kube-system
spec:
  replicas: 2                    # 通常 ≥2，但只有一个 active（leader election）
  template:
    spec:
      containers:
      - name: csi-plugin          # CSI driver 主容器，实现 gRPC
        image: registry/csi-disk-plugin:v1.30
      - name: csi-provisioner     # 另一个 sidecar，处理 PVC→PV
        image: registry/sig-storage/csi-provisioner:v4.0
      - name: csi-attacher        # ★ 本文主角 external-attacher
        image: registry/sig-storage/csi-attacher:v4.6.0
        args:
        - --v=5
        - --csi-address=$(ADDRESS)
        - --leader-election=true            # 多副本时必须开
        - --default-fstype=ext4
        - --worker-threads=10               # 并发 attach/detach 工作线程
        env:
        - name: ADDRESS
          value: /csi/csi.sock
        volumeMounts:
        - name: socket-dir
          mountPath: /csi
```

### 5.2 与 CSI driver 的 gRPC 契约

external-attacher 通过 Unix domain socket 调用 CSI driver 主容器实现的 CSI gRPC 接口：

| CSI RPC | external-attacher 何时调用 | 用途 |
|:---|:---|:---|
| `GetPluginInfo` | 启动时校验 driver name 与 `--driver-name` 一致 | 自检 |
| `GetPluginCapabilities` | 启动时 | 检查 `CONTROLLER_SERVICE` 能力 |
| `ControllerGetCapabilities` | 启动时 | 检查 `PUBLISH_UNPUBLISH_VOLUME` 能力 |
| `ControllerPublishVolume` | Watch 到 `VA.attached==false` | ★ 真正的 attach |
| `ControllerUnpublishVolume` | Watch 到 VA 被请求删除 | ★ 真正的 detach |

### 5.3 完整 attach 工作流

```
       VA 新建/更新事件（attached=false）
                    │
                    ▼
   ┌──────────────────────────────────────────┐
   │ 1. external-attacher reconcile handler    │
   │    读 VA.spec：                            │
   │      attacher == 自己 driver name?         │──否──▶ 忽略（不属于自己的 VA）
   │      source.persistentVolumeName?          │
   └────────────────┬──────────────────────────┘
                    │ 是
                    ▼
   ┌──────────────────────────────────────────┐
   │ 2. 读取 PV 对象（用 spec.source 的名字）  │
   │    从 PV.spec.csi 提取 volumeHandle /     │
   │    volumeAttributes / secretReferences    │
   └────────────────┬──────────────────────────┘
                    ▼
   ┌──────────────────────────────────────────┐
   │ 3. （可选）读取 Node 对象，获取            │
   │    node ID（云厂商的 instance ID），      │
   │    通过 CSINode 拓扑信息映射              │
   └────────────────┬──────────────────────────┘
                    ▼
   ┌──────────────────────────────────────────┐
   │ 4. （可选）解析 Secret，准备凭证           │
   │    （PV.spec.csi.nodePublishSecretRef     │
   │     或 controllerPublishSecretRef）       │
   └────────────────┬──────────────────────────┘
                    ▼
   ┌──────────────────────────────────────────┐
   │ 5. gRPC 调用 CSI driver：                 │
   │    ControllerPublishVolume({              │
   │      volume_id,                            │
   │      node_id,                              │
   │      volume_capability,                    │
   │      readonly,                             │
   │      secrets,                              │
   │      volume_context                        │
   │    })                                      │
   └────────────────┬──────────────────────────┘
                    │
            ┌───────┴────────┐
            │                │
         成功              失败
            │                │
            ▼                ▼
   更新 VA.status:    更新 VA.status:
   attached=true      attachError={message,time}
   attachmentMetadata=driver返回   保持 attached=false
            │                │
            │                ▼
            │        reconcile 退避后重试
            │        （指数退避，默认最长 ~5min）
            ▼
   保持监听，等待 detach 触发
```

### 5.4 完整 detach 工作流（含 finalizer）

```
       AD Controller 删除 VolumeAttachment
                    │
                    ▼
   ┌──────────────────────────────────────────┐
   │ 1. external-attacher Watch 到             │
   │    DeletionTimestamp 不为空               │
   │    （但 finalizer 阻止真正删除）          │
   └────────────────┬──────────────────────────┘
                    ▼
   ┌──────────────────────────────────────────┐
   │ 2. gRPC 调用 CSI driver：                 │
   │    ControllerUnpublishVolume({            │
   │      volume_id,                            │
   │      node_id,                              │
   │      secrets                               │
   │    })                                      │
   └────────────────┬──────────────────────────┘
                    │
            ┌───────┴────────┐
            │                │
         成功              失败
            │                │
            ▼                ▼
   移除 finalizer     更新 VA.status:
   external-attacher/<driver>     detachError={...}
            │           VA 真正被 GC 失败
            ▼                │
   VA 从 etcd 消失           ▼
                       退避重试，VA 仍存在
```

### 5.5 finalizer 的作用

external-attacher 在**首次认领** VolumeAttachment 时会注入一个 finalizer，格式为 `external-attacher/<csi-driver-name>`（例如 `external-attacher/diskplugin.csi.alibabacloud.com`）。它的作用：

- **防止 VA 在 detach 完成前被 GC**。即使 AD Controller 想删 VA，etcd 也只设置 `deletionTimestamp`，对象依然存在。
- external-attacher 必须先成功 `ControllerUnpublishVolume`，才能移除 finalizer，VA 才会真正消失。
- 这是 CSI 路径下"detach 一定能等到完成（或永久失败重试）"的保证。

### 5.6 多副本与 leader election

external-attacher 通常以多副本部署，但通过 `--leader-election=true` 保证**只有一个副本真正干活**：

```
   ┌────────────┐    ┌────────────┐
   │ attacher-0 │    │ attacher-1 │
   │ (leader)   │    │ (standby)  │
   │ 处理 VA    │    │ 仅续约     │
   └─────┬──────┘    └────────────┘
         │ leader lease 失效
         ▼
                    ┌────────────┐
                    │ attacher-1 │
                    │ 接管成为    │
                    │ 新 leader   │
                    └────────────┘
```

> ⚠️ 不开 leader election + 多副本 = **同一 VA 被两个 attacher 同时处理**，可能引发竞态（attach 两次、finalizer 抖动）。生产必须开。

### 5.7 `attachRequired=false` 的特殊情形

CSI driver 可在 `CSIDriver` 对象中声明：

```yaml
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: nfs.csi.k8s.io
spec:
  attachRequired: false     # ★ 此 driver 无 attach 阶段
  podInfoOnMount: true
```

含义：

- 该驱动（典型如 NFS、SMB、SSHFS 这类**网络文件系统**）**不需要把卷 attach 到节点**——节点只要能 mount 就能访问。
- AD Controller **不创建 VolumeAttachment 对象**，直接跳到 kubelet 的 mount 阶段。
- `kubectl get volumeattachment` 中看不到这类卷的对象，属正常。

这是排障时一个常见的认知陷阱：**不是所有 PV 都有对应的 VolumeAttachment**。

---

<!-- chunk: 6. 生命周期状态机 -->
## 6. VolumeAttachment 生命周期状态机

### 6.1 状态流转图

```
                      AD Controller 创建 VA
                              │
                              ▼
                    ┌───────────────────┐
                    │     Pending       │  attached=false, 无 error
                    │ (等待 attach)     │
                    └─────────┬─────────┘
                              │
                  external-attacher
                  ControllerPublishVolume
                              │
              ┌───────────────┼───────────────┐
              │ 成功           │ 失败           │
              ▼               ▼               ▼
    ┌─────────────────┐  ┌────────────────┐
    │    Attached     │  │ Pending+Error  │
    │ attached=true   │  │ attached=false │
    │                 │  │ attachError    │
    └────────┬────────┘  └───────┬────────┘
             │                   │ external-attacher 退避重试
             │                   │（指数退避，最长 ~5min）
             │                   ▼
             │              ┌────────────────┐
             │              │  重试中...      │ ──成功──▶ Attached
             │              └────────────────┘
             │
             │ AD Controller 发起删除
             │ （Pod 删除 / 迁移）
             ▼
    ┌─────────────────┐
    │   Detaching     │  deletionTimestamp != nil
    │ (finalizer 保留)│  attached 可能仍 true
    └────────┬────────┘
             │ external-attacher
             │ ControllerUnpublishVolume
             │
     ┌───────┴────────┐
     │ 成功            │ 失败
     ▼                ▼
   移除 finalizer   ┌─────────────────┐
   VA GC 消失       │ Detaching+Error │
   │                │ detachError     │
   ▼                └────────┬────────┘
┌────────┐                  │ 退避重试
│ Gone   │ ◀────────────────┘ 成功后移除 finalizer
│(etcd 中│
│ 不存在)│
└────────┘
```

### 6.2 状态与字段对照表

| 逻辑状态 | `status.attached` | `deletionTimestamp` | `attachError/detachError` | 含义 |
|:---|:---:|:---:|:---:|:---|
| **Pending** | false | 空 | null | 等待 external-attacher 处理 |
| **Pending+AttachError** | false | 空 | 非 null | attach 失败，等待重试 |
| **Attached** | true | 空 | null | attach 成功，卷在用 |
| **Detaching** | true/false | 非空 | null | detach 进行中 |
| **Detaching+DetachError** | true/false | 非空 | 非 null | detach 失败，等待重试 |
| **Gone** | — | — | — | 对象已从 etcd 删除 |

### 6.3 重试与退避

external-attacher 对失败操作采用指数退避重试：

- 初始退避：1s。
- 倍增：每次失败 ×2。
- 上限：默认约 5min（受 `--reconcile-sync` 影响的全量同步也会兜底）。
- 永久性错误（CSI driver 返回 gRPC `Aborted`/`Unimplemented` 等）可能停止重试。

```
失败次数    退避间隔        累计等待
   1          1s            1s
   2          2s            3s
   3          4s            7s
   4          8s           15s
   5         16s           31s
   6         32s           63s
   7         64s          127s
   8        128s          255s
   9        ~256s          ~511s ≈ 8.5min
  ≥10        ~300s（封顶）
```

### 6.4 终态：为什么会"卡住"

VolumeAttachment 长期不消失，几乎都是因为 **detach 卡住**：

1. **节点 NotReady**：CSI driver 无法访问该节点的资源（例如云盘已随 VM 销毁），`ControllerUnpublishVolume` 报错，external-attacher 持续重试。
2. **云盘状态异常**：底层云 API 报 "volume is in use by another instance"，无法 detach。
3. **finalizer 残留但 attacher 已死**：external-attacher Pod 异常且无法恢复，没人移除 finalizer。
4. **CSIDriver.spec.attachRequired=false**：这类卷根本不应该有 VA，若出现则是异常残留。

第 8 节给出排障方法。

---

<!-- chunk: 7. 协作时序 -->
## 7. 与 PV/PVC/kubelet 的协作时序

### 7.1 Pod 启动到卷可用的完整时序

```
 Pod 用户        Scheduler      PV Controller    AD Controller    external-attacher    kubelet(VolumeManager)
   │                │                │                 │                  │                    │
   │ kubectl apply  │                │                 │                  │                    │
   │────Pod(YAML)──▶│                │                 │                  │                    │
   │                │                │                 │                  │                    │
   │                │ 调度决策        │                 │                  │                    │
   │                │ pod.nodeName=N │                 │                  │                    │
   │                │───────────────▶│                 │                  │                    │
   │                │                │                 │                  │                    │
   │                │           PVC 已 Bound?          │                  │                    │
   │                │           是 ── 跳过             │                  │                    │
   │                │           否 ── 创建 PV 绑定 PVC │                  │                    │
   │                │                │                 │                  │                    │
   │                │                │      Pod 调度事件                   │                    │
   │                │                │         (nodeName 非空)             │                    │
   │                │                │──────────────▶ │                    │                    │
   │                │                │  desiredState += (N, V)             │                    │
   │                │                │  reconcile: attach                  │                    │
   │                │                │                 │                    │                    │
   │                │                │                 │  CSI 路径:        │                    │
   │                │                │                 │  Create VA(       │                    │
   │                │                │                 │   attacher=driver,│                    │
   │                │                │                 │   nodeName=N,     │                    │
   │                │                │                 │   pvName=V)       │                    │
   │                │                │                 │──────────────────▶│                    │
   │                │                │                 │                    │                    │
   │                │                │                 │                    │ Watch 到新 VA     │
   │                │                │                 │                    │ attached=false    │
   │                │                │                 │                    │ gRPC:             │
   │                │                │                 │                    │ ControllerPublish │
   │                │                │                 │                    │ Volume            │
   │                │                │                 │                    │   │               │
   │                │                │                 │                    │   ▼ 云 API        │
   │                │                │                 │                    │ AttachDisk       │
   │                │                │                 │                    │   │ 成功          │
   │                │                │                 │                    │   ▼               │
   │                │                │                 │  Update VA.status: │                    │
   │                │                │                 │   attached=true    │                    │
   │                │                │                 │◀──────────────────│                    │
   │                │                │                 │                    │                    │
   │                │                │                 │  actualState +=   │                    │
   │                │                │                 │   (N, V)          │                    │
   │                │                │                 │                    │                    │
   │                │                │                 │                    │      Pod 已调度 + VA.attached=true
   │                │                │                 │                    │◀──────────────────│
   │                │                │                 │                    │   kubelet VolumeManager
   │                │                │                 │                    │   waitForAttach()
   │                │                │                 │                    │   gRPC to csi-node:
   │                │                │                 │                    │     NodeStageVolume   (格式化 + 全局挂载)
   │                │                │                 │                    │     NodePublishVolume (bind mount 到 Pod)
   │                │                │ │                 │                    │                    │
   │                │                │                 │                    │                    │ Pod Running
   │                │                │                 │                    │                    │ 容器可访问 /data
```

### 7.2 Pod 删除时的反向时序

```
 Pod 删除       kubelet VolumeManager    AD Controller    external-attacher    云后端
   │                  │                       │                  │              │
   │ delete Pod       │                       │                  │              │
   │─────────────────▶│                       │                  │              │
   │                  │ NodeUnpublishVolume   │                  │              │
   │                  │ NodeUnstageVolume     │                  │              │
   │                  │ (umount + 解除全局挂载)│                  │              │
   │                  │                       │                  │              │
   │                  │ Pod 删除完成          │                  │              │
   │                  │                       │                  │              │
   │            PodInformer 触发 desiredState -= (N, V)            │              │
   │                  │                       │ reconcile         │              │
   │                  │                       │ detach            │              │
   │                  │                       │ Delete VA         │              │
   │                  │                       │──────────────────▶│              │
   │                  │                       │  (deletionTimestamp)             │
   │                  │                       │  finalizer 阻止 GC               │
   │                  │                       │                  │              │
   │                  │                       │                  │ gRPC:        │
   │                  │                       │                  │ Controller   │
   │                  │                       │                  │ UnpublishVol │
   │                  │                       │                  │   │          │
   │                  │                       │                  │   ▼──────────▶│ DetachDisk
   │                  │                       │                  │   │ 成功      │
   │                  │                       │                  │   ▼          │
   │                  │                       │                  │ 移除 finalizer
   │                  │                       │                  │ VA 真正消失  │
   │                  │                       │                  │              │
   │                  │                       │  actualState -=  │              │
   │                  │                       │   (N, V)         │              │
```

### 7.3 关键同步点

- **kubelet 的 `waitForAttach`**：在 NodeStageVolume 之前，kubelet 会轮询 `VolumeAttachment.status.attached`（CSI 路径）或调用 in-tree plugin 的 `WaitForAttach`。这就是为什么 attach 没完成时 Pod 卡在 `ContainerCreating`。
- **actualStateOfWorld 的更新时机**：CSI 路径下，AD Controller 在 VA.attached 变 true 后才把 (node, volume) 加入 actualState。这意味着若 external-attacher 异常，actualState 永远不更新，但 desiredState 已有 → 持续重试创建/检查 VA。
- **detach 前必须先 unmount**：AD Controller 在触发 detach 前会通过节点上的 csi-node 确认卷已 unmount（通过 `NodeGetCapabilities` 报告的 `VOLUME_UNSTAGE` 状态），否则会等待。这是节点 NotReady 时 detach 卡住的根因之一。

---

<!-- chunk: 8. 生产排障 -->
## 8. 生产排障

### 8.1 基本诊断流程

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看所有 VolumeAttachment 概览
kubectl get volumeattachment

# 输出示例：
# NAME                                                              ATTACHER                          PV                                         NODE                ATTACHED   AGE
# csi-3f8a2c1b-diskplugin-csi-alibabacloud-com                      diskplugin.csi.alibabacloud.com   pvc-55a1b2c3-...                           cn-hangzhou.i-...   true       45m
# csi-9e2d1f4a-diskplugin-csi-alibabacloud-com                      diskplugin.csi.alibabacloud.com   pvc-77c3d4e5-...                           cn-hangzhou.i-...   false      12m   ← 注意：attached=false 且 age 较大，可能卡住
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 2. 查看具体 VA 的完整 spec 与 status（含 attachError/detachError）
kubectl get volumeattachment <va-name> -o yaml

# 重点关注：
#   status.attached:            是否已 attach
#   status.attachError.message: attach 失败原因
#   status.detachError.message: detach 失败原因
#   metadata.deletionTimestamp: 是否被请求删除（detaching 中）
#   metadata.finalizers:        finalizer 是否残留
#   spec.nodeName:              目标节点
#   spec.source.persistentVolumeName: 关联的 PV
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 3. 查看关联 PV 与 PVC（确认卷本身健康）
kubectl get pv <pv-name> -o wide
kubectl get pvc -A --field-selector spec.volumeName=<pv-name>

# 4. 查看目标节点是否 Ready
kubectl get node <node-name> -o wide
kubectl describe node <node-name> | grep -A5 "Conditions:"
```

### 8.2 CSI driver 配置确认

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 CSIDriver，重点关注 attachRequired 字段
kubectl get csidriver -o wide

# 若某 driver 的 attachRequired=false（如 NFS），它不会有 VolumeAttachment
# 若 attachRequired=true（如 EBS、ESSD），必须存在 VA 才能 mount
kubectl get csidriver <driver-name> -o yaml | grep -A2 "attachRequired"
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点的 CSINode 信息（确认 driver 在节点上注册）
kubectl get csinode <node-name> -o yaml
# 关注 spec.drivers[]：每个 driver 应有 nodeID 与拓扑键
```

### 8.3 external-attacher 日志检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 找到 CSI Controller Pod（external-attacher 所在）
kubectl -n kube-system get pods -l app=csi-disk-controller -o wide
# 或
kubectl -n kube-system get pods | grep -E "csi.*controller"

# 查看 external-attacher 容器日志
kubectl -n kube-system logs <csi-controller-pod> -c csi-attacher --tail=100

# 关注关键词：
#   "ControllerPublishVolume"      ← attach 调用
#   "ControllerUnpublishVolume"    ← detach 调用
#   "attached" / "detached"        ← 状态变更
#   "error" / "failed"             ← 失败原因
#   "finalizer"                    ← finalizer 操作
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 用更详细日志级别（需临时重启 attacher 加 --v=5，非此处操作）
# 这里仅查询当前已有日志中的 reconcile 事件
kubectl -n kube-system logs <csi-controller-pod> -c csi-attacher --tail=500 | \
  grep -E "reconcile|publish|attach|detach"
```

### 8.4 常见问题与对策

#### 问题 1：Pod 卡在 ContainerCreating，VA.attached=false

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get volumeattachment | grep <pv-name>
kubectl get volumeattachment <va-name> -o jsonpath='{.status.attachError.message}'
```

常见根因：

| attachError.message 关键词 | 根因 | 对策 |
|:---|:---|:---|
| `volume is already attached to another node` | 多节点争用 / 旧 detach 未完成 | 检查旧节点 VA 是否残留 |
| `InstanceLimitExceeded` / `VolumeLimitExceeded` | 节点 attach 数超限 | 检查 `kubectl describe node` 的 `Allocatable: attachable-volumes-*` |
| `NotFound` volume / instance | 云盘或 VM 已被外部删除 | 确认底层资源，必要时重建 |
| `timeout` / context deadline | 云 API 慢或网络问题 | 看 attacher 日志、云 API 限流 |

#### 问题 2：VolumeAttachment 长期不消失（detaching 卡住）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get volumeattachment <va-name> -o jsonpath='{.metadata.deletionTimestamp}'
kubectl get volumeattachment <va-name> -o jsonpath='{.status.detachError.message}'
kubectl get node <node-name> -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'
```

最常见根因：**节点 NotReady 或失联**。CSI driver 在 detach 时可能需要与该节点通信（部分实现），节点不可达导致 `ControllerUnpublishVolume` 失败。

> ⚠️ 这是触发 `force-detach` 决策的关键场景。**必须先确认数据安全**——若节点只是临时网络抖动，强制 detach 后节点恢复，可能造成双写。详见 8.5。

#### 问题 3：节点已删除但 VA 残留

云上场景：节点 VM 被直接销毁（如 Spot 中断、scale down），但 AD Controller 没收到事件，VA 残留。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认节点对象是否还存在
kubectl get node <node-name>
# 若 NotFound，但 VA 仍指向它 → 残留
```

#### 问题 4：CSI driver 升级后 VA 行为异常

新版 driver 可能改变了 `attachRequired` 或拓扑键。升级前应清空所有 VA（ draining 节点）或确保版本兼容。

### 8.5 强制删除卡住的 VolumeAttachment（高风险）

> ⚠️ **🔴 高危操作** — 仅在以下**全部**条件满足时执行：
>
> 1. 已确认底层云盘**确实已从目标节点卸载**（云控制台 DetachDisk 成功 / 卷状态 Available）。
> 2. 已确认该节点上**没有** Pod 在使用该卷（`kubectl get pod -A --field-selector spec.nodeName=<node>` 无引用）。
> 3. 已确认 external-attacher **永久不可恢复**（Pod CrashLoopBackOff 且无法修复，或整个 CSI 部署已被卸载）。
> 4. 已在变更窗口、有回滚预案、已通知存储团队。
>
> **否则**：跳过本节，先修复 CSI driver 或节点，让 external-attacher 正常完成 detach。强制删除 VA 不会让底层云盘自动 detach——它只是欺骗 Kubernetes 让它认为可以重新 attach，可能引发"云盘仍挂在旧节点但 K8s 已认为可用"的灾难性竞态。

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，必须双人复核并确认底层状态
# 强制移除 finalizer，让 VA 被 GC
# 前置：已在云控制台确认云盘已 detach，节点上无 Pod 引用
kubectl patch volumeattachment <va-name> -p '{"metadata":{"finalizers":[]}}' --type=merge

# 验证 VA 已消失
kubectl get volumeattachment <va-name>
# Expected: Error from server (NotFound)
```

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，必须双人复核并确认底层状态
# （备用方案）直接 delete 会因 finalizer 卡住，需配合 patch
kubectl delete volumeattachment <va-name> --wait=false
# 然后立即 patch 移除 finalizer（同上）
```

> **永远不要用** `kubectl delete volumeattachment --force --grace-period=0` **绕过 finalizer**——它不会移除 finalizer，VA 仍会卡在 Terminating。

### 8.6 排障决策树

```
Pod 挂卷异常？
    │
    ▼
kubectl get volumeattachment | grep <pv>
    │
    ├─ 无 VA 对象 ──────▶ 检查 CSIDriver.attachRequired 是否 false
    │                      若 false（如 NFS）→ 看 csi-node 日志
    │                      若 true → 看 AD Controller 是否在 reconcile
    │
    ├─ VA.attached=false ─▶ 看 status.attachError.message
    │                      │
    │                      ├─ "another node" → 多节点争用，等 detach
    │                      ├─ "limit exceeded" → 节点配额
    │                      ├─ "NotFound" → 底层资源丢失
    │                      └─ 其他 → external-attacher 日志
    │
    └─ VA.attached=true 但 Pod 仍 ContainerCreating
                            │
                            ▼
                        问题在 mount 阶段（kubelet/csi-node）
                        看 kubelet 日志、csi-node 日志
                        （不在本文范围，参见容器存储深度剖析）
```

---

<!-- chunk: 9. 监控与告警 -->
## 9. 监控与告警

### 9.1 关键 Prometheus 指标

| 指标 | 来源 | 含义 |
|:---|:---|:---|
| `kube_volumeattachment_status_attached` | kube-state-metrics | 1=已 attach，0=未 attach；按 volumeattachment 维度 |
| `kube_volumeattachment_created` | kube-state-metrics | VA 创建时间戳（用于计算"卡住"时长） |
| `kube_volumeattachment_info` | kube-state-metrics | 标签：attacher / nodeName / persistentVolumeName |
| `csi_sidecar_operations_total` | external-attacher | 按 operation（publish/unpublish）+ succeeded 计数 |
| `csi_sidecar_operations_seconds` | external-attacher | 操作耗时直方图 |
| `attachdetach_controller_force_detached_volumes_total` | kube-controller-manager | 被 force detach 的卷计数（异常信号） |

### 9.2 推荐告警规则

```yaml
groups:
  - name: volumeattachment-alerts
    rules:
      - alert: VolumeAttachmentStuckAttaching
        # VA 创建超过 5 分钟仍未 attached
        expr: |
          kube_volumeattachment_status_attached == 0
          and on(volumeattachment)
          (time() - kube_volumeattachment_created) > 300
        for: 2m
        labels:
          severity: warning
        annotations:
          summary: "VolumeAttachment {{ $labels.volumeattachment }} attach 卡住超过 5 分钟"
          description: "PV {{ $labels.persistentvolume }} 在节点 {{ $labels.nodename }} 上 attach 失败，请检查 external-attacher 与 CSI driver。"

      - alert: VolumeAttachmentStuckDetaching
        # VA 有 deletionTimestamp（kube-state-metrics 通过 status 暴露）且长期不消失
        # 注意：kube-state-metrics v2.8+ 暴露 metadata_deletion_grace_period
        expr: |
          kube_volumeattachment_status_attached == 1
          and on(volumeattachment)
          (kube_volumeattachment_metadata_resource_version) > 0
        # 更精确做法：用自定义 exporter 或 klog 解析 deletionTimestamp
        for: 15m
        labels:
          severity: critical
        annotations:
          summary: "VolumeAttachment {{ $labels.volumeattachment }} detach 卡住"
          description: "卷卸载长时间未完成，可能节点 NotReady 或云盘状态异常。"

      - alert: CSIAttacherOperationFailureRate
        # external-attacher 操作失败率突增
        expr: |
          sum(rate(csi_sidecar_operations_total{succeeded="false",operation=~"publish.*"}[5m]))
          /
          sum(rate(csi_sidecar_operations_total{operation=~"publish.*"}[5m]))
          > 0.3
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "CSI attacher 操作失败率 > 30%"
```

### 9.3 关键 klog 关键词（kube-controller-manager）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在 kube-controller-manager 日志中追踪 AD Controller 行为
kubectl -n kube-system logs kube-controller-manager-<master> | \
  grep -iE "attachdetach|attach.*volume|detach.*volume"

# 关注：
#   "Starting attach/detach controller"        ← 启动
#   "attach volume" / "detach volume"          ← 操作日志
#   "desired state of world" / "actual state"  ← 状态收敛
#   "giving up" / "exceeded retry"             ← 重试上限
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# external-attacher 关键 klog
kubectl -n kube-system logs <csi-controller-pod> -c csi-attacher | \
  grep -iE "controller.*publish|controller.*unpublish|finalizer|error"

# 关注：
#   "Adding finalizer"                        ← 认领 VA
#   "Removing finalizer"                      ← detach 完成
#   "ControllerPublishVolume failed"          ← attach 失败
#   "retrying"                                ← 退避重试
```

---

<!-- chunk: 10. 设计权衡 -->
## 10. 设计权衡与已知边界

### 10.1 为什么把 attach 独立成对象

将 attach 状态从 kube-controller-manager 内存搬到 etcd，带来三大收益：

1. **可观测性**：`kubectl get volumeattachment` 直接看状态，无需进 master 查 klog。
2. **故障恢复**：kube-controller-manager 重启后，actualStateOfWorld 可从 etcd 重建，不会丢失"哪些卷已 attach"的认知。
3. **解耦**：CSI 时代，attach 工作可由独立进程（external-attacher）完成，master 不需云凭证。

### 10.2 已知边界

| 边界 | 说明 |
|:---|:---|
| **force-detach 与数据一致性** | AD Controller 有 force-detach 机制，但仅在节点彻底失联超时后触发；force-detach 仍可能导致底层云盘双挂载，需文件系统层（如 cluster-aware fs）保护 |
| **多 Pod 共享卷的 attach 计数** | AD Controller 维护 per-node 引用计数，所有引用 Pod 都删除后才 detach；这是 `Multi-Attach`（RWX）的基础 |
| **拓扑约束** | WaitForFirstConsumer 模式下，attach 必须在调度决策后；VA 的 nodeName 由调度器决定，不能提前创建 |
| **VA 不是卷的最终真相** | 底层云控制台才是。VA 反映的是 K8s 视角，与云后端可能短暂不一致（如手动在云控制台 detach） |
| **etcd 写放大** | 大规模集群（数万 VA）频繁 status 更新会冲击 etcd；这是 attach 频繁场景下的性能瓶颈 |

### 10.3 与 PV/PVC 保护机制的协同

VA 的 finalizer（`external-attacher/<driver>`）与 PV/PVC 的 finalizer（`kubernetes.io/pv-protection` / `kubernetes.io/pvc-protection`）形成多层防护：

```
PVC 删除 ──▶ pvc-protection finalizer 阻止（直到 Pod 都不再用）
              │
              ▼ 移除后
PV 进入 Released ──▶ pv-protection 阻止（直到 PVC 已删）
              │
              ▼
若 PV 已 detach ──▶ VolumeAttachment 的 external-attacher finalizer 阻止
              │      （直到 ControllerUnpublishVolume 成功）
              ▼
最终：PV 删除、VA 不存在、云盘（按 reclaimPolicy）保留或删除
```

任一层的 finalizer 卡住，都会让上层对象无法回收。详见 [[存储/K8s存储/02-pv-architecture-fundamentals.md|PV 架构基础]]。

---

<!-- chunk: 11. 相关文档 -->
## 11. 相关文档

### 库内深度关联

- [[集群基础/控制平面/22-container-storage-deep-dive.md|容器存储深度剖析]] — 存储全链路（attach + mount）的总览视角，本文是其 attach 章节的源码级展开
- [[存储/K8s存储/05-csi-drivers-integration.md|CSI 驱动集成]] — CSI 生态全景（provisioner/attacher/resizer/snapshotter sidecar 协作）
- [[存储/K8s存储/02-pv-architecture-fundamentals.md|PV 架构基础]] — PV/PVC 绑定与 finalizer 保护机制
- [[存储/K8s存储/16-csi-migration-in-tree-to-csi.md|CSI 迁移：in-tree 到 CSI]] — in-tree 卷如何通过 CSIMigration 走 CSI 路径
- [[集群基础/控制平面/13-kube-controller-manager-deep-dive.md|kube-controller-manager 深度剖析]] — AD Controller 在 KCM 各 controller 中的定位
- [[集群基础/控制平面/15-kubelet-deep-dive.md|kubelet 深度剖析]] — 节点侧 VolumeManager 的 mount/unmount 阶段（attach 的下半场）

### 上游参考

- [kubernetes/csi-api: VolumeAttachment v1](https://kubernetes.io/docs/reference/generated/kubernetes-api/v1.33/#volumeattachment-v1-storage-k8s-io) — 对象 schema 官方定义
- [kubernetes-csi/external-attacher](https://github.com/kubernetes-csi/external-attacher) — sidecar 源码与文档
- [CSI Spec: ControllerPublishVolume](https://github.com/container-storage-interface/spec/blob/master/csi.proto) — gRPC 接口规范
- [KEP-625: CSI Migration](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/625-csi-migration) — in-tree 到 CSI 的设计

---

**本文档要点速记**：

| 关键认知 | 一句话 |
|:---|:---|
| attach vs mount | attach 是节点级（云 API），mount 是 Pod 级（kubelet） |
| CSI 路径的物化 | VolumeAttachment 是 AD Controller 与 external-attacher 间的契约 |
| in-tree vs CSI | in-tree 由 AD Controller 直接调云 API；CSI 由 external-attacher 间接调 |
| finalizer 的意义 | 保证 detach 必先于 VA 消失 |
| 排障第一入口 | `kubectl get volumeattachment -o yaml` 看 status.attached 与 error |
| attachRequired=false | NFS 类驱动无 attach 阶段，不产生 VA |
| 强制删 VA | 仅在确认底层已 detach 后移除 finalizer，否则灾难 |

---

<!-- risk-assessed -->
