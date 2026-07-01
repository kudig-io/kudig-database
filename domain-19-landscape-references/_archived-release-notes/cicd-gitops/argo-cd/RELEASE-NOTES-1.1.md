---
title: argo-cd v1.1 Release Notes
description: argo-cd v1.1 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v1.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
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
- argo-cd v1.1 Release Notes 是什么
- 如何 argo-cd v1.1 Release Notes
trigger_keywords:
- argo-cd
- v1.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- gitops-basics
---



# argo-cd v1.1 Release Notes

Source: [v1.1.2](https://github.com/argoproj/argo-cd/releases/tag/v1.1.2)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.1.2/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.1.2/manifests/ha/install.yaml
```

### Changes since v1.1.1

-  'argocd app wait' should print correct sync status (#2049)
- Check that TLS is enabled when registering [[Dex|DEX]] Handlers (#2047)
- Do not ignore [[Argo|Argo]] hooks when there is a [[Helm|Helm]] hook. (#1952)