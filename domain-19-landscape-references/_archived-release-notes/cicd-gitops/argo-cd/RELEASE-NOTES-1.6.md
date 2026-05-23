---
title: argo-cd v1.6 Release Notes
description: argo-cd v1.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- argocd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v1.6 Release Notes 是什么
- 如何 argo-cd v1.6 Release Notes
trigger_keywords:
- argo-cd
- v1.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- gitops-basics
created: "2026-05-23"
---

# argo-cd v1.6 Release Notes

Source: [v1.6.2](https://github.com/argoproj/argo-cd/releases/tag/v1.6.2)

## Quick Start

### Non-HA:

```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.6.2/manifests/install.yaml
```

### HA:

```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.6.2/manifests/ha/install.yaml
```

## Changes

- feat: adding validate for app create and app set (#4016)
- fix: use glob matcher in casbin built-in model (#3966)
- fix: Normalize [[Helm|Helm]] chart path when chart name contains a slash (#3987)
- fix: allow duplicates when using generateName (#3878)
- fix: nil pointer dereference while syncing an app (#3915)