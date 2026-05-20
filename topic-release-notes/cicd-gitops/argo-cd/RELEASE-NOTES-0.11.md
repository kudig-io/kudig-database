---
title: argo-cd v0.11 Release Notes
description: argo-cd v0.11 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v0.11 Release Notes 是什么
- 如何 argo-cd v0.11 Release Notes
trigger_keywords:
- argo-cd
- v0.11
- Release
- Notes
- release
- notes
---

# argo-cd v0.11 Release Notes

Source: [v0.11.2](https://github.com/argoproj/argo-cd/releases/tag/v0.11.2)

# Quickstart
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v0.11.2/manifests/install.yaml
```

# Changes since v0.11.1:
+ Adds client retry. Fixes #959 (#1119)
- Prevent deletion hotloop (#1115)
- Fix EncodeX509KeyPair function so it takes in account chained certificates (#1137) (@amarruedo)
- Exclude metrics.k8s.io from watch (#1128)
- Fix issue where dex restart could cause login failures (#1114)
- Relax ingress/service health check to accept non-empty ingress list (#1053)
- [UI] Correctly handle empty response from repository/<repo>/apps API