# Pod Lifecycle

## 概述
Pod 遵循一个确定的生命周期，从 `Pending` 阶段开始，如果至少一个主容器正常启动则进入 `Running`，最终根据容器终止情况进入 `Succeeded` 或 `Failed` 阶段。Pod 被视为相对短暂（ephemeral）的实体。

## 核心概念/原理
- **Pod Phase（阶段）**：`Pending` → `Running` → `Succeeded`/`Failed`/`Unknown`。
  - `Pending`：已被集群接受，但容器尚未全部就绪（包括调度、镜像拉取时间）。
  - `Running`：已绑定到节点，至少有一个容器仍在运行。
  - `Succeeded`：所有容器成功终止且不会重启。
  - `Failed`：所有容器终止，且至少有一个失败退出。
  - `Unknown`：无法获取 Pod 状态（通常是与节点通信失败）。
- **容器状态**：`Waiting`、`Running`、`Terminated`。
- **Restart Policy**：
  - `Always`（默认）：任何终止都重启。
  - `OnFailure`：仅在非零退出码时重启。
  - `Never`：不自动重启。
- **CrashLoopBackOff**：容器反复崩溃时，kubelet 会应用指数退避延迟（10s、20s、40s…，上限 300s）。

## 关键机制或特性
- **Pod Conditions**：包括 `PodScheduled`、`Initialized`、`ContainersReady`、`Ready`、`PodReadyToStartContainers`、`DisruptionTarget`、`PodResizePending`、`PodResizeInProgress`。
- **Readiness Gates**：允许应用向 PodStatus 注入额外的就绪条件，Pod 只有在所有自定义条件为 `True` 时才被视为 `Ready`。
- **Pod 终止流程**：
  1. 设置 `deletionTimestamp` 和优雅期（默认 30s）。
  2. 执行 `preStop` Hook。
  3. 发送 TERM（SIGTERM）信号。
  4. 控制平面将终止中的 Pod 从 EndpointSlice 中移除（`ready=false`）。
  5. 优雅期过后发送 KILL（SIGKILL）信号，强制清理。
- **强制终止**：`--grace-period=0 --force` 可立即从 API Server 删除 Pod。
- **Sidecar 容器终止顺序**：Sidecar 容器在主容器完全终止后才接收 TERM 信号，并按定义顺序的反向终止。
- **容器级 Restart Policy（Beta）**：`ContainerRestartRules` 特性门控启用后，可为单个容器指定 `restartPolicy` 和 `restartPolicyRules`。
- **Pod 原地重启（Alpha）**：`RestartAllContainersOnContainerExits` 允许通过规则触发整个 Pod 的原地重启（保留 UID、IP、Volume）。

## 使用场景
- 需要理解 Pod 健康状态和故障排查（如 `CrashLoopBackOff`）。
- 配置优雅关闭流程，确保应用在删除 Pod 时有时间处理未完成请求。
- 使用 Sidecar 容器时，需理解其特殊的启动和终止顺序。

## 最佳实践/注意事项
- 区分 `Status`（kubectl 显示字段）与 `phase`（API 数据模型）。
- 为需要长时间关闭的应用设置足够的 `terminationGracePeriodSeconds`。
- 如果 `preStop` Hook 执行时间较长，务必相应增加 `terminationGracePeriodSeconds`。
- 设置 `activeDeadlineSeconds` 防止 Init 容器无限期失败（但注意该字段在 Init 容器完成后仍然生效）。
- 调试 `CrashLoopBackOff` 时，优先查看容器日志和 `kubectl describe pod` 事件。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/
