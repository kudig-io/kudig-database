---
title: rook v1.0 Release Notes
description: rook v1.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- rook
- ceph
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v1.0 Release Notes 是什么
- 如何 rook v1.0 Release Notes
trigger_keywords:
- rook
- v1.0
- Release
- Notes
- release
- notes
---

# rook v1.0 Release Notes

Source: [v1.0.6](https://github.com/rook/rook/releases/tag/v1.0.6)

Rook v1.0.6 is a patch release limited in scope and focusing on bug fixes.

# Improvements

## Ceph
- Set public-addr flag for MGR (#3136, @galexrt)
- Remove the 20GB default for OSD db size and allow ceph-volume to use all available space (#3448, @travisn)
- Correctly set osd mem target for init-ed clusters (#3638, @odinuge)  
- Properly propagate errors when deleting mds deployment (#3641, @odinuge) 
