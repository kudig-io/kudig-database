---
title: flux v0.40 Release Notes
description: flux v0.40 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.40 Release Notes — Kubernetes 生产运维知识库
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
- flux v0.40 Release Notes 是什么
- 如何 flux v0.40 Release Notes
trigger_keywords:
- flux
- v0.40
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Flux|flux]] v0.40 Release Notes

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



<!-- risk-assessed -->
