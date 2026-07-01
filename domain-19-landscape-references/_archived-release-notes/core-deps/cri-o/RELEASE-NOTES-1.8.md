---
title: cri-o v1.8 Release Notes
description: cri-o v1.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cri-o v1.8 Release Notes 是什么
- 如何 cri-o v1.8 Release Notes
trigger_keywords:
- cri-o
- v1.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# cri-o v1.8 Release Notes

Source: [v1.8.5](https://github.com/cri-o/cri-o/releases/tag/v1.8.5)

Welcome to the v1.8.5 release of CRI-O!




Please try out the release binaries and report any issues at
https://github.[[entities/kubernetes.md|kubernetes]]-incubator/cri-o/issues.



### Contributors

* Antonio Murdaca
* Daniel J Walsh
* Mrunal Patel

### Changes

* 70cd264be version: bump v1.8.5
* 278bb8568 Merge pull request #1311 from runcom/fix-listen-1.8
* ff1544999 cmd/crio: fix listen address dir creation
* fcf909a70 Merge pull request #1202 from runcom/auto-build-ci-sys-cont-1.8
* 9869ff489 Merge pull request #1233 from runcom/lock-1.8
* 77fb62e35 lib,oci: drop stateLock when possible
* 912754b55 Merge pull request #1226 from runcom/sys-cont-1.8
* 7d945a9f7 contrib: import system containers
* fd78dea27 Merge pull request #1211 from mrunalp/v1.8.4
* 77a522335 version: bump to v1.8.5-dev
* 34fc624d3 contrib: test: add CI system container

### Dependency Changes

Previous release can be found at [v1.8.4](https://github.com/kubernetes-incubator/cri-o/releases/tag/v1.8.4)
