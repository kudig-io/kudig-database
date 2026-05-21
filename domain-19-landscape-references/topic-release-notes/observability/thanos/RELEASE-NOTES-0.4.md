---
title: thanos v0.4 Release Notes
description: thanos v0.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- flux
- minio
- gateway
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- thanos v0.4 Release Notes 是什么
- 如何 thanos v0.4 Release Notes
trigger_keywords:
- thanos
- v0.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

# thanos v0.4 Release Notes

Source: [v0.4.0](https://github.com/thanos-io/thanos/releases/tag/v0.4.0)

:warning: **IMPORTANT** :warning: This is the last release that supports gossip. From Thanos v0.5.0, gossip will be completely removed.

Major improvements:
* This release also disables gossip mode by default for all components.
See [this](docs/proposals/approved/201809_gossip-removal.md) for more details.
* Store Gateway startup process is massively improved in both efficiency and memory consumption
* Remote receiver component was added.
* StoreUI works now beautifully :tulip: 
* Timeout improvements for Querier
* Control of concurrency and sample limits on Store Gateway gRPC API
* Graceful handling and deletion of partial uploads made by Compactor.

### Added

- [thanos.io](https://thanos.io) website & automation :tada: 
- [#1053](https://github.com/improbable-eng/thanos/pull/1053) compactor: Compactor & store gateway now handles incomplete uploads gracefully. Added hard limit on how long block upload can take (30m).
- [#811](https://github.com/improbable-eng/thanos/pull/811) Remote write receiver component :heart: :heart: thanks to RedHat (@brancz) contribution.
- [#910](https://github.com/improbable-eng/thanos/pull/910) Query's stores UI page is now sorted by type and old DNS or File SD stores are removed after 5 minutes (configurable via the new `--store.unhealthy-timeout=5m` flag).
- [#905](https://github.com/improbable-eng/thanos/pull/905) Thanos support for Query API: /api/v1/labels. Notice that the API was added in Prometheus v2.6.
- [#798](https://github.com/improbable-eng/thanos/pull/798) Ability to limit the maximum number of concurrent request to Series() calls in Thanos Store and the maximum amount of samples we handle.
- [#1060](https://github.com/improbable-eng/thanos/pull/1060) Allow specifying region attribute in S3 storage configuration

:warning: **WARNING** :warning: #798 adds a new default limit to Thanos Store: `--store.grpc.series-max-concurrency`. Most likely you will want to make it the same as `--query.max-concurrent` on Thanos Query.

New options:

  New Store flags:
  
    * `--store.grpc.series-sample-limit` limits the amount of samples that might be retrieved on a single Series() call. By default it is 0. Consider enabling it by setting it to more than 0 if you are running on limited resources.
    * `--store.grpc.series-max-concurrency` limits the number of concurrent Series() calls in Thanos Store. By default it is 20. Considering making it lower or bigger depending on the scale of your deployment.

  New Store metrics:
  
    * `thanos_bucket_store_queries_dropped_total` shows how many queries were dropped due to the samples limit;
    * `thanos_bucket_store_queries_concurrent_max` is a constant metric which shows how many Series() calls can concurrently be executed by Thanos Store;
    * `thanos_bucket_store_queries_in_flight` shows how many queries are currently "in flight" i.e. they are being executed;
    * `thanos_bucket_store_gate_duration_seconds` shows how many seconds it took for queries to pass through the gate in both cases - when that fails and when it does not.
    
  New Store tracing span:
    * `store_query_gate_ismyturn` shows how long it took for a query to pass (or not) through the gate.
    
- [#1016](https://github.com/improbable-eng/thanos/pull/1016) Added option for another DNS resolver (miekg/dns client). 
Note that this is required to have SRV resolution working on [Golang 1.11+ with KubeDNS below v1.14](https://github.com/golang/go/issues/27546)

   New Querier and Ruler flag: `-- store.sd-dns-resolver` which allows to specify resolver to use. Either `golang` or `miekgdns`
   
- [#986](https://github.com/improbable-eng/thanos/pull/986) Allow to save some startup & sync time in store gateway as it is no longer needed to compute index-cache from block index on its own for larger blocks.
  The store Gateway still can do it, but it first checks bucket if there is index-cached uploaded already. 
  In the same time, compactor precomputes the index cache file on every compaction. 

  New Compactor flag: `--index.generate-missing-cache-file` was added to allow quicker addition of index cache files. If enabled it precomputes missing files on compactor startup. Note that it will take time and it's only one-off step per bucket.

- [#887](https://github.com/improbable-eng/thanos/pull/887) Compact: Added new `--block-sync-concurrency` flag, which allows you to configure number of goroutines to use when syncing block metadata from object storage.
- [#928](https://github.com/improbable-eng/thanos/pull/928) Query: Added `--store.response-timeout` flag. If a Store doesn't send any data in this specified duration then a Store will be ignored and partial data will be returned if it's enabled. 0 disables timeout.
- [#893](https://github.com/improbable-eng/thanos/pull/893) S3 storage backend has graduated to `stable` maturity level.
- [#936](https://github.com/improbable-eng/thanos/pull/936) Azure storage backend has graduated to `stable` maturity level.
- [#937](https://github.com/improbable-eng/thanos/pull/937) S3: added trace functionality. You can add `trace.enable: true` to enable the minio client's verbose logging.
- [#953](https://github.com/improbable-eng/thanos/pull/953) Compact: now has a hidden flag `--debug.accept-malformed-index`. Compaction index verification will ignore out of order label names.
- [#963](https://github.com/improbable-eng/thanos/pull/963) GCS: added possibility to inline ServiceAccount into GCS config.
- [#1010](https://github.com/improbable-eng/thanos/pull/1010) Compact: added new flag `--compact.concurrency`. Number of goroutines to use when compacting groups.
- [#1028](https://github.com/improbable-eng/thanos/pull/1028) Query: added `--query.default-evaluation-interval`, which sets default evaluation interval for sub queries.
- [#980](https://github.com/improbable-eng/thanos/pull/980) Ability to override Azure storage endpoint for other regions (China)
- [#1021](https://github.com/improbable-eng/thanos/pull/1021) Query API `series` now supports POST method.
- [#939](https://github.com/improbable-eng/thanos/pull/939) Query API `query_range` now supports POST method.

### Changed 

- [#970](https://github.com/improbable-eng/thanos/pull/970) Deprecated `partial_response_disabled` proto field. Added `partial_response_strategy` instead. Both in gRPC and Query API.
  No `PartialResponseStrategy` field for `RuleGroups` by default means `abort` strategy (old PartialResponse disabled) as this is recommended option for Rules and alerts.

  Metrics:
    
    * Added `thanos_rule_evaluation_with_warnings_total` to Ruler.
    * DNS `thanos_ruler_query_apis*` are now `thanos_ruler_query_apis_*` for consistency.
    * DNS `thanos_querier_store_apis*` are now `thanos_querier_store_apis__*` for consistency.
    * Query Gate `thanos_bucket_store_series*` are now `thanos_bucket_store_series_*` for consistency.
    * Most of thanos ruler metris related to rule manager has `strategy` label.
  
  Ruler tracing spans:
  
    * `/rule_instant_query HTTP[client]` is now `/rule_instant_query_part_resp_abort HTTP[client]"` if request is for abort strategy.
    
- [#1009](https://github.com/improbable-eng/thanos/pull/1009): Upgraded Prometheus (~v2.7.0-rc.0 to v2.8.1)  and TSDB (`v0.4.0` to `v0.6.1`) deps.
  
  Changes that affects Thanos:
   * query: 
     * [ENHANCEMENT] In histogram_quantile merge buckets with equivalent le values. #5158.   
     * [ENHANCEMENT] Show list of offending labels in the error message in many-to-many scenarios. #5189   
     * [BUGFIX] Fix panic when aggregator param is not a literal. #5290
   * ruler: 
     * [ENHANCEMENT] Reduce time that Alertmanagers are in flux when reloaded. #5126
     * [BUGFIX] prometheus_rule_group_last_evaluation_timestamp_seconds is now a unix timestamp. #5186
     * [BUGFIX] prometheus_rule_group_last_duration_seconds now reports seconds instead of nanoseconds. Fixes our [issue #1027](https://github.com/improbable-eng/thanos/issues/1027)
     * [BUGFIX] Fix sorting of rule groups. #5260
   * store: [ENHANCEMENT] Fast path for EmptyPostings cases in Merge, Intersect and Without.
   * tooling: [FEATURE] New dump command to tsdb tool to dump all samples.
   * compactor: [ENHANCEMENT] When closing the db any running compaction will be cancelled so it doesn't block.
   * [CHANGE] Renamed flag `--sync-delay` to `--consistency-delay` [#1053](https://github.com/improbable-eng/thanos/pull/1053)

  
  For ruler essentially whole TSDB CHANGELOG applies beween v0.4.0-v0.6.1: https://github.com/prometheus/tsdb/blob/master/CHANGELOG.md
  
  Note that this was added on TSDB and Prometheus: [FEATURE] Time-ovelapping blocks are now allowed. #370
  Whoever due to nature of Thanos compaction (distributed systems), for safety reason this is disabled for Thanos compactor for now.
 
- [#868](https://github.com/improbable-eng/thanos/pull/868) Go has been updated to 1.12.
- [#1055](https://github.com/improbable-eng/thanos/pull/1055) Gossip flags are now disabled by default and deprecated. 
- [#964](https://github.com/improbable-eng/thanos/pull/964) repair: Repair process now sorts the series and labels within block.
- [#1073](https://github.com/improbable-eng/thanos/pull/1073) Store: index cache for requests. It now calculates the size properly (includes slice header), has anti-deadlock safeguard and reports more metrics.

### Fixed

- [#921](https://github.com/improbable-eng/thanos/pull/921) `thanos_objstore_bucket_last_successful_upload_time` now does not appear when no blocks have been uploaded so far.
- [#966](https://github.com/improbable-eng/thanos/pull/966) Bucket: verify no longer warns about overlapping blocks, that overlap `0s` 
- [#848](https://github.com/improbable-eng/thanos/pull/848) Compact: now correctly works with time series with duplicate labels.
- [#894](https://github.com/improbable-eng/thanos/pull/894) Thanos Rule: UI now correctly shows evaluation time.
- [#865](https://github.com/improbable-eng/thanos/pull/865) Query: now properly parses DNS SRV Service Discovery.
- [#889](https://github.com/improbable-eng/thanos/pull/889) Store: added safeguard against merging posting groups segfault
- [#941](https://github.com/improbable-eng/thanos/pull/941) Sidecar: added better handling of intermediate restarts.
- [#933](https://github.com/improbable-eng/thanos/pull/933) Query: Fixed 30 seconds lag of adding new store to query.
- [#962](https://github.com/improbable-eng/thanos/pull/962) Sidecar: Make config reloader file writes atomic. 
- [#982](https://github.com/improbable-eng/thanos/pull/982) Query: now advertises Min & Max Time accordingly to the nodes.
- [#1041](https://github.com/improbable-eng/thanos/issues/1038) Ruler is now able to return long time range queries.
- [#904](https://github.com/improbable-eng/thanos/pull/904) Compact: Skip compaction for blocks with no samples.
- [#1070](https://github.com/improbable-eng/thanos/pull/1070) Downsampling works back again. Deferred closer errors are now properly captured.

See the full changelog [here](CHANGELOG.md)