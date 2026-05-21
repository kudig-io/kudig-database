---
title: argo-cd v0.6 Release Notes
description: argo-cd v0.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- statefulset
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v0.6 Release Notes 是什么
- 如何 argo-cd v0.6 Release Notes
trigger_keywords:
- argo-cd
- v0.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# argo-cd v0.6 Release Notes

Source: [v0.6.2](https://github.com/argoproj/argo-cd/releases/tag/v0.6.2)

Bug fixes:
* Health check for StatefulSets, DaemonSet, and ReplicaSets were failing due to use of wrong converters