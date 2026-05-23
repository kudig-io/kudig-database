---
title: argo-cd v0.9 Release Notes
description: argo-cd v0.9 Release Notes — Kubernetes 生产运维知识库
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
- argo-cd v0.9 Release Notes 是什么
- 如何 argo-cd v0.9 Release Notes
trigger_keywords:
- argo-cd
- v0.9
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

# argo-cd v0.9 Release Notes

Source: [v0.9.2](https://github.com/argoproj/argo-cd/releases/tag/v0.9.2)

* Update to kustomize 1.0.8
- Fix issue where argocd-server logged credentials in plain text during repo add (issue #653)
- Credentials not being accepted for Google Source Repositories (issue #651)
- Azure Repos do not work as a repository (issue #643)
- Temporary ignore [[Service|service]] catalog resources (issue #650)
- Normalize policies by always adding space after comma
