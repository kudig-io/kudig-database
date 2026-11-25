---
title: prometheus v3.1 Release Notes
description: prometheus v3.1 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v3.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v3.1 Release Notes 是什么
- 如何 prometheus v3.1 Release Notes
trigger_keywords:
- prometheus
- v3.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Prometheus|prometheus]] v3.1 Release Notes

Source: [v3.1.0](https://github.com/prometheus/prometheus/releases/tag/v3.1.0)

## What's Changed

 * [SECURITY] upgrade golang.org/x/crypto to address reported CVE-2024-45337. #15691
 * [CHANGE] Notifier: Increment prometheus_notifications_errors_total by the number of affected alerts rather than per batch. #15428
 * [CHANGE] API: list rules field "groupNextToken:omitempty" renamed to "groupNextToken". #15400
 * [ENHANCEMENT] OTLP translate: keep identifying attributes in target_info. #15448
 * [ENHANCEMENT] Paginate rule groups, add infinite scroll to rules within groups. #15677
 * [ENHANCEMENT] TSDB: Improve calculation of space used by labels. #13880
 * [ENHANCEMENT] Rules: new metric rule_group_last_rule_duration_sum_seconds. #15672
 * [ENHANCEMENT] Observability: Export 'go_sync_mutex_wait_total_seconds_total' metric. #15339
 * [ENHANCEMEN] Remote-Write: optionally use a DNS resolver that picks a random IP. #15329
 * [PERF] Optimize `l=~".+"` matcher. #15474, #15684
 * [PERF] TSDB: Cache all symbols for compaction . #15455
 * [PERF] TSDB: MemPostings: keep a map of label values slices. #15426
 * [PERF] Remote-Write: Remove interning hook. #15456
 * [PERF] Scrape: optimize string manipulation for experimental native histograms with custom buckets. #15453
 * [PERF] TSDB: reduce memory allocations. #15465, #15427
 * [PERF] Storage: Implement limit in mergeGenericQuerier. #14489
 * [PERF] TSDB: Optimize inverse matching. #14144
 * [PERF] Regex: use stack memory for lowercase copy of string. #15210
 * [PERF] TSDB: When deleting from postings index, pause to unlock and let readers read. #15242
 * [BUGFIX] Main: Avoid possible segfault at exit. (#15724)
 * [BUGFIX] Rules: Do not run rules concurrently if uncertain about dependencies. #15560
 * [BUGFIX] PromQL: Adds test for `absent`, `absent_over_time` and `deriv` func with histograms. #15667
 * [BUGFIX] PromQL: Fix various bugs related to quoting UTF-8 characters. #15531
 * [BUGFIX] Scrape: fix nil panic after scrape loop reload. #15563
 * [BUGFIX] Remote-write: fix panic on repeated log message. #15562
 * [BUGFIX] Scrape: reload would ignore always_scrape_classic_histograms and convert_classic_histograms_to_nhcb configs. #15489
 * [BUGFIX] TSDB: fix data corruption in experimental native histograms. #15482
 * [BUGFIX] PromQL: Ignore histograms in all time related functions. #15479
 * [BUGFIX] OTLP receiver: Convert metric metadata. #15416
 * [BUGFIX] PromQL: Fix `resets` function for histograms. #15527
 * [BUGFIX] PromQL: Fix behaviour of `changes()` for mix of histograms and floats. #15469
 * [BUGFIX] PromQL: Fix behaviour of some aggregations with histograms. #15432
 * [BUGFIX] allow quoted exemplar keys in openmetrics text format. #15260
 * [BUGFIX] TSDB: fixes for rare conditions when loading write-behind-log (WBL). #15380
 * [BUGFIX] `round()` function did not remove `__name__` label. #15250
 * [BUGFIX] Promtool: analyze block shows metric name with 0 cardinality. #15438
 * [BUGFIX] PromQL: Fix `count_values` for histograms. #15422
 * [BUGFIX] PromQL: fix issues with comparison binary operations with `bool` modifier and native histograms. #15413
 * [BUGFIX] PromQL: fix incorrect "native histogram ignored in aggregation" annotations. #15414
 * [BUGFIX] PromQL: Corrects the behaviour of some operator and aggregators with Native Histograms. #15245
 * [BUGFIX] TSDB: Always return unknown hint for first sample in non-gauge histogram chunk. #15343
 * [BUGFIX] PromQL: Clamp functions: Ignore any points with native histograms. #15169
 * [BUGFIX] TSDB: Fix race on stale values in headAppender. #15322
 * [BUGFIX] UI: Fix selector / series formatting for empty metric names. #15340
 * [BUGFIX] OTLP receiver: Allow colons in non-standard units. #15710


<!-- risk-assessed -->
