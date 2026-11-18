---
title: Kubernetes 多集群 Fleet GitOps 运维指南
description: 覆盖 Argo CD ApplicationSet 模式、Karmada/OCM/Cluster API 概述、跨集群 Secret 同步、发布门控、漂移检测与 Fleet 可观测性的生产级运维手册
summary: 覆盖 Argo CD ApplicationSet 模式、Karmada/OCM/Cluster API 概述、跨集群 Secret 同步、发布门控、漂移检测与 Fleet 可观测性的生产级运维手册
category: release-change-management
tags:
- production
- best-practices
- playbook
- gitops
- fleet
- multi-cluster
- applicationset
- karmada
- ocm
- cluster-api
- secret-sync
- drift-detection
- observability
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 多集群 Fleet GitOps 运维指南 是什么
- ApplicationSet 怎么用
- Karmada OCM Cluster API 区别
- 跨集群 Secret 同步方案
- GitOps 发布门控怎么实现
- Fleet 漂移检测怎么做
trigger_keywords:
- fleet
- multi-cluster
- applicationset
- karmada
- ocm
- cluster-api
- secret sync
- promotion gate
- drift detection
- gitops
prerequisites:
- argocd-basics
- gitops-basics
- kubernetes-concepts
- helm-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **内容已并入主场**：[[wiki/11-发布变更/01-GitOps/10-fleet-gitops-operations-guide.md|Kubernetes 多集群 Fleet GitOps 运维指南]]（本页为重排去重桩，正文见主场）
