---
title: 第九课：Job 和 CronJob - 任务调度 [fundamentals]
description: 'title: 第九课：Job 和 CronJob - 任务调度'
summary: 'title: 第九课：Job 和 CronJob - 任务调度'
category: learning
tags:
- k8s
- training
- hands-on
- redis
- hpa
- daemonset
- job
- cronjob
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 第九课：Job 和 CronJob - 任务调度 是什么
- 如何 第九课：Job 和 CronJob - 任务调度
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 第九课：Job
- CronJob
- 任务调度
- production
- operations
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- redis-basics
---



---
title: 第九课：Job 和 [[CronJob|CronJob]] - 任务调度
description: '# 第九课：Job 和 CronJob - 任务调度'
category: learning
tags:
- tutorial
- k8s
- training
- lecturer
- redis
- job
- cronjob
- rbac
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 初学者
- 运维工程师
- 培训师
- 技术经理
estimated_read_time: 5min
intent_queries:
- 第九课：Job 和 CronJob - 任务调度 是什么
- 如何 第九课：Job 和 CronJob - 任务调度
trigger_keywords:
- 第九课：Job
- CronJob
- 任务调度
- k8s
- learning
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'

tier: peripheral---

# 第九课：Job 和 CronJob - 任务调度

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 20 分钟

---

## 学习目标

1. 理解 Job 和 CronJob 的概念
2. 掌握 Job 和 CronJob 的创建和配置
3. 了解并行执行策略
4. 学会排查任务失败问题

---

## 1. 问题引入

### 1.1 问题场景

```
【场景】

你有一个数据处理任务，需要：
• 批量处理 10000 条数据
• 每天凌晨 2 点执行数据库备份
• 跑完就结束，不需要持续运行

问题：这种"一次性"或"定时"任务怎么管理？

【Deployment/Service 的问题】

Deployment 适合持续运行的服务（Web 应用、API）。
但对于一次性任务，Deployment 不太合适：
• 任务完成后 Pod 仍然运行，浪费资源
• 无法设置定时执行
• 无法追踪任务执行状态

【解决方案】

Job 和 CronJob！

Job = 一次性任务（批处理、离线计算）
CronJob = 定时任务（每天备份、周期报表）
```

### 1.2 类比说明

```
【餐厅类比】

Deployment = 餐厅的常驻服务员（持续工作）
Job = 外卖订单（来了就做，做完就结束）
CronJob = 餐厅的定时任务（每天 11 点备菜、每周一进货）

【K8s 类比】

Deployment = 长期运行的服务
Job = 一次性的批处理任务
CronJob = 周期性的定时任务
```

---

## 2. Job 详解

### 2.1 基本 Job 配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
【YAML 示例】

apiVersion: batch/v1
kind: Job
metadata:
  name: my-job
spec:
  template:
    spec:
      containers:
      - name: my-container
        image: my-app:1.0
        command: ["python", "process.py"]
      restartPolicy: OnFailure   # 失败时重启容器，不重启 Pod

【参数说明】

• restartPolicy: OnFailure → 容器失败时重启容器
• restartPolicy: Never → 容器失败时完全不重启
• restartPolicy: Always → 仅用于 CronJob

【创建命令】

kubectl apply -f job.yaml

【查看 Job】

kubectl get jobs

【查看 Pod】

kubectl get pods -n <namespace> | grep my-job
```

### 2.2 并行执行

```
【场景】

你有 10000 条数据需要处理，单个任务太慢。
你希望同时跑 10 个任务并行处理。

【配置并行】

apiVersion: batch/v1
kind: Job
metadata:
  name: my-parallel-job
spec:
  parallelism: 10           # 同时运行 10 个 Pod
  completions: 100         # 总共需要完成 100 个任务
  backoffLimit: 3          # 失败重试次数
  template:
    spec:
      containers:
      - name: my-container
        image: my-app:1.0
        command: ["python", "process.py"]
      restartPolicy: OnFailure

【参数解释】

• parallelism: 10 → 同时运行 10 个 Pod
• completions: 100 → 需要完成 100 个任务才算成功
• backoffLimit: 3 → 失败最多重试 3 次
```

### 2.3 工作队列模式

```
【场景】

你有 N 个任务，但不知道具体有多少。
所有任务都放到一个队列里，Job 自动消费。

