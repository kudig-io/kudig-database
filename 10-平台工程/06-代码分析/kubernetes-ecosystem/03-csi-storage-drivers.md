---
title: CSI 存储驱动集成源码分析
description: 基于 kubernetes-1.36.2 pkg/volume/csi 与 alibaba-cloud-csi-driver-1.36.1 源码的 CSI 三服务协议、external sidecar 架构与卷完整生命周期剖析
summary: 从 K8s 侧 csi_attacher/csi_mounter 与驱动侧 CreateVolume/NodePublishVolume 双向剖析（行号实测）CSI 卷从 PVC 创建到容器内挂载的完整链路：external-provisioner/attacher sidecar 分工、AD controller 与 kubelet volumemanager 的职责边界，给出存储链路分层排障方法。
category: source-analysis
tags:
- k8s
- source-code
- csi
- storage
- pv
- pvc
- volume
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 30min
intent_queries:
- CSI 驱动如何与 K8s 集成
- PVC 创建到 Pod 挂载的完整流程
- external-provisioner 与 attacher 的作用
- 卷挂载失败如何分层排查
trigger_keywords:
- CSI
- external-provisioner
- external-attacher
- NodePublishVolume
- VolumeAttachment
- volumemanager
- 卷挂载
related_domains:
- 存储
- 集群基础
- 数据库中间件
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# CSI 存储驱动集成源码分析

> **源码基线**：`33-源码/控制平面/kubernetes-1.36.2/pkg/volume/csi/`（K8s 侧）+ `33-源码/存储/alibaba-cloud-csi-driver-1.36.1/`（驱动侧），行号实测
> 本篇属 [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 生态集成系列]]。

## 一、CSI 协议：三个 gRPC 服务的分工

CSI 与 CRI 同为「K8s 定契约、厂商做实现」的 gRPC 协议，但拆成三个服务、部署在两个位置：

| 服务 | 部署位置 | 职责 | 典型 RPC |
|------|---------|------|---------|
| Identity | 两侧都有 | 自报家门、能力协商 | GetPluginInfo / GetPluginCapabilities |
| Controller | 中心化 Deployment/StatefulSet | 云 API 操作（建盘/挂盘到节点） | CreateVolume / ControllerPublishVolume |
| Node | 每节点 DaemonSet | 本机操作（格式化/mount） | NodeStageVolume / NodePublishVolume |

**K8s 从不直接调 CSI 驱动的 Controller 服务**——中间隔着一层 external sidecar（社区维护的适配器容器，与驱动容器同 Pod 共享 unix socket）：

```
K8s 对象世界                 sidecar（watch K8s → 调 CSI）        CSI 驱动
PVC 创建 ────────▶ external-provisioner ──CreateVolume──▶ 云 API 建盘 → 建 PV
VolumeAttachment ─▶ external-attacher ─ControllerPublish─▶ 云 API 挂盘到节点
PVC 扩容 ────────▶ external-resizer ──ControllerExpand──▶ 云 API 扩盘
VolumeSnapshot ──▶ external-snapshotter ─CreateSnapshot──▶ 云 API 打快照
（Node 服务例外：kubelet 直连节点上的 CSI socket，无 sidecar 中转）
```

这个设计让 CSI 驱动完全不需要理解 K8s：驱动只实现纯粹的 CSI gRPC，watch/informer/对象状态机全部由 sidecar 承担——与 [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|06 篇]]控制器模式的又一次复用。

## 二、卷的完整生命周期：六步链路

```
① Provision   PVC + StorageClass → external-provisioner → CreateVolume(:146) → PV 建立并绑定
② Schedule    调度器（WaitForFirstConsumer 时延迟绑定，拓扑约束在此生效）
③ Attach      AD controller 建 VolumeAttachment → external-attacher → ControllerPublishVolume
④ Stage       kubelet 直调 Node 服务：NodeStageVolume（格式化+挂到节点全局目录，每盘一次）
⑤ Publish     NodePublishVolume(:271)（bind mount 到 Pod 目录，每 Pod 一次）
⑥ 容器启动     CRI CreateContainer 把 Pod 目录 bind 进容器
```

两侧源码锚点：

```go
// K8s 侧 pkg/volume/csi/（实测行号）
// csi_attacher.go —— 第③步的 K8s 内部实现：等 VolumeAttachment.status.attached
func (c *csiAttacher) Attach(logger, spec, nodeName)                    // :63
// csi_mounter.go —— 第⑤步：kubelet 调 NodePublishVolume 前的准备与调用
func (c *csiMountMgr) SetUpAt(dir string, mounterArgs)                  // :103

// 驱动侧 alibaba-cloud-csi-driver-1.36.1/pkg/disk/（实测行号）
func (cs *controllerServer) CreateVolume(ctx, req)                      // controllerserver.go:146  调云 API 创建云盘
func (ns *nodeServer) NodePublishVolume(ctx, req)                       // nodeserver.go:271        bind mount 到 Pod 目录
```

