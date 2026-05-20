---
title: rook v1.11 Release Notes
description: rook v1.11 Release Notes — Kubernetes 生产运维知识库
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
- rook v1.11 Release Notes 是什么
- 如何 rook v1.11 Release Notes
trigger_keywords:
- rook
- v1.11
- Release
- Notes
- release
- notes
---

# rook v1.11 Release Notes

Source: [v1.11.11](https://github.com/rook/rook/releases/tag/v1.11.11)

# Improvements
Rook v1.11.11 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- object: Unique username for OBC even when preceding OBC was retained (#12884, @haslersn)
- object: Avoid creating same bucket for two different OBC (#12804, @thotz)
- csi: Add csi pods to the list to force delete pod on an unavailable node (#12681, @Madhu-1)
- operator: Fix formatting of some logger methods (#12666, @polyedre)
