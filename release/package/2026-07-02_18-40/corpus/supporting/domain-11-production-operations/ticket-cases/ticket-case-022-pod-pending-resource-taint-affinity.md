---
title: 阿里云专有云 Pod Pending（资源不足 / taint 不匹配 / 亲和性冲突）
description: 大数据 Spark 任务提交后 Driver Pod 长时间 Pending，根因为节点资源不足、污点容忍缺失与 Pod 反亲和性冲突叠加，含诊断、修复与验证。
summary: 大数据 Spark 任务提交后 Driver Pod 长时间 Pending，根因为节点资源不足、污点容忍缺失与 Pod 反亲和性冲突叠加，含诊断、修复与验证。
category: production-operations
tags:
- aliyun
- private-cloud
- ack
- pod-pending
- scheduling
- resource-pressure
- taint
- affinity
- spark
- ticket-case
tier: supporting
created: 2026-06-26
updated: 2026-06-26
incident_id: TC-2026-022
priority: P1
severity: high
affected_cluster: ack-prod-vpc02
affected_namespace: data-platform
ticket_type: 调度失败
skill_ref: Pod Pending 诊断
fta_ref: 'FTA: Pod Pending'
last_updated: 2026-06-26
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
- 阿里云专有云 Pod Pending（资源不足 / taint 不匹配 / 亲和性冲突） 如何处理
trigger_keywords:
- aliyun
- private-cloud
- ack
- pod-pending
- scheduling
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
- target: '[[domain-17-system-foundation/topic-dictionary/scheduling/taint.md]]'
  type: related_to
- target: '[[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-017-pod-pending-resource-exhaustion.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-027-pod-pending-resource-taint.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单 022：Pod Pending（资源不足 / taint 不匹配 / 亲和性冲突）

## 1. 工单描述

**用户原始描述：**

> 我们在阿里云专有云 ACK 集群上跑 Spark 离线任务，namespace 是 data-platform。今天早上 09:30 提交了一批 ETL 任务后，Spark Driver Pod 一直 Pending，已经半个多小时了。kubectl describe pod 看到说什么 insufficient cpu、node(s) had taint {dedicated: bigdata}、node(s) didn't match pod affinity rules 之类的话。我们不太确定到底是哪个原因，之前这个任务都是能跑起来的。昨天晚上有同事对节点做了污点操作。麻烦帮忙看一下，今天这批数据要赶在 12 点前跑完给业务出报表。

## 2. 分类与优先级判定

- **任务类型：** 调度失败 / Pod Pending / 资源与污点/亲和性冲突
- **优先级：** P1（生产环境 + 离线报表链路阻塞 + 有截止时间）
- **严重程度：** high
- **响应时限：** 15 分钟内给出修复方案
- **安全级别：** 低风险（主要涉及调度与资源调整，写操作需确认）

## 3. 诊断步骤

### 3.1 查看 Pending Pod 状态与事件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 列出 data-platform 下所有 Pod
kubectl get pod -n data-platform

# 重点查看 Pending Pod 的详细事件
kubectl describe pod spark-etl-driver-7d9f4b8c5-x2k9m -n data-platform

# 同时查看所有 Pending Pod
kubectl get pod -n data-platform --field-selector=status.phase=Pending
```
### 3.2 检查 Pod 资源请求与限制

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pod YAML
kubectl get pod spark-etl-driver-7d9f4b8c5-x2k9m -n data-platform -o yaml

# 查看对应 Deployment/SparkApplication 的资源配置
kubectl get deployment spark-etl-driver -n data-platform -o yaml
kubectl get sparkapplication spark-etl -n data-platform -o yaml
```
### 3.3 检查节点资源与污点

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有节点资源使用
kubectl top node

# 查看节点标签与污点
kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints,LABELS:.metadata.labels

# 查看大数据专用节点
kubectl get node -l dedicated=bigdata
kubectl describe node $(kubectl get node -l dedicated=bigdata -o jsonpath='{.items[0].metadata.name}')
```
### 3.4 检查命名空间 ResourceQuota 与 LimitRange

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 ResourceQuota
kubectl get resourcequota -n data-platform
kubectl describe resourcequota -n data-platform

# 查看 LimitRange
kubectl get limitrange -n data-platform
kubectl describe limitrange -n data-platform
```
### 3.5 查看调度器日志

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 kube-scheduler Pod 日志（如使用 ACK 托管版，需通过 ASO 或专有云平台查看）
kubectl logs -n kube-system -l component=kube-scheduler --tail=200

