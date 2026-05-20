---
title: Jobs
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- job
- cronjob
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Jobs 是什么
- 如何 Jobs
trigger_keywords:
- Jobs
- dictionary
title_en: Jobs
---


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

## 实战 YAML 示例

### 基础 Job：数据迁移

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration-v2
  namespace: prod
  labels:
    app: db-migration
    version: v2
spec:
  backoffLimit: 3                            # 最多重试 3 次
  activeDeadlineSeconds: 1800                # 最长运行 30 分钟
  ttlSecondsAfterFinished: 3600              # 完成后保留 1 小时
  template:
    metadata:
      labels:
        app: db-migration
    spec:
      restartPolicy: Never                   # 失败后不重启，方便查日志
      serviceAccountName: migration-sa
      containers:
      - name: migrate
        image: myregistry.com/db-migrate:v2.0.0
        command: ["./migrate", "--target=v2", "--confirm"]
        env:
        - name: DB_URL
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: url
        resources:
          requests:
            cpu: "200m"
            memory: "256Mi"
          limits:
            cpu: "1000m"
            memory: "512Mi"
  # Pod 失败策略：忽略因节点维护导致的驱逐
  podFailurePolicy:
    rules:
    - action: Ignore
      onPodConditions:
      - type: DisruptionTarget                # 集群中断不计入失败次数
```

### 并行 Indexed Job：批处理任务

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: video-transcode
  namespace: prod
spec:
  completions: 100                           # 需要完成 100 个任务
  parallelism: 10                            # 同时运行 10 个 Pod
  completionMode: Indexed                    # 每个 Pod 获得唯一索引
  backoffLimit: 5
  backoffLimitPerIndex: 2                    # 每个索引最多重试 2 次
  maxFailedIndexes: 5                        # 最多允许 5 个索引失败
  ttlSecondsAfterFinished: 7200
  template:
    spec:
      restartPolicy: Never
      containers:
      - name: transcode
        image: myregistry.com/transcoder:v1.0
        command:
        - /bin/sh
        - -c
        - |
          echo "处理任务索引: ${JOB_COMPLETION_INDEX}"
          ./transcode --task-id=${JOB_COMPLETION_INDEX} --total=100
        env:
        - name: JOB_COMPLETION_INDEX
          valueFrom:
            fieldRef:
              fieldPath: metadata.annotations['batch.kubernetes.io/job-completion-index']
        resources:
          requests:
            cpu: "500m"
            memory: "1Gi"
          limits:
            cpu: "2000m"
            memory: "4Gi"
```

## 故障排查

### Job 达到 backoffLimit 后标记为 Failed
- **症状**: Job 状态为 Failed，条件显示 `BackoffLimitExceeded`。
- **常见原因**: Pod 中的应用持续失败（配置错误、依赖不可达、代码 bug）。
- **诊断命令**:
  ```bash
  # 查看 Job 条件
  kubectl describe job db-migration-v2 -n prod | grep -A 5 "Conditions"
  # 查看所有相关 Pod（包括已失败的）
  kubectl get pods -n prod -l job-name=db-migration-v2
  # 查看最近失败 Pod 的日志
  kubectl logs -n prod -l job-name=db-migration-v2 --tail=50
  ```

### Job 运行超时
- **症状**: Job 状态为 Failed，原因为 `DeadlineExceeded`。
- **常见原因**: `activeDeadlineSeconds` 设置过小，或任务实际运行时间超出预期。
- **诊断命令**:
  ```bash
  kubectl get job db-migration-v2 -n prod -o jsonpath='{.status.conditions}'
  ```
- **解决方案**: 增大 `activeDeadlineSeconds`，或优化任务执行效率。

### 已完成 Job 堆积导致 etcd 压力
- **症状**: `kubectl get jobs` 返回大量已完成 Job，API Server 响应变慢。
- **诊断命令**:
  ```bash
  kubectl get jobs -n prod --no-headers | wc -l
  kubectl get jobs -n prod --field-selector=status.successful=1 --no-headers | wc -l
  ```
- **解决方案**: 为 Job 设置 `ttlSecondsAfterFinished`，使用 CronJob 的 `successfulJobsHistoryLimit`/`failedJobsHistoryLimit`。

## 生产就绪检查清单

- [ ] `backoffLimit` 根据任务特性设置（幂等任务可多重试，非幂等任务限制重试）
- [ ] `activeDeadlineSeconds` 已设置，防止任务无限运行
- [ ] `ttlSecondsAfterFinished` 已设置，自动清理历史 Job
- [ ] `podFailurePolicy` 配置了对 `DisruptionTarget` 的 Ignore 规则
- [ ] Job 逻辑具备幂等性
- [ ] `restartPolicy` 调试阶段用 `Never`，生产环境按需选择
- [ ] 并行 Job 的 `parallelism` 考虑集群资源容量
- [ ] Indexed Job 的应用正确使用了 `JOB_COMPLETION_INDEX`

## 命令快速参考

```bash
# 查看 Job 状态
kubectl get jobs -n prod

# 查看 Job 详情和条件
kubectl describe job <job-name> -n prod

# 查看 Job 创建的 Pod
kubectl get pods -n prod -l job-name=<job-name>

# 查看 Job Pod 的日志
kubectl logs job/<job-name> -n prod

# 手动删除 Job 及其 Pod
kubectl delete job <job-name> -n prod

# 仅删除 Job 保留 Pod
kubectl delete job <job-name> -n prod --cascade=orphan

# 暂停 Job
kubectl patch job <job-name> -n prod -p '{"spec":{"suspend":true}}'
```

## 交叉引用

- [Job/CronJob 高级用法](../../domain-4-workloads/05-job-cronjob-advanced.md)
- [CronJob 定时任务](./cronjob.md)
- [已完成 Job 自动清理](./automatic-cleanup-for-finished-jobs.md)
- [Job/CronJob 故障树分析 (FTA)](../../topic-fta/list/job-cronjob-fta.md)
- [工作负载故障排查手册](../../domain-4-workloads/07-workload-troubleshooting-handbook.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/job/
