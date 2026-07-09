---
title: CronJob 任务执行失败：ETL 作业 OOM 与历史堆积
description: 专有云 ACK 集群数据管道 CronJob 因内存限制不足反复 OOMKilled，导致大量失败 Job 堆积、后续调度被阻塞的工单闭环样本。
summary: 专有云 ACK 集群数据管道 CronJob 因内存限制不足反复 OOMKilled，导致大量失败 Job 堆积、后续调度被阻塞的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- cronjob
- job
- oom
- etl
- p1
- batch
tier: peripheral
created: '2026-06-26T07:00:00+08:00'
updated: '2026-06-26T10:30:00+08:00'
incident_id: INC-2026-ACK-029
priority: P1
severity: high
affected_cluster: ack-zyy-prod-06
affected_namespace: data-pipeline
ticket_type: 批处理任务异常
skill_ref:
- CronJob 失败诊断
- CronJob 最佳实践
fta_ref:
- 'FTA: CronJob OOM 与调度失败'
last_updated: 2026-06-26 10:30:00+08:00
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
- CronJob 任务执行失败：ETL 作业 OOM 与历史堆积 如何处理
trigger_keywords:
- CronJob
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
- target: '[[concepts/cronjob.md]]'
  type: related_to
- target: '[[生产运维/ticket-cases/ticket-case-049-job-cronjob-execution-failure.md]]'
  type: related_to
- target: '[[生产运维/ticket-cases/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户反馈其部署在专有云 ACK 集群 `ack-zyy-prod-06` 的每日 ETL 任务已经连续两天未产出结果，`data-pipeline` 命名空间下出现大量失败 Job。客户描述如下：

> “我们的 etl-daily CronJob 每天早上 6 点跑，昨天开始一直没成功。kubectl get jobs 看到几十个失败的 Job，describe pod 看到 OOMKilled。业务方催着要数据，麻烦尽快处理。”

该 ETL 任务负责从前一天的订单、支付、日志表中抽取数据并写入数仓，延迟会直接影响 BI 报表与运营决策。

## 分类与优先级判定

- **工单类型**：批处理任务异常 / CronJob 失败。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境核心数据管道中断，导致下游报表与数据服务延迟，属于关键业务路径异常。
2. 失败 Job 大量堆积会进一步消耗 API Server 资源，影响同命名空间其他任务调度。
3. 问题根因明确指向 OOM，修复后需清理历史 Job 并验证，预计 30 分钟内可闭环。

## 诊断步骤

按“先 CronJob 与 Job 状态、再 Pod 退出码与日志、最后资源与并发策略”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 CronJob 与最近 Job 列表
kubectl get cronjob -n data-pipeline
kubectl get jobs -n data-pipeline -l app=etl-daily --sort-by='.metadata.creationTimestamp'

# 2. 查看 CronJob 的最近执行记录与并发策略
kubectl describe cronjob etl-daily -n data-pipeline

# 3. 查看失败 Pod 状态与退出原因
kubectl get pod -n data-pipeline -l job-name=etl-daily-28734521 -o wide
kubectl describe pod -n data-pipeline etl-daily-28734521-xxxxx | tail -60

# 4. 查看 Pod 退出码与 OOM 事件
kubectl get events -n data-pipeline --field-selector reason=OOMKilled --sort-by='.lastTimestamp'

# 5. 查看任务日志
kubectl logs -n data-pipeline -l job-name=etl-daily-28734521 --tail=300

# 6. 检查 CronJob 资源限制与调度时间
kubectl get cronjob etl-daily -n data-pipeline -o yaml | grep -A 30 "resources:|schedule:|concurrencyPolicy:|startingDeadlineSeconds:|successfulJobHistoryLimit:|failedJobHistoryLimit:"

# 7. 检查命名空间 ResourceQuota 与 LimitRange
kubectl get resourcequota -n data-pipeline
kubectl get limitrange -n data-pipeline -o yaml

# 8. 检查 controller-manager 是否有异常
kubectl logs -n kube-system -l component=kube-controller-manager --tail=200 | grep etl-daily
```
## 根因分析

经排查，确认 CronJob `etl-daily` 的 Job Pod 因 **内存限制不足** 反复 OOMKilled：

```
State:          Waiting
Reason:         CrashLoopBackOff
Last State:     Terminated
Reason:         OOMKilled
Exit Code:      137
```

该 CronJob 配置如下关键字段：

```yaml
schedule: "0 6 * * *"
concurrencyPolicy: Forbid
startingDeadlineSeconds: 300
successfulJobHistoryLimit: 3
failedJobHistoryLimit: 5
jobTemplate:
  spec:
    template:
      spec:
        containers:
          - name: etl
            image: registry-vpc.cn-beijing.aliyuncs.com/data/etl-daily:v2.1.0
            resources:
              requests:
                cpu: 500m
                memory: 256Mi
              limits:
                cpu: 1000m
                memory: 512Mi
