# Scheduling Framework

## 概述

调度框架（Scheduling Framework）是 Kubernetes 调度器的可插拔架构。它由一组直接编译到调度器中的"插件"API 组成。这些 API 允许将大多数调度功能实现为插件，同时保持调度核心轻量且可维护。该功能在 Kubernetes v1.19 中达到 stable 状态。

## 核心概念/原理

每次调度一个 Pod 的尝试分为两个阶段：**调度周期（scheduling cycle）** 和 **绑定周期（binding cycle）**。

- **调度周期**：为 Pod 选择一个节点。
- **绑定周期**：将该决策应用到集群中。

调度周期和绑定周期合称为"调度上下文"。调度周期串行运行，而绑定周期可以并发运行。如果 Pod 被确定为不可调度或发生内部错误，调度或绑定周期可以被中止，Pod 会返回到队列中重试。

## 关键机制或特性

### 扩展点（Extension Points）

1. **PreEnqueue**：在将 Pod 添加到内部活动队列之前调用。只有所有 PreEnqueue 插件返回 `Success`，Pod 才能进入活动队列。
2. **EnqueueExtension**：插件可以控制是否根据集群变化重试被该插件拒绝的 Pod 的调度。
3. **QueueingHint**（v1.34+ stable）：决定 Pod 是否可以重新排队到活动队列或退避队列的回调函数。
4. **QueueSort**：用于对调度队列中的 Pod 进行排序。一次只能启用一个队列排序插件。
5. **PreFilter**：用于预处理 Pod 信息或检查集群/Pod 必须满足的某些条件。
6. **Filter**：用于过滤掉无法运行 Pod 的节点。如果某个 Filter 插件将节点标记为不可行，则不会为该节点调用剩余的 Filter 插件。
7. **PostFilter**：仅在未找到可行节点时调用。典型的实现是抢占（preemption），尝试通过驱逐其他 Pod 使当前 Pod 可调度。
8. **PreScore**：执行"预评分"工作，生成 Score 插件可共享的状态。
9. **Score**：对通过过滤阶段的节点进行排名。调度器会调用每个评分插件为每个节点打分。
10. **NormalizeScore**：在计算最终节点排名之前修改分数。
11. **Reserve**：在调度器实际绑定 Pod 之前执行，防止竞争条件。包含 `Reserve` 和 `Unreserve` 两个方法。
12. **Permit**：在调度周期结束时调用，可以阻止或延迟绑定到候选节点。支持 approve、deny 或 wait（带超时）。
13. **PreBind**：在 Pod 绑定之前执行所需的工作（如准备网络卷）。
14. **Bind**：将 Pod 绑定到节点。如果某个 Bind 插件选择处理该 Pod，则跳过剩余的 Bind 插件。
15. **PostBind**：在 Pod 成功绑定后调用，用于清理相关资源。

### 容量评分（Capacity Scoring）

v1.33+ alpha（`StorageCapacityScoring` 特性门控）：扩展 VolumeBinding 插件，根据节点上的存储容量对节点进行评分，适用于支持 Storage Capacity 的 CSI 卷。

## 使用场景

- 需要实现自定义调度逻辑时，可以通过调度框架编写插件。
- 企业需要根据不同的工作负载类型配置不同的调度策略时，可以配置多个调度配置文件（scheduler profiles）。
- 需要在绑定前执行额外准备工作（如动态卷准备）时，可以实现 PreBind 插件。

## 最佳实践/注意事项

- `Unreserve` 方法的实现必须是幂等的，且不能失败。
- 调度周期串行执行，绑定周期可并发执行。
- 大多数调度插件在 Kubernetes v1.18+ 中默认启用。
- 可以配置多个调度配置文件以适应不同类型的负载。

## 参考链接

- [Kubernetes 官方文档 - Scheduling Framework](https://kubernetes.io/docs/concepts/scheduling-eviction/scheduling-framework/)
