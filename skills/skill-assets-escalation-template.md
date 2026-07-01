---
title: Escalation Template
description: '| **安全事件** | 结合其他安全告警，怀疑节点被入侵导致的异常 |'
category: skills
tags:
- k8s
- troubleshooting
- skill
- etcd
- apiserver
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Escalation Template 是什么
- 如何 Escalation Template
trigger_keywords:
- Escalation
- Template
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# Escalation Template

### 立即升级触发条件 / Immediate Escalation Triggers

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

| 条件 / Condition | 说明 / Description |
|---|---|
| **集群级问题** | >50% 的节点处于 NotReady 状态 |
| **控制平面全部不可用** | 所有 control-plane 节点均 NotReady（etcd 集群可能已丢失 quorum） |
| **apiserver 不可达** | `kubectl get nodes` 命令本身超时或失败（无法执行任何诊断命令） |
| **级联问题** | NotReady 节点数量在 5 分钟内持续增加（可能是底层基础设施问题） |
| **安全事件** | 结合其他安全告警，怀疑节点被入侵导致的异常 |

---

### 3.1 完整诊断路径 / Complete Diagnostic Path

按时间顺序列出已执行的每个诊断步骤及每步输出摘要：

```
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

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- troubleshooting.md|k8s-structured-troubleshooting]] — 结构化排障方法论：配置优先、全组件排障指南
- [[kudig-prompts-catalog]] — [[entities/kudig-prompts-catalog.md|KUDIG Prompts Catalog]]
- [[skills/FTA Methodology and Core Principles.md|[[FTA Methodology and Core Principles|FTA Methodology and Core Principles]]]] — FTA Methodology and Core Principles
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
