---
title: flux v0.26 Release Notes
description: flux v0.26 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.26 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flux
- crd
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.26 Release Notes 是什么
- 如何 flux v0.26 Release Notes
trigger_keywords:
- flux
- v0.26
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Flux|flux]] v0.26 Release Notes

Source: [v0.26.3](https://github.com/fluxcd/flux2/releases/tag/v0.26.3)

## Highlights

Flux v0.26.3 is a patch release that comes with fixes to bootstrap. Users are encouraged to upgrade for the best experience.

In addition, kustomize-controller was update to be on par with Kustomize [v4.5.2 release](https://github.com/kubernetes-sigs/kustomize/releases/tag/kustomize%2Fv4.5.2).

## Components changelog
- kustomize-controller [v0.20.2](https://github.com/fluxcd/kustomize-controller/blob/v0.20.2/CHANGELOG.md)

## CLI changelog
- PR #2418 - @stefanprodan - Fix bootstrap: Reset schema cache after applying CRDs
- PR #2416 - @fluxcdbot - Update kustomize-controller to v0.20.2
- PR #2415 - @stefanprodan - Add GitRepository namespace arg to `flux create image update`

