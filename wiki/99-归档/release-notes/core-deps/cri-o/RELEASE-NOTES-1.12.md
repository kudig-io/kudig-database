---
title: cri-o v1.12 Release Notes
description: cri-o v1.12 Release Notes — Kubernetes 生产运维知识库
summary: cri-o v1.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cri-o v1.12 Release Notes 是什么
- 如何 cri-o v1.12 Release Notes
trigger_keywords:
- cri-o
- v1.12
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




# cri-o v1.12 Release Notes

Source: v1.12.10](https://github.com/cri-o/cri-o/releases/tag/v1.12.10)

CRI-O 1.12.10

Welcome to the v1.12.10 release of CRI-O!



Please try out the release binaries and report any issues at
https://github.com/kubernetes-sigs/cri-o/issues.

### Contributors

* Giuseppe Scrivano
* Mrunal Patel
* Urvashi Mohnani

### Changes

* 2c94bb71 version: 1.12.10
* 4e37578a Merge pull request #2148 from giuseppe/race-fixes-1.12
* f7f31279 container_create: fix race with sandbox being stopped
* 81f3ac0c server: serialize StopPodSandbox for the same sandbox
* c507f147 sandbox: simplify if condition
* 47efa17d version: v1.12.10-dev

### Dependency Changes

Previous release can be found at [v1.12.9](https://github.com/kubernetes-sigs/cri-o/releases/tag/v1.12.9)

<!-- risk-assessed -->
