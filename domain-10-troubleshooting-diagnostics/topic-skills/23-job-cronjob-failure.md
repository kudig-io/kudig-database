---
title: Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation
description: '- 运维工程师'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- etcd
- controller-manager
- statefulset
- daemonset
- job
- cronjob
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation 是什么
- 如何 Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation 故障排查
- Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation 排障步骤
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
- troubleshooting-methodology
- etcd-basics
skill_id: SKILL-23_JOB_CRONJOB_FAILURE-001
skill_name: Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation
version: 1.0.0
created: "2026-05-23"
---

---
skill_id: "SKILL-WORK-004"
skill_name: "Job/CronJob 故障诊断与修复 / Job & [[CronJob|CronJob]] Failure Diagnosis & Remediation"
version: "1.0"
category: "workload"
severity_range: "P0-P2"
k8s_versions:
  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"
estimated_resolution_time: "10-45min"
risk_level: "medium"
agent_execution_mode: "L2-semi-auto"
trigger_keywords:
  - "Job"
  - "CronJob"
  - "job failed"
  - "cronjob not running"
  - "定时任务"
  - "批处理"
  - "batch job"
  - "exit code"
  - "backoff"
  - "missed schedule"
  - "ttl"
  - "parallelism"
trigger_events:
  - "BackoffLimitExceeded"
  - "DeadlineExceeded"
  - "SuccessfulCreate"
  - "MissSchedule"
  - "JobAlreadyActive"
trigger_metrics:
  - 'kube_job_status_failed'
  - 'kube_cronjob_status_active'
  - 'kube_job_status_active'
  - 'kube_job_spec_completions - kube_job_status_succeeded'
difficulty: "intermediate"
reading_level: "intermediate"
audience:
  - SRE
  - 运维工程师
  - 技术支持
estimated_read_time: "12min"
prerequisites:
  - "domain-02-workloads-applications"
  - "kubectl-basics"
related_skills:
  - "SKILL-POD-001"
  - "SKILL-POD-002"
  - "SKILL-IMAGE-001"
  - "SKILL-NODE-002"
fta_refs:
  - "domain-10-troubleshooting-diagnostics/topic-fta/list/job-cronjob-fta.md"
knowledge_refs:
  - "domain-10-troubleshooting-diagnostics/18-cronjob-troubleshooting.md"
  - "domain-10-troubleshooting-diagnostics/22-job-troubleshooting.md"
  - "domain-02-workloads-applications/"
cross_refs:
  - type: "fta"
    path: "../domain-10-troubleshooting-diagnostics/topic-fta/list/job-cronjob-fta.md"
    label: "Job/CronJob 故障树分析"
  - type: "domain"
    path: "../domain-10-troubleshooting-diagnostics/18-cronjob-troubleshooting.md"
    label: "CronJob 深度排查"
  - type: "domain"
    path: "../domain-10-troubleshooting-diagnostics/22-job-troubleshooting.md"
    label: "Job 深度排查"
authors:
  - name: KUDIG Team
    role: contributor

tier: peripheral---

# Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation

Job 和 CronJob 是 [[entities/kubernetes|[[Kubernetes|kubernetes]]]] 批处理工作负载的核心控制器。Job 用于一次性任务执行，CronJob 用于定时触发 Job。它们的故障模式与长期运行的工作负载（Deployment/StatefulSet/DaemonSet）有显著差异：任务完成后 Pod 会终止、CronJob 的调度依赖控制器时间同步、Job 的完成条件涉及并行度和成功计数、历史 Job 的清理依赖 TTL 机制。

本 [[SKILL|Skill]] 覆盖 Job 执行失败、CronJob 未触发、错过调度、并发控制问题、重试耗尽、历史堆积、时区偏差等 10 种根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| Job 状态为 Failed | `kubectl get job` | 0.95 |
| CronJob 未按预期创建 Job | `kubectl get jobs -l managed-by-cronjob` | 0.90 |
| Job Pod 退出码非零 | `kubectl get pods --selector=job-name` | 0.90 |
| 历史 Job/Pod 大量堆积 | `kubectl get jobs` 数量异常 | 0.85 |
| CronJob 错过调度时间 | `kubectl describe cronjob` 查看 Events | 0.90 |
| Job 长时间 Running 不结束 | `kubectl get job` active 时间 | 0.85 |

**排除条件**: 通用 Pod CrashLoop → SKILL-POD-001; 镜像拉取失败 → SKILL-IMAGE-001; 调度失败 → SKILL-POD-002; 节点资源压力 → SKILL-NODE-002

