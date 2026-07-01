---
title: thanos v0.22 Release Notes
description: thanos v0.22 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.22 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.22 Release Notes 是什么
- 如何 thanos v0.22 Release Notes
trigger_keywords:
- thanos
- v0.22
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Thanos|thanos]] v0.22 Release Notes

Source: [v0.22.0](https://github.com/thanos-io/thanos/releases/tag/v0.22.0)

# Highlights

- Added offline deduplication that helps saving even more space on the remote object storage & increases query performance
- The [receive dual-mode](https://github.com/thanos-io/thanos/blob/release-0.22/docs/proposals-accepted/202012-receive-split.md) has been implemented
- Receive now is able to ingest exemplars and it is possible to query them
- Receive now uses a few percent less of CPU time with the same load
- Azure object storage has been refactored to support MSI authentication & it's now much more configurable

This release contains improvements from 28 authors (`git log --pretty="%ae" origin/release-0.21...origin/release-0.22  | sort | uniq | wc -l`)! Thanks to all of them! Thanos wouldn't be what it is without all of their effort and time! :heart:  

### Added

- [#4394](https://github.com/thanos-io/thanos/pull/4394) Add error logs to receiver when write request rejected with invalid replica
- [#4403](https://github.com/thanos-io/thanos/pull/4403) UI: Add sorting and filtering to flags page
- [#4299](https://github.com/thanos-io/thanos/pull/4299) Tracing: Add tracing to exemplar APIs.
- [#4327](https://github.com/thanos-io/thanos/pull/4327) Add environment variable substitution to all YAML configuration flags.
- [#4239](https://github.com/thanos-io/thanos/pull/4239) Add penalty based deduplication mode for compactor.
- [#4292](https://github.com/thanos-io/thanos/pull/4292) Receive: Enable exemplars ingestion and querying.
- [#4392](https://github.com/thanos-io/thanos/pull/4392) Tools: Added `--delete-blocks` to bucket rewrite tool to mark the original blocks for deletion after rewriting is done.
- [#3970](https://github.com/thanos-io/thanos/pull/3970) Azure: Adds more configuration options for Azure blob storage. This allows for pipeline and reader specific configuration. Implements HTTP transport configuration options. These options allows for more fine-grained control on timeouts and retries. Implements MSI authentication as second method of authentication via a service principal token.
- [#4406](https://github.com/thanos-io/thanos/pull/4406) Tools: Add retention command for applying retention policy on the bucket.
- [#4430](https://github.com/thanos-io/thanos/pull/4430) Compact: Add flag `downsample.concurrency` to specify the concurrency of downsampling blocks.
- [#4231](https://github.com/thanos-io/thanos/pull/4231) Receive: Implemented the [receive dual-mode](https://github.com/thanos-io/thanos/blob/release-0.22/docs/proposals-accepted/202012-receive-split.md). This means that it is now you can run receive as ingester (TSDB only, no hashring awareness), as router (no TSDB, only forwarding mode) and old mode (router + ingester) in one service. Consider splitting functionality if you want to scale your hashring into multiple rings or you want horizontally autoscale your ingester more often.
- [#3678](https://github.com/thanos-io/thanos/pull/3678) UI: add support for new duration format in graph range input. Now it is possible to use composite durations such as `5m30s`.

### Fixed

- [#4384](https://github.com/thanos-io/thanos/pull/4384) Fix the experimental PromQL editor when used on multiple line.
- [#4342](https://github.com/thanos-io/thanos/pull/4342) ThanosSidecarUnhealthy doesn't fire if the sidecar is never healthy
- [#4388](https://github.com/thanos-io/thanos/pull/4388) Receive: fix bug in forwarding remote-write requests within the hashring via gRPC when TLS is enabled on the HTTP server but not on the gRPC server.
- [#4340](https://github.com/thanos-io/thanos/pull/4340) UI: now displays the duration and all annotations of an alert in the alerts page.
- [#4348](https://github.com/thanos-io/thanos/pull/4348) Fixed parsing of the port in the log middleware.
- [#4417](https://github.com/thanos-io/thanos/pull/4417) UI: fixed the night mode in Bucket UI.
- [#4442](https://github.com/thanos-io/thanos/pull/4442) Ruler: fix SIGHUP reload signal not working.

### Changed

- [#4354](https://github.com/thanos-io/thanos/pull/4354) Receive: use the S2 library for decoding Snappy data; saves about 5-7% of CPU time in the Receive component when handling incoming remote write requests
- [#4369](https://github.com/thanos-io/thanos/pull/4354) Build: Pin upgrade Alpine's version.
- [#4404](https://github.com/thanos-io/thanos/pull/4404) Receive: added extra validation for the tenant's label name. Some unsupported formats could have passed before.