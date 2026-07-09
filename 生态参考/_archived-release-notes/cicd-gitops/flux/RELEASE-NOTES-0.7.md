---
title: flux v0.7 Release Notes
description: flux v0.7 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.7 Release Notes — Kubernetes 生产运维知识库
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
- flux v0.7 Release Notes 是什么
- 如何 flux v0.7 Release Notes
trigger_keywords:
- flux
- v0.7
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




# [[Flux|flux]] v0.7 Release Notes

Source: [v0.7.7](https://github.com/fluxcd/flux2/releases/tag/v0.7.7)

CHANGELOG
- PR #867 - @stefanprodan - Add support for getting resources by name
- PR #866 - @stefanprodan - Add support for multiple values files to create hr
- PR #863 - @hiddeco - Fix GitLab bootstrap by using (sub)group paths
- PR #862 - @squaremo - Rename flux delete auto to flux delete image
- PR #861 - @stefanprodan - Refactor components status check
- PR #858 - @hiddeco - Put CHANGELOG URL on new line in commit / PR body
- PR #857 - @fluxcdbot - Update toolkit components
- PR #854 - @hiddeco - Move migration sub-menu to top-menu
- PR #853 - @hiddeco - Tune component update configuration
- PR #845 - @jonathan-innis - Replace kubectl rollout with kstatus checks



<!-- risk-assessed -->
