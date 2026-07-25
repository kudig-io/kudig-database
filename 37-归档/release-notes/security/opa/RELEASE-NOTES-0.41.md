---
title: thanos v0.41 Release Notes
description: thanos v0.41 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.41 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
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
- thanos v0.41 Release Notes 是什么
- 如何 thanos v0.41 Release Notes
trigger_keywords:
- thanos
- v0.41
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Thanos|thanos]] v0.41 Release Notes

Source: [v0.41.0](https://github.com/thanos-io/thanos/releases/tag/v0.41.0)

This is a good one, among several bug fixes, we have batched Series and Query RPCs now - this should lead to drastic improvements in network bandwidth and decent improvements in CPU/Memory usage of Querier and all its endpoints.


## Changes

### Fixed

- [#8378](https://github.com/thanos-io/thanos/pull/8378): Store: fix the reuse of dirty posting slices
- [#8558](https://github.com/thanos-io/thanos/pull/8558): Query-Frontend: Fix not logging requests when external-prefix is set in query
- [#8254](https://github.com/thanos-io/thanos/issues/8254) Receive: Endless loop of retried replication with capnproto and distributors
- [#8618](https://github.com/thanos-io/thanos/pull/8618): Query-Frontend: ensure slow query and stats logging on request failures
- [#8480](https://github.com/thanos-io/thanos/pull/8480): Store: fix(readerpool): avoid inserting nil LazyBinaryReader on error to avoid panic

### Added

- [#8623](https://github.com/thanos-io/thanos/pull/8623): Query: Enable batching of Series per SeriesResponse.
- [#8582](https://github.com/thanos-io/thanos/pull/8582): Sidecar: support --storage.tsdb.delay-compact-file.path [[Prometheus|Prometheus]] flag.
- [#8595](https://github.com/thanos-io/thanos/pull/8595): *: add --shipper.upload-compacted flag for controlling upload concurrency in components that use shippper

### Changed

- [#8630](https://github.com/thanos-io/thanos/pull/8630): Receive: *breaking :warning:* shuffle sharding now uses consistent hashing for stability on scale. Existing tenant-to-node assignments will change on upgrade.
- [#8555](https://github.com/thanos-io/thanos/pull/8555): Promu: re-add Darwin and FreeBSD as release platforms
- [#8661](https://github.com/thanos-io/thanos/pull/8661): Upgrade Prometheus to v3.8.0. `tsdb.enable-native-histograms` flag in Receiver and Ruler becomes a no-op and deprecated. Now native histogram ingestion is always enabled.

## New Contributors
* @hek2mgl made their first contribution in https://github.com/thanos-io/thanos/pull/8425
* @jrisebor made their first contribution in https://github.com/thanos-io/thanos/pull/8569
* @prymitive made their first contribution in https://github.com/thanos-io/thanos/pull/8582
* @abhiweye made their first contribution in https://github.com/thanos-io/thanos/pull/8592
* @TessaIO made their first contribution in https://github.com/thanos-io/thanos/pull/8556
* @dethi made their first contribution in https://github.com/thanos-io/thanos/pull/8622
* @adezxc made their first contribution in https://github.com/thanos-io/thanos/pull/8623
* @erlan-z made their first contribution in https://github.com/thanos-io/thanos/pull/8480
* @dowgird made their first contribution in https://github.com/thanos-io/thanos/pull/8595
* @pvlltvk made their first contribution in https://github.com/thanos-io/thanos/pull/8618

<!-- risk-assessed -->
