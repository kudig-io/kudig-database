---
title: rook v1.6 Release Notes
description: rook v1.6 Release Notes — Kubernetes 生产运维知识库
summary: rook v1.6 Release Notes — Kubernetes 生产运维知识库
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
- rook v1.6 Release Notes 是什么
- 如何 rook v1.6 Release Notes
trigger_keywords:
- rook
- v1.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Rook|rook]] v1.6 Release Notes

Source: [v1.6.11](https://github.com/rook/rook/releases/tag/v1.6.11)

# Improvements
Rook v1.6.11 is a patch release limited in scope and focusing on small feature additions and bug fixes to the Ceph operator.
- rgw: Allow reconcile to complete even during a downgrade (#9137, @travisn)
- docs: Add OMAP quick fix warning to the upgrade guide (#9187, @BlaineEXE)
- multus: Do not build all the args to remote exec cmd (#8860, @leseb)
- multus: do not fail on keys deletion (#8868, @leseb)

