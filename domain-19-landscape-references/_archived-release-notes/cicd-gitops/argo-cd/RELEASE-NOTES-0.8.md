---
title: argo-cd v0.8 Release Notes
description: argo-cd v0.8 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- argocd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v0.8 Release Notes 是什么
- 如何 argo-cd v0.8 Release Notes
trigger_keywords:
- argo-cd
- v0.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
---



# argo-cd v0.8 Release Notes

Source: [v0.8.2](https://github.com/argoproj/argo-cd/releases/tag/v0.8.2)

## v0.8.2 (2018-09-12)
- Downgrade ksonnet from v0.12.0 to v0.11.0 due to quote unescape regression
- Fix CLI panic when performing an initial `[[ArgoCD|argocd]] sync/wait`
