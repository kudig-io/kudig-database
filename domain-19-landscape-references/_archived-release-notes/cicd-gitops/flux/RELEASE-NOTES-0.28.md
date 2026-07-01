---
title: flux v0.28 Release Notes
description: flux v0.28 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.28 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flux
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
- flux v0.28 Release Notes 是什么
- 如何 flux v0.28 Release Notes
trigger_keywords:
- flux
- v0.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Flux|flux]] v0.28 Release Notes

Source: [v0.28.5](https://github.com/fluxcd/flux2/releases/tag/v0.28.5)

Flux v0.28.5 is a patch release that comes with various improvements and dependency updates to the controller components. Please consult the changelogs from the list below for a precise overview of changes. Users are (as always) encouraged to upgrade for the best experience.

**Note** that if you are upgrading from v0.27 you need to follow the [Upgrade Flux to the Source v1beta2 API guide](https://github.com/fluxcd/flux2/discussions/2567).

## Components Changelog

- kustomize-controller to [v0.22.3](https://github.com/fluxcd/kustomize-controller/blob/v0.22.3/CHANGELOG.md)
- source-controller to [v0.22.5](https://github.com/fluxcd/source-controller/blob/v0.22.5/CHANGELOG.md)
- image-automation-controller to [v0.21.3](https://github.com/fluxcd/image-automation-controller/blob/v0.21.3/CHANGELOG.md)
- notification-controller to [v0.23.2](https://github.com/fluxcd/notification-controller/blob/v0.23.2/CHANGELOG.md)

## CLI Changelog

- PR #2594 - @fluxcdbot - Update toolkit components
- PR #2584 - @souleb - Diff: Update homeport/Dyff to v1.5.2

