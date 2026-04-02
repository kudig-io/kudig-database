# Ephemeral Volumes（临时卷）

## 概述

Ephemeral Volumes（临时卷）是生命周期与 Pod 绑定的存储卷，随 Pod 的创建而创建，随 Pod 的删除而删除。它们适用于不需要数据在 Pod 重启后仍然持久保存的场景，如缓存、临时工作区或只读输入数据。

## 核心概念/原理

- **生命周期绑定 Pod**：Pod 停止或重启后，临时卷中的数据会丢失（容器崩溃不会导致数据丢失，因为 Pod 仍在节点上）。
- **内联定义**：临时卷直接在 Pod 规格中内联声明，简化了应用部署和管理。
- **两类临时卷**：
  1. **本地临时卷**：由 kubelet 管理，如 `emptyDir`、`configMap`、`secret`、`downwardAPI`。
  2. **第三方临时卷**：由 CSI 驱动提供，包括 CSI 临时卷（inline CSI）和通用临时卷（generic ephemeral）。

## 关键机制或特性

### 本地临时卷

- 由节点本地资源支持（磁盘或内存）。
- 不支持跨节点共享，且不受存储容量感知调度的约束。

### CSI 临时卷（CSI Ephemeral Volumes）

- 在 Pod 规格中直接内联声明 `csi` 卷。
- 由节点本地的 CSI 驱动在 Pod 调度到节点后创建。
- 不支持存储容量感知调度，也不受 Pod 存储资源使用限制约束。
- 示例：
  ```yaml
  volumes:
  - name: my-csi-inline-vol
    csi:
      driver: inline.storage.kubernetes.io
      volumeAttributes:
        foo: bar
  ```

### 通用临时卷（Generic Ephemeral Volumes）

- 在 Pod 规格中使用 `ephemeral.volumeClaimTemplate` 定义 PVC 模板。
- 控制器自动创建与 Pod 同名（`pod-name-volume-name`）的 PVC，Pod 删除后通过垃圾回收自动删除 PVC。
- 可由任何支持动态供给的存储驱动提供，包括 CSI 驱动。
- 支持 `WaitForFirstConsumer` 绑定模式，调度器可自由选择适合的节点。
- 命名确定性：`{pod-name}-{volume-name}`，方便查找和交互。

### 安全与配额

- 允许通过 Pod 创建间接创建 PVC，即使用户没有直接创建 PVC 的权限。
- 正常 PVC 的命名空间配额仍然适用，防止绕过其他策略。
- 集群管理员可通过准入 Webhook 限制通用临时卷的使用。

## 使用场景

- **缓存与临时数据**：缓存服务将不常用的数据从内存移到较慢的存储中。
- **构建与批处理任务**：为编译、数据处理等任务提供临时工作区。
- **只读输入数据**：通过 `configMap`、`secret` 或 `image` 卷为应用注入配置或密钥。
- **需要特殊存储特性的临时空间**：通过 CSI 临时卷使用高性能或具备加密能力的存储。

## 最佳实践/注意事项

- 对于需要调度器考虑节点约束的场景，优先使用**通用临时卷**并配置 `WaitForFirstConsumer`。
- 使用 CSI 临时卷时，不要在 `volumeAttributes` 中暴露通常由管理员控制的敏感参数（如 StorageClass 级别的配置）。
- 集群管理员可以通过从 CSIDriver 的 `volumeLifecycleModes` 中移除 `Ephemeral` 来禁止特定 CSI 驱动作为内联临时卷使用。
- 通用临时卷的 PVC 名称由 Pod 名和卷名组合而成，注意避免与其他 Pod 或手动创建的 PVC 发生命名冲突。

## 参考链接

- https://kubernetes.io/docs/concepts/storage/ephemeral-volumes/
