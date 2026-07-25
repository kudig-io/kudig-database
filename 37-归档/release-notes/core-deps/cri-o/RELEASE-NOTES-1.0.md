---
title: cri-o v1.0 Release Notes
description: cri-o v1.0 Release Notes — Kubernetes 生产运维知识库
summary: cri-o v1.0 Release Notes — Kubernetes 生产运维知识库
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
- cri-o v1.0 Release Notes 是什么
- 如何 cri-o v1.0 Release Notes
trigger_keywords:
- cri-o
- v1.0
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




# cri-o v1.0 Release Notes

Source: [v1.0.9](https://github.com/cri-o/cri-o/releases/tag/v1.0.9)

Welcome to the v1.0.9 release of CRI-O!




Please try out the release binaries and report any issues at
https://github.[[实体/kubernetes.md|kubernetes]]-incubator/cri-o/issues.



### Contributors

* Antonio Murdaca
* Daniel J Walsh
* Mrunal Patel

### Changes

* d170260fc version: bump v1.0.9
* 0f7d5d583 cmd/crio: fix listen address dir creation
* 2d6cd9f0a Merge pull request #1203 from runcom/auto-build-ci-sys-cont-1.7
* 215b2e633 Merge pull request #1231 from runcom/lock-1.7
* 699aefa3c lib,oci: drop stateLock when possible
* 5a83e5e97 Merge pull request #1224 from runcom/sys-cont-1.7
* 585cf56fe contrib: import system containers
* bdaba5c4e Merge pull request #1212 from runcom/bump-v1.0.8
* ddfa98207 version: bump v1.0.9-dev
* 65c60e02d contrib: test: add CI system container

### Dependency Changes

Previous release can be found at [v1.0.8](https://github.com/kubernetes-incubator/cri-o/releases/tag/v1.0.8)



<!-- risk-assessed -->
