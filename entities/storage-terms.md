---
title: K8s 存储术语参考
description: '# K8s 存储术语参考'
summary: '本页汇总了 **存储** 领域的 17 个 Kubernetes 术语定义与概念说明。'
category: references
tags:
- k8s
- dictionary
- storage
- kubelet
- job
- cronjob
- gpu
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- K8s 存储术语参考 是什么
- 如何 K8s 存储术语参考
trigger_keywords:
- K8s
- 存储术语参考
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---



# K8s 存储术语参考

本页汇总了 **存储** 领域的 17 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[entities/k8s-storage-ecosystem.md|k8s-storage-ecosystem]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **CSI Volume Cloning（CSI 卷克隆）** | Csi Volume Cloning | CSI 卷克隆功能允许用户在创建新的 PersistentVolumeClaim（PVC）时，通过引用一个已有的 PVC 作为数据源，让后端存储系统创建一... |
| **Dynamic Volume Provisioning（动态卷供给）** | Dynamic Volume Provisioning | 动态卷供给允许存储卷在需要时按需创建 |
| **Ephemeral Volumes（临时卷）** | Ephemeral Volumes | Ephemeral Volumes（临时卷）是生命周期与 Pod 绑定的存储卷，随 Pod 的创建而创建，随 Pod 的删除而删除 |
| **高性能存储网络（RDMA / NVMe-oF）** | High Performance Storage Networks | 在 AI 训练、高性能计算（HPC）和低延迟数据库场景中，存储 I/O 往往是整体性能的瓶颈 |
| **Local ephemeral storage（本地临时存储）** | Local Ephemeral Storage | 节点的本地临时存储由本地可写设备（如磁盘）或 RAM 支持 |
| **Node-specific Volume Limits（节点特定卷限制）** | Node Specific Volume Limits | Kubernetes 需要尊重每个节点可以附加（attach）的卷数量上限 |
| **对象存储与数据流水线** | Object Storage And Data Pipelines | 在 Kubernetes 上运行 AI/ML、大数据和云原生应用时，**对象存储（Object Storage）** 已成为海量非结构化数据的事实标准存储层 |
| **Persistent Volumes（持久卷）** | Persistent Volumes | PersistentVolume（PV）和 PersistentVolumeClaim（PVC）是 Kubernetes 中用于抽象存储供给与消费的 AP... |
| **Projected Volumes（投射卷）** | Projected Volumes | Projected Volume 是一种将多个现有的卷源（如 Secret、ConfigMap、downwardAPI、serviceAccountTok... |
| **Storage Capacity（存储容量）** | Storage Capacity | 存储容量跟踪是 Kubernetes 在 v1 |
| **Storage Classes（存储类）** | Storage Classes | StorageClass 是 Kubernetes 中用于描述管理员所提供的存储“类别”的 API 资源 |
| **Volume Attributes Classes（卷属性类）** | Volume Attributes Classes | VolumeAttributesClass（VAC）是 Kubernetes 在 v1 |
| **Volume Health Monitoring（卷健康监控）** | Volume Health Monitoring | 卷健康监控是 Kubernetes CSI 实现的一部分，允许 CSI 驱动检测底层存储系统的异常卷状态，并将这些异常作为事件报告到相关的 Persist... |
| **Volume Snapshot Classes（卷快照类）** | Volume Snapshot Classes | VolumeSnapshotClass 与 StorageClass 类似，它提供了一种由管理员描述快照“类别”的机制 |
| **Volume Snapshots（卷快照）** | Volume Snapshots | 在 Kubernetes 中，VolumeSnapshot 表示对存储系统上某个卷在特定时间点的快照 |
| **Volumes（卷）** | Volumes | Kubernetes Volumes 为 Pod 中的容器提供了一种通过文件系统访问和共享数据的机制 |
| **Windows Storage（Windows 存储）** | Windows Storage | Windows 节点上的存储行为与 Linux 节点存在显著差异，主要是由于 Windows 的文件系统架构、NTFS、注册表和 SAM（Security... |

---

### CSI Volume Cloning（CSI 卷克隆）

