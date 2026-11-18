---
title: Pod Pending：资源不足与污点不匹配
description: 专有云 ACK 集群 AI 训练任务因 GPU 节点污点未容忍及资源请求过大导致 Pod 长时间 Pending 的工单闭环样本。
summary: 专有云 ACK 集群 AI 训练任务因 GPU 节点污点未容忍及资源请求过大导致 Pod 长时间 Pending 的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- pod-pending
- gpu
- taint
- resource
- p1
- scheduling
tier: peripheral
created: '2026-06-26T08:30:00+08:00'
updated: '2026-06-26T11:00:00+08:00'
incident_id: INC-2026-ACK-027
priority: P1
severity: high
affected_cluster: ack-zyy-prod-04
affected_namespace: ai-training
ticket_type: 调度异常
skill_ref:
- Pod Pending 诊断
- Pod 调度策略
fta_ref:
- 'FTA: Pod Pending 根因分析'
last_updated: 2026-06-26 11:00:00+08:00
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
- Pod Pending：资源不足与污点不匹配 如何处理
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
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
---

> **内容已并入主场**：[[wiki/13-生产运维/05-工单案例/ticket-case-027-pod-pending-resource-taint.md|Pod Pending：资源不足与污点不匹配]]（本页为重排去重桩，正文见主场）
