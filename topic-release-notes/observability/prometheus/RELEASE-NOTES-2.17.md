---
title: prometheus v2.17 Release Notes
description: prometheus v2.17 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.17 Release Notes 是什么
- 如何 prometheus v2.17 Release Notes
trigger_keywords:
- prometheus
- v2.17
- Release
- Notes
- release
- notes
---

# prometheus v2.17 Release Notes

Source: [v2.17.2](https://github.com/prometheus/prometheus/releases/tag/v2.17.2)

* [BUGFIX] Federation: Register federation metrics #7081
* [BUGFIX] PromQL: Fix panic in parser error handling #7132
* [BUGFIX] Rules: Fix reloads hanging when deleting a rule group that is being evaluated #7138
* [BUGFIX] TSDB: Fix a memory leak when prometheus starts with an empty TSDB WAL #7135
* [BUGFIX] TSDB: Make isolation more robust to panics in web handlers #7129 #7136
