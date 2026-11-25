---
title: flux v0.25 Release Notes
description: flux v0.25 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.25 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- flux v0.25 Release Notes 是什么
- 如何 flux v0.25 Release Notes
trigger_keywords:
- flux
- v0.25
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




# [[Flux|flux]] v0.25 Release Notes

Source: [v0.25.3](https://github.com/fluxcd/flux2/releases/tag/v0.25.3)

## CLI Changelog
- PR #2305 - @stefanprodan - Update kubectl to 1.23.1 in flux-cli container image
- PR #2304 - @stefanprodan - ci: Fix release notes generator
- PR #2301 - @stefanprodan - Sign the release artifacts checksums and images
- PR #2300 - @stefanprodan - Fix Azure e2e tests and GoReleaser buildx directive
- PR #2296 - @relu - Fix Archlinux PKGBUILD check() run on ARM
- PR #2295 - @stefanprodan - Publish Flux Software Bill of Materials (SBOM)
- PR #2294 - @stefanprodan - Improve the bootstrap e2e test workflow



<!-- risk-assessed -->
