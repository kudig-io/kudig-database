# Pod Group Policies

## 概述
Pod Group Policies 是 Workload API 的组成部分（Alpha，v1.35 默认禁用）。Workload 中定义的每个 Pod 组都必须声明一个调度策略，该策略决定调度器如何处理该组 Pod 的集合。

## 核心概念/原理
目前 API 支持两种策略类型，每个组必须且只能指定一种：
1. **basic（基本策略）**
   - 调度器将组内所有 Pod 视为独立实体，按标准 Kubernetes 行为逐个调度。
   - 主要用于组织 Pod 以提升可观测性和管理性，适用于不需要同时启动的组，或为未来引入非“全有或全无”约束做准备。
2. **gang（集体调度策略）**
   - 强制“全有或全无”调度（gang scheduling）。
   - 适用于紧耦合工作负载，部分启动会导致死锁或资源浪费的场景（如分布式训练、批处理作业）。
   - 需要指定 `minCount` 参数：只有当至少有 `minCount` 个 Pod 能够同时调度时，该组才会被允许绑定到节点。

## 关键机制或特性
- **策略冲突**：一个 Pod 组不能同时指定两种策略。
- **调度器行为**：
  - `basic`：Pod 独立参与调度，失败不会影响组内其他 Pod。
  - `gang`：组内 Pod 会等待彼此都被创建并满足 `minCount` 后，才一起进行绑定。

## 使用场景
- **basic**：逻辑上属于同一应用但启动顺序无关的组件，或当前仅需分组标签的场景。
- **gang**：
  - 需要所有 worker 同时运行的机器学习训练任务。
  - 基于仲裁或全连接的分布式计算框架（如 MPI）。

## 最佳实践/注意事项
- 使用 gang 策略时，确保 `minCount` 设置合理：过小可能导致资源碎片，过大可能导致调度失败。
- 需要 gang 调度时，务必启用 `GangScheduling` 特性门控。
- 对于不依赖同时启动的通用应用，优先使用 `basic` 策略，避免不必要的调度复杂度。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/workload-api/policies/
