---
title: Pod 持续 Pending：资源不足、Taint 不匹配与亲和性冲突
description: 专有云 ACK 集群业务发布时大量 Pod 处于 Pending 状态，根因涉及节点资源耗尽、Toleration 缺失与 Pod 亲和性冲突的工单闭环样本。
summary: 专有云 ACK 集群业务发布时大量 Pod 处于 Pending 状态，根因涉及节点资源耗尽、Toleration 缺失与 Pod 亲和性冲突的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- pod-pending
- schedule
- taint
- affinity
- resource
- p1
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T17:00:00+08:00'
incident_id: INC-2026-ACK-047
priority: P1
severity: high
affected_cluster: ack-zyy-prod-05
affected_namespace: risk-engine
ticket_type: 调度故障
skill_ref:
- '[[工作负载/00-core-workloads/22-cluster-capacity-planning.md|集群容量规划]]'
- '[[工作负载/00-core-workloads/23-resource-management.md|资源管理]]'
- '[[故障诊断/topic-skills/skill-set/k8s-deployment-rollout/SKILL.md|Deployment
  滚动发布诊断 Skill]]'
fta_ref:
- 'FTA: Pod Pending 排障'
last_updated: 2026-06-26 17:00:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Pod 持续 Pending：资源不足、Taint 不匹配与亲和性冲突 如何处理
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

客户在专有云 ACK 集群 `ack-zyy-prod-05` 进行风控服务 `risk-engine` 新版本发布时，发现 30 个新 Pod 中有 22 个长时间处于 `Pending` 状态，发布进度卡在 27%。客户描述如下：

> “我们今天下午做风险引擎 v2.3.0 上线，Deployment 副本数从 30 扩到 60，结果一大半 Pod 起不来，kubectl get pod 看都是 Pending。describe pod 里面有 ‘0/12 nodes are available’ 的提示，具体原因五花八门，有的说是 Insufficient cpu，有的说是 node(s) had taint，还有的说是 node affinity。我们已经在 ACK 控制台看了节点资源，CPU 和内存好像还够，麻烦帮忙看看。”

受影响命名空间为 `risk-engine`，业务为实时风控决策服务，Pending Pod 过多会导致风控请求排队，影响支付链路风控拦截能力。

## 分类与优先级判定

- **工单类型**：调度故障 / 容量与调度约束。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境业务发布受阻，部分服务副本无法启动，造成服务降级。
2. Pod Pending 原因涉及资源、Taint、亲和性等多维度调度约束，需要系统排查。
3. 影响风控链路，但未完全中断服务，符合 P1 “生产环境 + 服务降级” 标准，需在 15 分钟内给出修复方案。

## 诊断步骤

按“先看 Pending 原因摘要，再逐项拆解资源/Taint/亲和性”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Pending Pod 列表与状态
kubectl get pod -n risk-engine -l app=risk-engine | grep Pending
kubectl get deployment risk-engine -n risk-engine -o wide

# 2. 提取典型 Pending Pod 的调度失败事件
kubectl describe pod -n risk-engine $(kubectl get pod -n risk-engine -l app=risk-engine | grep Pending | head -1 | awk '{print $1}') | grep -A 30 Events

# 3. 批量查看所有 Pending Pod 的调度提示
for p in $(kubectl get pod -n risk-engine -l app=risk-engine | grep Pending | awk '{print $1}'); do
  echo "=== $p ==="
  kubectl describe pod -n risk-engine $p | grep -A 5 "FailedScheduling|0/.* nodes are available"
done

# 4. 检查节点资源分配与可分配余量
kubectl describe node | grep -A 10 "Allocated resources"
kubectl top node

# 5. 检查节点 Taint 与 Pod Toleration
kubectl get node -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints[*].effect
kubectl get pod -n risk-engine -l app=risk-engine -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.tolerations}{"\n"}{end}' | head -10

# 6. 检查 Pod 亲和性与反亲和性配置
kubectl get deployment risk-engine -n risk-engine -o yaml | grep -A 30 affinity

# 7. 检查 ResourceQuota 与 LimitRange 是否拦截
kubectl get resourcequota -n risk-engine
kubectl describe resourcequota -n risk-engine
kubectl get limitrange -n risk-engine -o yaml

# 8. 使用 scheduler 日志进一步确认调度决策
kubectl logs -n kube-system -l component=kube-scheduler --tail=200 | grep -i "risk-engine" | tail -30

