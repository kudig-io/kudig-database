# Pod Quality of Service Classes

## 概述
Kubernetes 根据 Pod 内容器的资源请求（requests）和限制（limits）为每个 Pod 分配一个服务质量（QoS）等级。该等级用于在节点资源不足时决定驱逐优先级。

## 核心概念/原理
可能的 QoS 等级有三种，按驱逐优先级从高到低排列：
1. **BestEffort**：没有任何容器设置 CPU 或内存的 request/limit，也没有 Pod 级资源设置。节点资源紧张时最优先被驱逐。
2. **Burstable**：不满足 Guaranteed 条件，但至少有一个容器或 Pod 设置了 CPU 或内存的 request/limit。
3. **Guaranteed**：最严格的资源约束，最不容易被驱逐；只有该等级可使用 `static` CPU 管理策略申请独占 CPU。

**Guaranteed 的判定条件**：
- 每个容器（或 Pod 级资源）必须同时设置内存 request 和 limit，且两者相等。
- 每个容器（或 Pod 级资源）必须同时设置 CPU request 和 limit，且两者相等。

## 关键机制或特性
- **节点压力驱逐**：当节点资源不足时，kubelet 优先驱逐 `BestEffort`，其次是 `Burstable`，最后是 `Guaranteed`。仅超出自身 request 的 Pod 才会被驱逐。
- **资源超限处理**：任何容器超出其资源 limit 都会被 kubelet 终止并重启（如 OOM Kill 或 CPU 限流），不影响同一 Pod 内的其他容器。
- **QoS 不变性**：Pod 创建后 QoS 等级终身不变。若进行原地 resize 导致 QoS 变更，则 resize 会被拒绝。
- **Memory QoS（cgroup v2，Alpha）**：利用 `memory.min` 和 `memory.high` 保证内存可用性，与 QoS 等级协同工作但机制不同。

## 使用场景
- 对延迟敏感的负载应设置为 `Guaranteed`，以获得最强的资源保障和最低的驱逐风险。
- 可容忍一定资源波动的批处理或开发测试负载可设置为 `Burstable`。
- 非关键后台任务可使用 `BestEffort`，充分利用节点空闲资源。

## 最佳实践/注意事项
- 生产环境中关键应用建议配置 `Guaranteed` QoS。
- 设置 `Guaranteed` 时务必确保所有容器的 CPU 和内存 request/limit 完全相等。
- 调度器在进行抢占（preemption）时**不考虑** QoS 等级，抢占决策基于优先级和资源需求。
- 使用 Pod 级资源（Beta）简化 Guaranteed 配置时，需确保 Pod 级 request 和 limit 相等。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/pod-qos/
