---
title: "[2026-06-10] [P1] CronJob 并发策略导致任务堆积"
category: case-study
tags: [production, incident, workloads, cronjob, job, batch]
date: "2026-06-10"
severity: P1
mttr: "22min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
---

# [2026-06-10] CronJob 并发策略 Forbid 导致对账任务堆积，数据库连接池耗尽

## 工单信息
- **工单编号**: INC-2026-0610-013
- **发现时间**: 2026-06-10 04:30 UTC
- **恢复时间**: 2026-06-10 04:52 UTC
- **影响范围**: `prod-batch` namespace，对账系统数据库连接池
- **业务影响**: 数据库连接池耗尽，所有依赖同一数据库的在线服务响应延迟飙升

## 问题现象
04:30，数据库监控告警 `postgres_active_connections` 达到 800/800（max_connections=800）。在线服务 `order-api`、`payment-api` 的 P99 延迟从 150ms 飙升至 4s。

排查发现大量 `reconcile-job-*` Pod 处于 `Running` 状态：
```bash
kubectl get jobs -n prod-batch | grep reconcile
# reconcile-20260610-0000   1/1     170m   170m
# reconcile-20260610-0100   1/1     110m   110m
# reconcile-20260610-0200   1/1     50m    50m
# reconcile-20260610-0300   1/1     Running   10m
```

每个 Job 的 Pod 都持有大量数据库连接。

## 诊断过程

**04:32** — 检查 CronJob 配置：
```bash
kubectl get cronjob reconcile-job -n prod-batch -o yaml
# spec:
#   schedule: "0 * * * *"
#   concurrencyPolicy: Forbid
#   jobTemplate:
#     spec:
#       activeDeadlineSeconds: 7200
#       template:
#         spec:
#           containers:
#           - name: reconciler
#             resources:
#               requests:
#                 cpu: "1"
#                 memory: "2Gi"
```

**04:34** — 查看 Job 运行时长：
```bash
kubectl get job reconcile-20260610-0000 -n prod-batch -o jsonpath='{.status.conditions[?(@.type=="Complete")].lastTransitionTime}'
# （无 Complete 条件，Job 仍在运行）

kubectl logs -n prod-batch job/reconcile-20260610-0000 --tail -n 20
# 2026-06-10 04:33:12 INFO  Processing batch 15,892/20,000
# 2026-06-10 04:33:15 INFO  Current connection count: 120
# ...
```

**04:36** — 分析 Job 变慢原因：
```bash
# 对账数据量从 5 月的 100 万条增长到 6 月的 200 万条
# 但 Job 的资源配置未调整，处理速度下降
# 06-10 00:00 的 Job 预计需要 4 小时完成，而 schedule 是每小时一次
```

**04:38** — `concurrencyPolicy: Forbid` 的行为分析：
- 00:00 Job 启动，预计运行 4 小时
- 01:00 CronJob 触发，但上一个 Job 仍在运行，根据 `Forbid` 策略，跳过本次调度
- 02:00 同样跳过
- 03:00 跳过
- 04:00 跳过
- 但 00:00 的 Job 因连接数高、处理慢，仍未完成
- 更关键的是：另一个团队 06-01 部署了新的 `daily-report` CronJob（schedule: `0 0 * * *`），该 Job 的查询未加索引，执行了全表扫描，锁住了对账表，导致 `reconcile-job` 被阻塞

## 根因
1. `daily-report` CronJob（新部署）在 00:00 执行，查询未优化，导致对账表被长时间锁定
2. `reconcile-job` 的 00:00 批次被阻塞，运行时间从正常的 45min 延长至 4h+
3. `concurrencyPolicy: Forbid` 导致后续小时调度的对账任务全部被跳过
4. 积压的对账 Job 持有大量数据库连接，导致连接池耗尽，在线服务受影响

## 修复动作

**04:40** — 终止异常 Job：
```bash
kubectl delete job reconcile-20260610-0000 -n prod-batch --cascade=foreground
# 释放数据库连接
```

**04:42** — 手动触发一次对账：
```bash
kubectl create job --from=cronjob/reconcile-job reconcile-20260610-0430 -n prod-batch
kubectl get job reconcile-20260610-0430 -n prod-batch
# reconcile-20260610-0430   1/1     Running   2m
```

**04:45** — 调整 `reconcile-job` 资源配置：
```bash
kubectl patch cronjob reconcile-job -n prod-batch --type='merge' -p '
{
  "spec": {
    "jobTemplate": {
      "spec": {
        "template": {
          "spec": {
            "containers": [{
              "name": "reconciler",
              "resources": {
                "requests": {"cpu": "2", "memory": "4Gi"},
                "limits": {"cpu": "4", "memory": "8Gi"}
              }
            }]
          }
        }
      }
    }
  }
}'
```

**04:48** — 暂停有问题的 `daily-report` Job：
```bash
kubectl patch cronjob daily-report -n prod-batch --type='merge' -p '{"spec":{"suspend":true}}'
```

## 验证
- 04:50 — 数据库连接数从 800 降至 120
- 04:51 — 在线服务 P99 延迟从 4s 恢复至 180ms
- 04:52 — 对账 Job 正常完成

## 复盘
- **直接原因**: daily-report Job 锁定对账表 → reconcile-job 运行时间超长 → concurrencyPolicy:Forbid 跳过调度 → 连接池耗尽
- **根本原因**: 
  1. 新 CronJob 未经数据库影响评审
  2. 未评估数据增长对批处理时长的影响
- **改进措施**:
  1. 将 `concurrencyPolicy: Forbid` 改为 `Replace`，确保最新任务总能运行
  2. 为所有批处理 Job 添加 `activeDeadlineSeconds: 3600`，超时自动终止
  3. 新 CronJob 部署前必须经过 DBA 评审，检查 SQL 执行计划
  4. 数据库连接池隔离：批处理 Job 使用独立的数据库连接池（只读副本）
  5. 添加告警：`cronjob_last_schedule_time - cronjob_last_successful_time > 2h`
- **相关 Skill**: [[ts-workloads]]
- **相关 FTA**: [[job-cronjob-fta]]
