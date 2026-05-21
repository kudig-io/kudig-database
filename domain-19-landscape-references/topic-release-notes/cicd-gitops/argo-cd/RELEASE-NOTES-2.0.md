---
title: argo-cd v2.0 Release Notes
description: argo-cd v2.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- argocd
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v2.0 Release Notes 是什么
- 如何 argo-cd v2.0 Release Notes
trigger_keywords:
- argo-cd
- v2.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gitops-basics
---

# argo-cd v2.0 Release Notes

Source: [v2.0.5](https://github.com/argoproj/argo-cd/releases/tag/v2.0.5)

## Quick Start

### Non-HA:

```bash
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.0.5/manifests/install.yaml
```

#### HA:
```bash
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.0.5/manifests/ha/install.yaml
```

#### Bug Fixes

- fix: allow argocd-notification ingress to repo-server (#6746)
- fix: argocd-server crashes due to nil pointer dereference (#6757)
- fix: WebUI failure when loading pod view 't.parentRefs is undefined' (#6490) (#6535)
- fix: prevent 'cannot read property "filter" of undefined' during nodes filtering (#6453)
- fix: download Pod Logs button not honouring argocd-server rootpath (#6548) (#6627)
- fix: Version warning banner in docs (#6682)
- fix: upgrade gitops engine to fix workflow health check