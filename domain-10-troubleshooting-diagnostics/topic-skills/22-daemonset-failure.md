---
title: DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation
description: '- 运维工程师'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- controller-manager
- cilium
- flannel
- calico
- daemonset
- operator
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation 是什么
- 如何 DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation 故障排查
- DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation 排障步骤
trigger_keywords:
- DaemonSet
- 故障诊断与修复
- DaemonSet
- Failure
- Diagnosis
- Remediation
- troubleshooting
- diagnostics
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- cilium-basics
- cni-basics
- logging-basics
skill_id: SKILL-22_DAEMONSET_FAILURE-001
skill_name: DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation
version: 1.0.0
created: "2026-05-23"
---

---
skill_id: "SKILL-WORK-003"
skill_name: "[[DaemonSet|DaemonSet]] 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation"
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
  - "DaemonSet"
  - "daemonset"
  - "ds"
  - "节点缺少 Pod"
  - "node missing pod"
  - "kube-proxy"
  - "calico-node"
  - "[[Cilium|cilium]]"
  - "[[domain-19-landscape-references/01-cncf-landscape/graduated/fluentd/fluentd|[[Fluentd|fluentd]]]]"
  - "node-exporter"
  - "系统组件"
  - "污点"
  - "taint"
trigger_events:
  - "FailedDaemonPod"
  - "FailedPlacement"
  - "InsufficientResource"
  - "TaintManagerEviction"
trigger_metrics:
  - 'kube_daemonset_status_desired_number_scheduled - kube_daemonset_status_current_number_scheduled > 0'
  - 'kube_daemonset_status_number_ready / kube_daemonset_status_desired_number_scheduled < 1'
  - 'kube_pod_container_status_waiting_reason{reason="CrashLoopBackOff"}'
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
  - "SKILL-WORK-001"
  - "SKILL-NODE-001"
  - "SKILL-POD-001"
  - "SKILL-POD-002"
  - "SKILL-IMAGE-001"
  - "SKILL-NODE-002"
fta_refs:
  - "domain-10-troubleshooting-diagnostics/topic-fta/list/daemonset-fta.md"
knowledge_refs:
  - "domain-10-troubleshooting-diagnostics/20-daemonset-troubleshooting.md"
  - "domain-02-workloads-applications/"
cross_refs:
  - type: "fta"
    path: "../domain-10-troubleshooting-diagnostics/topic-fta/list/daemonset-fta.md"
    label: "DaemonSet 故障树分析"
  - type: "domain"
    path: "../domain-10-troubleshooting-diagnostics/20-daemonset-troubleshooting.md"
    label: "DaemonSet 深度排查"
  - type: "[[SKILL|skill]]"
    path: "../domain-10-troubleshooting-diagnostics/topic-skills/01-node-notready.md"
    label: "SKILL-NODE-001 节点诊断"
authors:
  - name: KUDIG Team
    role: contributor

tier: peripheral---

# DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation

DaemonSet 确保集群中每个（或部分）节点上运行一个 Pod 副本，是集群基础设施的核心载体（如 kube-proxy、CNI 插件、日志收集器、监控代理）。当 DaemonSet 问题时，影响的不是单一应用，而是整个节点或集群的基础功能：网络不通、日志丢失、监控中断、安全代理失效。

与 Deployment 不同，DaemonSet 的调度逻辑直接绑定节点，故障模式集中在节点排除（污点、亲和性）、资源竞争、特权权限、host 网络冲突等方面。

本 Skill 覆盖节点缺失 Pod、CrashLoopBackOff、更新卡住、资源不足、污点排斥、hostPort 冲突、特权权限等 10 种根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| 某些节点缺少 DaemonSet Pod | `kubectl get pods -n <ns> -l <selector> -o wide` | 0.95 |
| DaemonSet Pod 处于 CrashLoopBackOff | `kubectl get pods -n <ns> -l <selector>` | 0.90 |
| 节点上系统服务（如日志/监控）中断 | 节点功能缺失告警 | 0.85 |
| DaemonSet 更新不进展 | `kubectl rollout status ds/<name>` | 0.90 |
| 新加入节点未自动部署系统 Pod | 新节点状态检查 | 0.90 |

**排除条件**: 节点 NotReady → SKILL-NODE-001; 通用 Pod CrashLoop → SKILL-POD-001; 镜像拉取失败 → SKILL-IMAGE-001

