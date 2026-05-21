---
title: prometheus v2.47 Release Notes
description: prometheus v2.47 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.47 Release Notes 是什么
- 如何 prometheus v2.47 Release Notes
trigger_keywords:
- prometheus
- v2.47
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

# prometheus v2.47 Release Notes

Source: [v2.47.2](https://github.com/prometheus/prometheus/releases/tag/v2.47.2)

This is a patch release to fix a bug, and to rebuild with Go 1.21.3.

* [BUGFIX] TSDB: Fix counter reset edgecases causing native histogram panics. #12838
