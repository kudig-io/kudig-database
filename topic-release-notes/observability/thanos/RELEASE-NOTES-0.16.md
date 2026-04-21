# thanos v0.16 Release Notes

Source: [v0.16.0](https://github.com/thanos-io/thanos/releases/tag/v0.16.0)

Highlights:

* New Thanos component, [Query Frontend](https://thanos.io/tip/components/query-frontend.md/) has more options and supports shared cache (currently: Memcached).
* Added debug mode in Thanos UI that allows to filter Stores to query from by their IPs from Store page (!). This helps enormously in e.g debugging the slowest store etc. All raw Thanos API allows passing
`storeMatch[]` arguments with `__address__` matchers.
* Improved debuggability on all Thanos components by exposing [off-CPU profiles thanks to fgprof endpoint](https://github.com/felixge/fgprof).
* Significantly improved sidecar latency and CPU usage for metrics fetches.

### Fixed

- [#3234](https://github.com/thanos-io/thanos/pull/3234) UI: Fix assets not loading when `--web.prefix-header` is used.
- [#3184](https://github.com/thanos-io/thanos/pull/3184) Compactor: Fixed support for `web.external-prefix` for Compactor UI.

### Added

- [#3114](https://github.com/thanos-io/thanos/pull/3114) Query Frontend: Added support for Memacached cache.
    - **breaking** Renamed flag `log_queries_longer_than` to `log-queries-longer-than`.
- [#3166](https://github.com/thanos-io/thanos/pull/3166) UIs: Added UI for passing a `storeMatch[]` parameter to queries.
- [#3181](https://github.com/thanos-io/thanos/pull/3181) Logging: Added debug level logging for responses between 300-399
- [#3133](https://github.com/thanos-io/thanos/pull/3133) Query: Allowed passing a `storeMatch[]` to Labels APIs; Time range metadata based store filtering is supported on Labels APIs.
- [#3146](https://github.com/thanos-io/thanos/pull/3146) Sidecar: Significantly improved sidecar latency (reduced ~2x). Added `thanos_sidecar_prometheus_store_received_frames` histogram metric.
- [#3147](https://github.com/thanos-io/thanos/pull/3147) Querier: Added `query.metadata.default-time-range` flag to specify the default metadata time range duration for retrieving labels through Labels and Series API when the range parameters are not specified. The zero value means range covers the time since the beginning.
- [#3207](https://github.com/thanos-io/thanos/pull/3207) Query Frontend: Added `cache-compression-type` flag to use compression in the query frontend cache.
- [#3122](https://github.com/thanos-io/thanos/pull/3122) *: All Thanos components have now `/debug/fgprof` endpoint on HTTP port allowing to get [off-CPU profiles as well](https://github.com/felixge/fgprof).
- [#3109](https://github.com/thanos-io/thanos/pull/3109) Query Frontend: Added support for `Cache-Control` HTTP response header which controls caching behaviour. So far `no-store` value is supported and it makes the response skip cache.
- [#3092](https://github.com/thanos-io/thanos/pull/3092) Tools: Added `tools bucket cleanup` CLI tool that deletes all blocks marked to be deleted.

### Changed

- [#3136](https://github.com/thanos-io/thanos/pull/3136) Sidecar: **breaking** Added metric `thanos_sidecar_reloader_config_apply_operations_total` and rename metric `thanos_sidecar_reloader_config_apply_errors_total` to `thanos_sidecar_reloader_config_apply_operations_failed_total`.
- [#3154](https://github.com/thanos-io/thanos/pull/3154) Querier: **breaking** Added metric `thanos_query_gate_queries_max`. Remove metric `thanos_query_concurrent_selects_gate_queries_in_flight`.
- [#3154](https://github.com/thanos-io/thanos/pull/3154) Store: **breaking** Renamed metric `thanos_bucket_store_queries_concurrent_max` to `thanos_bucket_store_series_gate_queries_max`.
- [#3179](https://github.com/thanos-io/thanos/pull/3179) Store: context.Canceled will not increase `thanos_objstore_bucket_operation_failures_total`.
- [#3136](https://github.com/thanos-io/thanos/pull/3136) Sidecar: Improved detection of directory changes for Prometheus config.
    - **breaking** Added metric `thanos_sidecar_reloader_config_apply_operations_total` and rename metric `thanos_sidecar_reloader_config_apply_errors_total` to `thanos_sidecar_reloader_config_apply_operations_failed_total`.
- [#3022](https://github.com/thanos-io/thanos/pull/3022) *: Thanos images are now build with Go 1.15.
- [#3205](https://github.com/thanos-io/thanos/pull/3205) *: Updated TSDB to ~2.21