# 9. 通过 ACK 控制台查看节点池与伸缩组状态
ack-cli nodepool list --cluster ack-zyy-prod-05
```
## 根因分析

通过批量 describe Pod 与节点状态，发现 Pending 原因分为三类：

**第一类：资源不足（约占 10 个 Pod）**

```
0/12 nodes are available: 3 Insufficient cpu, 2 Insufficient memory.
```

集群节点池 `np-risk-cpu` 为 `ecs.c7.2xlarge` 规格，单节点可分配 CPU 为 7.6 Core（扣除系统预留）。当前节点上已运行大量其他业务 Pod，CPU Request 余量不足。新 Pod 的 `resources.requests.cpu` 设置为 `2`，而剩余节点上最大连续可用 CPU 不足 2 Core。

**第二类：Taint 不匹配（约占 8 个 Pod）**

```
0/12 nodes are available: 5 node(s) had taint {dedicated: risk-engine:NoSchedule}, that the pod didn't tolerate.
```

运维团队此前为风控业务单独划分了 5 台专用节点，并打了 `dedicated=risk-engine:NoSchedule` 的 Taint。但新版本的 Deployment YAML 中误将 `tolerations` 段删除，导致这些 Pod 无法调度到专用节点，只能去抢占通用节点资源，进一步加剧了通用节点的资源紧张。

**第三类：Pod 反亲和性冲突（约占 4 个 Pod）**

```
0/12 nodes are available: 4 node(s) didn't match pod anti-affinity rules.
```

Deployment 中配置了 `podAntiAffinity`，要求同一个 `app=risk-engine` 的 Pod 在同一节点上最多运行 2 个。但之前因临时手动扩容与故障迁移，部分节点上已经运行了 3 个 Pod，导致新 Pod 无法在这些节点上调度，而剩余节点又因 CPU 不足无法承接。

## 修复命令

**第一步：为风险引擎 Pod 补充正确的 Toleration**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment risk-engine -n risk-engine --type='json' -p='[
  {
    "op": "add",
    "path": "/spec/template/spec/tolerations",
    "value": [
      {
        "key": "dedicated",
        "operator": "Equal",
        "value": "risk-engine",
        "effect": "NoSchedule"
      }
    ]
  }
]'
```
**第二步：临时降低非核心 Pod 优先级，释放通用节点资源**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 将通用节点上一些低优先级、可中断的 Job Pod 缩容
kubectl scale deployment data-sync-batch -n data-platform --replicas=0
```
**第三步：为风险引擎节点池扩容 4 台节点**

```bash
aliyun cs POST /clusters/ack-zyy-prod-05/nodes \
  --body '{
    "count": 4,
    "instance_type": "ecs.c7.2xlarge",
    "nodepool_id": "np-risk-cpu",
    "labels": [{"key": "dedicated", "value": "risk-engine"}],
    "taints": [{"key": "dedicated", "value": "risk-engine", "effect": "NoSchedule"}]
  }'
```

**第四步：修正 Pod 反亲和性阈值，允许单节点 3 个 Pod**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment risk-engine -n risk-engine --type='json' -p='[
  {
    "op": "replace",
    "path": "/spec/template/spec/affinity/podAntiAffinity/preferredDuringSchedulingIgnoredDuringExecution",
    "value": [
      {
        "weight": 100,
        "podAffinityTerm": {
          "labelSelector": {
            "matchExpressions": [
              {"key": "app", "operator": "In", "values": ["risk-engine"]}
            ]
          },
          "topologyKey": "kubernetes.io/hostname"
        }
      }
    ]
  }
]'
```
**第五步：触发滚动重启，应用新的 Toleration 与亲和性**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment risk-engine -n risk-engine
kubectl rollout status deployment risk-engine -n risk-engine --timeout=600s
```
## 验证命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 Pending Pod 全部 Running
kubectl get pod -n risk-engine -l app=risk-engine | grep -v Running
kubectl get deployment risk-engine -n risk-engine

# 2. 验证新节点已加入并带有正确 Taint/Label
kubectl get node -l dedicated=risk-engine -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints[*].effect,STATUS:.status.conditions[-1].type

# 3. 检查节点资源分配是否健康
kubectl describe node -l dedicated=risk-engine | grep -A 10 "Allocated resources"

# 4. 验证 Pod 分布符合反亲和性预期
kubectl get pod -n risk-engine -l app=risk-engine -o wide | awk '{print $7}' | sort | uniq -c

# 5. 检查 ResourceQuota 使用率
kubectl describe resourcequota -n risk-engine

# 6. 业务层面验证风控接口可用性
kubectl run risk-test --image=registry.aliyuncs.com/acs/busybox -n risk-engine --rm -it --restart=Never -- wget -qO- http://risk-engine:8080/healthz
```
## 回复客户话术

