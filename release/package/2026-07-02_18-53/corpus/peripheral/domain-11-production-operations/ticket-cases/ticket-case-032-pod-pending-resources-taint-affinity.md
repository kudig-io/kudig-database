---
title: Pod Pending：资源请求过大 + 节点 taint/反亲和性冲突
description: 专有云 ACK 大数据命名空间 Spark 驱动 Pod 因内存请求超出节点容量、未容忍 NoSchedule taint 且 podAntiAffinity
  冲突导致长期 Pending 的工单闭环样本。
summary: 专有云 ACK 大数据命名空间 Spark 驱动 Pod 因内存请求超出节点容量、未容忍 NoSchedule taint 且 podAntiAffinity
  冲突导致长期 Pending 的工单闭环样本。
category: production-operations
tags:
- ack
- zyy
- spark
- pod-pending
- scheduler
- taint
- affinity
- p1
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:30:00+08:00'
incident_id: TC-2026-032
priority: P1
severity: high
affected_cluster: ack-zyy-prod-01
affected_namespace: data-platform
ticket_type: 调度失败
skill_ref:
- '[[domain-02-workloads-applications/核心工作负载/19-scheduler-configuration.md|Scheduler
  配置]]'
- '[[domain-02-workloads-applications/核心工作负载/22-cluster-capacity-planning.md|集群容量规划]]'
fta_ref:
- '[[domain-10-troubleshooting-diagnostics/FTA故障树/list/scheduler-fta.md|FTA: 调度失败]]'
- '[[domain-10-troubleshooting-diagnostics/FTA故障树/list/pod-fta.md|FTA: Pod 异常]]'
last_updated: 2026-06-26 16:30:00+08:00
duplicate_of: INC-2026-ACK-047
status: duplicate
duplication_reason: 与 "INC-2026-ACK-047" 主题重复，内容角度相似，降低 RAG 权重
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Pod Pending：资源请求过大 + 节点 taint/反亲和性冲突 如何处理
trigger_keywords:
- ack
- zyy
- spark
- pod-pending
- scheduler
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
- target: '[[domain-11-production-operations/工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户反馈大数据平台提交 Spark 批处理任务后，驱动 Pod `spark-driver-etl-xxx` 在 `data-platform` 命名空间下一直处于 Pending，已持续超过 30 分钟，导致当日报表未生成。客户描述如下：

> “我们通过 Spark Operator 提交了一个 ETL 任务，driver 的 Pod 一直 Pending。describe pod 看到 `0/8 nodes are available: 3 Insufficient memory, 3 node(s) had taint {dedicated=bigdata: NoSchedule}, and 2 node(s) didn't match pod anti-affinity rules`。我们之前跑过类似任务都没问题，今天突然就不行了。麻烦看一下是不是资源不够还是调度配置有问题。”

该集群为专有云 `ack-zyy-prod-01`，`data-platform` 命名空间承载离线 ETL 与特征工程任务，当前为夜间批处理窗口。

## 分类与优先级判定

- **工单类型**：调度失败 / Pod Pending。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境批处理窗口内关键任务无法启动，影响下游报表与数据同步。
2. Pod Pending 原因明确指向资源、污点与反亲和性三重约束，需快速判断是扩容还是修改任务配置。
3. 延误将导致数据 SLA 违约，需在 15 分钟内给出可执行方案。

## 诊断步骤

按“先看 Pod 事件、再看节点资源与标签、最后看调度器日志”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认 Pod 当前状态与事件
kubectl get pod -n data-platform -l spark-role=driver
kubectl describe pod spark-driver-etl-7d9c4f8b5-xk2z9 -n data-platform | tail -60

# 2. 查看 Pod 资源请求与调度约束
kubectl get pod spark-driver-etl-7d9c4f8b5-xk2z9 -n data-platform -o yaml | grep -A 30 'resources:|nodeSelector:|tolerations:|affinity:'

