# Pod Priority and Preemption

## 概述

Pod 优先级和抢占（Pod Priority and Preemption）是 Kubernetes v1.14 中达到 stable 的特性。Pod 可以具有优先级，表示该 Pod 相对于其他 Pod 的重要性。如果某个 Pod 无法被调度，调度器会尝试抢占（驱逐）优先级较低的 Pod，以使该 pending Pod 能够被调度。

## 核心概念/原理

### PriorityClass

PriorityClass 是一个非命名空间对象，定义了优先级类名称到整数值的映射。值越高，优先级越高。名称必须是有效的 DNS 子域名，且不能以 `system-` 为前缀。

- 取值范围：-2147483648 到 1000000000（32 位整数）。
- 大于 10 亿的值保留给内置的关键系统 Pod 使用。
- `globalDefault`：表示该 PriorityClass 的值应用于没有指定 `priorityClassName` 的 Pod。系统中只能有一个 `globalDefault` 为 true 的 PriorityClass。
- `preemptionPolicy`：
  - `PreemptLowerPriority`（默认）：允许该 PriorityClass 的 Pod 抢占低优先级 Pod。
  - `Never`：该 PriorityClass 的 Pod 不会抢占其他 Pod，但可能被更高优先级的 Pod 抢占。

### Pod 优先级

创建 Pod 时在 `priorityClassName` 字段指定 PriorityClass 名称。优先级准入控制器会解析并填充整数值。如果找不到对应的 PriorityClass，Pod 会被拒绝。

### 抢占（Preemption）

当调度器无法为 pending Pod 找到满足所有要求的节点时，会触发抢占逻辑。调度器会寻找这样的节点：移除一个或多个优先级低于 pending Pod 的 Pod 后，pending Pod 可以被调度到该节点上。如果找到这样的节点，低优先级 Pod 会被驱逐，然后 pending Pod 被调度到该节点。

## 关键机制或特性

- **nominatedNodeName**：当 Pod P 抢占了一个或多个 Pod 后，Pod P 状态中的 `nominatedNodeName` 字段会被设置为目标节点名称。这有助于调度器跟踪为 Pod P 预留的资源。但 Pod P 最终不一定会调度到 nominated node 上。
- **非抢占式 PriorityClass**（v1.24+ stable）：允许高优先级 Pod 在调度队列中排在低优先级 Pod 前面，但不主动抢占正在运行的 Pod，适用于希望优先但不中断现有工作的数据科学工作负载。
- **PodDisruptionBudget（PDB）支持**：调度器在抢占时尽量尊重 PDB，但这是 best effort。如果找不到不违反 PDB 的受害者，仍然会进行抢占。
- **与 QoS 的交互**：Pod 优先级和 QoS 类是两个正交的特性。调度器的抢占逻辑不考虑 QoS，但 kubelet 的节点压力驱逐会使用优先级来确定驱逐顺序。

## 使用场景

- 关键生产服务需要确保在资源紧张时优先获得调度机会。
- 数据科学批处理作业希望在有空闲资源时优先运行，但不想打断现有工作（使用 `preemptionPolicy: Never`）。
- 集群升级或节点维护后，需要快速重新调度高优先级工作负载。

## 最佳实践/注意事项

- 在不可信用户的多租户集群中，恶意用户可能创建最高优先级的 Pod 导致其他 Pod 被驱逐。管理员应使用 ResourceQuota 限制用户创建高优先级 Pod。
- 抢占受害者的优雅终止期会产生时间差，可以通过将低优先级 Pod 的优雅终止期设置为 0 或较小值来最小化这个间隙。
- 如果 pending Pod 对节点上的低优先级 Pod 有 Pod 间亲和性，调度器不会在该节点上抢占任何 Pod。
- 调度器不支持跨节点抢占（cross node preemption）。
- 现有 Pod 在添加 `globalDefault` PriorityClass 后优先级不会自动改变，只影响之后创建的 Pod。

## 参考链接

- [Kubernetes 官方文档 - Pod Priority and Preemption](https://kubernetes.io/docs/concepts/scheduling-eviction/pod-priority-preemption/)
