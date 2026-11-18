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
skill_id: SKILL-POD-002
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

> **内容已并入主场**：[[wiki/19-故障诊断/08-技能体系/03-pod-pending.md|Pod Pending 调度失败诊断与修复]]（本页为重排去重桩，正文见主场）
