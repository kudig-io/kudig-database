---
title: Pod Pending 调度失败诊断与修复
description: '# Pod Pending 调度失败诊断与修复'
summary: 'Pod Pending 是 Kubernetes 集群中最常见的工单类型之一。当 Pod 被创建但无法被调度到任何节点上运行时，其 `.status.phase` 将保持为 `Pending`。这种状态可能持续数秒（正常调度延迟）到数天（配置错误或资源不足），直接影响业务部署和弹性伸缩。'
category: pod
tags:
- k8s
- skills
- sop
- runbook
- kubelet
- scheduler
- helm
- hpa
- pdb
- statefulset
tier: core
created: '2026-05-23'
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 30min
intent_queries:
- Pod Pending 调度失败诊断与修复 是什么
- 如何 Pod Pending 调度失败诊断与修复
trigger_keywords:
- Pending
- FailedScheduling
- Unschedulable
- 调度失败
- Pod挂起
- 无法调度
- 资源不足
- Insufficient cpu
- Insufficient memory
- node(s) had taint
- no nodes available
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- gpu-scheduling-basics
skill_id: SKILL-03_POD_PENDING-001
skill_name: Pod Pending 调度失败诊断与修复
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




<!-- condition: kubectl get [[Pods|pods]] -A --field-selector=status.phase=Pending -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{\"\n\"}{end}' 显示有 Pending Pod -->

# Pod Pending 调度失败诊断与修复

> **[[SKILL|Skill]] ID**: SKILL-POD-002  
> **Agent 模式**: L2-semi-auto — 低风险操作自动执行，中/高风险需人工审批  
> **适用版本**: [[Kubernetes|Kubernetes]] v1.28 – v1.32  
> **预计修复时间**: 5–30 分钟

---

## 1. 概述

Pod Pending 是 Kubernetes 集群中最常见的工单类型之一。当 Pod 被创建但无法被调度到任何节点上运行时，其 `.status.phase` 将保持为 `Pending`。这种状态可能持续数秒（正常调度延迟）到数天（配置错误或资源不足），直接影响业务部署和弹性伸缩。

**典型触发场景**：
- 业务 Deployment 扩容或新部署后，部分/全部 Pod 长时间处于 Pending 状态
- HPA 触发扩容但新 Pod 无法被调度，导致服务过载
- 节点维护（cordon/drain）后，被驱逐的 Pod 无法在其他节点上重新调度

**常见根因类别**：
- **资源不足**: CPU / 内存 requests 超出集群可分配资源
- **Taint / Toleration 不匹配**: 节点有 taint 但 Pod 缺少对应的 toleration
- **亲和性约束冲突**: nodeSelector、nodeAffinity、podAffinity / podAntiAffinity 无法满足
- **PVC 未绑定**: Pod 引用的 PersistentVolumeClaim 处于 Pending 或 Lost 状态
- **ResourceQuota 耗尽**: Namespace 级别配额已满
- **调度器异常**: kube-scheduler 不健康或自定义调度器未部署
- **SchedulingGates 阻止**: **[v1.28+]** Pod 被 schedulingGates 门控阻止进入调度队列

**前置条件**：
- **RBAC 权限**:
  - 最小权限: 目标 namespace 内 `pods`, `events`, `nodes`, `persistentvolumeclaims`, `resourcequotas`, `limitranges` 的 `get/list/watch`
  - 修复权限: 上述资源的 `patch`/`update`，以及 `pods/eviction` 的 `create`（如需驱逐）
  - 验证命令: `kubectl auth can-i list pods -n <namespace>`
- **工具要求**:
  - `kubectl` >= v1.28（客户端版本建议与集群版本相差不超过 1 个 minor）
  - `jq` >= 1.6（可选，用于 JSON 解析）
- **集群组件**: Metrics Server（`kubectl top node` 需要）

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | Pod `.status.phase` 为 Pending，持续超过 30 秒 / Pod status shows Pending for more than 30 seconds | `kubectl get pod <pod> -n <ns> -o jsonpath='{.status.phase}'` 返回 `Pending` | 0.95 | 如果 Pod 已有 `nodeName` 分配但仍显示 Pending，可能是 kubelet 未响应 → 检查 SKILL-NODE-001 |
| S2 | Events 中出现 `FailedScheduling` 事件 / Events show FailedScheduling reason | `kubectl get events -n <ns> --field-selector reason=FailedScheduling,involvedObject.name=<pod>` | 0.95 | 如果 Event 是历史遗留（>1h 前），而 Pod 已在运行，则不匹配 |
| S3 | 调度消息包含 "Insufficient cpu" / Scheduling message contains "Insufficient cpu" | `kubectl describe pod <pod> -n <ns>` Events 部分包含 `Insufficient cpu` | 0.90 | 如果节点存在但被 cordon → 结合 S5 taint 判断 |
| S4 | 调度消息包含 "Insufficient memory" / Scheduling message contains "Insufficient memory" | `kubectl describe pod <pod> -n <ns>` Events 部分包含 `Insufficient memory` | 0.90 | 如果是 ephemeral-storage 不足，消息会有所不同 |
| S5 | 调度消息包含 "node(s) had taint" / Scheduling message mentions taint mismatch | `kubectl describe pod <pod>` Events 部分包含 `node(s) had taint` 且 Pod 没有对应 toleration | 0.90 | 如果 taint 是 `node.kubernetes.io/not-ready`，则节点本身可能有问题 → SKILL-NODE-001 |
| S6 | 调度消息包含 "didn't match Pod's node affinity/selector" / Node affinity/selector mismatch | `kubectl describe pod <pod>` Events 部分包含 `node(s) didn't match Pod's node affinity/selector` | 0.85 | 如果所有节点都不匹配，需要确认是否标签配置错误还是真的缺少对应节点 |
| S7 | Pod 引用的 PVC 处于 Pending 状态 / PVC referenced by Pod is in Pending state | `kubectl get pvc -n <ns>` 显示关联 PVC 的状态为 Pending | 0.80 | PVC Pending 可能是独立的存储问题 → 可能需要 SKILL-STORE-001 |
| S8 | Events 中出现 "exceeded quota" / ResourceQuota exceeded message in events | `kubectl get events -n <ns>` 包含 `exceeded quota` 或 `forbidden: exceeded quota` | 0.85 | 如果是 Pod 创建被拒（而非调度失败），可能是 admission webhook 问题 |
| S9 | Pod 长时间 Pending 但没有任何 Events / Pod stays Pending with no events at all | `kubectl get events -n <ns> --field-selector involvedObject.name=<pod>` 返回空 | 0.70 | 可能是调度器自身异常、SchedulingGates 阻止、或 Pod 刚创建（短暂正常） |
| S10 | 多个 Pod 同时 Pending / Multiple Pods Pending simultaneously | `kubectl get pods -n <ns> --field-selector status.phase=Pending` 返回多个结果 | 0.80 | 集群范围性问题，需要评估是否是集群级资源耗尽 |
| S11 | `kube_pod_status_unschedulable` 指标升高 / Metric shows unschedulable pods | PromQL: `kube_pod_status_unschedulable{namespace="<ns>"} > 0` | 0.85 | 需确认是否为该 Pod，而非其他 Pod 的残留指标 |

### 2.2 工单关键词映射

以下是 Agent 进行 NLP 意图匹配时的常见工单描述模式：

**中文工单描述**：
- "Pod 一直在 Pending，已经等了 10 分钟了"
- "部署新服务后 Pod 调度不上去"
- "扩容后新增的 Pod 都在排队"
- "HPA 扩了好多 Pod 但都 Pending"
- "节点资源不足导致 Pod 无法调度"
- "Pod 挂起状态，events 显示 FailedScheduling"
- "集群资源明明还有，但 Pod 就是调度不了"
- "新建的 Pod 没有被分配到任何节点"

**英文工单描述**：
- "Pod stuck in Pending state for over 10 minutes"
- "FailedScheduling: Insufficient cpu"
- "Pods not scheduling after deployment"
- "0/5 nodes are available: 3 Insufficient memory, 2 had taint"
- "New pods won't schedule — no nodes available"
- "HPA scaled up replicas but they're all Pending"
- "Pod pending with unbound PersistentVolumeClaims"

### 2.3 排除标准

此 Skill **不适用** 于以下场景：

| 排除条件 | 判断方法 | 应使用的 Skill / 动作 |
|---------|---------|---------------------|
| Pod 已处于 `ContainerCreating` 状态 | `kubectl get pod` 状态列显示 `ContainerCreating` 而非 `Pending` | 不同问题：可能是镜像拉取失败、网络配置、runtime 问题。参考 SKILL-POD-001 或镜像拉取诊断 |
| Pod 处于 `Running` 但不健康 | `kubectl get pod` 状态列显示 `Running`，但 readiness probe 失败 | Pod 已成功调度，问题出在应用层。参考 SKILL-POD-001 |
| Pod 处于 `CrashLoopBackOff` | `kubectl get pod` 状态列显示 `CrashLoopBackOff` | Pod 已调度成功但容器持续崩溃。使用 SKILL-POD-001 |
| Pod 处于 `Evicted` 状态 | `kubectl get pod` 状态列显示 `Evicted` | 节点资源压力导致驱逐，使用 SKILL-NODE-001 |
| 节点本身 `NotReady` 导致 Pod 无法运行 | `kubectl get nodes` 显示节点 `NotReady` | 根因在节点层，优先使用 SKILL-NODE-001 |
| Pod 被 admission webhook 拒绝创建 | `kubectl get pod` 找不到 Pod，但 `kubectl get events` 有 admission 拒绝消息 | 不是调度问题，是准入控制问题 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: 统计当前 namespace 中 Pending Pod 的数量
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n <namespace> --field-selector status.phase=Pending --no-headers | wc -l
```
> **判断规则**:
> - 1 个 Pod → 单点问题，可能是个别配置错误
> - 2-10 个 Pod → 局部问题，可能影响特定 Deployment / StatefulSet
> - 10+ 个 Pod → 集群级问题，可能是资源耗尽或调度器异常

**Step T2**: 检查 Pending Pod 是否属于同一个 Workload（Deployment / StatefulSet / Job）
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n <namespace> --field-selector status.phase=Pending -o custom-columns=NAME:.metadata.name,OWNER:.metadata.ownerReferences[0].name,KIND:.metadata.ownerReferences[0].kind
```
> **判断规则**:
> - 所有 Pending Pod 属于同一 owner → 定向问题（该 workload 特有配置）
> - Pending Pod 分布在多个 owner → 系统性问题（集群资源或调度器层面）

**Step T3**: 快速检查集群节点资源利用率
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl top nodes --no-headers 2>/dev/null || echo "metrics-server not available, skip"
```
> **判断规则**:
> - 所有节点 CPU/Memory 使用率 > 80% → 集群资源紧张，高优先级
> - 部分节点使用率高、部分低 → 可能是亲和性/taint 导致的调度不均
> - 使用率均不高 → 问题可能不在资源层面（taint、affinity、PVC、quota 等）

**Step T4**: 检查是否跨 namespace 出现 Pending
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods --all-namespaces --field-selector status.phase=Pending --no-headers | wc -l
```
> **判断规则**:
> - 仅当前 namespace → 可能是 ResourceQuota 或 namespace 级配置
> - 多个 namespace → 集群级问题

### 3.2 严重性分级

| 条件 | 级别 | 说明 |
|------|------|------|
| 生产环境关键服务无法扩容，正在影响用户流量；或核心 StatefulSet（如数据库）无法调度 | P1 | 直接影响用户，需立即处理 |
| 新部署的生产服务全部 Pending，尚未接入流量但阻塞发布流程 | P2 | 阻塞部署流水线，需尽快处理 |
| 预发/测试环境 Pod Pending，或生产环境非关键服务少量 Pod Pending | P3 | 不影响线上用户，可排队处理 |
| 集群级调度器异常，所有新 Pod 均无法调度 | P1 | 全集群影响，属于基础设施问题 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过标准诊断流程，立即升级至人工 SRE**：

- **条件 1**: kube-scheduler Pod 本身处于非 Running 状态（控制面问题）
  ```bash
  kubectl get pods -n kube-system -l component=kube-scheduler --no-headers | grep -v Running
  ```
- **条件 2**: 集群中 50% 以上的节点处于 `NotReady` 状态（集群级灾难）
  ```bash
  kubectl get nodes --no-headers | awk '{print $2}' | grep -c NotReady
  ```
- **条件 3**: 同一集群中多个 namespace 的核心服务同时 Pending（控制面或基础设施问题嫌疑）
- **条件 4**: Pending 持续超过 1 小时且影响生产用户流量

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 在 2 分钟内获取 Pod 调度失败的直接原因消息，确定诊断方向

**Step D1.1**: 获取 Pod 完整状态和调度信息
- **命令**:
  ```bash
  kubectl get pod <pod-name> -n <namespace> -o yaml
  ```
- **超时**: 10s
- **关注字段**:
  - `.status.phase` — 确认确实是 `Pending`
  - `.status.conditions` — 查找 `type: PodScheduled`，其 `status` 和 `reason`
  - `.spec.schedulerName` — 是否使用自定义调度器（默认为 `default-scheduler`）
  - `.spec.nodeName` — 如果已分配但仍 Pending，说明 kubelet 未响应
  - `.spec.schedulingGates` — **[v1.28+]** 如果存在，Pod 将不会进入调度队列
- **判断规则**:
  - 如果 `.spec.schedulingGates` 非空 → 根因 RC-012（SchedulingGates 阻止），跳转 Section 5
  - 如果 `.spec.schedulerName` 不是 `default-scheduler` → 记录，继续 D1.4 检查自定义调度器
  - 如果 `.spec.nodeName` 已分配 → 不是调度问题，可能是 kubelet 问题，转 SKILL-NODE-001
  - 其他情况 → 继续 D1.2

**Step D1.2**: 获取 Pod Events，聚焦 FailedScheduling 消息
- **命令**:
  ```bash
  kubectl describe pod <pod-name> -n <namespace>
  ```
- **超时**: 10s
- **关注区域**: 输出末尾的 `Events:` 部分
- **预期输出模式**: 
  ```
  Events:
    Type     Reason            Age   From               Message
    ----     ------            ----  ----               -------
    Warning  FailedScheduling  XXs   default-scheduler  0/N nodes are available: ...
  ```
- **判断规则**:
  - 如果有 `FailedScheduling` 事件 → 解析 Message 内容，跳转 D1.3
  - 如果 Events 为空 → 跳转 D1.4（可能是调度器未处理）
  - 如果有 `Scheduled` 事件但 Pod 仍 Pending → 异常状态，需人工检查

**Step D1.3**: 解析 FailedScheduling 消息（核心决策节点）
- **命令**: 无需额外命令，基于 D1.2 的输出解析
- **超时**: N/A（解析逻辑）
- **FailedScheduling 消息解析决策树**:

  FailedScheduling 消息结构为 `0/N nodes are available: X reason1, Y reason2, ...`，每种原因对应不同根因：

  | 消息模式 | 含义 | 根因分类 | 下一步 |
  |---------|------|---------|-------|
  | `X Insufficient cpu` | X 个节点 CPU allocatable 不足以满足 Pod 的 CPU request | RC-001 | → D2.1 资源分析 |
  | `X Insufficient memory` | X 个节点内存 allocatable 不足以满足 Pod 的 memory request | RC-002 | → D2.1 资源分析 |
  | `X node(s) had taint {key=value:effect}, that the pod didn't tolerate` | X 个节点有 taint 但 Pod 未 tolerate | RC-003 | → D2.2 Taint 分析 |
  | `X node(s) didn't match Pod's node affinity/selector` | X 个节点不满足 Pod 的 nodeSelector 或 nodeAffinity | RC-004 | → D2.3 亲和性分析 |
  | `X node(s) didn't match pod anti-affinity rules` | X 个节点因 podAntiAffinity 被排除 | RC-005 | → D2.3 亲和性分析 |
  | `X node(s) didn't match pod topology spread constraints` | X 个节点不满足 TopologySpreadConstraints | RC-013 | → D2.3 亲和性分析 |
  | `X node(s) didn't find available persistent volumes to bind` | X 个节点没有可用的 PV 绑定 | RC-006 | → D2.4 PVC 分析 |
  | `pod has unbound immediate PersistentVolumeClaims` | Pod 引用的 PVC 未绑定 | RC-006 | → D2.4 PVC 分析 |
  | `exceeded quota` | 超出 ResourceQuota 限制 | RC-007 | → D2.5 Quota 分析 |

  > **重要**: FailedScheduling 消息通常包含**多个原因的组合**（例如 `0/5 nodes are available: 2 Insufficient cpu, 3 node(s) had taint`）。需要将所有原因汇总，确定主要瓶颈。

  - **判断规则**:
    - 如果所有节点都因同一原因被排除 → 单一根因，直接定位
    - 如果不同节点因不同原因被排除 → 多因素问题，需逐一排查
    - 如果消息中出现 `preemption was not helpful` → 即使抢占也无法满足，资源严重不足

**Step D1.4**: 检查调度器状态
- **命令**:
  ```bash
  # 检查默认调度器
  kubectl get pods -n kube-system -l component=kube-scheduler
  
  # 如果 Pod 使用自定义调度器
  kubectl get pods --all-namespaces -l app=<custom-scheduler-name>
  ```
- **超时**: 10s
- **判断规则**:
  - 默认调度器 Running 且 Ready → 调度器正常，问题在 Pod 约束或集群资源
  - 调度器非 Running → 根因 RC-009，需立即恢复调度器
  - 自定义调度器未找到 → 根因 RC-011，自定义调度器未部署
  - 如果 D1.2 Events 为空且调度器正常 → 继续 Phase 2 深度检查

---

### Phase 2: 深度检查（只读，零风险）

> **目标**: 基于 Phase 1 确定的方向，进行精确根因定位

**Step D2.1**: 资源分析 — 比较 Pod requests 与节点可分配资源
- **命令**:
  ```bash
  # 查看所有节点的可分配资源总量
  kubectl get nodes -o custom-columns=\
  NAME:.metadata.name,\
  STATUS:.status.conditions[-1].type,\
  CPU_ALLOC:.status.allocatable.cpu,\
  MEM_ALLOC:.status.allocatable.memory,\
  PODS_ALLOC:.status.allocatable.pods

  # 查看 Pod 的资源请求
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"container: "}{.name}{"\n  cpu request: "}{.resources.requests.cpu}{"\n  mem request: "}{.resources.requests.memory}{"\n"}{end}'

  # 查看每个节点的已分配资源（关键：allocatable vs allocated）
  kubectl describe node <node-name> | grep -A 10 "Allocated resources"
  ```
- **超时**: 15s
- **判断规则**:
  - 如果 Pod 的单个容器 CPU/Memory request 超过任何节点的 allocatable → RC-001/RC-002（Pod request 过大）
  - 如果 Pod request 合理但所有节点 allocated 接近 allocatable → RC-001/RC-002（集群资源耗尽）
  - 如果集群总量充足但没有单个节点有足够剩余 → RC-010（资源碎片化）
  - 如果 Init Container 也有大资源 request → 需要 `max(initContainers requests, sum(containers requests))`

**Step D2.2**: Taint / Toleration 分析
- **命令**:
  ```bash
  # 查看所有节点的 taints
  kubectl get nodes -o custom-columns=\
  NAME:.metadata.name,\
  TAINTS:.spec.taints[*].key

  # 查看节点 taints 详情
  kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, taints: .spec.taints}'

  # 查看 Pod 的 tolerations
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.tolerations}' | jq .
  ```
- **超时**: 10s
- **判断规则**:
  - 如果节点 taint key 未出现在 Pod tolerations 中 → RC-003
  - 常见场景:
    - `node-role.kubernetes.io/control-plane:NoSchedule` — Pod 未 tolerate 控制面 taint
    - `node.kubernetes.io/not-ready:NoSchedule` — 节点 NotReady 的自动 taint → SKILL-NODE-001
    - `node.kubernetes.io/unschedulable:NoSchedule` — 节点被 cordoned
    - 自定义 taint（如 `gpu=true:NoSchedule`、`dedicated=team-a:NoSchedule`）
  - 如果 taint effect 是 `PreferNoSchedule` → 不会完全阻止调度，但会降低优先级
  - 如果 taint effect 是 `NoExecute` → 不仅阻止新调度，还会驱逐已有 Pod

**Step D2.3**: Affinity / NodeSelector / TopologySpreadConstraints 分析
- **命令**:
  ```bash
  # 查看 Pod 的 nodeSelector
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeSelector}' | jq .

  # 查看 Pod 的 affinity 完整配置
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.affinity}' | jq .

  # 查看 Pod 的 TopologySpreadConstraints
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.topologySpreadConstraints}' | jq .

  # 查看所有节点的 labels（用于对照 nodeSelector / nodeAffinity）
  kubectl get nodes --show-labels
  ```
- **超时**: 10s
- **判断规则**:
  - **nodeSelector**: 如果 Pod 指定了 `nodeSelector`，但没有节点拥有这些标签 → RC-004
  - **nodeAffinity (requiredDuringSchedulingIgnoredDuringExecution)**: 硬性约束，无匹配节点则无法调度 → RC-004
  - **nodeAffinity (preferredDuringSchedulingIgnoredDuringExecution)**: 软性约束，不会直接导致 Pending
  - **podAntiAffinity (requiredDuringSchedulingIgnoredDuringExecution)**: 如果在所有可用节点上都存在与规则冲突的 Pod → RC-005
  - **TopologySpreadConstraints** (`whenUnsatisfiable: DoNotSchedule`): 如果无法满足拓扑分布约束 → RC-013
  - **TopologySpreadConstraints** (`whenUnsatisfiable: ScheduleAnyway`): 软约束，不会导致 Pending

**Step D2.4**: PVC 绑定状态检查
- **命令**:
  ```bash
  # 列出 namespace 下所有 PVC 及其状态
  kubectl get pvc -n <namespace>

  # 获取 Pod 引用的 volume 列表
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .spec.volumes[*]}{.name}{"\t"}{.persistentVolumeClaim.claimName}{"\n"}{end}'

  # 查看 PVC 详情（如果有 Pending 的 PVC）
  kubectl describe pvc <pvc-name> -n <namespace>

  # 检查 StorageClass 是否存在
  kubectl get storageclass
  ```
- **超时**: 10s
- **判断规则**:
  - PVC 状态 `Pending` → 检查 PVC 的 Events（是否缺少 StorageClass、provisioner 异常、无可用 PV）
  - PVC 状态 `Lost` → PV 已被删除，需要重新创建或恢复
  - PVC 引用的 StorageClass 不存在 → RC-006（StorageClass 配置错误）
  - PVC 的 accessMode 与可用 PV 不匹配 → RC-006
  - **[v1.29+]** ReadWriteOncePod GA — 确认 PV 不是已被其他 Pod 独占

**Step D2.5**: ResourceQuota 检查
- **命令**:
  ```bash
  # 查看 namespace 下的 ResourceQuota
  kubectl get resourcequota -n <namespace>

  # 查看 ResourceQuota 详情（已使用 vs 限制）
  kubectl describe resourcequota -n <namespace>
  ```
- **超时**: 10s
- **预期输出模式**:
  ```
  Name:       my-quota
  Resource    Used    Hard
  --------    ----    ----
  cpu         3800m   4
  memory      6Gi     8Gi
  pods        18      20
  ```
- **判断规则**:
  - 如果 `Used` 已等于或接近 `Hard` 限制 → RC-007
  - 重点关注: `requests.cpu`、`requests.memory`、`pods`、`count/deployments.apps`
  - 如果 ResourceQuota 中有 `limits.cpu` 或 `limits.memory`，但 Pod 未设置 limits → 需要先检查 LimitRange

**Step D2.6**: LimitRange 检查
- **命令**:
  ```bash
  # 查看 namespace 下的 LimitRange
  kubectl get limitrange -n <namespace>

  # 查看 LimitRange 详情
  kubectl describe limitrange -n <namespace>
  ```
- **超时**: 10s
- **判断规则**:
  - 如果 LimitRange 设置了 `min` 且 Pod 的 request 低于 min → 创建失败（admission 拒绝），通常不会到 Pending 阶段
  - 如果 LimitRange 设置了 `default` 和 `defaultRequest` → Pod 未设置 request 时会自动注入，可能导致 request 超预期
  - 如果 LimitRange 的 `max` 与 ResourceQuota 的 `Hard` 不协调 → RC-008

**Step D2.7**: 调度器健康检查
- **命令**:
  ```bash
  # 检查 kube-scheduler 状态
  kubectl get pods -n kube-system -l component=kube-scheduler -o wide

  # 查看调度器最近日志
  kubectl logs -n kube-system -l component=kube-scheduler --tail=50

  # 检查调度器 leader election（多副本场景）
  kubectl get lease -n kube-system kube-scheduler -o yaml
  ```
- **超时**: 15s
- **判断规则**:
  - 调度器 Pod 非 Running 或 CrashLoopBackOff → RC-009
  - 日志中出现 `unable to schedule`、`error`、`panic` → RC-009
  - Leader lease 过期或无 holder → 调度器无法正常选举，RC-009
  - 调度器正常但 Pod 无 Events → 可能是 SchedulingGates 或 Priority/Preemption 问题

**Step D2.8**: 资源碎片化分析
- **命令**:
  ```bash
  # 综合查看每个节点的可用资源
  # 需要对比 allocatable 与已 allocated 的差值
  for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== Node: $node ==="
    kubectl describe node "$node" | grep -A 5 "Allocated resources" | grep -E "(cpu|memory)"
  done
  ```
- **超时**: 30s
- **判断规则**:
  - 如果集群 CPU 总 allocatable 远大于总 allocated + Pod request，但没有单个节点能容纳 Pod → RC-010（碎片化）
  - 碎片化的典型场景：Pod request 4 CPU，但每个节点只剩余 2 CPU
  - 解决方向: bin-packing、descheduler、或添加更大规格节点

**Step D2.9**: PodDisruptionBudget (PDB) 检查
- **命令**:
  ```bash
  # 查看 namespace 下的 PDB
  kubectl get pdb -n <namespace>

  # 查看 PDB 详情
  kubectl describe pdb -n <namespace>
  ```
- **超时**: 10s
- **判断规则**:
  - PDB 本身不直接导致 Pending，但在 preemption 场景中，PDB 可能阻止低优先级 Pod 被抢占
  - 如果 FailedScheduling 消息包含 `preemption: 0/N nodes are available: X No preemption victims found` → PDB 可能阻止抢占

**Step D2.10**: **[v1.28+]** SchedulingGates 检查
- **命令**:
  ```bash
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.schedulingGates}' | jq .
  ```
- **超时**: 10s
- **判断规则**:
  - 如果返回非空数组（如 `[{"name": "example.com/my-gate"}]`）→ RC-012
  - SchedulingGates 是外部控制器设置的门控，Pod 必须等待控制器移除 gate 后才能进入调度队列
  - 检查是否有对应的控制器在运行
- **版本差异**:
  - **[v1.28]**: SchedulingGates 进入 Beta
  - **[v1.30+]**: SchedulingGates GA，广泛使用

---

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: 模拟 Pod 调度（dry-run）
- **风险级别**: 🟢 低（只读操作）
- **命令**:
  ```bash
  # 创建一个一次性测试 Pod 来验证调度能力（dry-run 不实际创建）
  kubectl run schedule-test --image=busybox --restart=Never \
    --dry-run=server -o yaml \
    --overrides='{"spec":{"nodeSelector":{"<key>":"<value>"}}}' \
    -n <namespace>
  ```
- **超时**: 15s
- **判断规则**:
  - dry-run 成功 → 调度约束问题可能已在 Pod spec 中，对比差异
  - dry-run 被 admission webhook 拒绝 → D3.2

**Step D3.2**: 检查 Mutating / Validating Admission Webhooks
- **风险级别**: 🟢 低（只读操作）
- **命令**:
  ```bash
  # 列出所有 MutatingWebhookConfigurations
  kubectl get mutatingwebhookconfigurations

  # 列出所有 ValidatingWebhookConfigurations
  kubectl get validatingwebhookconfigurations

  # 查看特定 webhook 详情
  kubectl get mutatingwebhookconfigurations -o yaml | grep -A 20 "webhooks:"
  ```
- **超时**: 10s
- **判断规则**:
  - 如果存在 mutating webhook 注入了意外的 nodeSelector、toleration 或 resource request → 导致调度约束变化
  - 如果 validating webhook 失败导致 Pod 创建异常 → 不是调度问题

**Step D3.3**: 检查 PriorityClass 配置
- **风险级别**: 🟢 低（只读操作）
- **命令**:
  ```bash
  # 查看 Pod 的 priorityClassName
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.priorityClassName}'

  # 列出集群中所有 PriorityClass
  kubectl get priorityclass

  # 查看具体 PriorityClass 的优先级值
  kubectl get priorityclass -o custom-columns=NAME:.metadata.name,VALUE:.value,PREEMPTION:.preemptionPolicy
  ```
- **超时**: 10s
- **判断规则**:
  - 如果 Pod 没有设置 PriorityClass 或优先级很低 → 可能被高优先级 Pod 抢占了资源
  - 如果 Pod 的 `preemptionPolicy` 为 `Never` → Pod 不会抢占其他 Pod，只能等待资源释放

**Step D3.4**: **[v1.30+]** Pod Scheduling Readiness 诊断
- **风险级别**: 🟢 低（只读操作）
- **命令**:
  ```bash
  # 检查 Pod 的 SchedulingGates
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.schedulingGates}' | jq .

  # 检查 Pod 的 Scheduling Readiness Conditions
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .status.conditions[?(@.type=="PodScheduled")]}{.type}{"="}{.status}{" reason="}{.reason}{" message="}{.message}{"\n"}{end}'

  # 查看集群中所有带有 SchedulingGates 的 Pod
  kubectl get pods --all-namespaces -o json | jq '.items[] | select(.spec.schedulingGates != null) | {namespace: .metadata.namespace, name: .metadata.name, gates: .spec.schedulingGates}'

  # 检查是否有控制器负责管理该 gate
  kubectl get pods --all-namespaces -l <gate-controller-label> 2>/dev/null || echo "Gate controller not identified"
  ```
- **超时**: 15s
- **判断规则**:
  - `schedulingGates` 非空 → Pod 被外部控制器门控，不会进入调度队列 (RC-012)
  - 常见的 gate 来源:
    - `cluster-autoscaler.kubernetes.io/scheduling-gate`: Cluster Autoscaler 等待节点准备就绪
    - `custom-controller/resource-gate`: 自定义控制器等待资源就绪
  - 如果 gate 长时间未被清除 → 检查对应的 gate controller 是否正常工作
  - **[v1.30+]**: SchedulingGates GA，广泛用于资源预留、弹性扩容等场景
  - **[v1.31+]**: PodSchedulingReadiness 增强，提供更详细的调度就绪状态信息

**Step D3.5**: Topology Spread Constraints 冲突诊断
- **风险级别**: 🟢 低（只读操作）
- **命令**:
  ```bash
  # 查看 Pod 的 TopologySpreadConstraints 配置
  kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 20 topologySpreadConstraints

  # 分析具体的拓扑分布约束
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .spec.topologySpreadConstraints[*]}{"topologyKey: "}{.topologyKey}{"\n  maxSkew: "}{.maxSkew}{"\n  whenUnsatisfiable: "}{.whenUnsatisfiable}{"\n  labelSelector: "}{.labelSelector}{"\n---\n"}{end}'

  # 查看当前各拓扑域的 Pod 分布情况
  # 对于 zone 拓扑约束
  kubectl get pods -n <namespace> -l <label-selector> -o custom-columns=NAME:.metadata.name,NODE:.spec.nodeName,ZONE:.spec.nodeName --no-headers | while read name node zone; do
    if [ -n "$node" ]; then
      actual_zone=$(kubectl get node $node -o jsonpath='{.metadata.labels.topology\.kubernetes\.io/zone}')
      echo "$name -> $node -> $actual_zone"
    else
      echo "$name -> (unscheduled)"
    fi
  done

  # 检查各 zone 的节点数和可用资源
  kubectl get nodes -o custom-columns=NAME:.metadata.name,ZONE:.metadata.labels.topology\.kubernetes\.io/zone,CPU_ALLOC:.status.allocatable.cpu,MEM_ALLOC:.status.allocatable.memory
  ```
- **超时**: 20s
- **判断规则**:
  - **maxSkew 过小**（如 maxSkew=1）且 `whenUnsatisfiable: DoNotSchedule` → 当拓扑域不均衡时无法调度
  - **zone 数量不足** → 例如只有 2 个 zone 但要求 3 副本均匀分布
  - **与 nodeAffinity 的组合约束** → nodeAffinity 限制了可用节点，导致拓扑分布无法满足
  - **labelSelector 不匹配** → 拓扑约束基于的 labelSelector 匹配不到任何 Pod
  - **minDomains 设置过高** → **[v1.30+ GA]** 要求的最小拓扑域数量超过实际可用数量
  - 缓解方案:
    - 将 `whenUnsatisfiable` 从 `DoNotSchedule` 改为 `ScheduleAnyway`
    - 增加 `maxSkew` 的容忍度
    - 检查并调整 nodeAffinity 约束

**Step D3.6**: GPU/特殊资源调度诊断
- **风险级别**: 🟢 低（只读操作）
- **命令**:
  ```bash
  # 检查 Pod 的特殊资源请求
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"container: "}{.name}{"\n  requests: "}{.resources.requests}{"\n  limits: "}{.resources.limits}{"\n"}{end}'

  # 检查集群中的 GPU 可用情况
  kubectl describe nodes | grep -E "nvidia.com/gpu|amd.com/gpu|Allocatable" | head -30

  # 查看每个节点的 GPU 分配情况
  kubectl get nodes -o custom-columns=NAME:.metadata.name,GPU_ALLOC:.status.allocatable.nvidia\.com/gpu,GPU_CAP:.status.capacity.nvidia\.com/gpu

  # 检查 GPU device plugin 状态
  kubectl get pods -n kube-system -l k8s-app=nvidia-device-plugin-daemonset -o wide 2>/dev/null || \
  kubectl get pods -n kube-system -l name=nvidia-device-plugin-ds -o wide 2>/dev/null || \
  echo "NVIDIA device plugin not found - check your GPU plugin deployment"

  # 检查 Node 上的扩展资源（FPGA, RDMA, InfiniBand 等）
  kubectl get node <node-name> -o jsonpath='{.status.allocatable}' | jq .

  # **[v1.32+]** 检查 Dynamic Resource Allocation (DRA) 资源
  kubectl get resourceclaims -n <namespace> 2>/dev/null || echo "DRA not enabled or no ResourceClaims"
  kubectl get resourceclasses 2>/dev/null || echo "DRA not enabled"
  ```
- **超时**: 20s
- **判断规则**:
  - `nvidia.com/gpu` 请求但集群无 GPU 节点 → 需要添加 GPU 节点
  - GPU allocatable 为 0 但 capacity 不为 0 → GPU device plugin 可能未正常工作
  - Device plugin Pod 不在 Running 状态 → 需要修复 device plugin
  - **[v1.32+]** ResourceClaim 未绑定 → DRA 控制器或驱动问题
  - 特殊资源错误模式:
    - `nvidia.com/gpu: 8` 但节点最大只有 4 GPU → 请求量超过单节点容量
    - GPU 节点有 taint 但 Pod 未 tolerate → 结合 D2.2 taint 分析

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | **CPU 资源不足** — 集群中没有节点有足够的可分配 CPU 满足 Pod 的 `resources.requests.cpu` | 高 | D1.3 消息含 `Insufficient cpu`；D2.1 确认节点 allocated CPU 接近 allocatable | pod-fta: BE-SCHED-CPU |
| RC-002 | **内存资源不足** — 集群中没有节点有足够的可分配内存满足 Pod 的 `resources.requests.memory` | 高 | D1.3 消息含 `Insufficient memory`；D2.1 确认节点 allocated memory 接近 allocatable | pod-fta: BE-SCHED-MEM |
| RC-003 | **节点 Taint 与 Pod Toleration 不匹配** — 所有可用节点都有 taint，但 Pod 没有相应 toleration | 高 | D1.3 消息含 `node(s) had taint`；D2.2 确认 taint key 不在 Pod tolerations 中 | pod-fta: BE-SCHED-TAINT |
| RC-004 | **NodeSelector / NodeAffinity 无匹配节点** — Pod 指定的节点选择条件没有任何节点满足 | 中 | D1.3 消息含 `didn't match Pod's node affinity/selector`；D2.3 确认无节点拥有匹配标签 | pod-fta: BE-SCHED-AFFINITY |
| RC-005 | **Pod 反亲和规则导致无可用节点** — PodAntiAffinity 要求 Pod 不与某些 Pod 共存于同一拓扑域，但所有节点都已有冲突 Pod | 中 | D1.3 消息含 `didn't match pod anti-affinity rules`；D2.3 确认 anti-affinity 配置 | pod-fta: BE-SCHED-ANTI-AFFINITY |
| RC-006 | **PVC 未绑定 / StorageClass 不存在** — Pod 挂载的 PVC 处于 Pending 或 Lost 状态，或引用了不存在的 StorageClass | 中 | D1.3 消息含 `unbound immediate PersistentVolumeClaims` 或 `didn't find available persistent volumes`；D2.4 确认 PVC 状态 | pod-fta: BE-SCHED-PVC |
| RC-007 | **ResourceQuota 已用尽** — Namespace 的 ResourceQuota 中 CPU/内存/Pod 数等已达到 Hard 限制 | 中 | D1.3 或 Event 消息含 `exceeded quota`；D2.5 确认 Used ≈ Hard | pod-fta: BE-SCHED-QUOTA |
| RC-008 | **LimitRange 约束不满足** — LimitRange 注入的默认 request/limit 导致 Pod 实际资源需求超出预期 | 低 | D2.6 确认 LimitRange 的 default 值；Pod 未显式设置 request 但被注入了过大值 | pod-fta: BE-SCHED-LIMITRANGE |
| RC-009 | **调度器异常** — kube-scheduler 不健康（CrashLoop、OOM、leader election 失败）导致 Pod 无法被处理 | 低 | D1.4 调度器非 Running；D2.7 日志异常；D1.2 Pod 无任何 Event | scheduler-fta: BE-SCHED-HEALTH |
| RC-010 | **资源碎片化** — 集群总资源充足，但没有单个节点有足够连续可用资源满足 Pod request | 中 | D2.1 集群总量充足但每个节点剩余不足；D2.8 碎片化分析确认 | pod-fta: BE-SCHED-FRAGMENT |
| RC-011 | **自定义调度器未部署** — Pod 的 `schedulerName` 指定了自定义调度器，但该调度器未在集群中运行 | 低 | D1.1 `schedulerName` 非默认值；D1.4 找不到对应调度器 Pod | scheduler-fta: BE-SCHED-CUSTOM |
| RC-012 | **SchedulingGates 阻止调度** — **[v1.28+]** Pod 的 `spec.schedulingGates` 非空，外部控制器未移除门控 | 低 | D1.1 `schedulingGates` 非空；D2.10 确认 gate 列表 | pod-fta: BE-SCHED-GATE |
| RC-013 | **TopologySpreadConstraints 无法满足** — Pod 的拓扑分布约束（`whenUnsatisfiable: DoNotSchedule`）在当前拓扑域分布下无法满足 | 中 | D1.3 消息含 `didn't match pod topology spread constraints`；D2.3 确认 TopologySpreadConstraints 配置 | pod-fta: BE-SCHED-TOPOLOGY |
| RC-014 | **PriorityClass 抢占导致的 Pending 链式反应** — 高优先级 Pod 抢占低优先级 Pod 的资源，导致低优先级 Pod Pending，进而触发连锁反应 | ~4% | Events 中出现 `Preempted` 或 `PreemptionVictim`；D3.3 确认 PriorityClass 配置；低优先级 Pod 被驱逐后无法重新调度 | pod-fta: BE-SCHED-PREEMPTION |
| RC-015 | **GPU/特殊资源调度失败** — Pod 请求了 nvidia.com/gpu、FPGA、RDMA、InfiniBand 等特殊资源，但集群中无可用资源或 device plugin 未正常工作 | ~7% | Pod requests 包含 `nvidia.com/gpu` 或其他扩展资源；节点 allocatable 中无对应资源或为 0；device plugin DaemonSet Pod 异常；**[v1.32+]** ResourceClaim 未绑定 | pod-fta: BE-SCHED-GPU |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 添加缺失的 Toleration
- **适用根因**: RC-003
- **前置检查**:
  ```bash
  # 确认目标 taint 和 Pod 当前 tolerations
  kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, taints: .spec.taints}'
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.tolerations}' | jq .
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 修改 Deployment / StatefulSet 的 Pod template 添加 toleration
  # 以 Deployment 为例
  kubectl patch deployment <deployment-name> -n <namespace> --type=json -p='[
    {
      "op": "add",
      "path": "/spec/template/spec/tolerations/-",
      "value": {
        "key": "<taint-key>",
        "operator": "Equal",
        "value": "<taint-value>",
        "effect": "<NoSchedule|NoExecute|PreferNoSchedule>"
      }
    }
  ]'
  ```
  > **注意**: 修改 Deployment 的 Pod template 会触发滚动更新，创建新的 Pod
- **后置验证**:
  ```bash
  # 等待新 Pod 创建并检查状态
  kubectl rollout status deployment/<deployment-name> -n <namespace> --timeout=120s
  kubectl get pods -n <namespace> -l <selector> -o wide
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 撤销 patch（通过移除 toleration）
  kubectl patch deployment <deployment-name> -n <namespace> --type=json -p='[
    {
      "op": "remove",
      "path": "/spec/template/spec/tolerations/<index>"
    }
  ]'
  # 或回滚到上一版本
  kubectl rollout undo deployment/<deployment-name> -n <namespace>
  ```

#### REM-002: 移除或调整 nodeSelector / nodeAffinity
- **适用根因**: RC-004
- **前置检查**:
  ```bash
  # 确认当前 nodeSelector 和可用节点标签
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeSelector}' | jq .
  kubectl get nodes --show-labels | grep -i "<expected-label-key>"
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  # 方案 A: 为节点添加匹配的标签（如果节点确实应该匹配）
  kubectl label node <node-name> <key>=<value>

  # 方案 B: 移除 Pod 的 nodeSelector（通过修改 Deployment）
  kubectl patch deployment <deployment-name> -n <namespace> --type=json -p='[
    {
      "op": "remove",
      "path": "/spec/template/spec/nodeSelector/<key>"
    }
  ]'
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace> -l <selector> -o wide
  # 确认 Pod 已调度到节点上
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 方案 A 回滚: 移除标签
  kubectl label node <node-name> <key>-

  # 方案 B 回滚:
  kubectl rollout undo deployment/<deployment-name> -n <namespace>
  ```

#### REM-003: 降低 Pod 的 Resource Requests
- **适用根因**: RC-001, RC-002, RC-010
- **前置检查**:
  ```bash
  # 查看 Pod 当前的 resource requests 与实际使用量对比
  kubectl top pod <pod-name> -n <namespace> --containers 2>/dev/null
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"container: "}{.name}{"\n  cpu request: "}{.resources.requests.cpu}{"\n  mem request: "}{.resources.requests.memory}{"\n  cpu limit: "}{.resources.limits.cpu}{"\n  mem limit: "}{.resources.limits.memory}{"\n"}{end}'
  ```
  > **安全检查**: 只有在 Pod 实际使用量远低于 request 时才建议降低。如果 Pod 确实需要这些资源，应该考虑 REM-006 扩容节点
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch deployment <deployment-name> -n <namespace> --type=json -p='[
    {
      "op": "replace",
      "path": "/spec/template/spec/containers/0/resources/requests/cpu",
      "value": "<new-cpu-request>"
    },
    {
      "op": "replace",
      "path": "/spec/template/spec/containers/0/resources/requests/memory",
      "value": "<new-memory-request>"
    }
  ]'
  ```
- **后置验证**:
  ```bash
  kubectl rollout status deployment/<deployment-name> -n <namespace> --timeout=120s
  kubectl get pods -n <namespace> -l <selector> -o wide
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment-name> -n <namespace>
  ```

#### REM-004: 移除 SchedulingGates [v1.28+]
- **适用根因**: RC-012
- **前置检查**:
  ```bash
  # 确认 SchedulingGates 内容
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.schedulingGates}' | jq .
  
  # 确认是否有对应的控制器应该自动移除 gate
  # 如果控制器存在但未工作，应先修复控制器
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 直接移除所有 schedulingGates（适用于门控控制器问题的紧急场景）
  kubectl patch pod <pod-name> -n <namespace> --type=json -p='[
    {
      "op": "remove",
      "path": "/spec/schedulingGates"
    }
  ]'
  ```
  > **注意**: 直接修改 Pod 的 schedulingGates 是允许的（API server 设计如此），但应确认移除 gate 不会导致未就绪的 Pod 被错误调度
- **后置验证**:
  ```bash
  kubectl get pod <pod-name> -n <namespace> -o wide
  # 确认 Pod 已离开 Pending 状态
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 无法直接回滚（Pod 一旦被调度无法再添加 gate）
  # 如需回滚，需删除 Pod 让控制器重新创建
  kubectl delete pod <pod-name> -n <namespace>
  ```

#### REM-005: 调整 TopologySpreadConstraints
- **适用根因**: RC-013
- **前置检查**:
  ```bash
  # 查看当前拓扑分布约束
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.topologySpreadConstraints}' | jq .
  
  # 查看各拓扑域的 Pod 分布情况
  kubectl get pods -n <namespace> -l <selector> -o custom-columns=NAME:.metadata.name,NODE:.spec.nodeName
  kubectl get nodes -o custom-columns=NAME:.metadata.name,ZONE:.metadata.labels.topology\\.kubernetes\\.io/zone
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 A: 将 whenUnsatisfiable 从 DoNotSchedule 改为 ScheduleAnyway
  kubectl patch deployment <deployment-name> -n <namespace> --type=json -p='[
    {
      "op": "replace",
      "path": "/spec/template/spec/topologySpreadConstraints/0/whenUnsatisfiable",
      "value": "ScheduleAnyway"
    }
  ]'

  # 方案 B: 增加 maxSkew 的容忍度
  kubectl patch deployment <deployment-name> -n <namespace> --type=json -p='[
    {
      "op": "replace",
      "path": "/spec/template/spec/topologySpreadConstraints/0/maxSkew",
      "value": 3
    }
  ]'
  ```
- **后置验证**:
  ```bash
  kubectl rollout status deployment/<deployment-name> -n <namespace> --timeout=120s
  kubectl get pods -n <namespace> -l <selector> -o custom-columns=NAME:.metadata.name,NODE:.spec.nodeName,STATUS:.status.phase
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment-name> -n <namespace>
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批）

