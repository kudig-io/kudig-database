---
title: argo-cd v1.4 Release Notes
description: argo-cd v1.4 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v1.4 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v1.4 Release Notes 是什么
- 如何 argo-cd v1.4 Release Notes
trigger_keywords:
- argo-cd
- v1.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
---



# [[Argo|argo]]-cd v1.4 Release Notes

Source: [v1.4.3](https://github.com/argoproj/argo-cd/releases/tag/v1.4.3)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.4.3/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.4.3/manifests/ha/install.yaml
```

### Bug Fixes

* fix: evaluate attack vector of GHSA-qm7j-c969-7j4q on ArgoCD (CVE-2020-5260)