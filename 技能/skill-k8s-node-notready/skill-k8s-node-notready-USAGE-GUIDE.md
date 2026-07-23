---
title: Usage Guide
description: '- [[技能/troubleshoot-pod-issues.md|Pod 故障排查]]'
summary: '- [[技能/troubleshoot-pod-issues.md|Pod 故障排查]]'
category: skills
tags:
- k8s
- troubleshooting
- skill
- kubelet
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Usage Guide 是什么
- 如何 Usage Guide
trigger_keywords:
- Usage
- Guide
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Usage Guide

### 3.2 root-cause-map.yaml — RC 决策图

每个 RC 条目具有标准结构。`fta_mapping` 字段将 RC 关联到对应的 FTA 底事件：

```yaml
- id: RC-001
  name:
    cn: "kubelet 进程崩溃或未运行"
    en: "kubelet process crashed or not running"
  probability: high
  diagnostic_evidence:
    primary: [D2.1, D2.2]       # References to diagnostic-workflow.md step IDs
    secondary: [D1.5]
  diagnostic_rules:
    - step: D2.1
      condition: "kubelet service not active/running"
      confidence: 0.9
  remediation:
    primary: REM-003             # Primary remediation in remediation-playbook.md
    alternatives: [REM-006]
  fta_mapping:                   # Bidirectional link to FTA
    file: "故障诊断/topic-fta/list/node-fta.md"
    step_ids: ["evt_kubelet_down", "evt_heartbeat_fail"]
  related_causes: [RC-008]       # Causes that may co-occur
```

### 4.2 JSON flow_steps — 步骤类型

`flow_steps` 数组中有四种步骤类型：

| 动作类型 | 示例 `step` ID | 用途 |
|---------|---------------|------|
| `gate_or` | `gate_root_or`、`cat_kubelet` | OR 门 — 任一子条件成立则向前路由 |
| `gate_and` | `evt_and_pleg_timeout`、`evt_and_mem_low` | AND 门 — 所有条件必须同时成立 |
| `bottom_event` | `evt_kubelet_down`、`evt_cni_fail` | 叶节点 — 实际诊断检查 |
| `category_gate` | `cat_nstat`、`cat_resource` | 顶级分类路由器 |

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/diagnostic-overview/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/troubleshoot-pod-issues.md|Pod 故障排查]]

## 使用指南

### 何时使用本技能

- 节点状态变为 NotReady
- 节点资源压力告警
- Pod 被意外驱逐
- 节点无法加入集群

### 使用步骤

```bash
# 步骤1：确认问题
kubectl get nodes
kubectl describe node <node-name>

# 步骤2：执行诊断
# 按照 SKILL.md 中的流程执行

# 步骤3：执行修复
# 根据根因选择对应修复方案

# 步骤4：验证恢复
kubectl get nodes
kubectl get pods -o wide --field-selector spec.nodeName=<node>
```

### 注意事项

- 🔴 drain 节点前确认 Pod 可迁移
- 🟡 重启 kubelet 会短暂影响节点上 Pod
- 🟢 诊断命令均为只读操作

## 面试要点

1. **Q：节点故障的应急响应流程？**
   A：确认影响→隔离节点→诊断根因→执行修复→验证恢复→复盘归档。

2. **Q：如何最小化节点故障影响？**
   A：PDB 保护、多副本部署、反亲和调度、健康检查、自动修复。

3. **Q：节点故障的常见根因？**
   A：kubelet 异常、资源耗尽、网络问题、证书过期、内核/驱动问题。

## Related

- [[技能/ts-security-auth.md|ts-security-auth]] — 安全认证故障排查
- [[技能/troubleshoot-pod-issues.md|troubleshoot-pod-issues]] — [[技能/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[故障诊断/FTA故障树/list/node-fta.md|node-fta]] — node-fta
- [[实体/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
