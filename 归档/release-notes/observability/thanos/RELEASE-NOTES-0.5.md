---
title: thanos v0.5 Release Notes
description: thanos v0.5 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
- minio
- gateway
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.5 Release Notes 是什么
- 如何 thanos v0.5 Release Notes
trigger_keywords:
- thanos
- v0.5
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




# [[Thanos|thanos]] v0.5 Release Notes

Source: [v0.5.0](https://github.com/thanos-io/thanos/releases/tag/v0.5.0)

TL;DR: Store LRU cache is no longer leaking, Upgraded Thanos UI to [[Prometheus|Prometheus]] 2.9, Fixed auto-downsampling, Moved to Go 1.12.5 and more.

This version moved tarballs to Golang 1.12.5 from 1.11 as well, so same warning applies if you use `container_memory_usage_bytes` from cadvisor. Use `container_memory_working_set_bytes` instead.

*breaking* As announced couple of times this release also removes gossip with all configuration flags (`--cluster.*`).

### Fixed

- [#1142](https://github.com/improbable-eng/thanos/pull/1142) fixed major leak on store LRU cache for index items (postings and series).
- [#1163](https://github.com/improbable-eng/thanos/pull/1163) sidecar is no longer blocking for custom Prometheus versions/builds. It only checks if flags return non 404, then it performs optional checks.
- [#1146](https://github.com/improbable-eng/thanos/pull/1146) store/bucket: make getFor() work with interleaved resolutions.
- [#1157](https://github.com/improbable-eng/thanos/pull/1157) querier correctly handles duplicated stores when some store changes external labels in place.

### Added

- [#1094](https://github.com/improbable-eng/thanos/pull/1094) Allow configuring the response header timeout for the S3 client.

### Changed

- [#1118](https://github.com/improbable-eng/thanos/pull/1118) *breaking* swift: Added support for cross-domain authentication by introducing `userDomainID`, `userDomainName`, `projectDomainID`, `projectDomainName`. 
  The outdated terms `tenantID`, `tenantName` are deprecated and have been replaced by `projectID`, `projectName`. 

- [#1066](https://github.com/improbable-eng/thanos/pull/1066) Upgrade Thanos ui to Prometheus v2.9.1.

  Changes from the upstream:
  * query:
    - [ENHANCEMENT] Update moment.js and moment-timezone.js [PR #4679](https://github.com/prometheus/prometheus/pull/4679)
    - [ENHANCEMENT] Support to query elements by a specific time [PR #4764](https://github.com/prometheus/prometheus/pull/4764)
    - [ENHANCEMENT] Update to Bootstrap 4.1.3 [PR #5192](https://github.com/prometheus/prometheus/pull/5192)
    - [BUGFIX] Limit number of merics in prometheus UI [PR #5139](https://github.com/prometheus/prometheus/pull/5139)
    - [BUGFIX] Web interface Quality of Life improvements [PR #5201](https://github.com/prometheus/prometheus/pull/5201)
  * rule:
    - [ENHANCEMENT] Improve rule views by wrapping lines [PR #4702](https://github.com/prometheus/prometheus/pull/4702)
    - [ENHANCEMENT] Show rule evaluation errors on rules page [PR #4457](https://github.com/prometheus/prometheus/pull/4457)
    
- [#1156](https://github.com/improbable-eng/thanos/pull/1156) Moved CI and docker multistage to Golang 1.12.5 for latest mem alloc improvements. 
- [#1103](https://github.com/improbable-eng/thanos/pull/1103) Updated go-cos deps. (COS bucket client).
- [#1149](https://github.com/improbable-eng/thanos/pull/1149) Updated google Golang API deps (GCS bucket client).
- [#1190](https://github.com/improbable-eng/thanos/pull/1190) Updated minio deps (S3 bucket client). This fixes minio retries.

- [#1133](https://github.com/improbable-eng/thanos/pull/1133) Use prometheus v2.9.2, common v0.4.0 & tsdb v0.8.0.

  Changes from the upstreams:
  * store gateway:
    - [ENHANCEMENT] Fast path for EmptyPostings cases in Merge, Intersect and Without.
  * store gateway & compactor:
    - [BUGFIX] Fix fd and vm_area leak on error path in chunks.NewDirReader.
    - [BUGFIX] Fix fd and vm_area leak on error path in index.NewFileReader.
  * query:
    - [BUGFIX] Make sure subquery range is taken into account for selection #5467
    - [ENHANCEMENT] Check for cancellation on every step of a range evaluation. #5131
    - [BUGFIX] Exponentation operator to drop metric name in result of operation. #5329
    - [BUGFIX] Fix output sample values for scalar-to-vector comparison operations. #5454
  * rule:
    - [BUGFIX] Reload rules: copy state on both name and labels. #5368

## Deprecated 

- [#1008](https://github.com/improbable-eng/thanos/pull/1008) *breaking* Removed Gossip implementation. All `--cluster.*` flags removed and Thanos will error out if any is provided.

See full [CHANGELOG here](/CHANGELOG.md)

<!-- risk-assessed -->
