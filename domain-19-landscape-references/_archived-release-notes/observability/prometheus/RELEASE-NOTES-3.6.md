---
title: prometheus v3.6 Release Notes
description: prometheus v3.6 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v3.6 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v3.6 Release Notes 是什么
- 如何 prometheus v3.6 Release Notes
trigger_keywords:
- prometheus
- v3.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---



# [[Prometheus|prometheus]] v3.6 Release Notes

Source: [v3.6.0](https://github.com/prometheus/prometheus/releases/tag/v3.6.0)

* [FEATURE] PromQL: Add `step()`, and `min()` and `max()` on durations, behind feature flag `promql-duration-expr`. #16777
* [FEATURE] API: Add a `/v1/status/tsdb/blocks` endpoint exposing metadata about loaded blocks. #16695
* [FEATURE] Templates: Add `toDuration()` and `now()` functions. #16619
* [ENHANCEMENT] Discovery: Add support for attaching namespace metadata to targets. #16831
* [ENHANCEMENT] OTLP: Support new `UnderscoreEscapingWithoutSuffixes` strategy via `otlp.translation_strategy`. #16849
* [ENHANCEMENT] OTLP: Support including scope metadata as metric labels via `otlp.promote_scope_metadata`. #16878
* [ENHANCEMENT] OTLP: Add `__type__` and `__unit__` labels when feature flag `type-and-unit-labels` is enabled. #16630
* [ENHANCEMENT] Tracing: Send the traceparent HTTP header during scrapes. #16425
* [ENHANCEMENT] UI: Add option to disable info and warning query messages under `Query page settings`. #16901
* [ENHANCEMENT] UI: Improve metadata handling for `_count/_sum/_bucket` suffixes. #16910
* [ENHANCEMENT] TSDB: Track stale series in the Head block via the `prometheus_tsdb_head_stale_series` metric. #16925
* [PERF] PromQL: Improve performance due to internal optimizations. #16797
* [BUGFIX] Config: Fix "unknown global name escaping method" error messages produced during config validation. #16801
* [BUGFIX] Discovery: Fix race condition during shutdown. #16820
* [BUGFIX] OTLP: Generate `target_info` samples between the earliest and latest samples per resource. #16737
* [BUGFIX] PromQL: Fail when `NaN` is passed as parameter to `topk()`, `bottomk()`, `limitk()` and `limit_ratio()`. #16725
* [BUGFIX] PromQL: Fix extrapolation for native counter histograms. #16828
* [BUGFIX] PromQL: Reduce numerical errors by disabling some optimizations. #16895
* [BUGFIX] PromQL: Fix inconsistencies when using native histograms in subqueries. #16879
* [BUGFIX] PromQL: Fix inconsistent annotations for `rate()` and `increase()` on histograms when feature flag `type-and-unit-labels` is enabled. #16915
* [BUGFIX] Scraping: Fix memory corruption in `slicelabels` builds. #16946
* [BUGFIX] TSDB: Fix panic on append when feature flag `created-timestamp-zero-ingestion` is enabled. #16332
* [BUGFIX] TSDB: Fix panic on append for native histograms with empty buckets. #16893
