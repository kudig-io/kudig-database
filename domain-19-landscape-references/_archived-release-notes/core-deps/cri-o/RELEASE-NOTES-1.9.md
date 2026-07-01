---
title: cri-o v1.9 Release Notes
description: cri-o v1.9 Release Notes — Kubernetes 生产运维知识库
summary: cri-o v1.9 Release Notes — Kubernetes 生产运维知识库
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
- cri-o v1.9 Release Notes 是什么
- 如何 cri-o v1.9 Release Notes
trigger_keywords:
- cri-o
- v1.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# cri-o v1.9 Release Notes

Source: [v1.9.16](https://github.com/cri-o/cri-o/releases/tag/v1.9.16)

CRI-O 1.9.16

Welcome to the v1.9.16 release of CRI-O!



Please try out the release binaries and report any issues at
https://github.com/kubernetes-sigs/cri-o/issues.

### Contributors

* Mrunal Patel
* Giuseppe Scrivano
* Urvashi Mohnani
* Antonio Murdaca
* Sascha Grunert

### Changes

* 743773c0a Tag v1.9.16
* 6e2ced68a Merge pull request #2516 from umohnani8/1.9
* 06dd0aab7 Add state of infracontainer to disk when stopped
* 7155a7596 Merge pull request #2182 from openSUSE/release-1.9-oob-log-fix
* 1b650eb8f Fix possible out of bounds access during log parsing
* f9033b742 Merge pull request #2142 from mrunalp/carry_1973
* c25e360e9 test: Switch to custom k8s fork as upstream is broken
* dbf5c04e1 container_create: fix race with sandbox being stopped
* ce28be370 server: serialize StopPodSandbox for the same sandbox
* e5f784326 sandbox: simplify if condition
* 65493c794 test: Don't build [[Kubernetes|kubernetes]]
* 1129fb660 Update golang to 1.11.6
* 2a2db8ffc runPodSandbox: clean up containers on error path
* 8b405135e version: v1.9.16-dev

### Dependency Changes

Previous release can be found at [v1.9.15](https://github.com/kubernetes-sigs/cri-o/releases/tag/v1.9.15)