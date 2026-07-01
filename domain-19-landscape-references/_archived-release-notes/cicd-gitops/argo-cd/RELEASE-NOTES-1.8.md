---
title: argo-cd v1.8 Release Notes
description: argo-cd v1.8 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v1.8 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v1.8 Release Notes 是什么
- 如何 argo-cd v1.8 Release Notes
trigger_keywords:
- argo-cd
- v1.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
---



# [[Argo|argo]]-cd v1.8 Release Notes

Source: [v1.8.7](https://github.com/argoproj/argo-cd/releases/tag/v1.8.7)

## Quick Start

### Non-HA:
```bash
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.8.7/manifests/install.yaml
```

#### HA:
```bash
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.8.7/manifests/ha/install.yaml
```

## Important note

This release fixed a regression regarding which cluster resources are permitted on the AppProject level. Previous to this fix, after #3960 has been merged, all cluster resources were allowed on project level when neither of the allow or deny lists was defined. However, the correct behavior is to block all resources in this case. 

If you have Projects with empty allow and deny lists, but want the associated applications be able to sync cluster resources, you will have to adapt your cluster resources allow lists to explicitly allow the resources.

#### Bug Fixes
- fix: redact sensitive data in logs (#5662)
- fix: Properly escape HTML for error message from CLI SSO (#5563)
- fix: Empty resource whitelist allowed all resources (#5540) (#5551)