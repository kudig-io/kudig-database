---
title: thanos v0.13 Release Notes
description: thanos v0.13 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.13 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- opa
- minio
- gateway
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
- thanos v0.13 Release Notes 是什么
- 如何 thanos v0.13 Release Notes
trigger_keywords:
- thanos
- v0.13
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- policy-basics
---



# [[Thanos|thanos]] v0.13 Release Notes

Source: [v0.13.0](https://github.com/thanos-io/thanos/releases/tag/v0.13.0)

## Highlights:

* Rare overlapping block issue fixed
* Deduplication bug with counters fixed
* Receiver is now multi-tenant (multi TSDB)
* Added @memcached meta and chunk caching. (e.g allows to significantly limit objstore traffic) 
* TSDB isolation added for Ruler and Receive
* Querier performance significantly improved especially when used with duplicated Store APIs (e.g for HA purposes).

Thanks all for contributing! :hugs: 

# Fixed

- [#2548](https://github.com/thanos-io/thanos/pull/2548) Query: Fixed rare cases of double counter reset accounting when querying `rate` with deduplication enabled.
- [#2536](https://github.com/thanos-io/thanos/pull/2536) S3: Fixed AWS STS endpoint url to https for Web Identity providers on AWS EKS.
- [#2501](https://github.com/thanos-io/thanos/pull/2501) Query: Gracefully handle additional fields in `SeriesResponse` protobuf message that may be added in the future.
- [#2568](https://github.com/thanos-io/thanos/pull/2568) Query: Don't close the connection of strict, static nodes if establishing a connection had succeeded but Info() call failed.
- [#2615](https://github.com/thanos-io/thanos/pull/2615) Rule: Fix bugs where rules were out of sync.
- [#2614](https://github.com/thanos-io/thanos/pull/2614) Tracing: Disabled Elastic APM Go Agent default tracer on initialization to disable the default metric gatherer.
- [#2525](https://github.com/thanos-io/thanos/pull/2525) Query: Fixed logging for dns resolution error in the `Query` component.
- [#2484](https://github.com/thanos-io/thanos/pull/2484) Query/Ruler: Fixed issue #2483, when web.route-prefix is set, it is added twice in HTTP router prefix.
- [#2416](https://github.com/thanos-io/thanos/pull/2416) Bucket: Fixed issue #2416 bug in `inspect --sort-by` doesn't work correctly in all cases.
- [#2719](https://github.com/thanos-io/thanos/pull/2719) Query: `irate` and `resets` use now counter downsampling aggregations.
- [#2705](https://github.com/thanos-io/thanos/pull/2705) minio-go: Added support for `af-south-1` and `eu-south-1` regions.
- [#2753](https://github.com/thanos-io/thanos/issues/2753) Sidecar, Receive, Rule: Fixed possibility of out of order uploads in error cases. This could potentially cause Compactor to create overlapping blocks.

### Added

- [#2012](https://github.com/thanos-io/thanos/pull/2012) Receive: Added multi-tenancy support (based on header)
- [#2502](https://github.com/thanos-io/thanos/pull/2502) StoreAPI: Added `hints` field to `SeriesResponse`. Hints in an opaque data structure that can be used to carry additional information from the store and its content is implementation specific.
- [#2521](https://github.com/thanos-io/thanos/pull/2521) Sidecar: Added `thanos_sidecar_reloader_reloads_failed_total`, `thanos_sidecar_reloader_reloads_total`, `thanos_sidecar_reloader_watch_errors_total`, `thanos_sidecar_reloader_watch_events_total` and `thanos_sidecar_reloader_watches` metrics.
- [#2412](https://github.com/thanos-io/thanos/pull/2412) UI: Added React UI from Prometheus upstream. Currently only accessible from Query component as only `/graph` endpoint is migrated.
- [#2532](https://github.com/thanos-io/thanos/pull/2532) Store: Added hidden option `--store.caching-bucket.config=<yaml content>` (or `--store.caching-bucket.config-file=<file.yaml>`) for experimental caching bucket, that can cache chunks into shared memcached. This can speed up querying and reduce number of requests to object storage.
- [#2579](https://github.com/thanos-io/thanos/pull/2579) Store: Experimental caching bucket can now cache metadata as well. Config has changed from #2532.
- [#2526](https://github.com/thanos-io/thanos/pull/2526) Compact: In case there are no labels left after deduplication via `--deduplication.replica-label`, assign first `replica-label` with value `deduped`.
- [#2621](https://github.com/thanos-io/thanos/pull/2621) Receive: Added flag to configure forward request timeout. Receive write will complete request as soon as a quorum of writes succeeds.

### Changed

- [#2194](https://github.com/thanos-io/thanos/pull/2194) Updated to golang v1.14.2.
- [#2505](https://github.com/thanos.io/thanos/pull/2505) Store: Removed obsolete `thanos_store_node_info` metric.
- [2513](https://github.com/thanos-io/thanos/pull/2513) Tools: Moved `thanos bucket` commands to `thanos tools bucket`, also
moved `thanos check rules` to `thanos tools rules-check`. `thanos tools rules-check` also takes rules by `--rules` repeated flag not argument
anymore.
- [#2548](https://github.com/thanos-io/thanos/pull/2548/commits/53e69bd89b2b08c18df298eed7d90cb7179cc0ec) Store, Querier: remove duplicated chunks on StoreAPI.
- [#2596](https://github.com/thanos-io/thanos/pull/2596) Updated Prometheus dependency to [@cd73b3d33e064bbd846fc7a26dc8c313d46af382](https://github.com/prometheus/prometheus/commit/cd73b3d33e064bbd846fc7a26dc8c313d46af382) which falls in between v2.17.0 and v2.18.0.
    - Receive,Rule: TSDB now supports isolation of append and queries.
    - Receive,Rule: TSDB now holds less WAL files after Head Truncation.
- [#2450](https://github.com/thanos-io/thanos/pull/2450) Store: Added Regex-set optimization for `label=~"a|b|c"` matchers.
- [#2526](https://github.com/thanos-io/thanos/pull/2526) Compact: In case there are no labels left after deduplication via `--deduplication.replica-label`, assign first `replica-label` with value `deduped`.
- [2603](https://github.com/thanos-io/thanos/pull/2603) Store/Querier: Significantly optimize cases where StoreAPIs or blocks returns exact overlapping chunks (e.g Store GW and sidecar or brute force Store Gateway HA).
