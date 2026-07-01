---
title: flux v2.8 Release Notes
description: flux v2.8 Release Notes — Kubernetes 生产运维知识库
summary: flux v2.8 Release Notes — Kubernetes 生产运维知识库
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
- flux v2.8 Release Notes 是什么
- 如何 flux v2.8 Release Notes
trigger_keywords:
- flux
- v2.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Flux|flux]] v2.8 Release Notes

Source: [v2.8.5](https://github.com/fluxcd/flux2/releases/tag/v2.8.5)

## Highlights

Flux v2.8.5 is a patch release that includes bug fixes and improvements across kustomize-controller, source-controller, and notification-controller. Users are encouraged to upgrade for the best experience.

ℹ️ Please follow the [Upgrade Procedure for Flux v2.7+](https://github.com/fluxcd/flux2/discussions/5572) for a smooth upgrade from Flux v2.6 to the latest version.

Fixes:

- Fix a race condition where a cancelled reconciliation could leave stale data in the cache, causing Kustomizations to get stuck (kustomize-controller)
- Fix Azure Blob prefix option not being passed to the storage client (source-controller)

Improvements:

- Improve error message for encrypted SSH keys without password (source-controller)
- Add optional `email` and `audience` fields to the GCR Receiver for tighter verification (notification-controller)
- Add provider manifest example for Azure Event Hub managed identity authentication (notification-controller)

## Components changelog

- kustomize-controller [v1.8.3](https://github.com/fluxcd/kustomize-controller/blob/v1.8.3/CHANGELOG.md)
- source-controller [v1.8.2](https://github.com/fluxcd/source-controller/blob/v1.8.2/CHANGELOG.md)
- notification-controller [v1.8.3](https://github.com/fluxcd/notification-controller/blob/v1.8.3/CHANGELOG.md)

## CLI changelog
* Update toolkit components by @fluxcdbot in https://github.com/fluxcd/flux2/pull/5822


**Full Changelog**: https://github.com/fluxcd/flux2/compare/v2.8.4...v2.8.5