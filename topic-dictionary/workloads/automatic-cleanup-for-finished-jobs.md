# Automatic Cleanup for Finished Jobs

## 概述
TTL-after-finished 控制器为已完成的 Job 对象提供基于生存时间（TTL）的自动清理机制。它有助于减少 API Server 中已完成 Job 的累积，降低 etcd 压力。

## 核心概念/原理
- **触发时机**：计时器在 Job 状态变为 `Complete` 或 `Failed` 时开始计时。
- **级联删除**：TTL 到期后，控制器会自动删除 Job 及其依赖对象（如 Pod），并遵守对象的 finalizers 等生命周期保证。
- **配置字段**：在 Job 的 `spec.ttlSecondsAfterFinished` 字段中指定 TTL 秒数。

## 关键机制或特性
- **动态修改**：可以在 Job 创建后或完成后修改 `ttlSecondsAfterFinished` 字段，但若在原有 TTL 已过期后再延长，Kubernetes 不保证一定保留该 Job。
- **时间偏差敏感**：TTL 控制器依赖 Job 状态中的时间戳判断 TTL 是否到期，集群时钟偏差可能导致清理时间出现偏差。
- **多种设置方式**：
  - 在 Job 清单中直接声明。
  - 为已完成的 Job 手动设置。
  - 通过 mutating admission webhook 动态注入。
  - 编写自定义控制器按策略管理 TTL。

## 使用场景
- 大规模批处理平台中自动清理已成功或失败的临时 Job。
- 与 CronJob 配合，管理周期性任务产生的历史 Job（但 CronJob 本身也有 history limit）。
- 需要按完成状态设置不同保留策略的场景（可通过 webhook 实现）。

## 最佳实践/注意事项
- 建议为直接创建的 Job（ unmanaged jobs ）设置 `ttlSecondsAfterFinished`，因为默认删除策略可能导致 Pod 在 Job 删除后残留。
- 设置非零 TTL 时，注意集群时钟同步，避免意外提前或延迟清理。
- 若需要长期保留 Job 状态供审计使用，应使用外部日志/审计系统，而非依赖 Kubernetes API 对象。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/ttlafterfinished/
