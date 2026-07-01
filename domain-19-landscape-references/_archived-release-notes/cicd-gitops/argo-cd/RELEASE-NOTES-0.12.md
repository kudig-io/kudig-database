---
title: argo-cd v0.12 Release Notes
description: argo-cd v0.12 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v0.12 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v0.12 Release Notes 是什么
- 如何 argo-cd v0.12 Release Notes
trigger_keywords:
- argo-cd
- v0.12
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
---



# [[Argo|argo]]-cd v0.12 Release Notes

Source: [v0.12.3](https://github.com/argoproj/argo-cd/releases/tag/v0.12.3)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v0.12.3/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v0.12.3/manifests/ha/install.yaml
```


## Changes since v0.12.2

- Application controller becomes unresponsive (#1476)