## 快速分级（2 分钟内完成）

```
任务关键度 + 影响范围
├── 关键批处理任务失败（如数据清洗、对账）──────→ P0（立即处理）
├── CronJob 长时间未触发（如备份任务）───────────→ P0（30min 内修复）
├── 非关键批处理任务失败─────────────────────────→ P1（1h 内修复）
├── 历史 Job 堆积导致资源泄漏─────────────────────→ P1（2h 内修复）
├── Job 执行超时─────────────────────────────────→ P2（4h 内修复）
└── 定时任务时区偏差─────────────────────────────→ P2（下次维护窗口）
```

**立即升级条件**：
- 关键定时任务（如数据库备份、日志归档）长时间未执行
- Job 失败导致数据不一致或丢失
- 历史 Job 堆积导致 API Server/etcd 性能问题

## 执行流程

```
工单/告警触发
    │
    ▼
┌──────────────┐    Step: D1.1-D1.5
│ Phase 1      │    内容: kubectl 快速检查（只读，零风险）
│ 快速检查      │
└──────┬───────┘
       │ 无法确认根因
       ▼
┌──────────────┐    Step: D2.1-D2.6
│ Phase 2      │    内容: 深度分析（只读，零风险）
│ 深度检查      │
└──────┬───────┘
       │ 需主动探测/修复
       ▼
┌──────────────┐    Step: D3.1-D3.3
│ Phase 3      │    内容: 主动探测（低风险，可能需审批）
│ 主动探测      │
└──────┬───────┘
       │ 确认根因
       ▼
┌──────────────┐    RC-001~010
│ 根因匹配      │
└──────┬───────┘
       │
       ▼
┌──────────────┐    REM-001~008
│ 修复操作      │    风险: LOW → MEDIUM → HIGH → CRITICAL
└──────┬───────┘
       │
       ▼
┌──────────────┐    V1~V6
│ 验证确认      │
└──────────────┘
```

## 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | Job 状态 Failed | `kubectl get job` | 0.95 | 无 |
| S2 | CronJob 未创建新 Job | `kubectl get jobs` 按时间检查 | 0.90 | 未到调度时间 |
| S3 | Job Pod 退出码非零 | `kubectl get pods --selector=job-name` | 0.90 | 应用自身 bug |
| S4 | 历史 Job 大量堆积 | `kubectl get jobs | wc -l` | 0.85 | 无 |
| S5 | Job 长时间 Running | `kubectl get job` active 时间 | 0.85 | 任务本身耗时 |
| S6 | CronJob 错过调度 | `kubectl describe cronjob` Events | 0.90 | 时区差异 |
| S7 | Job 重试次数耗尽 | `kubectl describe job` Events | 0.90 | 无 |
| S8 | 并行 Job 数量异常 | `kubectl get pods --selector=job-name` | 0.80 | 无 |

### 2.2 工单关键词映射

- "定时任务没有执行"
- "Job 失败了，exit code 1"
- "CronJob 没有按时创建 Job"
- "历史 Job 太多了，没清理"
- "批处理任务跑了很久没结束"
- "Job 重试了 6 次还是失败"
- "CronJob 调度时间不对"
- "并行 Job 太多了，资源不够"

### 2.3 排除标准

- 通用 Pod 启动失败 → 使用 SKILL-POD-001/002
- 镜像拉取失败 → 使用 SKILL-IMAGE-001
- 节点资源压力导致 Pod 驱逐 → 使用 SKILL-NODE-002
- 应用业务逻辑错误 → 不在本 Skill 范围（需开发者介入）

## 快速分级（2 分钟内完成）

### 3.1 影响评估

**Step T1**: 检查 Job/CronJob 整体状态
```bash
# Job 状态
kubectl get job <name> -n <namespace> -o jsonpath='{
  "succeeded": .status.succeeded,
  "failed": .status.failed,
  "active": .status.active,
  "conditions": .status.conditions
}' | jq .
# CronJob 状态
kubectl get cronjob <name> -n <namespace> -o jsonpath='{
  "lastSchedule": .status.lastScheduleTime,
  "active": .status.active
}' | jq .
```
> **判断规则**: failed > 0 或 active 长时间不结束 → 有问题

**Step T2**: 检查 Job 历史堆积
```bash
kubectl get jobs -n <namespace> | wc -l
kubectl get jobs -n <namespace> --sort-by=.metadata.creationTimestamp | head -5
```
> **判断规则**: Job 数量 > 100（或异常多）→ RC-009（历史堆积）

