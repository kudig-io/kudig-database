---
title: Pod 大量 Pending：节点 CPU/内存资源不足
description: 专有云 ACK 集群因业务突发扩容导致节点 CPU/内存资源耗尽，大量 Pod 长期处于 Pending 状态的工单闭环样本。
summary: 专有云 ACK 集群因业务突发扩容导致节点 CPU/内存资源耗尽，大量 Pod 长期处于 Pending 状态的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- pod-pending
- resource-exhaustion
- scheduler
- p1
- capacity
tier: core
created: '2026-06-26T14:30:00+08:00'
updated: '2026-06-26T17:00:00+08:00'
incident_id: INC-2026-ACK-017
priority: P1
severity: high
affected_cluster: ack-zyy-prod-05
affected_namespace: flashsale
ticket_type: 调度失败
skill_ref:
- Pod Pending 排查
- Pod 调度保障
fta_ref:
- 'FTA: 资源不足导致 Pod Pending'
last_updated: 2026-06-26 17:00:00+08:00
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
- Pod 大量 Pending：节点 CPU/内存资源不足 如何处理
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

> **内容已并入主场**：[[wiki/13-生产运维/05-工单案例/ticket-case-017-pod-pending-resource-exhaustion.md|Pod 大量 Pending：节点 CPU/内存资源不足]]（本页为重排去重桩，正文见主场）
