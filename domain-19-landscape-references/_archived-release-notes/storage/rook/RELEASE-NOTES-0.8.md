---
title: rook v0.8 Release Notes
description: rook v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v0.8 Release Notes 是什么
- 如何 rook v0.8 Release Notes
trigger_keywords:
- rook
- v0.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Rook|rook]] v0.8 Release Notes

Source: [v0.8.3](https://github.com/rook/rook/releases/tag/v0.8.3)

Rook v0.8.3 is a patch release limited in scope and focusing on bug fixes.

## Improvements
- OSD can now be configured in K8s clusters where the [hostname label is different from the node name](https://github.com/rook/rook/issues/2148)  (@travisn)
- Fix regression in v0.8.2 that caused [PVCs to fail](https://github.com/rook/rook/issues/2149) to be mounted in some clusters due to unexpected logging (@rootfs)