---
title: Pod Pending：资源不足与 Taint 不匹配
description: 专有云 ACK 集群因节点资源不足叠加自定义 taint 导致业务 Pod 长时间 Pending 的工单闭环样本。
summary: 专有云 ACK 集群因节点资源不足叠加自定义 taint 导致业务 Pod 长时间 Pending 的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- pod-pending
- scheduler
- taints
- resources
- p1
tier: core
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T15:45:00+08:00'
incident_id: INC-2026-ACK-042
priority: P1
severity: high
affected_cluster: ack-zyy-prod-04
affected_namespace: data-proc
ticket_type: 调度失败
skill_ref:
- '[[wiki/19-故障诊断/06-FTA故障树/list/pod-fta.md|Pod 异常故障树分析]]'
- '[[wiki/10-平台工程/03-治理/01-capacity-planning-resource-assessment.md|容量规划与资源评估]]'
fta_ref:
- '[[wiki/19-故障诊断/06-FTA故障树/list/scheduler-fta.md|FTA: Scheduler
  异常]]'
last_updated: 2026-06-26 15:45:00+08:00
duplicate_of: INC-2026-ACK-047
status: duplicate
duplication_reason: 与 "INC-2026-ACK-047" 主题重复，内容角度相似，降低 RAG 权重
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Pod Pending：资源不足与 Taint 不匹配 如何处理
trigger_keywords:
- Pod
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
relationships:
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-017-pod-pending-resource-exhaustion.md]]'
  type: related_to
---

> **内容已并入主场**：[[wiki/13-生产运维/05-工单案例/ticket-case-042-pod-pending-resource-taint.md|Pod Pending：资源不足与 Taint 不匹配]]（本页为重排去重桩，正文见主场）
