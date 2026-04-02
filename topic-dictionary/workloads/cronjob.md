# CronJob

## 概述
CronJob 用于按重复的时间表创建 Job，类似于 Unix 系统中的 crontab。它适合执行定期任务，如数据备份、报表生成、定时清理等。

## 核心概念/原理
- **调度语法**：`spec.schedule` 使用标准 Cron 语法（分 时 日 月 周），也支持扩展宏（如 `@hourly`、`@daily`、`@weekly`、`@monthly`、`@yearly`）。
- **时区（`timeZone`）**：v1.27 起稳定支持。默认使用 kube-controller-manager 的本地时区；可通过 `spec.timeZone` 指定时区（如 `Etc/UTC`、`Asia/Shanghai`）。
  - **不支持**在 `spec.schedule` 中使用 `CRON_TZ` 或 `TZ` 变量。
- **Job 模板（`jobTemplate`）**：定义 CronJob 创建的 Job 的规格，与 Job 的 schema 相同，但不包含 `apiVersion` 和 `kind`。
- **启动截止时间（`startingDeadlineSeconds`）**：允许 Job 在错过计划时间后多久内仍可启动。若超过则跳过该次执行。默认值表示无限制（但受 100 次错过限制约束）。
- **并发策略（`concurrencyPolicy`）**：
  - `Allow`（默认）：允许并发执行。
  - `Forbid`：禁止并发，若前一次未结束则跳过。
  - `Replace`：若前一次未结束，则替换为新的 Job。
- **历史限制**：
  - `successfulJobsHistoryLimit`：保留的成功 Job 数，默认 3。
  - `failedJobsHistoryLimit`：保留的失败 Job 数，默认 1。
- **挂起（`suspend`）**：设置为 `true` 可暂停后续执行；已启动的 Job 不受影响。

## 关键机制或特性
- **近似调度**：CronJob 控制器大约每分钟检查一次调度，某些情况下可能创建 0 个或 2 个 Job。
- **100 次错过限制**：若从上次计划时间到现在错过的调度超过 100 次，控制器会报错并跳过启动。设置 `startingDeadlineSeconds` 可改变计算窗口。
- ** Job 注解**：v1.32 起，CronJob 会在创建的 Job 上添加注解 `batch.kubernetes.io/cronjob-scheduled-timestamp`，记录原始计划时间（RFC3339）。
- **幂等性**：由于可能出现重复执行或跳过，Job 任务应设计为幂等。

## 使用场景
- 定期数据库备份、日志归档。
- 定时生成业务报表或发送通知。
- 周期性的数据清理和同步任务。

## 最佳实践/注意事项
- 确保任务本身是幂等的，以应对可能的重复创建或跳过。
- 若任务执行时间较长且不希望重叠，使用 `concurrencyPolicy: Forbid` 或 `Replace`。
- 设置合理的 `startingDeadlineSeconds`，避免因控制器短暂不可用而大量堆积任务。
- 注意 CronJob 名称长度不能超过 52 个字符（控制器会自动追加 11 个字符，Job 名称总长不能超过 63）。
- 通过 `timeZone` 字段指定时区，避免依赖节点本地时区。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/cron-jobs/
