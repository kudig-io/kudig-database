# Volumes（卷）

## 概述

Kubernetes Volumes 为 Pod 中的容器提供了一种通过文件系统访问和共享数据的机制。容器内的磁盘文件默认是临时的，容器崩溃或停止后数据会丢失。Volume 解决了数据持久化和容器间共享存储的问题。

## 核心概念/原理

- **Volume 本质**：一个目录，可能包含数据，可被 Pod 中的容器访问。具体目录如何产生、由什么介质支持、包含什么内容，取决于使用的 volume 类型。
- **生命周期**：
  - **Ephemeral volume**（临时卷）：生命周期与 Pod 绑定，Pod 被删除时卷也被销毁（如 `emptyDir`）。
  - **Persistent volume**（持久卷）：生命周期独立于 Pod，Pod 删除后卷仍然存在。
- **使用方式**：在 Pod 的 `.spec.volumes` 中定义卷，在 `.spec.containers[*].volumeMounts` 中声明挂载路径。

## 关键机制或特性

### 常见卷类型

| 类型 | 说明 |
|------|------|
| `emptyDir` | Pod 分配到节点时创建的空目录，可用于容器间共享临时数据；支持设置 `sizeLimit`，也可设置为 `medium: Memory`（tmpfs）。 |
| `hostPath` | 挂载宿主机上的文件或目录到 Pod 中，存在安全风险，建议尽量使用 `local` PV 替代。 |
| `configMap` / `secret` | 将 ConfigMap 或 Secret 中的数据作为文件挂载到 Pod 中，默认只读。 |
| `downwardAPI` | 将 Pod 的元数据（如 labels、annotations）以文件形式暴露给容器。 |
| `persistentVolumeClaim` | 通过 PVC 挂载持久卷，实现数据的持久化存储。 |
| `nfs` / `iscsi` / `fc` | 挂载现有的网络存储或块存储设备。 |
| `csi` | 通过容器存储接口（CSI）挂载第三方存储系统提供的卷，是当前推荐的扩展方式。 |
| `projected` | 将多个现有卷源映射到同一个目录中。 |
| `image`（Beta） | 将 OCI 镜像或制品作为只读卷挂载到容器中。 |

### 子路径（subPath / subPathExpr）

- `subPath`：指定卷内的子路径进行挂载，使同一个卷可在同一 Pod 中被多个容器以不同子目录挂载。
- `subPathExpr`：支持使用 downward API 环境变量动态构建子路径名。

### 挂载传播（Mount Propagation）

- `None`（默认）：容器内外的挂载互不可见。
- `HostToContainer`：宿主机后续挂载对该容器可见。
- `Bidirectional`：容器内挂载也会传播回宿主机及其他使用相同卷的 Pod（仅允许特权容器使用）。

### 递归只读挂载（Recursive Read-Only Mounts）

- Kubernetes v1.33 [stable]：设置 `recursiveReadOnly: Enabled` 可使挂载点及其所有子挂载都变为递归只读。

## 使用场景

- **数据持久化**：数据库、文件服务等需要保存数据，防止容器重启后数据丢失。
- **配置注入**：通过 `configMap` 或 `secret` 卷将配置和敏感信息挂载到容器中。
- **容器间共享数据**：同一 Pod 内的多个容器通过 `emptyDir` 共享临时文件或缓存。
- **访问外部存储**：通过 `nfs`、`iscsi` 或 CSI 驱动连接企业级存储系统。

## 最佳实践/注意事项

- 尽量避免使用 `hostPath` 卷，以防止安全风险和节点差异导致的问题；如需本地存储，优先使用 `local` PersistentVolume。
- `emptyDir` 的默认存储介质取决于节点的 kubelet 根目录所在磁盘，可通过 `medium: Memory` 使用内存加速访问。
- 使用 `subPath` 挂载的容器不会自动接收到 ConfigMap/Secret 的更新。
- 尽量使用 CSI 驱动替代已弃用的 in-tree 存储插件。

## 参考链接

- https://kubernetes.io/docs/concepts/storage/volumes/
