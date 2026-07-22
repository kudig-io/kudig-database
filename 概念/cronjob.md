---
title: CronJob
summary: CronJob 用于在 Kubernetes 集群中按时间计划运行任务。
category: concepts
tags:
- core-concept
- k8s
- workloads
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# CronJob

## 概述

CronJob 是 Kubernetes 中按 Cron 表达式定时运行的批处理工作负载控制器，每次触发会创建一个 Job，Job 再创建 Pod 执行任务。它替代了传统 Linux 的 crontab，把定时任务纳入声明式、可观测、可重试、跨节点调度的统一体系。典型场景包括：数据库备份、定时报表生成、证书续期、日志清理、数据同步、定期 ETL、缓存预热等。

## 架构与工作原理

```
CronJob (batch/v1)
   │ spec.schedule: "0 2 * * *"   # 每天凌晨 2 点
   │ spec.jobTemplate
   ▼
触发时刻 → 创建 Job (batch/v1)
              │ spec.completions / parallelism
              ▼
           Pod(s)  → 执行任务 → 成功退出 (exit 0)
                      │
                      │ 失败 → 按 backoffLimit 重试
                      ▼
                   Job 记录状态（Succeeded / Failed）
```

**工作流**：
1. CronJob Controller（kube-controller-manager 内）每分钟评估所有 CronJob 的 schedule。
2. 到点则根据 `jobTemplate` 创建一个 Job；Job Controller 再创建 Pod 执行。
3. Pod 失败按 `backoffLimit`（默认 6）重试；达到 `activeDeadlineSeconds` 仍未完成则标记失败。
4. 保留的历史 Job 受 `successfulJobsHistoryLimit`（默认 3）和 `failedJobsHistoryLimit`（默认 1）约束，超出自动清理。
5. `concurrencyPolicy` 决定：上次还没跑完又到点时怎么办——`Allow`（并发）/ `Forbid`（跳过）/ `Replace`（停旧起新）。

**Cron 表达式**：5 字段（标准 cron）或 6 字段（带秒，1.27+）。时区默认 UTC，建议用环境变量或显式说明。

## 关键组件与特性

| 字段 | 作用 |
|------|------|
| `schedule` | Cron 表达式（必填） |
| `jobTemplate` | Job 模板 |
| `concurrencyPolicy` | Allow / Forbid / Replace |
| `startingDeadlineSeconds` | 错过启动窗口多久内仍可补跑（默认无） |
| `successfulJobsHistoryLimit` | 保留成功 Job 数（默认 3） |
| `failedJobsHistoryLimit` | 保留失败 Job 数（默认 1） |
| `timeZone` | 1.25+ beta，指定时区（如 Asia/Shanghai） |
| `suspend` | 临时挂起（不触发新 Job） |

## 配置示例

```yaml
---
apiVersion: batch/v1
kind: CronJob
metadata:
  name: db-backup
  namespace: production
spec:
  schedule: "0 19 * * *"             # UTC 19:00 = 北京时间次日 03:00
  timeZone: UTC
  concurrencyPolicy: Forbid          # 上次没跑完则跳过本次
  successfulJobsHistoryLimit: 5
  failedJobsHistoryLimit: 3
  startingDeadlineSeconds: 300       # 延迟 5 分钟内仍可触发
  jobTemplate:
    spec:
      backoffLimit: 3                # 失败重试次数
      activeDeadlineSeconds: 3600    # 单次最长 1 小时
      template:
        spec:
          restartPolicy: OnFailure
          serviceAccountName: backup-sa
          containers:
          - name: pgdump
            image: postgres:16
            env:
            - {name: PGHOST,   valueFrom: {secretKeyRef: {name: db, key: host}}}
            - {name: PGPASSWORD, valueFrom: {secretKeyRef: {name: db, key: password}}}
            command:
            - /bin/sh
            - -c
            - |
              set -e
              TS=$(date -u +%Y%m%d-%H%M)
              pg_dump -U app -Fc mydb | gzip > /backup/db-${TS}.dump.gz
              # 上传到对象存储（mc 为 minio client）
              mc alias set s3 https://s3.example.com "$AWS_KEY" "$AWS_SECRET"
              mc cp /backup/db-${TS}.dump.gz s3/backups/
              # 保留近 30 天
              mc find s3/backups/ --older-than 30d --exec "mc rm {}"
```

## 常用操作与命令

```bash
# 查看 CronJob 与触发历史
kubectl get cronjob -n production
kubectl get jobs -n production
kubectl describe cronjob db-backup -n production

# 手动触发一次（创建同名 Job，不会污染 CronJob 调度）
kubectl create job --from=cronjob/db-backup db-backup-manual -n production

# 查看具体某次执行
kubectl logs job/db-backup-28712345 -n production
kubectl get pod -n production -l job-name=db-backup-28712345

# 临时挂起 / 恢复
kubectl patch cronjob db-backup -p '{"spec":{"suspend":true}}' -n production
kubectl patch cronjob db-backup -p '{"spec":{"suspend":false}}' -n production

# 修改计划
kubectl patch cronjob db-backup -p '{"spec":{"schedule":"*/30 * * * *"}}' -n production
```

