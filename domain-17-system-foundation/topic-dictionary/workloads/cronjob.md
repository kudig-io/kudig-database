---
title: CronJob
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- controller-manager
- job
- cronjob
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CronJob 是什么
- 如何 CronJob
trigger_keywords:
- CronJob
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
created: "2026-05-23"
---

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
- ** Job 注解**：v1.32 起，CronJob 会在创建的 Job 上添加注解 `batch.[[entities/kubernetes|[[Kubernetes|kubernetes]]]].io/cronjob-scheduled-timestamp`，记录原始计划时间（RFC3339）。
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

## 实战 YAML 示例

以下为生产级数据库备份 CronJob 配置：

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: db-backup
  namespace: prod
  labels:
    app: db-backup
    tier: maintenance
spec:
  schedule: "0 2 * * *"                      # 每天凌晨 2:00 执行
  timeZone: "Asia/Shanghai"                  # 明确指定时区
  concurrencyPolicy: Forbid                  # 禁止并发（上一次未完成则跳过）
  startingDeadlineSeconds: 600               # 错过计划时间后 10 分钟内仍可启动
  successfulJobsHistoryLimit: 7              # 保留最近 7 次成功记录
  failedJobsHistoryLimit: 3                  # 保留最近 3 次失败记录
  jobTemplate:
    spec:
      activeDeadlineSeconds: 3600            # Job 最长运行 1 小时
      ttlSecondsAfterFinished: 86400         # 完成后保留 24 小时
      backoffLimit: 2                        # 最多重试 2 次
      template:
        metadata:
          labels:
            app: db-backup
        spec:
          restartPolicy: OnFailure
          serviceAccountName: db-backup-sa
          securityContext:
            runAsNonRoot: true
            runAsUser: 1000
          containers:
          - name: backup
            image: myregistry.com/db-backup-tool:v1.5.0
            command:
            - /bin/sh
            - -c
            - |
              set -euo pipefail
              TIMESTAMP=$(date +%Y%m%d-%H%M%S)
              BACKUP_FILE="/backup/db-${TIMESTAMP}.sql.gz"
              echo "开始备份: ${TIMESTAMP}"
              pg_dump -h $DB_HOST -U $DB_USER -d $DB_NAME | gzip > "${BACKUP_FILE}"
              echo "备份完成: ${BACKUP_FILE}, 大小: $(du -h ${BACKUP_FILE} | cut -f1)"
              # 清理 30 天前的备份
              find /backup -name "db-*.sql.gz" -mtime +30 -delete
              echo "旧备份清理完成"
            env:
            - name: DB_HOST
              value: "postgres-headless.prod.svc.cluster.local"
            - name: DB_NAME
              value: "mydb"
            - name: DB_USER
              valueFrom:
                secretKeyRef:
                  name: db-credentials
                  key: username
            - name: PGPASSWORD
              valueFrom:
                secretKeyRef:
                  name: db-credentials
                  key: password
            resources:
              requests:
                cpu: "100m"
                memory: "256Mi"
              limits:
                cpu: "500m"
                memory: "512Mi"
            volumeMounts:
            - name: backup-storage
              mountPath: /backup
          volumes:
          - name: backup-storage
            persistentVolumeClaim:
              claimName: db-backup-pvc
```

## 故障排查

### CronJob 不触发 / 到时间未执行
- **症状**: 到达计划时间后无新 Job 创建。
- **常见原因**: `suspend: true` 误开启；`concurrencyPolicy: Forbid` 且上一个 Job 仍在运行；时区配置错误。
- **诊断命令**:
  ```bash
  # 查看 CronJob 状态和最后调度时间
  kubectl get cronjob db-backup -n prod
  # 查看 CronJob 详细配置
  kubectl describe cronjob db-backup -n prod
  # 查看是否有正在运行的 Job
  kubectl get jobs -n prod -l app=db-backup
  # 检查 kube-controller-manager 日志
  kubectl logs -n kube-system -l component=kube-controller-manager --tail=50 | grep -i cronjob
  ```

### Job 反复失败
- **症状**: `failedJobsHistoryLimit` 中积累了多次失败。
- **常见原因**: 备份目标不可达、权限不足、存储空间已满。
- **诊断命令**:
  ```bash
  # 查看失败 Job 的 Pod 日志
  kubectl get jobs -n prod -l app=db-backup --sort-by=.status.startTime
  kubectl logs job/<latest-failed-job-name> -n prod
  ```

### CronJob 创建了重复 Job
- **症状**: 同一调度周期内出现 2 个 Job。
- **常见原因**: CronJob 控制器的已知行为（约每分钟检查），在高负载场景下可能出现。
- **解决方案**: 使用 `concurrencyPolicy: Forbid` 防止并发；确保 Job 逻辑幂等。

## 生产检查清单

- [ ] `timeZone` 已明确指定，不依赖节点默认时区
- [ ] `concurrencyPolicy` 根据业务需求设置（备份任务建议 `Forbid`）
- [ ] `startingDeadlineSeconds` 已设置，防止控制器重启后大量补执行
- [ ] `activeDeadlineSeconds` 已为 Job 设置超时限制
- [ ] `ttlSecondsAfterFinished` 已设置，自动清理已完成 Job
- [ ] Job 逻辑具备幂等性
- [ ] 备份等关键任务已配置告警（Job 失败通知）
- [ ] CronJob 名称不超过 52 个字符
- [ ] 历史 Job 保留数量合理，避免 API Server 压力

## 命令快速参考

```bash
# 查看 CronJob 列表和上次调度时间
kubectl get cronjob -n prod

# 手动触发一次 CronJob（创建即时 Job）
kubectl create job --from=cronjob/db-backup manual-backup-$(date +%s) -n prod

# 暂停 CronJob
kubectl patch cronjob db-backup -n prod -p '{"spec":{"suspend":true}}'

# 恢复 CronJob
kubectl patch cronjob db-backup -n prod -p '{"spec":{"suspend":false}}'

# 查看 CronJob 创建的 Job 列表
kubectl get jobs -n prod -l app=db-backup --sort-by=.status.startTime

# 查看最近一次 Job 的日志
kubectl logs job/$(kubectl get jobs -n prod -l app=db-backup --sort-by=.status.startTime -o jsonpath='{.items[-1].metadata.name}') -n prod
```

## 交叉引用

- [Job/CronJob 高级用法](../../domain-02-workloads-applications/05-job-cronjob-advanced.md)
- [已完成 Job 自动清理](./automatic-cleanup-for-finished-jobs.md)
- [Job/CronJob 故障树分析 (FTA)](../../domain-10-troubleshooting-diagnostics/topic-fta/list/job-cronjob-fta.md)
- [工作负载管理总览](./workload-management.md)
- [工作负载故障排查手册](../../domain-02-workloads-applications/07-workload-troubleshooting-handbook.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/cron-jobs/
