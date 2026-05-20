---
title: thanos v0.15 Release Notes
description: thanos v0.15 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- minio
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.15 Release Notes 是什么
- 如何 thanos v0.15 Release Notes
trigger_keywords:
- thanos
- v0.15
- Release
- Notes
- release
- notes
---

# thanos v0.15 Release Notes

Source: [v0.15.0](https://github.com/thanos-io/thanos/releases/tag/v0.15.0)

# Highlights:
* Added new Thanos component: [Query Frontend](https://thanos.io/v0.15/components/query-frontend/) responsible for response caching,
query scheduling and parallelization (based on Cortex Query Frontend).
* Added various new, improved UIs to Thanos based on React: Querier' BuildInfo & Flags, Ruler UI, BlockViewer.
* Optimized Sidecar, Store, Receive, Ruler data retrieval with new TSDB ChunkIterator, capping chunks to 120 samples, fixed various leaks.
* Fixed sample limit on Store Gateway.
* Added S3 Server Side Encryption options.
* Tons of other important fixes!

Thanks to all contributors! 🤗

### Fixed

- [#2665](https://github.com/thanos-io/thanos/pull/2665) Swift: Fix issue with missing Content-Type HTTP headers.
- [#2800](https://github.com/thanos-io/thanos/pull/2800) Query: Fix handling of `--web.external-prefix` and `--web.route-prefix`.
- [#2834](https://github.com/thanos-io/thanos/pull/2834) Query: Fix rendered JSON state value for rules and alerts should be in lowercase.
- [#2866](https://github.com/thanos-io/thanos/pull/2866) Receive, Querier: Fixed leaks on receive and querier Store API Series, which were leaking on errors.
- [#2937](https://github.com/thanos-io/thanos/pull/2937) Receive: Fixing auto-configuration of `--receive.local-endpoint`.
- [#2895](https://github.com/thanos-io/thanos/pull/2895) Compact: Fix increment of `thanos_compact_downsample_total` metric for downsample of 5m resolution blocks.
- [#2858](https://github.com/thanos-io/thanos/pull/2858) Store: Fix `--store.grpc.series-sample-limit` implementation. The limit is now applied to the sum of all samples fetched across all queried blocks via a single Series call, instead of applying it individually to each block.
- [#2936](https://github.com/thanos-io/thanos/pull/2936) Compact: Fix ReplicaLabelRemover panic when replicaLabels are not specified.
- [#2956](https://github.com/thanos-io/thanos/pull/2956) Store: Fix fetching of chunks bigger than 16000 bytes.
- [#2970](https://github.com/thanos-io/thanos/pull/2970) Store: Upgrade minio-go/v7 to fix slowness when running on EKS.
- [#2957](https://github.com/thanos-io/thanos/pull/2957) Rule: *breaking :warning:* Now sets all of the relevant fields properly; avoids a panic when `/api/v1/rules` is called and the time zone is _not_ UTC; `rules` field is an empty array now if no rules have been defined in a rule group.
Thanos Rule's `/api/v1/rules` endpoint no longer returns the old, deprecated `partial_response_strategy`. The old, deprecated value has been fixed to `WARN` for quite some time. _Please_ use `partialResponseStrategy`.
- [#2976](https://github.com/thanos-io/thanos/pull/2976) Query: Better rounding for incoming query timestamps.
- [#2929](https://github.com/thanos-io/thanos/pull/2929) Mixin: Fix expression for 'unhealthy sidecar' alert and increase the timeout for 10 minutes.
- [#3024](https://github.com/thanos-io/thanos/pull/3024) Query: Consider group name and file for deduplication.
- [#3012](https://github.com/thanos-io/thanos/pull/3012) Ruler,Receiver: Fix TSDB to delete blocks in atomic way.
- [#3046](https://github.com/thanos-io/thanos/pull/3046) Ruler,Receiver: Fixed framing of StoreAPI response, it was one chunk by one.
- [#3095](https://github.com/thanos-io/thanos/pull/3095) Ruler: Update the manager when all rule files are removed.
- [#3105](https://github.com/thanos-io/thanos/pull/3105) Querier: Fix overwriting `maxSourceResolution` when auto downsampling is enabled.
- [#3010](https://github.com/thanos-io/thanos/pull/3010) Querier: Added `--query.lookback-delta` flag to override the default lookback delta in PromQL. The flag should be lookback delta should be set to at least 2 times of the slowest scrape interval. If unset it will use the PromQL default of 5m.

### Added

- [#2305](https://github.com/thanos-io/thanos/pull/2305) Receive,Sidecar,Ruler: Propagate correct (stricter) MinTime for TSDBs that have no block.
- [#2849](https://github.com/thanos-io/thanos/pull/2849) Query, Ruler: Added request logging for HTTP server side.
- [#2832](https://github.com/thanos-io/thanos/pull/2832) ui React: Add runtime and build info page
- [#2926](https://github.com/thanos-io/thanos/pull/2926) API: Add new blocks HTTP API to serve blocks metadata. The status endpoints (`/api/v1/status/flags`, `/api/v1/status/runtimeinfo` and `/api/v1/status/buildinfo`) are now available on all components with a HTTP API.
- [#2892](https://github.com/thanos-io/thanos/pull/2892) Receive: Receiver fails when the initial upload fails.
- [#2865](https://github.com/thanos-io/thanos/pull/2865) ui: Migrate Thanos Ruler UI to React
- [#2964](https://github.com/thanos-io/thanos/pull/2964) Query: Add time range parameters to label APIs. Add `start` and `end` fields to Store API `LabelNamesRequest` and `LabelValuesRequest`.
- [#2996](https://github.com/thanos-io/thanos/pull/2996) Sidecar: Add `reloader_config_apply_errors_total` metric. Add new flags `--reloader.watch-interval`, and `--reloader.retry-interval`.
- [#2973](https://github.com/thanos-io/thanos/pull/2973) Add Thanos Query Frontend component.
- [#2980](https://github.com/thanos-io/thanos/pull/2980) Bucket Viewer: Migrate block viewer to React.
- [#2725](https://github.com/thanos-io/thanos/pull/2725) Add bucket index operation durations: `thanos_bucket_store_cached_series_fetch_duration_seconds` and `thanos_bucket_store_cached_postings_fetch_duration_seconds`.
- [#2931](https://github.com/thanos-io/thanos/pull/2931) Query: Allow passing a `storeMatch[]` to select matching stores when debugging the querier. See [documentation](https://thanos.io/tip/components/query.md/#store-filtering)

### Changed

- [#2893](https://github.com/thanos-io/thanos/pull/2893) Store: Rename metric `thanos_bucket_store_cached_postings_compression_time_seconds` to `thanos_bucket_store_cached_postings_compression_time_seconds_total`.
- [#2915](https://github.com/thanos-io/thanos/pull/2915) Receive,Ruler: Enable TSDB directory locking by default. Add a new flag (`--tsdb.no-lockfile`) to override behavior.
- [#2902](https://github.com/thanos-io/thanos/pull/2902) Querier UI:Separate dedupe and partial response checkboxes per panel in new UI.
- [#2991](https://github.com/thanos-io/thanos/pull/2991) Store: *breaking :warning:* `operation` label value `getrange` changed to `get_range` for `thanos_store_bucket_cache_operation_requests_total` and `thanos_store_bucket_cache_operation_hits_total` to be consistent with bucket operation metrics.
- [#2876](https://github.com/thanos-io/thanos/pull/2876) Receive,Ruler: Updated TSDB and switched to ChunkIterators instead of sample one, which avoids unnecessary decoding / encoding.
- [#3064](https://github.com/thanos-io/thanos/pull/3064) s3: *breaking :warning:* Add SSE/SSE-KMS/SSE-C configuration. The S3 `encrypt_sse: true` option is now deprecated in favour of `sse_config`. If you used `encrypt_sse`, the migration strategy is to set up the following block:

```yaml
sse_config:
  type: SSE-S3
```