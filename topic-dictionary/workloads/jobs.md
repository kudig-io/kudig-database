# Jobs

## 概述
Job 用于表示一次性任务，运行到完成即停止。Job 会创建一个或多个 Pod，并在达到指定成功完成数后结束。若 Pod 失败，Job 会根据配置进行重试。删除 Job 会级联删除其创建的 Pod。

## 核心概念/原理
- **Pod 模板**：`spec.template` 是必填字段，`restartPolicy` 只能为 `Never` 或 `OnFailure`。
- **并行模式**：
  - **非并行 Job**：不设置 `completions` 和 `parallelism`，默认均为 1。
  - **固定完成数并行 Job**：设置 `completions` 为所需成功数；可设置 `parallelism`。
  - **工作队列并行 Job**：不设置 `completions`，设置 `parallelism`；任一 Pod 成功后不再创建新 Pod，所有 Pod 终止后 Job 完成。
- **完成模式（`completionMode`）**：
  - `NonIndexed`（默认）：任意 Pod 成功都计入完成数。
  - `Indexed`：每个 Pod 获得唯一索引（0 到 `completions-1`），Job 在每个索引都有成功 Pod 后才算完成。索引可通过注解、标签、主机名和环境变量 `JOB_COMPLETION_INDEX` 获取。
- **失败重试**：
  - `backoffLimit`：默认 6，表示 Pod 失败重试次数上限。
  - `backoffLimitPerIndex`（v1.33 Stable）：为 Indexed Job 的每个索引独立设置重试上限；支持 `maxFailedIndexes`。
- **Pod 失败策略（`podFailurePolicy`，v1.31 Stable）**：基于容器退出码或 Pod 条件（如 `DisruptionTarget`）自定义失败处理，支持动作：`FailJob`、`Ignore`、`Count`、`FailIndex`。
- **成功策略（`successPolicy`）**：Indexed Job 可定义基于成功索引的规则，无需所有索引成功即可声明 Job 成功。

## 关键机制或特性
- **Job 终止与清理**：
  - `activeDeadlineSeconds`：Job 总运行时间上限，到达后强制终止所有 Pod。
  - `ttlSecondsAfterFinished`：Job 完成后自动清理的 TTL。
  - 终端条件：`Complete`（成功）或 `Failed`（失败）。v1.31 起，控制器会等所有 Pod 终止后才添加终端条件。
- **中间条件**：
  - `FailureTarget`：触发 Job 失败并清理 Pod。
  - `SuccessCriteriaMet`：触发 Job 成功并清理 lingering Pod。
- **挂起（Suspend）**：设置 `spec.suspend: true` 可暂停 Job，删除所有运行中 Pod；恢复后重新创建。
- **弹性索引 Job（Elastic Indexed Jobs，v1.31 Stable）**：可同时修改 `parallelism` 和 `completions` 且保持相等，实现 Indexed Job 的弹性扩缩容。
- **可变的调度指令（Mutable Scheduling Directives，v1.27 Stable）**：允许在 Job 启动前更新 Pod 模板的节点亲和性、节点选择器、容忍度等字段。
- **自定义选择器（`manualSelector`）**：高级用法，可手动指定 Pod 选择器，但需谨慎避免与其他控制器冲突。

## 使用场景
- 批处理计算、数据迁移、报表生成。
- 需要并行处理的分布式任务（如科学计算、视频转码）。
- 基于工作队列的任务消费模式。

## 最佳实践/注意事项
- 使用 `restartPolicy: Never` 调试 Job 更方便查看失败日志。
- 应用代码需具备幂等性，以应对 Pod 重启或重新调度。
- 使用 `podFailurePolicy` 忽略由集群中断（如抢占、驱逐）导致的失败，避免不必要的重试。
- 为已完成 Job 设置 `ttlSecondsAfterFinished`，防止 etcd 中堆积过多历史对象。
- 使用 Indexed Job 时，确保应用能正确读取 `JOB_COMPLETION_INDEX` 或主机名来分配任务。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/job/
