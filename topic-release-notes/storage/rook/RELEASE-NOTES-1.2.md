---
title: rook v1.2 Release Notes
description: rook v1.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- helm
- rook
- ceph
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v1.2 Release Notes 是什么
- 如何 rook v1.2 Release Notes
trigger_keywords:
- rook
- v1.2
- Release
- Notes
- release
- notes
---

# rook v1.2 Release Notes

Source: [v1.2.7](https://github.com/rook/rook/releases/tag/v1.2.7)

# Improvements

Rook v1.2.7 is a patch release limited in scope and focusing on bug fixes.

## Ceph

- Apply the expected lower PG count for rgw metadata pools (#5091, @travisn)
- Reject devices smaller than 5GiB for OSDs (#5089, @leseb)
- Add extra check for filesystem to skip boot volumes for OSD configuration (#5022, @leseb)
- Avoid duplication of mon pod anti-affinity (#4998, @travisn)
- Update service monitor definition during upgrade (#5078, @umangachapagain)
- Resizer container fix due to misinterpretation of the cephcsi version (#5073, @Madhu-1)
- Set ResourceVersion for Prometheus rules (#4528, @galexrt)
- Upgrade doc clarification for RBAC related to the helm chart (#5054, @PCatinean)