---
title: rook v1.8 Release Notes
description: rook v1.8 Release Notes — Kubernetes 生产运维知识库
summary: rook v1.8 Release Notes — Kubernetes 生产运维知识库
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
- rook v1.8 Release Notes 是什么
- 如何 rook v1.8 Release Notes
trigger_keywords:
- rook
- v1.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Rook|rook]] v1.8 Release Notes

Source: [v1.8.10](https://github.com/rook/rook/releases/tag/v1.8.10)

# Improvements
Rook v1.8.10 is a patch release limited in scope and focusing on small feature additions and bug fixes to the Ceph operator.

- core: Improve detection of filesystem properties for disk in use (#10230, @leseb)
- osd: Remove broken argument for upgraded OSDs on PVCs in legacy lvm mode (#10298, @leseb)
- osd: Allow the osd to take two hours to start in case of ceph maintenance (#10250, @travisn)
- operator: Report telemetry 'rook/version' in mon store (#10161, @BlaineEXE)

