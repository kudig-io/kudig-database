# Advanced Pod Configuration

## 概述
本页涵盖 Pod 的高级配置主题，包括 PriorityClass、RuntimeClass、安全上下文（security context）以及影响 Pod 调度的相关机制。

## 核心概念/原理
- **PriorityClass**：集群范围的 API 对象，将优先级名称映射为整数值。数值越高优先级越高。当资源不足时，kube-scheduler 可抢占（驱逐）低优先级 Pod 以调度高优先级 Pod。
  - 内置类：`system-cluster-critical`（集群关键系统组件）、`system-node-critical`（节点关键组件，最高优先级）。
- **RuntimeClass**：允许为 Pod 指定低级别容器运行时，适用于需要不同隔离级别或运行时特性的场景（如 Kata Containers、gVisor）。
- **安全上下文（Security Context）**：
  - **Pod 级**：`pod.spec.securityContext` 应用于整个 Pod，可设置 `runAsUser`、`runAsGroup`、`fsGroup`、SELinux、seccomp 等。
  - **容器级**：`container.securityContext` 可对单个容器进行更细粒度的控制，如 `capabilities` 增删、`allowPrivilegeEscalation`、`runAsNonRoot`、AppArmor 等。
- **调度影响机制**：
  - `nodeSelector`：最简单的节点选择约束。
  - `nodeAffinity`：基于节点标签的复杂约束（优先/强制）。
  - `podAffinity` / `podAntiAffinity`：基于其他 Pod 标签的 placement 约束。
  - `tolerations`：允许 Pod 调度到带有匹配 taint 的节点上。
- **Pod Overhead**：记录 Pod 基础设施本身消耗的资源（超出容器请求/限制的部分），由 RuntimeClass 定义。

## 关键机制或特性
- **特权模式（Privileged Mode）**：`securityContext` 中可启用特权模式，但会覆盖许多其他安全设置，应尽量避免，优先使用细粒度权限配置。
- **Windows HostProcess**：通过 `windowsOptions.hostProcess` 在 Windows 上运行特权容器。

## 使用场景
- 需要保证关键业务优先调度时使用 PriorityClass。
- 对安全隔离有更高要求时，使用 RuntimeClass 切换至沙箱运行时。
- 需要 Pod 运行在特定硬件（GPU、SSD）或特定拓扑区域时，使用 Affinity 和 Taints/Tolerations。
- 多租户环境中通过 Security Context 加固容器安全。

## 最佳实践/注意事项
- 尽量避免使用特权容器；使用 capabilities、seccomp、AppArmor 等细粒度控制。
- 为系统组件预留 `system-cluster-critical` 或 `system-node-critical` 优先级。
- 使用 `podAntiAffinity` 将同一应用的副本分散到不同节点/可用区，提高容错性。
- 配置 RuntimeClass 前，需确保集群管理员已在相应节点上安装并配置好底层运行时。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/advanced-pod-config/
