---
title: prometheus v0.18 Release Notes
description: prometheus v0.18 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- job
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v0.18 Release Notes 是什么
- 如何 prometheus v0.18 Release Notes
trigger_keywords:
- prometheus
- v0.18
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

# [[Prometheus|prometheus]] v0.18 Release Notes

Source: [0.18.0](https://github.com/prometheus/prometheus/releases/tag/0.18.0)

Breaking change:
Support for the old alerting rule syntax was dropped for this version as announced with the 0.17.0 release.
- [BUGFIX] Fix operator precedence in PromQL
- [BUGFIX] Never drop still open head chunk
- [BUGFIX] Fix missing 'keep_common' when printing AST node
- [CHANGE/BUGFIX] Target identity considers path and parameters additionally to host and port
- [CHANGE] Rename metric `prometheus_local_storage_invalid_preload_requests_total` to `prometheus_local_storage_non_existent_series_matches_total`
- [CHANGE] Support for old alerting rule syntax dropped
- [FEATURE] Deduplicate targets within the same scrape job
- [FEATURE] Add varbit chunk encoding (higher compression, more CPU usage – disabled by default)
- [FEATURE] Add `holt_winters` query function
- [FEATURE] Add relative complement `unless` operator to PromQL
- [ENHANCEMENT] Quarantine series file if data corruption is encountered (instead of crashing)
- [ENHANCEMENT] Validate Alertmanager URL
- [ENHANCEMENT] Use UTC for build timestamp
- [ENHANCEMENT] Improve index query performance (especially for active time series)
- [ENHANCEMENT] Instrument configuration reload duration
- [ENHANCEMENT] Instrument retrieval layer
- [ENHANCEMENT] Add Go version to `prometheus_build_info` metric
