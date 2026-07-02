---
title: flux v0.29 Release Notes
description: flux v0.29 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.29 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- flux
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- flux v0.29 Release Notes 是什么
- 如何 flux v0.29 Release Notes
trigger_keywords:
- flux
- v0.29
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Flux|flux]] v0.29 Release Notes

Source: [v0.29.5](https://github.com/fluxcd/flux2/releases/tag/v0.29.5)

Flux v0.29.5 is patch release which improves the Condition handling of `HelmRepository` resources, and handling of file formats while decrypting Secret generator entries with [[SOPS|SOPS]] to ensure encrypted files in format A can be decrypted to target format B.

In addition, we now recover from Kustomize build panics to guarantee continuity of operations when running into invalid object data.

**Note** that [v0.29.0](https://github.com/fluxcd/flux2/releases/v0.29.0) includes breaking changes.

## Components Changelog

- source-controller to [v0.24.3](https://github.com/fluxcd/source-controller/blob/v0.24.3/CHANGELOG.md)
- kustomize-controller to [v0.24.4](https://github.com/fluxcd/kustomize-controller/blob/v0.24.4/CHANGELOG.md)

## CLI Changelog
- PR #2686 - @fluxcdbot - Update toolkit components



<!-- risk-assessed -->
