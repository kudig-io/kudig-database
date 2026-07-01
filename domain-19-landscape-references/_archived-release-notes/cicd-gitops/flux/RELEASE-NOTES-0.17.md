---
title: flux v0.17 Release Notes
description: flux v0.17 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.17 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flux
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.17 Release Notes 是什么
- 如何 flux v0.17 Release Notes
trigger_keywords:
- flux
- v0.17
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Flux|flux]] v0.17 Release Notes

Source: [v0.17.2](https://github.com/fluxcd/flux2/releases/tag/v0.17.2)

CHANGELOG
- PR #1844 - @squaremo - Update Makefile target in release workflow
- PR #1838 - @superbrothers - Add dynamic completion support
- PR #1833 - @superbrothers - Fix `flux completion zsh` to work with `source` only
- PR #1830 - @makkes - fix: don't set 'Install' field by default in 'create hr'
- PR #1828 - @squaremo - Use a file to record successful manifests build


## Docker images

- `docker pull fluxcd/flux-cli:v0.17.2`
- `docker pull ghcr.io/fluxcd/flux-cli:v0.17.2`