```

近两天业务数据量增长约 3 倍，ETL 任务需要加载更多中间结果到内存进行聚合，实际峰值内存超过 1.2Gi，远超 512Mi 的 Limit，因此每次运行都会被内核 OOM Killer 终止。由于 `concurrencyPolicy: Forbid`，每次失败 Job 仍保留在历史中，新的调度因历史 Job 未清理而继续失败；同时 `failedJobHistoryLimit: 5` 只能保留 5 个失败 Job，但客户手动重试产生了大量额外 Job，导致命名空间内 Job 对象堆积。

根本原因是 **ETL 任务内存 Limit 与数据增长不匹配，且 CronJob 的保留策略未做清理**。

## 修复命令

**第一步：清理历史失败 Job，释放 API Server 压力**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 删除所有状态为 Failed 的 etl-daily Job
kubectl get jobs -n data-pipeline -l app=etl-daily -o json | \
  jq -r '.items[] | select(.status.failed > 0) | .metadata.name' | \
  xargs -I {} kubectl delete job {} -n data-pipeline

# 删除所有非 Running 的 etl-daily Pod（避免残留）
kubectl get pod -n data-pipeline -l app=etl-daily --field-selector status.phase!=Running -o name | \
  xargs -I {} kubectl delete {} -n data-pipeline
```
**第二步：调整 CronJob 内存限制与重试策略**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch cronjob etl-daily -n data-pipeline --type='merge' -p '{
  "spec": {
    "jobTemplate": {
      "spec": {
        "backoffLimit": 3,
        "activeDeadlineSeconds": 3600,
        "template": {
          "spec": {
            "containers": [
              {
                "name": "etl",
                "resources": {
                  "requests": {"cpu": "1000m", "memory": "1Gi"},
                  "limits":   {"cpu": "2000m", "memory": "4Gi"}
                }
              }
            ]
          }
        }
      }
    },
    "successfulJobHistoryLimit": 2,
    "failedJobHistoryLimit": 2
  }
}'
```
**第三步：手动触发一次测试 Job 验证修复效果**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create job etl-daily-test-20260626 --from=cronjob/etl-daily -n data-pipeline
kubectl wait --for=condition=complete job/etl-daily-test-20260626 -n data-pipeline --timeout=1800s
```
**第四步：如数据量持续增长，考虑拆分任务或启用临时大规格节点**

```bash
# 为数据管道命名空间创建高内存节点池
aliyun cs POST /clusters/ack-zyy-prod-06/nodes \
  --body '{
    "count": 1,
    "instance_type": "ecs.r7.4xlarge",
    "nodepool_id": "np-zyy-etl",
    "image_id": "aliyun_3_x64_20G_alibase_20240618.vhd"
  }'
```

**第五步：为 ETL Pod 增加节点亲和性，优先调度到高内存节点池**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch cronjob etl-daily -n data-pipeline --type='merge' -p '{
  "spec": {
    "jobTemplate": {
      "spec": {
        "template": {
          "spec": {
            "nodeSelector": {
              "nodepool": "np-zyy-etl"
            },
            "tolerations": [
              {
                "key": "dedicated",
                "operator": "Equal",
                "value": "etl",
                "effect": "NoSchedule"
              }
            ]
          }
        }
      }
    }
  }
}'
```
## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 历史失败 Job 已清理
kubectl get jobs -n data-pipeline -l app=etl-daily

# 2. 测试 Job 已完成
kubectl get job etl-daily-test-20260626 -n data-pipeline -o jsonpath='{.status.conditions[?(@.type=="Complete")].status}'

# 3. 测试 Pod 没有 OOMKilled
kubectl get pod -n data-pipeline -l job-name=etl-daily-test-20260626 -o jsonpath='{.items[0].status.containerStatuses[0].lastState}'

# 4. 查看 ETL 任务日志确认数据处理正常
kubectl logs -n data-pipeline -l job-name=etl-daily-test-20260626 --tail=100

# 5. CronJob 下次调度时间正确
kubectl get cronjob etl-daily -n data-pipeline -o jsonpath='{.status.nextScheduleTime}'

# 6. 资源限制已更新
kubectl get cronjob etl-daily -n data-pipeline -o jsonpath='{.spec.jobTemplate.spec.template.spec.containers[0].resources}'

# 7. 数仓目标表有新增数据（示例）
kubectl exec -n data-pipeline deploy/dwh-cli -- psql -c "SELECT COUNT(*) FROM dw.fact_orders WHERE dt = CURRENT_DATE - 1;"
```
## 回复客户话术

