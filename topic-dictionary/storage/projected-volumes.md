# Projected Volumes（投射卷）

## 概述

Projected Volume 是一种将多个现有的卷源（如 Secret、ConfigMap、downwardAPI、serviceAccountToken 等）映射到同一个目录中的卷类型。它提供了一种“一体化”的方式，将不同来源的数据集中投射到容器的文件系统中。

## 核心概念/原理

- **统一目录**：Projected Volume 将多个独立的卷源合并挂载到容器内的同一个路径下。
- **源类型**：所有源必须与 Pod 位于同一命名空间。
- **只读属性**：Projected Volume 中的内容默认是只读的。

## 关键机制或特性

### 支持的卷源类型

| 类型 | 说明 |
|------|------|
| `secret` | 将 Secret 的键值对作为文件投射到目录中。 |
| `configMap` | 将 ConfigMap 的键值对作为文件投射到目录中。 |
| `downwardAPI` | 将 Pod 的元数据或资源信息以文件形式投射。 |
| `serviceAccountToken` | 将当前 ServiceAccount 的 Token 注入到指定路径，用于访问 Kubernetes API。 |
| `clusterTrustBundle` | 将 ClusterTrustBundle 对象的内容作为自动更新的 PEM 文件注入（v1.33 beta）。 |
| `podCertificate` | 为 Pod 安全地提供私钥和 X.509 证书链，并自动轮换（v1.35 beta）。 |

### 权限与模式

- 可在 `projected` 级别设置 `defaultMode`。
- 也可为每个单独的投影项设置 `mode`，实现对特定文件的权限控制。

### ServiceAccountToken 投射

- 可配置 `audience`（受众）、`expirationSeconds`（过期时间，最小 600 秒）和 `path`（相对挂载路径）。
- 当 Pod 设置了统一的 `runAsUser` 时，kubelet 会将 token 文件权限设为 `0600`，确保只有指定用户可读取。

### 安全上下文交互

- **Linux**：如果 Pod 设置了 `RunAsUser`，Projected 文件的所有权会设置为对应的容器用户。
- **Windows**：由于 SAM 数据库隔离，文件所有权无法强制设置为容器用户，默认由 `BUILTIN\Administrators` 等管理。

## 使用场景

- **统一凭证与配置目录**：将 API Token、CA 证书和应用配置集中投射到一个目录，方便应用统一读取。
- **安全注入 ServiceAccount Token**：避免将 Token 直接嵌入镜像，通过投射卷动态注入并自动管理过期时间。
- **Pod 身份认证**：为工作负载提供访问 Kubernetes API 或其他服务所需的证书和信任链。

## 最佳实践/注意事项

- 使用 `subPath` 挂载 Projected Volume 的容器不会自动接收到卷源内容的更新。
- 在 Windows Pod 中，不建议使用 Linux 的 `RunAsUser` 选项，否则可能导致 Pod 卡在 `ContainerCreating` 状态。
- 对于 `podCertificate` 投影，推荐优先使用 `credentialBundlePath`（合并的 PEM 文件），而不是分离的 `keyPath` 和 `certificateChainPath`，以避免证书轮换时出现密钥与证书不匹配的问题。
- `clusterTrustBundle` 和 `podCertificate` 功能需要开启对应的特性门和 runtime-config。

## 参考链接

- https://kubernetes.io/docs/concepts/storage/projected-volumes/