#### REM-006: 增加 ResourceQuota 限额
- **适用根因**: RC-007
- **影响说明**: 增加 namespace 配额可能导致该 namespace 过度消耗集群资源，影响其他 namespace 的可用资源。需评估集群总体容量。
- **审批提示**: "建议将 namespace `<ns>` 的 ResourceQuota `<quota-name>` 的 CPU 限额从 `<current>` 提升到 `<new>`。当前集群 CPU 总利用率约 X%，影响范围为该 namespace 的 Pod 创建能力。是否批准？"
- **前置检查**:
  ```bash
  # 确认当前配额使用情况
  kubectl describe resourcequota <quota-name> -n <namespace>
  
  # 评估集群总体资源
  kubectl top nodes --no-headers
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch resourcequota <quota-name> -n <namespace> --type=merge -p '{
    "spec": {
      "hard": {
        "requests.cpu": "<new-cpu-limit>",
        "requests.memory": "<new-memory-limit>",
        "pods": "<new-pod-limit>"
      }
    }
  }'
  ```
- **后置验证**:
  ```bash
  kubectl describe resourcequota <quota-name> -n <namespace>
  kubectl get pods -n <namespace> --field-selector status.phase=Pending
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch resourcequota <quota-name> -n <namespace> --type=merge -p '{
    "spec": {
      "hard": {
        "requests.cpu": "<original-cpu-limit>",
        "requests.memory": "<original-memory-limit>",
        "pods": "<original-pod-limit>"
      }
    }
  }'
  ```

#### REM-007: 添加新节点 / 扩容节点池
- **适用根因**: RC-001, RC-002, RC-010
- **影响说明**: 添加节点涉及基础设施变更，可能产生额外成本。对于云环境，触发 Cluster Autoscaler 或手动扩容节点池。
- **审批提示**: "集群资源不足以调度 Pod。建议将节点池从 `<current-count>` 扩容到 `<new-count>` 个节点。预估月增成本约 $X。是否批准？"
- **前置检查**:
  ```bash
  # 确认集群当前节点数和资源状态
  kubectl get nodes -o wide
  kubectl top nodes --no-headers
  
  # 检查 Cluster Autoscaler 状态（如果启用）
  kubectl get pods -n kube-system -l app=cluster-autoscaler
  kubectl get configmap -n kube-system cluster-autoscaler-status -o yaml 2>/dev/null
  ```
- **执行命令**:
  ```bash
  # 云厂商 CLI 示例（需要根据实际环境调整）
  
  # AWS EKS:
  # aws eks update-nodegroup-config --cluster-name <cluster> --nodegroup-name <nodegroup> --scaling-config minSize=X,maxSize=Y,desiredSize=Z
  
  # GKE:
  # gcloud container clusters resize <cluster> --node-pool <pool> --num-nodes <count>
  
  # AKS:
  # az aks nodepool scale --resource-group <rg> --cluster-name <cluster> --name <nodepool> --node-count <count>
  
  # 通用 — 等待新节点就绪
  kubectl get nodes -w
  ```
- **后置验证**:
  ```bash
  kubectl get nodes -o wide
  kubectl get pods -n <namespace> --field-selector status.phase=Pending
  ```
- **回滚命令**:
  ```bash
  # 缩减节点池回原始大小（需确保 PDB 允许 Pod 迁移）
  # 云厂商 CLI 示例（与扩容类似，调整数量即可）
  ```

#### REM-008: 移除或调整 PDB 以允许调度
- **适用根因**: RC-005（间接，PDB 阻止 preemption）
- **影响说明**: 调整 PDB 可能降低服务的高可用保障。在自愿中断（如节点维护）时，PDB 是防止服务完全不可用的关键安全阀。
- **审批提示**: "PDB `<pdb-name>` 的 `minAvailable` 设置为 `<value>`，可能阻止调度器通过抢占释放资源。建议临时调整为 `<new-value>`。是否批准？"
- **前置检查**:
  ```bash
  kubectl get pdb <pdb-name> -n <namespace> -o yaml
  kubectl get pods -n <namespace> -l <pdb-selector> -o wide
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch pdb <pdb-name> -n <namespace> --type=merge -p '{
    "spec": {
      "minAvailable": <new-value>
    }
  }'
  ```
- **后置验证**:
  ```bash
  kubectl describe pdb <pdb-name> -n <namespace>
  kubectl get pods -n <namespace> --field-selector status.phase=Pending
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch pdb <pdb-name> -n <namespace> --type=merge -p '{
    "spec": {
      "minAvailable": <original-value>
    }
  }'
  ```

#### REM-009: 修复 PVC 问题（创建缺失的 StorageClass / 重新绑定）
- **适用根因**: RC-006
- **影响说明**: 存储操作可能涉及数据持久性。错误的 StorageClass 配置可能导致数据丢失或性能问题。
- **审批提示**: "PVC `<pvc-name>` 引用了不存在的 StorageClass `<sc-name>`。建议创建该 StorageClass 或修改 PVC 使用现有的 `<existing-sc>`。是否批准？"
- **前置检查**:
  ```bash
  # 查看 PVC 的期望 StorageClass
  kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.spec.storageClassName}'
  
  # 查看可用的 StorageClass
  kubectl get storageclass
  
  # 查看 PVC Events
  kubectl describe pvc <pvc-name> -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 A: 如果是 StorageClass 名称拼写错误，删除 PVC 重建（注意数据丢失风险）
  # 1. 导出 PVC 定义
  kubectl get pvc <pvc-name> -n <namespace> -o yaml > /tmp/pvc-backup.yaml
  # 2. 修改并重新创建（需要先删除 Pod → PVC → 重建 PVC → 重建 Pod）

  # 方案 B: 如果确实需要创建新 StorageClass
  # kubectl apply -f <storageclass-definition.yaml>
  
  # 方案 C: 如果 PV 已存在但未绑定（手动绑定）
  kubectl patch pv <pv-name> --type=merge -p '{
    "spec": {
      "claimRef": {
        "namespace": "<namespace>",
        "name": "<pvc-name>"
      }
    }
  }'
  ```
- **后置验证**:
  ```bash
  kubectl get pvc <pvc-name> -n <namespace>
  # 预期: STATUS 显示 Bound
  kubectl get pods -n <namespace> --field-selector status.phase=Pending
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 根据具体方案进行回滚
  # 方案 C 回滚: 移除 PV 的 claimRef
  kubectl patch pv <pv-name> --type=json -p='[{"op": "remove", "path": "/spec/claimRef"}]'
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导）

#### REM-010: 移除节点 Taints
- **适用根因**: RC-003
- **影响说明**: 移除节点 taint 会允许所有未设置对应 toleration 的 Pod 调度到该节点。这可能导致不符合预期的工作负载（如非 GPU 任务调度到 GPU 节点、普通应用调度到专用节点）被调度上来，影响节点的专用性和资源隔离。在控制面节点上移除 `node-role.kubernetes.io/control-plane:NoSchedule` 更是高风险操作。
- **操作步骤**:
  1. 确认 taint 的用途和设置原因:
     ```bash
     kubectl describe node <node-name> | grep -A 5 Taints
     ```
  2. 评估移除 taint 的影响范围:
     ```bash
     # 查看有多少 Pod 没有对应 toleration（即移除后可能调度过来的 Pod）
     kubectl get pods --all-namespaces -o json | jq '[.items[] | select(.spec.tolerations == null or (.spec.tolerations | map(.key) | contains(["<taint-key>"]) | not))] | length'
     ```
  3. 移除特定 taint:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

     ```bash
     kubectl taint nodes <node-name> <taint-key>=<taint-value>:<effect>-
     ```
- **安全检查**:
  - ❌ 绝不要移除 `node-role.kubernetes.io/control-plane:NoSchedule`（除非明确要在控制面节点调度工作负载）
  - ❌ 绝不要移除 `node.kubernetes.io/not-ready:NoSchedule`（这是系统自动添加的，表示节点不健康）
  - ⚠️ 移除自定义 taint 前，确认该 taint 的业务含义
- **回滚方案**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl taint nodes`：变更污点影响 Pod 调度

  ```bash
  # 重新添加 taint
  kubectl taint nodes <node-name> <taint-key>=<taint-value>:<effect>
  
  # 如果需要驱逐已调度上来的不当 Pod
  kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --grace-period=30
  # 然后重新 uncordon 并添加 taint
  kubectl uncordon <node-name>
  kubectl taint nodes <node-name> <taint-key>=<taint-value>:<effect>
  ```

#### REM-011: 使用 Descheduler 重新平衡工作负载
- **适用根因**: RC-010（资源碎片化）
- **影响说明**: Descheduler 会驱逐部分 Pod 以实现更好的资源分布。被驱逐的 Pod 将短暂不可用（依赖 Deployment/ReplicaSet 重新创建）。可能触发级联效应。
- **操作步骤**:
  1. 检查是否已部署 Descheduler:
     ```bash
     kubectl get pods --all-namespaces -l app=descheduler
     ```
  2. 如果未部署，使用 Helm 安装:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

     ```bash
     helm repo add descheduler https://kubernetes-sigs.github.io/descheduler/
     helm install descheduler descheduler/descheduler -n kube-system \
       --set schedule="*/5 * * * *" \
       --set deschedulerPolicy.strategies.LowNodeUtilization.enabled=true
     ```
  3. 如果已部署，触发一次性运行:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

     ```bash
     kubectl create job --from=cronjob/descheduler descheduler-manual -n kube-system
     ```
- **安全检查**:
  - 确认所有关键服务都有 PDB
  - 确认 Deployment 的 `maxUnavailable` 设置合理
  - 在非高峰期执行
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 删除 descheduler job 停止驱逐
  kubectl delete job descheduler-manual -n kube-system
  # 被驱逐的 Pod 会被 controller 自动重建
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-012: 生产环境紧急集群扩容
- **适用根因**: RC-001, RC-002（大规模资源不足）
- **审批要求**: 需要 SRE Lead / 基础设施 Manager 审批。涉及成本增加和基础设施变更。
- **数据备份**: 无需数据备份（添加节点不影响现有数据），但需记录当前集群状态快照：
  ```bash
  # 保存当前集群状态
  kubectl get nodes -o yaml > /tmp/nodes-snapshot-$(date +%Y%m%d%H%M%S).yaml
  kubectl get pods --all-namespaces -o wide > /tmp/pods-snapshot-$(date +%Y%m%d%H%M%S).txt
  ```
- **操作步骤**:
  1. 评估需要的节点数量和规格:
     ```bash
     # 计算总缺口
     # Pending Pod 的 total CPU request / 单节点可分配 CPU = 需要的节点数（向上取整）
     ```
  2. 紧急扩容节点池（以主流云厂商为例）:
     ```bash
     # AWS EKS
     aws eks update-nodegroup-config \
       --cluster-name <cluster> \
       --nodegroup-name <nodegroup> \
       --scaling-config minSize=<min>,maxSize=<max>,desiredSize=<desired>
     
     # 等待节点 Ready
     kubectl get nodes -w
     ```
  3. 验证 Pending Pod 开始调度:
     ```bash
     kubectl get pods --all-namespaces --field-selector status.phase=Pending
     ```
  4. 后续: 评估是否需要永久扩容或优化资源使用
- **回滚方案**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

  ```bash
  # 缩容回原始大小（需在 Pending 问题另行解决后）
  # 缩容前确认 PDB 和工作负载安全
  kubectl drain <new-node> --ignore-daemonsets --delete-emptydir-data
  # 然后通过云厂商 CLI 缩减节点池
  ```

#### REM-013: 恢复异常的 kube-scheduler
- **适用根因**: RC-009
- **审批要求**: 需要集群管理员权限。控制面组件操作需谨慎。
- **数据备份**: 备份调度器配置:
  ```bash
  kubectl get pod -n kube-system -l component=kube-scheduler -o yaml > /tmp/scheduler-backup.yaml
  ```
- **操作步骤**:
  1. 检查调度器失败原因:
     ```bash
     kubectl logs -n kube-system -l component=kube-scheduler --tail=100 --previous
     kubectl describe pod -n kube-system -l component=kube-scheduler
     ```
  2. 如果是 OOM:
     ```bash
     # 检查当前资源限制
     kubectl get pod -n kube-system -l component=kube-scheduler -o jsonpath='{.items[0].spec.containers[0].resources}'
     # 需要修改 static pod manifest（/etc/kubernetes/manifests/kube-scheduler.yaml on control plane node）
     ```
  3. 如果是配置错误:
     ```bash
     # 检查调度器配置
     kubectl get configmap -n kube-system kube-scheduler-configuration -o yaml 2>/dev/null
     ```
  4. 紧急措施 — 重启调度器:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

     ```bash
     # 对于 static pod，在控制面节点上:
     # mv /etc/kubernetes/manifests/kube-scheduler.yaml /tmp/ && sleep 5 && mv /tmp/kube-scheduler.yaml /etc/kubernetes/manifests/
     
     # 对于 Deployment 部署的调度器:
     kubectl rollout restart deployment <scheduler-deployment> -n kube-system
     ```
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 使用备份的配置恢复
  kubectl apply -f /tmp/scheduler-backup.yaml
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 1 分钟内）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# V1: 确认目标 Pod 已离开 Pending 状态
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.phase}'
# 预期: Running 或 ContainerCreating（正在启动中，说明已成功调度）

# V2: 确认 Pod 已被分配到节点
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeName}'
# 预期: 返回具体节点名称（非空）

# V3: 确认 Events 中出现 Scheduled 事件
kubectl get events -n <namespace> --field-selector reason=Scheduled,involvedObject.name=<pod-name> --sort-by='.lastTimestamp'
# 预期: 有 Successfully assigned <pod-name> to <node-name>

# V4: 确认 namespace 中无新增 Pending Pod
kubectl get pods -n <namespace> --field-selector status.phase=Pending --no-headers | wc -l
# 预期: 0（或仅保留与本次修复无关的 Pod）

# V5: 如果是 Deployment，确认 rollout 状态
kubectl rollout status deployment/<deployment-name> -n <namespace> --timeout=120s
# 预期: deployment "xxx" successfully rolled out
```
### 7.2 短期监控（5-15 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| Pending Pod 计数 | `kube_pod_status_phase{phase="Pending",namespace="<ns>"}` | 降为 0 | 修复后 5 分钟仍 >0 |
| Pod 调度延迟 | `kube_pod_status_scheduled_time` | 正常范围（<30s） | 持续 >60s |
| 新 FailedScheduling 事件 | `kubectl get events -n <ns> --field-selector reason=FailedScheduling --sort-by='.lastTimestamp'` | 无新事件产生 | 修复后仍有新的 FailedScheduling |
| 节点资源利用率 | `kubectl top nodes` | 稳定，不超过 85% | CPU/Memory >90% |
| Pod Restart 计数 | `kubectl get pods -n <ns> -l <selector>` | RESTARTS 列为 0 | 调度后频繁重启 → 可能是不同问题 |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] 所有目标 Pod 的 `.status.phase` 为 `Running`
- [ ] 所有目标 Pod 的 `.status.conditions` 中 `PodScheduled` 为 `True`
- [ ] 所有目标 Pod 的 `.status.conditions` 中 `Ready` 为 `True`
- [ ] 最近 5 分钟内无新的 `FailedScheduling` 事件
- [ ] 如果是 Deployment / StatefulSet，`availableReplicas` 等于 `replicas`
- [ ] 节点资源利用率在安全范围内（CPU <85%, Memory <85%）
- [ ] 业务层面确认服务正常（如有可用的健康检查端点）

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| Pod 再次进入 Pending | `kubectl get pods -n <ns> --field-selector status.phase=Pending` | 每 30 分钟 | 重新触发此 Skill 诊断 |
| HPA 触发的 Pod 能否正常调度 | 观察 HPA 扩容行为和新 Pod 状态 | 每 1 小时 | 可能需要进一步扩容或调整 request |
| 集群 Autoscaler 行为 | `kubectl get events -n kube-system --field-selector reason=ScaledUpGroup` | 每 1 小时 | 如果 autoscaler 频繁触发，评估基线资源是否充足 |
| ResourceQuota 再次接近上限 | `kubectl describe resourcequota -n <ns>` | 每 4 小时 | 提前扩容配额或优化资源使用 |
| 被调整的 Taint / Affinity 是否被外部系统恢复 | `kubectl get nodes -o json | jq '.items[].spec.taints'` | 每 2 小时 | 如果有自动化工具重新添加 taint，需从源头修复 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 |
|------|------|
| 诊断超时 | 诊断工作流执行超过 15 分钟未确认根因 |
| 修复失败 | 同一修复操作执行 2 次仍未通过 Section 7 验证 |
| 严重性升级 | 初始分级为 P3 但 Pending Pod 数量快速增长超过 10 个，或影响面扩展到生产关键服务 |
| 未知根因 | 诊断完成（Phase 1-3 全部执行）但无法匹配 Section 5 中任何已知根因 |
| 调度器层面问题 | kube-scheduler 本身异常，Agent 无法通过标准修复恢复 |
| 多 Skill 交叉 | 诊断过程中发现问题涉及节点问题（SKILL-NODE-001）+ 调度失败的组合 |

### 8.2 升级消息模板

```
【{severity}】Pod Pending 调度失败 - {cluster_name}/{namespace}
- 问题概述: {pending_pod_count} 个 Pod 处于 Pending 状态，已持续 {duration}
- 影响范围: 
  - Namespace: {namespace}
  - 受影响 Workload: {workload_list}
  - 是否影响生产流量: {yes/no}
- 已完成诊断:
  - Phase 1 快速检查: {completed/skipped}
  - Phase 2 深度检查: {completed_steps}
  - Phase 3 主动探测: {completed_steps}
- 初步发现:
  - FailedScheduling 消息: {scheduling_message}
  - 可能根因: {suspected_rc_ids}
  - 资源状态: CPU {cpu_usage}% / Memory {mem_usage}%
- 已尝试修复:
  - {rem_id}: {result}
- 需要: {action_needed}
  - [ ] 人工确认根因
  - [ ] 审批高风险修复操作
  - [ ] 基础设施变更（扩容）
- 工单编号: {ticket_id}
- 诊断数据包: {data_package_link}
```

### 8.3 交接信息包

升级时，Agent 需准备以下信息（按优先级排列）：

1. **FailedScheduling 完整消息** — 最近一次调度尝试的完整 Event 消息
2. **诊断路径和每步输出** — 按 Phase 整理，包含命令和输出摘要
3. **已排除的根因及原因** — 例如 "RC-003 Taint 已排除：Pod 有匹配的 toleration"
4. **集群资源快照** — `kubectl top nodes` 和 `kubectl describe nodes | grep -A 10 "Allocated resources"` 输出
5. **Pod YAML 快照** — 完整的 Pod spec 和 status
6. **相关 PVC / ResourceQuota / LimitRange 状态** — 如果涉及
7. **调度器日志片段** — 最近 50 行调度器日志
8. **最近 30 分钟的关键事件时间线**:
   ```bash
   kubectl get events -n <namespace> --sort-by='.lastTimestamp' | tail -30
   ```

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| SchedulingGates | Beta | Beta | GA | GA | GA |
| TopologySpreadConstraints `minDomains` | Beta | Beta | GA | GA | GA |
| PodSchedulingReadiness | Beta | GA | GA | GA | GA |
| ReadWriteOncePod PV access mode | Beta | GA | GA | GA | GA |
| Node swap support | Alpha | Alpha | Beta | Beta | GA |
| Scheduler `preemptionPolicy` 字段 | GA | GA | GA | GA | GA |
| In-place Pod resource resize | Alpha | Alpha | Alpha | Beta | Beta |
| MatchLabelKeys in TopologySpreadConstraints | Alpha | Beta | Beta | GA | GA |
| Scheduler performance (QueueingHint) | - | Alpha | Beta | Beta | GA |
| Dynamic Resource Allocation (DRA) | Alpha | Alpha | Beta | Beta | GA |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl get pod -o yaml` — `schedulingGates` 字段 | 可用 (Beta) | 可用 (Beta) | 可用 (GA) | 可用 (GA) | 可用 (GA) |
| `kubectl describe pod` — SchedulingGates 显示 | 显示在 Conditions | 显示在 Conditions | 显示在 Conditions | 显示在 Conditions + 独立区域 | 显示在 Conditions + 独立区域 |
| `kubectl get events` — 调度器 Event 消息格式 | 标准格式 | 标准格式 | 增加了更详细的 preemption 信息 | 同 v1.30 | 增加了 QueueingHint 相关日志 |
| `kubectl top nodes` | 需要 metrics-server | 需要 metrics-server | 需要 metrics-server | 需要 metrics-server | 需要 metrics-server |
| `kubectl debug` — Pod 调试 | Ephemeral containers GA | 同 v1.28 | Custom profiles GA | 同 v1.30 | 同 v1.30 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Pod (`spec.schedulingGates`) | v1 (Beta feature) | v1 (Beta feature) | v1 (GA feature) | v1 (GA feature) | v1 (GA feature) |
| PriorityClass | scheduling.k8s.io/v1 | scheduling.k8s.io/v1 | scheduling.k8s.io/v1 | scheduling.k8s.io/v1 | scheduling.k8s.io/v1 |
| ResourceQuota | v1 | v1 | v1 | v1 | v1 |
| LimitRange | v1 | v1 | v1 | v1 | v1 |
| PodDisruptionBudget | policy/v1 | policy/v1 | policy/v1 | policy/v1 | policy/v1 |
| StorageClass | storage.k8s.io/v1 | storage.k8s.io/v1 | storage.k8s.io/v1 | storage.k8s.io/v1 | storage.k8s.io/v1 |
| CSIDriver | storage.k8s.io/v1 | storage.k8s.io/v1 | storage.k8s.io/v1 | storage.k8s.io/v1 | storage.k8s.io/v1 |

### 9.4 调度器版本特定注意事项

- **v1.28**: SchedulingGates 进入 Beta，默认启用。如果 Pod 长期 Pending 且无 Event，首先检查 `schedulingGates`
- **v1.29**: PodSchedulingReadiness GA。调度器对大量 Pending Pod 的处理效率提升
- **v1.30**: SchedulingGates GA。调度器引入 QueueingHint 框架（Beta），改善调度队列性能。`TopologySpreadConstraints.minDomains` GA
- **v1.31**: TopologySpreadConstraints 的 `matchLabelKeys` GA，允许更精细的拓扑控制。Node swap support 进入 Beta
- **v1.32**: QueueingHint GA，显著改善大规模集群（5000+ 节点）的调度吞吐量。Dynamic Resource Allocation GA，新型资源调度模型

### 9.5 DRA (Dynamic Resource Allocation) 与 GPU 调度演进

> **背景**: Kubernetes v1.32 中 DRA GA 是 GPU/特殊硬件调度的重大变革，替代了传统 Device Plugin 模式。

#### 9.5.1 传统 Device Plugin vs DRA 模式对比

| 特性 | Device Plugin (v1.8+) | DRA (v1.32 GA) |
|-----|----------------------|----------------|
| 资源发现 | 节点级静态注册 | 动态资源声明，支持跨节点 |
| 分配粒度 | 整数单位（1 GPU, 2 GPU） | 支持分片/共享（MIG, time-slicing） |
| 拓扑感知 | 有限支持（NUMA hints） | 原生支持复杂拓扑约束 |
| 声明方式 | `resources.requests["nvidia.com/gpu"]` | `ResourceClaim` + `ResourceClaimTemplate` |
| 调度器集成 | 简单过滤 | 完整调度协商流程 |
| 典型厂商 | NVIDIA device-plugin, AMD device-plugin | NVIDIA DRA driver, Intel GPU plugin (DRA mode) |

#### 9.5.2 版本演进详解

**v1.30 (DRA Beta)**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 DRA 功能门是否启用
kubectl get --raw /metrics | grep dra_enabled
# 或检查 kube-scheduler 启动参数
ps aux | grep kube-scheduler | grep DynamicResourceAllocation
```
- ResourceClaim API 进入 Beta
- 需要显式启用 `DynamicResourceAllocation` feature gate
- GPU 厂商驱动开始适配 DRA 模式

**v1.31 (PodSchedulingReadiness 增强)**:
- SchedulingGates 与 DRA 更好集成
- ResourceClaim 状态更新更可靠
- 调度器对 ResourceClaim 绑定的处理优化
```yaml
# v1.31+ 中 Pod 可以同时使用 SchedulingGates 和 ResourceClaims
spec:
  schedulingGates:
  - name: "resource-prepared"
  resourceClaims:
  - name: gpu
```

**v1.32 (DRA GA)**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 ResourceClaim 状态 (v1.32+)
kubectl get resourceclaims -A
kubectl describe resourceclaim <claim-name>

# 检查 ResourceSlice（DRA 资源池）
kubectl get resourceslices -o wide

# 检查 DeviceClass（替代传统 extended resources）
kubectl get deviceclasses
```
- 默认启用，无需手动开启 feature gate
- 新增 `ResourceSlice` 和 `DeviceClass` API
- 调度器完整支持 structured parameters

#### 9.5.3 DRA 相关 Pending 诊断要点

**v1.32+ 中 GPU Pod Pending 的新诊断路径**:

| 检查项 | 命令 | 期望结果 |
|-------|------|----------|
| ResourceClaim 是否创建 | `kubectl get resourceclaim -l app=<pod-label>` | 存在对应 claim |
| ResourceClaim 是否绑定 | `kubectl get rc <claim> -o jsonpath='{.status.allocation}'` | 非空，包含 allocation 信息 |
| ResourceSlice 是否存在 | `kubectl get resourceslices` | 有可用 slices |
| DRA driver Pod 状态 | `kubectl get pods -n kube-system -l app.kubernetes.io/component=dra-driver` | Running |
| Pending 原因是否包含 DRA 关键字 | 检查 FailedScheduling event | `cannot allocate ResourceClaim` 表示 DRA 相关问题 |

**DRA 模式下 GPU 调度失败的典型 Event 消息**:
```
Events:
  Type     Reason            Message
  ----     ------            -------
  Warning  FailedScheduling  0/10 nodes are available: 
           ResourceClaim my-gpu-claim cannot be allocated because:
           - node1: no device available matching request
           - node2: no device available matching request
           ...
```

#### 9.5.4 DRA 调度问题根因补充

| 根因 | 症状 | 诊断命令 | 修复建议 |
|-----|------|---------|----------|
| ResourceClaim 未被 driver 处理 | `.status.allocation` 持续为空 | `kubectl describe resourceclaim <name>` | 检查 DRA driver Pod 日志 |
| DeviceClass 配置错误 | Event: `DeviceClass not found` | `kubectl get deviceclasses` | 确认 DeviceClass 已创建且名称匹配 |
| ResourceSlice 容量不足 | 所有节点均无可用设备 | `kubectl get resourceslices -o yaml | grep -A5 devices` | 扩容 GPU 节点或检查设备分配状态 |
| structured parameters 语法错误 | ResourceClaim 创建失败 | `kubectl get resourceclaimtemplates -o yaml` | 校验 selector/request 语法 |
| driver 与 DRA API 版本不兼容 | driver CrashLoop 或 API error | 检查 driver 镜像版本与集群版本兼容性 | 升级 DRA driver 至兼容版本 |

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| 将 Taint 问题误诊为资源不足 | FailedScheduling 消息同时包含 `Insufficient cpu` 和 `had taint`（因为部分节点资源不足，部分有 taint），运维人员只关注资源 | 实际是 taint 阻止了资源充足的节点被使用，添加 toleration 即可解决 | 完整解析 FailedScheduling 消息中**每个节点的排除原因**，优先处理 taint/affinity 问题 |
| 将 Affinity 问题误诊为资源不足 | 集群有空闲节点但 Pod 仍 Pending；消息显示 `Insufficient cpu` | nodeSelector 或 nodeAffinity 将 Pod 限制在特定节点上，这些节点确实资源不足，但其他空闲节点不满足 affinity | 检查 Pod 的 nodeSelector 和 affinity 配置，计算**符合约束条件的节点集合**的可用资源 |
| 将 LimitRange 注入的资源误认为是用户设置 | Pod 没有显式设置 resource requests，但实际 request 很大 | LimitRange 自动注入了 `defaultRequest`，导致 Pod 的实际 request 超出预期 | 使用 `kubectl describe limitrange` 检查默认注入值；对比 Pod spec 中用户设置 vs LimitRange 注入 |
| 忽略 SchedulingGates 导致的 "无 Event" Pending | Pod 长时间 Pending 但 Events 为空，怀疑是调度器问题 | **[v1.28+]** Pod 被 SchedulingGates 阻止进入调度队列，调度器根本不会处理它 | D1.1 中首先检查 `spec.schedulingGates` 字段是否非空 |
| 将碎片化问题误诊为 "集群资源够用但调度器有 bug" | `kubectl top nodes` 显示集群总利用率只有 60%，但 Pod 就是调度不了 | 没有单个节点有足够的连续可用资源；例如 Pod 需要 4 CPU，但每个节点只剩 2 CPU | 执行 D2.8 碎片化分析，对比**单节点可用**与 Pod request，而非仅看集群总量 |
| PVC 问题误诊为调度问题 | FailedScheduling 消息含 `unbound immediate PersistentVolumeClaims` | PVC 未绑定可能是 StorageClass 配置错误、provisioner 未部署、或存储后端异常 | 分支诊断 D2.4，独立排查 PVC 问题，可能需要 SKILL-STORE-001 |
| 将 ResourceQuota 耗尽误诊为集群资源不足 | 新 Pod 无法创建，且 Events 中有资源相关错误 | ResourceQuota 限制了 namespace 级别的资源总量，集群实际上有足够资源 | 执行 D2.5，区分 "集群资源不足" 和 "namespace 配额不足" |
| 混淆 `PreferNoSchedule` 和 `NoSchedule` taint | 节点有 taint 但 Pod "应该" 无法调度 | `PreferNoSchedule` 是软约束，调度器在没有其他选择时仍会调度 Pod 到该节点 | 检查 taint 的 effect 字段，只有 `NoSchedule` 和 `NoExecute` 才会硬性阻止 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 参考路径 | 说明 |
|------|---------|------|
| Kubernetes 调度器架构 | `domain-4-workloads-scheduling/` | 调度器工作原理：过滤（Filter）→ 打分（Score）→ 绑定（Bind）流程 |
| FTA 调度故障树 | `故障诊断/FTA故障树/list/scheduler-fta.md` | 调度器问题的完整 FTA 分析模型，含概率和因果链 |
| FTA Pod 故障树 | `故障诊断/FTA故障树/list/pod-fta.md` | Pod 生命周期中所有可能的问题点 |
| 结构化故障排查 — 调度问题 | `故障诊断/高级排障/structural-` | 人类可读的深度排查指南 |
| 通用故障排查方法论 | `故障诊断/` | 系统化故障排查的理论基础和方法 |
| 节点资源管理 | `domain-4-workloads-scheduling/` | Node allocatable、eviction threshold、resource requests/limits 的关系 |
| 存储体系 | `存储/` | PVC/PV/StorageClass 的工作机制和常见问题 |
| 集群弹性伸缩 | `平台工程/` | Cluster Autoscaler、Karpenter 等自动化扩容机制 |
| Pod 优先级与抢占 | `domain-4-workloads-scheduling/` | PriorityClass、preemption 机制、PDB 如何影响抢占 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-03 | v1.0 | 初始版本发布，覆盖 13 个根因分类 | 基于前 200 条 Pod Pending 工单的根因分布统计建立 |

### 10.4 反馈与贡献

如果在使用此 Skill 诊断过程中遇到以下情况，请反馈至知识库维护团队：

- **新根因**: 诊断完成但无法匹配 Section 5 中任何已知根因 → 提交新根因模式
- **误诊纠正**: 按 Skill 指引诊断但实际根因与预测不同 → 更新 Section 10.1
- **版本差异**: 新 Kubernetes 版本引入了影响调度的行为变更 → 更新 Section 9
- **命令变更**: kubectl 命令输出格式变化导致解析失败 → 更新 Section 4 中的命令
- **效率改进**: 发现更快速的诊断路径或更准确的置信度权重 → 提交优化建议

## Related

- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/pod-index.md|Pod 知识图谱索引]]
- [[21-生态参考/03-领域索引/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]

```

<!-- risk-assessed -->