> 您好，经排查，ETL 任务 `etl-daily` 连续失败的根因是 **数据量增长后，CronJob 容器内存 Limit（512Mi）不足，任务运行时触发 OOMKilled**。由于 `concurrencyPolicy: Forbid` 且历史失败 Job 未及时清理，后续调度也被阻塞。我们已完成以下处置：
>
> 1. 清理了所有历史失败 Job 与残留 Pod；
> 2. 将 ETL 容器内存 Limit 从 512Mi 提升至 4Gi，requests 从 256Mi 提升至 1Gi；
> 3. 设置 `backoffLimit: 3`、`activeDeadlineSeconds: 3600`，并减少历史 Job 保留数量；
> 4. 手动触发一次测试 Job，已正常完成并写入数仓。
>
> 当前数据管道已恢复， yesterday 的数据已补入。建议后续：
> - 根据数据增长趋势定期评估 ETL 资源需求，参考 CronJob 最佳实践；
> - 配置 批处理任务 OOM 告警；
> - 对大数据量任务考虑按业务域拆分为多个 Job，降低单 Pod 内存峰值。
>
> 如有数据口径或调度异常，请随时联系。

## 是否需要升级及交接信息

- **是否升级**：已定位并止血，暂不需要升级；若内存持续提升后仍 OOM，需升级至 **数据平台团队** 与 **ACK 资源管理团队** 评估是否需要更大规格节点或任务拆分。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-029`
  - 根因：`ETL CronJob 内存 Limit 不足导致 OOM，失败 Job 堆积阻塞后续调度`
  - 影响集群：`ack-zyy-prod-06`
  - 影响命名空间：`data-pipeline`
  - 影响任务：`cronjob/etl-daily`
  - 临时修复：清理失败 Job + 提升内存 Limit + 手动触发补跑
  - 长期方案：按数据量增长趋势调整资源基线，拆分大数据量任务，启用高内存节点池
  - 待跟进：确认明日 6 点 CronJob 自动调度成功，监控内存使用峰值

## 复盘与沉淀

本次故障是批处理任务中非常典型的 **“数据增长 + 固定资源限制”** 组合问题。CronJob 与 Deployment 不同，它的 Pod 是短生命周期的，一旦失败就会留下 Job 对象。如果 `concurrencyPolicy: Forbid` 或 `startingDeadlineSeconds` 设置不合理，很容易出现“失败 Job 不清理、新 Job 无法创建”的恶性循环。

复盘要点：
1. **CronJob 资源规划应随数据量调整**：ETL、报表、备份等批处理任务的资源需求通常与数据量成正比，不能一成不变。建议每月 review 一次任务内存/CPU 峰值。
2. **合理设置历史保留策略**：`successfulJobHistoryLimit` 与 `failedJobHistoryLimit` 不建议设置过大，否则大量 Job 对象会拖慢 kube-controller-manager。一般设置为 1~3。
3. **失败 Job 自动清理**：可配合 TTL 控制器，使用 `ttlSecondsAfterFinished` 自动删除已完成 Job，避免手动清理。
4. **OOM 快速识别**：`Exit Code 137` 与 `Reason: OOMKilled` 是明确信号，应优先查看 `kubectl describe pod` 与事件，而不是反复重试。
5. **并发策略与调度窗口设计**：对于必须按时完成的数据管道任务，应谨慎使用 `concurrencyPolicy: Forbid`。若前一次执行因资源不足卡住，后续调度会被无限期阻塞。可考虑使用 `Allow` 配合 `ttlSecondsAfterFinished`，或设置合理的 `startingDeadlineSeconds` 与 `suspend` 策略，避免异常状态持续累积。

后续 SOP 更新要点：
- 在 CI 模板中为 CronJob 默认设置 `backoffLimit`、`activeDeadlineSeconds`、`ttlSecondsAfterFinished`；
- 将 CronJob 资源基线写入 CronJob 最佳实践；
- 配置 Prometheus 告警：`kube_job_status_failed{job_name=~"etl-.*"} > 0` 持续 10 分钟触发 P1；
- 将本案例写入 CronJob 失败回复模板，提升一线响应效率。

## Related

- CronJob
- Job/CronJob 执行失败：退避重试耗尽与镜像拉取异常
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang


<!-- risk-assessed -->
