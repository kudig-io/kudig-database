---
title: 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation (skills)
description: '| S7 | 调度失败且目标节点有 Pressure | `FailedScheduling` + 节点 Pressure | 0.85
  | 资源不足 → SKILL-POD-002 |'
summary: '| S7 | 调度失败且目标节点有 Pressure | `FailedScheduling` + 节点 Pressure | 0.85 | 资源不足
  → SKILL-POD-002 |'
category: skills
tags:
- k8s
- troubleshooting
- skill
- memorypressure
- diskpressure
- pidpressure
- evicted
- 节点资源压力
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation 是什么
- 如何 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation
trigger_keywords:
- 节点资源压力诊断与修复
- Node
- Resource
- Pressure
- Diagnosis
- Remediation
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation

### 症状识别



### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 节点状态包含 MemoryPressure | `kubectl get nodes` | 0.95 | 节点 NotReady → SKILL-NODE-001 |
| S2 | 节点状态包含 DiskPressure | `kubectl get nodes` | 0.95 | 节点 NotReady → SKILL-NODE-001 |
| S3 | 节点状态包含 PIDPressure | `kubectl get nodes` | 0.95 | 节点 NotReady → SKILL-NODE-001 |
| S4 | Pod 状态为 Evicted | `kubectl get [[Pods|pods]] -A | grep Evicted` | 0.90 | 节点被 drain → 人工操作 |
| S5 | Pod 被 OOMKilled (exit 137) | `kubectl get events --field-selector reason=OOMKilled` | 0.85 | 容器 limits 过低 → SKILL-POD-001 |
| S6 | 镜像拉取失败且节点 DiskPressure | `ImagePullBackOff` + DiskPressure | 0.80 | 镜像不存在 → SKILL-IMAGE-001 |
| S7 | 调度失败且目标节点有 Pressure | `FailedScheduling` + 节点 Pressure | 0.85 | 资源不足 → SKILL-POD-002 |
| S8 | 容器运行时响应缓慢 | `crictl ps` 超时 | 0.75 | 运行时崩溃 → SKILL-NODE-001 |

### 诊断工作流



### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集节点资源状态，无需 SSH。所有命令均为只读。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 获取所有压力节点概览
- **命令**:
  ```bash
  kubectl get nodes -o custom-columns=\
    NAME:.metadata.name,\
    STATUS:.status.conditions[-1].type,\
    MEM_PRESSURE:"status.conditions[?(@.type=='MemoryPressure')].status",\
    DISK_PRESSURE:"status.conditions[?(@.type=='DiskPressure')].status",\
    PID_PRESSURE:"status.conditions[?(@.type=='PIDPressure')].status"
  ```
- **超时**: 10s
- **预期输出模式**: 节点列表及三种压力条件状态
- **判断规则**:
  - MemoryPressure=True → 记录节点，关注 RC-001/002/003
  - DiskPressure=True → 记录节点，关注 RC-004/005/006/007
  - PIDPressure=True → 记录节点，关注 RC-008/009
  - 多种 Pressure 同时出现 → 通常根因关联（如 DiskPressure + PIDPressure 可能都是日志膨胀导致）
- **版本差异**: 无

**Step D1.2**: 检查节点详细 Conditions 和事件
- **命令**:
  ```bash
  kubectl describe node <node-name> | grep -A 10 "Conditions:"
  kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> \
    --sort-by=.lastTimestamp | tail -20
  ```
- **超时**: 15s
- **预期输出模式**: Conditions 详情和近期事件
- **判断规则**:
  - 出现 `NodeHasDiskPressure` 事件 → RC-004/005/006/007
  - 出现 `NodeHasMemoryPressure` 事件 → RC-001/002/003
  - 出现 `NodeHasPIDPressure` 事件 → RC-008/009
  - 出现 `Evicted` 事件，Reason 包含 `The node was low on resource` → 确认资源类型
  - 事件 Message 包含 `eviction threshold` → 记录具体阈值和当前值
- **版本差异**: 无

**Step D1.3**: 检查被驱逐 Pod 详情
- **命令**:
  ```bash
  kubectl get pods -A --field-selector=status.phase=Failed -o json | \
    jq -r '.items[] | select(.status.reason=="Evicted") |
    "\(.metadata.namespace)/\(.metadata.name) | \(.spec.nodeName) | \(.status.message)"' | head -20
  ```
- **超时**: 10s
- **预期输出模式**: 被驱逐 Pod 列表及驱逐原因
- **判断规则**:
  - Message 包含 `memory` → RC-001/002/003（内存压力）
  - Message 包含 `disk` / `ephemeral-storage` → RC-004/005/006/007（磁盘压力）
  - Message 包含 `pid` → RC-008/009（PID 压力）
  - 驱逐数量多且集中 → 压力严重，需立即处理
- **版本差异**: 无

**Step D1.4**: 检查节点资源分配和预留
- **命令**:
  ```bash
  kubectl describe node <node-name> | grep -A 20 "Allocated resources"
  kube
...(截断)

### Phase 2: 深度检查（只读，零风险，需 SSH）

> **目标**: SSH 登录压力节点，检查系统级资源使用。所有命令均为只读。
> **前提**: 需要对压力节点的 SSH 访问权限
> **预计耗时**: 5-15 分钟

**Step D2.1**: 检查系统内存使用详情
- **命令**:
  ```bash
  ssh <node-ip> "fr

--- (内容截断，完整内容见源文件) ---

## 相关链接

- [[技能/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[技能/skill-assets-escalation-template.md|skill-assets-escalation-template]] — Escalation Template
- [[技能/ts-cluster-operations.md|ts-cluster-operations]] — 集群运维故障排查
- [[技能/ts-storage.md|ts-storage]] — 存储故障排查
- [[技能/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
