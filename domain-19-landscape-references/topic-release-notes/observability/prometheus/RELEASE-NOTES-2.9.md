---
title: prometheus v2.9 Release Notes
description: prometheus v2.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.9 Release Notes 是什么
- 如何 prometheus v2.9 Release Notes
trigger_keywords:
- prometheus
- v2.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

# prometheus v2.9 Release Notes

Source: [v2.9.2](https://github.com/prometheus/prometheus/releases/tag/v2.9.2)

* [BUGFIX] Make sure subquery range is taken into account for selection #5467
* [BUGFIX] Exhaust every request body before closing it #5166
* [BUGFIX] Cmd/promtool: return errors from rule evaluations #5483
* [BUGFIX] Remote Storage: string interner should not panic in release #5487
* [BUGFIX] Fix memory allocation regression in mergedPostings.Seek tsdb#586