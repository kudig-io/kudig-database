---
title: flux v0.19 Release Notes
description: flux v0.19 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.19 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- flux v0.19 Release Notes 是什么
- 如何 flux v0.19 Release Notes
trigger_keywords:
- flux
- v0.19
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




# [[Flux|flux]] v0.19 Release Notes

Source: [v0.19.1](https://github.com/fluxcd/flux2/releases/tag/v0.19.1)

If you are upgrading from 0.17 or older versions, please see the [Upgrade Flux to the v1beta2 API](https://github.com/fluxcd/flux2/discussions/1916) guide.

CHANGELOG
- PR #1996 - @hiddeco - e2e/azure: update dependencies
- PR #1993 - @fluxcdbot - Update toolkit components
- PR #1985 - @makkes - Add Max Jonas Werner to maintainer list
- PR #1984 - @stefanprodan - Fix bootstrap path check
- PR #1983 - @SomtochiAma - Add unit tests for create secret export
- PR #1982 - @stefanprodan - Add poll interval flag to flux check cmd
- PR #1978 - @darkowlzz - Minor improvements in the release procedure docs
- PR #1977 - @stefanprodan - e2e: Add test for libgit2 tag semver range
- PR #1976 - @stefanprodan - Install envtest before running the unit tests
- PR #1975 - @johngmyers - Fix inadequate quoting of KUBEBUILDER_ASSETS
- PR #1970 - @phillebaba - Fix infrastructure clean up on Azure e2e test failure


## Docker images

- `docker pull fluxcd/flux-cli:v0.19.1`
- `docker pull ghcr.io/fluxcd/flux-cli:v0.19.1`


<!-- risk-assessed -->
