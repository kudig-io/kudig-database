---
title: rook v1.16 Release Notes
description: rook v1.16 Release Notes — Kubernetes 生产运维知识库
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
- rook v1.16 Release Notes 是什么
- 如何 rook v1.16 Release Notes
trigger_keywords:
- rook
- v1.16
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# rook v1.16 Release Notes

Source: [v1.16.9](https://github.com/rook/rook/releases/tag/v1.16.9)

# Improvements
Rook v1.16.9 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- discover: Delete ceph-volume log to reduce discover daemon log size (#16276, @anshuman-agarwala)
- core: Update go modules needed for security checks (#16140, @travisn)
- core: CephObjectRealm controller generated generated AccessKey invalid chars (#16078, @raaizik)
- exporter: Add Hostnetwork bool to ceph-exporter (#16025, @adilGhaffarDev)
