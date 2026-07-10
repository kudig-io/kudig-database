---
title: flux v0.4 Release Notes
description: flux v0.4 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.4 Release Notes — Kubernetes 生产运维知识库
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
- flux v0.4 Release Notes 是什么
- 如何 flux v0.4 Release Notes
trigger_keywords:
- flux
- v0.4
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




# [[Flux|flux]] v0.4 Release Notes

Source: [v0.4.3](https://github.com/fluxcd/flux2/releases/tag/v0.4.3)

CHANGELOG
- PR #557 - @hiddeco - Properly clean-up package build dirs
- PR #556 - @relu - Use mock archive for aur publishers
- PR #555 - @relu - Remove `ids` GoReleaser attr in AUR pkg publisher
- PR #554 - @relu - Fix GoReleaser AUR publish concurrent exec issue
- PR #553 - @relu - Fix GoReleaser AUR package publishing
- PR #552 - @relu - Fix GitHub Actions release workflow syntax error
- PR #551 - @fluxcdbot - Update helm-controller to v0.4.2
- PR #550 - @stefanprodan - Publish install manifest to GitHub releases
- PR #549 - @relu - Add AUR_BOT_SSH_PRIVATE_KEY env var for goreleaser
- PR #548 - @relu - Fix list parsing issue in the docs
- PR #547 - @stefanprodan - Add create secret git command
- PR #546 - @stefanprodan - Add labels to generated [[Secrets|secrets]]
- PR #535 - @stefanprodan - Automate Flux upgrades with GitHub Actions
- PR #534 - @stefanprodan - Specify where to place Kubernetes manifests after bootstrap
- PR #532 - @relu - Automated AUR publishing



<!-- risk-assessed -->
