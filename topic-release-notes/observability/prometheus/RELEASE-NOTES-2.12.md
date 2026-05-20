---
title: prometheus v2.12 Release Notes
description: prometheus v2.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.12 Release Notes 是什么
- 如何 prometheus v2.12 Release Notes
trigger_keywords:
- prometheus
- v2.12
- Release
- Notes
- release
- notes
---

# prometheus v2.12 Release Notes

Source: [v2.12.0](https://github.com/prometheus/prometheus/releases/tag/v2.12.0)

* [FEATURE] Track currently active PromQL queries in a log file. #5794
* [FEATURE] Enable and provide binaries for `mips64` / `mips64le` architectures. #5792
* [ENHANCEMENT] Improve responsiveness of targets web UI and API endpoint. #5740
* [ENHANCEMENT] Improve remote write desired shards calculation. #5763
* [ENHANCEMENT] Flush TSDB pages more precisely. tsdb#660
* [ENHANCEMENT] Add `prometheus_tsdb_retention_limit_bytes` metric. tsdb#667
* [ENHANCEMENT] Add logging during TSDB WAL replay on startup. tsdb#662
* [ENHANCEMENT] Improve TSDB memory usage. tsdb#653, tsdb#643, tsdb#654, tsdb#642, tsdb#627
* [BUGFIX] Check for duplicate label names in remote read. #5829
* [BUGFIX] Mark deleted rules' series as stale on next evaluation. #5759
* [BUGFIX] Fix JavaScript error when showing warning about out-of-sync server time. #5833
* [BUGFIX] Fix `promtool test rules` panic when providing empty `exp_labels`. #5774
* [BUGFIX] Only check last directory when discovering checkpoint number. #5756
* [BUGFIX] Fix error propagation in WAL watcher helper functions. #5741
* [BUGFIX] Correctly handle empty labels from alert templates. #5845
