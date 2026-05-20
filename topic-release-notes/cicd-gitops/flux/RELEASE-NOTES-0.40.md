---
title: flux v0.40 Release Notes
description: flux v0.40 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flux
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.40 Release Notes 是什么
- 如何 flux v0.40 Release Notes
trigger_keywords:
- flux
- v0.40
- Release
- Notes
- release
- notes
---

# flux v0.40 Release Notes

Source: [v0.40.2](https://github.com/fluxcd/flux2/releases/tag/v0.40.2)

Flux v0.40.2 is a patch release which includes an update of the notification-controller to prevent an issue with the default API version used for ImageRepositories when no specific version is configured on a Receiver. Users are encouraged to upgrade for the best experience.

:warning: Note that v0.40.0 contained breaking changes, please refer to [the changelog](https://github.com/fluxcd/flux2/releases/tag/v0.40.0) for more information.

## Components changelog

- notification-controller [v0.32.1](https://github.com/fluxcd/notification-controller/blob/v0.32.1/CHANGELOG.md)

## CLI Changelog
- PR #3645 - @hiddeco - Update dependencies
- PR #3644 - @fluxcdbot - Update toolkit components
- PR #3638 - @dependabot[bot] - build(deps): bump actions/cache from 3.2.5 to 3.2.6
- PR #3637 - @dependabot[bot] - build(deps): bump github/codeql-action from 2.2.4 to 2.2.5

