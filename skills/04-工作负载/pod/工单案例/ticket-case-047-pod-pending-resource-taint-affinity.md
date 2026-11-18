---
title: Pod 持续 Pending：资源不足、Taint 不匹配与亲和性冲突
description: 专有云 ACK 集群业务发布时大量 Pod 处于 Pending 状态，根因涉及节点资源耗尽、Toleration 缺失与 Pod 亲和性冲突的工单闭环样本。
summary: 专有云 ACK 集群业务发布时大量 Pod 处于 Pending 状态，根因涉及节点资源耗尽、Toleration 缺失与 Pod 亲和性冲突的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- pod-pending
- schedule
- taint
- affinity
- resource
- p1
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T17:00:00+08:00'
incident_id: INC-2026-ACK-047
priority: P1
severity: high
affected_cluster: ack-zyy-prod-05
affected_namespace: risk-engine
ticket_type: 调度故障
skill_ref:
- '[[wiki/02-工作负载/01-核心工作负载/22-cluster-capacity-planning.md|集群容量规划]]'
- '[[wiki/02-工作负载/01-核心工作负载/23-resource-management.md|资源管理]]'
- '[[wiki/19-故障诊断/08-技能体系/skill-set/k8s-deployment-rollout/SKILL.md|Deployment
  滚动发布诊断 Skill]]'
fta_ref:
- 'FTA: Pod Pending 排障'
last_updated: 2026-06-26 17:00:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Pod 持续 Pending：资源不足、Taint 不匹配与亲和性冲突 如何处理
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

> **内容已并入主场**：[[wiki/13-生产运维/05-工单案例/ticket-case-047-pod-pending-resource-taint-affinity.md|Pod 持续 Pending：资源不足、Taint 不匹配与亲和性冲突]]（本页为重排去重桩，正文见主场）