**Step T3**: 检查 CronJob 调度时间
```bash
kubectl get cronjob <name> -n <namespace> -o jsonpath='{.spec.schedule}'
kubectl get cronjob <name> -n <namespace> -o jsonpath='{.status.lastScheduleTime}'
date -u
```
> **判断规则**: lastScheduleTime 远早于当前时间 → RC-002/003（未触发/错过调度）

**Step T4**: 检查最近 Events
```bash
kubectl get events -n <namespace> --field-selector involvedObject.kind=Job --sort-by=.lastTimestamp | tail -15
kubectl get events -n <namespace> --field-selector involvedObject.kind=CronJob --sort-by=.lastTimestamp | tail -15
```
> **判断规则**: BackoffLimitExceeded → RC-006; MissSchedule → RC-003; JobAlreadyActive → RC-004

### 3.2 严重性分级

| 条件 | 级别 | 说明 |
|------|------|------|
| 关键定时任务失败/未触发 | P0 | 15min 内修复 |
| 批处理任务失败导致数据问题 | P0 | 30min 内修复 |
| 非关键 Job 失败 | P1 | 1h 内修复 |
| 历史 Job 堆积 | P1 | 2h 内修复 |
| Job 执行超时 | P2 | 4h 内修复 |
| 时区偏差 | P2 | 下次维护窗口 |

### 3.3 立即升级触发条件

- 关键备份/归档任务失败
- 数据对账/清洗任务失败导致数据不一致
- CronJob 控制器异常导致所有定时任务未触发
- 历史 Job 堆积影响集群性能

## 诊断工作流

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
    "failedJobsHistoryLimit": .spec.failedJobsHistoryLimit
  }' | jq .
  ```
- **超时**: 10s
- **判断规则**:
  - schedule 表达式格式错误 → RC-002
  - timezone 设置与实际需求不符 → RC-003（时区偏差）
  - concurrencyPolicy=Forbid 且前一个 Job 未结束 → RC-004
  - startingDeadlineSeconds 过短 → RC-003（错过调度窗口）

**Step D1.5**: 检查 Controller Manager 状态和时间
- **命令**:
  ```bash
  kubectl get pods -n kube-system -l component=kube-controller-manager
  # 检查 CronJob 控制器日志中的时间相关信息
  kubectl logs -n kube-system <kube-controller-manager-pod> | \
    grep -iE 'cronjob|CronJob' | tail -10
  ```
- **超时**: 15s
- **判断规则**:
  - Controller Manager Pod 不健康 → RC-010（控制器异常）
  - 日志显示 `Missed schedule` → RC-003
  - 日志显示 `Too many missed start times` → RC-003

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
  kubectl get events -n <namespace> --field-selector involvedObject.name=<job-name> \
    --sort-by=.lastTimestamp
  ```
- **超时**: 10s
- **判断规则**:
  - `BackoffLimitExceeded` → RC-006（重试耗尽）
  - `DeadlineExceeded` → RC-007（超时）
  - `FailedCreate` → RC-001（创建失败）
  - `SuccessfulCreate` + 后续失败 → 创建成功但执行失败

**Step D2.3**: 验证 CronJob schedule 表达式
- **命令**:
  ```bash
  # 使用在线工具或脚本验证 cron 表达式
  SCHEDULE=$(kubectl get cronjob <name> -n <namespace> -o jsonpath='{.spec.schedule}')
  echo "Schedule: $SCHEDULE"
  # 解析 cron 表达式（5 字段格式）
  # 分 时 日 月 周
  ```
- **超时**: 5s
- **判断规则**:
  - 表达式格式错误（如 6 个字段而非 5 个）→ RC-002
  - 表达式设置的时间不可能到达（如 2月30日）→ RC-002
  - 步长值不合理（如 */0）→ RC-002

**Step D2.4**: 检查历史 Job 堆积情况
- **命令**:
  ```bash
  # 统计所有历史 Job
  kubectl get jobs -n <namespace> | wc -l
  # 按 CronJob 分组统计
  kubectl get jobs -n <namespace> -o json | jq -r '
    .items[] | select(.metadata.ownerReferences[0].kind == "CronJob") |
    .metadata.ownerReferences[0].name'
    | sort | uniq -c | sort -rn | head -10
  # 检查 Pod 数量
  kubectl get pods -n <namespace> | grep -E 'Completed|Error' | wc -l
  ```
