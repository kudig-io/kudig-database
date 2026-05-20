---
title: rook v0.9 Release Notes
description: rook v0.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rook
- ceph
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v0.9 Release Notes 是什么
- 如何 rook v0.9 Release Notes
trigger_keywords:
- rook
- v0.9
- Release
- Notes
- release
- notes
---

# rook v0.9 Release Notes

Source: [v0.9.3](https://github.com/rook/rook/releases/tag/v0.9.3)

Rook v0.9.3 is a patch release limited in scope and focusing on bug fixes.

## Improvements

### Cassandra
- Fix the mount point for the PVs (#2443, @yanniszark)

### Ceph
- Improve mon failover cleanup and operator restart during failover (#2262 #2570, @travisn)
- Enable host ipc for osd encryption (#923, @noahdesu)  
- Add missing "host path requires privileged" setting to the helm chart (#2735, @galexrt)