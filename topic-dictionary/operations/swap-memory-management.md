# Swap 内存管理

## 概述

Kubernetes 可以配置为在节点上使用 swap（交换）内存，允许内核将不活跃的内存页换出到后备存储，从而释放物理内存。这对具有大内存占用但只在特定时间访问部分内存的工作负载非常有用，也有助于防止 Pod 在内存压力峰值期间被终止，并提高节点内存管理的灵活性。

## 核心概念/原理

- **Linux 节点**：支持 swap，但需要手动在每个节点上启用。默认情况下，如果 Linux 节点启用了 swap，kubelet 将**不会**启动。
- **Windows 节点**：默认**需要** swap 空间。如果 Windows 节点禁用了 swap，kubelet 将**不会**启动。
- **Kubelet 启动行为**：当 kubelet 配置为 `failSwapOn: false` 时，可以在启用了 swap 的节点上启动，并默认通过 CRI 指示容器运行时为 Kubernetes 工作负载分配零 swap。
- **Swap 行为配置**：通过 `KubeletConfiguration` 中的 `memorySwap.swapBehavior` 设置节点在存在 swap 时的行为。

## 关键机制或特性

### Swap 行为

| 行为 | 说明 |
|------|------|
| `NoSwap`（默认） | 节点上的 Pod 不能使用 swap。 |
| `LimitedSwap` | Kubernetes 工作负载可以利用 swap 内存。 |

注意：选择 `NoSwap` 且 `failSwapOn: false` 时，Kubernetes 管理的容器不使用 swap，但系统服务（包括 kubelet 本身）仍可使用 swap。

### Container Runtime 集成

kubelet 通过 CRI 指示容器运行时在 cgroup 层面（如 cgroup v2 的 `memory.swap.max`）配置 swap，容器运行时负责将这些设置写入容器级 cgroup。

### 可观测性

- **kubelet 指标端点**：`/metrics/resource` 和 `/stats/summary` 提供 swap 使用指标，如 `node_swap_usage_bytes`、`container_swap_usage_bytes`、`container_swap_limit_bytes`。
- **kubectl top --show-swap**：`kubectl top nodes --show-swap` 和 `kubectl top pods --show-swap` 可直观查看 swap 使用情况。
- **节点状态字段**：`node.status.nodeInfo.swap.capacity` 报告节点的 swap 容量。
- **Node Feature Discovery (NFD)**：可用于发现哪些节点配置了 swap。

### LimitedSwap 的 swap 限制计算

对于 `LimitedSwap`，仅允许 **Burstable QoS** 的 Pod 使用 swap。**BestEffort** 和 **Guaranteed** QoS 的 Pod 禁止使用 swap。单个容器的 swap 上限计算公式为：

```
(containerMemoryRequest / nodeTotalMemory) × totalPodsSwapAvailable
```

即 swap 使用量与容器的内存请求、节点总物理内存和可用于 Pod 的总 swap 内存成比例。

## 使用场景

- **内存使用波动大的工作负载**：大内存应用但访问模式稀疏，swap 可提供额外缓冲。
- **避免内存压力峰值导致 Pod 被驱逐**：swap 可吸收短期内存尖峰，降低 OOM 风险。
- **开发/测试环境**：需要更灵活的内存配置，而不追求极致性能可预测性。

## 最佳实践/注意事项

- **强烈建议加密 swap 空间**，以降低数据泄露风险。
- **控制平面节点不建议启用 swap**，因为控制平面主要运行 Guaranteed QoS Pod，swap 可能影响关键服务性能。
- **为 swap 使用专用高速磁盘**（如 SSD/NVMe），避免与系统/容器运行时共享磁盘导致 I/O 竞争。
- **禁用系统关键守护进程的 swap**：如遇到性能下降，可将系统 slice 的 cgroup `memory.swap.max` 设为 0。
- **优先保障系统关键守护进程的 I/O 延迟**：通过配置 `io.latency` 提高系统 slice 的 I/O 优先级。
- **调度器目前不考虑 swap**：Kubernetes 1.35 的调度器在调度决策时不考虑 swap 资源。管理员可以通过为启用 swap 的节点添加污点（taints），确保只有明确需要 swap 的负载被调度到这些节点。
- **内存支撑卷（memory-backed volumes）**：如 `emptyDir`（`medium: Memory`）和 Secret 卷挂载使用 `tmpfs`，kubelet 会尝试使用 `noswap` 挂载选项确保内容始终保留在内存中（Linux 内核 6.3+ 原生支持）。
- **驱逐阈值调整**：建议将 kubelet 的内存驱逐阈值设置为略低于 `vm.min_free_kbytes`，以便在 kubelet 驱逐 Pod 之前，内核可以先尝试 swap。
- **性能可预测性降低**：启用 swap 可能导致“吵闹邻居”问题和意外性能回退，尤其在 IOPS 受限环境（如云 VM）中。

## 参考链接

- [Swap Memory Management - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/swap-memory-management/)