CSI 卷克隆功能允许用户在创建新的 PersistentVolumeClaim（PVC）时，通过引用一个已有的 PVC 作为数据源，让后端存储系统创建一个与源卷内容完全相同的副本。克隆在功能上与普通卷相同，唯一的区别是在供给时不会创建空卷，而是复制已有卷的数据。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/csi-volume-cloning.md`）*

---

### Dynamic Volume Provisioning（动态卷供给）

动态卷供给允许存储卷在需要时按需创建。没有动态供给时，集群管理员必须手动联系云或存储提供商创建新存储，然后再在 Kubernetes 中创建 PersistentVolume 对象来表示它们。动态供给消除了这一繁琐过程，当用户创建 PersistentVolumeClaim（PVC）时，系统会自动为其创建相应的存储卷。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/dynamic-volume-provisioning.md`）*

---

### Ephemeral Volumes（临时卷）

Ephemeral Volumes（临时卷）是生命周期与 Pod 绑定的存储卷，随 Pod 的创建而创建，随 Pod 的删除而删除。它们适用于不需要数据在 Pod 重启后仍然持久保存的场景，如缓存、临时工作区或只读输入数据。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/ephemeral-volumes.md`）*

---

### 高性能存储网络（RDMA / NVMe-oF）

在 AI 训练、高性能计算（HPC）和低延迟数据库场景中，存储 I/O 往往是整体性能的瓶颈。传统的 TCP/IP 网络存储协议（如 NFS、iSCSI）在带宽和延迟上已无法满足万卡 GPU 集群和 NVMe 全闪存阵列的需求。**RDMA（Remote Direct Memory Access）** 和 **NVMe over Fabrics（NVMe-oF）** 通过绕过操作系统内核、直接在网络适配器和内存之间传输数据，将存储访问延迟从毫秒级降低到微秒级。2026 年，这两项技术正在成为 Kubernetes 上高性能存储的核心支撑。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/high-performance-storage-networks.md`）*

---

### Local ephemeral storage（本地临时存储）

节点的本地临时存储由本地可写设备（如磁盘）或 RAM 支持。“临时”意味着 Kubernetes 不提供长期的持久性保证。Pod 使用本地临时存储作为临时工作区、缓存和日志存放位置。kubelet 也使用此类存储来保存容器镜像、运行中容器的可写层以及节点级容器日志。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/local-ephemeral-storage.md`）*

---

### Node-specific Volume Limits（节点特定卷限制）

Kubernetes 需要尊重每个节点可以附加（attach）的卷数量上限。云厂商（如 AWS、GCP、Azure）通常对每块虚拟机可挂载的磁盘数量有限制。如果不遵守这些限制，调度到该节点的 Pod 可能会因卷无法附加而卡在等待状态。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/node-specific-volume-limits.md`）*

---

### 对象存储与数据流水线

在 Kubernetes 上运行 AI/ML、大数据和云原生应用时，**对象存储（Object Storage）** 已成为海量非结构化数据的事实标准存储层。相比块存储和文件系统，对象存储具有**近乎无限的扩展性、较低的成本和天然的云原生 API 接口**。2026 年的最佳实践要求 Kubernetes 平台具备高效的对象存储集成能力，以及基于 Kubernetes 原生资源（Jobs/CronJobs/Argo Workflows）编排的**数据流水线（Data Pipelines）**。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/object-storage-and-data-pipelines.md`）*

---

### Persistent Volumes（持久卷）

PersistentVolume（PV）和 PersistentVolumeClaim（PVC）是 Kubernetes 中用于抽象存储供给与消费的 API 资源。PV 代表集群中的一块存储，由管理员预先创建或通过 StorageClass 动态供给；PVC 是用户对存储的请求，类似于 Pod 消耗节点资源，PVC 消耗 PV 资源。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/persistent-volumes.md`）*

---

### Projected Volumes（投射卷）

Projected Volume 是一种将多个现有的卷源（如 Secret、ConfigMap、downwardAPI、serviceAccountToken 等）映射到同一个目录中的卷类型。它提供了一种“一体化”的方式，将不同来源的数据集中投射到容器的文件系统中。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/projected-volumes.md`）*

---

### Storage Capacity（存储容量）

存储容量跟踪是 Kubernetes 在 v1.24 达到稳定（stable）的一项功能。它使 Kubernetes 能够跟踪集群中各节点的可用存储容量，并在调度 Pod 时将其作为考量因素，从而减少因节点存储不足导致的调度失败和重试。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/storage-capacity.md`）*

---

### Storage Classes（存储类）

