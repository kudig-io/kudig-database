---
title: flux v2.2 Release Notes
description: flux v2.2 Release Notes — Kubernetes 生产运维知识库
summary: flux v2.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- helm
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
- flux v2.2 Release Notes 是什么
- 如何 flux v2.2 Release Notes
trigger_keywords:
- flux
- v2.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- monitoring-basics
---



# [[Flux|flux]] v2.2 Release Notes

Source: [v2.2.3](https://github.com/fluxcd/flux2/releases/tag/v2.2.3)

## Highlights

Flux v2.2.3 is a patch release which comes with various fixes and improvements. Users are encouraged to upgrade for the best experience.

:bulb: For upgrading to Flux v2.2, please see [the procedure documented in 2.2.0](https://github.com/fluxcd/flux2/releases/tag/v2.2.0).

This release updates the [[Kubernetes|Kubernetes]] dependencies to v1.28.6 and various other dependencies to their latest version to patch upstream CVEs.

All controllers are built with Go 1.21.6 using Alpine Linux 3.19.1 base image.

> [!NOTE]
> Due to breaking changes in [Helm v3.14.0](https://github.com/helm/helm/releases/tag/v3.14.0), the helm-controller version included in this patch release comes with Helm SDK v3.13.3.
> A preview build of the helm-controller with the latest Helm SDK is available at [helm-controller#879](https://github.com/fluxcd/helm-controller/pull/879).

Fixes:

- Reconciling empty directories and directories without Kubernetes manifests no longer results in an error. This regressing bug was introduced with the kustomize-controller upgrade to Kustomize v5.3 and has been fixed in this patch release.
- The regression due to which `Roles` and `ClusterRoles` with aggregated roles were continuous reconciled by kustomize-controller has been fixed.
- Fix the Git revision displaying when notification-controller sends alerts to Grafana.
- The HelmRelease status reporting has been improved by ensuring that the stale failure conditions get updated after failure recovery.

See the components changelog for a full list of bug fixes.

## Components changelog

- source-controller [v1.2.4](https://github.com/fluxcd/source-controller/blob/v1.2.4/CHANGELOG.md)
- kustomize-controller [v1.2.2](https://github.com/fluxcd/kustomize-controller/blob/v1.2.2/CHANGELOG.md)
- notification-controller [v1.2.4](https://github.com/fluxcd/notification-controller/blob/v1.2.4/CHANGELOG.md)
- helm-controller [v0.37.4](https://github.com/fluxcd/helm-controller/blob/v0.37.4/CHANGELOG.md)
- image-reflector-controller [v0.31.2](https://github.com/fluxcd/image-reflector-controller/blob/v0.31.2/CHANGELOG.md)
- image-automation-controller [v0.37.1](https://github.com/fluxcd/image-automation-controller/blob/v0.37.1/CHANGELOG.md)

## CLI Changelog

- PR #4589 - @stefanprodan - Update dependencies
- PR #4585 - @dependabot[bot] - build(deps): bump the ci group with 3 updates
- PR #4583 - @fluxcdbot - Update toolkit components
- PR #4575 - @stefanprodan - Update dependencies to Kubernetes v1.28.6
- PR #4573 - @dependabot[bot] - build(deps): bump the ci group with 5 updates
- PR #4558 - @twinguy - `flux check` should error on unrecognised args
- PR #4557 - @twinguy - `flux stats` should error on unrecognised args
- PR #4554 - @dependabot[bot] - build(deps): bump the ci group with 3 updates
- PR #4553 - @twinguy - Properly detect unexpected arguments during uninstall
- PR #4535 - @dependabot[bot] - build(deps): bump github.com/cloudflare/circl from 1.3.6 to 1.3.7
- PR #4533 - @darkowlzz - tests/int: Add separate resource cleanup step