> 您好，经排查，本次 `risk-engine` 发布大量 Pod Pending 的根因是 **三类调度约束叠加**：部分节点 **CPU 资源不足**、新版本 Deployment **缺失专用节点 Toleration**、以及 **Pod 反亲和性阈值与当前节点实际分布冲突**。我们已完成以下处置：
>
> 1. 为 `risk-engine` Deployment 补回 `dedicated=risk-engine` 的 Toleration，使其能调度到专用节点；
> 2. 对 `np-risk-cpu` 节点池扩容 4 台 `ecs.c7.2xlarge` 节点；
> 3. 临时缩容了 `data-platform` 命名空间中的非核心批处理任务，释放通用节点资源；
> 4. 将 Pod 反亲和性从强制改为偏好，避免与现有分布冲突。
>
> 当前所有 Pending Pod 已 Running，Deployment 副本数达到 60/60，风控接口健康检查正常。建议后续：
> - 在发布模板中固化 Toleration 与亲和性配置，避免后续版本误删；
> - 配置发布前容量预检，参考 [[工作负载/00-core-workloads/22-cluster-capacity-planning.md|集群容量规划]]；
> - 为 `risk-engine` 节点池启用 Cluster Autoscaler，参考 ACK 集群自动扩缩容。
>
> 如有新异常，请随时联系。

## 复盘与沉淀

本次 Pod Pending 故障是调度约束组合问题的典型案例。核心教训包括：

1. **Toleration 与 Taint 必须成对管理**：业务专属节点通过 Taint 隔离后，所有对应工作负载必须显式声明 Toleration。建议在 Helm Chart / Kustomize 模板中将 Toleration、NodeSelector、亲和性放在同一处基线配置，并通过 CI 校验发布包与基线差异。
2. **资源 Request 不等于资源充足**：节点 `kubectl top` 显示 CPU 使用率 50% 并不代表可调度余量充足。Kubernetes 调度器以 Request 为准，若大量 Pod Request 设置不合理（如 Request 远高于实际使用），会出现“使用率不高但无法调度”的假象。
3. **反亲和性规则需与副本数和节点数匹配**：`podAntiAffinity` 的 `requiredDuringSchedulingIgnoredDuringExecution` 在节点数不足时会直接导致 Pending。对于需要高密度部署的服务，建议使用 `preferredDuringSchedulingIgnoredDuringExecution`，或根据节点池规模动态计算阈值。

建议将本案例加入 Pod Pending FTA，并在日常巡检中增加调度约束一致性检查：
- 检查带有 `dedicated` Taint 的节点是否所有对应 Pod 都有 Toleration；
- 检查 ResourceQuota 使用率是否超过 80%；
- 检查节点池可分配资源是否满足下一次滚动发布所需资源总量。

后续 SOP 更新要点：
1. 发布前执行 `kubectl describe node` 统计目标节点池剩余可分配资源；
2. 节点池 Taint 变更必须同步更新对应业务 Chart 的 Toleration；
3. 对关键业务启用发布卡点：若预测发布所需资源超过节点池余量，则自动触发节点池扩容。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，暂不需要升级；若扩容后节点池仍频繁资源不足，需升级至 **容量规划团队** 评估是否需要调整节点规格或拆分业务域。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-047`
  - 根因：资源不足 + Toleration 缺失 + Pod 反亲和性冲突
  - 影响集群：`ack-zyy-prod-05`
  - 影响命名空间：`risk-engine`
  - 临时修复：补充 Toleration、缩容非核心负载、扩容节点池、调整反亲和性
  - 长期方案：固化发布模板、启用发布前容量预检、配置节点池自动扩缩容
  - 待跟进：确认 4 台新节点稳定运行 24 小时，更新发布 SOP 与基线模板

## Related

- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502


<!-- risk-assessed -->
