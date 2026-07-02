---
title: kind v0.2 Release Notes
description: kind v0.2 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- kind v0.2 Release Notes 是什么
- 如何 kind v0.2 Release Notes
trigger_keywords:
- kind
- v0.2
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




# kind v0.2 Release Notes

Source: [0.2.1](https://github.com/kubernetes-sigs/kind/releases/tag/0.2.1)

0.2.1 is a bug fix release

# Breaking Changes

NONE

# New Features

-  The hostpath provisioner is now enabled by default (#397)

# Fixes

- fix `kind build node-image` on macOS, previously `--type=bazel` and `--type=docker` (the default) did not work properly on not-Linux in `0.2.0` (#413)
- fix possible panic in failed `kind create cluster` calls, previously if multiple nodes failed to come up kind could panic (#407)


<h1 id="contributors">Contributors</h1>

Thanks to everyone who committed to this release! ❤️

- @BenTheElder 
- @joejulian
- @k8s-ci-robot 
- @neolit123
- @akutz 



<!-- risk-assessed -->