【配置】

apiVersion: batch/v1
kind: Job
metadata:
  name: queue-job
spec:
  parallelism: 5
  template:
    spec:
      containers:
      - name: worker
        image: my-worker:1.0
        command: ["python", "worker.py"]
      restartPolicy: OnFailure

【说明】

• 队列模式需要 worker 自己从队列获取任务
• Job 负责管理 worker 的生命周期
• 队列可以是 Redis、RabbitMQ 等
```

---

## 3. CronJob 详解

### 3.1 基本 CronJob 配置

```
【YAML 示例】

apiVersion: batch/v1
kind: CronJob
metadata:
  name: my-cronjob
spec:
  schedule: "0 2 * * *"        # 每天凌晨 2 点执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: my-backup:1.0
            command: ["python", "backup.py"]
            volumeMounts:
            - name: data
              mountPath: /data
          restartPolicy: OnFailure
          volumes:
          - name: data
            persistentVolumeClaim:
              claimName: backup-pvc

【 schedule 格式】

┌───────────── minute (0 - 59)
│ ┌───────────── hour (0 - 23)
│ │ ┌───────────── day of month (1 - 31)
│ │ │ ┌───────────── month (1 - 12)
│ │ │ │ ┌───────────── day of week (0 - 6) (Sunday to Saturday)
│ │ │ │ │
* * * * *

示例：
• "0 * * * *" → 每小时整点执行
• "0 2 * * *" → 每天凌晨 2 点执行
• "0 2 * * 0" → 每周日凌晨 2 点执行
• "0 */4 * * *" → 每 4 小时执行一次
```

### 3.2 并发策略

```
【场景】

定时任务执行时间过长，下一个执行时间到了，上一个还没结束怎么办？

【配置】

apiVersion: batch/v1
kind: CronJob
metadata:
  name: my-cronjob
spec:
  schedule: "0 2 * * *"
  concurrencyPolicy: Forbid   # 禁止并发运行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: task
            image: my-task:1.0
          restartPolicy: OnFailure

【并发策略】

• concurrencyPolicy: Allow → 允许并发运行（默认）
• concurrencyPolicy: Forbid → 跳过新执行，如果上一个还在运行
• concurrencyPolicy: Replace → 取消上一个，用新的替换
```

### 3.3 跳过和重试

```
【配置】

apiVersion: batch/v1
kind: CronJob
metadata:
  name: my-cronjob
spec:
  schedule: "0 2 * * *"
  startingDeadlineSeconds: 200  # 超过这个时间未执行，就算错过
  successfulJobsHistoryLimit: 3 # 只保留最近 3 个成功 Job
  failedJobsHistoryLimit: 1     # 只保留最近 1 个失败 Job
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: task
            image: my-task:1.0
          restartPolicy: OnFailure

【参数说明】

• startingDeadlineSeconds: 错过执行时间后的宽限期
• successfulJobsHistoryLimit: 成功 Job 历史保留数量
• failedJobsHistoryLimit: 失败 Job 历史保留数量
```

---

## 4. 常见问题

### 4.1 Job 失败排查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
【排查步骤】

1. 查看 Job 状态
   kubectl get jobs

   如果 FAILED 为 1，说明有任务失败了。

2. 查看 Job 详情
   kubectl describe job <job-name>

3. 查看 Pod 日志
   kubectl get pods -n <namespace> | grep <job-name>
   kubectl logs <pod-name> -n <namespace>

4. 检查失败原因
   常见原因：
   • 命令错误（command 写错）
   • 资源不足（OOM、CPU 超限）
   • 依赖服务不可用（数据库连不上）
   • 权限问题（无法访问某个资源）

5. 如果需要重试
   kubectl delete job <job-name>
   然后重新创建
```

### 4.2 CronJob 没有执行

```
【排查步骤】

1. 查看 CronJob 状态
   kubectl get cronjobs

2. 查看 CronJob 详情
   kubectl describe cronjob <name>

3. 检查最后执行时间
   kubectl get jobs --watch | grep <cronjob-name>

4. 检查调度是否被跳过
   如果 concurrencyPolicy: Forbid 且上一个还在运行，新的会被跳过。

5. 查看 CronJob 的 Events
   kubectl describe cronjob <name> | grep -A10 "Events"
```

