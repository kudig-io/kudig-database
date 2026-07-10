---
title: Pod 长期 Pending：节点 CPU/内存资源不足
description: 专有云 ACK 集群业务扩容后大量 Pod 处于 Pending 状态，根因为节点资源不足的工单闭环样本。
summary: 专有云 ACK 集群业务扩容后大量 Pod 处于 Pending 状态，根因为节点资源不足的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- pod-pending
- resource-exhaustion
- capacity
- p1
- scheduling
tier: peripheral
created: '2026-06-26T10:00:00+08:00'
updated: '2026-06-26T12:20:00+08:00'
incident_id: INC-2026-ACK-012
priority: P1
severity: high
affected_cluster: ack-zyy-prod-02
affected_namespace: fintech-core
ticket_type: 调度异常
skill_ref:
- Pod Pending FTA
- Pod 调度排障
fta_ref:
- 'FTA: 资源不足导致 Pod Pending'
last_updated: 2026-06-26 12:20:00+08:00
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
- Pod 长期 Pending：节点 CPU/内存资源不足 如何处理
trigger_keywords:
- Pod
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

客户在 `ack-zyy-prod-02` 集群执行金融核心服务扩容后，发现大量新 Pod 持续处于 `Pending` 状态，已有服务无法水平扩展。客户描述如下：

> “上午我们对 fintech-core 命名空间里的 risk-engine 做了扩容，replicas 从 10 调到 25，但是新 Pod 全部 Pending。describe pod 看到 `Insufficient cpu` 和 `Insufficient memory`。集群节点都是 Ready，但资源看起来吃满了。能不能赶紧帮忙扩一下，现在高峰期风控请求有堆积。”

受影响命名空间为 `fintech-core`，主要涉及 `risk-engine`、`payment-gateway`、`user-profile` 三个 Deployment，均为金融交易链路核心组件。

## 分类与优先级判定

- **工单类型**：调度异常 / 资源容量不足。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境核心服务扩容失败，业务请求堆积，服务降级明显。
2. Pod 明确报 `Insufficient cpu` 与 `Insufficient memory`，根因清晰但需快速扩容。
3. 高峰期金融业务对延迟敏感，需在 15 分钟内给出扩容与调度方案。

## 诊断步骤

按“先看 Pod 事件、再看节点资源、最后看调度约束”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Pending Pod 列表与事件
kubectl get pod -n fintech-core | grep Pending
kubectl describe pod -n fintech-core risk-engine-xxx | grep -A 10 Events

# 2. 查看节点资源分配率
kubectl top node
kubectl describe node | grep -A 5 "Allocated resources"

# 3. 按节点统计已分配资源
kubectl get node -o json | jq -r '.items[] | "\(.metadata.name) cpu:\(.status.allocatable.cpu)/mem:\(.status.allocatable.memory)"'

# 4. 查看集群自动伸缩器状态
kubectl get configmap cluster-autoscaler-status -n kube-system -o yaml
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=200 | grep -iE "scale up|insufficient|node group"

# 5. 检查节点池与伸缩组状态
aliyun cs GET /clusters/ack-zyy-prod-02/nodepools
aliyun cs GET /clusters/ack-zyy-prod-02/nodes

# 6. 检查 Pod 是否有不合理的资源请求或反亲和性
kubectl get deployment risk-engine -n fintech-core -o yaml | grep -A 20 resources
kubectl get pod -n fintech-core -o wide | grep risk-engine
```
## 根因分析

`fintech-core` 命名空间内现有 Pod 的资源请求（requests）已接近节点可分配总量。本次 `risk-engine` 从 10 副本扩容到 25 副本，新增 15 个 Pod 每个请求 `cpu: 2`、`memory: 4Gi`，总计需要额外 30 核 CPU 与 60Gi 内存。而当前节点池 `np-fintech-compute` 中所有节点的 allocatable 资源已被占满：

```
Warning  FailedScheduling  12s  default-scheduler  0/12 nodes are available:
  8 Insufficient cpu, 6 Insufficient memory.
```

同时，`cluster-autoscaler` 配置的最大节点数为 20，当前已扩容至 20 台，无法再横向扩容。根本原因是节点池资源上限与业务扩容需求不匹配，且未提前触发容量预警。

## 修复命令

**第一步：确认可低优先级业务 Pod，临时释放资源**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看非核心命名空间中资源占用较高的 Pod
kubectl top pod -A --sort-by=cpu | head -30
kubectl top pod -A --sort-by=memory | head -30
```
**第二步：临时缩容开发/测试环境的低优先级负载（经客户确认后执行）**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl scale deployment dev-dashboard -n dev-tools --replicas=0
kubectl scale deployment test-data-generator -n qa --replicas=0
```
**第三步：提升节点池最大节点数并触发扩容**

```bash
aliyun cs PUT /clusters/ack-zyy-prod-02/nodepools/np-fintech-compute \
  --body '{"auto_scaling":{"max_nodes":30,"min_nodes":12,"enable":true}}'
