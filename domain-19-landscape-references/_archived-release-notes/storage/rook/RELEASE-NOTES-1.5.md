---
title: rook v1.5 Release Notes
description: rook v1.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- rook v1.5 Release Notes 是什么
- 如何 rook v1.5 Release Notes
trigger_keywords:
- rook
- v1.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Rook|rook]] v1.5 Release Notes

Source: [v1.5.12](https://github.com/rook/rook/releases/tag/v1.5.12)

# Improvements
Rook v1.5.12 is a patch release limited in scope and focusing on small feature additions and bug fixes.

## Ceph
- Fix OSD hostpath to prevent risk of data corruption on restart (#7886, @satoru-takeuchi)
- Double the mon failover timeout (to 20 minutes) during node drain (#7801, @sp98)
- Improve reliability of mon failover when the operator is restarted during failover (#7884, @travisn)
- Allow heap dump generation when logCollector sidecar is not running (#7847, @leseb)
