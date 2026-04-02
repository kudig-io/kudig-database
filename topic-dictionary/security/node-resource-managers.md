# Node Resource Managers（节点资源管理器）

## 概述

为了支持对延迟敏感（latency-critical）和高吞吐量（high-throughput）的工作负载，Kubernetes 提供了一套节点资源管理器（Node Resource Managers）。这些管理器旨在协调和优化节点上为 Pod 分配 CPU、设备（devices）和内存（大页，hugepages）资源时的对齐方式，以最大程度地提升工作负载性能。

## 核心概念/原理

- **硬件拓扑感知（Hardware Topology Alignment）**：现代服务器通常具有 NUMA（Non-Uniform Memory Access）、Socket、物理核心（Physical Core）等复杂拓扑结构。资源分配时的拓扑对齐对高性能工作负载至关重要。
- **Topology Manager**：作为 kubelet 的核心协调组件，Topology Manager 负责统筹各个资源管理器（CPU Manager、Memory Manager、Device Manager）的优化决策，并根据用户指定的策略进行整体资源管理。
- **资源管理器家族**：主要包括 CPU Manager、Memory Manager 和 Device Manager，各自负责特定资源的分配策略和 NUMA 亲和性优化。

## 关键机制或特性

### 1. CPU 管理策略（CPU Manager）

FEATURE STATE: `Kubernetes v1.26 [stable]`

默认情况下，kubelet 使用 CFS quota 强制执行 Pod 的 CPU limit。当节点运行大量 CPU 密集型 Pod 时，工作负载可能会在不同的 CPU 核心之间迁移。对于大部分工作负载，这没有问题；但对于对 CPU 缓存亲和性和调度延迟敏感的应用，kubelet 允许通过 CPU Manager 策略进行更细粒度的控制。

可用策略：

| 策略 | 说明 |
|------|------|
| `none` | 默认策略，显式启用现有的默认 CPU 亲和性方案。Guaranteed 和 Burstable Pod 的 CPU 限制通过 CFS quota 强制执行。 |
| `static` | 允许 **Guaranteed** Pod 中具有**整数 CPU request** 的容器访问节点上的**独占 CPU**。独占性通过 `cpuset` cgroup 控制器强制执行。 |

#### Static 策略的工作原理

- 初始共享池（shared pool）包含节点上的所有 CPU。`reservedSystemCPUs` 中预留的 CPU 会从共享池中按物理核心 ID 升序移除。
- **BestEffort** 和 **Burstable** Pod 的容器在共享池上运行。
- **Guaranteed** Pod 中具有**分数 CPU request** 的容器也在共享池上运行。
- 只有 **Guaranteed** Pod 中具有**整数 CPU request** 的容器才会被分配独占 CPU。
- 被静态分配的 CPU 从共享池中移除，并放入容器的 cpuset 中。这些容器**不使用 CFS quota**，其 CPU 使用量由 cpuset 的调度域本身限制。

#### Static 策略选项

| 选项 | 成熟度 | 说明 |
|------|--------|------|
| `full-pcpus-only` | GA | 始终分配完整的物理核心，避免 SMT 系统上不同容器共享同一物理核心导致的 noisy neighbors 问题。 |
| `distribute-cpus-across-numa` | Beta | 当需要多个 NUMA 节点时，均匀分布 CPU，避免并行同步代码因某个 NUMA 节点 CPU 较少而产生瓶颈。 |
| `align-by-socket` | Alpha | 按物理插槽（socket）边界对齐 CPU，而非 NUMA 边界。与 `TopologyManager` 的 `single-numa-node` 策略不兼容。 |
| `distribute-cpus-across-cores` | Alpha | 将虚拟核心（硬件线程）分配到不同的物理核心，减少同一物理核心上的竞争。 |
| `strict-cpu-reservation` | GA | 禁止任何工作负载（无论 QoS 类别）使用 `reservedSystemCPUs` 中指定的 CPU 核心。 |
| `prefer-align-cpus-by-uncorecache` | Beta | 以最佳 effort 方式按 uncore（Last-Level Cache, LLC）边界对齐 CPU，减少跨缓存延迟和缓存级 noisy neighbors。 |

