---
title: flux v2.0 Release Notes
description: flux v2.0 Release Notes — Kubernetes 生产运维知识库
summary: flux v2.0 Release Notes — Kubernetes 生产运维知识库
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
- flux v2.0 Release Notes 是什么
- 如何 flux v2.0 Release Notes
trigger_keywords:
- flux
- v2.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Flux|flux]] v2.0 Release Notes

Source: [v2.0.1](https://github.com/fluxcd/flux2/releases/tag/v2.0.1)

## Highlights

Flux `v2.0.1` is a patch release which comes with various fixes. Users are encouraged to upgrade for the best experience. 

:bulb: For upgrading from Flux `v0.x`, please see [the procedure documented in 2.0.0](https://github.com/fluxcd/flux2/releases/tag/v2.0.0).

### Fixes

- Fix AWS auth for cross-region ECR repositories (`source-controller`, `image-reflector-controller`).
- Prevent spurious alerts for skipped resources (`kustomize-controller`).
- List removed resources for `flux diff ks --kustomization-file` (`flux` CLI).
- Fix SLSA provenance generation for the Flux CLI binaries.

## Components changelog

- source-controller [v1.0.1](https://github.com/fluxcd/source-controller/blob/v1.0.1/CHANGELOG.md)
- kustomize-controller [v1.0.1](https://github.com/fluxcd/kustomize-controller/blob/v1.0.1/CHANGELOG.md)
- image-reflector-controller [v0.29.1](https://github.com/fluxcd/image-reflector-controller/blob/v0.29.1/CHANGELOG.md)

## CLI Changelog

- PR #4068 - @stefanprodan - Update dependencies
- PR #4065 - @hiddeco - action: support `openssl` and `sha256sum`
- PR #4062 - @souleb - diff: Take into account the server-side inventory for local Flux Kustomizations
- PR #4061 - @hiddeco - action: re-allow configuration of non-default token
- PR #4057 - @fluxcdbot - Update toolkit components
- PR #4052 - @stefanprodan - docs: Link to the Flux GitHub Action documentation
- PR #4051 - @hiddeco - action: use `$RUNNER_TOOL_CACHE`, support MacOS and Windows, validate checksum
- PR #4046 - @stefanprodan - ci: backport: set write permissions
- PR #4043 - @stefanprodan - ci: release: extract the image tag from GITHUB_REF
- PR #4041 - @hiddeco - ci: release: disable interpretation backslash esc

## New Documentation

- [Flux GitHub Action](https://fluxcd.io/flux/flux-gh-action/)
- [SLSA provenance verification](https://fluxcd.io/flux/security/slsa-assessment/#provenance-verification)