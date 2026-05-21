---
title: cri-o v1.16 Release Notes
description: cri-o v1.16 Release Notes — Kubernetes 生产运维知识库
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
- cri-o v1.16 Release Notes 是什么
- 如何 cri-o v1.16 Release Notes
trigger_keywords:
- cri-o
- v1.16
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# cri-o v1.16 Release Notes

Source: [v1.16.6](https://github.com/cri-o/cri-o/releases/tag/v1.16.6)

CRI-O 1.16.6

Welcome to the v1.16.6 release of CRI-O!


Please try out the release binaries and report any issues at
https://github.com/cri-o/cri-o/issues.

### Contributors

* Mrunal Patel
* Ralf Haferkamp
* Lokesh Mandvekar
* Peter Hunt

### Changes

* a3564c522 version: Release 1.16.6
* 879dc16b0 Update installation steps for CentOS
* ae2f8a856 Add docs and completions for default_env
* daf941fee Add a test for container default env
* 43ce51797 Add support for default_env in crio configuration
* bbd5647c5 bump conmon to 2.0.15
* 43d75e549 sandbox: Make sure the label annotation is proper JSON
* 9c67d5500 container_server: Wrap a few more errors in LoadSandbox

### Dependency Changes

Previous release can be found at [v1.16.5](https://github.com/cri-o/cri-o/releases/tag/v1.16.5)

* **github.com/containers/conmon**  v2.0.11 -> v2.0.15
