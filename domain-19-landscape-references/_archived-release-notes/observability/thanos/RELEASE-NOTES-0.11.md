---
title: thanos v0.11 Release Notes
description: thanos v0.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- minio
- gateway
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.11 Release Notes 是什么
- 如何 thanos v0.11 Release Notes
trigger_keywords:
- thanos
- v0.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
created: "2026-05-23"
---

# [[Thanos|thanos]] v0.11 Release Notes

Source: [v0.11.0](https://github.com/thanos-io/thanos/releases/tag/v0.11.0)

### Fixed

- [#2033](https://github.com/thanos-io/thanos/pull/2033) Minio-go: Fixed Issue #1494 support Web Identity providers for IAM credentials for AWS EKS.
- [#1985](https://github.com/thanos-io/thanos/pull/1985) Store Gateway: Fixed case where series entry is larger than 64KB in index.
- [#2051](https://github.com/thanos-io/thanos/pull/2051) Ruler: Fixed issue where ruler does not expose shipper metrics.
- [#2101](https://github.com/thanos-io/thanos/pull/2101) Ruler: Fixed bug where thanos_alert_sender_errors_total was not registered.
- [#1789](https://github.com/thanos-io/thanos/pull/1789) Store Gateway: Improve timeouts.
- [#2139](https://github.com/thanos-io/thanos/pull/2139) Properly handle SIGHUP for reloading.
- [#2040](https://github.com/thanos-io/thanos/pull/2040) UI: Fix URL of alerts in Ruler
- [#2033](https://github.com/thanos-io/thanos/pull/1978) Ruler: Fix tracing in Thanos Ruler

### Added

- [#2003](https://github.com/thanos-io/thanos/pull/2003) Query: Support downsampling for /series.
- [#1952](https://github.com/thanos-io/thanos/pull/1952) Store Gateway: Implemented [binary index header](https://thanos.io/proposals/201912_thanos_binary_index_header.md/). This significantly reduces resource consumption (memory, CPU, net bandwidth) for startup and data loading processes as well as baseline memory. This means that adding more blocks into object storage, without querying them will use almost no resources. This, however, **still means that querying large amounts of data** will result in high spikes of memory and CPU use as before, due to simply fetching large amounts of metrics data. Since we fixed baseline, we are now focusing on query performance optimizations in separate initiatives. To enable experimental `index-header` mode run store with hidden `experimental.enable-index-header` flag.
- [#2009](https://github.com/thanos-io/thanos/pull/2009) Store Gateway: Minimum age of all blocks before they are being read. Set it to a safe value (e.g 30m) if your object storage is eventually consistent. GCS and S3 are (roughly) strongly consistent.
- [#1963](https://github.com/thanos-io/thanos/pull/1963) Mixin: Add Thanos Ruler alerts.
- [#1984](https://github.com/thanos-io/thanos/pull/1984) Query: Add cache-control header to not cache on error.
- [#1870](https://github.com/thanos-io/thanos/pull/1870) UI: Persist settings in query.
- [#1969](https://github.com/thanos-io/thanos/pull/1969) Sidecar: allow setting http connection pool size via flags.
- [#1967](https://github.com/thanos-io/thanos/issues/1967) Receive: Allow local TSDB compaction.
- [#1939](https://github.com/thanos-io/thanos/pull/1939) Ruler: Add TLS and authentication support for query endpoints with the `--query.config` and `--query.config-file` CLI flags. See [documentation](docs/components/rule.md/#configuration) for further information.
- [#1982](https://github.com/thanos-io/thanos/pull/1982) Ruler: Add support for Alertmanager v2 API endpoints.
- [#2030](https://github.com/thanos-io/thanos/pull/2030) Query: Add `thanos_proxy_store_empty_stream_responses_total` metric for number of empty responses from stores.
- [#2049](https://github.com/thanos-io/thanos/pull/2049) Tracing: Support sampling on Elastic APM with new sample_rate setting.
- [#2008](https://github.com/thanos-io/thanos/pull/2008) Querier, Receiver, Sidecar, Store: Add gRPC [health check](https://github.com/grpc/grpc/blob/master/doc/health-checking.md) endpoints.
- [#2145](https://github.com/thanos-io/thanos/pull/2145) Tracing: track query sent to prometheus via remote read api.

### Changed

- [#1970](https://github.com/thanos-io/thanos/issues/1970) *breaking* Receive: Use gRPC for forwarding requests between peers. Note that existing values for the `--receive.local-endpoint` flag and the endpoints in the hashring configuration file must now specify the receive gRPC port and must be updated to be a simple `host:port` combination, e.g. `127.0.0.1:10901`, rather than a full HTTP URL, e.g. `http://127.0.0.1:10902/api/v1/receive`.
- [#1933](https://github.com/thanos-io/thanos/pull/1933) Add a flag `--tsdb.wal-compression` to configure whether to enable tsdb wal compression in ruler and receiver.
- [#2021](https://github.com/thanos-io/thanos/pull/2021) Rename metric `thanos_query_duplicated_store_address` to `thanos_query_duplicated_store_addresses_total` and `thanos_rule_duplicated_query_address` to `thanos_rule_duplicated_query_addresses_total`.