- **超时**: 10s
- **判断规则**:
  - Job 数量 > 100 且无 TTL → RC-009（历史堆积）
  - Completed Pod 大量堆积 → RC-009

**Step D2.5**: 检查资源使用和竞争
- **命令**:
  ```bash
  kubectl describe job <name> -n <namespace> | grep -A 10 "Events"
  kubectl top pod -n <namespace> --selector=job-name=<job-name> 2>/dev/null
  # 检查节点资源
  kubectl get pod <job-pod> -n <namespace> -o jsonpath='{.spec.nodeName}'
  kubectl describe node <node> | grep -A 10 "Allocated resources"
  ```
- **超时**: 15s
- **判断规则**:
  - `FailedScheduling` 事件 → RC-001（资源不足）
  - 节点 CPU/内存耗尽 → RC-008（资源竞争）

**Step D2.6**: 检查并发 Job 状态
- **命令**:
  ```bash
  # 获取由同一 CronJob 创建的活跃 Job
  kubectl get jobs -n <namespace> -o json | jq -r '
    .items[] | select(.status.active > 0 and .metadata.ownerReferences[0].name == "<cronjob-name>") |
    .metadata.name'
  # 检查并行 Pod 数量
  kubectl get pods -n <namespace> --selector=job-name=<job-name> --field-selector=status.phase=Running
  ```
- **超时**: 10s
- **判断规则**:
  - 活跃 Job 数量 > 1 且 concurrencyPolicy=Forbid → RC-004
  - 并行 Pod 数量 > parallelism 设置 → RC-004

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: 手动触发 Job 执行测试
- **命令**:
  ```bash
  # 从 CronJob 创建一次性 Job
  kubectl create job --from=cronjob/<cronjob-name> <test-job-name> -n <namespace>
  ```
- **超时**: 30s
- **风险级别**: 🟢 低（创建测试 Job，不影响现有任务）
- **判断规则**:
  - 手动触发成功执行 → CronJob 调度问题（RC-002/003/010）
  - 手动触发也失败 → Job 模板问题（RC-001/005/006/007）

**Step D3.2**: 临时增加 backoffLimit 测试
- **命令**:
  ```bash
  kubectl patch job <name> -n <namespace> -p '{"spec":{"backoffLimit":10}}'
  ```
- **超时**: 10s
- **风险级别**: 🟡 中（增加重试次数可能消耗更多资源）
- **判断规则**:
  - 增加后成功 → RC-006（重试不足）
  - 增加后仍失败 → RC-005（应用逻辑错误）

**Step D3.3**: 清理历史 Job 释放资源
- **命令**:
  ```bash
  # 删除所有 Completed Job（保留最近 10 个）
  kubectl get jobs -n <namespace> --field-selector status.successful=1 \
    --sort-by=.status.completionTime | tail -n +11 | awk '{print $1}' | \
    xargs -r kubectl delete job -n <namespace>
  ```
- **超时**: 15s
- **风险级别**: 🟡 中（删除历史 Job，丢失执行记录）
- **判断规则**:
  - 清理后新 Job 可正常创建 → RC-009（历史堆积导致）

## 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | Job Pod 启动失败（调度/镜像/权限） | 高 | D1.2 Pod Pending/Error；D2.1 启动日志 | job_pod_startup_failure |
| RC-002 | CronJob schedule 表达式错误 | 高 | D1.4 schedule 格式错误；D2.3 验证失败 | cron_schedule_invalid |
| RC-003 | CronJob 错过调度（时区/控制器延迟） | 中 | D1.4 timezone 设置；D1.5 MissSchedule 事件；D2.3 时间偏差 | missed_schedule |
| RC-004 | 并发控制问题（concurrencyPolicy/parallelism） | 中 | D1.1 active Jobs 多；D2.6 并发验证；D1.4 concurrencyPolicy | concurrency_misconfig |
| RC-005 | 应用执行失败（退出码非零） | 高 | D1.2 exitCode != 0；D2.1 应用错误日志 | application_execution_failure |
| RC-006 | 重试次数耗尽（backoffLimit 过低） | 中 | D1.3 backoffLimit 值；D2.2 BackoffLimitExceeded；D3.2 验证 | backoff_exhausted |
| RC-007 | Job 执行超时（activeDeadlineSeconds） | 中 | D1.1 active 时间长；D1.3 deadline 设置；D2.2 DeadlineExceeded | execution_timeout |
| RC-008 | 资源竞争（并行 Job 过多/节点资源不足） | 低 | D2.5 资源耗尽；D2.6 并行 Pod 多 | resource_contention |
| RC-009 | 历史 Job/Pod 堆积（无 TTL 或 TTL 过长） | 中 | D2.4 Job 数量异常；D1.3 ttl 未设置 | history_accumulation |
| RC-010 | CronJob 控制器异常（Controller Manager 问题） | 低 | D1.5 Controller 不健康；D3.1 手动触发正常 | controller_malfunction |