源码级要点：

- **Attach 是「写对象+等状态」**：`csiAttacher.Attach`(:63) 只创建 VolumeAttachment 对象然后轮询 status——真正挂盘的是 external-attacher 驱动的云 API 调用。卡 attach 时先看 `kubectl get volumeattachment` 的 status 与 attacher 日志，而非 kubelet
- **Stage/Publish 两级挂载的意义**：全局目录（Stage）让同一块盘可被同节点多个 Pod（RWO 语义内）bind 共享，格式化只发生一次
- **AD controller vs kubelet volumemanager**：attach/detach 默认由 KCM 内的 AD controller 集中决策（[[10-平台工程/06-代码分析/kubernetes-core/03-kube-controller-manager-deep-dive.md|03 篇]]），kubelet volumemanager（[[10-平台工程/06-代码分析/kubernetes-core/08-kubelet-deep-dive.md|08 篇]]第四节）只负责 mount 侧调谐——节点失联时由中心侧强制 detach（6 分钟超时），避免盘被「幽灵节点」占住

## 三、拓扑感知与延迟绑定

云盘有可用区属性，而 PVC 创建时 Pod 还没调度——先建盘可能建错区。`volumeBindingMode: WaitForFirstConsumer` 把第①步推迟到第②步之后：调度器选定节点 → 节点拓扑标签（`topology.kubernetes.io/zone`）作为 `CreateVolume` 的 `accessibility_requirements` 传入 → 盘建在 Pod 所在区。**多可用区集群 StorageClass 不设 WFFC 是「Pod 与盘不同区永远挂不上」的第一大来源**。

反向约束同样存在：已有 PV 的拓扑会通过调度器的 VolumeBinding 插件（[[10-平台工程/06-代码分析/kubernetes-core/04-kube-scheduler-deep-dive.md|04 篇]]过滤阶段）限制 Pod 只能去盘所在区——StatefulSet Pod「调度不出去」常是盘先锚定了区。

## 四、驱动部署形态与能力矩阵

以 alibaba-cloud-csi-driver 为典型（EBS 类驱动通用形态）：

- **csi-provisioner**（中心 Deployment）：driver 容器 + provisioner/attacher/resizer/snapshotter sidecar
- **csi-plugin**（DaemonSet）：driver 容器 + node-driver-registrar（向 kubelet 插件注册机制报到，socket 路径 `/var/lib/kubelet/plugins_registry/`）
- 能力经 `CSIDriver` 对象声明：`attachRequired: false`（NAS/OSS 类免 attach 步骤）、`podInfoOnMount`、`fsGroupPolicy` 等——同一套协议同时覆盖块存储（disk）、共享文件（nas）、对象存储（ossfs）三类后端，选型对比见 [[06-存储/01-K8s存储/index.md|存储域：K8s 存储]]

## 五、生产排障速查

| 症状 | 生命周期定位 | 检查手段 |
|------|-------------|---------|
| PVC 一直 Pending | ①Provision | provisioner sidecar 日志、StorageClass 参数、云配额；WFFC 模式下先看 Pod 是否已调度 |
| Pod 卡「Multi-Attach error」 | ③Attach | `kubectl get volumeattachment`、原节点是否失联未 detach（RWO 盘被占） |
| Unable to attach or mount volumes 超时 | ③~⑤ | attacher 日志（③）→ 节点 csi-plugin 日志（④⑤）→ volumemanager reconcile（08 篇 :33） |
| 挂载成功但读写报错 | ④Stage 文件系统层 | 盘的 fs 损坏（dmesg）、fsGroup 权限、多 Pod 写 RWO |
| 扩容不生效 | resizer 链路 | PVC status conditions、`allowVolumeExpansion`、文件系统扩容需 Pod 重建或在线 resize 支持 |
| 节点下线后盘卸不掉 | 强制 detach 超时 | AD controller 日志、VolumeAttachment finalizer、云侧盘状态 |

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 系列总览]]
- [[10-平台工程/06-代码分析/kubernetes-core/08-kubelet-deep-dive.md|kubernetes-core 08 - kubelet 源码深度剖析]]（volumemanager 一侧）
- [[10-平台工程/06-代码分析/kubernetes-core/03-kube-controller-manager-deep-dive.md|kubernetes-core 03 - KCM 源码深度剖析]]（AD controller 一侧）
- [[06-存储/01-K8s存储/index.md|存储域：K8s 存储]]
- [[06-存储/01-K8s存储/03-pvc-expansion-guide.md|存储域：PVC 扩容指南]]
- [[07-数据库中间件/00-总览/01-database-on-kubernetes-guide.md|数据库域：Database on K8s]]（有状态负载存储实践）
