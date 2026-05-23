---
title: cri-o v1.15 Release Notes
description: cri-o v1.15 Release Notes — Kubernetes 生产运维知识库
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
- cri-o v1.15 Release Notes 是什么
- 如何 cri-o v1.15 Release Notes
trigger_keywords:
- cri-o
- v1.15
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# cri-o v1.15 Release Notes

Source: [v1.15.4](https://github.com/cri-o/cri-o/releases/tag/v1.15.4)

CRI-O 1.15.4

Welcome to the v1.15.4 release of CRI-O!

Note: this is the final tagged release of 1.15


Please try out the release binaries and report any issues at
https://github.com/cri-o/cri-o/issues.

### Contributors

* Peter Hunt
* Mrunal Patel
* Kir Kolyshkin
* Ryan Phillips
* Urvashi Mohnani

### Changes

* f3f90846b bump to v1.15.4
* c2ebdf9f4 server/ContainerStatus: don't lock for c.State()
* cf9047092 test: update image digest to fix test
* c216af138 test: check for rw mounts
* 4405ebb9d port error: check for error
* 3415467b9 port forward: drain the stream on error
* 5b275d4af Restore sandbox selinux labels directly from config.json
* 8ddfe046f conmonmon: errorf when OOM killing
* 4d1a0633f klog: don't write to /tmp
* 24a29f033 Pass down the integer value of the stop signal
* 0dbcac6f2 exec: Close pipe fds to prevent hangs
* a45b77090 bats: add conmonmon test suite
* dd6411c27 add conmonmon
* 685ec07bb fix selinux label on volume mount directory creation

### Dependency Changes

Previous release can be found at [v1.15.3](https://github.com/cri-o/cri-o/releases/tag/v1.15.3)

* **github.com/containers/psgo**  v1.3.2 -> v1.4.0
* **k8s.io/klog**                 v0.3.3 -> v1.0.0
