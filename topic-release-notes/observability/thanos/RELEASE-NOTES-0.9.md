# thanos v0.9 Release Notes

Source: [v0.9.0](https://github.com/thanos-io/thanos/releases/tag/v0.9.0)

Thanks to all contributors!

Worth-noting changes: Support for AlibabaCloud object storage; LightStep tracing; Ruler fixes, Store UI page fixed, Store gateway has now metrics for startup cycle plus optimization.

### Added

- [#1678](https://github.com/thanos-io/thanos/pull/1678) Add Lightstep as a tracing provider.
- [#1687](https://github.com/thanos-io/thanos/pull/1687) Add a new `--grpc-grace-period` CLI option to components which serve gRPC to set how long to wait until gRPC Server shuts down.
- [#1660](https://github.com/thanos-io/thanos/pull/1660) Sidecar: Add a new `--prometheus.ready_timeout` CLI option to the sidecar to set how long to wait until Prometheus starts up.
- [#1573](https://github.com/thanos-io/thanos/pull/1573) `AliYun OSS` object storage, see [documents](docs/storage.md#aliyun-oss) for further information.
- [#1680](https://github.com/thanos-io/thanos/pull/1680) Add a new `--http-grace-period` CLI option to components which serve HTTP to set how long to wait until HTTP Server shuts down.
- [#1712](https://github.com/thanos-io/thanos/pull/1712) Bucket: Rename flag on bucket web component from `--listen` to `--http-address` to match other components.
- [#1733](https://github.com/thanos-io/thanos/pull/1733) Compactor: New metric `thanos_compactor_iterations_total` on Thanos Compactor which shows the number of successful iterations.
- [#1758](https://github.com/thanos-io/thanos/pull/1758) Bucket: `thanos bucket web` now supports `--web.external-prefix` for proxying on a subpath.
- [#1770](https://github.com/thanos-io/thanos/pull/1770) Bucket: Add `--web.prefix-header` flags to allow for bucket UI to be accessible behind a reverse proxy.
- [#1668](https://github.com/thanos-io/thanos/pull/1668) Receiver: Added TLS options for both server and client remote write.

### Fixed

- [#1656](https://github.com/thanos-io/thanos/pull/1656) Store Gateway: Store now starts metric and status probe HTTP server earlier in its start-up sequence. `/-/healthy` endpoint now starts to respond with success earlier. `/metrics` endpoint starts serving metrics earlier as well. Make sure to point your readiness probes to the `/-/ready` endpoint rather than `/metrics`.
- [#1669](https://github.com/thanos-io/thanos/pull/1669) Store Gateway: Fixed store sharding. Now it does not load excluded meta.jsons and load/fetch index-cache.json files.
- [#1670](https://github.com/thanos-io/thanos/pull/1670) Sidecar: Fixed un-ordered blocks upload. Sidecar now uploads the oldest blocks first.
- [#1568](https://github.com/thanos-io/thanos/pull/1709) Store Gateway: Store now retains the first raw value of a chunk during downsampling to avoid losing some counter resets that occur on an aggregation boundary.
- [#1751](https://github.com/thanos-io/thanos/pull/1751) Querier: Fixed labels for StoreUI
- [#1773](https://github.com/thanos-io/thanos/pull/1773) Ruler: Fixed the /api/v1/rules endpoint that returned 500 status code with `failed to assert type of rule ...` message.
- [#1770](https://github.com/thanos-io/thanos/pull/1770) Querier: Fixed `--web.external-prefix` 404s for static resources.
- [#1785](https://github.com/thanos-io/thanos/pull/1785) Ruler: The /api/v1/rules endpoints now returns the original rule filenames.
- [#1791](https://github.com/thanos-io/thanos/pull/1791) Ruler: Ruler now supports identical rule filenames in different directories.
- [#1562](https://github.com/thanos-io/thanos/pull/1562) Querier: Downsampling option now carries through URL.
- [#1675](https://github.com/thanos-io/thanos/pull/1675) Querier: Reduced resource usage while using certain queries like `offset`.
- [#1725](https://github.com/thanos-io/thanos/pull/1725) & [#1718](https://github.com/thanos-io/thanos/pull/1718) Store Gateway: Per request memory improvements.

### Changed

- [#1666](https://github.com/thanos-io/thanos/pull/1666) Compact: `thanos_compact_group_compactions_total` now counts block compactions, so operations that resulted in a compacted block. The old behaviour
is now exposed by new metric: `thanos_compact_group_compaction_runs_started_total` and `thanos_compact_group_compaction_runs_completed_total` which counts compaction runs overall.
- [#1748](https://github.com/thanos-io/thanos/pull/1748) Updated all dependencies.
- [#1694](https://github.com/thanos-io/thanos/pull/1694) `prober_ready` and `prober_healthy` metrics are removed, for sake of `status`. Now `status` exposes same metric with a label, `check`. `check` can have "healty" or "ready" depending on status of the probe.
- [#1790](https://github.com/thanos-io/thanos/pull/1790) Ruler: Fixes subqueries support for ruler.
- [#1769](https://github.com/thanos-io/thanos/pull/1769) & [#1545](https://github.com/thanos-io/thanos/pull/1545) Adjusted most of the metrics histogram buckets.