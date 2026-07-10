---
title: CronJob 执行失败：历史任务挂起导致新调度被跳过
description: 专有云 ACK 定时 ETL 任务因 concurrencyPolicy=Forbid 且旧 Job 长时间未结束，后续调度被跳过，造成数据缺失的工单闭环样本。
summary: 专有云 ACK 定时 ETL 任务因 concurrencyPolicy=Forbid 且旧 Job 长时间未结束，后续调度被跳过，造成数据缺失的工单闭环样本。
category: production-operations
tags:
- ack
- zyy
- cronjob
- job
- etl
- concurrency
- p1
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:30:00+08:00'
incident_id: TC-2026-034
priority: P1
severity: high
affected_cluster: ack-zyy-prod-01
affected_namespace: etl
ticket_type: CronJob 执行失败
skill_ref:
- '[[工作负载/核心工作负载/05-job-cronjob-advanced.md|Job/CronJob
  进阶]]'
- '[[发布变更/GitOps/99-argo-cd-gitops-guide.md|GitOps
  变更管理]]'
fta_ref:
- '[[故障诊断/FTA故障树/list/job-cronjob-fta.md|FTA:
  CronJob 执行失败]]'
- '[[故障诊断/FTA故障树/list/deployment-fta.md|FTA: 工作负载发布失败]]'
last_updated: 2026-06-26 16:30:00+08:00
duplicate_of: INC-2026-ACK-049
status: duplicate
duplication_reason: 与 "INC-2026-ACK-049" 主题重复，内容角度相似，降低 RAG 权重
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- CronJob 执行失败：历史任务挂起导致新调度被跳过 如何处理
trigger_keywords:
- ack
- zyy
- cronjob
- job
- etl
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
relationships:
- target: '[[概念/cronjob.md]]'
  type: related_to
- target: '[[生产运维/工单案例/ticket-case-049-job-cronjob-execution-failure.md]]'
  type: related_to
- target: '[[生产运维/工单案例/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户反馈 `etl` 命名空间下的每日报表 CronJob `daily-report` 已连续 3 天未生成结果，线上数据看板出现空窗。客户描述如下：

> “我们的 daily-report CronJob 应该是每天凌晨 2 点跑，但最近三天都没数据。kubectl get cronjob 看到 LAST SCHEDULE 是三天前的，SUSPEND 也是 False。describe cronjob 没看出什么异常。我手动用 `kubectl create job --from=cronjob/daily-report` 创建了一个 Job，它跑了一会就变成 Completed 了。为什么自动调度没触发？麻烦查一下。”

该集群为专有云 `ack-zyy-prod-01`，`etl` 命名空间承担离线报表、数据清洗等定时任务。

## 分类与优先级判定

- **工单类型**：CronJob 执行失败 / 调度跳过。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境连续 3 天未生成报表，影响运营数据看板与下游数据消费。
2. 手动触发可成功，说明镜像与脚本本身正常，问题集中在 CronJob 调度策略。
3. 需在 15 分钟内定位为何自动调度未触发并给出修复方案。

## 诊断步骤

按“先看 CronJob 状态、再看 Job/Pod 历史、最后看 controller-manager 日志”的顺序排查：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 查看 CronJob 配置与调度历史
kubectl get cronjob daily-report -n etl -o yaml
kubectl describe cronjob daily-report -n etl | tail -40

# 2. 列出该 CronJob 创建的所有 Job
kubectl get jobs -n etl -l app=daily-report --sort-by=.status.startTime

# 3. 查看仍在运行或异常的 Job/Pod
kubectl get pod -n etl -l job-name=daily-report-xxx -o wide
kubectl logs -n etl job/daily-report-xxx --tail=100

# 4. 检查最近事件
kubectl get events -n etl --sort-by='.lastTimestamp' | grep daily-report | tail -30

# 5. 查看 kube-controller-manager 日志中的 CronJob 调度记录
kubectl logs -n kube-system -l component=kube-controller-manager --tail=300 | grep -i 'cronjob|daily-report' | tail -50

