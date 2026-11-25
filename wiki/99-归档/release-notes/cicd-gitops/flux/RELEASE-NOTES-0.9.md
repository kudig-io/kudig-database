---
title: flux v0.9 Release Notes
description: flux v0.9 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.9 Release Notes — Kubernetes 生产运维知识库
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
- flux v0.9 Release Notes 是什么
- 如何 flux v0.9 Release Notes
trigger_keywords:
- flux
- v0.9
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




# [[Flux|flux]] v0.9 Release Notes

Source: [v0.9.1](https://github.com/fluxcd/flux2/releases/tag/v0.9.1)

CHANGELOG
- PR #1048 - @hiddeco - Restore default key algorithm flag create source
- PR #1043 - @fluxcdbot - Update toolkit components
- PR #1034 - @stealthybox - Fix anchor on kustomize migration link
- PR #1033 - @stefanprodan - Move the v1 vs v2 FAQ to the migration section
- PR #1022 - @hiddeco - Use path with '/' slashes in created Kustomization
- PR #1018 - @tvories - [docs] Fix fish completions example
- PR #1008 - @stefanprodan - faq: Can I use Flux HelmReleases without GitOps?
- PR #1001 - @hiddeco - Add `sourcesecret` and `kustomization` manifestgen



<!-- risk-assessed -->
