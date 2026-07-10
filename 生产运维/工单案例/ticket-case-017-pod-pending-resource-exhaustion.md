---
title: Pod 大量 Pending：节点 CPU/内存资源不足
description: 专有云 ACK 集群因业务突发扩容导致节点 CPU/内存资源耗尽，大量 Pod 长期处于 Pending 状态的工单闭环样本。
summary: 专有云 ACK 集群因业务突发扩容导致节点 CPU/内存资源耗尽，大量 Pod 长期处于 Pending 状态的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- pod-pending
- resource-exhaustion
- scheduler
- p1
- capacity
tier: core
created: '2026-06-26T14:30:00+08:00'
updated: '2026-06-26T17:00:00+08:00'
incident_id: INC-2026-ACK-017
priority: P1
severity: high
affected_cluster: ack-zyy-prod-05
affected_namespace: flashsale
ticket_type: 调度失败
skill_ref:
- Pod Pending 排查
- Pod 调度保障
fta_ref:
- 'FTA: 资源不足导致 Pod Pending'
last_updated: 2026-06-26 17:00:00+08:00
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
- Pod 大量 Pending：节点 CPU/内存资源不足 如何处理
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
- target: '[[生产运维/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[生产运维/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[生产运维/ticket-cases/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户在促销预热期间对秒杀服务进行扩容，通过 HPA 将 `flashsale-api` Deployment 副本数从 20 扩展到 80，但发现新增 Pod 大量处于 `Pending` 状态。客户描述如下：

> “我们在 ACK 集群 ack-zyy-prod-05 的 flashsale 命名空间扩容秒杀服务，kubectl get pod 看到很多 Pod 一直 Pending，describe pod 提示 Insufficient cpu 和 Insufficient memory。节点池已经开到最大了，但还是调度不上去。业务压力越来越大，请求响应开始变慢，麻烦尽快看一下。”

受影响命名空间为 `flashsale`，核心应用包括 `flashsale-api`、`flashsale-order`、`flashsale-stock`。当前节点池 `np-zyy-flashsale` 实例规格为 `ecs.c7.xlarge`，节点数量已触达伸缩组上限 8 台，但仍无法满足突发资源需求。

## 分类与优先级判定

- **工单类型**：调度失败 / 资源容量不足。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境服务扩容受阻，现有 Pod 仍可运行，但新增实例无法调度，业务处理能力受限。
2. 报错明确为 `Insufficient cpu` 与 `Insufficient memory`，属于集群容量瓶颈。
3. 处于促销预热阶段，若不及时扩容，高峰期将出现服务降级，需在 30 分钟内给出完整扩容方案。

## 诊断步骤

按“先看 Pod 事件、再看节点资源、再看调度器日志”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Pending Pod 列表与状态
kubectl get pod -n flashsale | grep Pending

# 2. 查看任意 Pending Pod 的 Events，确认调度失败原因
kubectl describe pod -n flashsale $(kubectl get pod -n flashsale | grep Pending | head -1 | awk '{print $1}') | tail -50

# 3. 查看所有节点资源分配情况
kubectl describe node | grep -A 5 "Allocated resources"

# 4. 使用 ack-cli 查看节点池容量与已分配率
ack-cli nodepool list --cluster ack-zyy-prod-05
ack-cli node status --cluster ack-zyy-prod-05 --resource-usage

# 5. 查看 scheduler 日志，确认是否有其他调度约束（taint、亲和性）
kubectl logs -n kube-system -l component=kube-scheduler --tail=200 | grep -iE "Insufficient|Predicates|flashsale"

# 6. 检查节点池伸缩组上限
aliyun cs GET /clusters/ack-zyy-prod-05/nodepools/np-zyy-flashsale

# 7. 检查 ResourceQuota 是否限制了 flashsale 命名空间
kubectl get resourcequota -n flashsale -o yaml