## 快速分级（2 分钟内完成）

```
DaemonSet 类型 + 影响范围
├── CNI 类 DaemonSet 问题（calico-node/cilium）──────→ P0（网络中断）
├── kube-proxy 问题────────────────────────────────→ P0（Service 不通）
├── 日志/监控类 DaemonSet 问题───────────────────────→ P1（可观测性中断）
├── 安全/审计类 DaemonSet 问题───────────────────────→ P1（安全边界失效）
├── 单节点缺失非关键 DaemonSet───────────────────────→ P2（局部影响）
└── 更新策略卡住但不影响当前运行─────────────────────→ P2（4h 内处理）
```

**立即升级条件**：
- CNI DaemonSet（calico-node/cilium/flannel）大面积问题
- kube-proxy DaemonSet 大面积问题
- 超过 30% 节点缺失同一 DaemonSet

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
| S1 | 某些节点缺少 DaemonSet Pod | `kubectl get pods -n <ns> -l <selector> -o wide` | 0.95 | 节点被手动排除 → 检查操作记录 |
| S2 | DaemonSet DESIRED > CURRENT | `kubectl get ds -n <ns>` | 0.95 | 节点正在加入/退出 → 等待完成 |
| S3 | DaemonSet Pod CrashLoopBackOff | `kubectl get pods -n <ns> -l <selector>` | 0.90 | 应用自身 bug |
| S4 | DaemonSet 更新卡住 | `kubectl rollout status ds/<name>` | 0.90 | 应用启动慢 |
| S5 | 新节点未自动部署 DaemonSet Pod | 新节点加入后检查 | 0.90 | 节点 NotReady → SKILL-NODE-001 |
| S6 | 节点上系统功能缺失（如无日志/监控） | 可观测性告警 | 0.85 | 配置问题 |
| S7 | DaemonSet Pod 被驱逐 | `kubectl get events --field-selector reason=Evicted` | 0.85 | 节点资源压力 → SKILL-NODE-002 |
| S8 | 特权容器启动失败 | Pod Events 显示权限错误 | 0.80 | 安全策略限制 |

### 2.2 工单关键词映射

- "某些节点上没有 calico-node"
- "kube-proxy Pod 在 CrashLoopBackOff"
- "新加入的节点没有 fluentd"
- "DaemonSet 更新一直不完成"
- "node-exporter 在某些节点缺失"
- "DaemonSet Pod 因为污点没有调度"
- "hostPort 冲突导致 Pod 启动失败"

### 2.3 排除标准

- 节点状态 NotReady → 使用 SKILL-NODE-001
- 通用 Pod CrashLoopBackOff → 使用 SKILL-POD-001
- 镜像拉取失败 → 使用 SKILL-IMAGE-001
- 节点资源压力导致驱逐 → 使用 SKILL-NODE-002

## 快速分级（2 分钟内完成）

### 3.1 影响评估

**Step T1**: 检查 DaemonSet 整体状态
```bash
kubectl get daemonset <name> -n <namespace>
# 关注: DESIRED, CURRENT, READY, UP-TO-DATE, AVAILABLE
```
> **判断规则**: DESIRED > CURRENT → 有节点缺失；READY < CURRENT → 有 Pod 未就绪

**Step T2**: 识别缺失 Pod 的节点
```bash
kubectl get pods -n <namespace> -l <selector> -o json | \
  jq -r '.items[].spec.nodeName' | sort | uniq > /tmp/has_pod_nodes.txt
kubectl get nodes -o json | jq -r '.items[].metadata.name' | sort > /tmp/all_nodes.txt
comm -23 /tmp/all_nodes.txt /tmp/has_pod_nodes.txt
```
> **判断规则**: 输出为缺失 DaemonSet Pod 的节点列表

**Step T3**: 检查缺失 Pod 节点的状态
```bash
kubectl get nodes <missing-node> -o wide
kubectl describe node <missing-node> | grep -A 10 "Taints:"
```
> **判断规则**: 节点有 NoSchedule/NoExecute 污点 → RC-003/004

**Step T4**: 统计问题范围
```bash
kubectl get daemonset <name> -n <namespace> -o jsonpath='{
  "desired": .status.desiredNumberScheduled,
  "current": .status.currentNumberScheduled,
  "ready": .status.numberReady,
  "available": .status.numberAvailable
}' | jq .
```
> **判断规则**: 缺失比例 > 30% → 集群级影响，升级处理

