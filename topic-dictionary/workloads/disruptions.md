# Disruptions

## 概述
本页介绍影响 Pod 可用性的中断类型，以及如何通过 Pod Disruption Budget（PDB）等机制来管理自愿中断，帮助应用所有者和集群管理员维护高可用性。

## 核心概念/原理
- **非自愿中断（Involuntary Disruptions）**：无法避免的中断，例如硬件故障、节点内核崩溃、节点网络分区、资源不足导致的驱逐等。
- **自愿中断（Voluntary Disruptions）**：由人或控制器主动发起的中断，例如：
  - 删除 Deployment 或直接删除 Pod
  - 更新 Deployment 的 Pod 模板导致滚动重启
  - `kubectl drain` 节点进行维护或缩容
  - 调度器抢占（preemption）低优先级 Pod
- **Pod Disruption Budget（PDB）**：限制因自愿中断而同时不可用的 Pod 数量，确保应用始终维持最低可用副本数。

## 关键机制或特性
- **PDB 工作原理**：通过标签选择器指定受保护的 Pod 组，并设置 `minAvailable` 或 `maxUnavailable`。使用 Eviction API（如 `kubectl drain`）时，调度器会尊重 PDB 约束。
- **PDB 不限制的情况**：直接删除 Deployment 或 Pod 会绕过 PDB；滚动更新不受 PDB 限制（由工作负载控制器自行管理）。
- **DisruptionTarget 条件（Stable）**：Pod 即将因中断被删除时，会添加 `DisruptionTarget` 条件，并附带具体原因：
  - `PreemptionByScheduler`
  - `DeletionByTaintManager`
  - `EvictionByEvictionAPI`
  - `DeletionByPodGC`
  - `TerminationByKubelet`
- **Unhealthy Pod Eviction Policy**：建议设置为 `AlwaysAllow`，以便在节点维护期间允许驱逐不健康的 Pod。

## 使用场景
- 运行基于仲裁的应用（如 etcd、ZooKeeper），需要保证最低副本数。
- 集群管理员进行节点维护、升级或缩容时，确保业务不中断。
- 多租户环境中，应用团队通过 PDB 声明可用性需求。

## 最佳实践/注意事项
- 为高可用应用配置 PDB，但不要依赖 PDB 防止所有中断（特别是直接删除操作）。
- 复制应用并跨机架/可用区分布，以进一步降低中断影响。
- 若集群未启用任何自动自愿中断源，可暂时跳过 PDB 配置。
- 集群管理员应使用遵守 Eviction API 的工具执行维护操作。
- 为 PDB 设置 `AlwaysAllow` 不健康 Pod 驱逐策略，避免节点 drain 被卡住。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/disruptions/
