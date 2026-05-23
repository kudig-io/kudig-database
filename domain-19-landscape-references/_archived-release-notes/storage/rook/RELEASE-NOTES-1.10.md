---
title: rook v1.10 Release Notes
description: rook v1.10 Release Notes — Kubernetes 生产运维知识库
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
- rook v1.10 Release Notes 是什么
- 如何 rook v1.10 Release Notes
trigger_keywords:
- rook
- v1.10
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Rook|rook]] v1.10 Release Notes

Source: v1.10.13](https://github.com/rook/rook/releases/tag/v1.10.13)

# Improvements
Rook v1.10.13 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- osd: Handle global or node-local device class configuration correctly (#11966, @satoru-takeuchi)
- manifest: Add missing quote (#11880, @DjVinnii)
- object: Make OBC genUserID unique across clusters (#11665, @BlaineEXE)
- file: Check if the filesystem exists before checking dependencies (#11221, @zhucan)
- core: On crash pod ensure rook version label is not set (#11760, @gaord)
