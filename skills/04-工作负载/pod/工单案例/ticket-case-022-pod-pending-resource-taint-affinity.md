---
title: 阿里云专有云 Pod Pending（资源不足 / taint 不匹配 / 亲和性冲突）
description: 大数据 Spark 任务提交后 Driver Pod 长时间 Pending，根因为节点资源不足、污点容忍缺失与 Pod 反亲和性冲突叠加，含诊断、修复与验证。
summary: 大数据 Spark 任务提交后 Driver Pod 长时间 Pending，根因为节点资源不足、污点容忍缺失与 Pod 反亲和性冲突叠加，含诊断、修复与验证。
category: production-operations
tags:
- aliyun
- private-cloud
- ack
- pod-pending
- scheduling
- resource-pressure
- taint
- affinity
- spark
- ticket-case
tier: supporting
created: 2026-06-26
updated: 2026-06-26
incident_id: TC-2026-022
priority: P1
severity: high
affected_cluster: ack-prod-vpc02
affected_namespace: data-platform
ticket_type: 调度失败
skill_ref: Pod Pending 诊断
fta_ref: 'FTA: Pod Pending'
last_updated: 2026-06-26
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
- 阿里云专有云 Pod Pending（资源不足 / taint 不匹配 / 亲和性冲突） 如何处理
trigger_keywords:
- aliyun
- private-cloud
- ack
- pod-pending
- scheduling
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
- target: '[[wiki/21-知识字典/scheduling/taint.md]]'
  type: related_to
- target: '[[wiki/21-知识字典/scheduling/affinity.md]]'
  type: related_to
- target: '[[wiki/11-生产运维/05-工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[wiki/11-生产运维/05-工单案例/ticket-case-017-pod-pending-resource-exhaustion.md]]'
  type: related_to
- target: '[[wiki/11-生产运维/05-工单案例/ticket-case-027-pod-pending-resource-taint.md]]'
  type: related_to
---

> **内容已并入主场**：[[wiki/11-生产运维/05-工单案例/ticket-case-022-pod-pending-resource-taint-affinity.md|阿里云专有云 Pod Pending（资源不足 / taint 不匹配 / 亲和性冲突）]]（本页为重排去重桩，正文见主场）
