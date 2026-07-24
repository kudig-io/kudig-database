---
title: Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation (skills)
description: '| S2 | CronJob 未创建新 Job | `kubectl get jobs` 按时间检查 | 0.90 | 未到调度时间 |'
summary: '| S2 | CronJob 未创建新 Job | `kubectl get jobs` 按时间检查 | 0.90 | 未到调度时间 |'
category: skills
tags:
- k8s
- troubleshooting
- skill
- job
- cronjob
- job failed
- cronjob not running
- 定时任务
- statefulset
- rbac
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation 是什么
- 如何 Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation
trigger_keywords:
- Job
- CronJob
- 故障诊断与修复
- Job
- CronJob
- Failure
- Diagnosis
- Remediation
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Job/CronJob 故障诊断与修复 / Job & [[CronJob|CronJob]] Failure Diagnosis & Remediation

### 症状识别



### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | Job 状态 Failed | `kubectl get job` | 0.95 | 无 |
| S2 | CronJob 未创建新 Job | `kubectl get [[Jobs|jobs]]` 按时间检查 | 0.90 | 未到调度时间 |
| S3 | Job Pod 退出码非零 | `kubectl get [[Pods|pods]] --selector=job-name` | 0.90 | 应用自身 bug |
| S4 | 历史 Job 大量堆积 | `kubectl get jobs | wc -l` | 0.85 | 无 |
| S5 | Job 长时间 Running | `kubectl get job` active 时间 | 0.85 | 任务本身耗时 |
| S6 | CronJob 错过调度 | `kubectl describe cronjob` Events | 0.90 | 时区差异 |
| S7 | Job 重试次数耗尽 | `kubectl describe job` Events | 0.90 | 无 |
| S8 | 并行 Job 数量异常 | `kubectl get pods --selector=job-name` | 0.80 | 无 |

### 诊断工作流



### Phase 1: 快速检查（只读，零风险）

**Step D1.1**: 获取 Job/CronJob 概览
- **命令**:
  ```bash
  # Job
  kubectl get job <name> -n <namespace> -o wide
  kubectl describe job <name> -n <namespace> | head -40
  # CronJob
  kubectl get cronjob <name> -n <namespace> -o wide
  kubectl describe cronjob <name> -n <namespace> | head -40
  ```
- **超时**: 10s
- **判断规则**:
  - Job: succeeded < completions → RC-001/005/006/008
  - Job: active > 0 且持续时间 > activeDeadlineSeconds → RC-007（超时）
  - CronJob: lastScheduleTime 为空或很久之前 → RC-002/003
  - CronJob: active Jobs 数量 > 1 且 concurrencyPolicy=Forbid → RC-004

**Step D1.2**: 检查 Job 关联 Pod 状态
- **命令**:
  ```bash
  kubectl get pods -n <namespace> --selector=job-name=<job-name> \
    -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,READY:.status.containerStatuses[*].ready,RESTARTS:.status.containerStatuses[*].restartCount,EXIT:.status.containerStatuses[*].state.terminated.exitCode
  ```
- **超时**: 10s
- **判断规则**:
  - Pod 状态 Error + exitCode != 0 → RC-005（应用执行失败）
  - Pod 状态 CrashLoopBackOff → RC-001（启动失败）
  - Pod 状态 Running 但长时间不结束 → RC-007（执行超时）

**Step D1.3**: 检查 Job 配置参数
- **命令**:
  ```bash
  kubectl get job <name> -n <namespace> -o jsonpath='{
    "completions": .spec.completions,
    "parallelism": .spec.parallelism,
    "backoffLimit": .spec.backoffLimit,
    "ttlSecondsAfterFinished": .spec.ttlSecondsAfterFinished,
    "activeDeadlineSeconds": .spec.activeDeadlineSeconds
  }' | jq .
  ```
- **超时**: 10s
- **判断规则**:
  - backoffLimit 过低（如 0 或 1）→ RC-006（重试不足）
  - activeDeadlineSeconds 过短 → RC-007（超时阈值不合理）
  - completions > 1 但 parallelism = 1 → 执行慢（可能正常）
  - ttlSecondsAfterFinished 未设置 → RC-009（无自动清理）

