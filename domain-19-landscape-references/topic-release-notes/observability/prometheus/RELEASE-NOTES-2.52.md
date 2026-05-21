---
title: prometheus v2.52 Release Notes
description: prometheus v2.52 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.52 Release Notes 是什么
- 如何 prometheus v2.52 Release Notes
trigger_keywords:
- prometheus
- v2.52
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

# prometheus v2.52 Release Notes

Source: [v2.52.0](https://github.com/prometheus/prometheus/releases/tag/v2.52.0)

* [CHANGE] TSDB: Fix the predicate checking for blocks which are beyond the retention period to include the ones right at the retention boundary. #9633
* [FEATURE] Kubernetes SD: Add a new metric `prometheus_sd_kubernetes_failures_total` to track failed requests to Kubernetes API. #13554
* [FEATURE] Kubernetes SD: Add node and zone metadata labels when using the endpointslice role. #13935
* [FEATURE] Azure SD/Remote Write: Allow usage of Azure authorization SDK. #13099
* [FEATURE] Alerting: Support native histogram templating. #13731
* [FEATURE] Linode SD: Support IPv6 range discovery and region filtering. #13774
* [ENHANCEMENT] PromQL: Performance improvements for queries with regex matchers. #13461
* [ENHANCEMENT] PromQL: Performance improvements when using aggregation operators. #13744
* [ENHANCEMENT] PromQL: Validate label_join destination label. #13803
* [ENHANCEMENT] Scrape: Increment `prometheus_target_scrapes_sample_duplicate_timestamp_total` metric on duplicated series during one scrape. #12933
* [ENHANCEMENT] TSDB: Many improvements in performance. #13742 #13673 #13782
* [ENHANCEMENT] TSDB: Pause regular block compactions if the head needs to be compacted (prioritize head as it increases memory consumption). #13754
* [ENHANCEMENT] Observability: Improved logging during signal handling termination. #13772
* [ENHANCEMENT] Observability: All log lines for drop series use "num_dropped" key consistently. #13823
* [ENHANCEMENT] Observability: Log chunk snapshot and mmaped chunk replay duration during WAL replay. #13838
* [ENHANCEMENT] Observability: Log if the block is being created from WBL during compaction. #13846
* [BUGFIX] PromQL: Fix inaccurate sample number statistic when querying histograms. #13667
* [BUGFIX] PromQL: Fix `histogram_stddev` and `histogram_stdvar` for cases where the histogram has negative buckets. #13852
* [BUGFIX] PromQL: Fix possible duplicated label name and values in a metric result for specific queries. #13845
* [BUGFIX] Scrape: Fix setting native histogram schema factor during scrape. #13846
* [BUGFIX] TSDB: Fix counting of histogram samples when creating WAL checkpoint stats. #13776
* [BUGFIX] TSDB: Fix cases of compacting empty heads. #13755
* [BUGFIX] TSDB: Count float histograms in WAL checkpoint. #13844
* [BUGFIX] Remote Read: Fix memory leak due to broken requests. #13777
* [BUGFIX] API: Stop building response for `/api/v1/series/` when the API request was cancelled. #13766
* [BUGFIX] promtool: Fix panic on `promtool tsdb analyze --extended` when no native histograms are present. #13976
