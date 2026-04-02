# User Namespaces

## 概述
用户命名空间（User Namespaces）是 Linux 的一项特性，用于将容器内的用户与主机（节点）上的用户隔离。容器内以 root 运行的进程，在主机上可映射为非 root 用户，从而显著降低容器逃逸后对主机或其他 Pod 的危害。

## 核心概念/原理
- **启用方式**：Pod 通过设置 `pod.spec.hostUsers: false` 来启用用户命名空间（默认 `true`，即与主机共享用户命名空间）。
- **UID/GID 映射**：kubelet 会为每个 Pod 分配唯一的主机 UID/GID 映射范围，确保同一节点上不同 Pod 的映射不重叠。
- **Capabilities 隔离**：授予 Pod 的 capabilities 仅在 Pod 的用户命名空间内有效，在宿主机上基本无效。例如：
  - `CAP_SYS_MODULE` 无法加载内核模块。
  - `CAP_SYS_ADMIN` 仅限 Pod 内部使用。
- **卷挂载行为**：`runAsUser`、`runAsGroup`、`fsGroup` 等字段始终指容器内部用户；Pod 挂载卷时看到的文件所有权与未启用用户命名空间时一致。
- **默认 UID/GID 范围**：启用后，有效范围为 0–65535；超出此范围的文件将显示为 overflow ID（通常为 65534）。

## 关键机制或特性
- **节点要求**：
  - Linux 6.3+（tmpfs 支持 idmap 挂载）。
  - 文件系统（如 ext4、xfs、btrfs、overlayfs、tmpfs）支持 idmap 挂载。
  - OCI 运行时：crun ≥1.9（推荐 ≥1.13）或 runc ≥1.2。
  - CRI 运行时：containerd ≥2.0 或 CRI-O ≥1.25。
- **kubelet 子 ID 配置**：
  - 系统需存在 `kubelet` 用户。
  - 需安装 `getsubids`（shadow-utils）。
  - `/etc/subuid` 和 `/etc/subgid` 中需为 `kubelet` 用户配置子ordinate ID 范围。
  - 起始 ID 必须是 65536 的倍数且 ≥65536；数量至少为 `65536 × maxPods`。
- **每 Pod ID 数量**：自 v1.33 起，可通过 `KubeletConfiguration` 的 `userNamespaces.idsPerPod` 配置（默认 65536，必须是 65536 的倍数）。
- **Pod 安全准入放宽（Alpha）**：启用用户命名空间的 Linux Pod，Pod Security Standards 对 `runAsNonRoot`、`runAsUser`、`procMount` 等字段的约束会适当放宽。
- **限制**：启用用户命名空间时，不允许同时使用 `hostNetwork`、`hostIPC`、`hostPID`，也不允许使用 `volumeDevices`（原始块设备）。

## 使用场景
- 运行需要 root 权限但希望降低宿主机风险的应用。
- 对安全要求较高的多租户环境。
- 使用 distroless 或最小化镜像时，进一步增强隔离。

## 最佳实践/注意事项
- 确保节点内核、文件系统、运行时均支持 idmap 挂载后再启用此特性。
- 配置好 `/etc/subuid` 和 `/etc/subgid`，避免 ID 范围重叠。
- 需要访问主机命名空间或块设备的 Pod 无法启用用户命名空间。
- 可通过 kubelet 指标 `started_user_namespaced_pods_total` 和 `started_user_namespaced_pods_errors_total` 监控使用情况。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/user-namespaces/