### 3.2 严重性分级

| 条件 | 级别 | 说明 |
|------|------|------|
| CNI/kube-proxy 类 DaemonSet 大面积问题 | P0 | 15min 内修复 |
| >30% 节点缺失同一 DaemonSet | P0 | 30min 内修复 |
| 单节点缺失关键 DaemonSet（如监控） | P1 | 1h 内修复 |
| 单节点缺失非关键 DaemonSet | P2 | 4h 内修复 |
| 更新策略卡住但不影响服务 | P2 | 4h 内修复 |

### 3.3 立即升级触发条件

- CNI DaemonSet 问题导致 Pod 间网络不通
- kube-proxy 问题导致 Service 不通
- >50% 节点缺失同一基础设施 DaemonSet

## 诊断工作流

### Phase 1: 快速检查（只读，零风险）

**Step D1.1**: 获取 DaemonSet 概览
- **命令**:
  ```bash
  kubectl get daemonset <name> -n <namespace> -o wide
  kubectl describe daemonset <name> -n <namespace> | head -40
  ```
- **超时**: 10s
- **判断规则**:
  - DESIRED > CURRENT → 有节点未调度（RC-001/003/004）
  - READY < CURRENT → 有 Pod 未就绪（RC-002/005/006/007/008）
  - UP-TO-DATE < CURRENT → 更新策略卡住（RC-009）

**Step D1.2**: 列出所有 DaemonSet Pod 及所在节点
- **命令**:
  ```bash
  kubectl get pods -n <namespace> -l <selector> \
    -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,READY:.status.containerStatuses[*].ready,NODE:.spec.nodeName,RESTARTS:.status.containerStatuses[*].restartCount
  ```
- **超时**: 10s
- **判断规则**:
  - 某些节点无对应 Pod → RC-001/003/004
  - Pod 状态 CrashLoopBackOff → RC-002/005/006/008
  - RESTARTS 频繁 → RC-002/005/006

**Step D1.3**: 检查缺失 Pod 的节点详情
- **命令**:
  ```bash
  # 获取缺失 Pod 的节点
  kubectl get nodes -o json | jq -r '.items[].metadata.name' | sort > /tmp/all_nodes.txt
  kubectl get pods -n <namespace> -l <selector> -o json | jq -r '.items[].spec.nodeName' | sort | uniq > /tmp/has_pod.txt
  MISSING=$(comm -23 /tmp/all_nodes.txt /tmp/has_pod.txt)
  for node in $MISSING; do
    echo "=== $node ==="
    kubectl get node $node -o jsonpath='{.spec.taints}' | jq -c .
    kubectl get node $node -o jsonpath='{.metadata.labels}' | jq -c .
  done
  ```
- **超时**: 15s
- **判断规则**:
  - 节点有 NoSchedule/NoExecute 污点 → RC-003/004（污点排斥）
  - 节点标签不匹配 DaemonSet nodeSelector → RC-001（节点选择器不匹配）
  - 节点 Ready 但无 Pod → 需进一步检查

**Step D1.4**: 检查 DaemonSet 的调度约束
- **命令**:
  ```bash
  kubectl get daemonset <name> -n <namespace> -o jsonpath='{.spec.template.spec}' | jq '{
    nodeSelector: .nodeSelector,
    affinity: .affinity,
    tolerations: .tolerations,
    hostNetwork: .hostNetwork,
    hostPID: .hostPID,
    hostIPC: .hostIPC
  }'
  ```
- **超时**: 10s
- **判断规则**:
  - nodeSelector 设置但节点无对应标签 → RC-001
  - tolerations 未包含节点污点 → RC-004
  - hostNetwork=true 但端口冲突 → RC-006

**Step D1.5**: 检查 Pod 事件和日志
- **命令**:
  ```bash
  # 获取一个异常 Pod 的事件
  kubectl get events -n <namespace> --field-selector involvedObject.name=<bad-pod> --sort-by=.lastTimestamp | tail -15
  # 快速查看日志
  kubectl logs -n <namespace> <bad-pod> --tail=30 2>/dev/null || echo "Cannot get logs"
  ```
- **超时**: 15s
- **判断规则**:
  - `FailedScheduling` → RC-003/004
  - `FailedMount` → 存储问题
  - `ImagePullBackOff` → SKILL-IMAGE-001
  - 日志显示权限错误 → RC-008（特权问题）
  - 日志显示端口绑定失败 → RC-006（hostPort 冲突）

