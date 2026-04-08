# thanos v0.18 Release Notes

Source: [v0.18.0](https://github.com/thanos-io/thanos/releases/tag/v0.18.0)

### Highlights

- Several big optimizations speeding up query performance were introduced!
- A new command was added, `thanos tools bucket rewrite`, enabling the deletion of series from given block.
- The Query Frontend now supports proxying requests to the `labels` and `series` API endpoints.
- `thanos tools bucket replicate` can now copy particular blocks by ID.
- The number of series touched by the Store component when servicing a `Series` call can now be limited using a CLI flag.

### Added

- [#3380](https://github.com/thanos-io/thanos/pull/3380) Mixin: Add block deletion panels for compactor dashboards.
- [#3568](https://github.com/thanos-io/thanos/pull/3568) Store: Optimized inject label stage of index lookup.
- [#3566](https://github.com/thanos-io/thanos/pull/3566) StoreAPI: Support label matchers in labels API.
- [#3531](https://github.com/thanos-io/thanos/pull/3531) Store: Optimized common cases for time selecting smaller amount of series by avoiding looking up symbols.
- [#3469](https://github.com/thanos-io/thanos/pull/3469) StoreAPI: Added `hints` field to `LabelNamesRequest` and `LabelValuesRequest`. Hints are an opaque data structure that can be used to carry additional information from the store and its content is implementation-specific.
- [#3421](https://github.com/thanos-io/thanos/pull/3421) Tools: Added `thanos tools bucket rewrite` command allowing to delete series from given block.
- [#3509](https://github.com/thanos-io/thanos/pull/3509) Store: Added a CLI flag to limit the number of series that are touched.
- [#3444](https://github.com/thanos-io/thanos/pull/3444) Query Frontend: Make POST request to downstream URL for labels and series API endpoints.
- [#3388](https://github.com/thanos-io/thanos/pull/3388) Tools: Bucket replicator now can specify block IDs to copy.
- [#3385](https://github.com/thanos-io/thanos/pull/3385) Tools: Bucket prints extra statistics for block index with debug log-level.
- [#3121](https://github.com/thanos-io/thanos/pull/3121) Receive: Added `--receive.hashrings` alternative to `receive.hashrings-file` flag (lower priority). The flag expects the literal hashring configuration in JSON format.
### Fixed

- [#3567](https://github.com/thanos-io/thanos/pull/3567) Mixin: Reintroduce `thanos_objstore_bucket_operation_failures_total` alert.
- [#3527](https://github.com/thanos-io/thanos/pull/3527) Query Frontend: Fix query_range behavior when start/end times are the same
- [#3560](https://github.com/thanos-io/thanos/pull/3560) Query Frontend: Allow separate label cache
- [#3672](https://github.com/thanos-io/thanos/pull/3672) Rule: Prevent crashing due to `no such host error` when using `dnssrv+` or `dnssrvnoa+`.
- [#3461](https://github.com/thanos-io/thanos/pull/3461) Compact, Shipper, Store: Fixed panic when no external labels are set in block metadata.

### Changed

- [#3496](https://github.com/thanos-io/thanos/pull/3496) S3: Respect SignatureV2 flag for all credential providers.
- [#2732](https://github.com/thanos-io/thanos/pull/2732) Swift: Switched to a new library [ncw/swift](https://github.com/ncw/swift) providing large objects support.
   By default, segments will be uploaded to the same container directory `segments/` if the file is bigger than `1GB`.
   To change the defaults see [the docs](./docs/storage.md#openstack-swift).
- [#3626](https://github.com/thanos-io/thanos/pull/3626) Shipper: Failed upload of `meta.json` file doesn't cause block cleanup anymore. This has a potential to generate corrupted blocks under specific conditions. Partial block is left in bucket for later cleanup.