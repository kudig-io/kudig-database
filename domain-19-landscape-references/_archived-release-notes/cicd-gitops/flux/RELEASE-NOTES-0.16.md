---
title: flux v0.16 Release Notes
description: flux v0.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flux
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.16 Release Notes 是什么
- 如何 flux v0.16 Release Notes
trigger_keywords:
- flux
- v0.16
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Flux|flux]] v0.16 Release Notes

Source: [v0.16.2](https://github.com/fluxcd/flux2/releases/tag/v0.16.2)

CHANGELOG
- PR #1690 - @fluxcdbot - Update toolkit components
- PR #1688 - @allenporter - Replace init() with TestMain()
- PR #1687 - @allenporter - Remove deprecated io/ioutil usage
- PR #1683 - @charles-woshicai - feat: display success message while create [[Secrets|secrets]] via `flux` cli.
- PR #1682 - @stefanprodan - Refactor test helpers
- PR #1672 - @souleb - Adds a watch flag to the get command
- PR #1671 - @allenporter - Add tests for flux trace command
- PR #1668 - @dmitrika - chore: remove deprecated io/ioutil
- PR #1653 - @hiddeco - Provide suggestion for some fields in bug report
- PR #1651 - @hiddeco - Transform GitHub issue template to new format
- PR #1628 - @darkowlzz - internal/utils: Add unit tests
- PR #1626 - @allenporter - Fix trace for optional GitRepository.Spec.Reference
- PR #1609 - @hiddeco - Request reconcile using patch instead of update


## Docker images

- `docker pull fluxcd/flux-cli:v0.16.2`
- `docker pull ghcr.io/fluxcd/flux-cli:v0.16.2`