### Phase 2: 深度检查（只读，零风险）

**Step D2.1**: 检查节点资源是否足够调度 DaemonSet
- **命令**:
  ```bash
  kubectl describe node <missing-node> | grep -A 15 "Allocated resources"
  kubectl top node <missing-node> 2>/dev/null || echo "metrics-server unavailable"
  ```
- **超时**: 10s
- **判断规则**:
  - 节点资源已耗尽 → RC-005（资源不足）
  - DaemonSet 资源请求过高 → RC-005

**Step D2.2**: 检查污点与容忍度匹配
- **命令**:
  ```bash
  NODE_TAINTS=$(kubectl get node <node> -o json | jq -c '.spec.taints')
  DS_TOLERATIONS=$(kubectl get daemonset <name> -n <namespace> -o json | jq -c '.spec.template.spec.tolerations')
  echo "Node taints: $NODE_TAINTS"
  echo "DaemonSet tolerations: $DS_TOLERATIONS"
  ```
- **超时**: 10s
- **判断规则**:
  - 节点有污点但 DaemonSet 无对应 toleration → RC-004
  - 控制平面节点有 `node-role.kubernetes.io/control-plane:NoSchedule` → 需要添加 toleration

**Step D2.3**: 检查 hostPort/hostNetwork 冲突
- **命令**:
  ```bash
  # 获取 DaemonSet 的端口配置
  kubectl get daemonset <name> -n <namespace> -o json | jq '.spec.template.spec.containers[].ports'
  # 检查节点上已使用的端口
  ssh <node-ip> "ss -tlnp | grep <host-port>"
  ```
- **超时**: 10s
- **判断规则**:
  - hostPort 已被占用 → RC-006
  - hostNetwork=true 且与节点服务冲突 → RC-006

**Step D2.4**: 检查安全上下文和特权要求
- **命令**:
  ```bash
  kubectl get daemonset <name> -n <namespace> -o json | jq '.spec.template.spec.containers[].securityContext'
  # 检查 PodSecurityPolicy/PSA 限制
  kubectl get namespace <namespace> -o jsonpath='{.metadata.labels}' | jq '. | with_entries(select(.key | startswith("pod-security")))'
  ```
- **超时**: 10s
- **判断规则**:
  - 需要 privileged 但 PSA 为 restricted → RC-008（权限不足）
  - 需要 hostPath 但 PSP 不允许 → RC-008

**Step D2.5**: 检查 DaemonSet 更新策略状态
- **命令**:
  ```bash
  kubectl get daemonset <name> -n <namespace> -o jsonpath='{.spec.updateStrategy}' | jq .
  kubectl rollout status daemonset <name> -n <namespace>
  ```
- **超时**: 10s
- **判断规则**:
  - `maxUnavailable` 为 0 且只有一个节点 → RC-009（更新策略阻塞）
  - `maxSurge` 设置不合理 → RC-009

**Step D2.6**: 检查 Controller Manager 日志
- **命令**:
  ```bash
  kubectl logs -n kube-system <kube-controller-manager-pod> | \
    grep -iE 'daemonset|DaemonSet' | grep <daemonset-name> | tail -20
  ```
- **超时**: 15s
- **判断规则**:
  - 日志显示调度失败原因 → RC-001/003/004/005
  - 日志显示创建失败 → RC-002/006/008

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: 测试节点上端口可用性
- **命令**:
  ```bash
  ssh <node-ip> "nc -zv localhost <host-port> 2>&1 || echo 'Port not in use'"
  ```
- **超时**: 10s
- **风险级别**: 🟢 低
- **判断规则**: 端口已被占用 → RC-006

**Step D3.2**: 手动触发 DaemonSet Pod 删除（强制重建）
- **命令**:
  ```bash
  kubectl delete pod <bad-pod> -n <namespace>
  ```
- **超时**: 15s
- **风险级别**: 🟡 中（短暂服务中断）
- **判断规则**: 删除后重建成功 → 可能是临时问题；仍失败 → 根因未解决

**Step D3.3**: 临时添加 toleration 测试
- **命令**:
  ```bash
  kubectl patch daemonset <name> -n <namespace> --type='json' -p='[
    {"op": "add", "path": "/spec/template/spec/tolerations/-", "value": {
      "key": "<taint-key>", "operator": "Exists", "effect": "<taint-effect>"
    }}
  ]'
  ```
