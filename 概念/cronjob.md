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

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/pods.md|Pod]]
- [[概念/deployments.md|Deployment]]
- [[概念/secrets.md|Secrets]] — 注入凭据
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
