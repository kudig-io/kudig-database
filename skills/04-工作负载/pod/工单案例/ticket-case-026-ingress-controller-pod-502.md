---
title: Ingress 控制器 Pod 异常导致 404/502
description: 专有云 ACK 集群 Ingress-Nginx 控制器因内存限制过小而 OOM，导致外部流量出现大量 404/502 的工单闭环样本。
summary: 专有云 ACK 集群 Ingress-Nginx 控制器因内存限制过小而 OOM，导致外部流量出现大量 404/502 的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- ingress-nginx
- '502'
- '404'
- oom
- p0
- application-access
tier: peripheral
created: '2026-06-26T09:00:00+08:00'
updated: '2026-06-26T11:20:00+08:00'
incident_id: INC-2026-ACK-026
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-03
affected_namespace: kube-system
ticket_type: 应用访问异常
skill_ref:
- Ingress 访问异常诊断
- ACK Ingress-Nginx 运维
fta_ref:
- 'FTA: Ingress 404/502 根因分析'
last_updated: 2026-06-26 11:20:00+08:00
duplicate_of: TC-2026-021
status: duplicate
duplication_reason: 与 TC-2026-021 主题重复，内容角度相似，降低 RAG 权重
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Ingress 控制器 Pod 异常导致 404/502 如何处理
trigger_keywords:
- Ingress
prerequisites:
- kubectl-basics
- k8s-networking
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
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
- target: '[[wiki/22-概念/03-网络/ingress.md]]'
  type: related_to
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[wiki/13-生产运维/05-工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---

> **内容已并入主场**：[[wiki/13-生产运维/05-工单案例/ticket-case-026-ingress-controller-pod-502.md|Ingress 控制器 Pod 异常导致 404/502]]（本页为重排去重桩，正文见主场）
