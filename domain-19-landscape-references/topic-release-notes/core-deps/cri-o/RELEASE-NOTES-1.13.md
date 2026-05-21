---
title: cri-o v1.13 Release Notes
description: cri-o v1.13 Release Notes — Kubernetes 生产运维知识库
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
- cri-o v1.13 Release Notes 是什么
- 如何 cri-o v1.13 Release Notes
trigger_keywords:
- cri-o
- v1.13
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# cri-o v1.13 Release Notes

Source: [v1.13.12](https://github.com/cri-o/cri-o/releases/tag/v1.13.12)

CRI-O 1.13.12

Welcome to the v1.13.12 release of CRI-O!



Please try out the release binaries and report any issues at
https://github.com/cri-o/cri-o/issues.

### Contributors

* Urvashi Mohnani
* Daniel J Walsh
* Mrunal Patel
* Peter Hunt
* Valentin Rothberg

### Changes

* 6957864d8 Add the container IDs that cri-o assigns to various logs
* 0dcba1f1a Bump version to 1.13.12
* 3735d11e4 Vendor latest opencontainers/runtime-tools
* d124cec8e [1.13] update github.com/containers/image
* d162b4edc test: test failures and successes correctly

### Dependency Changes

Previous release can be found at [v1.13.11](https://github.com/cri-o/cri-o/releases/tag/v1.13.11)

* **github.com/containers/image**  915c7e6d2070 -> 24693941df9d
* **github.com/opencontainers/runtime-spec**   v1.0.0 -> 1722abf79c2f
* **github.com/opencontainers/runtime-tools**  1c243a8a8eb4 -> d1bf3e66ff0a