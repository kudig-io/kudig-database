---
title: Remediation Playbook
description: '- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]'
summary: '- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]'
category: skills
tags:
- k8s
- troubleshooting
- skill
- pdb
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Remediation Playbook 是什么
- 如何 Remediation Playbook
trigger_keywords:
- Remediation
- Playbook
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Remediation Playbook

### 修复操作



## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## 修复 Playbook 使用指南

### 修复流程

```
故障发现 → 影响评估 → 选择 Playbook → 执行修复 → 验证恢复 → 复盘归档
```

### 修复命令风险等级

| 等级 | 含义 | 示例 |
|---|---|---|
| 🟢 | 只读/诊断 | kubectl get, kubectl describe, kubectl logs |
| 🟡 | 配置变更 | kubectl edit, kubectl patch, kubectl scale |
| 🔴 | 破坏性操作 | kubectl delete, kubectl drain, etcd restore |

### 常用修复命令

```bash
# 🟢 诊断类
kubectl get pods -A --field-selector=status.phase!=Running
kubectl describe pod <pod> -n <ns>
kubectl logs <pod> -n <ns> --previous
# 🟡 修复类
kubectl rollout restart deploy/<name> -n <ns>
kubectl scale deploy/<name> --replicas=3 -n <ns>
# 🔴 高风险（需审批）
kubectl delete pod <pod> -n <ns> --grace-period=0 --force
kubectl drain <node> --ignore-daemonsets
```

## 面试要点

1. **Q：如何设计修复 Playbook？**
   A：明确触发条件、列出诊断步骤、提供修复命令、标注风险等级、包含验证方法、记录回滚方案。

2. **Q：修复操作的安全原则？**
   A：最小影响原则、先备份后操作、灰度验证、可回滚、审批流程、操作记录。

3. **Q：如何自动化修复？**
   A：Runbook 自动化(Ansible)、自愈控制器(Operator)、告警触发修复、ChatOps 集成。

## Related

- [[技能/工作负载/pod/诊断排障/ts-workloads.md|ts-workloads]] — 工作负载故障排查
- [[pdb-fta]] — [[技能/工作负载/hpa-vpa/pdb-fta.md|[[PDB 异常故障树分析|PDB 异常故障树分析]]]]
- [[技能/工作负载/pod/培训/测验/assessment-daily-check-quiz.md|assessment-daily-check-quiz]] — Daily Check Quiz
- [[psp-scc-fta]] — PSP/SCC 异常故障树分析
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
