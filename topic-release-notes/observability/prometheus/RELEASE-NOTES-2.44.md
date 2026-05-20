---
title: prometheus v2.44 Release Notes
description: prometheus v2.44 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.44 Release Notes 是什么
- 如何 prometheus v2.44 Release Notes
trigger_keywords:
- prometheus
- v2.44
- Release
- Notes
- release
- notes
---

# prometheus v2.44 Release Notes

Source: [v2.44.0](https://github.com/prometheus/prometheus/releases/tag/v2.44.0)

This version is built with Go tag `stringlabels`, to use the smaller data
structure for Labels that was optional in the previous release. For more
details about this code change see #10991.

* [CHANGE] Remote-write: Raise default samples per send to 2,000. #12203
* [FEATURE] Remote-read: Handle native histograms. #12085, #12192
* [FEATURE] Promtool: Health and readiness check of prometheus server in CLI. #12096
* [FEATURE] PromQL: Add `query_samples_total` metric, the total number of samples loaded by all queries. #12251
* [ENHANCEMENT] Storage: Optimise buffer used to iterate through samples. #12326
* [ENHANCEMENT] Scrape: Reduce memory allocations on target labels. #12084
* [ENHANCEMENT] PromQL: Use faster heap method for `topk()` / `bottomk()`. #12190
* [ENHANCEMENT] Rules API: Allow filtering by rule name. #12270
* [ENHANCEMENT] Native Histograms: Various fixes and improvements. #11687, #12264, #12272
* [ENHANCEMENT] UI: Search of scraping pools is now case-insensitive. #12207
* [ENHANCEMENT] TSDB: Add an affirmative log message for successful WAL repair. #12135
* [BUGFIX] TSDB: Block compaction failed when shutting down. #12179
* [BUGFIX] TSDB: Out-of-order chunks could be ignored if the write-behind log was deleted. #12127

Images are available on Docker Hub: `docker pull prom/prometheus:v2.44.0`