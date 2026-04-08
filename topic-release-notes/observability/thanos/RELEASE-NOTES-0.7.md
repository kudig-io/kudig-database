# thanos v0.7 Release Notes

Source: [v0.7.0](https://github.com/thanos-io/thanos/releases/tag/v0.7.0)

## v0.7.0

Accepted into CNCF:
- Thanos moved to new repository <https://github.com/thanos-io/thanos>
- Docker images moved to <https://quay.io/thanos/thanos> and mirrored at <https://hub.docker.com/r/thanosio/thanos>
- Slack moved to <https://slack.cncf.io> `#thanos`/`#thanos-dev` / `#thanos-prs`

### Added

- [#1378](https://github.com/thanos-io/thanos/pull/1378) Thanos Receive now exposes `thanos_receive_config_hash`, `thanos_receive_config_last_reload_successful` and `thanos_receive_config_last_reload_success_timestamp_seconds` metrics to track latest configuration change
- [#1268](https://github.com/thanos-io/thanos/pull/1268) Thanos Sidecar added support for newest Prometheus streaming remote read added [here](https://github.com/prometheus/prometheus/pull/5703). This massively improves memory required by single 
  request for both Prometheus and sidecar. Single requests now should take constant amount of memory on sidecar, so resource consumption prediction is now straightforward. This will be used if you have Prometheus `2.13` or `2.12-master`.   
- [#1358](https://github.com/thanos-io/thanos/pull/1358) Added `part_size` configuration option for HTTP multipart requests minimum part size for S3 storage type
- [#1363](https://github.com/thanos-io/thanos/pull/1363) Thanos Receive now exposes `thanos_receive_hashring_nodes` and `thanos_receive_hashring_tenants` metrics to monitor status of hash-rings
- [#1395](https://github.com/thanos-io/thanos/pull/1395) Thanos Sidecar added `/-/ready` and `/-/healthy` endpoints to Thanos sidecar.
- [#1297](https://github.com/thanos-io/thanos/pull/1297) Thanos Compact added `/-/ready` and `/-/healthy` endpoints to Thanos compact.
- [#1431](https://github.com/thanos-io/thanos/pull/1431) Thanos Query added hidden flag to allow the use of downsampled resolution data for instant queries.
- [#1408](https://github.com/thanos-io/thanos/pull/1408) Thanos Store Gateway can now allow the specifying of supported time ranges it will serve (time sharding). Flags: `min-time` & `max-time`

### Changed

- [#1414](https://github.com/thanos-io/thanos/pull/1413) Upgraded important dependencies: Prometheus to 2.12-rc.0. TSDB is now part of Prometheus.
- [#1380](https://github.com/thanos-io/thanos/pull/1380) Upgraded important dependencies: Prometheus to 2.11.1 and TSDB to 0.9.1. Some changes affecting Querier:
  - [ENHANCEMENT] Query performance improvement: Efficient iteration and search in HashForLabels and HashWithoutLabels. #5707
  - [ENHANCEMENT] Optimize queries using regexp for set lookups. tsdb#602
  - [BUGFIX] prometheus_tsdb_compactions_failed_total is now incremented on any compaction failure. tsdb#613
  - [BUGFIX] PromQL: Correctly display {__name__="a"}.
- [#1338](https://github.com/thanos-io/thanos/pull/1338) Thanos Query still warns on store API duplicate, but allows a single one from duplicated set. This is gracefully warn about the problematic logic and not disrupt immediately.
- [#1385](https://github.com/thanos-io/thanos/pull/1385) Thanos Compact exposes flag to disable downsampling `downsampling.disable`. 

### Fixed

- [#1327](https://github.com/thanos-io/thanos/pull/1327) Thanos Query `/series` API end-point now properly returns an empty array just like Prometheus if there are no results
- [#1302](https://github.com/thanos-io/thanos/pull/1302) Thanos now efficiently reuses HTTP keep-alive connections
- [#1371](https://github.com/thanos-io/thanos/pull/1371) Thanos Receive fixed race condition in hashring
- [#1430](https://github.com/thanos-io/thanos/pull/1430) Thanos fixed value of GOMAXPROCS inside container.

### Deprecated

- [#1458](https://github.com/thanos-io/thanos/pull/1458) Thanos Query and Receive now use common instrumentation middleware. As as result, for sake of `http_requests_total` and `http_request_duration_seconds_bucket`; Thanos Query no longer exposes `thanos_query_api_instant_query_duration_seconds`, `thanos_query_api_range_query_duration_second` metrics and Thanos Receive no longer exposes `thanos_http_request_duration_seconds`, `thanos_http_requests_total`, `thanos_http_response_size_bytes`.
- [#1423](https://github.com/thanos-io/thanos/pull/1423) Thanos Bench deprecated.
