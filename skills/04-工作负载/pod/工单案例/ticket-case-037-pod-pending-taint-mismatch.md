---
title: Pod Pending：节点 taint 与 toleration 不匹配
description: 专有云 ACK 集群新上线 AI 推理服务 Pod 长期处于 Pending，根因为节点 taint 未配置对应 toleration，含诊断、修复与验证。
summary: 专有云 ACK 集群新上线 AI 推理服务 Pod 长期处于 Pending，根因为节点 taint 未配置对应 toleration，含诊断、修复与验证。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- pod-pending
- taint
- toleration
- gpu
- scheduling
- p1
tier: peripheral
created: '2026-06-26T10:00:00+08:00'
updated: '2026-06-26T12:20:00+08:00'
incident_id: TC-2026-037
priority: P1
severity: high
affected_cluster: ack-zyy-prod-04
affected_namespace: ai-inference
ticket_type: 调度故障
skill_ref:
- Pod Pending 排查
- 资源调度治理
fta_ref:
- 'FTA: Pod Pending'
last_updated: 2026-06-26 12:20:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Pod Pending：节点 taint 与 toleration 不匹配 如何处理
trigger_keywords:
- ack
- zyy
- pod-pending
- taint
- toleration
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
- target: '[[wiki/17-系统基础/06-知识字典/scheduling/toleration.md]]'
  type: related_to
- target: '[[wiki/17-系统基础/06-知识字典/scheduling/taint.md]]'
  type: related_to
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
---

> **内容已并入主场**：[[wiki/13-生产运维/05-工单案例/ticket-case-037-pod-pending-taint-mismatch.md|Pod Pending：节点 taint 与 toleration 不匹配]]（本页为重排去重桩，正文见主场）