# 6. 手动触发一次用于对比
kubectl create job daily-report-manual -n etl --from=cronjob/daily-report
kubectl wait --for=condition=complete job/daily-report-manual -n etl --timeout=300s
```
## 根因分析

综合 CronJob 配置、Job 历史与 controller-manager 日志，判定根因为 **CronJob 的 `concurrencyPolicy` 设置为 `Forbid`，且三天前启动的 Job 因任务卡死未结束，导致后续所有自动调度被跳过**，置信度 **高**。

1. **`concurrencyPolicy: Forbid` 语义**：当上一次调度产生的 Job 仍在运行时，新的调度时间点会被直接跳过，不会创建新 Job，也不会在日志中留下明显错误。
2. **历史 Job 挂起**：三天前的 Job `daily-report-20260623020000` 因依赖的外部 Hive 查询超时，Pod 一直 Running，Job 状态为未 Complete，CronJob 控制器因此持续跳过新调度。
3. **手动触发成功**：手动创建的 Job 不经过 CronJob 调度器，且当时外部依赖已恢复，因此能正常完成。

## 修复命令

**第一步：删除挂起的历史 Job，释放调度锁**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl delete job daily-report-20260623020000 -n etl
```
**第二步：修改 CronJob 配置，允许 Replace 并增加超时控制**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch cronjob daily-report -n etl --type=merge --patch '{
  "spec": {
    "concurrencyPolicy": "Replace",
    "startingDeadlineSeconds": 600,
    "jobTemplate": {
      "spec": {
        "activeDeadlineSeconds": 1800,
        "ttlSecondsAfterFinished": 86400
      }
    }
  }
}'
```
**第三步：手动触发一次 CronJob，验证新策略生效**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create job daily-report-verify -n etl --from=cronjob/daily-report
kubectl wait --for=condition=complete job/daily-report-verify -n etl --timeout=1200s
kubectl logs -n etl job/daily-report-verify --tail=50
```
**第四步：清理旧配置并提交 GitOps 仓库**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 将修改后的 YAML 导出并提交到 GitOps 仓库
kubectl get cronjob daily-report -n etl -o yaml | sed '/status:/,$d' > etl-daily-report.yaml
# git add / commit / push 到 Argo CD 仓库
```
## 验证命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. CronJob 配置已更新
kubectl get cronjob daily-report -n etl -o jsonpath='{.spec.concurrencyPolicy}{"\n"}{.spec.startingDeadlineSeconds}{"\n"}'

# 2. 等待下一个调度周期，确认新 Job 自动创建
kubectl get jobs -n etl -l app=daily-report --sort-by=.status.startTime

# 3. 新 Job 完成且 Pod 退出码为 0
kubectl get pod -n etl -l job-name=daily-report-xxx -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\t"}{.status.containerStatuses[0].state.terminated.exitCode}{"\n"}{end}'

# 4. 报表输出文件已生成
kubectl run s3-ls --rm -it --restart=Never -n etl --image=registry-vpc.cn-zhangjiakou.aliyuncs.com/acs/busybox:latest -- \
  wget -qO- http://minio.etl.svc.cluster.local:9000/report/daily/$(date +%Y%m%d).csv

# 5. controller-manager 无 skip 日志
kubectl logs -n kube-system -l component=kube-controller-manager --tail=100 | grep daily-report | grep -i skip || echo "无 skip 记录"
```
## 回复客户话术

