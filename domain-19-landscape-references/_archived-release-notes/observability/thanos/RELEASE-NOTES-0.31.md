---
title: thanos v0.31 Release Notes
description: thanos v0.31 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- redis
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.31 Release Notes 是什么
- 如何 thanos v0.31 Release Notes
trigger_keywords:
- thanos
- v0.31
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- redis-basics
created: "2026-05-23"
---

# [[Thanos|thanos]] v0.31 Release Notes

Source: [v0.31.0](https://github.com/thanos-io/thanos/releases/tag/v0.31.0)

## What's Changed

### Added

- [#5990](https://github.com/thanos-io/thanos/pull/5990) Cache/Redis: add support for Redis Sentinel via new option `master_name`.
- [#6008](https://github.com/thanos-io/thanos/pull/6008) *: Add counter metric `gate_queries_total` to gate.
- [#5926](https://github.com/thanos-io/thanos/pull/5926) Receiver: Add experimental string interning in writer. Can be enabled with a hidden flag `--writer.intern`.
- [#5773](https://github.com/thanos-io/thanos/pull/5773) Store: Support disabling cache index header file by setting `--disable-caching-index-header-file`. When toggled, Stores can run without needing persistent disks.
- [#5653](https://github.com/thanos-io/thanos/pull/5653) Receive: Allow setting hashing algorithm per tenant in hashrings config.
- [#6074](https://github.com/thanos-io/thanos/pull/6074) *: Add histogram metrics `thanos_store_server_series_requested` and `thanos_store_server_chunks_requested` to all Stores.
- [#6074](https://github.com/thanos-io/thanos/pull/6074) *: Allow configuring series and sample limits per `Series` request for all Stores.
- [#6104](https://github.com/thanos-io/thanos/pull/6104) Store: Support S3 session token.
- [#5548](https://github.com/thanos-io/thanos/pull/5548) Query: Add experimental support for load balancing across multiple Store endpoints.
- [#6148](https://github.com/thanos-io/thanos/pull/6148) Query-frontend: Add `traceID` to slow query detected log line.
- [#6153](https://github.com/thanos-io/thanos/pull/6153) Query-frontend: Add `remote_user` (from http basic auth) and `remote_addr` to slow query detected log line.

### Fixed

- [#5995](https://github.com/thanos-io/thanos/pull/5995) Sidecar: Loads TLS certificate during startup.
- [#6044](https://github.com/thanos-io/thanos/pull/6044) Receive: Mark out-of-window errors as conflict when out-of-window samples ingestion is used.
- [#6050](https://github.com/thanos-io/thanos/pull/6050) Store: Re-try bucket store initial sync upon failure.
- [#6067](https://github.com/thanos-io/thanos/pull/6067) Receive: Fix panic when querying uninitialized TSDBs.
- [#6082](https://github.com/thanos-io/thanos/pull/6082) Query: Don't error when no stores are matched.
- [#6098](https://github.com/thanos-io/thanos/pull/6098) Cache/Redis: Upgrade `rueidis` to v0.0.93 to fix potential panic when the client-side caching is disabled.
- [#6103](https://github.com/thanos-io/thanos/pull/6103) Mixins(Rule): Fix expression for long rule evaluations.
- [#6121](https://github.com/thanos-io/thanos/pull/6121) Receive: Deduplicate meta-monitoring queries for [Active Series Limiting](https://thanos.io/tip/components/receive.md/#active-series-limiting-experimental).
- [#6137](https://github.com/thanos-io/thanos/pull/6137) Downsample: Repair of non-empty XOR chunks during 1h downsampling.
- [#6125](https://github.com/thanos-io/thanos/pull/6125) Query Frontend: Fix vertical shardable instant queries do not produce sorted results for `sort`, `sort_desc`, `topk` and `bottomk` functions.
- [#6203](https://github.com/thanos-io/thanos/pull/6203) Receive: Fix panic in head compaction under high query load.

### Changed

- [#6010](https://github.com/thanos-io/thanos/pull/6010) *: Upgrade [[Prometheus|Prometheus]] to v0.42.0.
- [#5999](https://github.com/thanos-io/thanos/pull/5999) *: Upgrade Alertmanager dependency to v0.25.0.
- [#5887](https://github.com/thanos-io/thanos/pull/5887) Tracing: Make sure rate limiting sampler is the default, as was the case in version pre-0.29.0.
- [#5997](https://github.com/thanos-io/thanos/pull/5997) Rule: switch to miekgdns DNS resolver as the default one.
- [#6035](https://github.com/thanos-io/thanos/pull/6035) Tools (replicate): Support all types of matchers to match blocks for replication. Change matcher parameter from string slice to a single string.
- [#6131](https://github.com/thanos-io/thanos/pull/6131) Store: *breaking :warning:* Use Histograms instead of Summaries for bucket metrics.
