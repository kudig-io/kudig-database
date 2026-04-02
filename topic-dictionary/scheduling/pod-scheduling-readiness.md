# Pod Scheduling Readiness

## 概述

Pod 调度就绪性（Pod Scheduling Readiness）允许用户通过设置或移除 Pod 的 `.spec.schedulingGates` 字段来控制 Pod 何时准备好被调度器考虑。在 Kubernetes v1.30 中达到 stable 状态。

## 核心概念/原理

传统上，Pod 一旦创建就被认为是可调度就绪的。但在实际场景中，某些 Pod 可能会长期处于"缺少必要资源"的状态，不必要地消耗调度器（以及下游集成组件如 Cluster AutoScaler）的资源。

通过 `schedulingGates` 字段，可以显式控制 Pod 进入调度队列的时机。

## 关键机制或特性

- **schedulingGates 字段**：包含一个字符串列表，每个字符串代表一个条件，Pod 必须满足所有这些条件才会被视为可调度。
- **生命周期限制**：该字段只能在 Pod 创建时初始化（由客户端创建或在准入阶段修改）。创建后，可以以任意顺序移除每个 schedulingGate，但不允许添加新的调度门控。
- **Pod 状态**：带有 schedulingGates 的 Pod 会处于 `SchedulingGated` 状态。
- **可观测性**：`scheduler_pending_pods` 指标新增了 `"gated"` 标签，用于区分 Pod 是不可调度还是显式标记为未准备好调度。可以通过 `scheduler_pending_pods{queue="gated"}` 查看。
- **可变调度指令**：在 Pod 具有调度门控时，可以变更其调度指令，但只能收紧（tighten）这些指令：
  - `.spec.nodeSelector` 只允许增加。
  - `spec.affinity.nodeAffinity` 的 `requiredDuringSchedulingIgnoredDuringExecution` 中的 `NodeSelectorTerms` 为空时可以设置；不为空时只允许增加 `matchExpressions` 或 `fieldExpressions`。
  - `.preferredDuringSchedulingIgnoredDuringExecution` 允许所有更新。

## 使用场景

- 当 Pod 依赖外部资源（如持久卷、配置、密钥）尚未就绪时，暂时阻止调度器处理该 Pod。
- 与 Cluster AutoScaler 配合，避免在资源未完全准备好时触发不必要的扩容操作。
- 需要按顺序或条件启动的工作负载，可以等待某些前置条件满足后再允许调度。

## 最佳实践/注意事项

- `schedulingGates` 只能在 Pod 创建时设置，创建后只能移除不能添加。
- 移除所有 `schedulingGates` 后，Pod 才会进入正常的调度流程。
- 在变更调度指令时要确保只收紧约束，否则更新会被拒绝。

## 参考链接

- [Kubernetes 官方文档 - Pod Scheduling Readiness](https://kubernetes.io/docs/concepts/scheduling-eviction/pod-scheduling-readiness/)
