---
title: rook v1.3 Release Notes
description: rook v1.3 Release Notes — Kubernetes 生产运维知识库
summary: rook v1.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rook
- ceph
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v1.3 Release Notes 是什么
- 如何 rook v1.3 Release Notes
trigger_keywords:
- rook
- v1.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Rook|rook]] v1.3 Release Notes

Source: [v1.3.11](https://github.com/rook/rook/releases/tag/v1.3.11)

# Improvements

Rook v1.3.11 is a patch release limited in scope to a single bug fix.

## Ceph
- The Ceph-CSI driver was being unexpectedly removed by the garbage collector in some clusters. For more details to apply a fix during the upgrade to this patch release, see [these steps](https://github.com/rook/rook/issues/6162#issuecomment-691273679). (#6162, @Madhu-1)