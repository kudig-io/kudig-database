---
title: thanos v0.26 Release Notes
description: thanos v0.26 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.26 Release Notes — Kubernetes 生产运维知识库
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
- thanos v0.26 Release Notes 是什么
- 如何 thanos v0.26 Release Notes
trigger_keywords:
- thanos
- v0.26
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




# [[Thanos|thanos]] v0.26 Release Notes

Source: [v0.26.0](https://github.com/thanos-io/thanos/releases/tag/v0.26.0)

## What's Changed
### Fixed
- [#5281](https://github.com/thanos-io/thanos/pull/5281) Blocks: Use correct separators for filesystem paths and object storage paths respectively.
- [#5300](https://github.com/thanos-io/thanos/pull/5300) Query: Ignore cache on queries with deduplication off.

### Added

- [#5220](https://github.com/thanos-io/thanos/pull/5220) Query Frontend: Add `--query-frontend.forward-header` flag, forward headers to downstream querier.
- [#5250](https://github.com/thanos-io/thanos/pull/5250/files) Querier: Expose Query and QueryRange APIs through [[gRPC|GRPC]].
- [#5290](https://github.com/thanos-io/thanos/pull/5290) Add support for [ppc64le](https://en.wikipedia.org/wiki/Ppc64)

### Changed

- [#4838](https://github.com/thanos-io/thanos/pull/4838) Tracing: Chanced client for Stackdriver which deprecated "type: STACKDRIVER" in tracing YAML configuration. Use `type: GOOGLE_CLOUD` instead (`STACKDRIVER` type remains for backward compatibility).
- [#5170](https://github.com/thanos-io/thanos/pull/5170) All: Upgraded the TLS version from TLS1.2 to TLS1.3.
- [#5205](https://github.com/thanos-io/thanos/pull/5205) Rule: Add ruler labels as external labels in stateless ruler mode.
- [#5206](https://github.com/thanos-io/thanos/pull/5206) Cache: Add timeout for groupcache's fetch operation.
- [#5218](https://github.com/thanos-io/thanos/pull/5218) Tools: Thanos tools bucket downsample is now running continously.
- [#5231](https://github.com/thanos-io/thanos/pull/5231) Tools: Bucket verify tool ignores blocks with deletion markers.
- [#5244](https://github.com/thanos-io/thanos/pull/5244) Query: Promote negative offset and `@` modifier to stable features as per Prometheus [#10121](https://github.com/prometheus/prometheus/pull/10121).
- [#5255](https://github.com/thanos-io/thanos/pull/5255) InfoAPI: Set store API unavailable when stores are not ready.
- [#5256](https://github.com/thanos-io/thanos/pull/5256) Update Prometheus deps v2.33.5.
- [#5271](https://github.com/thanos-io/thanos/pull/5271) DNS: Fix miekgdns resolver to work with CNAME records too.

### Removed

- [#5145](https://github.com/thanos-io/thanos/pull/5145) UI: Remove old Prometheus UI.

## New Contributors
* @tomas-mota made their first contribution in https://github.com/thanos-io/thanos/pull/5202
* @appit-online made their first contribution in https://github.com/thanos-io/thanos/pull/5170
* @pablo-ruth made their first contribution in https://github.com/thanos-io/thanos/pull/5224
* @lcasi made their first contribution in https://github.com/thanos-io/thanos/pull/5220
* @dimitarvdimitrov made their first contribution in https://github.com/thanos-io/thanos/pull/5229
* @guilhermef made their first contribution in https://github.com/thanos-io/thanos/pull/5267
* @Zophar78 made their first contribution in https://github.com/thanos-io/thanos/pull/5273
* @jgbernalp made their first contribution in https://github.com/thanos-io/thanos/pull/5233
* @Ebaneck made their first contribution in https://github.com/thanos-io/thanos/pull/5289
* @mgiessing made their first contribution in https://github.com/thanos-io/thanos/pull/5290

**Full Changelog**: https://github.com/thanos-io/thanos/compare/v0.25.2...v0.26.0

<!-- risk-assessed -->