## 最佳实践

1. **幂等设计**：定时任务必须可重复执行不产生副作用（如备份用带时间戳文件名、删除前判断存在）。
2. **设置 activeDeadlineSeconds**：避免任务卡死占用资源、堆积延迟。
3. **concurrencyPolicy: Forbid**：长任务场景防止重叠执行导致数据错乱。
4. **资源 requests/limits**：批处理任务也要限资源，防止突发吃光节点。
5. **失败告警**：配合 Prometheus 抓 `kube_job_status_failed`，或用 `failedJobsHistoryLimit` 保留记录供排查。
6. **时区显式声明**：用 `timeZone` 字段或注释说明 UTC，避免跨团队误解。
7. **日志结构化**：任务日志写 JSON，便于 Loki/ELK 检索与告警。

## 常见陷阱

- **任务没触发**：CronJob 被 suspend，或 schedule 时区与预期不一致（UTC vs 本地）。
- **并发重复执行**：concurrencyPolicy=Allow 下，长任务叠加导致数据竞争。
- **错过执行窗口**：controller-manager 曾停机，超 `startingDeadlineSeconds` 则跳过。
- **Pod 一直 Pending**：资源不足或节点 Taint，Job 永不执行最终超时；检查 events。
- **历史 Job 堆积**：historyLimit 未生效或失败次数多，导致 etcd 膨胀；手动清理。
- **容器退出码非 0 仍标记成功**：检查 `command` 是否吞掉错误（如 `set -e` 缺失）。
- **大任务 OOMKilled**：批量处理内存峰值未评估，limit 设得太低。

## 源码实现分析

### CronJob Controller 调度逻辑

```go
// k8s.io/kubernetes/pkg/controller/cronjob/cronjob_controller.go
func (jm *Controller) syncCronJob(ctx context.Context, cj *batch.CronJob) error {
    // 1. 解析 cron schedule（支持时区）
    sched, err := cron.ParseStandard(cj.Spec.Schedule)
    // 2. 计算上次应调度时间
    var earliestTime time.Time
    if cj.Status.LastScheduleTime != nil {
        earliestTime = cj.Status.LastScheduleTime.Time
    } else {
        earliestTime = cj.CreationTimestamp.Time
    }
    // 3. 检查是否错过调度窗口
    if cj.Spec.StartingDeadlineSeconds != nil {
        // 错过超过 startingDeadlineSeconds 则跳过
        if time.Since(earliestTime) > time.Duration(*cj.Spec.StartingDeadlineSeconds)*time.Second {
            return nil // 跳过本次执行
        }
    }
    // 4. 检查并发策略
    activeJobs := jm.getActiveJobs(cj)
    if cj.Spec.ConcurrencyPolicy == batch.ForbidConcurrent && len(activeJobs) > 0 {
        return nil // 禁止并发，跳过
    }
    if cj.Spec.ConcurrencyPolicy == batch.ReplaceConcurrent {
        // 删除当前运行的 Job，创建新的
        jm.deleteActiveJobs(activeJobs)
    }
    // 5. 创建新 Job
    job := jm.getJobFromTemplate(cj, scheduledTime)
    jm.kubeClient.BatchV1().Jobs(ns).Create(ctx, job)
    // 6. 更新 Status.LastScheduleTime
    cj.Status.LastScheduleTime = &metav1.Time{Time: scheduledTime}
    jm.kubeClient.BatchV1().CronJobs(ns).UpdateStatus(ctx, cj)
    return nil
}
```

### CronJob → Job → Pod 层次关系

```
┌──────────────────────────────────────────────────────────┐
│          CronJob → Job → Pod 层次关系                 │
├──────────────────────────────────────────────────────────┤
│  CronJob (db-backup, schedule: "0 2 * * *")              │
│    │  每次触发创建一个 Job                              │
│    ├─ Job (db-backup-28712345) [完成]                    │
│    │     └─ Pod (db-backup-28712345-x7k2p) [Succeeded]  │
│    ├─ Job (db-backup-28712346) [完成]                    │
│    │     └─ Pod (db-backup-28712346-m3n8q) [Succeeded]  │
│    └─ Job (db-backup-28712347) [运行中]                  │
│          └─ Pod (db-backup-28712347-p5r1s) [Running]    │
│                                                          │
│  关键参数:                                              │
│  • successfulJobsHistoryLimit: 保留成功 Job 数 (默认 3) │
│  • failedJobsHistoryLimit: 保留失败 Job 数 (默认 1)     │
│  • concurrencyPolicy: Allow/Forbid/Replace              │
│  • startingDeadlineSeconds: 错过窗口后跳过            │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：生产级定时备份任务

```yaml
# 🟡 中风险：创建定时任务影响集群资源
apiVersion: batch/v1
kind: CronJob
metadata:
  name: db-backup
  namespace: production