# 若使用 ASO 管理，可通过 ASO 控制台查看调度事件
# aliyun cs k8s 命令行也可获取集群事件
aliyun cs k8s GET /clusters/<cluster-id>/events
```
### 3.6 检查 Pod 亲和性与容忍配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 提取 Pod 的 affinity 与 tolerations
kubectl get pod spark-etl-driver-7d9f4b8c5-x2k9m -n data-platform -o jsonpath='{.spec.affinity}' | python -m json.tool
kubectl get pod spark-etl-driver-7d9f4b8c5-x2k9m -n data-platform -o jsonpath='{.spec.tolerations}' | python -m json.tool
```
### 3.7 诊断过程补充说明

Pod Pending 的排障核心在于逐条解读 `kubectl describe pod` 输出的 Events 中 `FailedScheduling` 提示。在阿里云 ACK 专有云环境中，调度器通常采用默认的 kube-scheduler，其过滤阶段会依次检查 Predicates（节点资源、污点容忍、亲和性、卷拓扑等）。如果同一 Pod 同时命中多个失败原因，事件会按顺序列出最常见的一条或几条，因此不能仅看第一条提示就下结论，需要结合节点状态、Pod 规格与集群事件综合判断。

对于 Spark 这类 Driver + Executor 分离架构，Driver 通常请求较大资源并承担任务协调职责，而 Executor 数量多、资源请求相对较小。如果 Driver 设置了强反亲和性要求不能与 Executor 同节点，而大数据节点已经被 Executor 占满，就会出现 "所有节点都满足资源，但没有节点满足亲和性" 的现象。此时即使扩容节点，如果新节点没有 `dedicated=bigdata` 标签或存在 `NoSchedule` 污点，Driver 仍然无法调度。

另外，ResourceQuota 与 LimitRange 的拦截往往表现为 "exceeded quota" 或 "minimum memory usage per Container" 等提示，这类错误在 describe 事件中会非常明确。若 Pending 原因涉及 ResourceQuota，通常需要调整 Quota 或降低 Pod 资源请求，而不是扩展节点。实际排查时应先区分是调度器层面的节点选择问题，还是准入控制层面的配额拦截问题。

在专有云 ASO 管理平台中，也可以通过集群事件页面筛选 `Warning` 级别事件，快速定位最近一段时间内所有 FailedScheduling 的 Pod。对于大数据批处理任务，建议结合 Spark Operator 的 `SparkApplication` 状态字段 `applicationState.state` 判断任务是卡在调度阶段还是运行阶段，避免重复提交导致资源进一步紧张。

## 4. 根因分析

综合 Pod 事件、节点资源、污点与亲和性配置，判定根因为 **"大数据专用节点资源已接近耗尽，且昨晚新增的污点 dedicated=bigdata:NoSchedule 未在 Spark Driver Pod 中配置 toleration，同时 Pod 反亲和性要求 Driver 与 Executor 不能同节点，进一步压缩了可选节点范围"**，置信度 **高**。

1. **资源不足：** 大数据节点 CPU 与内存长期高位运行，新提交任务请求 4C8G 后无可调度节点。
2. **污点未容忍：** 昨晚运维同学为防止非大数据任务占用专用节点，给 bigdata 节点打了 `dedicated=bigdata:NoSchedule` 污点，但 Spark 任务的 toleration 未同步更新。
3. **亲和性冲突：** Spark Driver 配置了 `podAntiAffinity`，要求不能与 Executor 同节点，而 Executor 已经占用了所有 bigdata 节点，导致 Driver 无节点可选。

### 4.1 风险与影响评估

- **业务影响：** 09:30 提交的 ETL 任务 Pending，12 点报表可能无法按时产出，直接影响业务部门决策。
- **扩散风险：** 若后续继续提交 Spark 任务，Pending 队列会持续增长，影响更多业务报表，甚至造成集群调度队列拥堵。
- **资源争抢风险：** 在资源不足的情况下，若用户反复提交任务，可能触发 kube-scheduler 更高的调度重试频率，进一步增加 apiserver 与 etcd 负载。
- **数据风险：** 不涉及数据丢失，但任务延迟可能导致下游依赖空跑、重复跑或失败，需人工补数。
- **运维风险：** 夜间污点变更未同步到应用配置，说明变更流程存在缺口，后续需建立节点属性变更通知机制。

## 5. 修复命令

### 5.1 临时缓解：为现有 Pod 添加 toleration 并重新调度

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 为当前 SparkApplication 添加 toleration
cat <<'EOF' | kubectl patch sparkapplication spark-etl -n data-platform --type=merge --patch-file=/dev/stdin
spec:
  driver:
    tolerations:
      - key: "dedicated"
        operator: "Equal"
        value: "bigdata"
        effect: "NoSchedule"
  executor:
    tolerations:
      - key: "dedicated"
        operator: "Equal"
        value: "bigdata"
        effect: "NoSchedule"
EOF

