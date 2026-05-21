---
title: prometheus v2.16 Release Notes
description: prometheus v2.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- grafana
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.16 Release Notes 是什么
- 如何 prometheus v2.16 Release Notes
trigger_keywords:
- prometheus
- v2.16
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
---

# prometheus v2.16 Release Notes

Source: [v2.16.0](https://github.com/prometheus/prometheus/releases/tag/v2.16.0)

* [FEATURE] React UI: Support local timezone on /graph #6692
* [FEATURE] PromQL: add absent_over_time query function #6490
* [FEATURE] Adding optional logging of queries to their own file #6520
* [ENHANCEMENT] React UI: Add support for rules page and "Xs ago" duration displays #6503
* [ENHANCEMENT] React UI: alerts page, replace filtering togglers tabs with checkboxes #6543
* [ENHANCEMENT] TSDB: Export metric for WAL write errors #6647
* [ENHANCEMENT] TSDB: Improve query performance for queries that only touch the most recent 2h of data. #6651
* [ENHANCEMENT] PromQL: Refactoring in parser errors to improve error messages #6634
* [ENHANCEMENT] PromQL: Support trailing commas in grouping opts #6480
* [ENHANCEMENT] Scrape: Reduce memory usage on reloads by reusing scrape cache #6670
* [ENHANCEMENT] Scrape: Add metrics to track bytes and entries in the metadata cache #6675
* [ENHANCEMENT] promtool: Add support for line-column numbers for invalid rules output #6533
* [ENHANCEMENT] Avoid restarting rule groups when it is unnecessary #6450
* [BUGFIX] React UI: Send cookies on fetch() on older browsers #6553
* [BUGFIX] React UI: adopt grafana flot fix for stacked graphs #6603
* [BUFGIX] React UI: broken graph page browser history so that back button works as expected #6659
* [BUGFIX] TSDB: ensure compactionsSkipped metric is registered, and log proper error if one is returned from head.Init #6616
* [BUGFIX] TSDB: return an error on ingesting series with duplicate labels #6664
* [BUGFIX] PromQL: Fix unary operator precedence #6579
* [BUGFIX] PromQL: Respect query.timeout even when we reach query.max-concurrency #6712
* [BUGFIX] PromQL: Fix string and parentheses handling in engine, which affected React UI #6612
* [BUGFIX] PromQL: Remove output labels returned by absent() if they are produced by multiple identical label matchers #6493
* [BUGFIX] Scrape: Validate that OpenMetrics input ends with `# EOF` #6505
* [BUGFIX] Remote read: return the correct error if configs can't be marshal'd to JSON #6622
* [BUGFIX] Remote write: Make remote client `Store` use passed context, which can affect shutdown timing #6673
* [BUGFIX] Remote write: Improve sharding calculation in cases where we would always be consistently behind by tracking pendingSamples #6511
* [BUGFIX] Ensure prometheus_rule_group metrics are deleted when a rule group is removed #6693
