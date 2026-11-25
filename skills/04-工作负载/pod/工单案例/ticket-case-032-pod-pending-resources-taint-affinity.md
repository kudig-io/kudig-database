---
title: Pod Pending：资源请求过大 + 节点 taint/反亲和性冲突
description: 专有云 ACK 大数据命名空间 Spark 驱动 Pod 因内存请求超出节点容量、未容忍 NoSchedule taint 且 podAntiAffinity
  冲突导致长期 Pending 的工单闭环样本。
summary: 专有云 ACK 大数据命名空间 Spark 驱动 Pod 因内存请求超出节点容量、未容忍 NoSchedule taint 且 podAntiAffinity
  冲突导致长期 Pending 的工单闭环样本。
category: production-operations
tags:
- ack
- zyy
- spark
- pod-pending
- scheduler
- taint
- affinity
- p1
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:30:00+08:00'
incident_id: TC-2026-032
priority: P1
severity: high
affected_cluster: ack-zyy-prod-01
affected_namespace: data-platform
ticket_type: 调度失败
skill_ref:
- '[[wiki/02-工作负载/01-核心工作负载/19-scheduler-configuration.md|Scheduler
  配置]]'
- '[[wiki/02-工作负载/01-核心工作负载/22-cluster-capacity-planning.md|集群容量规划]]'
fta_ref:
- '[[wiki/17-故障诊断/06-FTA故障树/list/scheduler-fta.md|FTA: 调度失败]]'
- '[[wiki/17-故障诊断/06-FTA故障树/list/pod-fta.md|FTA: Pod 异常]]'
last_updated: 2026-06-26 16:30:00+08:00
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
- Pod Pending：资源请求过大 + 节点 taint/反亲和性冲突 如何处理
trigger_keywords:
- ack
- zyy
- spark
- pod-pending
- scheduler
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
- target: '[[wiki/11-生产运维/05-工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[wiki/11-生产运维/05-工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[wiki/11-生产运维/05-工单案例/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
---

> **内容已并入主场**：[[wiki/11-生产运维/05-工单案例/ticket-case-032-pod-pending-resources-taint-affinity.md|Pod Pending：资源请求过大 + 节点 taint/反亲和性冲突]]（本页为重排去重桩，正文见主场）
