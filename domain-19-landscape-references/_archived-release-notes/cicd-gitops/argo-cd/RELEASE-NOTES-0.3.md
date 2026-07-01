---
title: argo-cd v0.3 Release Notes
description: argo-cd v0.3 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v0.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v0.3 Release Notes 是什么
- 如何 argo-cd v0.3 Release Notes
trigger_keywords:
- argo-cd
- v0.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Argo|argo]]-cd v0.3 Release Notes

Source: [v0.3.2](https://github.com/argoproj/argo-cd/releases/tag/v0.3.2)

* Application sync should delete 'unexpected' resources https://github.com/argoproj/argo-cd/issues/139
* Update ksonnet to v0.10.1
* Detect `unexpected` resources 
* Fix: App sync frequently fails due to concurrent app modification https://github.com/argoproj/argo-cd/issues/147
*  Fix: improve app state comparator: https://github.com/argoproj/argo-cd/issues/136, https://github.com/argoproj/argo-cd/issues/132