## 修复操作

### 6.1 🟢 低风险

#### REM-001: 修正 CronJob schedule 表达式
- **适用根因**: RC-002
- **前置检查**: D1.4/D2.3 确认表达式错误
- **执行**:
  ```bash
  kubectl patch cronjob <name> -n <namespace> -p \
    '{"spec":{"schedule":"<correct-cron-expression>"}}'
  ```
- **后置验证**: `kubectl get cronjob <name> -n <namespace> -o jsonpath='{.spec.schedule}'`
- **回滚**: 恢复原始 schedule 表达式

#### REM-002: 添加或修正 timezone 配置
- **适用根因**: RC-003
- **前置检查**: D1.4 timezone 与实际需求不符
- **执行**:
  ```bash
  kubectl patch cronjob <name> -n <namespace> -p \
    '{"spec":{"timeZone":"Asia/Shanghai"}}'
  ```
- **后置验证**: 观察下次调度时间是否符合预期
- **回滚**: 移除或恢复 timezone 设置
- **版本差异**: **[v1.24+]**: timeZone 字段支持（需 CronJobTimeZone feature gate，v1.25+ GA）

#### REM-003: 设置 TTL 自动清理
- **适用根因**: RC-009
- **前置检查**: D1.3/D2.4 确认无 TTL 且历史堆积
- **执行**:
  ```bash
  # 修改 CronJob 模板添加 TTL
  kubectl patch cronjob <name> -n <namespace> -p \
    '{"spec":{"jobTemplate":{"spec":{"ttlSecondsAfterFinished":3600}}}}'
  # 或修改 Job 直接添加 TTL
  kubectl patch job <name> -n <namespace> -p \
    '{"spec":{"ttlSecondsAfterFinished":3600}}'
  ```
- **后置验证**: 等待 Job 完成后检查是否自动删除
- **回滚**: 移除 TTL 设置

#### REM-004: 增加 backoffLimit
- **适用根因**: RC-006
- **前置检查**: D1.3/D2.2 确认重试耗尽
- **执行**:
  ```bash
  kubectl patch job <name> -n <namespace> -p '{"spec":{"backoffLimit":10}}'
  ```
- **后置验证**: 观察 Job 重试和最终状态
- **回滚**: 恢复原始 backoffLimit

### 6.2 🟡 中风险

#### REM-005: 调整 concurrencyPolicy
- **适用根因**: RC-004
- **审批提示**: "建议修改 CronJob <name> 的并发策略从 Forbid 改为 Replace/Allow。是否批准？"
- **执行**:
  ```bash
  kubectl patch cronjob <name> -n <namespace> -p \
    '{"spec":{"concurrencyPolicy":"Replace"}}'
  ```
- **后置验证**: 观察后续调度是否正常
- **回滚**: 恢复原始 concurrencyPolicy

#### REM-006: 调整 activeDeadlineSeconds
- **适用根因**: RC-007
- **审批提示**: "建议增加 Job <name> 的执行超时时间。是否批准？"
- **执行**:
  ```bash
  kubectl patch job <name> -n <namespace> -p \
    '{"spec":{"activeDeadlineSeconds":<new-timeout>}}'
  ```
- **后置验证**: 观察 Job 是否能在新超时内完成
- **回滚**: 恢复原始超时设置

#### REM-007: 手动清理历史 Job
- **适用根因**: RC-009
- **审批提示**: "建议清理 namespace <ns> 中的历史 Job，共 <count> 个。是否批准？"
- **执行**:
  ```bash
  # 删除所有成功的历史 Job
  kubectl delete jobs -n <namespace> --field-selector status.successful=1
  # 或保留最近 N 个
  kubectl get jobs -n <namespace> --field-selector status.successful=1 \
    --sort-by=.status.completionTime | tail -n +<N+1> | awk '{print $1}' | \
    xargs -r kubectl delete job -n <namespace>
  ```