- **超时**: 10s
- **风险级别**: 🟡 中（改变调度约束）
- **判断规则**: 添加 toleration 后 Pod 调度成功 → RC-004

## 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | nodeSelector/nodeAffinity 不匹配 | 高 | D1.3 节点标签不匹配；D1.4 nodeSelector 设置 | node_selector_mismatch |
| RC-002 | 容器启动失败（应用配置/依赖） | 高 | D1.5 CrashLoopBackOff；D2.6 日志错误 | container_startup_failure |
| RC-003 | 节点污点导致 Pod 未调度 | 高 | D1.3 节点有污点；D2.2 不匹配 | taint_exclusion |
| RC-004 | DaemonSet tolerations 不足 | 中 | D2.2 污点与容忍度不匹配；D3.3 验证 | insufficient_tolerations |
| RC-005 | 节点资源不足（CPU/内存/磁盘） | 中 | D2.1 资源耗尽；D1.5 FailedScheduling | node_resource_exhaustion |
| RC-006 | hostPort/hostNetwork 端口冲突 | 中 | D1.5 端口绑定失败；D2.3/D3.1 验证 | host_port_conflict |
| RC-007 | 镜像拉取失败 | 低 | D1.5 ImagePullBackOff；SKILL-IMAGE-001 验证 | image_pull_failure |
| RC-008 | 安全上下文/特权权限不足 | 中 | D1.5 权限错误；D2.4 PSA/PSP 限制 | security_context_denied |
| RC-009 | 更新策略卡住（maxUnavailable/maxSurge） | 低 | D1.1 UP-TO-DATE < CURRENT；D2.5 策略配置 | update_strategy_blocked |
| RC-010 | 节点被排除（cordon/drain） | 中 | D1.3 节点 SchedulingDisabled | node_excluded |

## 修复操作

### 6.1 🟢 低风险

#### REM-001: 修正 nodeSelector 或节点标签
- **适用根因**: RC-001
- **前置检查**: D1.3/D1.4 确认不匹配
- **执行**:
  ```bash
  # 方式1: 修改 DaemonSet nodeSelector
  kubectl patch daemonset <name> -n <namespace> -p \
    '{"spec":{"template":{"spec":{"nodeSelector":{"<key>":"<value>"}}}}}'
  # 方式2: 给节点添加标签
  kubectl label node <node> <key>=<value> --overwrite
  ```
- **后置验证**: `kubectl get pods -n <ns> -l <selector> -o wide`
- **回滚**: 恢复原始 nodeSelector 或删除节点标签

#### REM-002: 添加 tolerations
- **适用根因**: RC-003/004
- **前置检查**: D2.2 确认污点不匹配
- **执行**:
  ```bash
  kubectl patch daemonset <name> -n <namespace> --type='json' -p='[
    {"op": "add", "path": "/spec/template/spec/tolerations/-", "value": {
      "key": "<taint-key>", "operator": "Exists", "effect": "<taint-effect>"
    }}
  ]'
  ```
- **后置验证**: 检查缺失节点上是否创建 Pod
- **回滚**: 移除添加的 toleration

#### REM-003: 删除并重建异常 Pod
- **适用根因**: RC-002/006/009
- **前置检查**: 确认其他节点 Pod 正常
- **执行**:
  ```bash
  kubectl delete pod <bad-pod> -n <namespace>
  ```
- **后置验证**: 等待 DaemonSet 自动重建
- **回滚**: 无（DaemonSet 自动管理）

### 6.2 🟡 中风险

#### REM-004: 调整资源请求
- **适用根因**: RC-005
- **审批提示**: "建议降低 DaemonSet <name> 的资源请求，可能影响性能。是否批准？"
- **执行**:
  ```bash
  kubectl patch daemonset <name> -n <namespace> -p \
    '{"spec":{"template":{"spec":{"containers":[{"name":"<container>","resources":{"requests":{"cpu":"50m","memory":"64Mi"}}}]}}}}'
  ```
- **回滚**: 恢复原始资源请求

#### REM-005: 修改 hostPort 或改为非 hostNetwork
- **适用根因**: RC-006
- **审批提示**: "建议修改 DaemonSet 端口配置，可能影响外部访问。是否批准？"
- **执行**:
  ```bash
  # 修改 DaemonSet 使用不同 hostPort
  kubectl patch daemonset <name> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/ports/0/hostPort", "value": <new-port>}
  ]'
  ```
