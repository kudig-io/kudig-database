---
title: topic-skills — 工单智能体 Kubernetes 诊断 Skill 库 (skills)
description: '- 网络问题: [04-dns](./04-dns-resolution-failure.md) | [05-service](./05-service-connectivity.md) | [13-ingress](./13-ingress-gateway-failure.md)'
category: skills
tags:
- k8s
- troubleshooting
- skill
- job
- cronjob
- ingress
- gateway
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- topic-skills — 工单智能体 Kubernetes 诊断 Skill 库 是什么
- 如何 topic-skills — 工单智能体 Kubernetes 诊断 Skill 库
trigger_keywords:
- topic-skills
- 工单智能体
- Kubernetes
- 诊断
- Skill
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# topic-skills — 工单智能体 Kubernetes 诊断 [[SKILL|Skill]] 库

### 故障诊断

- 节点问题: [01-node-notready](./01-node-notready.md) | [11-control-plane](./11-control-plane-failure.md)
- Pod 异常: [02-crashloop](./02-pod-crashloop-oomkilled.md) | [03-pending](./03-pod-pending.md) | [10-image-pull](./10-image-pull-failure.md)
- 网络问题: [04-dns](./04-dns-resolution-failure.md) | [05-[[Service|service]]](./05-service-connectivity.md) | [13-[[Ingress|ingress]]](./13-ingress-gateway-failure.md)
- 存储问题: [07-pvc-storage](./07-pvc-storage-failure.md)
- 配置问题: [14-configmap-secret](./14-configmap-secret-failure.md)

### 3. 症状 → Skill 快速查找



## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[higress-fta]] — Higress 网关异常故障树分析
- [[cluster-upgrade-fta]] — 集群升级异常故障树分析
- [[job-cronjob-fta]] — Job/CronJob 异常故障树分析
- [[skills/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[kubernetes]] — Kubernetes (CNCF Graduated)
