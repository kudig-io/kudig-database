---
title: rook v1.1 Release Notes
description: rook v1.1 Release Notes — Kubernetes 生产运维知识库
summary: rook v1.1 Release Notes — Kubernetes 生产运维知识库
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
- rook v1.1 Release Notes 是什么
- 如何 rook v1.1 Release Notes
trigger_keywords:
- rook
- v1.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Rook|rook]] v1.1 Release Notes

Source: [v1.1.9](https://github.com/rook/rook/releases/tag/v1.1.9)

# Improvements

Rook v1.1.9 is a patch release limited in scope and focusing on bug fixes.

## Ceph
- CSI driver handling of upgrade from OCP 4.2 to OCP 4.3 (#4650, @Madhu-1)
- Fix object bucket provisioner when rgw not on port 80 (#4049, @bsperduto)
- Only perform upgrade checks when the Ceph image changes (#4379, @travisn)