- **回滚**: 恢复原始端口配置

#### REM-006: 添加特权权限或调整 PSA
- **适用根因**: RC-008
- **审批提示**: "建议放宽 DaemonSet 安全限制，可能降低安全性。是否批准？"
- **执行**:
  ```bash
  # 方式1: 给 namespace 添加 privileged 标签
  kubectl label namespace <namespace> pod-security.kubernetes.io/enforce=privileged --overwrite
  # 方式2: 给 DaemonSet 添加安全上下文
  kubectl patch daemonset <name> -n <namespace> -p \
    '{"spec":{"template":{"spec":{"securityContext":{"privileged":true}}}}}'
  ```
- **回滚**: 恢复 PSA 标签或移除 privileged

### 6.3 🔴 高风险

#### REM-007: 调整更新策略参数
- **适用根因**: RC-009
- **操作步骤**:
  1. 修改 maxUnavailable 或 maxSurge
  2. 观察更新进展
  3. 更新完成后恢复合理参数
- **安全检查**: 确认更新不会同时中断过多节点

### 6.4 ⚫ 严重

#### REM-008: 紧急替换 DaemonSet 版本
- **适用根因**: RC-002（严重 Bug）
- **审批要求**: 高级 SRE
- **操作步骤**:
  1. 回滚到上一个版本
  2. 验证所有节点恢复
  3. 分析新版本问题

## 验证确认

### 7.1 即时验证

```bash
# V1: DaemonSet 状态
kubectl get daemonset <name> -n <namespace>
# 预期: DESIRED == CURRENT == READY == UP-TO-DATE

# V2: 所有节点覆盖
kubectl get pods -n <namespace> -l <selector> -o wide
# 预期: 每个节点一个 Pod，且状态 Running

# V3: 功能验证（根据 DaemonSet 类型）
# CNI: Pod 间连通性测试
# kube-proxy: Service 访问测试
# 日志: 检查日志收集
# 监控: 检查指标上报
```

### 7.2 短期监控

| 监控项 | 指标 | 预期 | 异常 |
|-------|------|------|------|
| DaemonSet 覆盖率 | `kube_daemonset_status_current/desired` | =1 | <1 |
| Pod 重启率 | `kube_pod_container_status_restarts_total` | 稳定 | 持续增加 |
| 节点功能 | 应用特定指标 | 正常 | 缺失 |

### 7.3 解决确认标准

- [ ] DESIRED == CURRENT == READY
- [ ] 每个 Ready 节点都有一个对应的 DaemonSet Pod
- [ ] Pod 无 CrashLoopBackOff
- [ ] 对应功能正常（网络/日志/监控）

## 升级协议

- **升级条件**: >30% 节点问题、CNI/kube-proxy 问题、诊断超时 30min
- **升级消息**: 包含 DaemonSet 名称、缺失节点列表、影响功能

## 版本兼容矩阵

| 功能 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| maxSurge | 支持 | 支持 | 支持 | 支持 | 支持 |
| maxUnavailable | 支持 | 支持 | 支持 | 支持 | 支持 |
| PodDisruptionBudget for DaemonSet | 支持 | 支持 | 支持 | 支持 | 支持 |

## 知识进化

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| 将 cordon 误判为污点 | 节点无 Pod | 手动 cordon | 检查 `SchedulingDisabled` |
| 忽略控制平面污点 | master 节点无 Pod | 缺少 control-plane toleration | 检查节点 role |

## 云厂商特异性

| 平台 | 差异 | 备注 |
|------|------|------|
| ACK | 托管节点自动部署 | 检查节点池配置 |
| EKS | Managed Node Group | 自动管理 DaemonSet 兼容性 |
| GKE | Autopilot 限制 DaemonSet | 某些 DaemonSet 不允许 |
| AKS | 系统节点池 | 区分系统/用户节点池 |

## 自动化集成接口

```bash
./scripts/diagnose-daemonset-quick.sh --daemonset <NAME> --namespace <NS>
./scripts/diagnose-daemonset-deep.sh --daemonset <NAME> --namespace <NS>
./scripts/verify-daemonset.sh --daemonset <NAME> --namespace <NS>
```

---

*文档版本: 1.0*  
*Skill ID: SKILL-WORK-003*  
*创建时间: 2026-05*  
*维护者: Kudig Team*
