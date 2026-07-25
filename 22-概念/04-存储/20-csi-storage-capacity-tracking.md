---
sources:
- "存储/K8s存储/20-csi-storage-capacity-tracking.md"
title: CSIStorageCapacity 与存储容量感知调度
summary: 解析 CSIStorageCapacity 对象如何让 kube-scheduler 在拓扑感知下避免将 Pod 调度到存储不足的节点。
category: concepts
tags:
- csistoragecapacity
- capacity-tracking
- scheduling
- csi
- topology
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
estimated_read_time: 15min
intent_queries:
- CSIStorageCapacity 是什么
- 如何让调度器感知存储容量
- WaitForFirstConsumer 与容量
trigger_keywords:
- CSIStorageCapacity
- capacity
- 容量感知
- topology
- WaitForFirstConsumer
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
> 容量感知调度依赖 CSIStorageCapacity 对象的**时效性**。若驱动长时间未刷新，调度器可能依据过期数据做出错误决策——大规格卷上线前务必核对对象时间戳。

# CSIStorageCapacity 与存储容量感知调度

> **适用版本**: Kubernetes v1.24+（GA） | **API 组**: `storage.k8s.io/v1` | **最后更新**: 2026-07
> **文档定位**: 存储域容量感知调度的独立专题。本篇从源码级解析 CSIStorageCapacity 对象模型、external-provisioner 的容量上报链路，以及 kube-scheduler VolumeBinding 插件如何消费容量数据过滤候选节点。

## 目录