# 8. 查看 HPA 当前状态与目标副本数
kubectl get hpa -n flashsale flashsale-api -o yaml
```
## 根因分析

经过排查，发现 `flashsale` 命名空间内大量 Pod 的 Events 中出现以下信息：

```
0/8 nodes are available: 3 Insufficient cpu, 5 Insufficient memory. preemption: 0/8 nodes are available: 8 No preemption victims found for incoming pod.
```

根本原因为：业务突发扩容导致资源需求远超当前节点池容量。节点池 `np-zyy-flashsale` 使用 `ecs.c7.xlarge` 规格（4 vCPU / 8 GiB 内存），单节点扣除系统开销与 DaemonSet 占用后，可分配资源有限。虽然 HPA 将副本数提升到 80，但集群剩余可调度资源不足以支撑新增 60 个 Pod，且伸缩组 `max_size` 设置为 8，无法继续横向扩容。

进一步分析发现，部分业务 Pod 的 `resources.requests` 设置偏高（CPU 500m / 内存 1Gi），但实际利用率仅为 30% 左右，资源申请与实际使用存在较大偏差，加剧了资源碎片问题。加之节点池规格较小，单节点无法容纳更多 Pod，导致大量 Pending。

## 修复命令

**第一步：临时扩容节点池上限，允许 Cluster Autoscaler 继续增加节点**

```bash
# 调整节点池最大节点数从 8 到 20
aliyun cs POST /clusters/ack-zyy-prod-05/nodepools/np-zyy-flashsale \
  --body '{"auto_scaling":{"enable":true,"max_instances":20,"min_instances":3,"type":"cpu"}}'
```

**第二步：临时提升节点规格，创建高规格节点池承载大内存/CPU Pod**

```bash
# 创建临时高规格节点池
aliyun cs POST /clusters/ack-zyy-prod-05/nodepools \
  --body '{
    "nodepool_info": {"name": "np-zyy-flashsale-burst"},
    "scaling_group": {
      "instance_types": ["ecs.c7.4xlarge"],
      "min_instance": 0,
      "max_instance": 10,
      "image_id": "aliyun_3_x64_20G_alibase_20240618.vhd",
      "system_disk_category": "cloud_essd",
      "system_disk_size": 120
    },
    "kubernetes_config": {"labels": {"workload": "flashsale-burst"}}
  }'
