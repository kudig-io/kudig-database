---
title: cri-o v1.11 Release Notes
description: cri-o v1.11 Release Notes — Kubernetes 生产运维知识库
summary: cri-o v1.11 Release Notes — Kubernetes 生产运维知识库
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
- cri-o v1.11 Release Notes 是什么
- 如何 cri-o v1.11 Release Notes
trigger_keywords:
- cri-o
- v1.11
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




# cri-o v1.11 Release Notes

Source: [v1.11.15](https://github.com/cri-o/cri-o/releases/tag/v1.11.15)

CRI-O 1.11.15

Welcome to the v1.11.15 release of CRI-O!



Please try out the release binaries and report any issues at
https://github.com/cri-o/cri-o/issues.

### Contributors

* Peter Hunt
* Urvashi Mohnani
* Mrunal Patel

### Changes

* 3402c2c25 version 1.11.15
* f2f24f051 test: Exclude lock file in network tests
* 64e548650 partial pick of 73e72fcdf2
* 36d107e62 conmon: check it is a valid pid before killing it
* c5a915a49 backport empty circleCI config
* 74b5a1c23 Add state of infracontainer to disk when stopped
* 77aec42df Move to v1.11.15-dev

### Dependency Changes

Previous release can be found at [v1.11.14](https://github.com/cri-o/cri-o/releases/tag/v1.11.14)



<!-- risk-assessed -->
