---
title: argo-cd v1.2 Release Notes
description: argo-cd v1.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- argocd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v1.2 Release Notes 是什么
- 如何 argo-cd v1.2 Release Notes
trigger_keywords:
- argo-cd
- v1.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
---

# argo-cd v1.2 Release Notes

Source: [v1.2.5](https://github.com/argoproj/argo-cd/releases/tag/v1.2.5)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.2.5/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.2.5/manifests/ha/install.yaml
```

### Changes since v1.2.4

- Issue #2339 - Don't update 'status.reconciledAt' unless compared with latest git version