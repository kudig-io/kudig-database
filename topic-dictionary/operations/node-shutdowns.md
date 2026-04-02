# 节点关闭（Node Shutdowns）

## 概述

在 Kubernetes 集群中，节点可能会因为计划内维护或意外原因（如断电）而关闭。如果节点在关闭前未被清空（drain），可能导致工作负载失败。节点关闭分为**优雅关闭（graceful）**和**非优雅关闭（non-graceful）**两种类型。Kubernetes 提供了相应的机制来尽量降低节点关闭对工作负载的影响。

## 核心概念/原理

- **优雅节点关闭**：kubelet 尝试检测系统关闭信号，按照正常的 Pod 终止流程停止节点上的 Pod，并在关闭期间拒绝接收新 Pod。
- **非优雅节点关闭**：kubelet 的关闭管理器未检测到关闭事件，Pod 可能长时间停留在 Terminating 状态，StatefulSet 无法在新节点重建同名 Pod，卷也无法重新挂载。
- **systemd 抑制锁（inhibitor locks）**：Linux 上的优雅关闭依赖 systemd 的抑制锁来延迟关机，为 Pod 终止争取时间。
- **Windows 服务控制处理程序**：Windows 上的优雅关闭依赖 kubelet 以 Windows 服务运行，通过注册服务控制处理程序来延迟预关闭事件。

## 关键机制或特性

### 优雅节点关闭配置

通过 `KubeletConfiguration` 中的以下选项配置：

- `shutdownGracePeriod`：节点延迟关闭的总时长（普通 Pod + 关键 Pod 的优雅终止总时间）。
- `shutdownGracePeriodCriticalPods`：用于终止关键 Pod 的时长，必须小于 `shutdownGracePeriod`。

例如，`shutdownGracePeriod=30s`、`shutdownGracePeriodCriticalPods=10s`，则前 20 秒用于普通 Pod，后 10 秒用于关键 Pod。

### 基于 Pod 优先级的优雅关闭

FEATURE STATE: `Kubernetes v1.24 [beta]`（默认启用）

通过 `shutdownGracePeriodByPodPriority` 配置，可以按 Pod 的 PriorityClass 值分阶段关闭，实现更细粒度的关闭控制。需要启用 `GracefulNodeShutdownBasedOnPodPriority` 特性门控。

### 非优雅节点关闭处理

FEATURE STATE: `Kubernetes v1.28 [stable]`（默认启用）

当节点发生非优雅关闭时，可手动为节点添加污点 `node.kubernetes.io/out-of-service`（效果为 `NoExecute` 或 `NoSchedule`），系统会强制删除无对应容忍的 Pod，并立即执行卷分离操作，使 Pod 能在其他节点快速恢复。

### 强制存储分离超时

如果 Pod 删除在 6 分钟内未成功，且节点不健康，Kubernetes 将强制分离卷。此行为可选，可通过 `kube-controller-manager` 的 `disable-force-detach-on-timeout` 配置禁用。

## 使用场景

- **计划内节点维护**：在关机前启用优雅关闭，确保工作负载有序终止。
- **意外断电或硬件故障**：通过非优雅关闭处理机制（out-of-service 污点）快速恢复 StatefulSet 和带状态应用。
- **关键业务保护**：通过基于优先级的关闭策略，优先保证高优先级业务的终止时间。

## 最佳实践/注意事项

- 在计划内维护前，优先使用 `kubectl drain` 清空节点，减少工作负载中断。
- 配置合理的 `shutdownGracePeriod` 和 `shutdownGracePeriodCriticalPods`，确保关键 Pod 有足够时间优雅终止。
- 添加 `node.kubernetes.io/out-of-service` 污点前，务必确认节点确实已关机或断电（而非正在重启）。
- Pod 开始终止后，即使节点关闭被取消，已终止的 Pod 也不会被 kubelet 恢复，需要重新调度。
- 使用非优雅节点关闭流程时需谨慎，操作不当可能导致数据损坏。
- 控制平面节点不建议配置 swap，且应确保关键系统守护进程不受 swap 影响。

## 参考链接

- [Node Shutdowns - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/node-shutdown/)
