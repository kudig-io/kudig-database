---
title: Kubernetes 扩缩容最佳实践 [operations]
description: 生产环境 Kubernetes 扩缩容配置的最佳实践指南
summary: 生产环境 Kubernetes 扩缩容配置的最佳实践指南
category: best-practices/operations
tags:
- kubernetes
- scaling
- hpa
- vpa
- cluster-autoscaler
- kubelet
- prometheus
- helm
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- DevOps 工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- Kubernetes 扩缩容 最佳实践
- 如何 配置 HPA 自动扩缩容
- Kubernetes 集群自动扩缩容
trigger_keywords:
- Kubernetes
- 扩缩容
- HPA
- 自动扩缩容
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
cross_refs:
- type: domain
  path: ../../工作负载/
  label: 工作负载知识域
- type: domain
  path: ../../平台工程/
  label: 平台运维知识域
- type: best-practice
  path: ./deployment.md
  label: 部署策略最佳实践
---

> **内容已并入主场**：[[wiki/20-最佳实践/06-operations/scaling.md|Kubernetes 扩缩容最佳实践 [operations]]]（本页为重排去重桩，正文见主场）
