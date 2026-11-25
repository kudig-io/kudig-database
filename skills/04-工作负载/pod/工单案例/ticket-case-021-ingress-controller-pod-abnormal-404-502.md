---
title: 阿里云专有云 Ingress 控制器 Pod 异常导致业务访问 404/502
description: 电商大促期间 Nginx Ingress Controller Pod 漂移后业务入口出现 404/502，根因为控制器 Pod 配置与节点负载不均导致后端
  upstream 漂移，含诊断、修复与验证。
summary: 电商大促期间 Nginx Ingress Controller Pod 漂移后业务入口出现 404/502，根因为控制器 Pod 配置与节点负载不均导致后端
  upstream 漂移，含诊断、修复与验证。
category: production-operations
tags:
- aliyun
- private-cloud
- ack
- ingress
- nginx-ingress
- 502
- 404
- loadbalancer
- slb
- ticket-case
tier: supporting
created: 2026-06-26
updated: 2026-06-26
incident_id: TC-2026-021
priority: P0
severity: critical
affected_cluster: ack-prod-vpc02
affected_namespace: ingress-nginx
ticket_type: 入口网关故障
skill_ref: Ingress 网关诊断
fta_ref: 'FTA: Ingress 404/502'
last_updated: 2026-06-26
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 阿里云专有云 Ingress 控制器 Pod 异常导致业务访问 404/502 如何处理
trigger_keywords:
- aliyun
- private-cloud
- ack
- ingress
- nginx-ingress
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
- target: '[[wiki/11-生产运维/05-工单案例/ticket-case-046-ingress-controller-404-502.md]]'
  type: related_to
- target: '[[wiki/20-概念/03-网络/ingress.md]]'
  type: related_to
- target: '[[wiki/11-生产运维/05-工单案例/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
- target: '[[wiki/11-生产运维/05-工单案例/ticket-case-011-ingress-controller-pod-404-502.md]]'
  type: related_to
---

> **内容已并入主场**：[[wiki/11-生产运维/05-工单案例/ticket-case-021-ingress-controller-pod-abnormal-404-502.md|阿里云专有云 Ingress 控制器 Pod 异常导致业务访问 404/502]]（本页为重排去重桩，正文见主场）