StorageClass 是 Kubernetes 中用于描述管理员所提供的存储“类别”的 API 资源。不同的 StorageClass 可以映射到不同的服务质量（QoS）级别、备份策略或任意由集群管理员定义的策略。它使得用户无需了解底层存储的实现细节，即可按需请求不同特性的持久存储。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/storage-classes.md`）*

---

### Volume Attributes Classes（卷属性类）

VolumeAttributesClass（VAC）是 Kubernetes 在 v1.34 中达到 GA（默认启用）的一项功能，它允许管理员定义存储的可变“属性类”。与 StorageClass 主要关注卷的初始供给不同，VolumeAttributesClass 关注的是已创建卷的属性修改，例如调整 IOPS 或吞吐量。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/volume-attributes-classes.md`）*

---

### Volume Health Monitoring（卷健康监控）

卷健康监控是 Kubernetes CSI 实现的一部分，允许 CSI 驱动检测底层存储系统的异常卷状态，并将这些异常作为事件报告到相关的 PersistentVolumeClaim（PVC）或 Pod 上，帮助用户和运维人员及时发现存储问题。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/volume-health-monitoring.md`）*

---

### Volume Snapshot Classes（卷快照类）

VolumeSnapshotClass 与 StorageClass 类似，它提供了一种由管理员描述快照“类别”的机制。当动态创建卷快照时，VolumeSnapshotClass 定义了使用哪个 CSI 驱动、删除策略以及特定于存储提供商的参数。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/volume-snapshot-classes.md`）*

---

### Volume Snapshots（卷快照）

在 Kubernetes 中，VolumeSnapshot 表示对存储系统上某个卷在特定时间点的快照。卷快照为用户提供了一种标准化的方式，用于在不创建全新卷的情况下复制卷的内容。此功能对于数据库备份、灾难恢复和数据迁移等场景非常重要。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/volume-snapshots.md`）*

---

### Volumes（卷）

Kubernetes Volumes 为 Pod 中的容器提供了一种通过文件系统访问和共享数据的机制。容器内的磁盘文件默认是临时的，容器崩溃或停止后数据会丢失。Volume 解决了数据持久化和容器间共享存储的问题。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/volumes.md`）*

---

### Windows Storage（Windows 存储）

Windows 节点上的存储行为与 Linux 节点存在显著差异，主要是由于 Windows 的文件系统架构、NTFS、注册表和 SAM（Security Account Manager）数据库的隔离机制。Kubernetes 在 Windows 上支持部分卷类型和功能，但也有一些 Linux 特有的功能不被支持。

> *（内容已精简，完整版请参阅源文件 `domain-17-system-foundation/topic-dictionary/storage/windows-storage.md`）*

---

## 相关页面

- [[entities/k8s-storage-ecosystem.md|k8s-storage-ecosystem]]

## 来源文件

- `domain-17-system-foundation/topic-dictionary/storage/csi-volume-cloning.md`
- `domain-17-system-foundation/topic-dictionary/storage/dynamic-volume-provisioning.md`
- `domain-17-system-foundation/topic-dictionary/storage/ephemeral-volumes.md`
- `domain-17-system-foundation/topic-dictionary/storage/high-performance-storage-networks.md`
- `domain-17-system-foundation/topic-dictionary/storage/local-ephemeral-storage.md`
- `domain-17-system-foundation/topic-dictionary/storage/node-specific-volume-limits.md`
- `domain-17-system-foundation/topic-dictionary/storage/object-storage-and-data-pipelines.md`
- `domain-17-system-foundation/topic-dictionary/storage/persistent-volumes.md`
- `domain-17-system-foundation/topic-dictionary/storage/projected-volumes.md`
- `domain-17-system-foundation/topic-dictionary/storage/storage-capacity.md`
- `domain-17-system-foundation/topic-dictionary/storage/storage-classes.md`
- `domain-17-system-foundation/topic-dictionary/storage/volume-attributes-classes.md`
- `domain-17-system-foundation/topic-dictionary/storage/volume-health-monitoring.md`
- `domain-17-system-foundation/topic-dictionary/storage/volume-snapshot-classes.md`
- `domain-17-system-foundation/topic-dictionary/storage/volume-snapshots.md`
- `domain-17-system-foundation/topic-dictionary/storage/volumes.md`
- `domain-17-system-foundation/topic-dictionary/storage/windows-storage.md`

## Related

- [[entities/configuration-terms.md|configuration-terms]] — K8s 配置管理术语参考
- [[entities/observability-terms.md|observability-terms]] — K8s 可观测性术语参考
- [[entities/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows
