---
title: 节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation
description: '## 1. 概述'
summary: 'Node NotReady 是 [[Kubernetes|Kubernetes]] 集群中**爆炸半径最大**的问题类型之一。当节点进入 NotReady 状态时，'
category: node
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- controller-manager
- prometheus
- cilium
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
- 节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation 是什么
- 如何 节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation
trigger_keywords:
- NotReady
- NodeNotReady
- 节点不可用
- 节点异常
- kubelet stopped
- node unreachable
- 节点不可达
- NodeStatusUnknown
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
skill_id: SKILL-01_NODE_NOTREADY-001
skill_name: 节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation
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




<!-- condition: kubectl get nodes -o jsonpath='{range .items[?(@.status.conditions[?(@.type=="Ready" && @.status!="True")].nodeName)]}' 显示有 NotReady 节点 -->

# 节点 NotReady 诊断与修复 / Node NotReady Diagnosis & Remediation

---

## 1. 概述

Node NotReady 是 [[Kubernetes|Kubernetes]] 集群中**爆炸半径最大**的问题类型之一。当节点进入 NotReady 状态时，Kubernetes 控制平面（kube-controller-manager 的 node-lifecycle-controller）将在 `pod-eviction-timeout`（默认 5 分钟）后开始驱逐该节点上的所有非 [[DaemonSet|DaemonSet]] Pod，导致大规模服务中断。对于 control plane 节点，NotReady 可能直接威胁集群可用性。

> **版本差异说明 / Version Notes**:
> - `pod-eviction-timeout` 默认 5 分钟，自 v1.28+ 可通过 kube-controller-manager 的 `--node-monitor-grace-period` 调整
> - v1.29+ 引入 **PodDisruptionConditions** (GA)，驱逐的 Pod 会在 `.status.conditions` 中记录 `DisruptionTarget` 原因，便于后续排查
> - v1.28+ **GracefulNodeShutdown** (GA) 使节点在计划关机时可优雅驱逐 Pod，需检查是否因计划内关机导致 NotReady
> - v1.31+ **EventedPLEG** (GA) 替代 GenericPLEG，若 [[kubelet|kubelet]] 日志中出现 `EventedPLEG` 相关错误，诊断方式有所不同（见 D2.6）

### 典型触发场景

1. **kubelet 异常**: kubelet 进程崩溃、OOM、配置错误或无法启动，导致节点无法向 apiserver 上报心跳
2. **容器运行时问题**: containerd / CRI-O 守护进程异常、socket 断开，kubelet 无法执行容器操作，PLEG (Pod Lifecycle Event Generator) 不健康
3. **网络分区**: 节点与 apiserver 之间网络不通（防火墙规则变更、交换机问题、CNI 异常），apiserver 收不到心跳，标记节点为 Unknown → NotReady
4. **资源压力**: 磁盘空间耗尽（DiskPressure）、内存耗尽（MemoryPressure）、PID 耗尽（PIDPressure），触发 kubelet 内置的驱逐管理器
5. **证书过期**: kubelet 客户端证书或 serving 证书过期，无法与 apiserver 建立 TLS 连接

### 前置条件

- **RBAC 权限**:
  - 最小权限: 对 `nodes`, `pods`, `events`, `pods/log`, `pods/status`, `leases` (coordination.k8s.io) 的 `get/list/watch`
  - 如需执行修复: 额外需要 `nodes` 的 `patch`, `pods` 的 `delete/evict`
  - 验证命令: `kubectl auth can-i list nodes && kubectl auth can-i list pods`
- **SSH 访问**: 深度诊断（Phase 2+）需要对问题节点的 SSH 访问权限
- **工具要求**:
  - `kubectl` >= v1.28（客户端版本建议与集群版本相差不超过 1 个 minor）
  - `ssh` 及目标节点登录权限
  - `jq` >= 1.6（可选但推荐用于 JSON 解析）
  - `crictl` >= 1.28（如需直接操作容器运行时）
- **监控系统**: Prometheus + kube-state-metrics >= v2.10（用于 trigger_metrics 匹配）

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | `kubectl get nodes` 输出中节点 STATUS 列显示 `NotReady` / Node shows NotReady status | `kubectl get nodes` 并检查 STATUS 列 | 0.95 | 节点正在进行计划内维护（已标注 maintenance annotation）；新节点初始化尚未完成 |
| S2 | 节点状态在 Ready 和 NotReady 之间频繁切换（flapping）/ Node status flapping between Ready and NotReady | `kubectl get events --field-selector involvedObject.kind=Node --sort-by=.lastTimestamp` 查看短时间内交替出现 NodeReady 和 NodeNotReady 事件 | 0.85 | 节点正在进行滚动升级（kubelet 版本更新）期间短暂状态变更 |
| S3 | 节点上的 Pod 被大量驱逐 / Pods are being evicted from the node | `kubectl get events --field-selector reason=Evicted` 或 `kubectl get pods --field-selector spec.nodeName=<node> --all-namespaces` 显示大量 Evicted Pod | 0.80 | 用户主动执行 `kubectl drain`；HPA 缩容导致的正常 Pod 终止 |
| S4 | 节点 Condition 中 DiskPressure 为 True / Node DiskPressure condition is True | `kubectl describe node <node>` 的 Conditions 表中 DiskPressure=True | 0.90 | 临时性大文件写入已完成，磁盘压力即将自行恢复 |
| S5 | 节点 Condition 中 MemoryPressure 为 True / Node MemoryPressure condition is True | `kubectl describe node <node>` 的 Conditions 表中 MemoryPressure=True | 0.90 | 应用内存突增后已被 OOM Killer 回收，压力可能短暂出现后恢复 |
| S6 | 节点 Condition 中 PIDPressure 为 True / Node PIDPressure condition is True | `kubectl describe node <node>` 的 Conditions 表中 PIDPressure=True | 0.85 | 批处理任务（Job/CronJob）短期内创建大量进程但即将完成 |
| S7 | kubelet 日志中出现 apiserver 连接拒绝 / kubelet logs show connection refused to apiserver | SSH 到节点后 `journalctl -u kubelet --since "10 minutes ago" | grep -i "connection refused|dial tcp|TLS handshake"` | 0.75 | apiserver 正在重启或升级中（计划内变更），短暂连接失败 |
| S8 | 容器运行时 socket 无响应 / Container runtime socket not responding | SSH 到节点后检查 containerd socket: `crictl --runtime-endpoint unix:///run/containerd/containerd.sock info` 超时或报错 | 0.70 | containerd 正在执行 garbage collection，临时无响应（通常 <30s） |
| S9 | Prometheus 告警 `KubeNodeNotReady` 已触发 / Prometheus KubeNodeNotReady alert fired | 检查 Alertmanager 或 Prometheus 中 `kube_node_status_condition{condition="Ready",status="false"} == 1` | 0.95 | 告警系统延迟导致已恢复节点仍有未 resolve 的告警 |
| S10 | 节点上报的 Lease 对象长时间未更新 / Node Lease object not renewed | `kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'` 超过 `node-monitor-grace-period`（默认 40s） | 0.90 | 时钟偏差导致 renewTime 显示异常但节点实际运行正常 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "XX 节点状态异常，显示 NotReady，请排查"
- "集群中有节点不可用，Pod 被驱逐"
- "kubelet 挂了，节点状态变成 NotReady"
- "节点内存/磁盘告警，状态不正常"
- "节点不可达，无法调度 Pod"
- "k8s 节点失联，请尽快处理"
- "线上节点状态抖动，时好时坏"

**English ticket descriptions**:
- "Node is not ready, pods are being evicted"
- "Kubelet stopped posting status, node shows NotReady"
- "Node unreachable, workloads impacted"
- "Disk pressure on node causing evictions"
- "Container runtime down on worker node"
- "Node flapping between Ready and NotReady"
- "Multiple nodes showing Unknown status"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| 节点状态 Ready，但 Pod 处于 CrashLoopBackOff | SKILL-POD-001 | Pod 自身问题，非节点级问题 |
| 节点状态 Ready，但 Pod 长期 Pending | SKILL-POD-002 | 调度问题（资源不足、亲和性约束等），节点本身正常 |
| 节点状态 Ready，但出现证书相关错误 | SKILL-SEC-001 | 证书问题未影响到节点状态，属于安全类问题 |
| 节点被标记为 SchedulingDisabled（已 cordon）但状态为 Ready | 不适用本 Skill | 人工主动操作，非问题 |
| 新建集群中所有节点从未进入 Ready | 集群初始化问题 | 超出本 Skill 范围，需排查 bootstrap 流程 |
| 仅 kubelet 版本偏旧但节点运行正常 | 升级规划 | 版本差异不构成问题 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: 统计 NotReady 节点数量和总节点数
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取所有节点状态，统计 NotReady 数量
kubectl get nodes --no-headers | awk '{print $2}' | sort | uniq -c
# 或更精确的统计
echo "NotReady nodes:" && kubectl get nodes --no-headers | grep -c "NotReady" && \
echo "Total nodes:" && kubectl get nodes --no-headers | wc -l
```
> **判断规则**:
> - NotReady 节点数 / 总节点数 > 50% → **立即升级**（参见 3.3）
> - NotReady 节点数 / 总节点数 > 30% → **P0**
> - NotReady 节点数 > 1 → **P1**
> - NotReady 节点数 == 1 → **P2**（待 T2 进一步确认）

**Step T2**: 确认 NotReady 节点是否为控制平面节点
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 NotReady 节点是否包含 control-plane/master 角色
kubectl get nodes --no-headers | grep "NotReady" | grep -E "control-plane|master"
```
> **判断规则**:
> - 如果有控制平面节点 NotReady → 升级为 **P0**（无论数量）
> - 如果仅工作节点 NotReady → 保持 T1 的分级

**Step T3**: 评估工作负载影响
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 NotReady 节点上运行的 Pod 数量和关键 namespace
NODE_NAME="<notready-node>"
kubectl get pods --all-namespaces --field-selector spec.nodeName=${NODE_NAME} --no-headers | \
  awk '{print $1}' | sort | uniq -c | sort -rn
```
> **判断规则**:
> - kube-system namespace 中有关键组件（如 kube-proxy、CNI Pod）→ 影响集群基础设施
> - 生产 namespace 中有大量 Pod → 直接影响业务
> - 仅有 DaemonSet Pod → 影响相对有限

**Step T4**: 检查 NotReady 持续时间
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查节点 Ready condition 的 lastTransitionTime
kubectl get nodes -o custom-columns=NAME:.metadata.name,STATUS:.status.conditions[-1].type,LAST_TRANSITION:.status.conditions[-1].lastTransitionTime | grep -v "NAME"
```
> **判断规则**:
> - NotReady 持续 > 10 分钟 → Pod 驱逐可能已开始（默认 pod-eviction-timeout=5m）
> - NotReady 持续 < 2 分钟 → 可能是短暂抖动，需持续观察

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| >30% 节点 NotReady **或** 任何控制平面节点 NotReady | **P0** | 集群级问题，影响整体可用性。控制平面节点 NotReady 可能导致 apiserver 不可用（HA 场景下仍降级） | 立即响应，15min 内确认根因 |
| 多个工作节点 NotReady（2-30%） | **P1** | 多节点问题，可能导致部分服务降级或资源不足无法调度 | 15min 内响应，30min 内修复 |
| 单个工作节点 NotReady | **P2** | 单节点问题，影响该节点上的工作负载。如有足够冗余，影响可控 | 30min 内响应，2h 内修复 |
| 新加入的节点从未进入 Ready / 尚未承载业务流量 | **P3** | 新节点问题，不影响现有业务 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **集群级问题**: >50% 的节点处于 NotReady 状态
- **控制平面全部不可用**: 所有 control-plane 节点均 NotReady（etcd 集群可能已丢失 quorum）
- **apiserver 不可达**: `kubectl get nodes` 命令本身超时或失败（无法执行任何诊断命令）
- **级联问题**: NotReady 节点数量在 5 分钟内持续增加（可能是底层基础设施问题）
- **安全事件**: 结合其他安全告警，怀疑节点被入侵导致的异常

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集节点状态信息，无需 SSH 登录节点。所有命令均为只读操作。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 获取节点全局状态概览
- **命令**:
  ```bash
  kubectl get nodes -o wide
  ```
- **超时**: 10s
- **预期输出模式**: 表格输出包含 NAME, STATUS, ROLES, AGE, VERSION, INTERNAL-IP, EXTERNAL-IP, OS-IMAGE, KERNEL-VERSION, CONTAINER-RUNTIME
- **判断规则**:
  - STATUS 列为 `NotReady` → 记录节点名称、IP、版本信息，继续 D1.2
  - STATUS 列为 `Ready,SchedulingDisabled` → 节点已被 cordon，可能是 RC-012（手动操作），跳转 Section 5 确认
  - 命令超时 → apiserver 可能不可用，立即升级（参见 3.3）
- **版本差异**: 无

**Step D1.2**: 获取节点详细状态和 Conditions
- **命令**:
  ```bash
  kubectl describe node <node-name>
  ```
- **超时**: 15s
- **预期输出模式**: 关注以下 Conditions 字段：
  ```
  Conditions:
    Type                 Status  LastHeartbeatTime                 LastTransitionTime                Reason                       Message
    ----                 ------  -----------------                 ------------------                ------                       -------
    MemoryPressure       False   ...                               ...                               KubeletHasSufficientMemory   kubelet has sufficient memory available
    DiskPressure         False   ...                               ...                               KubeletHasNoDiskPressure     kubelet has no disk pressure
    PIDPressure          False   ...                               ...                               KubeletHasSufficientPID      kubelet has sufficient PID available
    Ready                True    ...                               ...                               KubeletReady                 kubelet is posting ready status
  ```
- **判断规则**:
  - `Ready` 状态为 `False`，Reason 为 `KubeletNotReady` → kubelet 无法正常工作，继续 D1.3 并重点关注 kubelet（RC-001）
  - `Ready` 状态为 `Unknown` → apiserver 长时间未收到心跳，可能是网络问题（RC-006）或 kubelet 停止（RC-001）
  - `MemoryPressure` 为 `True` → 记录，可能根因为 RC-004
  - `DiskPressure` 为 `True` → 记录，可能根因为 RC-003
  - `PIDPressure` 为 `True` → 记录，可能根因为 RC-005
  - Message 字段包含 `container runtime is down` → RC-002（容器运行时问题）
  - Message 字段包含 `PLEG is not healthy` → RC-008（PLEG 不健康）
  - Message 字段包含 `certificate` 或 `x509` → RC-007（证书问题），关联 SKILL-SEC-001
- **版本差异**:
  - **[v1.30+]**: 若启用了 Node swap support (beta)，MemoryPressure 计算可能包含 swap 使用量，需结合 `--fail-swap-on=false` 配置判断
  - **[v1.31+]**: 改进的节点状态上报可能包含更详细的 Reason 信息

**Step D1.3**: 检查节点事件
- **命令**:
  ```bash
  kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> \
    --sort-by=.lastTimestamp --no-headers | tail -30
  ```
- **超时**: 10s
- **预期输出模式**: 事件列表，关注 Warning 类型事件
- **判断规则**:
  - 出现 `NodeNotReady` 事件 → 确认 NotReady 时间点
  - 出现 `NodeHasDiskPressure` → 磁盘压力导致（RC-003）
  - 出现 `NodeHasMemoryPressure` → 内存压力导致（RC-004）
  - 出现 `NodeHasPIDPressure` → PID 压力导致（RC-005）
  - 出现 `NodeHasInsufficientMemory` → 内存不足（RC-004）
  - 出现 `InvalidDiskCapacity` → 磁盘配置异常（RC-003 变种）
  - 出现 `Rebooted` → 节点曾重启（关注 RC-009 内核/硬件问题）
  - 出现 `Starting` → kubelet 刚重启过（RC-001 的恢复迹象）
  - 无近期事件 → 可能是网络分区，apiserver 未收到任何更新（RC-006）
- **版本差异**: 无

**Step D1.4**: 检查节点 Taints
- **命令**:
  ```bash
  kubectl get node <node-name> -o jsonpath='{range .spec.taints[*]}{.key}={.value}:{.effect}{"\n"}{end}'
  ```
- **超时**: 5s
- **预期输出模式**: Taint 列表
- **判断规则**:
  - 存在 `node.kubernetes.io/not-ready:NoSchedule` → Kubernetes 自动添加的 taint，确认 NotReady 状态
  - 存在 `node.kubernetes.io/not-ready:NoExecute` → Pod 驱逐已触发
  - 存在 `node.kubernetes.io/unreachable:NoExecute` → 节点不可达
  - 存在 `node.kubernetes.io/unschedulable:NoSchedule` → 节点已被 cordon（RC-012）
  - 存在 `node.kubernetes.io/disk-pressure:NoSchedule` → DiskPressure（RC-003）
  - 存在 `node.kubernetes.io/memory-pressure:NoSchedule` → MemoryPressure（RC-004）
  - 存在 `node.kubernetes.io/pid-pressure:NoSchedule` → PIDPressure（RC-005）
- **版本差异**: 无

**Step D1.5**: 检查节点 Lease 对象
- **命令**:
  ```bash
  kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
  ```
- **超时**: 5s
- **预期输出模式**: ISO 8601 时间戳
- **判断规则**:
  - renewTime 距当前时间 > 40s（默认 node-monitor-grace-period）→ kubelet 未能续租，可能 kubelet 停止（RC-001）或网络不通（RC-006）
  - renewTime 距当前时间 < 40s 但节点仍 NotReady → 可能是 kubelet 报告了不健康状态（检查 Conditions 详情）
- **版本差异**: 无

---

### Phase 2: 深度检查（只读，零风险，需 SSH）

> **目标**: SSH 登录问题节点，检查系统级组件状态。所有命令均为只读操作。
> **前提**: 需要对问题节点的 SSH 访问权限
> **预计耗时**: 5-10 分钟

**Step D2.1**: 检查 kubelet 服务状态
- **命令**:
  ```bash
  ssh <node-ip> "systemctl status kubelet"
  ```
- **超时**: 10s
- **预期输出模式**: systemd unit 状态信息
- **判断规则**:
  - `Active: active (running)` → kubelet 进程在运行，问题可能在运行时层面或网络层面，继续 D2.2
  - `Active: inactive (dead)` → kubelet 未运行（RC-001），尝试查看 D2.2 中的日志了解停止原因
  - `Active: activating (auto-restart)` → kubelet 不断崩溃重启（RC-001），查看 D2.2 中的日志
  - `Active: failed` → kubelet 启动失败（RC-001），查看 D2.2 中的错误日志
  - `Loaded: not-found` → kubelet 服务未安装或 unit 文件丢失，极端情况
- **版本差异**: 无

**Step D2.2**: 检查 kubelet 日志
- **命令**:
  ```bash
  ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager -n 200"
  ```
- **超时**: 15s
- **预期输出模式**: kubelet 日志条目
- **判断规则**:
  - 日志包含 `connection refused` 或 `dial tcp <apiserver-ip>:6443: connect: connection refused` → 网络不通或 apiserver 不可达（RC-006）
  - 日志包含 `x509: certificate has expired` 或 `certificate signed by unknown authority` → 证书问题（RC-007），关联 SKILL-SEC-001
  - 日志包含 `PLEG is not healthy` → PLEG 不健康（RC-008），继续 D2.6
  - 日志包含 `container runtime is not running` 或 `runtime connect using default endpoints` → 容器运行时问题（RC-002）
  - 日志包含 `failed to garbage collect` + 磁盘相关错误 → 磁盘空间不足（RC-003）
  - 日志包含 `OOM` 或 `oom_kill` → 内存压力（RC-004）
  - 日志包含 `too many open files` 或 `no space left on device` → 资源耗尽（RC-003 或 RC-005）
  - 日志包含 `node not found` → 节点对象可能被意外删除
  - 日志包含 `failed to renew lease` → Lease 续租失败，检查网络和 apiserver
  - 日志包含 `use of closed network connection` → 网络连接异常（RC-006）
- **版本差异**:
  - **[v1.28+]**: GracefulNodeShutdown 默认启用。如果日志中出现 `shutting down gracefully`，可能节点正在优雅关机，不一定是问题
  - **[v1.30+]**: swap 相关日志 `swap is enabled` 在启用 NodeSwap feature gate 时属于正常信息

**Step D2.3**: 检查容器运行时（containerd）服务状态
- **命令**:
  ```bash
  ssh <node-ip> "systemctl status containerd"
  ```
- **超时**: 10s
- **预期输出模式**: systemd unit 状态信息
- **判断规则**:
  - `Active: active (running)` → containerd 在运行，继续 D2.4 检查日志
  - `Active: inactive (dead)` 或 `Active: failed` → containerd 未运行（RC-002），需要重启
  - `Active: activating (auto-restart)` → containerd 不断崩溃（RC-002）
- **版本差异**: 无
- **注意**: 部分集群使用 CRI-O 替代 containerd，需检查 `systemctl status crio`

**Step D2.4**: 检查容器运行时日志
- **命令**:
  ```bash
  ssh <node-ip> "journalctl -u containerd --since '30 minutes ago' --no-pager -n 100"
  ```
- **超时**: 15s
- **预期输出模式**: containerd 日志条目
- **判断规则**:
  - 日志包含 `failed to create shim` → shim 进程创建失败，可能磁盘满或 PID 耗尽
  - 日志包含 `context deadline exceeded` → containerd 内部操作超时，可能是磁盘 I/O 过慢
  - 日志包含 `plugin` + `error` → 特定 containerd 插件问题
  - 日志包含 `no space left on device` → 磁盘空间不足（RC-003）
  - 无异常日志 → containerd 正常，问题可能在 kubelet 或网络层
- **版本差异**: 无

**Step D2.5**: 检查系统资源压力
- **命令**:
  ```bash
  # 磁盘使用
  ssh <node-ip> "df -h / /var/lib/kubelet /var/lib/containerd /var/log"

  # 内存使用
  ssh <node-ip> "free -m"

  # PID 使用
  ssh <node-ip> "echo 'Current PIDs:' && ps aux --no-heading | wc -l && echo 'Max PIDs:' && cat /proc/sys/kernel/pid_max"

  # inode 使用（常被忽视的磁盘问题）
  ssh <node-ip> "df -i / /var/lib/kubelet /var/lib/containerd"
  ```
- **超时**: 10s
- **预期输出模式**: 资源使用数据
- **判断规则**:
  - 磁盘使用率 > 85%（kubelet 默认 imagefs.available 驱逐阈值为 15%）→ RC-003
  - 磁盘使用率 > 100%（已满）→ RC-003（紧急）
  - inode 使用率 > 90% → RC-003（inode 耗尽同样导致 DiskPressure）
  - 可用内存 < 100Mi → RC-004
  - PID 数量接近 pid_max（通常默认 32768 或 4194304）→ RC-005
  - swap 使用量大但 `--fail-swap-on=true`（默认）→ 不影响 kubelet，但可能是内存压力信号
- **版本差异**:
  - **[v1.30+]**: 若 NodeSwap feature gate 启用且 kubelet 配置 `swapBehavior: LimitedSwap`，swap 使用是预期行为

**Step D2.6**: 检查 PLEG（Pod Lifecycle Event Generator）健康状态
- **命令**:
  ```bash
  # 检查 kubelet 日志中的 PLEG 相关信息
  ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager | grep -i 'PLEG|pleg'"

  # 检查 kubelet 的 healthz 端点（如果 kubelet 仍在运行）
  ssh <node-ip> "curl -sk https://localhost:10250/healthz"
  ```
- **超时**: 10s
- **预期输出模式**: PLEG 相关日志和 healthz 状态
- **判断规则**:
  - 日志中频繁出现 `PLEG is not healthy` → RC-008。PLEG 不健康通常是由于 container runtime 响应慢，导致 relist 超时（默认 3 分钟）
  - 日志中出现 `GenericPLEG: Unable to retrieve pods` → container runtime 查询失败，关联 RC-002
  - healthz 返回 `ok` → kubelet 内部认为自己健康
  - healthz 返回非 `ok` 或连接失败 → kubelet 不健康
- **版本差异**:
  - **[v1.28+]**: EventedPLEG 作为 beta feature 可用（需手动启用），可减少 PLEG 不健康的误报
  - **[v1.31+]**: PLEG 性能改进，relist 超时处理更优雅

**Step D2.7**: 检查节点到 apiserver 的网络连通性
- **命令**:
  ```bash
  # 获取 apiserver 地址（从 kubelet 配置或 kubeconfig 中读取）
  ssh <node-ip> "cat /etc/kubernetes/kubelet.conf | grep server"

  # 测试网络连通性（不发送 TLS 请求，仅测 TCP 层）
  ssh <node-ip> "nc -zv <apiserver-ip> 6443 -w 5"

  # 或使用 curl 测试（含 TLS）
  ssh <node-ip> "curl -sk --max-time 5 https://<apiserver-ip>:6443/healthz"
  ```
- **超时**: 15s
- **预期输出模式**: 连接成功/失败信息
- **判断规则**:
  - TCP 连接失败 → 网络分区（RC-006），检查防火墙、路由、交换机
  - TCP 成功但 TLS 握手失败 → 证书问题（RC-007）
  - TCP 成功且 TLS 成功但 healthz 返回非 `ok` → apiserver 自身异常（超出本 Skill 范围）
  - 一切正常 → 网络层没问题，回到 D2.1/D2.2 重新检查 kubelet 内部错误
- **版本差异**: 无

**Step D2.8**: 检查 kubelet 证书有效期
- **命令**:
  ```bash
  # 检查 kubelet 客户端证书
  ssh <node-ip> "openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates -subject 2>/dev/null || echo 'Certificate file not found or not readable'"

  # 检查 kubelet serving 证书
  ssh <node-ip> "openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -dates -subject 2>/dev/null || echo 'Certificate file not found or not readable'"
  ```
- **超时**: 10s
- **预期输出模式**: 证书有效期信息（notBefore, notAfter）
- **判断规则**:
  - `notAfter` 早于当前时间 → 证书已过期（RC-007）
  - `notAfter` 在 7 天内 → 证书即将过期，建议预防性轮转
  - 证书文件不存在 → 证书可能被误删或 auto-rotation 失败（RC-007）
  - 证书有效 → 排除证书原因，继续其他诊断
- **版本差异**:
  - **[v1.28+]**: kubelet 证书自动轮转（RotateKubeletClientCertificate）默认启用（GA）
  - **[v1.29+]**: 改进的证书轮转日志，便于审计

**Step D2.9**: 检查内核日志
- **命令**:
  ```bash
  ssh <node-ip> "dmesg -T | tail -50"
  ```
- **超时**: 10s
- **预期输出模式**: 内核日志条目
- **判断规则**:
  - 出现 `Out of memory: Killed process` → OOM Killer 触发（RC-004），记录被杀的进程（如果是 kubelet/containerd 被杀，直接定位根因）
  - 出现 `Hardware Error` 或 `MCE` (Machine Check Exception) → 硬件问题（RC-009）
  - 出现 `I/O error` 或 `device not responding` → 磁盘硬件问题（RC-009）
  - 出现 `NMI watchdog: BUG: soft lockup` → CPU 软锁死（RC-009）
  - 出现 `nf_conntrack: table full` → conntrack 表满，可能影响网络（RC-006 变种）
  - 出现 `EXT4-fs error` 或 `XFS error` → 文件系统错误（RC-009）
  - 无异常条目 → 内核/硬件层面正常
- **版本差异**: 无（与 K8s 版本无关，取决于 OS/内核版本）

**Step D2.10**: 检查 NTP/时间同步
- **命令**:
  ```bash
  # 检查时间同步状态
  ssh <node-ip> "timedatectl status"

  # 或检查 chrony/ntpd 状态
  ssh <node-ip> "chronyc tracking 2>/dev/null || ntpq -p 2>/dev/null || echo 'No NTP service found'"

  # 对比节点时间与本地时间
  ssh <node-ip> "date -u"
  ```
- **超时**: 10s
- **预期输出模式**: 时间同步状态
- **判断规则**:
  - `System clock synchronized: no` → 时间未同步（RC-010）
  - 时间偏差 > 5 秒 → 可能导致证书验证失败和 Lease 续租异常（RC-010）
  - 时间偏差 > 1 分钟 → 严重偏差，几乎确定导致 TLS 握手失败（RC-010 + RC-007）
  - 时间同步正常 → 排除时间原因
- **版本差异**: 无

---

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: 从节点测试 apiserver 健康状态
- **命令**:
  ```bash
  ssh <node-ip> "curl -sk --max-time 10 https://<apiserver-ip>:6443/healthz?verbose"
  ```
- **超时**: 15s
- **风险级别**: 🟢 低（只读 HTTP GET 请求）
- **预期输出模式**: 健康检查各组件状态
- **判断规则**:
  - 所有组件返回 `ok` → apiserver 健康，问题在节点侧
  - 部分组件返回 `failed` → apiserver 自身有问题（如 etcd 连接异常）
  - 连接超时 → 网络不通（RC-006）
  - TLS 握手失败 → 证书问题（RC-007）
- **版本差异**: 无

**Step D3.2**: 检查 CNI 插件状态
- **命令**:
  ```bash
  # 检查 CNI 配置文件是否存在
  ssh <node-ip> "ls -la /etc/cni/net.d/"

  # 检查 CNI 二进制文件
  ssh <node-ip> "ls -la /opt/cni/bin/"

  # 如果使用 Calico，检查 calico-node 容器
  ssh <node-ip> "crictl ps | grep calico"

  # 如果使用 Cilium，检查 cilium agent
  ssh <node-ip> "crictl ps | grep cilium"
  ```
- **超时**: 10s
- **风险级别**: 🟢 低（只读操作）
- **预期输出模式**: CNI 配置和二进制文件列表
- **判断规则**:
  - `/etc/cni/net.d/` 为空 → CNI 插件未配置或配置被删除（RC-011）
  - CNI 二进制文件缺失 → CNI 插件安装不完整（RC-011）
  - calico-node/cilium 容器未运行 → CNI DaemonSet Pod 异常（RC-011）
  - 一切正常 → CNI 插件状态良好
- **版本差异**: 无

**Step D3.3**: 检查 kube-proxy 状态
- **命令**:
  ```bash
  # 检查 kube-proxy Pod 状态（如果使用 DaemonSet 部署）
  kubectl get pods -n kube-system -l k8s-app=kube-proxy --field-selector spec.nodeName=<node-name>

  # 在节点上检查 kube-proxy 进程
  ssh <node-ip> "crictl ps | grep kube-proxy"

  # 检查 iptables/ipvs 规则（判断 kube-proxy 是否正常工作）
  ssh <node-ip> "iptables -t nat -L KUBE-SERVICES 2>/dev/null | head -5 || ipvsadm -Ln 2>/dev/null | head -10"
  ```
- **超时**: 10s
- **风险级别**: 🟢 低（只读操作）
- **预期输出模式**: kube-proxy Pod 状态和规则信息
- **判断规则**:
  - kube-proxy Pod 不存在或 CrashLoopBackOff → kube-proxy 异常（不直接导致 NotReady，但影响 Service 网络）
  - iptables/ipvs 规则为空 → kube-proxy 未能同步规则
  - kube-proxy 正常 → 节点服务代理工作正常
- **版本差异**:
  - **[v1.29+]**: nftables 模式作为 alpha 可用
  - **[v1.31+]**: nftables 模式升级为 beta
  - **[v1.32+]**: nftables 模式 GA

---

### Phase 4: 批量 NotReady 级联故障分析

> **触发条件**: 多个节点同时进入 NotReady 状态（>2 个节点在 5 分钟内）
> **目标**: 分析批量 NotReady 的关联性，确定是独立问题还是共同根因导致的级联问题
> **预计耗时**: 5-15 分钟

**Step D4.1**: 批量节点关联性分析
- **命令**:
  ```bash
  # 获取所有 NotReady 节点的基础信息
  kubectl get nodes --no-headers | grep "NotReady" | awk '{print $1}' | while read node; do
    echo "=== Node: $node ==="
    kubectl get node $node -o jsonpath='IP={.status.addresses[?(@.type=="InternalIP")].address} Zone={.metadata.labels.topology\.kubernetes\.io/zone} Rack={.metadata.labels.topology\.kubernetes\.io/rack}{"\n"}'
  done

  # 检查 NotReady 节点的时间关联性
  kubectl get nodes -o custom-columns=NAME:.metadata.name,STATUS:.status.conditions[-1].type,LAST_TRANSITION:.status.conditions[-1].lastTransitionTime | grep -v "NAME"
  ```
- **超时**: 15s
- **判断规则**:
  - 多个节点在相同时间戳（±1 分钟）进入 NotReady → 可能是网络层面问题或控制平面问题
  - NotReady 节点属于同一 Zone/Rack → 可能是物理网络设备问题（交换机、TOR 问题）
  - NotReady 节点分布在不同 Zone → 可能是控制平面问题或 apiserver 网络问题
  - NotReady 节点 IP 在同一网段 → VLAN/子网问题可能性高

**Step D4.2**: 网络层面排查
- **命令**:
  ```bash
  # 从多个 NotReady 节点之间测试互联性（需要能 SSH 到其中一个）
  # 如果能 SSH 到任一节点：
  ssh <accessible-node-ip> "for ip in <other-node-ip1> <other-node-ip2>; do echo \"Testing \$ip:\"; ping -c 3 \$ip; done"

  # 检查 ARP 表（排除 ARP 风暴/无效 ARP）
  ssh <node-ip> "arp -n | head -20"

  # 检查路由表
  ssh <node-ip> "ip route show"

  # 检查网卡状态（link up/down）
  ssh <node-ip> "ip link show | grep -E 'eth|ens|bond'"
  ```
- **超时**: 30s
- **判断规则**:
  - 节点之间 ping 不通但 SSH 可达 → 云网络/SDN 配置问题
  - 网卡状态 `DOWN` → 物理网络问题
  - ARP 表异常（大量 incomplete/failed）→ 网络交换机问题
  - 路由表缺失默认路由 → 网络配置被破坏

**Step D4.3**: 控制平面健康检查
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查 etcd 集群健康状态
  kubectl get pods -n kube-system -l component=etcd
  kubectl exec -n kube-system etcd-<control-plane-node> -- etcdctl endpoint health --cluster 2>/dev/null || echo "etcdctl not accessible"

  # 检查 apiserver 响应时间
  time kubectl get nodes --request-timeout=5s >/dev/null 2>&1 && echo "apiserver responsive" || echo "apiserver slow/unresponsive"

  # 检查 kube-controller-manager 状态
  kubectl get pods -n kube-system -l component=kube-controller-manager

  # 检查控制平面节点负载
  kubectl top nodes -l node-role.kubernetes.io/control-plane= 2>/dev/null || echo "metrics not available"
  ```
- **超时**: 20s
- **判断规则**:
  - etcd 集群不健康（member 缺失、leader 频繁切换）→ 控制平面根因，立即升级
  - apiserver 响应慢（>3s）→ apiserver 过载或 etcd 问题
  - kube-controller-manager Pod 异常 → node-lifecycle-controller 可能未正确更新节点状态
  - **重要**: 如果控制平面有问题，多节点 NotReady 可能是误报，实际节点可能是健康的

**Step D4.4**: 时钟偏差批量检查
- **命令**:
  ```bash
  # 批量检查所有 NotReady 节点的时间同步状态
  kubectl get nodes --no-headers | grep "NotReady" | awk '{print $1}' | while read node; do
    ip=$(kubectl get node $node -o jsonpath='{.status.addresses[?(@.type=="InternalIP")].address}')
    echo "=== Node: $node ($ip) ==="
    ssh $ip "timedatectl status | grep -E 'synchronized|NTP' 2>/dev/null || date -u" 2>/dev/null || echo "SSH failed"
  done

  # 如果可以登录到节点，检查与 NTP 服务器的偏差
  ssh <node-ip> "chronyc tracking 2>/dev/null | grep 'System time' || ntpq -p 2>/dev/null || echo 'No NTP service'"
  ```
- **超时**: 30s
- **判断规则**:
  - 多节点时钟偏差 >5s → NTP 服务器问题或网络分区导致时钟无法同步
  - 时钟偏差方向一致（都快或都慢）→ NTP 源问题
  - 时钟偏差方向不一致 → 各节点独立的 NTP 配置问题
  - 时钟偏差 >1 分钟 → 几乎确定会导致 TLS 证书验证失败（RC-010 + RC-015 的组合）

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | **kubelet 进程崩溃或未运行** — kubelet 进程因 panic、OOM、配置错误等原因停止运行或反复崩溃重启，无法向 apiserver 发送心跳 | 高 | D2.1 显示 kubelet 未运行；D2.2 日志显示 panic/fatal；D1.5 Lease 未更新 | node-fta: BE-kubelet-crash |
| RC-002 | **容器运行时（containerd）异常** — containerd 或 CRI-O 守护进程停止、崩溃或响应超时，kubelet 无法执行容器操作导致 PLEG 不健康 | 高 | D2.3 显示 containerd 未运行；D2.4 日志有错误；D2.6 PLEG 不健康；D1.2 Message 包含 "container runtime" | node-fta: BE-runtime-failure |
| RC-003 | **节点磁盘空间耗尽（DiskPressure）** — 根分区、/var/lib/kubelet、/var/lib/containerd 或 /var/log 分区磁盘使用率超过驱逐阈值（默认 85%），或 inode 耗尽 | 高 | D1.2 DiskPressure=True；D2.5 磁盘使用率 >85%；D1.3 事件包含 NodeHasDiskPressure | node-fta: BE-disk-pressure |
| RC-004 | **节点内存耗尽（MemoryPressure）** — 节点可用内存低于 kubelet 驱逐阈值（默认 100Mi），触发内存压力条件 | 中 | D1.2 MemoryPressure=True；D2.5 可用内存极低；D2.9 OOM Killer 日志 | node-fta: BE-memory-pressure |
| RC-005 | **节点 PID 耗尽（PIDPressure）** — 节点上进程数量接近或达到 pid_max 限制，kubelet 报告 PID 压力 | 中 | D1.2 PIDPressure=True；D2.5 PID 数量接近上限；D2.2 日志包含 PID 相关错误 | node-fta: BE-pid-pressure |
| RC-006 | **节点与 apiserver 网络不通** — 防火墙规则变更、安全组配置、路由问题、物理网络问题导致节点无法与 apiserver 通信 | 中 | D2.7 TCP 连接失败；D2.2 日志包含 "connection refused"；D1.2 Ready=Unknown | node-fta: BE-network-partition |
| RC-007 | **kubelet 客户端证书过期** — kubelet 用于与 apiserver 通信的客户端证书过期或被吊销，TLS 握手失败 | 中 | D2.8 证书已过期；D2.2 日志包含 "x509"；D2.7 TLS 握手失败 | node-fta: BE-cert-expired |
| RC-008 | **PLEG 不健康导致 NotReady** — Pod Lifecycle Event Generator 的 relist 操作超时（>3min），通常由 container runtime 响应慢引起 | 中 | D2.6 日志出现 "PLEG is not healthy"；D1.2 Message 包含 "PLEG"；D2.3 containerd 延迟高 | node-fta: BE-pleg-unhealthy |
| RC-009 | **内核问题/硬件异常** — 服务器硬件问题（磁盘坏块、内存 ECC 错误、CPU MCE）、内核 panic、文件系统损坏 | 低 | D2.9 dmesg 包含 Hardware Error/MCE/I/O error；节点可能完全无法 SSH | node-fta: BE-hw-failure |
| RC-010 | **NTP 时间不同步** — 节点时钟偏差过大，导致 TLS 证书验证失败和 Lease 续租异常 | 低 | D2.10 时钟未同步或偏差 >5s；D2.8 证书看似有效但 TLS 仍失败 | node-fta: BE-ntp-drift |
| RC-011 | **CNI 插件异常** — CNI 配置文件缺失、CNI 二进制文件损坏、CNI DaemonSet Pod 异常，导致节点网络不可用，kubelet 报告 NetworkUnavailable | 中 | D3.2 CNI 配置缺失或 Pod 未运行；D1.2 NetworkUnavailable=True | node-fta: BE-cni-failure |
| RC-012 | **节点被手动 cordon/drain** — 运维人员手动执行了 `kubectl cordon` 或 `kubectl drain`，节点被标记为 SchedulingDisabled，不属于问题 | 低 | D1.4 存在 unschedulable taint；D1.1 STATUS 包含 "SchedulingDisabled" | N/A（非问题） |
| RC-013 | **内核 panic / 硬件问题** — 服务器发生内核崩溃、MCE (Machine Check Exception)、EDAC 内存错误或其他硬件级别问题，导致节点完全不可用或反复重启 | ~5% | D2.9 dmesg 包含 `kernel panic`、`MCE`、`EDAC` 错误；SSH 可能完全不可达；节点可能反复重启 | node-fta: BE-kernel-panic |
| RC-014 | **云厂商节点池异常** — 云平台层面的问题导致节点不可用，包括 ECS/EC2 实例状态异常、安全组变更、VPC 路由表异常、ENI 配额耗尽、节点池升级卡住等 | ~8% | 云厂商控制台/CLI 显示实例状态异常；D2.7 网络测试失败但非 K8s 层面问题；节点可能无法 SSH | node-fta: BE-cloud-provider |
| RC-015 | **kubelet 证书自动轮转失败** — kubelet 的 RotateKubeletClientCertificate 或 RotateKubeletServerCertificate 机制失败，CSR 未被自动批准或证书轮转过程出错 | ~4% | D2.8 证书已过期或即将过期；D2.2 日志包含 `TLS handshake error`、`certificate has expired`；`kubectl get csr` 显示 Pending CSR | node-fta: BE-cert-rotation-fail |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 取消节点 cordon 标记（Uncordon）
- **适用根因**: RC-012
- **前置检查**:
  ```bash
  # 确认节点确实处于 SchedulingDisabled 状态且非维护状态
  kubectl get node <node-name> -o jsonpath='{.spec.unschedulable}'
  # 预期: true
  # 检查是否有维护 annotation
  kubectl get node <node-name> -o jsonpath='{.metadata.annotations.maintenance\.scheduled}'
  # 预期: 无输出（无维护标记）
  ```
- **执行命令**:
  ```bash
  kubectl uncordon <node-name>
  ```
- **后置验证**:
  ```bash
  kubectl get node <node-name>
  # 预期: STATUS 列显示 Ready（不包含 SchedulingDisabled）
  ```
- **回滚命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

  ```bash
  kubectl cordon <node-name>
  ```

#### REM-002: 清理磁盘空间（容器镜像和日志）
- **适用根因**: RC-003
- **前置检查**:
  ```bash
  # 确认磁盘确实紧张
  ssh <node-ip> "df -h / /var/lib/containerd /var/lib/kubelet /var/log"
  # 确认使用率 > 85%
  ```
- **执行命令**:
  ```bash
  # Step 1: 清理已退出的容器
  ssh <node-ip> "crictl rmi --prune"

  # Step 2: 清理未使用的容器镜像（仅清理无运行容器引用的镜像）
  ssh <node-ip> "crictl rmi --prune"

  # Step 3: 清理旧的日志文件（仅清理已归档的日志）
  ssh <node-ip> "find /var/log -name '*.gz' -mtime +7 -delete 2>/dev/null; \
    find /var/log -name '*.old' -mtime +3 -delete 2>/dev/null"

  # Step 4: 清理 journal 日志（保留最近 2 天）
  ssh <node-ip> "journalctl --vacuum-time=2d"

  # Step 5: 手动触发容器 GC（可选，kubelet 会自动执行）
  # kubelet 的 imageGCHighThresholdPercent 默认 85%
  ```
- **后置验证**:
  ```bash
  ssh <node-ip> "df -h / /var/lib/containerd /var/lib/kubelet /var/log"
  # 预期: 使用率下降到 85% 以下
  kubectl get node <node-name> -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")].status}'
  # 预期: False（可能需要等 1-2 分钟 kubelet 重新评估）
  ```
- **回滚命令**:
  ```bash
  # 磁盘清理为不可逆操作，但删除的仅是缓存/日志，不影响服务
  # 如需恢复镜像，kubelet 会在调度 Pod 时自动拉取
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-003: 重启 kubelet 服务
- **适用根因**: RC-001, RC-008
- **影响说明**: 重启 kubelet 会导致节点上所有 Pod 短暂中断健康检查上报，正在运行的容器不会被终止（除非 kubelet 启动后发现不一致需要重建）。重启过程中节点无法接受新的 Pod 调度。
- **审批提示**: "建议重启节点 `<node-name>` 上的 kubelet 服务。该操作不会终止正在运行的容器，但节点在重启期间（约 10-30s）无法调度新 Pod。是否批准？"
- **前置检查**:
  ```bash
  # 确认 kubelet 确实异常
  ssh <node-ip> "systemctl status kubelet"
  # 预期: inactive/failed/activating 或 active 但日志有错误

  # 记录当前运行的 Pod 列表（用于后续对比）
  kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces -o wide > /tmp/pods-before-restart.txt
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  ssh <node-ip> "systemctl restart kubelet"
  ```
- **后置验证**:
  ```bash
  # 等待 30 秒后检查
  sleep 30

  # 检查 kubelet 状态
  ssh <node-ip> "systemctl status kubelet"
  # 预期: Active: active (running)

  # 检查节点状态
  kubectl get node <node-name>
  # 预期: STATUS 列显示 Ready

  # 检查 Conditions
  kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
  # 预期: Ready=True, 所有压力条件为 False
  ```
- **回滚命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # kubelet 重启为幂等操作，无需回滚
  # 如果重启后问题恶化，可再次停止 kubelet 并升级
  ssh <node-ip> "systemctl stop kubelet"
  # 注意: 停止 kubelet 后节点将确定变为 NotReady
  ```

#### REM-004: 重启 containerd 服务
- **适用根因**: RC-002, RC-008
- **影响说明**: 重启 containerd **会导致节点上所有容器短暂中断**。containerd 重启后会重新 recover 所有已有的 shim 进程，大多数容器会恢复运行。但如果容器的 restart policy 触发，可能导致部分 Pod 重启。这是比重启 kubelet 更具侵入性的操作。
- **审批提示**: "建议重启节点 `<node-name>` 上的 containerd 服务。**该操作会导致该节点上所有容器短暂中断（约 30-60s）**，大部分容器会自动恢复。请确认该节点上的工作负载可以承受短暂中断。是否批准？"
- **前置检查**:
  ```bash
  # 确认 containerd 确实异常
  ssh <node-ip> "systemctl status containerd"

  # 记录当前容器列表
  ssh <node-ip> "crictl ps -a" > /tmp/containers-before-restart.txt 2>/dev/null

  # 检查该节点上是否有 stateful 工作负载
  kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name} ownerKind={.metadata.ownerReferences[0].kind}{"\n"}{end}' | grep -i statefulset
  # 如果有 StatefulSet Pod，需要额外谨慎
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  ssh <node-ip> "systemctl restart containerd"

  # 等待 containerd 完全恢复
  sleep 10

  # 重启 kubelet 以确保重新同步
  ssh <node-ip> "systemctl restart kubelet"
  ```
- **后置验证**:
  ```bash
  # 等待 60 秒后检查
  sleep 60

  # 检查 containerd 状态
  ssh <node-ip> "systemctl status containerd"
  # 预期: Active: active (running)

  # 检查 kubelet 状态
  ssh <node-ip> "systemctl status kubelet"
  # 预期: Active: active (running)

  # 检查节点状态
  kubectl get node <node-name>
  # 预期: STATUS 列显示 Ready

  # 检查 Pod 状态
  kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces
  # 预期: 所有 Pod 恢复 Running 状态
  ```
- **回滚命令**:
  ```bash
  # containerd 重启为幂等操作
  # 如果重启后问题未解决，不要反复重启，应升级处理
  ```

#### REM-005: 调整 kubelet 驱逐阈值
- **适用根因**: RC-003, RC-004, RC-005
- **影响说明**: 修改 kubelet 驱逐阈值配置。需要重启 kubelet 生效。降低驱逐阈值可以暂时缓解资源压力导致的 NotReady，但需要同时解决资源问题的根本原因。
- **审批提示**: "建议调整节点 `<node-name>` 上的 kubelet 驱逐阈值以暂时缓解资源压力。修改后需重启 kubelet，节点将短暂不可用。是否批准？"
- **前置检查**:
  ```bash
  # 查看当前 kubelet 配置的驱逐阈值
  ssh <node-ip> "cat /var/lib/kubelet/config.yaml | grep -A 10 evictionHard"
  # 默认值:
  # evictionHard:
  #   imagefs.available: 15%
  #   memory.available: 100Mi
  #   nodefs.available: 10%
  #   nodefs.inodesFree: 5%
  #   pid.available: -1
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 备份现有配置
  ssh <node-ip> "cp /var/lib/kubelet/config.yaml /var/lib/kubelet/config.yaml.bak"

  # 根据具体资源压力类型调整阈值（示例：降低磁盘阈值）
  # 注意: 这只是临时缓解，必须同步清理磁盘或扩容
  ssh <node-ip> "sed -i 's/imagefs.available: 15%/imagefs.available: 10%/' /var/lib/kubelet/config.yaml"

  # 重启 kubelet 使配置生效
  ssh <node-ip> "systemctl restart kubelet"
  ```
- **后置验证**:
  ```bash
  sleep 30
  kubectl get node <node-name>
  # 预期: STATUS 列显示 Ready

  kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
  # 预期: 压力条件恢复为 False
  ```
- **回滚命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 恢复原始配置
  ssh <node-ip> "cp /var/lib/kubelet/config.yaml.bak /var/lib/kubelet/config.yaml && systemctl restart kubelet"
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-006: 排空节点并重启
- **适用根因**: RC-001, RC-002, RC-008, RC-009
- **影响说明**: 排空（drain）节点将驱逐所有非 DaemonSet Pod，这些 Pod 将被重新调度到其他节点。重启操作会导致该节点上的所有工作负载中断。如果集群资源紧张，被驱逐的 Pod 可能无法被调度到其他节点。
- **操作步骤**:
  1. **确认集群有足够资源接纳被驱逐的 Pod**:
     ```bash
     kubectl top nodes
     # 确认其他节点有足够 CPU 和内存余量
     ```
  2. **排空节点**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

     ```bash
     kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force --grace-period=60 --timeout=300s
     ```
  3. **等待 Pod 完成迁移**:
     ```bash
     kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces
     # 预期: 仅剩 DaemonSet Pod
     ```
  4. **重启节点**:
     ```bash
     ssh <node-ip> "reboot"
     ```
  5. **等待节点恢复**（约 2-5 分钟）:
     ```bash
     # 持续检查节点状态
     watch kubectl get node <node-name>
     ```
  6. **取消 cordon 标记**:
     ```bash
     kubectl uncordon <node-name>
     ```
- **安全检查**:
  - 确认 PodDisruptionBudget (PDB) 不会阻止 drain（`kubectl get pdb --all-namespaces`）
  - 确认无 local storage 的有状态工作负载（emptyDir 数据会丢失）
  - 确认集群其他节点可承载被迁移的 Pod
- **回滚方案**:
  ```bash
  # 如果 drain 过程中需要中止
  # Ctrl+C 中断 drain 命令，然后 uncordon
  kubectl uncordon <node-name>
  # 注意: 已经被驱逐的 Pod 不会自动回到原节点
  ```

#### REM-007: 替换节点（云环境）
- **适用根因**: RC-009, RC-001（反复发生且无法修复时）
- **影响说明**: 在云环境中，直接终止问题节点实例并创建新实例加入集群。这要求集群使用了 node autoscaler 或有手动添加节点的运维流程。
- **操作步骤**:
  1. **排空节点**（同 REM-006 步骤 1-3）
  2. **从集群中删除节点对象**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     kubectl delete node <node-name>
     ```
  3. **在云平台终止实例**（具体命令取决于云平台）:
     ```bash
     # AWS 示例
     aws ec2 terminate-instances --instance-ids <instance-id>
     # 或通过 Node Group / ASG 管理
     ```
  4. **创建新节点**:
     ```bash
     # 如果使用 Cluster Autoscaler，新节点会自动创建
     # 如果手动管理，按集群 join 流程添加新节点
     ```
  5. **验证新节点加入**:
     ```bash
     kubectl get nodes -w
     ```
- **安全检查**:
  - 确认节点上没有 local PV（本地持久化数据会丢失）
  - 确认 node group / ASG 的容量限制允许替换
  - 通知相关 team 即将执行节点替换
- **回滚方案**:
  - 节点替换后无法回滚到原实例
  - 需确保数据已通过其他方式备份（PV、远程存储等）

#### REM-008: 手动证书轮转
- **适用根因**: RC-007
- **影响说明**: 手动批准或重新生成 kubelet 证书。如果自动轮转机制失败，需要手动干预。操作不当可能导致节点永久失联。
- **操作步骤**:
  1. **检查待批准的 CSR**:
     ```bash
     kubectl get csr | grep -i pending
     ```
  2. **如有待批准的 CSR，手动批准**:
     ```bash
     kubectl certificate approve <csr-name>
     ```
  3. **如果无 CSR 或证书已过期，需要重新 bootstrap**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

     ```bash
     # 在节点上删除旧证书
     ssh <node-ip> "rm -f /var/lib/kubelet/pki/kubelet-client-current.pem"

     # 确保 bootstrap token 有效
     kubeadm token list
     # 如无有效 token，创建新 token
     kubeadm token create

     # 重启 kubelet 触发重新 bootstrap
     ssh <node-ip> "systemctl restart kubelet"
     ```
  4. **批准新的 CSR**:
     ```bash
     # 等待新的 CSR 出现
     kubectl get csr --watch
     # 批准
     kubectl certificate approve <new-csr-name>
     ```
- **安全检查**:
  - 确认 CSR 请求来源确实是目标节点（检查 CSR 的 requestor 和 subject）
  - 确认 bootstrap token 的有效期和权限范围
- **回滚方案**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 如果手动轮转导致问题恶化，恢复旧证书（如果有备份）
  ssh <node-ip> "cp /var/lib/kubelet/pki/kubelet-client-current.pem.bak /var/lib/kubelet/pki/kubelet-client-current.pem && systemctl restart kubelet"
  ```

#### REM-011: 内核 panic 后的节点恢复
- **适用根因**: RC-013（内核 panic / 硬件问题）
- **风险等级**: 🔴 高
- **影响说明**: 内核 panic 后的节点可能存在文件系统损坏、硬件问题残留问题。需要确认硬件健康后才能将节点重新投入使用。操作不当可能导致数据丢失或工作负载中断。
- **操作步骤**:
  1. **收集 kdump 日志（如果可用）**:
     ```bash
     # 检查 kdump 是否已捕获崩溃转储
     ssh <node-ip> "ls -la /var/crash/ 2>/dev/null || echo 'No crash dump found'"

     # 检查 kdump 服务状态
     ssh <node-ip> "systemctl status kdump"

     # 备份崩溃转储（用于后续分析）
     scp -r <node-ip>:/var/crash/ /tmp/crash-$(date +%Y%m%d%H%M%S)/
     ```
  2. **确认硬件健康状态**:
     ```bash
     # 检查 BIOS/UEFI POST 自检结果（需要控制台访问）
     # 在云环境中，检查实例状态：
     # AWS: aws ec2 describe-instance-status --instance-ids <instance-id>
     # 阿里云: aliyun ecs DescribeInstanceStatus --RegionId <region> --InstanceId.1 <instance-id>

     # 检查磁盘健康
     ssh <node-ip> "smartctl -H /dev/sda 2>/dev/null || echo 'smartctl not available'"

     # 检查内存错误
     ssh <node-ip> "dmesg -T | grep -iE 'memory error|EDAC|ECC|corrected|uncorrected' | tail -20"

     # 检查 MCE （Machine Check Exception）
     ssh <node-ip> "mcelog --client 2>/dev/null || cat /var/log/mcelog 2>/dev/null || echo 'mcelog not available'"
     ```
  3. **确认节点文件系统完整性**:
     ```bash
     # 检查文件系统错误
     ssh <node-ip> "dmesg -T | grep -iE 'EXT4-fs error|XFS error|I/O error|filesystem' | tail -20"

     # 检查 kubelet 数据目录完整性
     ssh <node-ip> "ls -la /var/lib/kubelet/ && ls -la /var/lib/containerd/"
     ```
  4. **节点重启（如果节点未自动重启）**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

     ```bash
     # 先排空节点工作负载
     kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force --grace-period=60 --timeout=300s

     # 重启节点
     ssh <node-ip> "reboot"
     ```
  5. **重启后验证**:
     ```bash
     # 等待节点重启（约 2-5 分钟）
     sleep 120

     # 检查节点状态
     kubectl get node <node-name>

     # 检查 kubelet 状态
     ssh <node-ip> "systemctl status kubelet"

     # 检查 containerd 状态
     ssh <node-ip> "systemctl status containerd"

     # 确认无新的内核错误
     ssh <node-ip> "dmesg -T | grep -iE 'error|panic|oops' | tail -20"
     ```
- **安全检查**:
  - 硬件自检通过（磁盘 SMART 正常、无 MCE 错误）
  - 文件系统无损坏
  - kdump 日志已备份用于后续分析
  - **如果硬件检查失败**：不要将节点重新投入使用，转入 REM-010（硬件更换）流程
- **回滚方案**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

  ```bash
  # 如果重启后节点仍不稳定，标记节点为不可调度
  kubectl cordon <node-name>

  # 通知基础设施团队进行硬件检查
  # 建议创建维护工单并记录：
  # - kdump 日志位置
  # - dmesg 错误输出
  # - 疑似问题硬件组件

  # 如果确认是硬件问题，按 REM-010 流程更换硬件或 REM-007 更换节点
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-009: 内核热补丁或 OS 升级
- **适用根因**: RC-009
- **审批要求**: 需要高级 SRE + 基础设施 Team Lead 审批
- **数据备份**: 升级前确保节点上无 local PV 数据，或已完成数据备份
- **操作步骤**:
  1. **排空节点**（同 REM-006）
  2. **评估内核问题**:
     ```bash
     ssh <node-ip> "uname -r"
     ssh <node-ip> "dmesg -T | grep -i 'bug|error|panic|oops'"
     ```
  3. **应用内核补丁**（具体取决于 OS 发行版）:
     ```bash
     # RHEL/CentOS
     ssh <node-ip> "yum update kernel -y"

     # Ubuntu
     ssh <node-ip> "apt-get update && apt-get install linux-image-generic -y"
     ```
  4. **重启节点以应用新内核**:
     ```bash
     ssh <node-ip> "reboot"
     ```
  5. **等待节点恢复并验证**
- **回滚方案**:
  - 大多数 Linux 发行版支持在 GRUB 中选择旧内核启动
  - 如果新内核导致问题，通过 IPMI/iLO/云控制台重启到旧内核
  ```bash
  # 查看可用内核列表
  ssh <node-ip> "grep menuentry /boot/grub2/grub.cfg"
  # 设置默认启动为旧内核
  ssh <node-ip> "grub2-set-default 1 && reboot"
  ```

#### REM-010: 硬件更换
- **适用根因**: RC-009
- **审批要求**: 需要高级 SRE + 数据中心运维 Team 审批
- **数据备份**: 确认所有需要保留的数据已备份到外部存储
- **操作步骤**:
  1. **排空节点并从集群中移除**（同 REM-007 步骤 1-2）
  2. **提交数据中心硬件更换工单**:
     - 记录问题硬件信息（服务器型号、序列号、问题组件）
     - 附上 dmesg 和硬件诊断日志
  3. **硬件更换完成后**:
     - 重新安装 OS 和 K8s 组件
     - 按集群 join 流程重新加入节点
  4. **验证新硬件**:
     ```bash
     # 运行硬件诊断
     ssh <new-node-ip> "smartctl -a /dev/sda"  # 磁盘健康
     ssh <new-node-ip> "mcelog --client"        # CPU/内存错误
     ```
- **回滚方案**:
  - 硬件更换为不可逆操作
  - 保留问题硬件的日志和诊断信息用于事后分析

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# V1: 确认节点状态恢复为 Ready
kubectl get node <node-name>
# 预期: STATUS 列显示 Ready

# V2: 确认所有 Conditions 恢复正常
kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
# 预期输出:
# MemoryPressure=False
# DiskPressure=False
# PIDPressure=False
# Ready=True

# V3: 确认 Node Lease 正常续租
kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
# 预期: 时间戳为最近几秒内

# V4: 确认 Pod 恢复调度和运行
kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces
# 预期: Pod 状态为 Running

# V5: 确认节点上 kubelet 版本和运行时信息正确
kubectl get node <node-name> -o jsonpath='kubelet={.status.nodeInfo.kubeletVersion} runtime={.status.nodeInfo.containerRuntimeVersion}'
# 预期: 版本信息与集群其他节点一致
```
### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| 节点 CPU 使用率 | `node_cpu_seconds_total` 或 `kubectl top node <node-name>` | 恢复后 CPU 使用率稳定在正常范围 | CPU 使用率持续 >90% 超过 5 分钟 |
| 节点内存使用率 | `node_memory_MemAvailable_bytes` 或 `kubectl top node <node-name>` | 可用内存保持在驱逐阈值以上 | 可用内存 <200Mi 且持续下降 |
| 节点磁盘使用率 | `node_filesystem_avail_bytes` 或 SSH `df -h` | 磁盘使用率保持在 85% 以下 | 磁盘使用率持续上升并再次接近阈值 |
| kubelet 运行中 Pod 数 | `kubelet_running_pods` | Pod 数量恢复到问题前水平 | Pod 数量持续为 0 或远低于预期 |
| kubelet 心跳 | `kube_node_status_condition{condition="Ready",status="true"}` | 持续为 1 | 值变为 0（节点再次 NotReady） |
| PLEG 延迟 | `kubelet_pleg_relist_duration_seconds` | P99 < 10s | P99 > 60s 或 relist 超时 |
| 容器重启次数 | `kube_pod_container_status_restarts_total` | 无异常增长 | 修复后容器重启次数持续增加 |
| 节点事件 | `kubectl get events --field-selector involvedObject.name=<node-name>` | 无新的 Warning 事件 | 出现新的 NodeNotReady 或资源压力事件 |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] 节点 STATUS 显示 Ready，且持续 Ready 超过 5 分钟
- [ ] 所有 Conditions（MemoryPressure, DiskPressure, PIDPressure）均为 False
- [ ] Node Lease 正常续租（renewTime 持续更新）
- [ ] 节点上的 Pod 已恢复正常运行（Running 状态）
- [ ] kubelet 和 containerd 进程稳定运行（无崩溃重启）
- [ ] 节点系统资源（CPU、内存、磁盘、PID）处于安全水位
- [ ] 无新增 Warning 事件
- [ ] 根因已明确记录并采取了预防措施（如需要）

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| 节点状态稳定性 | `kube_node_status_condition{condition="Ready"}` 监控 | 持续 | 如果再次 NotReady → 重新进入本 Skill 诊断流程 |
| 磁盘使用趋势 | `node_filesystem_avail_bytes` 趋势图 | 每小时 | 使用率线性增长 → 排查磁盘空间消耗源头（日志、镜像缓存） |
| 内存使用趋势 | `node_memory_MemAvailable_bytes` 趋势图 | 每小时 | 可用内存线性下降 → 排查内存泄漏 Pod |
| kubelet 重启次数 | `kubelet` systemd service 重启计数 | 每 4 小时 | 24h 内重启 >2 次 → 深度排查 kubelet 崩溃原因 |
| OOM 事件 | `dmesg | grep -i oom` | 每 4 小时 | 新的 OOM 事件 → 检查内存限制配置 |
| 证书有效期 | `openssl x509 ... -noout -enddate` | 每日 | 有效期 <7 天 → 预防性轮转或检查自动轮转机制 |
| 节点上 Pod 调度 | `kubectl get pods --field-selector spec.nodeName=<node-name>` | 每 4 小时 | 新 Pod 无法调度到该节点 → 检查 taints 和 node conditions |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **10 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后 V1-V5 验证失败 |
| **严重性升级** | 初始分级为 P2 但影响面扩大（如更多节点变为 NotReady） | 诊断过程中 NotReady 节点数增加 |
| **未知根因** | 完成 Phase 1-3 所有诊断步骤但无法匹配任何已知根因（RC-001 至 RC-012） | 所有诊断步骤均无明确异常发现 |
| **操作权限不足** | Agent 或操作人员无 SSH 访问权限，无法执行 Phase 2+ 诊断 | Phase 1 完成后需要 SSH 但无权限 |
| **安全疑虑** | 诊断过程中发现可疑安全指标（异常进程、未知网络连接） | 任何诊断步骤中发现安全异常 |

### 8.2 升级消息模板

```
【{severity}】节点 NotReady 诊断与修复 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: 节点 {node_name} ({node_ip}) 状态为 NotReady，持续 {duration}
- 影响范围: 
  - 受影响节点: {affected_node_count}/{total_node_count}
  - 受影响 Pod: {affected_pod_count} 个（namespace: {affected_namespaces}）
  - 是否涉及控制平面: {control_plane_affected}
- 已完成诊断:
  - Phase 1 快速检查: {phase1_summary}
  - Phase 2 深度检查: {phase2_summary}
  - Phase 3 主动探测: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-NODE-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤（D1.1 → D1.2 → ... → D3.3）及每步输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
   - 例: "RC-003 已排除 — D2.5 显示磁盘使用率 42%，低于阈值"
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
   - 例: "疑似 RC-006（网络分区）— D2.7 TCP 测试超时，但 D2.2 日志中无明确连接拒绝信息"
