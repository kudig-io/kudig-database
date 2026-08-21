---
title: 升级消息模板 / Escalation Message Template
description: '- 问题概述: 节点 {node_name} ({node_ip}) 状态为 NotReady，持续 {duration}'
summary: '- 问题概述: 节点 {node_name} ({node_ip}) 状态为 NotReady，持续 {duration}'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 升级消息模板 / Escalation Message Template 是什么
- 如何 升级消息模板 / Escalation Message Template
trigger_keywords:
- 升级消息模板
- Escalation
- Message
- Template
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
skill_id: SKILL-ESCALATION_TEMPLATE-001
skill_name: 升级消息模板 / Escalation Message Template
version: 1.0.0
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 升级消息模板 / Escalation Message Template

> **[[SKILL|Skill]] ID**: SKILL-NODE-001  
> **数据来源**: 19-故障诊断/08-技能体系/01-node-notready.md Section 8

Agent 在触发升级条件时，应使用此模板生成通知消息。
变量使用 `{variable_name}` 格式，由 Agent 在运行时填充。

---

## 1. 消息模板 / Message Template

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

### 模板变量说明 / Template Variable Reference

| 变量 / Variable | 类型 / Type | 说明 / Description |
|---|---|---|
| `{severity}` | string | 严重性级别: P0, P1, P2, P3 |
| `{cluster_name}` | string | 集群名称 |
| `{node_name}` | string | 问题节点名称 |
| `{node_ip}` | string | 问题节点 IP |
| `{duration}` | string | NotReady 持续时间（如 "15min"） |
| `{affected_node_count}` | integer | NotReady 节点数量 |
| `{total_node_count}` | integer | 集群总节点数 |
| `{affected_pod_count}` | integer | 受影响的 Pod 总数 |
| `{affected_namespaces}` | string | 受影响的 namespace 列表（逗号分隔） |
| `{control_plane_affected}` | string | 是否涉及控制平面节点: "是/否" |
| `{phase1_summary}` | string | Phase 1 诊断摘要 |
| `{phase2_summary}` | string | Phase 2 诊断摘要 |
| `{phase3_summary}` | string | Phase 3 诊断摘要 |
| `{suspected_root_cause}` | string | 可能的根因描述 |
| `{root_cause_id}` | string | 根因 ID（如 RC-006） |
| `{key_evidence}` | string | 关键证据描述 |
| `{attempted_remediation}` | string | 已尝试的修复操作 |
| `{remediation_result}` | string | 修复结果: "成功/失败/未执行" |
| `{action_needed}` | string | 需要人工执行的操作 |
| `{ticket_id}` | string | 工单编号 |

---

## 2. 自动升级条件 / Auto-Escalation Conditions

以下任一条件满足时，Agent 自动触发升级流程：

| 条件 / Condition | 说明 / Description | 触发时机 / Trigger Timing |
|---|---|---|
| **诊断超时** | 诊断工作流执行超过 **10 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后 V1-V5 验证失败 |
| **严重性升级** | 初始分级为 P2 但影响面扩大（如更多节点变为 NotReady） | 诊断过程中 NotReady 节点数增加 |
| **未知根因** | 完成 Phase 1-3 所有诊断步骤但无法匹配任何已知根因（RC-001 至 RC-012） | 所有诊断步骤均无明确异常发现 |
| **操作权限不足** | Agent 或操作人员无 SSH 访问权限，无法执行 Phase 2+ 诊断 | Phase 1 完成后需要 SSH 但无权限 |
| **安全疑虑** | 诊断过程中发现可疑安全指标（异常进程、未知网络连接） | 任何诊断步骤中发现安全异常 |

### 立即升级触发条件 / Immediate Escalation Triggers

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

| 条件 / Condition | 说明 / Description |
|---|---|
| **集群级问题** | >50% 的节点处于 NotReady 状态 |
| **控制平面全部不可用** | 所有 control-plane 节点均 NotReady（[[etcd|etcd]] 集群可能已丢失 quorum） |
| **apiserver 不可达** | `kubectl get nodes` 命令本身超时或失败（无法执行任何诊断命令） |
| **级联问题** | NotReady 节点数量在 5 分钟内持续增加（可能是底层基础设施问题） |
| **安全事件** | 结合其他安全告警，怀疑节点被入侵导致的异常 |

---

## 3. 交接信息包清单 / Handoff Information Package Checklist

升级时，Agent 需准备以下完整信息供人工接手：

### 3.1 完整诊断路径 / Complete Diagnostic Path

按时间顺序列出已执行的每个诊断步骤及每步输出摘要：

```
# 🟢 低风险：只读/信息收集，通常无副作用
{timestamp} - D1.1: kubectl get nodes -o wide → {output_summary}
{timestamp} - D1.2: kubectl describe node {node_name} → {conditions_summary}
{timestamp} - D1.3: kubectl get events → {events_summary}
...
{timestamp} - D2.7: nc -zv {apiserver_ip} 6443 → {connectivity_result}
```
### 3.2 已排除的根因 / Excluded Root Causes

列出已通过诊断排除的根因及排除依据：

```
- RC-003 已排除 — D2.5 显示磁盘使用率 42%，低于阈值
- RC-007 已排除 — D2.8 显示证书有效期至 2026-12-15
- ...
```

### 3.3 可能的根因假设 / Possible Root Cause Hypotheses

基于已有证据提出的根因假设及置信度：

```
- 疑似 RC-006（网络分区）— D2.7 TCP 测试超时，但 D2.2 日志中无明确连接拒绝信息
  置信度: 0.6
- ...
```

### 3.4 关键资源快照 / Critical Resource Snapshots

Agent 应在升级前收集以下快照文件：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 节点描述
kubectl describe node {node_name} > node-describe.txt

# 节点事件
kubectl get events --field-selector involvedObject.name={node_name} --sort-by=.lastTimestamp > node-events.txt

# 节点上的 Pod 状态
kubectl get pods --field-selector spec.nodeName={node_name} --all-namespaces -o wide > node-pods.txt

# kubelet 日志（最近 1 小时）
ssh {node_ip} "journalctl -u kubelet --since '1 hour ago' --no-pager" > kubelet-logs.txt
```
### 3.5 事件时间线 / Event Timeline

最近 30 分钟内的关键事件按时间排列：

```
HH:MM:SS - 首次检测到 NotReady / First NotReady detection
HH:MM:SS - 开始诊断 / Diagnosis started
HH:MM:SS - 发现异常 [描述] / Anomaly found [description]
HH:MM:SS - 尝试修复 [操作] / Remediation attempted [action]
HH:MM:SS - 修复结果 [成功/失败] / Remediation result [success/failure]
HH:MM:SS - 决定升级 / Escalation decided
```

---

## 4. 严重性分级参考 / Severity Classification Reference

| 条件 / Condition | 级别 / Level | SLA 要求 / SLA |
|---|---|---|
| >30% 节点 NotReady **或** 任何控制平面节点 NotReady | **P0** | 立即响应，15min 内确认根因 |
| 多个工作节点 NotReady（2-30%） | **P1** | 15min 内响应，30min 内修复 |
| 单个工作节点 NotReady | **P2** | 30min 内响应，2h 内修复 |
| 新加入的节点从未进入 Ready / 尚未承载业务流量 | **P3** | 4h 内处理 |


<!-- risk-assessed -->