- **后置验证**: `kubectl get jobs -n <namespace> | wc -l`
- **回滚**: 无法回滚（历史 Job 已删除）

### 6.3 🔴 高风险

#### REM-008: 强制删除卡住的 Job
- **适用根因**: RC-004/007/010
- **操作步骤**:
  1. 删除 Job（级联删除 Pod）
     ```bash
     kubectl delete job <name> -n <namespace> --cascade=foreground
     ```
  2. 如 Job 删除卡住，强制删除
     ```bash
     kubectl patch job <name> -n <namespace> -p '{"metadata":{"finalizers":[]}}' --type=merge
     kubectl delete job <name> -n <namespace> --force
     ```
  3. 重新创建 Job 或等待 CronJob 下次调度
- **安全检查**: 确认 Job 可安全中断
- **回滚**: 重新创建 Job

### 6.4 ⚫ 严重

#### REM-009: 重启 Controller Manager
- **适用根因**: RC-010
- **审批要求**: 高级 SRE
- **操作步骤**:
  1. 确认 Controller Manager 异常
  2. 重启 Controller Manager Pod（托管集群需云厂商操作）
  3. 验证所有 CronJob 恢复调度
- **回滚**: 无

## 验证确认

### 7.1 即时验证

```bash
# V1: Job 状态
kubectl get job <name> -n <namespace>
# 预期: succeeded == completions（对于一次性 Job）

# V2: CronJob 调度
kubectl get cronjob <name> -n <namespace>
# 预期: lastScheduleTime 接近当前时间

# V3: 新 Job 创建（对于 CronJob）
kubectl get jobs -n <namespace> -l cronjob.kubernetes.io/is-created-by=<cronjob-name>
# 预期: 有新的 Job 创建

# V4: Pod 成功完成
kubectl get pods -n <namespace> --selector=job-name=<job-name>
# 预期: Completed 或 Running 正常
```

### 7.2 短期监控

| 监控项 | 指标 | 预期 | 异常 |
|-------|------|------|------|
| Job 成功率 | `kube_job_status_succeeded` | =1 | =0 |
| CronJob 调度 | `kube_cronjob_status_lastScheduleTime` | 接近当前 | 滞后 |
| Job 堆积 | `kube_job_info` 数量 | 稳定 | 持续增长 |

### 7.3 解决确认标准

- [ ] Job 成功完成（succeeded == completions）
- [ ] CronJob 按 schedule 正常创建 Job
- [ ] 无 BackoffLimitExceeded 事件
- [ ] 无 DeadlineExceeded 事件（或超时阈值合理）
- [ ] 历史 Job 数量稳定（有 TTL 自动清理）

## 升级协议

- **升级条件**: 关键任务失败、所有 CronJob 未触发、控制器异常、诊断超时 30min
- **升级消息**: 包含 Job/CronJob 名称、失败次数、影响任务、最近 Events

## 版本兼容矩阵

| 功能 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| CronJob timeZone | GA | GA | GA | GA | GA |
| CronJob schedule substitution | - | - | - | - | alpha |
| Job backoffLimitPerIndex | beta | beta | GA | GA | GA |
| Job podFailurePolicy | GA | GA | GA | GA | GA |
| Job successPolicy | - | - | alpha | beta | beta |

## 知识进化

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| 将时区问题误判为未触发 | CronJob 没按时执行 | timezone 设置为 UTC | 检查时区配置 |
| 将正常重试误判为问题 | Job 多次失败 | backoffLimit 内正常重试 | 等待 backoffLimit 耗尽 |
| 将长时间任务误判为超时 | Job Running 很久 | 任务本身耗时 | 检查 activeDeadlineSeconds |

## 云厂商特异性

| 平台 | 差异 | 备注 |
|------|------|------|
| ACK | 托管 Controller Manager | 无法直接重启 |
| EKS | 托管 Controller Manager | 通过节点组管理 |
| GKE | Autopilot 限制 Job 资源 | 注意资源上限 |
| AKS | 托管 Controller Manager | 通过节点池管理 |

## 自动化集成接口

```bash
./scripts/diagnose-job-quick.sh --job <NAME> --namespace <NS>
./scripts/diagnose-cronjob-quick.sh --cronjob <NAME> --namespace <NS>
./scripts/verify-job.sh --job <NAME> --namespace <NS>
```

---

*文档版本: 1.0*  
*Skill ID: SKILL-WORK-004*  
*创建时间: 2026-05*  
*维护者: Kudig Team*