# 3. 检查节点资源与标签
kubectl top node
kubectl get nodes --show-labels | grep -E 'dedicated|bigdata'
kubectl describe node cn-zhangjiakou.172.16.3.10 | grep -A 15 'Allocated resources'

# 4. 查看当前已调度的大数据 Pod 分布
kubectl get pod -n data-platform -o wide -l spark-role=executor

# 5. 查看 kube-scheduler 日志中的调度失败原因
kubectl logs -n kube-system -l component=kube-scheduler --tail=300 | grep spark-driver-etl | tail -50

# 6. 使用 ACK 调度诊断工具
ack-cli scheduler diagnose --cluster ack-zyy-prod-01 --namespace data-platform --pod spark-driver-etl-7d9c4f8b5-xk2z9

# 7. 检查节点池与 auto scaler 状态
kubectl get nodepool -A
aliyun cs GET /clusters/ack-zyy-prod-01/nodepools
```
## 根因分析

综合 Pod 事件、节点资源与调度器日志，判定根因为 **Spark driver Pod 资源请求超出当前节点池可用容量，同时缺少对 `dedicated=bigdata:NoSchedule` 污点的容忍，且 podAntiAffinity 与同任务 executor 冲突**，置信度 **高**。

1. **资源请求过大**：driver Pod 声明 `memory: 32Gi`，而当前 `np-zyy-bigdata` 节点池实例规格为 `ecs.r6.xlarge`，单节点仅 32Gi 内存，扣除系统与 DaemonSet 占用后可用约 24Gi，无法满足。
2. **污点未容忍**：节点池为了隔离大数据任务，对节点打了 `dedicated=bigdata:NoSchedule` 污点，但新任务模板遗漏了 toleration，导致 3 台大数据节点全部不可调度。
3. **反亲和性冲突**：Pod 配置了 `podAntiAffinity`，要求不与同一 `spark-app-id` 的 executor 共存；已有 2 个 executor 占用了所有普通计算节点上可运行该 driver 的位置，进一步缩小可选节点集。

## 修复命令

**第一步：临时扩大节点池实例规格（长期方案）**

在 ACK 控制台 → 节点池 → `np-zyy-bigdata` 中修改实例规格为 `ecs.r6.2xlarge`（64Gi 内存），并开启自动修复。也可通过 API 扩容节点：

```bash
aliyun cs POST /clusters/ack-zyy-prod-01/nodes \
  --body '{"count":2,"instance_type":"ecs.r6.2xlarge","nodepool_id":"np-zyy-bigdata","image_id":"aliyun_3_x64_20G_alibase_20240618.vhd"}'
```

**第二步：为当前任务临时增加 toleration 与节点选择器**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch pod spark-driver-etl-7d9c4f8b5-xk2z9 -n data-platform --type=merge --patch '{
  "spec": {
    "nodeSelector": {"dedicated": "bigdata"},
    "tolerations": [
      {"key": "dedicated", "operator": "Equal", "value": "bigdata", "effect": "NoSchedule"}
    ]
  }
}'
```
> 注：由于 Pod spec 在创建后不可变，更推荐修改 SparkApplication/Deployment 模板后重新提交。上述 patch 仅用于说明字段，实际应删除旧 Pod 并由控制器重建。

**第三步：降低 driver 内存请求以适应当前节点（临时止血）**

修改 Spark 任务参数后重新提交：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 假设通过 SparkApplication CRD 提交
kubectl patch sparkapplication etl-daily -n data-platform --type=merge --patch '{
  "spec": {
    "driver": {
      "memory": "16g",
      "cores": 4
    }
  }
}'
```
**第四步：放宽反亲和性或拆分任务**

若业务允许 driver 与 executor 同节点，可移除 `podAntiAffinity` 中的 driver-executor 互斥规则；否则应扩容节点池并保留反亲和性：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get sparkapplication etl-daily -n data-platform -o yaml | sed '/podAntiAffinity/,/requiredDuringSchedulingIgnoredDuringExecution/d' | kubectl apply -f -
```
## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 新 Pod 已调度并 Running
kubectl get pod -n data-platform -l spark-role=driver -o wide

