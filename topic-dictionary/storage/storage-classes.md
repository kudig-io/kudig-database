# Storage Classes（存储类）

## 概述

StorageClass 是 Kubernetes 中用于描述管理员所提供的存储“类别”的 API 资源。不同的 StorageClass 可以映射到不同的服务质量（QoS）级别、备份策略或任意由集群管理员定义的策略。它使得用户无需了解底层存储的实现细节，即可按需请求不同特性的持久存储。

## 核心概念/原理

- **存储配置文件**：StorageClass 类似于存储系统的“配置文件”，定义了如何动态供给（provision）PersistentVolume。
- **Provisioner（供给器）**：每个 StorageClass 必须指定一个 provisioner，决定使用哪个卷插件来创建 PV。
- **参数化配置**：通过 `parameters` 向 provisioner 传递存储系统特定的参数。

## 关键机制或特性

### 主要字段

| 字段 | 说明 |
|------|------|
| `provisioner` | 指定用于动态供给的插件（如 `ebs.csi.aws.com`、`csi.vsphere.vmware.com`）。 |
| `parameters` | 传递给 provisioner 的键值对参数，最多 512 个，总长度不超过 256 KiB。 |
| `reclaimPolicy` | 动态创建的 PV 的回收策略，`Delete`（默认）或 `Retain`。 |
| `allowVolumeExpansion` | 是否允许通过修改 PVC 扩展卷大小。 |
| `mountOptions` | 挂载 PV 时使用的额外挂载选项。 |
| `volumeBindingMode` | 卷绑定模式：`Immediate`（立即绑定）或 `WaitForFirstConsumer`（等待首个消费者）。 |
| `allowedTopologies` | 限制卷的拓扑范围（如可用区）。 |

### 默认 StorageClass

- 通过注解 `storageclass.kubernetes.io/is-default-class: "true"` 标记默认类。
- 当 PVC 未指定 `storageClassName` 时，自动使用默认 StorageClass。
- 允许多个默认类共存（用于平滑迁移），创建 PVC 时会选择最新创建的默认类。
- ** retroactive default assignment（v1.28 stable）**：当默认 StorageClass 可用后，控制平面会自动为之前未设置 `storageClassName` 的现有 PVC 补全默认值（`storageClassName: ""` 的 PVC 不会被更新）。

### Volume Binding Mode

- **Immediate**：PVC 创建后立即进行绑定和动态供给，可能导致 Pod 调度到无法访问该卷的节点上。
- **WaitForFirstConsumer**：延迟到使用 PVC 的 Pod 创建后再进行绑定和供给，调度器可结合 Pod 的拓扑约束（node selector、affinity、tolerations 等）选择合适的节点。

### 常见 Provisioner 示例

- **AWS EBS CSI**：`ebs.csi.aws.com`
- **AWS EFS CSI**：`efs.csi.aws.com`
- **NFS**：需使用外部 provisioner（如 `example.com/external-nfs`）
- **vSphere CSI**：`csi.vsphere.vmware.com`
- **Local**：`kubernetes.io/no-provisioner`（不支持动态供给，但可延迟绑定）

## 使用场景

- **多层级存储服务**：提供高性能 SSD（fast）和标准磁盘（slow）等不同存储等级。
- **多租户隔离**：通过不同的 StorageClass 为不同团队或应用分配独立的存储后端。
- **拓扑感知调度**：使用 `WaitForFirstConsumer` 确保存储在 Pod 调度的可用区内创建。
- **自动扩展存储**：开启 `allowVolumeExpansion` 支持业务无中断地扩展卷容量。

## 最佳实践/注意事项

- 尽量只设置一个默认 StorageClass，避免用户因未指定类而获得不可预期的存储。
- 对于本地存储或拓扑受限的后端，务必将 `volumeBindingMode` 设置为 `WaitForFirstConsumer`。
- 使用 `WaitForFirstConsumer` 时，不要在 Pod 规格中使用 `nodeName` 直接指定节点，否则调度器会被绕过，PVC 可能一直停留在 Pending 状态。
- In-tree 存储插件（如 `kubernetes.io/gce-pd`、`kubernetes.io/vsphere-volume`）大多已弃用或移除，建议迁移到对应的 CSI 驱动。

## 参考链接

- https://kubernetes.io/docs/concepts/storage/storage-classes/