```

**第四步：手动触发 cluster-autoscaler 扩容（若未自动触发）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl annotate deployment risk-engine -n fintech-core cluster-autoscaler.kubernetes.io/safe-to-evict="false" --overwrite
# 等待 CA 识别 Pending Pod 并扩容
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=100 -f
```
**第五步：确认新节点加入并重新调度 Pending Pod**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get node -l nodepool=fp-fintech-compute -o wide
kubectl get pod -n fintech-core | grep Pending
```
## 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 新节点 Ready 且资源充足
kubectl get node -l nodepool=np-fintech-compute -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'

# 2. Pending Pod 全部 Running
kubectl get pod -n fintech-core | grep -v Running
kubectl rollout status deployment/risk-engine -n fintech-core --timeout=300s

# 3. 业务负载与节点资源使用
kubectl top pod -n fintech-core -l app=risk-engine
kubectl top node -l nodepool=np-fintech-compute

# 4. 自动伸缩器状态
kubectl get configmap cluster-autoscaler-status -n kube-system -o yaml
```
## 回复客户话术

> 您好，经排查，`fintech-core` 命名空间内大量 Pod Pending 的根因是 **节点池 CPU/内存资源已分配完毕，且 cluster-autoscaler 达到最大节点数上限**。`risk-engine` 从 10 扩容到 25 副本需要额外 30 核 CPU 与 60Gi 内存，当前节点池已无法承载。我们已完成以下处置：
>
> 1. 临时缩容开发/测试环境的低优先级负载，释放部分资源；
> 2. 将节点池 `np-fintech-compute` 最大节点数从 20 调整为 30；
> 3. 触发 cluster-autoscaler 扩容新节点，Pending Pod 已完成调度。
>
> 当前 `risk-engine` 所有副本已 Running，业务请求堆积应已缓解。建议后续：
> - 建立 容量规划 流程，扩容前评估节点池余量；
> - 配置节点资源分配率告警，阈值建议 CPU/Memory request 使用率 > 80%；
> - 为核心业务设置节点池最小节点数缓冲，避免高峰期扩容滞后。
>
> 如有需要，可进一步讨论专属节点池或包年包月预留实例方案。

## 复盘与沉淀

本次故障是典型的“业务扩容撞上资源天花板”场景。Kubernetes 的调度器只根据 requests 进行调度，不参考 limits 或实际使用率。如果业务长期将 requests 设置得很高，即使实际 CPU 使用率只有 10%，也会导致节点资源在调度视角下被占满。

在专有云 ACK 环境中，cluster-autoscaler 的扩容速度受限于底层 ECS 交付时间，通常需要 2-5 分钟。对于金融类高峰业务，这种滞后不可接受，因此需要在高峰期前预留节点缓冲，或采用包年包月实例作为基础容量。此外，调度约束如反亲和性、节点污点、Pod 拓扑分布也会进一步压缩可用节点范围，导致即使总资源充足，部分 Pod 仍无法调度。排查时需要综合考虑资源总量与调度约束两个维度。

建议建立以下机制：
1. 在业务上线/扩容前，使用 `kubectl describe node` 或 ACK 控制台查看节点资源分配率；
2. 设置告警规则：节点 CPU/Memory request 分配率 > 80% 触发 P2，> 90% 触发 P1；
3. 对非核心负载配置 `PriorityClass`，在资源紧张时优先驱逐低优先级 Pod；
4. 将本次处置写入 Pod Pending 资源不足回复模板。

另外，建议对 `risk-engine` 这类核心服务评估是否使用独占节点池或虚拟节点（Virtual Node）进行弹性承载，以降低对主节点池的冲击。在专有云 ACK 中，虚拟节点可以快速承载突发流量，但需要注意网络延迟与调度约束。对于常驻核心负载，包年包月的独占节点池在成本与稳定性上更具优势。最后，建议在变更管理流程中增加“扩容前资源余量检查”环节，任何涉及副本数增加的变更都需要通过容量审批，避免类似事件再次发生。

## 是否需要升级及交接信息

- **是否升级**：已定位并止血，暂不需要升级；若节点池扩容后仍无法满足业务增长，需升级至 **容量管理团队** 与 **财务/采购团队** 评估新增节点池。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-012`
  - 根因：`节点池资源耗尽且 cluster-autoscaler 达到最大节点数上限`
  - 影响集群：`ack-zyy-prod-02`
  - 影响命名空间：`fintech-core`
  - 临时修复：缩容低优先级负载 + 提升节点池 max_nodes
  - 长期方案：建立容量基线与预留缓冲机制
  - 待跟进：确认新节点稳定运行，更新容量规划模板与告警规则

## Related

- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502


<!-- risk-assessed -->
