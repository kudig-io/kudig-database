---
title: prometheus v3.7 Release Notes
description: prometheus v3.7 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v3.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v3.7 Release Notes 是什么
- 如何 prometheus v3.7 Release Notes
trigger_keywords:
- prometheus
- v3.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Prometheus|prometheus]] v3.7 Release Notes

Source: [v3.7.3](https://github.com/prometheus/prometheus/releases/tag/v3.7.3)

* [BUGFIX] UI: Revert changed (and breaking) redirect behavior for `-web.external-url` if `-web.route-prefix` is configured, which was introduced in #17240. #17389
* [BUGFIX] Fix federation of some native histograms. #17299 #17409
* [BUGFIX] promtool: `check config` would fail when `--lint=none` flag was set. #17399 #17414
* [BUGFIX] Remote-write: fix a deadlock in the queue resharding logic that can lead to suboptimal queue behavior. #17412