1. [概述](#概述)
2. [问题背景：为什么需要容量感知调度](#问题背景为什么需要容量感知调度)
3. [CSIStorageCapacity 对象模型](#csistoragecapacity-对象模型)
4. [工作机制：容量上报与调度器消费](#工作机制容量上报与调度器消费)
5. [与 topology / WaitForFirstConsumer 的联动](#与-topology--waitforfirstconsumer-的联动)
6. [部署与配置](#部署与配置)
7. [生产排障](#生产排障)
8. [相关文档](#相关文档)

---

## 概述

**CSIStorageCapacity** 是 `storage.k8s.io/v1` 中的一个对象，自 **Kubernetes 1.24 起 GA**（1.21 alpha、1.22 beta、1.24 GA，对应 KEP 1672）。它的核心使命只有一句话：

> 让 kube-scheduler 在为带 `WaitForFirstConsumer` 卷的 Pod 选择节点时，知道「某拓扑段内某 StorageClass 还有多少可用容量」，从而避免把 Pod 调度到节点后才发现底层存储池已满、PV 制备失败。

一句话定位：**它是调度器与 CSI 驱动之间的「容量缓存层」**。CSI 驱动（通过 external-provisioner sidecar）周期性把自己后端的可用容量写入这些对象，调度器在 Filter 阶段读取它们做拓扑过滤。

在引入此对象之前，动态制备的容量是调度器的「盲区」：调度器只看节点资源（CPU/内存/GPU），不知道存储池还剩多少。这在多可用区、本地盘、按需扩容的云盘场景下会导致大量 `Pod stuck in Pending` 与 `FailedScaleOperation`。

### 核心心智模型

```
┌─────────────────────────────────────────────────────────────────────┐
│  CSI 驱动后端存储池                                                   │
│  例: zone=us-east-1a, StorageClass=gp3-fast, 剩余 2 TiB              │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ ① CSI GetCapacity gRPC（周期轮询）
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  external-provisioner sidecar（--enable-capacity）                   │
│  把 GetCapacityResponse 翻译成 CSIStorageCapacity 对象                │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ ② 创建/更新 CSIStorageCapacity（etcd）
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│  kube-scheduler / VolumeBinding 插件（Filter 阶段）                   │
│  遍历候选节点 → 匹配 nodeTopology → 比对 maximumVolumeSize/capacity   │
│  → 过滤掉容量不足的节点                                                │
└─────────────────────────────────────────────────────────────────────┘
```

**关键边界**：CSIStorageCapacity 只对 `volumeBindingMode: WaitForFirstConsumer` 的动态制备生效。Immediate 模式下卷在 Pod 调度前已绑定，调度器不参与拓扑选择，因此容量对象不会被执行。

---

## 问题背景：为什么需要容量感知调度

### 传统动态制备的致命缺陷

考虑一个典型的多可用区集群，每个 zone 有独立的存储后端（AWS EBS gp3、阿里云 ESSD、GCP pd-ssd 都按 zone 隔离）：

```
集群: 3 个 zone（us-east-1a / 1b / 1c）
StorageClass: gp3-xfs
  volumeBindingMode: WaitForFirstConsumer   ← 延迟绑定，等 Pod 调度
  allowedTopologies: [zone in {1a, 1b, 1c}]
```

用户创建一个带 500 GiB PVC 的 Pod。传统流程如下：

```
1. kube-scheduler 为 Pod 评分 → 选中 node-1a（zone=us-east-1a）
   理由: node-1a CPU/内存最空闲
   ⚠️ 调度器不知道 zone-1a 的 gp3 存储池只剩 100 GiB

2. Pod 完成调度，node-1a 被绑定
   → 触发 WaitForFirstConsumer 的 VolumeBinding

3. external-provisioner 收到 PVC，调用 CSI CreateVolume(capacity=500Gi)
   → 底层存储池报错: insufficient capacity

4. PV 制备失败 → PVC Pending → Pod 卡在 ContainerCreating
   → 用户看到的症状: "pod has unbound immediate PersistentVolumeClaims"
```

**根因**：调度器的「拓扑选择」与存储的「容量约束」解耦了。调度器做完决策后，容量约束才在制备阶段暴露，此时节点已经绑定，回退成本极高（需要重新调度）。

### 为什么不能让 provisioner 直接反馈给调度器？

理论上调度器可以在 Filter 阶段同步调用 CSI `GetCapacity`，但有两个硬约束：

1. **同步 gRPC 会让调度延迟爆炸**：Filter 对每个候选节点、每个 PVC 都要调用一次，节点数 × PVC 数 × CSI 延迟，单 Pod 调度可能从毫秒级退化到秒级。
2. **CSI 驱动不在调度器进程内**：调度器不应直接持有到每个 CSI driver controller 的 gRPC 连接，这破坏了 CSI 的边车解耦模型。

CSIStorageCapacity 的解法是**用最终一致性缓存换取解耦**：provisioner 异步把容量写到 etcd，调度器读本地 informer 缓存，零额外 RPC。代价是容量数据有滞后（默认轮询间隔分钟级），但这对绝大多数场景可接受——存储池耗尽是慢过程，不是毫秒事件。

### 容量感知调度带来的收益

| 场景 | 无 CSIStorageCapacity | 有 CSIStorageCapacity |
|------|----------------------|----------------------|
| 多 AZ 云盘 | Pod 调到满池 AZ，PV 制备失败，重试 | 调度器直接跳过满池 AZ |
| 本地盘 / LVM | 本地盘满了才发现，需手动迁移 | 提前避开满盘节点 |
| 按需扩容云盘 | 扩容到配额上限才报错 | maximumVolumeSize 预暴露上限 |
| 大规格卷（TB 级） | 制备失败浪费分钟级时间 | 调度阶段即拦截 |

---

## CSIStorageCapacity 对象模型

CSIStorageCapacity 是 `storage.k8s.io/v1` 下的**集群作用域... 不，是命名空间作用域**对象（与常见误解相反——它放在 namespace 内，方便按驱动部署命名空间做隔离与 owner 引用）。源码定义见 `staging/src/k8s.io/api/storage/v1/types.go`。

### 完整 YAML 示例

```yaml
apiVersion: storage.k8s.io/v1
kind: CSIStorageCapacity
metadata:
  name: csisc-7f3a2c1b-us-east-1a-gp3
  namespace: kube-system                    # 与 provisioner 同命名空间
  # ownerReferences 指向 provisioner 的 Pod/ReplicaSet/Deployment
  # 当 provisioner 被删除时，容量对象自动被 GC，避免遗留过期数据
  ownerReferences:
  - apiVersion: apps/v1
    kind: ReplicaSet
    name: csi-provisioner-xxxxx
    uid: 9a8b7c6d-...
    controller: true
    blockOwnerDeletion: true
# nodeTopology: 该容量覆盖哪些节点。空 LabelSelector = 全集群
nodeTopology:
  matchLabels:
    topology.kubernetes.io/zone: us-east-1a
# storageClassName: 容量对应的 StorageClass 名（不可变）
storageClassName: gp3-xfs
# capacity: CSI GetCapacity 返回的 available_capacity（CSI spec 1.2）
capacity: 2199023255552        # 2 TiB，单位字节
# maximumVolumeSize: CSI spec 1.4 起的更精确字段，表示可创建的最大单卷
maximumVolumeSize: 2147483648000   # ~2000 GiB
```

### 字段逐项解析

下表对应 `CSIStorageCapacity` struct（types.go:707-764）：

| 字段 | 类型 | 作用域 | 说明 |
|------|------|--------|------|
| `metadata.name` | string | 命名空间内唯一 | 无业务含义。推荐 `csisc-<uuid>` 或反向域名（以驱动名结尾）。**必须** DNS 子域，最长 253 字符。 |
| `metadata.namespace` | string | 必填 | **命名空间作用域对象**。通常与 provisioner 部署的 namespace 一致，便于通过 ownerReferences 联动 GC。 |
| `nodeTopology` | `*LabelSelector` | 节点选择器 | 定义「哪些节点能访问这份存储」。**不可变**。语义见下方重点。 |
| `storageClassName` | string | 不可变 | 对应的 StorageClass 名。若该 StorageClass 被删除，对象即「过期」，应由 provisioner 清理。 |
| `capacity` | `*Quantity` | 可选 | 后端存储池**可用字节数**（CSI `GetCapacityResponse.available_capacity`）。CSI spec 1.2 定义。 |
| `maximumVolumeSize` | `*Quantity` | 可选 | **可创建的最大单卷**（CSI spec 1.4+）。比 capacity 更精确，调度器优先用它。 |

### nodeTopology 的三种语义（源码级，极易踩坑）

调度器在 `binder.go:1035` 的 `nodeHasAccess` 中判断节点是否可访问某容量对象：

```go
func (b *volumeBinder) nodeHasAccess(logger klog.Logger, node *v1.Node, capacity *storagev1.CSIStorageCapacity) bool {
    if capacity.NodeTopology == nil {
        // Unavailable —— 注意：返回 false，不是「全部可访问」！
        return false
    }
    selector, err := metav1.LabelSelectorAsSelector(capacity.NodeTopology)
    if err != nil { return false }
    return selector.Matches(labels.Set(node.Labels))
}
```

由此推导出三种 nodeTopology 取值对应完全不同的语义：

| `nodeTopology` 取值 | 语义 | 调度器行为 |
|---------------------|------|-----------|
| **省略（`nil`）** | 不可用 | 所有节点都匹配失败 → 该对象被忽略 |
| **`{}`（空 LabelSelector）** | 全集群可访问 | 所有节点都匹配成功 |
| **`matchLabels: {zone: 1a}`** | 仅该拓扑段 | 只有带 `zone=1a` 标签的节点匹配 |

**最常见的坑**：驱动开发者把 `nodeTopology` 写成 `nil` 期望「全集群可用」，结果调度器认为「全集群都不可用」，导致带此 StorageClass 的 Pod 全部 Pending。

### 「无容量」的三种等价情况（源码注释 types.go:692-696）

API 注释明确指出，以下三种情况都意味着「该拓扑段 + StorageClass 无容量」：

1. **不存在匹配的对象**（没有合适的 nodeTopology + storageClassName 组合）
2. **对象存在但 `capacity` 字段未设置**
3. **对象存在但 `capacity` 为 0**

加上 `maximumVolumeSize` 的处理逻辑（binder.go:1022-1033），完整的判定是：调度器取 `maximumVolumeSize`（若设置）否则回退 `capacity`，**若两者都未设置，视为容量不足**。

```go
func capacitySufficient(capacity *storagev1.CSIStorageCapacity, sizeInBytes int64) bool {
    limit := volumeLimit(capacity)
    return limit != nil && limit.Value() >= sizeInBytes   // nil 即不足
}
func volumeLimit(capacity *storagev1.CSIStorageCapacity) *resource.Quantity {
    if capacity.MaximumVolumeSize != nil {
        return capacity.MaximumVolumeSize   // 优先，更精确
    }
    return capacity.Capacity                // 回退
}
```

**含义**：CSI 驱动若想表达「我有无限容量、不用过滤」，**不能**通过留空字段实现。必须显式给一个很大的 `maximumVolumeSize`（例如 `999Pi`），否则会被当成容量不足。

---

## 工作机制：容量上报与调度器消费

整条链路分「上报方」与「消费方」两端，中间是 etcd 作为异步解耦层。

### 上报方：external-provisioner 的容量控制器

CSI 驱动本身不直接写 CSIStorageCapacity，而是由 **external-provisioner sidecar** 代劳。当 provisioner 以 `--enable-capacity` 启动时，它内部启动一个 capacity controller，周期性执行：

```
┌─────────────────────────────────────────────────────────────┐
│ external-provisioner 容量循环（--capacity-poll-interval）     │
├─────────────────────────────────────────────────────────────┤
│ for each StorageClass 引用的 CSI driver:                     │
│   1. 取该 driver 的 topology 信息（来自 CSINode / driver 自报） │
│   2. for each 拓扑段（如每个 zone）:                           │
│      a. 构造 GetCapacityRequest{topology, parameters}         │
│      b. 调 CSI gRPC GetCapacity()                             │
│      c. 拿到 available_capacity / maximum_volume_size         │
│      d. 构造 CSIStorageCapacity{nodeTopology, storageClassName,│
│         capacity, maximumVolumeSize}                          │
│      e. server-side apply 到 etcd（带 ownerReferences）        │
│   3. 删除不再匹配任何 StorageClass 的过期对象                  │
└─────────────────────────────────────────────────────────────┘
```

**关键设计点**：

- **ownerReferences 联动 GC**：每个对象把 provisioner 所在的 Deployment/ReplicaSet 设为 owner。provisioner 被卸载时，对象自动被 Kubernetes GC 清理，不残留过期容量数据。这也是 RBAC 里需要 `pods/get`、`replicasets/get` 权限的原因（沿 ownership chain 找 owner）。
- **拓扑段划分**：capacity controller 用 CSI driver 在 `GetPluginInfo` / Node 拓扑里声明的 `topologyKeys` 把集群切成多个段，每段一份对象。
- **幂等写**：用 `server-side apply` 或 patch，避免并发 provisioner 副本冲突（leader election 模式下只有主副本写）。

### 消费方：kube-scheduler 的 VolumeBinding 插件

调度器的容量消费发生在 **VolumeBinding 插件**（`pkg/scheduler/framework/plugins/volumebinding`）。它在 Filter 阶段对每个候选节点执行 `hasEnoughCapacity`（binder.go:976）：

```go
func (b *volumeBinder) hasEnoughCapacity(logger, provisioner, claim, storageClass, node) (bool, ...) {
    quantity, ok := claim.Spec.Resources.Requests[v1.ResourceStorage]
    if !ok { return true, nil, nil }   // PVC 没指定大小，不检查

    driver, err := b.csiDriverLister.Get(provisioner)
    if err != nil {
        if IsNotFound(err) { return true, nil, nil }  // 非 CSI 或未开启
        return false, nil, err
    }
    // 关键：只有 CSIDriver.Spec.StorageCapacity == true 才启用检查
    if driver.Spec.StorageCapacity == nil || !*driver.Spec.StorageCapacity {
        return true, nil, nil   // 该驱动未 opt-in，跳过
    }

    capacities, _ := b.csiStorageCapacityLister.List(labels.Everything())
    sizeInBytes := quantity.Value()
    for _, capacity := range capacities {
        if capacity.StorageClassName == storageClass.Name &&
           capacitySufficient(capacity, sizeInBytes) &&
           b.nodeHasAccess(node, capacity) {
            return true, capacity, nil   // 找到一个够用的即可
        }
    }
    // V(5) 日志: "Node has no accessible CSIStorageCapacity with enough capacity"
    return false, nil, nil
}
```

**读取链路**：调度器通过 informer 本地缓存读取，**零 gRPC 调用、零 etcd 读**——这是容量感知调度性能开销可忽略的根本原因。

### 完整时序流程图

```
                    ┌──────────────────┐
                    │  CSI Driver 控制面 │
                    │  (Controller Plugin)│
                    └────────┬─────────┘
                             │ ① gRPC GetCapacity
                             │   (topology, parameters)
                             ▼
   ┌──────────────────────────────────────────┐
   │ external-provisioner (--enable-capacity)  │
   │  capacity controller                       │
   │  poll interval: --capacity-poll-interval   │
   └────────┬──────────────────────────────────┘
            │ ② 创建/更新（SSA + ownerRef）
            ▼
   ┌──────────────────────┐   informer watch   ┌──────────────────────┐
   │      etcd            │ ─────────────────▶ │  kube-scheduler       │
   │ CSIStorageCapacity   │                     │  VolumeBinding Filter │
   │ (per zone per SC)    │ ◀───────────────── │  (本地 informer 缓存)   │
   └──────────────────────┘   不需要回写         └──────────┬───────────┘
                                                              │ ③ Filter
                                                              ▼
                                              ┌───────────────────────────┐
                                              │ 候选节点 = [node-1a, 1b, 1c] │
                                              │ 过滤掉 capacity 不足的       │
                                              │ → 剩余节点进入 Score 阶段     │
                                              └───────────────────────────┘
```

### 性能特征

| 维度 | 数值/特性 |
|------|----------|
| provisioner → driver | gRPC，每拓扑段一次，间隔默认分钟级 |
| provisioner → etcd | 写 QPS = 拓扑段数 × StorageClass 数 / poll interval |
| scheduler 读 | 纯本地缓存，每次 Filter 遍历全部对象（O(N)）|
| 大集群隐患 | N 节点 × M PVC × K 容量对象，K 很大时 Filter 有成本。源码 TODO 提到未来要加索引 |

源码注释（binder.go:999）保留了优化 TODO：
```go
// TODO (for beta): benchmark this and potentially introduce some kind of lookup structure
// (https://github.com/kubernetes/enhancements/issues/1698#issuecomment-654356718).
```
即当前实现是**线性扫描全部 CSIStorageCapacity**。在中型集群（数百对象）无压力，但超大规模集群需关注 scheduler profile。

---

## 与 topology / WaitForFirstConsumer 的联动

容量感知调度不是孤立机制，它深深嵌入 CSI 拓扑体系。理解联动需先明确各对象的职责分工。

### 仅对 WaitForFirstConsumer 生效

这是最核心的前提。两种 volumeBindingMode 的差异：

```yaml
# StorageClass A: 立即绑定 —— 调度器不参与
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata: { name: immediate-sc }
provisioner: disk.csi.cloud.com
volumeBindingMode: Immediate    # ← PVC 创建即制备，PV 先于 Pod 存在
# 调度器看到 PVC 时 PV 已绑定，不做拓扑选择 → CSIStorageCapacity 不生效

---
# StorageClass B: 延迟绑定 —— 调度器主导，容量感知生效
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata: { name: wffc-sc }
provisioner: disk.csi.cloud.com
volumeBindingMode: WaitForFirstConsumer   # ← 等 Pod 调度才绑定
# 调度器在 Filter 阶段为该 PVC 选节点 → 读取 CSIStorageCapacity 过滤
```

**为什么 Immediate 不生效**：Immediate 模式下，PVC 一创建 external-provisioner 就立刻制备 PV（在某个拓扑段），之后 Pod 调度时只能去 PV 所在拓扑段找节点——调度器无拓扑选择权，自然也用不上容量数据。

> 若希望在 Immediate 模式下也利用容量数据避免制备失败，provisioner 提供 `--capacity-for-immediate-binding` 选项：开启后，provisioner 在制备 Immediate 卷前会先检查是否有足够容量，不足则让 PVC 进入 Pending 重试，而非直接调 CSI 失败。注意这仍是 provisioner 侧的预防，不是调度器侧的过滤。

### 与 CSIDriver / CSINode 的三元组

容量感知调度依赖三个对象的配合：

```
CSIDriver（驱动声明）         CSINode（节点注册）        CSIStorageCapacity（容量）
┌─────────────────────┐     ┌────────────────────┐     ┌─────────────────────┐
│ spec.storageCapacity│     │ spec.driver.name   │     │ storageClassName    │
│   = true   ← opt-in │     │ nodeID / topology  │     │ nodeTopology        │
│ spec.volumeLifecycle│     │ accessibleMounts   │     │ capacity            │
│ ...                 │     │ ...                │     │ maximumVolumeSize   │
└─────────────────────┘     └────────────────────┘     └─────────────────────┘
        │                          │                           │
        │ ① 调度器查: 该驱动        │ ② 调度器查: 节点上装了      │ ③ 调度器查: 容量够不够
        │   是否 opt-in 容量调度     │   哪些 driver、拓扑标签     │   （仅当①为 true）
        ▼                          ▼                           ▼
```

**`CSIDriver.spec.storageCapacity`（types.go:363-380）** 是总开关：

```go
// storageCapacity indicates that the CSI volume driver wants pod scheduling
// to consider the storage capacity that the driver deployment will report by
// creating CSIStorageCapacity objects with capacity information, if set to true.
//
// The check can be enabled immediately when deploying a driver.
// In that case, provisioning new volumes with late binding
// will pause until the driver deployment has published
// some suitable CSIStorageCapacity object.
//
// +featureGate=CSIStorageCapacity
StorageCapacity *bool `json:"storageCapacity,omitempty"`
```

**关键行为**：当 `storageCapacity: true` 但驱动尚未发布任何 CSIStorageCapacity 对象时，所有 WaitForFirstConsumer 的 PVC 都会**卡住**——调度器找不到匹配对象，认为容量不足。这是「先发车后修路」的常见错误。

正确上线顺序：
1. 部署 CSIDriver（先设 `storageCapacity: false` 或留空）
2. 部署带 `--enable-capacity` 的 provisioner，确认 CSIStorageCapacity 对象已生成
3. 此时再把 `CSIDriver.spec.storageCapacity` 改为 `true`（1.23+ 起可变）

### 特性门控演进

| 版本 | 状态 | 特性门控 |
|------|------|----------|
| 1.21 | alpha | `CSIStorageCapacity` 默认关 |
| 1.22 | beta | 默认开，API `storage.k8s.io/v1beta1` |
| 1.24 | **GA** | API 升 `storage.k8s.io/v1`；门控锁死为开 |
| 1.27+ | GA | 门控移除（无法关闭） |

本仓库源码（`kubernetes-release-1.28/1.30/1.32/1.34`）均已为 GA，无需手动开 feature gate。`v1beta1` API 在 1.27 弃用、1.31 起完全移除——新部署必须用 `storage.k8s.io/v1`。

### 与 DRA（Dynamic Resource Allocation）的关系

Kubernetes 1.26+ 引入的 DRA 有**独立的容量模型**（`ResourceClaim` + `ResourceClass`），不复用 CSIStorageCapacity。原因：

- DRA 面向更通用的资源（GPU、FPGA、特殊存储），容量语义更复杂
- DRA 的调度走单独的 `DynamicResources` 插件，不走 VolumeBinding
- CSI 驱动可同时支持两套：传统 PV 走 CSIStorageCapacity，DRA 资源走 ResourceClaimTemplate

短期内（1.28–1.33）CSIStorageCapacity 仍是 CSI 存储容量感知的事实标准；DRA 的 CSI 集成在 1.32+ 才逐步成熟。详见 [[01-集群基础/03-控制平面/22-container-storage-deep-dive.md|容器存储深度剖析]]。

---

## 部署与配置

### 前置条件清单

- [ ] Kubernetes ≥ 1.24（GA，无需 feature gate）
- [ ] CSI 驱动实现了 `CONTROLLER_SERVICE.GetCapacity` RPC（CSI spec 1.2+；`maximum_volume_size` 需 1.4+）
- [ ] StorageClass 使用 `WaitForFirstConsumer` 绑定模式
- [ ] 节点带拓扑标签（如 `topology.kubernetes.io/zone`）

### 第一步：声明 CSIDriver 开启容量感知（可后置）

```yaml
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: disk.csi.cloud.example.com
spec:
  attachRequired: true
  podInfoOnMount: true
  # storageCapacity: true   ← 建议先用 false，确认对象生成后再开
  storageCapacity: false
```

🟢 **查看当前 CSIDriver 配置**（只读，无副作用）：
```bash
kubectl get csidriver disk.csi.cloud.example.com -o yaml | grep -A1 storageCapacity
```

### 第二步：部署带容量上报的 external-provisioner

完整的 Deployment 片段（生产可用）。关键 args 用注释标注：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: csi-provisioner
  namespace: kube-system
spec:
  replicas: 3                      # 建议 ≥2 配合 leader election
  selector: { matchLabels: { app: csi-provisioner } }
  template:
    metadata: { labels: { app: csi-provisioner } }
    spec:
      serviceAccountName: csi-provisioner
      containers:
      - name: csi-provisioner
        image: registry.k8s.io/sig-storage/csi-provisioner:v5.1.0
        args:
          # —— 基础参数 ——
          - --csi-address=$(ADDRESS)
          - --v=5
          - --leader-election=true          # 多副本必须开
          - --leader-election-namespace=kube-system

          # —— 容量感知核心参数 ——
          - --enable-capacity=true          # ★ 启用容量控制器（总开关）
          - --capacity-for-immediate-binding=false
            # false（默认）: Immediate 卷不查容量
            # true: Immediate 卷也先查容量，不足则 PVC Pending 重试
          - --capacity-poll-interval=5m0s
            # 轮询 CSI GetCapacity 的间隔。默认 5 分钟。
            # 越短数据越新鲜，但对 CSI driver 压力越大

          # —— 高级可选 ——
          # - --capacity-ownerref-level=2
          #   沿 ownership chain 几层设 owner: 0=Pod, 1=ReplicaSet, 2=Deployment
          #   Deployment 部署用 2，StatefulSet 用 1
        env:
        - name: ADDRESS
          value: /csi/csi.sock
        volumeMounts:
        - name: socket-dir
          mountPath: /csi
      volumes:
      - name: socket-dir
        hostPath: { path: /var/lib/kubelet/plugins/disk.csi.cloud.example.com }
```

### 第三步：补充 RBAC（官方 rbac.yaml 关键片段）

provisioner 需要对 `csistoragecapacities` 的全套权限（来自本仓库源码 `test/e2e/testing-manifests/storage-csi/external-provisioner/rbac.yaml`）：

```yaml
# 以下是 Role（命名空间级），因为 CSIStorageCapacity 是命名空间对象
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: kube-system
  name: external-provisioner-cfg
rules:
- apiGroups: ["storage.k8s.io"]
  resources: ["csistoragecapacities"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
# 沿 ownership chain 找 owner（Pod → ReplicaSet → Deployment）
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get"]
- apiGroups: ["apps"]
  resources: ["replicasets"]
  verbs: ["get"]
```

**注意**：`csistoragecapacities` 的写权限在 **Role**（命名空间级）而非 ClusterRole。因为对象是命名空间作用域，provisioner 只在自己部署的 namespace 内创建对象。漏配这一条会导致 provisioner 日志狂刷 `forbidden: cannot create csistoragecapacities`。

### 第四步：验证对象已生成

🟢 **列出所有容量对象**（只读）：
```bash
kubectl get csistoragecapacity -A
# 期望输出示例:
# NAMESPACE      NAME                                STORAGECLASS   CAPACITY   VOLUMEPLUGIN
# kube-system    csisc-7f3a-1a-gp3                   gp3-xfs        2Ti        
# kube-system    csisc-7f3a-1b-gp3                   gp3-xfs        1800Gi     
# kube-system    csisc-7f3a-1c-gp3                   gp3-xfs        0          
```

🟢 **查看单个对象详情**（只读）：
```bash
kubectl get csistoragecapacity csisc-7f3a-1a-gp3 -n kube-system -o yaml
# 重点核对: nodeTopology / storageClassName / capacity / maximumVolumeSize
```

### 第五步：开启 CSIDriver 容量开关

🟡 **修改 CSIDriver 开启容量感知**（会改变调度行为，回滚只需改回 false）：
```bash
kubectl patch csidriver disk.csi.cloud.example.com --type=merge \
  -p '{"spec":{"storageCapacity":true}}'
```

> 开启后立即生效。新调度的 WaitForFirstConsumer PVC 将受容量过滤。已调度的 Pod 不受影响。

---

## 生产排障

按「现象 → 定位 → 修复」组织。所有排障命令默认 🟢 只读，涉及修改处另标。

### 现象 1：WaitForFirstConsumer 的 PVC 一直 Pending

**典型事件**：
```
Warning  FailedScheduling  default-scheduler
  0/12 nodes are available: pod has unbound immediate PersistentVolumeClaims.
  6 Insufficient storage capacity for PVC ...
```

定位步骤：

🟢 **第一步：确认该驱动开启了容量感知**：
```bash
kubectl get csidriver -o custom-columns=NAME:.metadata.name,CAPACITY:.spec.storageCapacity
```
若目标驱动 `storageCapacity` 为 `<none>` 或 `false`，但调度日志却报 capacity 不足，说明驱动被错误标记——要么改 false，要么补齐对象。

🟢 **第二步：确认对象存在且匹配**：
```bash
# 按 StorageClass 过滤
kubectl get csistoragecapacity -A -o json | \
  jq '.items[] | select(.storageClassName=="gp3-xfs") | {name:.metadata.name, ns:.metadata.namespace, topo:.nodeTopology, cap:.capacity, max:.maximumVolumeSize}'
```
常见问题：
- 对象 `capacity` 为 `0` 或 `null` → 调度器视为不足
- 对象 `nodeTopology` 为 `null` → 视为不可访问（见上文三种语义）
- PVC 请求大小 > `maximumVolumeSize` → 被过滤

🟢 **第三步：看调度器日志的过滤记录**：
```bash
# 调度器在 V(5) 打印过滤原因
kubectl -n kube-system logs kube-scheduler-<master> -v=5 | \
  grep "no accessible CSIStorageCapacity"
```

### 现象 2：CSIStorageCapacity 对象根本没生成

🟢 **确认 provisioner 带了正确参数**：
```bash
kubectl -n kube-system get deploy csi-provisioner -o jsonpath='{.spec.template.spec.containers[*].args}' | tr ' ' '\n' | grep -i capacity
# 必须看到: --enable-capacity=true
```

🟢 **查 provisioner 日志找 capacity 控制器报错**：
```bash
kubectl -n kube-system logs deploy/csi-provisioner -c csi-provisioner | grep -i "capacity\|GetCapacity"
```
常见根因：
- `--enable-capacity` 未传 → 控制器根本没启动
- CSI driver 未实现 `GetCapacity` → 日志报 `rpc error: code = Unimplemented`
- StorageClass 的 `parameters` 与 GetCapability 不匹配 → driver 拒绝

🟢 **确认 RBAC 已授权**：
```bash
kubectl -n kube-system auth can-i create csistoragecapacities --as system:serviceaccount:kube-system:csi-provisioner
# 期望: yes
```

### 现象 3：调度器过滤了「明明够用」的节点（数据过期）

CSIStorageCapacity 是缓存，存在**时效性问题**：

- provisioner 每 `--capacity-poll-interval` 刷新一次
- 若后端容量在两次轮询之间耗尽（如他人并发抢占了配额），调度器仍会用旧数据放行
- 反之，后端已释放容量但对象还显示不足，Pod 会被错误过滤

🟢 **核对对象新鲜度**：
```bash
kubectl get csistoragecapacity -A -o custom-columns=NAME:.metadata.name,SC:.storageClassName,AGE:.metadata.creationTimestamp
```
- 若 `AGE` 远大于 poll interval（如间隔 5m 但对象几小时没更新），说明 provisioner 容量循环卡死
- `creationTimestamp` 是创建时间，更新只改 `resourceVersion`；更准确看：
```bash
kubectl get csistoragecapacity <name> -o jsonpath='{.metadata.resourceVersion}'
# 多次执行，若 resourceVersion 长期不变，说明没在刷新
```

🟡 **临时缩短轮询间隔**（会加重 driver 负载，谨慎）：
```bash
kubectl -n kube-system patch deploy csi-provisioner --type='json' \
  -p='[{"op":"replace","path":"/spec/template/spec/containers/0/args/-","value":"--capacity-poll-interval=2m0s"}]'
```

### 现象 4：CSIDriver 已开 storageCapacity 但驱动不实现 GetCapacity

某些驱动（如较老的 host-path、部分 NFS driver）未实现 `GetCapacity` RPC。此时若强行开 `storageCapacity: true`，provisioner 会持续报错，且永不生成对象，导致 WaitForFirstConsumer PVC 全部 Pending。

🟢 **判断驱动是否支持**：
```bash
# 看 CSIDriver 的 controller capabilities（部分驱动在 CSIDriver 对象或部署 yaml 标注）
kubectl get csidriver <name> -o yaml | grep -i capacity
# 或直接看 provisioner 启动日志
kubectl -n kube-system logs deploy/csi-provisioner | grep -i "getcapacity\|unsupported"
```
若不支持，**不要**开 `storageCapacity: true`，改回 `false` 或删除该字段。

### 现象 5：升级后 CSIStorageCapacity 消失（v1beta1 → v1）

1.27 前 `storage.k8s.io/v1beta1` 的对象在升级到 1.31+（移除 v1beta1）后会丢失。provisioner 升级后会自动按 v1 重建，但**过渡期**可能出现对象断档：

🟢 **核对 API 版本**：
```bash
kubectl api-resources | grep csistoragecapacities
# 期望仅看到 storage.k8s.io/v1
```
确保 provisioner 镜像版本 ≥ v3.0.0（用 v1 API）。旧版 provisioner 写 v1beta1 对象，升级后被拒。

### 排障速查表

| 现象 | 最可能根因 | 验证命令 |
|------|-----------|----------|
| WFFC PVC 一直 Pending | 驱动未开 storageCapacity / 对象不匹配 | `kubectl get csidriver -o yaml` |
| 对象根本不存在 | provisioner 缺 `--enable-capacity` | 查 args + 日志 |
| 对象存在但被忽略 | nodeTopology 为 nil 或 capacity 为 0 | `-o yaml` 看字段 |
| 部分节点被错误过滤 | 数据过期 / 拓扑标签缺失 | 看 resourceVersion + node labels |
| provisioner 日志报 forbidden | Role 缺 csistoragecapacities 权限 | `auth can-i` |

---

## 设计反思与边界

### 优点

1. **零调度延迟开销**：inform 缓存读取，无同步 RPC
2. **解耦清晰**：CSI 驱动、provisioner、调度器三者通过 etcd 解耦
3. **GC 友好**：ownerReferences 联动，无残留垃圾
4. **渐进启用**：CSIDriver.storageCapacity 可后置切换，降低上线风险

### 固有局限

1. **最终一致性**：容量数据有滞后，并发抢占配额时仍可能失败
2. **无预留机制**：对象只反映「当前可用」，不能为未创建的卷预留配额
3. **线性扫描**：调度器 O(N) 扫描全部对象，超大规模集群需关注
4. **不覆盖 Immediate 默认**：默认只对 WFFC 生效，Immediate 需额外参数
5. **不感知已用配额**：只看「可用」，不看「该 SC 已分配多少」，配额管理仍需 ResourceQuota

### 何时不该用

- **超富余存储池**（如自建 Ceph 集群容量远超需求）：capacity controller 徒增复杂度，收益有限
- **NFS / 共享文件系统**：通常 `Immediate` + ReadWriteMany，不经过调度器拓扑选择
- **驱动未实现 GetCapacity**：开了也是噪音，保持 `storageCapacity: false`
- **DRA 资源**：走 ResourceClaim，不复用本机制

---

## 相关文档

- [[01-集群基础/03-控制平面/22-container-storage-deep-dive.md|容器存储深度剖析]] — CSI 三段式 attach/mount 全链路，理解 capacity 在其中的位置
- [[06-存储/01-K8s存储/04-storageclass-dynamic-provisioning.md|StorageClass 动态制备]] — StorageClass 字段与 volumeBindingMode 详解
- [[06-存储/07-AI存储与高级/05-csi-topology-awareness.md|CSI 拓扑感知]] — topologyKey、AllowedTopologies、跨 AZ 调度排障
- [[06-存储/01-K8s存储/05-csi-drivers-integration.md|CSI 驱动集成与运维管理]] — 主流 CSI 驱动部署与 sidecar 配置矩阵

<!-- risk-assessed -->
