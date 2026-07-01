---
title: prometheus v2.34 Release Notes
description: prometheus v2.34 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v2.34 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- jaeger
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.34 Release Notes 是什么
- 如何 prometheus v2.34 Release Notes
trigger_keywords:
- prometheus
- v2.34
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- tracing-basics
- observability-basics
---



# [[Prometheus|prometheus]] v2.34 Release Notes

Source: [v2.34.0](https://github.com/prometheus/prometheus/releases/tag/v2.34.0)

* [CHANGE] UI: Classic UI removed. #10208
* [CHANGE] Tracing: Migrate from [[Jaeger|Jaeger]] to [[OpenTelemetry|OpenTelemetry]] based tracing. #9724, #10203, #10276
* [ENHANCEMENT] TSDB: Disable the chunk write queue by default and allow configuration with the experimental flag `--storage.tsdb.head-chunks-write-queue-size`. #10425
* [ENHANCEMENT] HTTP SD: Add a failure counter. #10372
* [ENHANCEMENT] Azure SD: Set Prometheus User-Agent on requests. #10209
* [ENHANCEMENT] Uyuni SD: Reduce the number of logins to Uyuni. #10072
* [ENHANCEMENT] Scrape: Log when an invalid media type is encountered during a scrape. #10186
* [ENHANCEMENT] Scrape: Accept application/openmetrics-text;version=1.0.0 in addition to version=0.0.1. #9431
* [ENHANCEMENT] Remote-read: Add an option to not use external labels as selectors for remote read. #10254
* [ENHANCEMENT] UI: Optimize the alerts page and add a search bar. #10142
* [ENHANCEMENT] UI: Improve graph colors that were hard to see. #10179
* [ENHANCEMENT] Config: Allow escaping of `$` with `$$` when using environment variables with external labels. #10129
* [BUGFIX] PromQL: Properly return an error from histogram_quantile when metrics have the same labelset. #10140 
* [BUGFIX] UI: Fix bug that sets the range input to the resolution. #10227
* [BUGFIX] TSDB: Fix a query panic when `memory-snapshot-on-shutdown` is enabled. #10348
* [BUGFIX] Parser: Specify type in metadata parser errors. #10269
* [BUGFIX] Scrape: Fix label limit changes not applying. #10370
