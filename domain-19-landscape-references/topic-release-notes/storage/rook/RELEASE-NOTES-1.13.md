---
title: rook v1.13 Release Notes
description: rook v1.13 Release Notes — Kubernetes 生产运维知识库
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
- rook v1.13 Release Notes 是什么
- 如何 rook v1.13 Release Notes
trigger_keywords:
- rook
- v1.13
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Rook|rook]] v1.13 Release Notes

Source: [v1.13.10](https://github.com/rook/rook/releases/tag/v1.13.10)

# Improvements
Rook v1.13.10 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- osd: Fix activate failure when block device moves (#14374, @BlaineEXE)
- csi: Update csi-addons repo link for correctly versioned download (#14408, @Madhu-1)