### 4.3 Job 完成但 Pod 还在

```
【问题】

Job 显示完成（COMPLETED），但 Pod 没有被清理。

【原因】

Job 完成后，Pod 默认不会被删除，除非设置 ttlSecondsAfterFinished。

【配置自动清理】

apiVersion: batch/v1
kind: Job
metadata:
  name: my-job
spec:
  ttlSecondsAfterFinished: 100  # 完成后 100 秒自动删除
  template:
    spec:
      containers:
      - name: my-container
        image: my-app:1.0
      restartPolicy: OnFailure
```

---

## 5. 数字人 Q&A 场景

### 5.1 用户问：如何让任务定时执行？

```
【回复】

"好问题！定时任务用 CronJob 来实现。

【创建 CronJob】

apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-cronjob
spec:
  schedule: "0 2 * * *"    # 每天凌晨 2 点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: my-backup:1.0
            command: ["python", "backup.py"]
          restartPolicy: OnFailure

【常用 schedule 表达式】

• "0 * * * *" → 每小时
• "0 2 * * *" → 每天凌晨 2 点
• "0 2 * * 0" → 每周日凌晨 2 点
• "0 */4 * * *" → 每 4 小时
• "0 0 1 * *" → 每月 1 日凌晨

【查看 CronJob】

kubectl get cronjobs
kubectl describe cronjob <name>

有其他问题吗？"
```

### 5.2 用户问：任务执行失败怎么重试？

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
【回复】

"Job 失败重试有两种方式：

【方式一：自动重试（配置 backoffLimit）】

apiVersion: batch/v1
kind: Job
metadata:
  name: my-job
spec:
  backoffLimit: 3    # 失败后重试 3 次
  template:
    spec:
      containers:
      - name: my-container
        image: my-app:1.0
      restartPolicy: OnFailure

【方式二：手动重试】

删除失败的 Job，重新创建：
kubectl delete job <job-name>
kubectl apply -f job.yaml

【查看失败原因】

kubectl describe job <job-name>
kubectl logs <pod-name>

看看 Events 和日志，找出失败原因。

【常见失败原因】

1. 命令错误 → 检查 command 配置
2. 依赖不可用 → 检查数据库/API 连通性
3. 资源不足 → 增加 resources limits
4. 权限问题 → 检查 RBAC 配置

有其他问题吗？"
```

---

## 6. 总结

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
【命令速查】

创建 Job：
kubectl apply -f job.yaml

查看 Job：
kubectl get jobs
kubectl describe job <name>

查看 Job 的 Pod：
kubectl get pods -n <namespace> | grep <job-name>

删除 Job：
kubectl delete job <name>

创建 CronJob：
kubectl apply -f cronjob.yaml

查看 CronJob：
kubectl get cronjobs
kubectl describe cronjob <name>

删除 CronJob：
kubectl delete cronjob <name>

【Job vs CronJob】

| 特性 | Job | CronJob |
|------|-----|---------|
| 执行次数 | 一次 | 周期 |
| schedule | 无 | 有（cron 表达式）|
| 使用场景 | 批量处理、离线计算 | 定时备份、报表生成 |
| 并行执行 | 支持 | 通过 Job 实现 |

【配置要点】

• restartPolicy: 通常设为 OnFailure 或 Never
• 并行度：通过 parallelism 和 completions 控制
• 失败重试：通过 backoffLimit 控制
• 定时执行：用 cron 表达式（0 2 * * *）

【下节课预告】

下节课我们会学习健康检查（Probe）：
• LivenessProbe、ReadinessProbe、StartupProbe
• 如何配置健康检查
• 排查健康检查失败的问题

有问题吗？"
```

---

**关联文档**:
- [../09-troubleshooting/09-health-check.md](../09-troubleshooting/09-health-check.md) — 健康检查
- [../../domain-10-troubleshooting-diagnostics/topic-skills/11-job-cronjob-failure.md](../../domain-10-troubleshooting-diagnostics/topic-skills/11-job-cronjob-failure.md) — Job/CronJob 问题 [[SKILL|Skill]]
- [../../domain-02-workloads-applications/](../../domain-02-workloads-applications/) — 工作负载文档

## See Also

- 09-hpa-basics
- 10-health-check
- 12-common-problems
- 13-daemonset-basics
