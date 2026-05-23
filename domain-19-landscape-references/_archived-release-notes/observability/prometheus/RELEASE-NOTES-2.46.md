---
title: prometheus v2.46 Release Notes
description: prometheus v2.46 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.46 Release Notes 是什么
- 如何 prometheus v2.46 Release Notes
trigger_keywords:
- prometheus
- v2.46
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

# [[Prometheus|prometheus]] v2.46 Release Notes

Source: [v2.46.0](https://github.com/prometheus/prometheus/releases/tag/v2.46.0)

* [FEATURE] Promtool: Add PromQL format and label matcher set/delete commands to promtool. #11411
* [FEATURE] Promtool: Add push metrics command. #12299
* [ENHANCEMENT] Promtool: Read from stdin if no filenames are provided in check rules. #12225
* [ENHANCEMENT] Hetzner SD: Support larger ID's that will be used by Hetzner in September. #12569
* [ENHANCEMENT] [[Kubernetes|Kubernetes]] SD: Add more labels for endpointslice and endpoints role. #10914
* [ENHANCEMENT] Kubernetes SD: Do not add [[Pods|pods]] to target group if the PodIP status is not set. #11642
* [ENHANCEMENT] OpenStack SD: Include instance image ID in labels. #12502
* [ENHANCEMENT] Remote Write receiver: Validate the metric names and labels. #11688
* [ENHANCEMENT] Web: Initialize `prometheus_http_requests_total` metrics with `code` label set to `200`. #12472
* [ENHANCEMENT] TSDB: Add Zstandard compression option for wlog. #11666
* [ENHANCEMENT] TSDB: Support native histograms in snapshot on shutdown. #12258
* [ENHANCEMENT] Labels: Avoid compiling regexes that are literal. #12434
* [BUGFIX] Histograms: Fix parsing of float histograms without zero bucket. #12577
* [BUGFIX] Histograms: Fix scraping native and classic histograms missing some histograms. #12554
* [BUGFIX] Histograms: Enable ingestion of multiple exemplars per sample. 12557
* [BUGFIX] File SD: Fix path handling in File-SD watcher to allow directory monitoring on Windows. #12488
* [BUGFIX] Linode SD: Cast `InstanceSpec` values to `int64` to avoid overflows on 386 architecture. #12568
* [BUGFIX] PromQL Engine: Include query parsing in active-query tracking. #12418
* [BUGFIX] TSDB: Handle TOC parsing failures. #10623
