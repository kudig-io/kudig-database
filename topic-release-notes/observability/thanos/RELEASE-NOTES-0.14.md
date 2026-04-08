# thanos v0.14 Release Notes

Source: [v0.14.0](https://github.com/thanos-io/thanos/releases/tag/v0.14.0)

## Highlights:

* Upgrade to Prometheus [@3268eac2ddda](https://github.com/prometheus/prometheus/commit/3268eac2ddda) which is after v2.18.1.
    - TSDB now does memory-mapping of Head chunks and reduces memory usage.
* Querier performs concurrent select per query.
* Changed `bucket tool bucket verify` `--id-whitelist` flag to `--id`.
* Store removed support to the legacy `index.cache.json`. The hidden flag `--store.disable-index-header` was removed.
* Store decreased memory allocations while querying block's index.

Thanks all for contributing! :hugs:

### Fixed

- [#2637](https://github.com/thanos-io/thanos/pull/2637) Compact: Detect retryable errors that are inside of a wrapped `tsdb.MultiError`.
- [#2648](https://github.com/thanos-io/thanos/pull/2648) Store: Allow index cache and caching bucket to be configured at the same time.
- [#2705](https://github.com/thanos-io/thanos/pull/2705) minio-go: Added support for `af-south-1` and `eu-south-1` regions.
- [#2728](https://github.com/thanos-io/thanos/pull/2728) Query: Fixed panics when using a larger number of replica labels with short series label sets.
- [#2787](https://github.com/thanos-io/thanos/pull/2787) Update Prometheus mod to pull in prometheus/prometheus#7414.
- [#2807](https://github.com/thanos-io/thanos/pull/2807) Store: Decreased memory allocations while querying block's index.
- [#2809](https://github.com/thanos-io/thanos/pull/2809) Query: `/api/v1/stores` now guarantees to return a string in the `lastError` field.

### Changed

- [#2658](https://github.com/thanos-io/thanos/pull/2658) [#2703](https://github.com/thanos-io/thanos/pull/2703) Upgrade to Prometheus [@3268eac2ddda](https://github.com/prometheus/prometheus/commit/3268eac2ddda) which is after v2.18.1.
    - TSDB now does memory-mapping of Head chunks and reduces memory usage.
- [#2667](https://github.com/thanos-io/thanos/pull/2667) Store: Removed support to the legacy `index.cache.json`. The hidden flag `--store.disable-index-header` was removed.
- [#2613](https://github.com/thanos-io/thanos/pull/2613) Store: Renamed the caching bucket config option `chunk_object_size_ttl` to `chunk_object_attrs_ttl`.
- [#2667](https://github.com/thanos-io/thanos/pull/2667) Compact: The deprecated flag `--index.generate-missing-cache-file` and the metric `thanos_compact_generated_index_total` were removed.
- [2603](https://github.com/thanos-io/thanos/pull/2603) Store/Querier: Significantly optimize cases where StoreAPIs or blocks returns exact overlapping chunks (e.g Store GW and sidecar or brute force Store Gateway HA).
- [#2671](https://github.com/thanos-io/thanos/pull/2671) *breaking* Tools: Bucket replicate flag `--resolution` is now in Go duration format.
- [#2671](https://github.com/thanos-io/thanos/pull/2671) Tools: Bucket replicate now replicates by default all blocks.
- [#2739](https://github.com/thanos-io/thanos/pull/2739) Changed `bucket tool bucket verify` `--id-whitelist` flag to `--id`.
- [#2748](https://github.com/thanos-io/thanos/pull/2748) Upgrade Prometheus to [@66dfb951c4ca](https://github.com/prometheus/prometheus/commit/66dfb951c4ca2c1dd3f266172a48a925403b13a5) which is after v2.19.0.
    - PromQL now allows us to executed concurrent selects.

### Added

- [#2671](https://github.com/thanos-io/thanos/pull/2671) Tools: Bucket replicate now allows passing repeated `--compaction` and `--resolution` flags.
- [#2657](https://github.com/thanos-io/thanos/pull/2657) Querier: Add the ability to perform concurrent select request per query.
- [#2754](https://github.com/thanos-io/thanos/pull/2671) UI: Add stores page in the React UI.
- [#2752](https://github.com/thanos-io/thanos/pull/2752) Compact: Add flag `--block-viewer.global.sync-block-interval` to configure metadata sync interval for the bucket UI.