# 2. 删除 Pending 的 Driver Pod，触发重新创建
kubectl delete pod spark-etl-driver-7d9f4b8c5-x2k9m -n data-platform
```
### 5.2 扩容大数据节点池

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 通过 ACK 控制台或 aliyun CLI 扩容 bigdata 节点池
aliyun cs k8s PUT /clusters/<cluster-id>/nodepools/<nodepool-id> \
  --header "Content-Type=application/json" \
  --body "{\"count\":3}"

# 等待节点 Ready
kubectl get node -l dedicated=bigdata -w
```
### 5.3 调整 Spark Driver 资源请求

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 若业务可接受，适当降低 Driver 资源请求以提升可调度性
cat <<'EOF' | kubectl patch sparkapplication spark-etl -n data-platform --type=merge --patch-file=/dev/stdin
spec:
  driver:
    cores: 2
    memory: "4096m"
  executor:
    cores: 2
    memory: "4096m"
EOF
```
### 5.4 调整 Pod 反亲和性策略

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 将反亲和性从 required 改为 preferred，提升调度成功率
cat <<'EOF' | kubectl patch sparkapplication spark-etl -n data-platform --type=merge --patch-file=/dev/stdin
spec:
  driver:
    affinity:
      podAntiAffinity:
        preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                  - key: spark-role
                    operator: In
                    values:
                      - executor
              topologyKey: kubernetes.io/hostname
EOF
```
## 6. 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认新 Driver Pod 已 Running
kubectl get pod -n data-platform -l spark-role=driver

# 2. 确认 Pod 调度到 bigdata 节点
kubectl get pod -n data-platform -l spark-role=driver -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.nodeName}{"\n"}{end}'

# 3. 查看节点资源使用
kubectl top node -l dedicated=bigdata

# 4. 查看 Spark 任务日志
kubectl logs -n data-platform -l spark-role=driver --tail=100

# 5. 确认无新的 Pending Pod
kubectl get pod -n data-platform --field-selector=status.phase=Pending

# 6. 查看调度事件确认无冲突
kubectl get events -n data-platform --sort-by='.lastTimestamp' | tail -30
```
## 7. 回复客户话术

> 您好，工单 TC-2026-022 已处理完成。
>
> **现象确认：** data-platform namespace 下 Spark ETL 任务提交后 Driver Pod 长时间 Pending，describe 事件显示 insufficient cpu、节点污点不匹配以及 pod 反亲和性冲突。
>
> **根因：** 大数据专用节点资源接近耗尽；昨晚新增的 `dedicated=bigdata:NoSchedule` 污点未在 Spark 任务中配置 toleration；同时 Driver 强制反亲和性要求不能与 Executor 同节点，导致无可用节点。
>
> **已执行修复：**
> 1. 为 SparkApplication 的 Driver 与 Executor 添加 dedicated=bigdata 污点容忍；
> 2. 删除 Pending Driver Pod 触发重新调度；
> 3. 扩容 bigdata 节点池 3 台节点；
> 4. 将 Driver 反亲和性从 required 调整为 preferred，提升调度成功率；
> 5. 在资源紧张时降低 Driver/Executor 资源请求。
>
> **当前状态：** Spark Driver Pod 已 Running 并调度到 bigdata 节点，任务日志正常，无新增 Pending Pod。
>
> **后续建议：**
> - 所有提交到 data-platform 的 Spark/Flink 任务统一配置 bigdata 污点容忍；
> - 建立大数据节点池容量监控与自动扩容策略；
> - 在 GitOps 中固化 Spark 任务的 affinity/toleration 模板；
> - 建议将离线任务与在线业务物理隔离，避免资源争抢；
> - 对关键报表任务预留节点资源或设置更高优先级（PriorityClass）。
>
> 如有异常请随时联系。

## 8. 是否需要升级及交接信息

- **是否升级：** 否（已闭环）
- **是否需要变更审批：** 是（节点池扩容与污点变更已记录变更台账）
- **交接信息：**
  - 已将 SparkApplication  toleration 与亲和性模板提交至 GitOps；
  - 建议大数据团队统一梳理所有离线任务的 toleration 配置；
  - 若后续仍频繁出现 Pending，建议评估大数据节点池自动扩缩容方案；
  - 本案例已沉淀至调度故障知识库，供后续离线任务排查参考。

---

*更新时间：2026-06-26 | 责任域：domain-11-production-operations/ticket-cases*

## Related

- 污点
- 亲和性
- Pod Pending：资源不足与 Taint 不匹配
- [[domain-11-production-operations/ticket-cases/ticket-case-017-pod-pending-resource-exhaustion.md|Pod 大量 Pending：节点 CPU/内存资源不足]]
- Pod Pending：资源不足与污点不匹配
- 亲和性
- Pod Pending：资源不足与 Taint 不匹配
- [[domain-11-production-operations/ticket-cases/ticket-case-017-pod-pending-resource-exhaustion.md|Pod 大量 Pending：节点 CPU/内存资源不足]]
- Pod Pending：资源不足与污点不匹配


<!-- risk-assessed -->
