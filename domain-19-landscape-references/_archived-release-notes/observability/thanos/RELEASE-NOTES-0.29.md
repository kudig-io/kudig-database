---
title: thanos v0.29 Release Notes
description: thanos v0.29 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.29 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- jaeger
- minio
- redis
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- thanos v0.29 Release Notes 是什么
- 如何 thanos v0.29 Release Notes
trigger_keywords:
- thanos
- v0.29
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- redis-basics
- tracing-basics
- observability-basics
---



# [[Thanos|thanos]] v0.29 Release Notes

Source: [v0.29.0](https://github.com/thanos-io/thanos/releases/tag/v0.29.0)

`v0.29.0` is out after *69* days of work since `v0.28.0`! Thank you to all 35 contributors who have contributed to this release. It wouldn't be the same without you. `v0.29.0` has no changes since the release candidate.

Some of the highlights include [[OpenTelemetry|OpenTelemetry]] support, Azure support has been improved with a new SDK, increased query speed, receive has new features to limit series per tenant.

First, let's celebrate new contributors, and then you can find the changelog where you can find all of the details. Please try it out and let us know if you spot any problems!

## New Contributors

* @nikitapecasa made their first contribution in https://github.com/thanos-io/thanos/pull/5448
* @shenxn made their first contribution in https://github.com/thanos-io/thanos/pull/5455
* @SrushtiSapkale made their first contribution in https://github.com/thanos-io/thanos/pull/5447
* @chris-ng-scmp made their first contribution in https://github.com/thanos-io/thanos/pull/5466
* @olasd made their first contribution in https://github.com/thanos-io/thanos/pull/5477
* @BouchaaraAdil made their first contribution in https://github.com/thanos-io/thanos/pull/5465
* @eharcevs made their first contribution in https://github.com/thanos-io/thanos/pull/5453
* @bishal7679 made their first contribution in https://github.com/thanos-io/thanos/pull/5486
* @naveensrinivasan made their first contribution in https://github.com/thanos-io/thanos/pull/5364
* @Firxiao made their first contribution in https://github.com/thanos-io/thanos/pull/5496
* @Akshit42-hue made their first contribution in https://github.com/thanos-io/thanos/pull/5529
* @audig made their first contribution in https://github.com/thanos-io/thanos/pull/5534
* @Juneezee made their first contribution in https://github.com/thanos-io/thanos/pull/5574
* @raptorsun made their first contribution in https://github.com/thanos-io/thanos/pull/5439
* @oronsh made their first contribution in https://github.com/thanos-io/thanos/pull/5596
* @jzelinskie made their first contribution in https://github.com/thanos-io/thanos/pull/5611
* @tusharxoxoxo made their first contribution in https://github.com/thanos-io/thanos/pull/5620
* @zvlb made their first contribution in https://github.com/thanos-io/thanos/pull/5573
* @padhiar-aditya made their first contribution in https://github.com/thanos-io/thanos/pull/5670
* @pedro-stanaka made their first contribution in https://github.com/thanos-io/thanos/pull/5666
* @prajain12 made their first contribution in https://github.com/thanos-io/thanos/pull/5678
* @xdavidwu made their first contribution in https://github.com/thanos-io/thanos/pull/5656
* @Abirdcfly made their first contribution in https://github.com/thanos-io/thanos/pull/5660
* @sdufel made their first contribution in https://github.com/thanos-io/thanos/pull/5684
* @mtlang made their first contribution in https://github.com/thanos-io/thanos/pull/5690
* @vhbfernandes made their first contribution in https://github.com/thanos-io/thanos/pull/5696
* @davinci26 made their first contribution in https://github.com/thanos-io/thanos/pull/5702
* @haanhvu made their first contribution in https://github.com/thanos-io/thanos/pull/5641
* @wanjunlei made their first contribution in https://github.com/thanos-io/thanos/pull/5723
* @dbut023 made their first contribution in https://github.com/thanos-io/thanos/pull/5674
* @utukJ made their first contribution in https://github.com/thanos-io/thanos/pull/5738
* @isantospardo made their first contribution in https://github.com/thanos-io/thanos/pull/5744
* @amincheloh made their first contribution in https://github.com/thanos-io/thanos/pull/5769
* @Rahulkumar2002 made their first contribution in https://github.com/thanos-io/thanos/pull/5749
* @aarontams made their first contribution in https://github.com/thanos-io/thanos/pull/5778

## Fixed
- [#5642](https://github.com/thanos-io/thanos/pull/5642) Receive: Log labels correctly in writer debug messages.
- [#5655](https://github.com/thanos-io/thanos/pull/5655) Receive: Fix recreating already pruned tenants.
- [#5702](https://github.com/thanos-io/thanos/pull/5702) Store: Upgrade minio-go/v7 to fix panic caused by leaked goroutines.
- [#5736](https://github.com/thanos-io/thanos/pull/5736) Compact: Fix crash in GatherNoCompactionMarkFilter.NoCompactMarkedBlocks.
- [#5763](https://github.com/thanos-io/thanos/pull/5763) Compact: Enable metadata cache.
- [#5759](https://github.com/thanos-io/thanos/pull/5759) Compact: Fix missing duration log key.
- [#5799](https://github.com/thanos-io/thanos/pull/5799) Query Frontend: Fixed sharding behaviour for vector matches. Now queries with sharding should work properly where the query looks like: `foo and without (lbl) bar`.

## Added
- [#5565](https://github.com/thanos-io/thanos/pull/5565) Receive: Allow remote write request limits to be defined per file and tenant (experimental).
* [#5654](https://github.com/thanos-io/thanos/pull/5654) Query: add `--grpc-compression` flag that controls the compression used in gRPC client. With the flag it is now possible to compress the traffic between Query and StoreAPI nodes - you get lower network usage in exchange for a bit higher CPU/RAM usage.
- [#5650](https://github.com/thanos-io/thanos/pull/5650) Query Frontend: Add sharded queries metrics. `thanos_frontend_sharding_middleware_queries_total` shows how many queries were sharded or not sharded.
- [#5658](https://github.com/thanos-io/thanos/pull/5658) Query Frontend: Introduce new optional parameters (`query-range.min-split-interval`, `query-range.max-split-interval`, `query-range.horizontal-shards`) to implement more dynamic horizontal query splitting.
- [#5721](https://github.com/thanos-io/thanos/pull/5721) Store: Add metric `thanos_bucket_store_empty_postings_total` for number of empty postings when fetching series.
- [#5723](https://github.com/thanos-io/thanos/pull/5723) Compactor: Support disable block viewer UI.
- [#5674](https://github.com/thanos-io/thanos/pull/5674) Query Frontend/Store: Add support connecting to redis using TLS.
- [#5734](https://github.com/thanos-io/thanos/pull/5734) Store: Support disable block viewer UI.
- [#5411](https://github.com/thanos-io/thanos/pull/5411) Tracing: Add OpenTelemetry Protocol exporter.
- [#5779](https://github.com/thanos-io/thanos/pull/5779) Objstore: Support specifying S3 storage class.
- [#5741](https://github.com/thanos-io/thanos/pull/5741) Query: add metrics on how much data is being selected by downstream Store APIs.
- [#5673](https://github.com/thanos-io/thanos/pull/5673) Receive: Reload tenant limit configuration on file change.
- [#5749](https://github.com/thanos-io/thanos/pull/5749) Query Frontend: Added small LRU cache to cache query analysis results.

## Changed

- [#5738](https://github.com/thanos-io/thanos/pull/5738) Global: replace `crypto/sha256` with `minio/sha256-simd` to make hash calculation faster in metadata and reloader packages.
- [#5648](https://github.com/thanos-io/thanos/pull/5648) Query Frontend: cache vertical shards in query-frontend.
- [#5753](https://github.com/thanos-io/thanos/pull/5753) Build with Go 1.19.
- [#5255](https://github.com/thanos-io/thanos/pull/5296) Query: Use k-way merging for the proxying logic. The proxying sub-system now uses much less resources (~25-80% less CPU usage, ~30-50% less RAM usage according to our benchmarks). Reduces query duration by a few percent on queries with lots of series.
- [#5690](https://github.com/thanos-io/thanos/pull/5690) Compact: update `--debug.accept-malformed-index` flag to apply to downsampling. Previously the flag only applied to compaction, and fatal errors would still occur when downsampling was attempted.
- [#5707](https://github.com/thanos-io/thanos/pull/5707) Objstore: Update objstore to latest version which includes a refactored Azure Storage Account implementation with a new SDK.
- [#5641](https://github.com/thanos-io/thanos/pull/5641) Store: Remove hardcoded labels in shard matcher.
- [#5641](https://github.com/thanos-io/thanos/pull/5641) Query: Inject unshardable le label in query analyzer.
- [#5685](https://github.com/thanos-io/thanos/pull/5685) Receive: Make active/head series limiting configuration per tenant by adding it to new limiting config.
- [#5411](https://github.com/thanos-io/thanos/pull/5411) Tracing: Change Jaeger exporter from OpenTracing to OpenTelemetry. *Options `RPC Metrics`, `Gen128Bit` and `Disabled` are now deprecated and won't have any effect when set :warning:.*
- [#5767](https://github.com/thanos-io/thanos/pull/5767) *: Upgrade Prometheus to v2.39.0.
- [#5771](https://github.com/thanos-io/thanos/pull/5771) *: Upgrade Prometheus to v2.39.1.


**Full Changelog**: https://github.com/thanos-io/thanos/compare/v0.28.1...v0.29.0