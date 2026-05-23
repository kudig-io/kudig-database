---
title: argo-cd v1.5 Release Notes
description: argo-cd v1.5 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v1.5 Release Notes 是什么
- 如何 argo-cd v1.5 Release Notes
trigger_keywords:
- argo-cd
- v1.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
created: "2026-05-23"
---

# argo-cd v1.5 Release Notes

Source: v1.5.8](https://github.com/argoproj/argo-cd/releases/tag/v1.5.8)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.5.8/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.5.8/manifests/ha/install.yaml
```

### Bug Fixes

* fix: upgrade awscli version (#3774)
* fix: html encode login error/description before rendering it (#3773)
* fix: oidc should set samesite cookie (#3632)
* fix: avoid panic in badge handler (#3741)