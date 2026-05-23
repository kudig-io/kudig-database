---
title: thanos v0.19 Release Notes
description: thanos v0.19 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.19 Release Notes 是什么
- 如何 thanos v0.19 Release Notes
trigger_keywords:
- thanos
- v0.19
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

# [[Thanos|thanos]] v0.19 Release Notes

Source: [v0.19.0](https://github.com/thanos-io/thanos/releases/tag/v0.19.0)


### Added

- [#3700](https://github.com/thanos-io/thanos/pull/3700) Compact/Web: Make old bucket viewer UI work with vanilla [[Prometheus|Prometheus]] blocks.
- [#3657](https://github.com/thanos-io/thanos/pull/3657) *: It's now possible to configure HTTP transport options for S3 client.
- [#3752](https://github.com/thanos-io/thanos/pull/3752) Compact/Store: Added `--block-meta-fetch-concurrency` allowing to configure number of go routines for block metadata synchronization.
- [#3723](https://github.com/thanos-io/thanos/pull/3723) Query Frontend: Added `--query-range.request-downsampled` flag enabling additional queries for downsampled data in case of empty or incomplete response to range request.
- [#3579](https://github.com/thanos-io/thanos/pull/3579) Cache: Added inmemory cache for caching bucket.
- [#3792](https://github.com/thanos-io/thanos/pull/3792) Receiver: Added `--tsdb.allow-overlapping-blocks` flag to allow overlapping tsdb blocks and enable vertical compaction.
- [#3740](https://github.com/thanos-io/thanos/pull/3740) Query: Added `--query.default-step` flag to set default step. Useful when your tenant scrape interval is stable and far from default UI's 1s.
- [#3686](https://github.com/thanos-io/thanos/pull/3686) Query/Sidecar: Added metric metadata API support. You can now configure you Querier to fetch Prometheus metrics metadata from leaf Prometheus-es!
- [#3031](https://github.com/thanos-io/thanos/pull/3031) Compact/Sidecar/Receive/Rule: Added `--hash-func`. If some function has been specified, writers calculate hashes using that function of each file in a block before uploading them. If those hashes exist in the `meta.json` file then Compact does not download the files if they already exist on disk and with the same hash. This also means that the data directory passed to Thanos Compact is only *cleared once at boot* or *if everything succeeds*. So, if you, for example, use [[domain-17-system-foundation/topic-dictionary/storage/persistent-volumes|persistent volumes]]es（卷）|volumes]] on k8s and your Thanos Compact crashes or fails to make an iteration properly then the last downloaded files are not wiped from the disk. The directories that were created the last time are only wiped again after a successful iteration or if the previously picked up blocks have disappeared.

### Fixed

- [#3705](https://github.com/thanos-io/thanos/pull/3705) Store: Fix race condition leading to failing queries or possibly incorrect query results.
- [#3661](https://github.com/thanos-io/thanos/pull/3661) Compact: Deletion-mark.json is deleted as the last one, which could in theory lead to potential store gateway load or query error for such in-deletion block.
- [#3760](https://github.com/thanos-io/thanos/pull/3760) Store: Fix panic caused by a race condition happening on concurrent index-header reader usage and unload, when `--store.enable-index-header-lazy-reader` is enabled.
- [#3759](https://github.com/thanos-io/thanos/pull/3759) Store: Fix panic caused by a race condition happening on concurrent index-header lazy load and unload, when `--store.enable-index-header-lazy-reader` is enabled.
- [#3773](https://github.com/thanos-io/thanos/pull/3773) Compact: Fixed compaction planner size check, making sure we don't create too large blocks.
- [#3814](https://github.com/thanos-io/thanos/pull/3814) Store: Decreased memory utilisation while fetching block's chunks.
- [#3815](https://github.com/thanos-io/thanos/pull/3815) Receive: Improve handling of empty time series from clients
- [#3795](https://github.com/thanos-io/thanos/pull/3795) s3: A truncated "get object" response is reported as error.
- [#3899](https://github.com/thanos-io/thanos/pull/3899) Receive: Correct the inference of client gRPC configuration.
- [#3943](https://github.com/thanos-io/thanos/pull/3943) Receive: Fixed memory regression introduced in v0.17.0.
- [#3960](https://github.com/thanos-io/thanos/pull/3960) Query: Fixed deduplication of equal alerts with different labels.

### Changed

- [#3804](https://github.com/thanos-io/thanos/pull/3804) Ruler, Receive, Querier: Updated Prometheus dependency. TSDB characteristics might have changed.