> 您好，工单 TC-2026-034 已处理完成。
>
> **现象确认：** CronJob `daily-report` 连续 3 天未自动生成报表，手动触发可正常完成。
>
> **根因：** CronJob 的 `concurrencyPolicy` 设置为 `Forbid`，三天前启动的 Job 因外部 Hive 查询超时而一直未结束，CronJob 控制器因此跳过了后续所有自动调度。
>
> **已执行修复：**
> 1. 删除挂起的历史 Job；
> 2. 将 `concurrencyPolicy` 改为 `Replace`，新调度到达时会自动替换旧 Job；
> 3. 设置 `startingDeadlineSeconds=600` 避免短窗口漏调；
> 4. 设置 `activeDeadlineSeconds=1800` 防止任务无限挂起；
> 5. 手动触发一次验证新策略可正常生成报表。
>
> **当前状态：** 手动验证 Job 已 Complete，报表文件已生成到 MinIO 指定目录。
>
> **后续建议：**
> - 参考 [[工作负载/核心工作负载/05-job-cronjob-advanced.md|Job/CronJob 进阶]] review 所有 CronJob 的 `concurrencyPolicy` 与超时设置；
> - 对关键 ETL 任务配置 Job 失败/漏调告警，监控 `kube_job_status_failed` 与 CronJob `last_successful_time`；
> - 在 GitOps 仓库中为 CronJob 增加 lint 规则，禁止生产环境使用无 `activeDeadlineSeconds` 的 `Forbid` 策略；
> - 对 Hive/Presto 等外部依赖增加超时熔断，避免单个任务挂起阻塞整条链路。
>
> 如有异常请随时联系。

## 复盘与沉淀

CronJob 的 `concurrencyPolicy` 是常见的“沉默杀手”。`Forbid` 在任务可能长时间运行的场景下会导致调度被静默跳过，而 `Allow` 则可能在任务积压时产生大量并发 Job。`Replace` 结合 `activeDeadlineSeconds` 是比较稳妥的生产实践：新调度会替换旧任务，同时避免单个 Job 无限挂起。

建议对所有生产 CronJob 建立基线检查：必须设置 `startingDeadlineSeconds`、`activeDeadlineSeconds`、`ttlSecondsAfterFinished`，并配置监控告警。在 GitOps 流程中加入 policy 校验，禁止无超时约束的 CronJob 合并到主干。对于依赖外部系统的 ETL 任务，还应在脚本内部增加超时与熔断逻辑，避免外部依赖拖垮整个调度链路。

在可观测性方面，除了监控 Job 成功/失败次数，还应重点观测 CronJob 的 `last_successful_time` 与 `last_schedule_time` 差值。当差值超过两个调度周期时触发告警，可以更早发现调度被跳过的问题。对于关键报表类 CronJob，建议在失败时自动通知下游消费方，并启动备用数据补跑流程。

建议将本案例写入 ETL 运维 runbook，作为 CronJob 漏调排查的标准流程：先检查 `concurrencyPolicy` 与正在运行的 Job 状态，再检查 `startingDeadlineSeconds` 与 `activeDeadlineSeconds`，最后确认 controller-manager 日志中是否存在 skip 记录。所有生产 CronJob 变更必须通过 GitOps 提交流水线，避免直接在集群中 `kubectl edit` 后忘记同步仓库。

对于多依赖的 ETL 链路，还应在任务入口增加 SLA 看板，展示最近 7 天每次调度的开始时间、持续时长与产出状态。这样不仅可以快速发现漏调，也能为后续优化任务并发度与外部依赖超时提供数据依据。

## 是否需要升级及交接信息

- **是否升级**：已闭环，无需升级。若 `Replace` 后仍频繁出现任务挂起，需升级至 **数据平台团队** 审查 ETL 依赖与超时策略。
- **是否需要变更审批**：是（修改 CronJob 调度策略已登记变更台账）。
- **交接信息**：
  - 故障单号：`TC-2026-034`
  - 根因：`concurrencyPolicy=Forbid + 历史 Job 挂起导致后续调度被跳过`
  - 影响命名空间：`etl`
  - 修复动作：删除挂起 Job + 修改 concurrencyPolicy 为 Replace + 增加超时控制
  - 待跟进：确认下一个自然调度周期（明日凌晨 2 点）自动触发成功

## Related

- CronJob
- Job/CronJob 执行失败：退避重试耗尽与镜像拉取异常
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- Job/CronJob 执行失败：退避重试耗尽与镜像拉取异常
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang


<!-- risk-assessed -->
