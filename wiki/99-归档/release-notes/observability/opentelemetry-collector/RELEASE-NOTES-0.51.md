---
title: opentelemetry-collector v0.51 Release Notes
description: opentelemetry-collector v0.51 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.51 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.51 Release Notes 是什么
- 如何 opentelemetry-collector v0.51 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.51
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.51 Release Notes

Source: [v0.51.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.51.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.51.0

### 🛑 Breaking changes 🛑

- Remove deprecated model module, everything is available in `pdata` and `semconv`. (#5281)
  - Old versions of the module are still available, but no new versions will be released.
- Remove deprecated LogRecord.Name field. (#5202)

### 🚩 Deprecations 🚩

- In preparation of migration to immutable slices for primitive type items, the following methods are renamed (#5344)
  - `Value.BytesVal` func is deprecated in favor of `Value.MBytesVal`.
  - `Value.SetBytesVal` func is deprecated in favor of `Value.SetMBytesVal`.
  - `Value.UpdateBytes` func is deprecated in favor of `Value.UpdateMBytes`.
  - `Value.InsertBytes` func is deprecated in favor of `Value.InsertMBytes`.
  - `Value.UpsertBytes` func is deprecated in favor of `Value.UpsertMBytes`.
  - `<HistogramDataPoint|Buckets>.BucketCounts` funcs are deprecated in favor of
    `<HistogramDataPoint|Buckets>.MBucketCounts`.
  - `<HistogramDataPoint|Buckets>.SetBucketCounts` funcs are deprecated in favor of
    `<HistogramDataPoint|Buckets>.SetMBucketCounts`.
  - `HistogramDataPoint.ExplicitBounds` func is deprecated in favor of `HistogramDataPoint.MExplicitBounds`.
  - `HistogramDataPoint.SetExplicitBounds` func is deprecated in favor of `HistogramDataPoint.SetMExplicitBounds`.

### 💡 Enhancements 💡

- `pdata`: Expose `pcommon.NewSliceFromRaw` and `pcommon.Slice.AsRaw` functions (#5311)

### 🧰 Bug fixes 🧰

- Fix Windows Event Logs ignoring user-specified logging options (#5298)


<!-- risk-assessed -->
