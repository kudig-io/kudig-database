---
title: prometheus v2.48 Release Notes
description: prometheus v2.48 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.48 Release Notes 是什么
- 如何 prometheus v2.48 Release Notes
trigger_keywords:
- prometheus
- v2.48
- Release
- Notes
- release
- notes
---

# prometheus v2.48 Release Notes

Source: [v2.48.1](https://github.com/prometheus/prometheus/releases/tag/v2.48.1)

* [BUGFIX] TSDB: Make the wlog watcher read segments synchronously when not tailing. #13224
* [BUGFIX] Agent: Participate in notify calls (fixes slow down in remote write handling introduced in 2.45). #13223