# 2. 调度事件显示 Scheduled
kubectl describe pod spark-driver-etl-7d9c4f8b5-new -n data-platform | grep -E 'Scheduled|Started'

# 3. 节点资源分配率正常
kubectl describe node cn-zhangjiakou.172.16.3.10 | grep -A 10 'Allocated resources'

# 4. Spark 任务日志正常
kubectl logs -n data-platform -l spark-role=driver --tail=50

# 5. 报表输出文件生成
kubectl exec -n data-platform deploy/spark-history-server -- hdfs dfs -ls /etl/output/daily/$(date +%Y%m%d)
```
## 回复客户话术

> 您好，工单 TC-2026-032 已处理完成。
>
> **现象确认：** Spark 驱动 Pod `spark-driver-etl-xxx` 在 `data-platform` 命名空间持续 Pending，导致 ETL 报表未生成。
>
> **根因：** 当前任务 driver 请求 32Gi 内存，而大数据节点池 `np-zyy-bigdata` 单节点可用内存仅约 24Gi；同时任务模板缺少对 `dedicated=bigdata:NoSchedule` 污点的容忍，且 podAntiAffinity 与同任务 executor 冲突，导致无节点可调度。
>
> **已执行修复：**
> 1. 将 `np-zyy-bigdata` 节点池扩容 2 台 `ecs.r6.2xlarge`（64Gi 内存）实例；
> 2. 在 Spark 任务模板中补充 `nodeSelector` 与 `tolerations`；
> 3. 将 driver 内存临时调整为 16Gi 以快速恢复批处理窗口；
> 4. 建议后续按业务需要决定是否保留 driver-executor 反亲和性。
>
> **当前状态：** 新 driver Pod 已调度并 Running，任务日志显示已开始读取数据。
>
> **后续建议：**
> - 参考 [[domain-02-workloads-applications/核心工作负载/22-cluster-capacity-planning.md|集群容量规划]] 在批处理高峰前预扩容大数据节点池；
> - 在 SparkApplication 模板中统一注入大数据节点污点容忍，避免遗漏；
> - 配置 Spark on K8s 监控，对 Pending 超过 5 分钟的 driver 触发 P2 告警；
> - 评估是否引入 Cluster Autoscaler 自动按队列长度扩容大数据节点池。
>
> 如有异常请随时联系。

## 复盘与沉淀

Spark 等大数据任务对节点污点、资源请求和反亲和性的组合约束非常敏感。一个字段遗漏就可能导致批处理窗口整体延误。建议在团队内部建立大数据任务模板（SparkApplication/Deployment 模板），强制注入 `nodeSelector`、`tolerations` 和合理的资源请求，并通过 CI lint 或 OPA/Gatekeeper 策略校验。

此外，节点池的容量规划不能只看 CPU，内存与 Pod 密度同样关键。对于夜间批处理高峰，应提前评估 `np-zyy-bigdata` 的资源水位，必要时配置 Cluster Autoscaler 或定时伸缩。调度失败时，优先使用 `ack-cli scheduler diagnose` 与 `kubectl describe pod` 中的事件快速定位，是资源、污点还是亲和性导致，从而决定是扩容节点还是调整任务。

## 是否需要升级及交接信息

- **是否升级**：已闭环，无需升级。若扩容后仍频繁 Pending，需升级至 **大数据平台团队** 审查任务资源基线。
- **是否需要变更审批**：是（节点池扩容与任务资源调整已登记变更台账）。
- **交接信息**：
  - 故障单号：`TC-2026-032`
  - 根因：`driver 资源请求超节点容量 + 污点未容忍 + 反亲和性冲突`
  - 影响命名空间：`data-platform`
  - 修复动作：节点池扩容 + 任务模板补充 toleration/nodeSelector + 临时降低 driver 内存
  - 待跟进：确认新节点加入并 Ready，评估是否将 driver 内存恢复为 32Gi 并继续扩容节点池

## Related

- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502


<!-- risk-assessed -->
