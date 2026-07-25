---
title: flux v2.7 Release Notes
description: flux v2.7 Release Notes — Kubernetes 生产运维知识库
summary: flux v2.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- flux v2.7 Release Notes 是什么
- 如何 flux v2.7 Release Notes
trigger_keywords:
- flux
- v2.7
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




# [[Flux|flux]] v2.7 Release Notes

Source: [v2.7.5](https://github.com/fluxcd/flux2/releases/tag/v2.7.5)

## Highlights

Flux v2.7.5 is a patch release that comes with fixes to helm-controller. Users are encouraged to upgrade for the best experience.

ℹ️ Please follow the [Upgrade Procedure for Flux v2.7+](https://github.com/fluxcd/flux2/discussions/5572) for a smooth upgrade from Flux v2.6 to the latest version.

Fixes:

- Fix HelmRelease history truncation when using the `RetryOnFailure` strategy.

:warning: Note that signature verification for OCI artifacts in source-controller is not compatible with Cosign v3.
Flux users are advised to use [Cosign v2.6](https://fluxcd.io/flux/flux-gh-action/#push-and-sign-kubernetes-manifests-to-container-registries) for signing Flux OCI artifacts and [[Helm|Helm]] charts, until support for Cosign v3 is added in Flux v2.8.

## Components changelog

- helm-controller [v1.4.5](https://github.com/fluxcd/helm-controller/blob/v1.4.5/CHANGELOG.md)

## CLI changelog

* [release/v2.7.x] Update toolkit components by @fluxcdbot in https://github.com/fluxcd/flux2/pull/5649


**Full Changelog**: https://github.com/fluxcd/flux2/compare/v2.7.4...v2.7.5



<!-- risk-assessed -->
