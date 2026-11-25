---
title: flux v2.5 Release Notes
description: flux v2.5 Release Notes — Kubernetes 生产运维知识库
summary: flux v2.5 Release Notes — Kubernetes 生产运维知识库
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
- flux v2.5 Release Notes 是什么
- 如何 flux v2.5 Release Notes
trigger_keywords:
- flux
- v2.5
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




# [[Flux|flux]] v2.5 Release Notes

Source: [v2.5.1](https://github.com/fluxcd/flux2/releases/tag/v2.5.1)

## Highlights

Flux v2.5.1 is a patch release which comes with various fixes. Users are encouraged to upgrade for the best experience.

Fixes:

- Fix a bug introduced in kustomize-controller v1.5.0 that was causing spurious logging for deprecated API versions and health check failures.
- Sanitize the kustomize-controller logs when encountering errors during [[SOPS|SOPS]] decryption.

## Components changelog

- kustomize-controller [v1.5.1](https://github.com/fluxcd/kustomize-controller/blob/v1.5.1/CHANGELOG.md)

## CLI Changelog

- PR #5215 - @matheuscscp - Update backport labels for 2.5
- PR #5214 - @fluxcdbot - Update kustomize-controller to v1.5.1



<!-- risk-assessed -->
