---
title: prometheus v2.54 Release Notes
description: prometheus v2.54 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.54 Release Notes 是什么
- 如何 prometheus v2.54 Release Notes
trigger_keywords:
- prometheus
- v2.54
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
created: "2026-05-23"
---

# [[Prometheus|prometheus]] v2.54 Release Notes

Source: [v2.54.1](https://github.com/prometheus/prometheus/releases/tag/v2.54.1)

* [BUGFIX] Scraping: allow multiple samples on same series, with explicit timestamps. #14685
* [BUGFIX] Docker SD: fix crash in `match_first_network` mode when container is reconnected to a new network. #14654
* [BUGFIX] PromQL: fix experimental native histogram counter reset detection on stale samples. #14514
* [BUGFIX] PromQL: fix experimental native histograms getting corrupted due to vector selector bug in range queries. #14538
* [BUGFIX] PromQL: fix experimental native histogram memory corruption when using histogram_count or histogram_sum. #14605

**Full Changelog**: https://github.com/prometheus/prometheus/compare/v2.54.0...v2.54.1