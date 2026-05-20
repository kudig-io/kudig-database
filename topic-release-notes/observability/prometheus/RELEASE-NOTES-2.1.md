---
title: prometheus v2.1 Release Notes
description: prometheus v2.1 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.1 Release Notes 是什么
- 如何 prometheus v2.1 Release Notes
trigger_keywords:
- prometheus
- v2.1
- Release
- Notes
- release
- notes
---

# prometheus v2.1 Release Notes

Source: [v2.1.0](https://github.com/prometheus/prometheus/releases/tag/v2.1.0)

This release includes multiple bugfixes and features. Also the version on the index has been bumped so the storage is not backward compatible. Prometheus 2.0 storage will work on 2.1 but not vice-versa.

* [FEATURE] New Service Discovery UI showing labels before and after relabelling.
* [FEATURE] New Admin APIs added to v1 to delete, snapshot and remove tombstones.
* [ENHANCEMENT] The graph UI autcomplete now includes your previous queries.
* [ENHANCEMENT] Federation is now much faster for large numbers of series.
* [ENHANCEMENT] Added new metrics to measure rule timings.
* [ENHANCEMENT] Rule evaluation times added to the rules UI.
* [ENHANCEMENT] Added metrics to measure modified time of file SD files.
* [ENHANCEMENT] Kubernetes SD now includes POD UID in discovery metadata.
* [ENHANCEMENT] The Query APIs now return optional stats on query execution times.
* [ENHANCEMENT] The index now no longer has the 4GiB size limit and is also smaller.
* [BUGFIX] Remote read `read_recent` option is now false by default.
* [BUGFIX] Pass the right configuration to each Alertmanager (AM) when using multiple AM configs.
* [BUGFIX] Fix not-matchers not selecting series with labels unset.
* [BUGFIX] tsdb: Fix occasional panic in head block.
* [BUGFIX] tsdb: Close files before deletion to fix retention issues on Windows and NFS.
* [BUGFIX] tsdb: Cleanup and do not retry failing compactions.
* [BUGFIX] tsdb: Close WAL while shutting down.
