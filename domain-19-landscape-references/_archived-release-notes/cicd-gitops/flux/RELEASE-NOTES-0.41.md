---
title: flux v0.41 Release Notes
description: flux v0.41 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.41 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
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
- flux v0.41 Release Notes 是什么
- 如何 flux v0.41 Release Notes
trigger_keywords:
- flux
- v0.41
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---



# [[Flux|flux]] v0.41 Release Notes

Source: [v0.41.2](https://github.com/fluxcd/flux2/releases/tag/v0.41.2)

Flux v0.41.2 is a patch release which fixes a range of bugs found in the controllers. Please refer to the individual component changelogs for more information.

:bulb: For more information about other features introduced in v0.41.0, please refer to [the changelog for this version](https://github.com/fluxcd/flux2/releases/tag/v0.41.0).

## Components Changelog

- source-controller [v0.36.1](https://github.com/fluxcd/source-controller/blob/v0.36.1/CHANGELOG.md)
- kustomize-controller [v0.35.1](https://github.com/fluxcd/kustomize-controller/blob/v0.35.1/CHANGELOG.md)
- helm-controller [v0.31.2](https://github.com/fluxcd/helm-controller/blob/v0.31.2/CHANGELOG.md)
- image-reflector-controller [v0.26.1](https://github.com/fluxcd/image-reflector-controller/blob/v0.26.1/CHANGELOG.md)

## CLI Changelog

- PR #3710 - @hiddeco - tests/azure: update toolkit components
- PR #3707 - @fluxcdbot - Update toolkit components
- PR #3706 - @hiddeco - build: update `actions/setup-go` in workflows
- PR #3704 - @dependabot[bot] - build(deps): bump peter-evans/create-pull-request from 4.2.3 to 4.2.4
- PR #3703 - @dependabot[bot] - build(deps): bump github/codeql-action from 2.2.6 to 2.2.7
- PR #3701 - @dependabot[bot] - build(deps): bump actions/checkout from 3.3.0 to 3.4.0
- PR #3685 - @dependabot[bot] - build(deps): bump actions/cache from 3.2.6 to 3.3.0
- PR #3684 - @dependabot[bot] - build(deps): bump github/codeql-action from 2.2.5 to 2.2.6
- PR #3683 - @dependabot[bot] - build(deps): bump docker/setup-buildx-action from 2.4.1 to 2.5.0

