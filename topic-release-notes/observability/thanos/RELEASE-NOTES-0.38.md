# thanos v0.38 Release Notes

Source: [v0.38.0](https://github.com/thanos-io/thanos/releases/tag/v0.38.0)

v0.38.0 is out now, please see the changelog below for what happened! 🚀

## Changelog

### Fixed
- [#8091](https://github.com/thanos-io/thanos/pull/8091) *: Add POST into allowed CORS methods header
- [#8046](https://github.com/thanos-io/thanos/pull/8046) Query-Frontend: Fix query statistic reporting for range queries when caching is enabled.
- [#7978](https://github.com/thanos-io/thanos/pull/7978) Receive: Fix deadlock during local writes when `split-tenant-label-name` is used
- [#8016](https://github.com/thanos-io/thanos/pull/8016) Query Frontend: Fix @ modifier not being applied correctly on sub queries.
-[#8175](https://github.com/thanos-io/thanos/pull/8175) Query: fix endpointset setup

### Added

- [#7907](https://github.com/thanos-io/thanos/pull/7907) Receive: Add `--receive.grpc-service-config` flag to configure gRPC service config for the receivers.
- [#7961](https://github.com/thanos-io/thanos/pull/7961) Store Gateway: Add `--store.posting-group-max-keys` flag to mark posting group as lazy if it exceeds number of keys limit. Added `thanos_bucket_store_lazy_expanded_posting_groups_total` for total number of lazy posting groups and corresponding reasons.
- [#8000](https://github.com/thanos-io/thanos/pull/8000) Query: Bump promql-engine, pass partial response through options
- [#7353](https://github.com/thanos-io/thanos/pull/7353) [#8045](https://github.com/thanos-io/thanos/pull/8045) Receiver/StoreGateway: Add `--matcher-cache-size` option to enable caching for regex matchers in series calls.
- [#8017](https://github.com/thanos-io/thanos/pull/8017) Store Gateway: Use native histogram for binary reader load and download duration and fixed download duration metric. #8017
- [#8131](https://github.com/thanos-io/thanos/pull/8131) Store Gateway: Optimize regex matchers for .* and .+. #8131
- [#7808](https://github.com/thanos-io/thanos/pull/7808) Query: Support chain deduplication algorithm.
- [#8158](https://github.com/thanos-io/thanos/pull/8158) Rule: Add support for query offset.
- [#8110](https://github.com/thanos-io/thanos/pull/8110) Compact: implement native histogram downsampling.
- [#7996](https://github.com/thanos-io/thanos/pull/7996) Receive: Add OTLP endpoint.

### Changed

- [#7890](https://github.com/thanos-io/thanos/pull/7890) Query,Ruler: *breaking :warning:* deprecated `--store.sd-file` and `--store.sd-interval` to be replaced with `--endpoint.sd-config` and `--endpoint-sd-config-reload-interval`; removed legacy flags to pass endpoints `--store`, `--metadata`, `--rule`, `--exemplar`.
- [#7012](https://github.com/thanos-io/thanos/pull/7012) Query: Automatically adjust `max_source_resolution` based on promql query to avoid querying data from higher resolution resulting empty results.
- [#8118](https://github.com/thanos-io/thanos/pull/8118) Query: Bumped promql-engine
- [#8135](https://github.com/thanos-io/thanos/pull/8135) Query: respect partial response in distributed engine
- [#8181](https://github.com/thanos-io/thanos/pull/8181) Deps: bump promql engine

## New Contributors
* @didukh86 made their first contribution in https://github.com/thanos-io/thanos/pull/7674
* @bluesky6529 made their first contribution in https://github.com/thanos-io/thanos/pull/7942
* @SungJin1212 made their first contribution in https://github.com/thanos-io/thanos/pull/7962
* @rvichery made their first contribution in https://github.com/thanos-io/thanos/pull/7978
* @abelsimonn made their first contribution in https://github.com/thanos-io/thanos/pull/8003
* @cswpy made their first contribution in https://github.com/thanos-io/thanos/pull/8031
* @rofc made their first contribution in https://github.com/thanos-io/thanos/pull/8034
* @dsabsay made their first contribution in https://github.com/thanos-io/thanos/pull/8044
* @celian-garcia made their first contribution in https://github.com/thanos-io/thanos/pull/8091
* @psliwka made their first contribution in https://github.com/thanos-io/thanos/pull/8092
* @Saumya40-codes made their first contribution in https://github.com/thanos-io/thanos/pull/8105
* @martafolf made their first contribution in https://github.com/thanos-io/thanos/pull/8075
* @miinsun made their first contribution in https://github.com/thanos-io/thanos/pull/8120

**Full Changelog**: https://github.com/thanos-io/thanos/compare/v0.37.2...v0.38.0