spec:
  schedule: "0 2 * * *"  # 每天凌晨 2点
  timeZone: "Asia/Shanghai"  # K8s 1.27+ 显式时区
  concurrencyPolicy: Forbid  # 禁止并发
  startingDeadlineSeconds: 600  # 错过 10分钟则跳过
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 3
  jobTemplate:
    spec:
      activeDeadlineSeconds: 3600  # 1小时超时
      backoffLimit: 2  # 失败重试 2 次
      template:
        spec:
          restartPolicy: Never
          containers:
          - name: backup
            image: registry/db-backup:v1.2.0
            command: ["/backup.sh"]
            resources:
              requests: {cpu: "500m", memory: "1Gi"}
              limits: {cpu: "2", memory: "4Gi"}
            env:
            - name: DB_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: db-credentials
                  key: password
```

### 场景二：手动触发与调试

```bash
# 🟡 中风险：手动创建 Job 消耗资源
# 手动触发一次（不影响 CronJob 调度）
kubectl create job --from=cronjob/db-backup db-backup-manual-$(date +%s) -n production
# 查看执行日志
kubectl logs -l job-name=db-backup-manual-1719000000 -n production -f
# 检查失败原因
kubectl describe job db-backup-28712347 -n production | grep -A10 Events
kubectl get pods -l job-name=db-backup-28712347 -n production
# 临时挂起（维护期间）
kubectl patch cronjob db-backup -p '{"spec":{"suspend":true}}' -n production
# 恢复
kubectl patch cronjob db-backup -p '{"spec":{"suspend":false}}' -n production
```

### 场景三：清理历史 Job 堆积

```bash
# 🟡 中风险：删除 Job 会级联删除 Pod
# 检查历史 Job 数量
kubectl get jobs -n production | wc -l
# 清理已完成的历史 Job
kubectl get jobs -n production -o json | \
  jq -r '.items[] | select(.status.succeeded==1) | .metadata.name' | \
  tail -n +4 | xargs -I{} kubectl delete job {} -n production
# 或者调整 historyLimit 自动清理
kubectl patch cronjob db-backup -p '{"spec":{"successfulJobsHistoryLimit":3,"failedJobsHistoryLimit":2}}' -n production
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | CronJob 保证精确执行 | 调度有延迟（controller-manager 周期）；错过 startingDeadlineSeconds 则跳过 |
| 2 | 默认禁止并发 | 默认 concurrencyPolicy=Allow，长任务会叠加；生产应设 Forbid |
| 3 | 时区默认是本地时间 | 默认 UTC！K8s 1.27+ 支持 timeZone 字段；之前版本需手动换算 |
| 4 | 任务失败会自动重试 | Job 默认 backoffLimit=6，但 CronJob 不会重新触发；需配置 backoffLimit |
| 5 | 历史 Job 自动清理 | 默认只保留 3 个成功 + 1 个失败；大量失败时会堆积，需监控 |
| 6 | 容器退出码 0 就是成功 | 脚本缺少 set -e 可能吐掉错误；确保错误时返回非 0 退出码 |

## 面试要点

1. **Q: CronJob 的调度流程是怎样的？错过执行会怎样？**
   A: ① CronJob Controller 每 10s 检查所有 CronJob；② 解析 schedule 计算上次应调度时间；③ 检查 concurrencyPolicy（Allow/Forbid/Replace）；④ 创建 Job（从 jobTemplate 生成）；⑤ 更新 LastScheduleTime。错过处理：若错过时间 > startingDeadlineSeconds，跳过本次执行（不补执行）；若 controller-manager 曾停机，重启后会检查所有错过的调度点。

2. **Q: concurrencyPolicy 三种策略的区别和适用场景？**
   A: ① Allow（默认）：允许多个 Job 并发运行；适用：无状态、幂等任务（如日志清理）。② Forbid：上一个 Job 未完成则跳过新调度；适用：数据库备份、ETL 等不能并发的任务。③ Replace：删除当前运行的 Job，创建新的；适用：实时性要求高、旧执行无价值的任务（如指标采集）。生产默认用 Forbid 最安全。

3. **Q: 如何设计一个可靠的定时任务？**
   A: ① 幂等性：任务可重复执行无副作用（带时间戳文件名、先查后删）；② 超时控制：activeDeadlineSeconds 防止卡死；③ 重试策略：backoffLimit=2-3，避免无限重试；④ 资源限制：requests/limits 防止吃光节点；⑤ 失败告警：Prometheus 监控 kube_job_status_failed；⑥ 日志结构化：JSON 格式便于检索；⑦ 凭据安全：用 Secret 而非环境变量明文。

4. **Q: CronJob 与外部调度器（Airflow/Argo Workflows）如何选择？**
   A: K8s CronJob：简单定时任务（备份、清理、报表），无依赖关系，无需可视化。Argo Workflows：DAG 工作流（多步骤依赖、条件分支、参数传递），可视化执行历史。Airflow：复杂数据管道（数百任务依赖、回填、重试策略、丰富插件）。原则：简单任务用 CronJob；复杂编排用专业工具。

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/pods.md|Pod]]
- [[概念/deployments.md|Deployment]]
- [[概念/secrets.md|Secrets]] — 注入凭据
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
