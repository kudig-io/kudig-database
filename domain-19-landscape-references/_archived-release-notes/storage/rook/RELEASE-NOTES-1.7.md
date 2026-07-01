---
title: rook v1.7 Release Notes
description: rook v1.7 Release Notes — Kubernetes 生产运维知识库
summary: rook v1.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rook
- ceph
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v1.7 Release Notes 是什么
- 如何 rook v1.7 Release Notes
trigger_keywords:
- rook
- v1.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Rook|rook]] v1.7 Release Notes

Source: [v1.7.11](https://github.com/rook/rook/releases/tag/v1.7.11)

# Improvements
Rook v1.7.11 is a patch release limited in scope and focusing on small feature additions and bug fixes to the Ceph operator.

- mgr: Update services with the `app=rook-ceph-mgr` label when the active Ceph mgr changes (#9467, @travisn)
- osd: Correct bluestore compression min blob size for ssd (#9582, @subhamkrai)
- build: Update to go v1.16.12 (#9478, @BlaineEXE)