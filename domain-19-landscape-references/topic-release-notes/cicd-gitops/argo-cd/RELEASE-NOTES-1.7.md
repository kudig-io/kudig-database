---
title: argo-cd v1.7 Release Notes
description: argo-cd v1.7 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v1.7 Release Notes 是什么
- 如何 argo-cd v1.7 Release Notes
trigger_keywords:
- argo-cd
- v1.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
---

# argo-cd v1.7 Release Notes

Source: [v1.7.14](https://github.com/argoproj/argo-cd/releases/tag/v1.7.14)

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

#### Bug Fixes
- fix: redact sensitive data in logs (#5662)
- fix: Properly escape HTML for error message from CLI SSO (#5563)
- fix: Empty resource whitelist allowed all resources (#5540) (#5551)