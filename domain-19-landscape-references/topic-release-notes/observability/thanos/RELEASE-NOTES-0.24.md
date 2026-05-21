---
title: thanos v0.24 Release Notes
description: thanos v0.24 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- grafana
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.24 Release Notes 是什么
- 如何 thanos v0.24 Release Notes
trigger_keywords:
- thanos
- v0.24
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
---

# thanos v0.24 Release Notes

Source: [v0.24.0](https://github.com/thanos-io/thanos/releases/tag/v0.24.0)

### Added

- [#4228](https://github.com/thanos-io/thanos/pull/4228) Tools `thanos bucket inspect`: Add flag `--output` to provide output method (table,csv,tsv).
- [#4282](https://github.com/thanos-io/thanos/pull/4282) Query: *breaking :warning:* Add `--endpoint` flag to the querier. The `--store` flag will eventually be replaced.
- [#4636](https://github.com/thanos-io/thanos/pull/4636) Azure: Support authentication using user-assigned managed identity
- [#4680](https://github.com/thanos-io/thanos/pull/4680) Query: Add `exemplar.partial-response` flag to control partial response.
- [#4679](https://github.com/thanos-io/thanos/pull/4679) Query: Add `enable-feature` flag to enable negative offsets and `@` modifier, similar to Prometheus.
- [#4696](https://github.com/thanos-io/thanos/pull/4696) Query: Add cache name to tracing spans.
- [#4710](https://github.com/thanos-io/thanos/pull/4710) Store: Add metric to capture timestamp of the last loaded block.
- [#4736](https://github.com/thanos-io/thanos/pull/4736) S3: Add capability to use custom AWS STS Endpoint.
- [#4764](https://github.com/thanos-io/thanos/pull/4764) Compact: Add `block-viewer.global.sync-block-timeout` flag to set the timeout of synchronization block metas.
- [#4801](https://github.com/thanos-io/thanos/pull/4801) Compact: Add Prometheus metrics for tracking the progress of compaction and downsampling.
- [#4444](https://github.com/thanos-io/thanos/pull/4444) UI: Add mark deletion and no compaction to the Block UI.
- [#4576](https://github.com/thanos-io/thanos/pull/4576) UI: Add filter compaction level to the Block UI.
- [#4731](https://github.com/thanos-io/thanos/pull/4731) Rule: Add stateless mode to ruler.
- [#4612](https://github.com/thanos-io/thanos/pull/4612) Sidecar: Add `--prometheus.http-client` and `--prometheus.http-client-file` flag for sidecar to connect to Prometheus with basic auth or TLS.
- [#4847](https://github.com/thanos-io/thanos/pull/4847) Query: Add `--alert.query-url` which is used in the UI for rules/alerts pages. By default the HTTP listen address is used for this URL.
- [#4848](https://github.com/thanos-io/thanos/pull/4848) Compactor: added Prometheus metric for tracking the progress of retention.
- [#4856](https://github.com/thanos-io/thanos/pull/4856) Mixin: Add Query Frontend to Grafana dashboard.
- [#4874](https://github.com/thanos-io/thanos/pull/4874) Query: Add `--endpoint-strict` flag to statically configure Thanos API server endpoints. It is similar to `--store-strict` but supports passing any Thanos gRPC APIs: StoreAPI, MetadataAPI, RulesAPI, TargetsAPI and ExemplarsAPI.
- [#4868](https://github.com/thanos-io/thanos/pull/4868) Rule: Support ruleGroup limit introduced by Prometheus v2.31.0.
- [#4897](https://github.com/thanos-io/thanos/pull/4897) Query: Add validation for querier address flags.

### Fixed

- [#4508](https://github.com/thanos-io/thanos/pull/4508) Sidecar, Mixin: Rename `ThanosSidecarUnhealthy` to `ThanosSidecarNoConnectionToStartedPrometheus`; Remove `ThanosSidecarPrometheusDown` alert; Remove unused `thanos_sidecar_last_heartbeat_success_time_seconds` metrics.
- [#4663](https://github.com/thanos-io/thanos/pull/4663) Fetcher: Fix discovered data races.
- [#4754](https://github.com/thanos-io/thanos/pull/4754) Query: Fix possible panic on stores endpoint.
- [#4753](https://github.com/thanos-io/thanos/pull/4753) Store: validate block sync concurrency parameter.
- [#4779](https://github.com/thanos-io/thanos/pull/4779) Examples: Fix the interactive test for MacOS users.
- [#4792](https://github.com/thanos-io/thanos/pull/4792) Store: Fix data race in BucketedBytes pool.
- [#4769](https://github.com/thanos-io/thanos/pull/4769) Query Frontend: Add "X-Request-ID" field and other fields to start call log.
- [#4709](https://github.com/thanos-io/thanos/pull/4709) Store: Fix panic when the application is stopped.
- [#4777](https://github.com/thanos-io/thanos/pull/4777) Query: Fix data race in exemplars server.
- [#4811](https://github.com/thanos-io/thanos/pull/4811) Query: Fix data race in metadata, rules, and targets servers.
- [#4795](https://github.com/thanos-io/thanos/pull/4795) Query: Fix deadlock in endpointset.
- [#4928](https://github.com/thanos-io/thanos/pull/4928) Azure: Only create an http client once, to conserve memory.
- [#4962](https://github.com/thanos-io/thanos/pull/4962) Compact/downsample: fix deadlock if error occurs with some backlog of blocks; fixes [this pull request](https://github.com/thanos-io/thanos/pull/4430). Affected versions are 0.22.0 - 0.23.1.

### Changed

- [#4864](https://github.com/thanos-io/thanos/pull/4864) UI: Remove the old PromQL editor.
- [#4708](https://github.com/thanos-io/thanos/pull/4708) Receive: Remove gRPC message size limit, which fixes errors commonly seen when receivers forward messages within a hashring.