4. **关键资源快照**:
   ```bash
   # 节点描述
   kubectl describe node <node-name> > node-describe.txt
   # 节点事件
   kubectl get events --field-selector involvedObject.name=<node-name> --sort-by=.lastTimestamp > node-events.txt
   # 节点上的 Pod 状态
   kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces -o wide > node-pods.txt
   # kubelet 日志（最近 1 小时）
   ssh <node-ip> "journalctl -u kubelet --since '1 hour ago' --no-pager" > kubelet-logs.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列
   - `HH:MM:SS` - 首次检测到 NotReady
   - `HH:MM:SS` - 开始诊断
   - `HH:MM:SS` - 发现异常 [描述]
   - `HH:MM:SS` - 尝试修复 [操作]
   - `HH:MM:SS` - 修复结果 [成功/失败]
   - `HH:MM:SS` - 决定升级

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| GracefulNodeShutdown | GA（默认启用） | GA | GA | GA | GA |
| Node swap support | alpha | alpha | beta（默认关闭） | beta | beta |
| kubelet 证书自动轮转 (RotateKubeletClientCertificate) | GA（默认启用） | GA | GA | GA | GA |
| kubelet 证书自动轮转 (RotateKubeletServerCertificate) | beta（默认启用） | beta | GA | GA | GA |
| EventedPLEG | beta（默认关闭） | beta（默认关闭） | beta（默认关闭） | beta（默认启用） | GA |
| `kubectl debug node/` | GA | GA | GA | GA | GA |
| Custom Debug Profiles | beta | beta | GA | GA | GA |
| NodeStatus 上报改进 | 基础 | 优化心跳频率 | 改进 Lease 上报 | 增强状态报告详细度 | 稳定 |
| Sidecar Containers | alpha | beta | beta | GA | GA |
| Node Resource Fit Scoring | 基础 | 基础 | 改进 | 改进 | 增强 |
| PodDisruptionConditions | beta | GA | GA | GA | GA |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl debug node/<name>` | 支持，使用 `--image` 指定调试镜像 | 同左 | 新增 `--profile` 参数（GA） | 同左 | 同左 |
| `kubectl get --raw /api/v1/nodes/<name>/proxy/healthz` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get --raw /api/v1/nodes/<name>/proxy/configz` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl top node` (metrics-server) | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get lease -n kube-node-lease` | 支持（v1.17+ GA） | 同左 | 同左 | 同左 | 同左 |
| `crictl` 版本要求 | >=1.28 | >=1.29 | >=1.30 | >=1.31 | >=1.32 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Node | v1 (core) | v1 | v1 | v1 | v1 |
| Lease | coordination.k8s.io/v1 | v1 | v1 | v1 | v1 |
| Event | events.k8s.io/v1 | v1 | v1 | v1 | v1 |
| CSR (CertificateSigningRequest) | certificates.k8s.io/v1 | v1 | v1 | v1 | v1 |
| RuntimeClass | node.k8s.io/v1 | v1 | v1 | v1 | v1 |

### 9.4 版本相关的诊断注意事项

- **[v1.28+]**: GracefulNodeShutdown 默认启用。当节点正在关机时，kubelet 会尝试优雅终止 Pod。在诊断时需注意区分计划关机和异常关机：
  - 检查 `shutdownGracePeriod` 和 `shutdownGracePeriodCriticalPods` 配置
  - 日志中出现 `shutting down gracefully` 不一定是问题

- **[v1.30+]**: Node swap support (beta) 可能影响内存压力的判断：
  - 如果 `NodeSwap` feature gate 启用且 `swapBehavior: LimitedSwap`，需同时检查 swap 使用情况
  - `free -m` 输出中的 Swap 行不再是“异常”信号
  - kubelet 的 `--fail-swap-on` 标志在启用 swap 时为 `false`

- **[v1.31+]**: EventedPLEG 默认启用：
  - 传统 GenericPLEG 的 relist 操作频率降低，`PLEG is not healthy` 误报减少
  - 但如果 EventedPLEG 本身异常，可能出现新的故障模式
  - 诊断时需检查 `--feature-gates=EventedPLEG=true` 是否生效
  - **新增**: kubelet graceful shutdown 行为增强，支持更精细的 Pod 终止顺序控制
  - **新增**: Pod 的 `terminationGracePeriodSeconds` 会被 kubelet 在 graceful shutdown 期间更准确地尊重

- **[v1.32+]**: nftables kube-proxy 模式 GA：
  - 使用 nftables 模式时，`iptables -L` 不再显示 kube-proxy 规则
  - 需使用 `nft list ruleset` 检查规则
  - **新增**: InPlacePodVerticalScaling (Beta) 对节点资源压力的影响：
    - Pod 可以在运行时动态调整 CPU/Memory requests 和 limits
    - 可能导致节点 allocated resources 突然变化
    - 诊断 MemoryPressure/DiskPressure 时需考虑 resize 操作的影响
  - **新增**: kubelet 证书轮转日志更详细，便于诊断 RC-015

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **网络抖动误判为 kubelet 崩溃** | Node Condition 中 Ready=Unknown，看似 kubelet 停止发送心跳 | 网络链路不稳定（交换机端口 flapping、MTU 问题、云网络限流），kubelet 实际在运行但心跳包被丢弃 | 先 SSH 到节点确认 kubelet 进程状态（D2.1），再测试网络连通性（D2.7）。如果 kubelet 运行正常且本地 healthz 正常，优先排查网络 |
| **DiskPressure 归因于镜像过多，实则是日志轮转失败** | DiskPressure=True，磁盘使用率高 | 容器日志（stdout/stderr）未正确配置轮转（logMaxSize/logMaxFiles），单个 Pod 的日志占用几十 GB | 在 D2.5 中不仅检查整体磁盘使用率，还要检查 `/var/log/pods/` 或 `/var/log/containers/` 下的大文件：`du -sh /var/log/pods/* | sort -rh | head -10` |
| **PLEG 不健康误判为容器运行时问题** | kubelet 日志出现 `PLEG is not healthy`，初步判断为 containerd 异常 | 实际是某个 Pod 的 container 处于 D 状态（不可中断的 I/O 等待），阻塞了 CRI 调用，containerd 本身正常 | 在 D2.6 之后检查是否有 D 状态进程：`ps aux | awk '$8=="D"'`。如果有，定位到具体容器和 Pod，问题在应用层而非运行时 |
| **证书过期误判为网络问题** | kubelet 日志出现 "connection refused" 或 TLS 错误 | kubelet 客户端证书已过期，TLS 握手失败被解读为网络问题 | 在排查网络问题（D2.7）前先检查证书有效期（D2.8）。TLS 握手失败和 TCP 连接失败有本质区别 |
| **cordon 操作误判为节点问题** | 用户报告 Pod 无法调度到某节点，误认为节点 NotReady | 运维人员之前执行了 `kubectl cordon` 但未记录，节点状态为 `Ready,SchedulingDisabled` | D1.1 中仔细区分 `NotReady` 和 `Ready,SchedulingDisabled`；D1.4 检查 taints 中的 `unschedulable` 标记 |
| **时间偏差导致的间歇性问题** | 节点状态不稳定，时好时坏，难以找到明确根因 | 节点 NTP 未同步，时钟偏差导致 TLS 证书间歇性验证失败和 Lease 续租异常 | 在诊断早期（D2.10）就检查时间同步。时间偏差是最容易被忽视但影响广泛的根因 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| kubelet 架构与内部机制 | `集群基础/` | 理解 kubelet 心跳机制、node-lifecycle-controller 的驱逐逻辑 |
| Node 故障树分析 | `故障诊断/topic-fta/list/node-fta.md` | 理解 Node NotReady 的完整因果链和概率模型 |
| 节点级故障排查深度指南 | `故障诊断/topic-structural-trouble-shooting/` | 超出本 Skill 覆盖范围的深度排查方法 |
| Kubernetes 故障排查方法论 | `故障诊断/` | 系统化故障排查的理论基础和方法论 |
| 证书管理与 TLS | `SKILL-SEC-001` (06-certificate-expiry.md) | kubelet 证书过期的详细诊断与修复（本 Skill 的 RC-007 关联） |
| Pod 驱逐与调度 | `SKILL-POD-002` (03-pod-pending.md) | 节点恢复后 Pod 重新调度的相关问题 |
| 容器运行时排障 | `故障诊断/topic-structural-trouble-shooting/` | containerd/CRI-O 深度排查 |
| Linux 内核排障 | `故障诊断/` | OOM Killer、内核 panic、硬件错误的深度分析 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-03 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 12 个根因、10 个修复操作 | 首批 Skill 库建设，基于 top 工单分析确定节点 NotReady 为最高优先级场景 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **GPU 节点 NotReady**: GPU 驱动异常导致的节点 NotReady 场景（NVIDIA device plugin crash, GPU memory error）
2. **Windows 节点**: Windows 容器节点的 NotReady 诊断差异（kubelet on Windows, containerd on Windows）
3. **ARM 架构节点**: ARM 节点的特定故障模式
4. **边缘节点**: 使用 KubeEdge / OpenYurt 等边缘方案的节点 NotReady 诊断差异（弱网环境、离线容忍）
5. **虚拟节点**: Virtual Kubelet 实现的虚拟节点 NotReady 诊断

## 修复动作

> **本章定位**: 基于 Section 6 修复操作的快速决策摘要，供 Agent 在 QA 语料和运行时直接引用。所有命令均保留风险标注。

### 修复动作速查表

| 根因 | 修复动作 | 风险 | 验证命令 |
|------|---------|------|---------|
| RC-012 节点被 cordon | `kubectl uncordon <node-name>` | 🟢 低风险 | `kubectl get node <node-name>` |
| RC-003 磁盘压力 | `ssh <node> "crictl rmi --prune && journalctl --vacuum-time=2d"` | 🟢 低风险（仅清理缓存/日志） | `kubectl get node <node> -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")].status}'` |
| RC-001 kubelet 异常 | `ssh <node> "systemctl restart kubelet"` | 🟡 中风险（节点短暂不可调度） | `ssh <node> "systemctl status kubelet" && kubectl get node <node>` |
| RC-002 containerd 异常 | `ssh <node> "systemctl restart containerd && systemctl restart kubelet"` | 🟡 中风险（容器短暂中断 30-60s） | `kubectl get pods --field-selector spec.nodeName=<node> --all-namespaces` |
| RC-005 资源压力阈值 | 调整 kubelet evictionHard 后重启 kubelet | 🟡 中风险（需重启 kubelet） | `kubectl get node <node> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'` |
| RC-007 证书过期 | 手动证书轮转或触发 CSR 批准 | 🟡 中风险（涉及 TLS 重建） | `openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates` |
| RC-001/RC-008 复杂问题 | `kubectl drain <node> --ignore-daemonsets --force` → 修复后 `kubectl uncordon <node>` | 🔴 高风险（驱逐所有 Pod） | `kubectl get pods --field-selector spec.nodeName=<node> --all-namespaces` |
| RC-009 硬件问题 | 节点替换（云环境终止实例并新建） | 🔴 高风险（数据可能丢失） | `kubectl get nodes` |

### danger_operations 高风险操作标注

以下操作涉及数据丢失或服务中断，**必须人工审批**后方可执行，Agent 仅提供指导：

```yaml
danger_operations:
  - operation: "kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --force"
    risk: "驱逐节点上所有非 DaemonSet Pod，可能导致服务中断；emptyDir 数据丢失"
    prerequisite:
      - "确认集群其他节点有足够资源接纳被驱逐的 Pod"
      - "确认无 local storage 的有状态工作负载"
      - "检查 PodDisruptionBudget 不会阻止 drain"
    rollback: "kubectl uncordon <node>（已被驱逐的 Pod 不会自动回到原节点）"

  - operation: "ssh <node> 'reboot'"
    risk: "节点重启期间所有工作负载中断"
    prerequisite:
      - " drain 完成且仅剩 DaemonSet Pod"
      - "确认节点非 etcd/control-plane 唯一成员"

  - operation: "kubectl delete node <node-name> && aws ec2 terminate-instances --instance-ids <id>"
    risk: "节点对象和云实例同时删除，local PV 数据永久丢失"
    prerequisite:
      - "确认节点上没有 local PV"
      - "确认数据已通过远程存储或备份保留"
      - "通知相关团队"
    rollback: "无法回滚到原实例，需重新 join 新节点"
```

### 通用验证步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认节点恢复 Ready
kubectl get node <node-name>

# 2. 确认所有压力条件为 False
kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'

# 3. 确认 Lease 正常续租
kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'

# 4. 确认 Pod 恢复正常运行
kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces
```
## Related

- [[生态参考/topic-index/node-index.md|Node 知识图谱索引]]

```

<!-- risk-assessed -->
