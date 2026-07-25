---
title: Usage Guide
description: '- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]'
summary: '- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]'
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
    file: "domain-10-troubleshooting-diagnostics/topic-fta/list/node-fta.md"
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

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[skills/ts-security-auth.md|ts-security-auth]] — 安全认证故障排查
- [[skills/troubleshoot-pod-issues.md|troubleshoot-pod-issues]] — [[skills/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/node-fta.md|node-fta]] — node-fta
- [[entities/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