**Step D1.4**: 检查 CronJob 调度配置
- **命令**:
  ```bash
  kubectl get cronjob <name> -n <namespace> -o jsonpath='{
    "schedule": .spec.schedule,
    "timezone": .spec.timeZone,
    "concurrencyPolicy": .spec.concurrencyPolicy,
    "startingDeadlineSeconds": .spec.startingDeadlineSeconds,
    "successfulJobsHistoryLimit": .spec.successfulJobsHistoryLimit,
    "failedJobsH
...(截断)

### Phase 2: 深度检查（只读，零风险）

**Step D2.1**: 分析 Job Pod 退出原因
- **命令**:
  ```bash
  kubectl logs -n <namespace> <job-pod> --tail=50
  kubectl get pod <job-pod> -n <namespace> -o jsonpath='{.status.containerStatuses[0].state.terminated}' | jq .
  kubectl get pod <job-pod> -n <namespace> -o jsonpath='{.status.containerStatuses[0].lastState.terminated}' | jq .
  ```
- **超时**: 15s
- **判断规则**:
  - 日志显示应用错误 → RC-005（应用逻辑错误）
  - exitCode 137 → 被 OOMKilled 或手动终止
  - exitCode 143 → SIGTERM 优雅终止
  - exitCode 1/2 → 应用自身错误
  - 无日志但 exitCode != 0 → 启动即崩溃

**Step D2.2**: 检查 Job 事件历史
- **命令**:
  ```bash
  kubectl get events -n <namespace> --field-s

--- (内容截断，完整内容见源文件) ---

## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## 生产案例

### 案例 1: CronJob 任务堆积导致资源耗尽

| 时间 | 事件 |
|------|------|
| 06:00 | 多个 CronJob Pod 同时运行，节点资源耗尽 |
| 06:05 | `kubectl get pods -l job-name` 显示 20+ 个 Running Pod |
| 06:08 | 前一任务未完成，新任务又启动(concurrencyPolicy=Allow) |
| 06:10 | 🟡 设置 concurrencyPolicy=Forbid + startingDeadlineSeconds |

**根因**: CronJob 未设置并发策略，任务执行时间超过调度间隔导致堆积。

### 案例 2: Job 失败后未重试导致数据未处理

**现象**: 数据处理 Job 失败后无重试，数据积压。

**诊断**: `backoffLimit` 默认为 6，但 Pod 错误类型为不可重试(OOMKilled)

**修复**: 🟢 增加 memory limit + 设置 restartPolicy=OnFailure

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 关键定时任务失败 | 手动触发 + 检查原因 |
| P1 | 任务堆积 | 调整并发策略 |
| P2 | 任务性能优化 | 调整资源和超时 |

## 面试要点

1. **Q: Job 与 CronJob 的区别？**
   A: Job: 一次性任务，运行到成功完成；CronJob: 定时调度 Job，使用 cron 表达式。Job 通过 completions/parallelism 控制并行度，CronJob 通过 concurrencyPolicy 控制并发。

2. **Q: CronJob 的 concurrencyPolicy 选项？**
   A: Allow(默认): 允许并发；Forbid: 前一任务未完成则跳过；Replace: 取消前一任务启动新任务。生产推荐 Forbid 避免资源堆积。

3. **Q: Job 失败重试机制？**
   A: backoffLimit(默认 6) 控制重试次数，每次重试间隔指数增长(10s/20s/40s...)。activeDeadlineSeconds 控制总超时。Pod restartPolicy 必须为 OnFailure 或 Never。

## Related

- [[rbac-fta]] — RBAC 异常故障树分析
- [[技能/工作负载/statefulset/skill-21-statefulset-failure.md|skill-21-statefulset-failure]] — StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation
- [[技能/节点/node/诊断排障/troubleshoot-node-issues.md|troubleshoot-node-issues]] — Troubleshoot Node Issues
- [[技能/工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]] — FTA Diagnostic Execution Engine
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