功能门控：
- `CPUManagerPolicyBetaOptions`（默认启用）：控制 Beta 级别选项的可见性。
- `CPUManagerPolicyAlphaOptions`（默认禁用）：控制 Alpha 级别选项的可见性。

> **注意**：启用 static 策略时，kubelet 要求 CPU 预留大于零，以防止共享池为空。此外，系统服务（如容器运行时和 kubelet 本身）仍可运行在这些独占 CPU 上，独占性仅延伸到其他 Pod。CPU Manager 不支持运行时的 CPU 离线/在线。

### 2. 内存管理策略（Memory Manager）

FEATURE STATE: `Kubernetes v1.32 [stable]`

Memory Manager 为 **Guaranteed** QoS 类别的 Pod 分配 RAM（内存和可选的 Linux 大页）资源。

- 使用**提示生成协议（hint generation protocol）**为 Pod 生成最合适的 NUMA 亲和性提示。
- 将这些亲和性提示反馈给 Topology Manager，Topology Manager 结合自身策略决定是拒绝还是允许 Pod 调度到该节点。
- 确保 Pod 请求的内存从**最少数量的 NUMA 节点**分配，以减少跨 NUMA 访问延迟。

### 3. 其他资源管理器

- **Device Manager**：负责为 Pod 分配设备（如 GPU、FPGA 等），并参与 Topology Manager 的 NUMA 对齐决策。详细配置请参阅 Kubernetes 官方文档的 Device Manager 部分。

## 使用场景

- **独占 CPU 分配**：为 Guaranteed Pod 中的关键容器分配整数个独占 CPU，减少上下文切换和 CPU 节流，适用于计算密集型、对延迟敏感的应用（如实时数据处理、高频交易）。
- **NUMA 感知优化**：在具有多个 NUMA 节点的服务器上，通过 Memory Manager 和 Topology Manager 确保内存和 CPU 分配位于同一 NUMA 节点或最少数量的 NUMA 节点上，降低内存访问延迟。
- **避免 Noisy Neighbors**：使用 `full-pcpus-only` 选项确保容器独占完整的物理核心，避免 SMT 环境下不同容器共享物理核心导致的性能干扰。
- **并行同步负载优化**：对于依赖 barrier 等同步原语的并行代码，使用 `distribute-cpus-across-numa` 均匀分布 CPU，防止因某个 NUMA 节点 CPU 不足而成为性能瓶颈。
- **大页内存应用**：为需要 Linux hugepages 的数据库、科学计算等应用提供 NUMA 对齐的大页内存分配。

## 最佳实践/注意事项

- **QoS 类别要求**：只有 `Guaranteed` QoS 且 CPU request 为整数的 Pod 才能从 static CPU 策略中获得独占 CPU。务必确保 Pod 的 `requests` 等于 `limits`，且 CPU 值为整数。
- **CPU 预留配置**：启用 static 策略时，必须设置大于零的 CPU 预留（`reservedSystemCPUs` 或 `--kube-reserved`/`--system-reserved` 中的 CPU 预留），否则共享池可能为空。
- **策略兼容性**：`align-by-socket` 选项与 `TopologyManager` 的 `single-numa-node` 策略**不兼容**，在同时启用时需谨慎。
- **Alpha 功能风险**：`align-by-socket` 和 `distribute-cpus-across-cores` 等 Alpha 级别的策略选项需要通过 `CPUManagerPolicyAlphaOptions` 功能门控显式开启，生产环境使用前请充分评估。
- **系统服务干扰**：虽然 static 策略为 Pod 分配了独占 CPU，但 kubelet、容器运行时等系统服务仍可能在这些 CPU 上运行，因此"独占"是相对于其他 Pod 而言的。如需完全隔离，可结合 `strict-cpu-reservation` 选项使用。
- **实时监控与验证**：在启用这些高级策略后，建议通过 `cat /sys/fs/cgroup/cpuset/.../cpuset.cpus` 等命令验证容器的实际 CPU 分配是否符合预期。

## 参考链接

- [Kubernetes 官方文档 - Node Resource Managers](https://kubernetes.io/docs/concepts/policy/node-resource-managers/)