```

**第三步：为 Pod 增加节点亲和性，引导新增 Pod 调度到高规格节点池**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment flashsale-api -n flashsale --type='json' -p='[
  {"op": "add", "path": "/spec/template/spec/affinity", "value": {
    "nodeAffinity": {
      "preferredDuringSchedulingIgnoredDuringExecution": [
        {"weight": 100, "preference": {"matchExpressions": [{"key": "workload", "operator": "In", "values": ["flashsale-burst"]}]}}
      ]
    }
  }}
]'
```
**第四步：优化 HPA 目标利用率与资源申请，避免过度申请**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment flashsale-api -n flashsale --type='json' -p='[
  {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/cpu", "value": "300m"},
  {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/memory", "value": "512Mi"}
]'
```
**第五步：临时缩减非核心副本数，为秒杀核心服务释放资源**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl scale deployment flashsale-log-collector -n flashsale --replicas=2
kubectl scale deployment flashsale-report -n flashsale --replicas=1
```
## 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 节点池最大节点数已调整
aliyun cs GET /clusters/ack-zyy-prod-05/nodepools/np-zyy-flashsale | jq '.auto_scaling.max_instances'

# 2. 新节点已加入并 Ready
kubectl get node -l workload=flashsale-burst -o wide

# 3. Pending Pod 数量下降
kubectl get pod -n flashsale | grep Pending | wc -l

# 4. 业务 Pod 全部 Running
kubectl get pod -n flashsale -l app=flashsale-api

# 5. 资源利用率回归合理区间
kubectl top pod -n flashsale -l app=flashsale-api

# 6. HPA 已触发扩容且副本数稳定
kubectl get hpa -n flashsale flashsale-api -o jsonpath='{.status.currentReplicas}/{.status.desiredReplicas}'
```
## 回复客户话术

> 您好，经排查，本次 Pod 大量 Pending 的根因是 **flashsale 命名空间资源需求超过当前节点池可用容量**。当前节点池 `np-zyy-flashsale` 最大节点数为 8，且单节点规格较小（`ecs.c7.xlarge`），HPA 扩容 80 副本后无足够 CPU/内存 可供调度。我们已完成以下处置：
>
> 1. 将节点池 `np-zyy-flashsale` 最大节点数临时从 8 调整为 20，触发 Cluster Autoscaler 自动扩容；
> 2. 创建临时高规格节点池 `np-zyy-flashsale-burst`（`ecs.c7.4xlarge`），承载突发大负载 Pod；
> 3. 为 `flashsale-api` 增加节点亲和性，优先调度到高规格节点；
> 4. 优化 `flashsale-api` 的 CPU/内存 requests（分别下调至 300m / 512Mi），缓解资源碎片；
> 5. 临时缩减非核心服务副本，为核心秒杀服务释放资源。
>
> 当前 Pending Pod 已逐步 Running，HPA 目标副本数已达成。建议后续：
> - 在促销前进行 容量规划，按峰值 QPS 预扩容节点池；
> - 配置 集群资源使用率告警 与 Pending Pod 告警；
> - 建立资源基线，定期 review requests/limits 是否与实际利用率匹配。
>
> 如有其他异常，请随时联系。

## 复盘与沉淀

本次故障反映出容量管理上的典型问题：业务扩容依赖 HPA，但集群底层容量没有同步预留，导致“有扩容意图、无扩容资源”。HPA 只能决定 Pod 数量，无法保证集群节点资源充足，二者必须配合 Cluster Autoscaler 与节点池预扩容策略。

在专有云 ACK 场景中，节点池扩缩容受限于底层 IaaS 库存、伸缩组配置与镜像准备时间，突发扩容往往存在 3-5 分钟延迟。对于秒杀、大促等可预测高峰，应在高峰前 30 分钟至 1 小时完成节点池预扩容，而非依赖实时自动扩容。

针对资源申请碎片问题，建议：
1. 使用 VPA 或手动 review 调整 requests，使其接近实际 P95 利用率；
2. 统一业务容器规格，减少因 requests 不规则导致的节点碎片；
3. 对高并发业务使用大规格节点（如 8 vCPU / 16 GiB 以上），提升单节点承载密度并减少调度碎片。

后续 SOP 更新要点：
1. 大促前必须执行容量演练，模拟 HPA 最大副本数下的节点需求；
2. 节点池 max_size 不应成为瓶颈，建议按业务峰值 1.5 倍设置；
3. 配置告警：`kube_pod_status_phase{phase="Pending"}` 持续 5 分钟 > 10 个触发 P1 告警；
4. 将本案例写入 Pod Pending 资源不足回复模板。

最后，建议将资源申请偏差率纳入 FinOps 指标，定期识别 requests 远高于 utilization 的工作负载，持续优化成本与调度效率。

## 是否需要升级及交接信息

- **是否升级**：已止血并恢复调度，暂不需要升级；若自动扩容后仍持续 Pending 或出现 IaaS 库存不足，需升级至 **平台基础设施团队** 与 **ACK 产品支持**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-017`
  - 根因：`flashsale` 命名空间扩容导致节点池 CPU/内存资源耗尽
  - 影响集群：`ack-zyy-prod-05`
  - 影响命名空间：`flashsale`
  - 临时修复：提升节点池上限 + 创建高规格临时节点池 + 优化资源申请
  - 长期方案：大促前预扩容、容量规划 SOP、资源申请偏差治理
  - 待跟进：确认促销结束后缩容临时节点池、更新资源基线

## Related

- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502


<!-- risk-assessed -->
