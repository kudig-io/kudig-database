# thanos v0.27 Release Notes

Source: [v0.27.0](https://github.com/thanos-io/thanos/releases/tag/v0.27.0)

## What's Changed

### Fixed

- [#5339](https://github.com/thanos-io/thanos/pull/5339) Receive: When running in routerOnly mode, an interupt (SIGINT) will now exit the process.
- [#5357](https://github.com/thanos-io/thanos/pull/5357) Store: Fix groupcache handling by making sure slashes in the cache's key are not getting interpreted by the router anymore.
- [#5427](https://github.com/thanos-io/thanos/pull/5427) Receive: Fix Ketama hashring replication consistency. With the Ketama hashring, replication is currently handled by choosing subsequent nodes in the list of endpoints. This can lead to existing nodes getting more series when the hashring is scaled. This change makes replication to choose subsequent nodes from the hashring which should not create new series in old nodes when the hashring is scaled. Ketama hashring can be used by setting `--receive.hashrings-algorithm=ketama`.

### Added

- [#5337](https://github.com/thanos-io/thanos/pull/5337) Thanos Object Store: Add the `prefix` option to buckets.
- [#5409](https://github.com/thanos-io/thanos/pull/5409) S3: Add option to force DNS style lookup.
- [#5352](https://github.com/thanos-io/thanos/pull/5352) Cache: Add cache metrics to groupcache: `thanos_cache_groupcache_bytes`, `thanos_cache_groupcache_evictions_total`, `thanos_cache_groupcache_items` and `thanos_cache_groupcache_max_bytes`.
- [#5391](https://github.com/thanos-io/thanos/pull/5391) Receive: Add relabeling support with the flag `--receive.relabel-config-file` or alternatively `--receive.relabel-config`.
- [#5408](https://github.com/thanos-io/thanos/pull/5408) Receive: Add support for consistent hashrings. The flag `--receive.hashrings-algorithm` uses default `hashmod` but can also be set to `ketama` to leverage consistent hashrings. More technical information can be found here: https://dgryski.medium.com/consistent-hashing-algorithmic-tradeoffs-ef6b8e2fcae8.
- [#5402](https://github.com/thanos-io/thanos/pull/5402) Receive: Implement api/v1/status/tsdb.

### Changed

- [#5410](https://github.com/thanos-io/thanos/pull/5410) Query: Close() after using query. This should reduce bumps in memory allocations.
- [#5417](https://github.com/thanos-io/thanos/pull/5417) Ruler: *Breaking if you have not set this value (`--eval-interval`) yourself and rely on that value. :warning:*. Change the default evaluation interval from 30s to 1 minute in order to be compliant with Prometheus alerting compliance specification: https://github.com/prometheus/compliance/blob/main/alert_generator/specification.md#executing-an-alerting-rule.

## New Contributors
* @jmjf made their first contribution in https://github.com/thanos-io/thanos/pull/5319
* @fgouteroux made their first contribution in https://github.com/thanos-io/thanos/pull/5339
* @heylongdacoder made their first contribution in https://github.com/thanos-io/thanos/pull/5324
* @bisakhmondal made their first contribution in https://github.com/thanos-io/thanos/pull/5239
* @djdongjin made their first contribution in https://github.com/thanos-io/thanos/pull/5383
* @nicolastakashi made their first contribution in https://github.com/thanos-io/thanos/pull/5387
* @roastiek made their first contribution in https://github.com/thanos-io/thanos/pull/5394
* @B0go made their first contribution in https://github.com/thanos-io/thanos/pull/5392
* @jademcosta made their first contribution in https://github.com/thanos-io/thanos/pull/5337
* @dudaduarte made their first contribution in https://github.com/thanos-io/thanos/pull/5337
* @Jakob3xD made their first contribution in https://github.com/thanos-io/thanos/pull/5409
* @4xoc made their first contribution in https://github.com/thanos-io/thanos/pull/5153

**Full Changelog**: https://github.com/thanos-io/thanos/compare/v0.26.0...v0.27.0